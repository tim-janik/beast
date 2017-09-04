// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsebus.hh"
#include "bsecategories.hh"
#include "bsetrack.hh"
#include "bsesong.hh"
#include "bseengine.hh"
#include "bsecsynth.hh"
#include "bsesubiport.hh"
#include "bsesuboport.hh"
#include "bseproject.hh"
#include "bsestorage.hh"
#include "bsecxxplugin.hh"


#define parse_or_return         bse_storage_scanner_parse_or_return
#define peek_or_return          bse_storage_scanner_peek_or_return


/* --- parameters --- */
enum
{
  PROP_0,
  PROP_INPUTS,
  PROP_OUTPUTS,
  PROP_SNET,
  PROP_MUTE,
  PROP_SOLO,
  PROP_SYNC,
  PROP_LEFT_VOLUME,
  PROP_RIGHT_VOLUME,
  PROP_MASTER_OUTPUT,
};


/* --- prototypes --- */
static gboolean bse_bus_ensure_summation (BseBus *self);

/* --- variables --- */
static gpointer		 bus_parent_class = NULL;


/* --- functions --- */
static void
bse_bus_init (BseBus *self)
{
  BSE_OBJECT_SET_FLAGS (self, BSE_SOURCE_FLAG_PRIVATE_INPUTS);
  self->left_volume = 1.0;
  self->right_volume = 1.0;
  self->synced = TRUE;
  self->saved_sync = self->synced;
  bse_sub_synth_set_null_shortcut (BSE_SUB_SYNTH (self), TRUE);
}

static void
bse_bus_dispose (GObject *object)
{
  BseBus *self = BSE_BUS (object);
  while (self->inputs)
    bse_bus_disconnect (self, BSE_ITEM (self->inputs->data));
  /* chain parent class' handler */
  G_OBJECT_CLASS (bus_parent_class)->dispose (object);

  assert_return (self->bus_outputs == NULL);
}

static void
bse_bus_finalize (GObject *object)
{
  BseBus *self = BSE_BUS (object);
  assert_return (self->inputs == NULL);
  assert_return (self->bus_outputs == NULL);
  assert_return (self->summation == NULL);
  /* chain parent class' handler */
  G_OBJECT_CLASS (bus_parent_class)->finalize (object);
}

static BseBus*
get_master (BseBus *self)
{
  BseItem *parent = BSE_ITEM (self)->parent;
  if (BSE_IS_SONG (parent))
    {
      BseSong *song = BSE_SONG (parent);
      return bse_song_find_master (song);
    }
  return NULL;
}

static void
bus_list_input_candidates (BseBus     *self,
                           BseIt3mSeq *iseq)
{
  BseItem *item = BSE_ITEM (self);
  bse_item_gather_items_typed (item, iseq, BSE_TYPE_BUS, BSE_TYPE_SONG, FALSE);
  bse_item_gather_items_typed (item, iseq, BSE_TYPE_TRACK, BSE_TYPE_SONG, FALSE);
  BseBus *master = get_master (self);
  if (master)
    bse_it3m_seq_remove (iseq, BSE_ITEM (master));
}

void
bse_bus_or_track_list_output_candidates (BseItem    *trackbus,
                                         BseIt3mSeq *iseq)
{
  if (BSE_IS_BUS (trackbus) || BSE_IS_TRACK (trackbus))
    bse_item_gather_items_typed (trackbus, iseq, BSE_TYPE_BUS, BSE_TYPE_SONG, FALSE);
}

static void
bse_bus_get_candidates (BseItem               *item,
                        guint                  param_id,
                        BsePropertyCandidates *pc,
                        GParamSpec            *pspec)
{
  BseBus *self = BSE_BUS (item);
  switch (param_id)
    {
      SfiRing *ring;
    case PROP_INPUTS:
      bse_property_candidate_relabel (pc, _("Available Inputs"), _("List of available synthesis signals to be used as bus input"));
      bus_list_input_candidates (self, pc->items);
      /* remove existing inputs from candidates */
      ring = bse_bus_list_inputs (self);
      while (ring)
        bse_it3m_seq_remove (pc->items, (BseItem*) sfi_ring_pop_head (&ring));
      /* SYNC: type partitions */
      bse_type_seq_append (pc->partitions, "BseTrack");
      bse_type_seq_append (pc->partitions, "BseBus");
      break;
    case PROP_OUTPUTS:
      bse_property_candidate_relabel (pc, _("Available Outputs"), _("List of available mixer busses to be used as bus output"));
      bse_bus_or_track_list_output_candidates (BSE_ITEM (self), pc->items);
      /* remove existing outputs */
      ring = bse_bus_list_outputs (self);
      while (ring)
        bse_it3m_seq_remove (pc->items, (BseItem*) sfi_ring_pop_head (&ring));
      break;
    case PROP_SNET:
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static gboolean
bse_bus_editable_property (BseObject      *object,
                           guint           param_id,
                           GParamSpec     *pspec)
{
  BseBus *self = BSE_BUS (object);
  switch (param_id)
    {
      BseItem *parent;
    case PROP_OUTPUTS:
      parent = BSE_ITEM (self)->parent;
      if (BSE_IS_SONG (parent))
        {
          BseSong *song = BSE_SONG (parent);
          BseBus *master = bse_song_find_master (song);
          if (self == master)
            return FALSE;
        }
      break;
    }
  return TRUE;
}

static void
bus_disconnect_outputs (BseBus *self)
{
  SfiRing *ring, *outputs = bse_bus_list_outputs (self);
  for (ring = outputs; ring; ring = sfi_ring_walk (ring, outputs))
    {
      Bse::Error error = bse_bus_disconnect (BSE_BUS (ring->data), BSE_ITEM (self));
      if (error != 0)
        Bse::warning ("%s:%d: unexpected error: %s", __FILE__, __LINE__, bse_error_blurb (error));
    }
  bse_source_clear_ochannels (BSE_SOURCE (self));       /* also disconnects master */
  g_object_notify (G_OBJECT (self), "master-output");   /* master may have changed */
  g_object_notify (G_OBJECT (self), "solo");            /* master may have changed */
}

static void
song_connect_master (BseSong        *song,
                     BseBus         *bus)
{
  if (BSE_ITEM (bus)->parent == BSE_ITEM (song))
    {
      bse_source_clear_ichannels (song->postprocess);
      BseSource *osource = BSE_SOURCE (bus);
      bse_source_must_set_input (song->postprocess, 0, osource, 0);
      bse_source_must_set_input (song->postprocess, 1, osource, 1);
      g_object_notify (G_OBJECT (bus), "master-output");
      g_object_notify (G_OBJECT (bus), "solo");
    }
}

static gdouble
center_volume (gdouble volume1,
               gdouble volume2)
{
  if (volume1 > 0 && volume2 > 0)
    {
      /* center volumes in decibel */
      volume1 = bse_db_from_factor (volume1, -200);
      volume2 = bse_db_from_factor (volume2, -200);
      return bse_db_to_factor ((volume1 + volume2) * 0.5);
    }
  else
    return (volume1 + volume2) * 0.5;
}

static void
bus_volume_changed (BseBus *self)
{
  if (self->bmodule)
    {
      double v1, v2;
      if (self->muted || self->solo_muted)
        {
          v1 = 0;
          v2 = 0;
        }
      else
        {
          double lvolume = self->left_volume;
          double rvolume = self->right_volume;
          if (self->synced)
            lvolume = rvolume = center_volume (lvolume, rvolume);
          v1 = lvolume;
          v2 = rvolume;
        }
      g_object_set (self->bmodule, "volume1", v1, "volume2", v2, NULL);
    }
}

void
bse_bus_or_track_set_outputs (BseItem        *trackbus,
                              BseIt3mSeq     *outputs_iseq)
{
  SfiRing **pbus_outputs;
  /* handle object types */
  if (BSE_IS_BUS (trackbus))
    pbus_outputs = &BSE_BUS (trackbus)->bus_outputs;
  else if (BSE_IS_TRACK (trackbus))
    pbus_outputs = &BSE_TRACK (trackbus)->bus_outputs;
  else
    return;
  /* save user provided order */
  SfiRing *saved_outputs = bse_it3m_seq_to_ring (outputs_iseq);
  /* provide sorted rings: bus_outputs, outputs */
  SfiRing *outputs = sfi_ring_sort (sfi_ring_copy (saved_outputs), sfi_pointer_cmp, NULL);
  *pbus_outputs = sfi_ring_sort (*pbus_outputs, sfi_pointer_cmp, NULL);
  /* get all output candidates */
  BseIt3mSeq *iseq = bse_it3m_seq_new();
  bse_bus_or_track_list_output_candidates (trackbus, iseq);
  SfiRing *candidates = sfi_ring_sort (bse_it3m_seq_to_ring (iseq), sfi_pointer_cmp, NULL);
  bse_it3m_seq_free (iseq);
  /* constrain the new output list */
  SfiRing *ring = sfi_ring_intersection (outputs, candidates, sfi_pointer_cmp, NULL);
  sfi_ring_free (candidates);
  sfi_ring_free (outputs);
  outputs = ring;
  /* remove stale outputs */
  ring = sfi_ring_difference (*pbus_outputs, outputs, sfi_pointer_cmp, NULL);
  while (ring)
    bse_bus_disconnect ((BseBus*) sfi_ring_pop_head (&ring), trackbus);
  /* add new outputs */
  ring = sfi_ring_difference (outputs, *pbus_outputs, sfi_pointer_cmp, NULL);
  while (ring)
    bse_bus_connect_unchecked ((BseBus*) sfi_ring_pop_head (&ring), trackbus);
  sfi_ring_free (outputs);
  /* restore user provided order */
  *pbus_outputs = sfi_ring_reorder (*pbus_outputs, saved_outputs);
  sfi_ring_free (saved_outputs);
}

static void
bse_bus_set_property (GObject      *object,
                      guint         param_id,
                      const GValue *value,
                      GParamSpec   *pspec)
{
  BseBus *self = BSE_BUS (object);
  switch (param_id)
    {
      SfiRing *inputs, *candidates, *ring, *saved_inputs;
      BseIt3mSeq *iseq;
      BseItem *parent;
      gboolean vbool;
    case PROP_INPUTS:
      /* save user provided order */
      saved_inputs = bse_it3m_seq_to_ring ((BseIt3mSeq*) g_value_get_boxed (value));
      /* provide sorted rings: self->inputs, inputs */
      inputs = sfi_ring_sort (sfi_ring_copy (saved_inputs), sfi_pointer_cmp, NULL);
      self->inputs = sfi_ring_sort (self->inputs, sfi_pointer_cmp, NULL);
      /* get all input candidates */
      iseq = bse_it3m_seq_new();
      bus_list_input_candidates (self, iseq);
      candidates = sfi_ring_sort (bse_it3m_seq_to_ring (iseq), sfi_pointer_cmp, NULL);
      bse_it3m_seq_free (iseq);
      /* constrain the new input list */
      ring = sfi_ring_intersection (inputs, candidates, sfi_pointer_cmp, NULL);
      sfi_ring_free (candidates);
      sfi_ring_free (inputs);
      inputs = ring;
      /* remove stale inputs */
      ring = sfi_ring_difference (self->inputs, inputs, sfi_pointer_cmp, NULL);
      while (ring)
        bse_bus_disconnect (self, (BseItem*) sfi_ring_pop_head (&ring));
      /* add new inputs */
      ring = sfi_ring_difference (inputs, self->inputs, sfi_pointer_cmp, NULL);
      while (ring)
        bse_bus_connect_unchecked (self, (BseItem*) sfi_ring_pop_head (&ring));
      sfi_ring_free (inputs);
      /* restore user provided order */
      self->inputs = sfi_ring_reorder (self->inputs, saved_inputs);
      sfi_ring_free (saved_inputs);
      break;
    case PROP_OUTPUTS:
      bse_bus_or_track_set_outputs (BSE_ITEM (self), (BseIt3mSeq*) g_value_get_boxed (value));
      break;
    case PROP_SNET:
      g_object_set_property (G_OBJECT (self), "BseSubSynth::snet", value);
      break;
    case PROP_MUTE:
      self->muted = sfi_value_get_bool (value);
      bus_volume_changed (self);
      break;
    case PROP_SOLO:
      parent = BSE_ITEM (self)->parent;
      if (BSE_IS_SONG (parent))
        {
          BseSong *song = BSE_SONG (parent);
          gboolean is_solo = sfi_value_get_bool (value);
          if (is_solo && song->solo_bus != self)
            bse_song_set_solo_bus (song, self);
          else if (!is_solo && song->solo_bus == self)
            bse_song_set_solo_bus (song, NULL);
        }
      break;
    case PROP_SYNC:
      vbool = sfi_value_get_bool (value);
      if (vbool != self->synced)
        {
          self->synced = vbool;
          if (self->synced)
            self->right_volume = self->left_volume = center_volume (self->right_volume, self->left_volume);
          bus_volume_changed (self);
          g_object_notify (G_OBJECT (self), "left-volume");
          g_object_notify (G_OBJECT (self), "right-volume");
        }
      self->saved_sync = self->synced;
      break;
    case PROP_LEFT_VOLUME:
      self->left_volume = sfi_value_get_real (value);
      if (self->synced)
        {
          self->right_volume = self->left_volume;
          g_object_notify (G_OBJECT (self), "right-volume");
        }
      bus_volume_changed (self);
      break;
    case PROP_RIGHT_VOLUME:
      self->right_volume = sfi_value_get_real (value);
      if (self->synced)
        {
          self->left_volume = self->right_volume;
          g_object_notify (G_OBJECT (self), "left-volume");
        }
      bus_volume_changed (self);
      break;
    case PROP_MASTER_OUTPUT:
      parent = BSE_ITEM (self)->parent;
      if (BSE_IS_SONG (parent))
        {
          BseSong *song = BSE_SONG (parent);
          BseBus *master = bse_song_find_master (song);
          if (sfi_value_get_bool (value))
            {
              if (master != self)
                {
                  if (master)
                    bus_disconnect_outputs (master);
                  bus_disconnect_outputs (self);
                  song_connect_master (song, self);
                }
            }
          else
            {
              if (master == self)
                bus_disconnect_outputs (self);
            }
        }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
bse_bus_get_property (GObject    *object,
                      guint       param_id,
                      GValue     *value,
                      GParamSpec *pspec)
{
  BseBus *self = BSE_BUS (object);
  switch (param_id)
    {
      BseItem *parent;
      BseIt3mSeq *iseq;
      SfiRing *ring;
    case PROP_INPUTS:
      iseq = bse_it3m_seq_new();
      ring = bse_bus_list_inputs (self);
      while (ring)
        bse_it3m_seq_append (iseq, (BseItem*) sfi_ring_pop_head (&ring));
      g_value_take_boxed (value, iseq);
      break;
    case PROP_OUTPUTS:
      iseq = bse_it3m_seq_new();
      ring = bse_bus_list_outputs (self);
      while (ring)
        bse_it3m_seq_append (iseq, (BseItem*) sfi_ring_pop_head (&ring));
      if (!ring && get_master (self) == self)
        bse_it3m_seq_append (iseq, BSE_ITEM (self)->parent);    /* requires proxy_notifies on parent */
      g_value_take_boxed (value, iseq);
      break;
    case PROP_SNET:
      g_object_get_property (G_OBJECT (self), "BseSubSynth::snet", value);
      break;
    case PROP_MUTE:
      g_value_set_boolean (value, self->muted);
      break;
    case PROP_SOLO:
      parent = BSE_ITEM (self)->parent;
      if (BSE_IS_SONG (parent))
        {
          BseSong *song = BSE_SONG (parent);
          g_value_set_boolean (value, song->solo_bus == self);
        }
      else
        g_value_set_boolean (value, FALSE);
      break;
    case PROP_SYNC:
      g_value_set_boolean (value, self->synced);
      break;
    case PROP_LEFT_VOLUME:
      sfi_value_set_real (value, self->synced ? center_volume (self->left_volume, self->right_volume) : self->left_volume);
      break;
    case PROP_RIGHT_VOLUME:
      sfi_value_set_real (value, self->synced ? center_volume (self->left_volume, self->right_volume) : self->right_volume);
      break;
    case PROP_MASTER_OUTPUT:
      parent = BSE_ITEM (self)->parent;
      if (BSE_IS_SONG (parent))
        {
          BseSong *song = BSE_SONG (parent);
          BseBus *master = bse_song_find_master (song);
          sfi_value_set_bool (value, self == master);
        }
      else
        sfi_value_set_bool (value, FALSE);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

void
bse_bus_change_solo (BseBus         *self,
                     gboolean        solo_muted)
{
  self->solo_muted = solo_muted;
  bus_volume_changed (self);
  g_object_notify (G_OBJECT (self), "solo");
  g_object_notify (G_OBJECT (self), "mute");
}

static void
bse_bus_set_parent (BseItem *item,
                    BseItem *parent)
{
  BseBus *self = BSE_BUS (item);
  self->solo_muted = FALSE;

  if (item->parent)
    bse_object_unproxy_notifies (item->parent, self, "notify::outputs");

  /* chain parent class' handler */
  BSE_ITEM_CLASS (bus_parent_class)->set_parent (item, parent);

  if (item->parent)
    bse_object_proxy_notifies (item->parent, self, "notify::outputs");

  while (self->inputs)
    bse_bus_disconnect (self, BSE_ITEM (self->inputs->data));

  if (self->summation)
    {
      BseItem *sitem = BSE_ITEM (self->summation);
      self->summation = NULL;
      BseContainer *container = BSE_CONTAINER (sitem->parent);
      bse_container_remove_item (container, sitem);
    }
  if (BSE_SUB_SYNTH (self)->snet)
    {
      /* there should be snet=NULL if we have not yet a parent, and
       * snet should be set to NULL due to uncrossing before we are orphaned
       */
      Bse::warning ("Bus[%p] has snet[%p] in set-parent", self, BSE_SUB_SYNTH (self)->snet);
    }
}

static void
bse_bus_prepare (BseSource *source)
{
  // BseBus *iput = BSE_BUS (source);
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (bus_parent_class)->prepare (source);
}

static void
bse_bus_context_create (BseSource *source,
                        guint      context_handle,
                        BseTrans  *trans)
{
  // BseBus *iput = BSE_BUS (source);
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (bus_parent_class)->context_create (source, context_handle, trans);
}

static void
bse_bus_context_connect (BseSource *source,
                         guint      context_handle,
                         BseTrans  *trans)
{
  // BseBus *iput = BSE_BUS (source);
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (bus_parent_class)->context_connect (source, context_handle, trans);
}

static void
bse_bus_reset (BseSource *source)
{
  // BseBus *iput = BSE_BUS (source);
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (bus_parent_class)->reset (source);
}

gboolean
bse_bus_get_stack (BseBus        *self,
                   BseContainer **snetp,
                   BseSource    **vinp,
                   BseSource    **voutp)
{
  BseItem *item = BSE_ITEM (self);
  BseProject *project = bse_item_get_project (item);
  if (!BSE_SUB_SYNTH (self)->snet && project && BSE_IS_SONG (item->parent))
    {
      assert_return (self->n_effects == 0, FALSE);
      bse_bus_ensure_summation (self);
      BseSNet *snet = (BseSNet*) bse_project_create_intern_csynth (project, "%BusEffectStack");
      self->vin = (BseSource*) bse_container_new_child_bname (BSE_CONTAINER (snet), BSE_TYPE_SUB_IPORT, "%VInput", NULL);
      bse_snet_intern_child (snet, self->vin);
      BseSource *vout = (BseSource*) bse_container_new_child_bname (BSE_CONTAINER (snet), BSE_TYPE_SUB_OPORT, "%VOutput", NULL);
      bse_snet_intern_child (snet, vout);
      self->bmodule = (BseSource*) bse_container_new_child_bname (BSE_CONTAINER (snet), g_type_from_name ("BseBusModule"), "%Volume", NULL);
      bse_snet_intern_child (snet, self->bmodule);
      g_object_set (self->bmodule,
                    "volume1", self->left_volume,
                    "volume2", self->right_volume,
                    NULL);
      bse_source_must_set_input (vout, 0, self->bmodule, 0);
      bse_source_must_set_input (vout, 1, self->bmodule, 1);
      g_object_set (self, "BseSubSynth::snet", snet, NULL); /* no undo */
      /* connect empty effect stack */
      bse_source_must_set_input (self->bmodule, 0, self->vin, 0);
      bse_source_must_set_input (self->bmodule, 1, self->vin, 1);
    }
  if (BSE_SUB_SYNTH (self)->snet)
    {
      if (snetp)
        *snetp = (BseContainer*) BSE_SUB_SYNTH (self)->snet;
      if (vinp)
        *vinp = self->vin;
      if (voutp)
        *voutp = self->bmodule;
      return TRUE;
    }
  return FALSE;
}

static gboolean
bse_bus_ensure_summation (BseBus *self)
{
  if (!self->summation)
    {
      BseItem *item = BSE_ITEM (self);
      if (BSE_IS_SONG (item->parent))
        self->summation = bse_song_create_summation (BSE_SONG (item->parent));
      if (self->summation)
        {
          bse_source_must_set_input (BSE_SOURCE (self), BSE_BUS_OCHANNEL_LEFT,
                                     self->summation, bse_source_find_ochannel (self->summation, "audio-out1"));
          bse_source_must_set_input (BSE_SOURCE (self), BSE_BUS_OCHANNEL_RIGHT,
                                     self->summation, bse_source_find_ochannel (self->summation, "audio-out2"));
        }
    }
  return self->summation != NULL;
}

static void
trackbus_update_outputs (BseItem *trackbus,
                         BseBus  *added,
                         BseBus  *removed)
{
  SfiRing *outputs = BSE_IS_TRACK (trackbus) ? BSE_TRACK (trackbus)->bus_outputs : BSE_BUS (trackbus)->bus_outputs;
  if (removed)
    outputs = sfi_ring_remove (outputs, removed);
  if (added)
    outputs = sfi_ring_append (outputs, added);
  if (BSE_IS_TRACK (trackbus))
    BSE_TRACK (trackbus)->bus_outputs = outputs;
  else
    BSE_BUS (trackbus)->bus_outputs = outputs;
}

static void
bus_uncross_input (BseItem *owner,
                   BseItem *item)
{
  // delete item via undoable method for undo-rcording
  if (BSE_IS_TRACK (item))
    {
      Bse::BusImpl &self = *owner->as<Bse::BusImpl*>();
      self.disconnect_track (*item->as<Bse::TrackImpl*>());
    }
  else /* IS_BUS */
    {
      Bse::BusImpl &self = *owner->as<Bse::BusImpl*>();
      Bse::BusImpl &bus = *item->as<Bse::BusImpl*>();
      self.disconnect_bus (bus);
    }
}

Bse::Error
bse_bus_connect (BseBus  *self,
                 BseItem *trackbus)
{
  /* get all input candidates */
  BseIt3mSeq *iseq = bse_it3m_seq_new();
  bus_list_input_candidates (self, iseq);
  /* find trackbus */
  gboolean found_candidate = FALSE;
  guint i;
  for (i = 0; i < iseq->n_items; i++)
    if (iseq->items[i] == trackbus)
      {
        found_candidate = TRUE;
        break;
      }
  bse_it3m_seq_free (iseq);
  /* add trackbus if valid */
  if (found_candidate)
    return bse_bus_connect_unchecked (self, trackbus);
  else
    return Bse::Error::SOURCE_CONNECTION_INVALID;
}

Bse::Error
bse_bus_connect_unchecked (BseBus  *self,
                           BseItem *trackbus)
{
  BseSource *osource;
  if (BSE_IS_TRACK (trackbus))
    osource = bse_track_get_output (BSE_TRACK (trackbus));
  else if (BSE_IS_BUS (trackbus))
    osource = BSE_SOURCE (trackbus);
  else
    return Bse::Error::SOURCE_TYPE_INVALID;
  if (!osource || !bse_bus_ensure_summation (self) ||
      BSE_ITEM (osource)->parent != BSE_ITEM (self)->parent)    /* restrict to siblings */
    return Bse::Error::SOURCE_PARENT_MISMATCH;
  Bse::Error error = bse_source_set_input (self->summation, 0, osource, 0);
  if (error == 0)
    {
      bse_source_must_set_input (self->summation, 1, osource, 1);
      self->inputs = sfi_ring_append (self->inputs, trackbus);
      trackbus_update_outputs (trackbus, self, NULL);
      bse_object_proxy_notifies (trackbus, self, "notify::inputs");
      bse_object_proxy_notifies (self, trackbus, "notify::outputs");
      bse_item_cross_link (BSE_ITEM (self), BSE_ITEM (trackbus), bus_uncross_input);
      g_object_notify (G_OBJECT (self), "inputs");
      g_object_notify (G_OBJECT (trackbus), "outputs");
    }
  return error;
}

Bse::Error
bse_bus_disconnect (BseBus  *self,
                    BseItem *trackbus)
{
  BseSource *osource;
  if (BSE_IS_TRACK (trackbus))
    osource = bse_track_get_output (BSE_TRACK (trackbus));
  else if (BSE_IS_BUS (trackbus))
    osource = BSE_SOURCE (trackbus);
  else
    return Bse::Error::SOURCE_TYPE_INVALID;
  if (!osource || !self->summation || !sfi_ring_find (self->inputs, trackbus))
    return Bse::Error::SOURCE_PARENT_MISMATCH;
  bse_object_unproxy_notifies (trackbus, self, "notify::inputs");
  bse_object_unproxy_notifies (self, trackbus, "notify::outputs");
  bse_item_cross_unlink (BSE_ITEM (self), BSE_ITEM (trackbus), bus_uncross_input);
  self->inputs = sfi_ring_remove (self->inputs, trackbus);
  trackbus_update_outputs (trackbus, NULL, self);
  Bse::Error error1 = bse_source_unset_input (self->summation, 0, osource, 0);
  Bse::Error error2 = bse_source_unset_input (self->summation, 1, osource, 1);
  g_object_notify (G_OBJECT (self), "inputs");
  g_object_notify (G_OBJECT (trackbus), "outputs");
  return error1 != 0 ? error1 : error2;
}

SfiRing*
bse_bus_list_inputs (BseBus *self)
{
  return sfi_ring_copy (self->inputs);
}

SfiRing*
bse_bus_list_outputs (BseBus *self)
{
  return sfi_ring_copy (self->bus_outputs);
}

static void
bus_restore_add_input (gpointer     data,
                       BseStorage  *storage,
                       BseItem     *from_item,
                       BseItem     *to_item,
                       const gchar *error)
{
  BseBus *self = BSE_BUS (from_item);
  BseSource *osource = to_item ? BSE_SOURCE (to_item) : NULL;

  if (error)
    bse_storage_warn (storage, "failed to add input to mixer bus \"%s\": %s", BSE_OBJECT_UNAME (self), error);
  else
    {
      Bse::Error cerror;
      if (osource)
        cerror = bse_bus_connect (self, BSE_ITEM (osource));
      else
        cerror = Bse::Error::SOURCE_NO_SUCH_MODULE;
      if (cerror != 0)
        bse_storage_warn (storage,
                          "failed to add input \"%s\" to mixer bus \"%s\": %s",
                          osource ? BSE_OBJECT_UNAME (osource) : ":<NULL>:",
                          BSE_OBJECT_UNAME (self),
                          bse_error_blurb (cerror));
    }
}

static void
bus_restore_start (BseObject  *object,
                   BseStorage *storage)
{
  BseBus *self = BSE_BUS (object);
  self->saved_sync = self->synced;
  /* support seperate left & right volumes */
  self->synced = FALSE;
  BSE_OBJECT_CLASS (bus_parent_class)->restore_start (object, storage);
}

static GTokenType
bus_restore_private (BseObject  *object,
                     BseStorage *storage,
                     GScanner   *scanner)
{
  BseBus *self = BSE_BUS (object);

  if (g_scanner_peek_next_token (scanner) == G_TOKEN_IDENTIFIER &&
      bse_string_equals ("bus-input", scanner->next_value.v_identifier))
    {
      parse_or_return (scanner, G_TOKEN_IDENTIFIER);    /* eat identifier */
      /* parse osource upath and queue handler */
      GTokenType token = bse_storage_parse_item_link (storage, BSE_ITEM (self), bus_restore_add_input, NULL);
      if (token != G_TOKEN_NONE)
        return token;
      /* close statement */
      parse_or_return (scanner, ')');
      return G_TOKEN_NONE;
    }
  else /* chain parent class' handler */
    return BSE_OBJECT_CLASS (bus_parent_class)->restore_private (object, storage, scanner);
}

static void
bus_restore_finish (BseObject *object,
                    guint      vmajor,
                    guint      vminor,
                    guint      vmicro)
{
  BseBus *self = BSE_BUS (object);
  /* restore real sync setting */
  g_object_set (self, /* no undo */
                "sync", self->saved_sync,
                NULL);
  BSE_OBJECT_CLASS (bus_parent_class)->restore_finish (object, vmajor, vminor, vmicro);
}

static void
bus_store_private (BseObject  *object,
                   BseStorage *storage)
{
  BseBus *self = BSE_BUS (object);

  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (bus_parent_class)->store_private)
    BSE_OBJECT_CLASS (bus_parent_class)->store_private (object, storage);

  SfiRing *inputs = bse_bus_list_inputs (self);
  while (inputs)
    {
      BseSource *osource = (BseSource*) sfi_ring_pop_head (&inputs);
      bse_storage_break (storage);
      bse_storage_printf (storage, "(bus-input ");
      bse_storage_put_item_link (storage, BSE_ITEM (self), BSE_ITEM (osource));
      bse_storage_printf (storage, ")");
    }
}

static void
bse_bus_class_init (BseBusClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (klass);
  BseItemClass *item_class = BSE_ITEM_CLASS (klass);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (klass);
  guint channel_id;

  bus_parent_class = g_type_class_peek_parent (klass);

  gobject_class->set_property = bse_bus_set_property;
  gobject_class->get_property = bse_bus_get_property;
  gobject_class->dispose = bse_bus_dispose;
  gobject_class->finalize = bse_bus_finalize;

  object_class->editable_property = bse_bus_editable_property;
  object_class->store_private = bus_store_private;
  object_class->restore_start = bus_restore_start;
  object_class->restore_private = bus_restore_private;
  object_class->restore_finish = bus_restore_finish;

  item_class->set_parent = bse_bus_set_parent;
  item_class->get_candidates = bse_bus_get_candidates;

  source_class->prepare = bse_bus_prepare;
  source_class->context_create = bse_bus_context_create;
  source_class->context_connect = bse_bus_context_connect;
  source_class->reset = bse_bus_reset;

  bse_object_class_add_param (object_class, _("Adjustments"), PROP_MUTE,
                              sfi_pspec_bool ("mute", _("Mute"), _("Mute: turn off the bus volume"), FALSE, SFI_PARAM_STANDARD ":skip-default"));
  bse_object_class_add_param (object_class, _("Adjustments"), PROP_SOLO,
                              sfi_pspec_bool ("solo", _("Solo"), _("Solo: mute all other busses"), FALSE, SFI_PARAM_STANDARD ":skip-default"));
  bse_object_class_add_param (object_class, _("Adjustments"), PROP_SYNC,
                              sfi_pspec_bool ("sync", _("Sync"), _("Syncronize left and right volume"), TRUE, SFI_PARAM_STANDARD ":skip-default"));
  bse_object_class_add_param (object_class, _("Adjustments"), PROP_LEFT_VOLUME,
			      sfi_pspec_real ("left-volume", _("Left Volume"), _("Volume adjustment in decibel of left bus channel"),
                                              bse_db_to_factor (0), bse_db_to_factor (BSE_MINDB), bse_db_to_factor (+24),
                                              bse_db_to_factor (0.1), SFI_PARAM_STANDARD ":scale:db-volume"));
  bse_object_class_add_param (object_class, _("Adjustments"), PROP_RIGHT_VOLUME,
			      sfi_pspec_real ("right-volume", _("Right Volume"), _("Volume adjustment in decibel of right bus channel"),
                                              bse_db_to_factor (0), bse_db_to_factor (BSE_MINDB), bse_db_to_factor (+24),
                                              bse_db_to_factor (0.1), SFI_PARAM_STANDARD ":scale:db-volume"));
  bse_object_class_add_param (object_class, _("Signal Inputs"),
                              PROP_INPUTS,
                              /* SYNC: type partitions determine the order of displayed objects */
                              bse_param_spec_boxed ("inputs", _("Input Signals"),
                                                    /* TRANSLATORS: the "tracks and busses" order in this tooltip needs
                                                     * to be preserved to match the GUI order of displayed objects.
                                                     */
                                                    _("Synthesis signals (from tracks and busses) used as bus input"),
                                                    BSE_TYPE_IT3M_SEQ, SFI_PARAM_GUI ":item-sequence"));
  bse_object_class_add_param (object_class, _("Signal Outputs"),
                              PROP_OUTPUTS,
                              bse_param_spec_boxed ("outputs", _("Output Signals"),
                                                    _("Mixer busses used as output for synthesis signals"),
                                                    BSE_TYPE_IT3M_SEQ, SFI_PARAM_GUI ":item-sequence"));
  bse_object_class_add_param (object_class, NULL, PROP_SNET, bse_param_spec_object ("snet", NULL, NULL, BSE_TYPE_CSYNTH, SFI_PARAM_READWRITE ":skip-undo"));
  bse_object_class_add_param (object_class, _("Internals"),
			      PROP_MASTER_OUTPUT,
			      sfi_pspec_bool ("master-output", _("Master Output"), NULL,
                                              FALSE, SFI_PARAM_STORAGE ":skip-default"));

  channel_id = bse_source_class_add_ichannel (source_class, "left-audio-in", _("Left Audio In"), _("Left channel input"));
  assert_return (channel_id == BSE_BUS_ICHANNEL_LEFT);
  channel_id = bse_source_class_add_ichannel (source_class, "right-audio-in", _("Right Audio In"), _("Right channel input"));
  assert_return (channel_id == BSE_BUS_ICHANNEL_RIGHT);
  channel_id = bse_source_class_add_ochannel (source_class, "left-audio-out", _("Left Audio Out"), _("Left channel output"));
  assert_return (channel_id == BSE_BUS_OCHANNEL_LEFT);
  channel_id = bse_source_class_add_ochannel (source_class, "right-audio-out", _("Right Audio Out"), _("Right channel output"));
  assert_return (channel_id == BSE_BUS_OCHANNEL_RIGHT);
}

BSE_BUILTIN_TYPE (BseBus)
{
  static const GTypeInfo bus_info = {
    sizeof (BseBusClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_bus_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    sizeof (BseBus),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_bus_init,
  };
  GType type = bse_type_register_static (BSE_TYPE_SUB_SYNTH,
                                         "BseBus",
                                         _("Bus implementation for songs, used to route track audio signals "
                                           "to the master output."),
                                         __FILE__, __LINE__,
                                         &bus_info);
  return type;
}

namespace Bse {

BusImpl::BusImpl (BseObject *bobj) :
  SubSynthImpl (bobj)
{}

BusImpl::~BusImpl ()
{}

Error
BusImpl::ensure_output ()
{
  BseBus *self = as<BseBus*>();
  Error error = Error::NONE;
  BseItem *parent = self->parent;
  if (BSE_IS_SONG (parent) && !self->bus_outputs)
    {
      BseSong *song = BSE_SONG (parent);
      BseBus *masterp = bse_song_find_master (song);
      if (masterp && self != masterp)
        {
          BusImpl &master = *masterp->as<BusImpl*>();
          error = master.connect_bus (*this);
        }
    }
  return error;
}

Error
BusImpl::connect_bus (BusIface &busi)
{
  BseBus *self = as<BseBus*>();
  BusImpl &bus = dynamic_cast<BusImpl&> (busi);
  if (!this->parent() || this->parent() != bus.parent())
    return Error::SOURCE_PARENT_MISMATCH;

  Error error = bse_bus_connect (self, bus.as<BseItem*>());
  if (error == 0)
    {
      // an undo lambda is needed for wrapping object argument references
      UndoDescriptor<BusImpl> bus_descriptor = undo_descriptor (bus);
      auto lambda = [bus_descriptor] (BusImpl &self, BseUndoStack *ustack) -> Error {
        return self.disconnect_bus (self.undo_resolve (bus_descriptor));
      };
      push_undo (__func__, *this, lambda);
    }
  return error;
}

Error
BusImpl::connect_track (TrackIface &tracki)
{
  BseBus *self = as<BseBus*>();
  TrackImpl &track = dynamic_cast<TrackImpl&> (tracki);
  if (!this->parent() || this->parent() != track.parent())
    return Error::SOURCE_PARENT_MISMATCH;

  Error error = bse_bus_connect (self, track.as<BseItem*>());
  if (error == 0)
    {
      // an undo lambda is needed for wrapping object argument references
      UndoDescriptor<TrackImpl> track_descriptor = undo_descriptor (track);
      auto lambda = [track_descriptor] (BusImpl &self, BseUndoStack *ustack) -> Error {
        return self.disconnect_track (self.undo_resolve (track_descriptor));
      };
      push_undo (__func__, *this, lambda);
    }
  return error;
}

Error
BusImpl::disconnect_bus (BusIface &busi)
{
  BseBus *self = as<BseBus*>();
  BusImpl &bus = dynamic_cast<BusImpl&> (busi);
  Error error = bse_bus_disconnect (self, busi.as<BseItem*>());
  if (error == 0)
    {
      // an undo lambda is needed for wrapping object argument references
      UndoDescriptor<BusImpl> bus_descriptor = undo_descriptor (bus);
      auto lambda = [bus_descriptor] (BusImpl &self, BseUndoStack *ustack) -> Error {
        return self.connect_bus (self.undo_resolve (bus_descriptor));
      };
      push_undo (__func__, *this, lambda);
    }
  return error;
}

Error
BusImpl::disconnect_track (TrackIface &tracki)
{
  BseBus *self = as<BseBus*>();
  TrackImpl &track = dynamic_cast<TrackImpl&> (tracki);
  Error error = bse_bus_disconnect (self, tracki.as<BseItem*>());
  if (error == 0)
    {
      // an undo lambda is needed for wrapping object argument references
      UndoDescriptor<TrackImpl> track_descriptor = undo_descriptor (track);
      auto lambda = [track_descriptor] (BusImpl &self, BseUndoStack *ustack) -> Error {
        return self.connect_track (self.undo_resolve (track_descriptor));
      };
      push_undo (__func__, *this, lambda);
    }
  return error;
}

} // Bse
