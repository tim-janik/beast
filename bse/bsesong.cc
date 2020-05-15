// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsesong.hh"
#include "bsemathsignal.hh"
#include "bsetrack.hh"
#include "bsepart.hh"
#include "bsebus.hh"
#include "bsepcmoutput.hh"
#include "bseproject.hh"
#include "bsemidireceiver.hh"
#include "bsestorage.hh"
#include "bsemain.hh"
#include "bsecsynth.hh"
#include "bsesequencer.hh"
#include "bsesubsynth.hh"
#include "bseserver.hh"
#include "bseengine.hh"	// FIXME: for bse_engine_sample_freq()
#include "bsecxxplugin.hh"
#include "bse/internal.hh"
#include <string.h>

enum
{
  PROP_0,
  PROP_PNET,
};


/* --- prototypes --- */
static void         bse_song_update_tpsi_SL   (BseSong            *song);
static void         bse_song_class_init       (BseSongClass       *klass);
static void         bse_song_init             (BseSong            *song);


/* --- variables --- */
static GTypeClass *parent_class = NULL;

/* --- functions --- */
BSE_BUILTIN_TYPE (BseSong)
{
  static const GTypeInfo song_info = {
    sizeof (BseSongClass),

    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_song_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,

    sizeof (BseSong),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_song_init,
  };

  return bse_type_register_static (BSE_TYPE_SNET,
				   "BseSong",
				   "BSE Song type",
                                   __FILE__, __LINE__,
                                   &song_info);
}

void
bse_song_timing_get_default (Bse::SongTiming *timing)
{
  assert_return (timing != NULL);

  timing->tick = 0;
  timing->bpm = 120;
  timing->numerator = 4;
  timing->denominator = 4;
  timing->tpqn = 384;
  timing->tpt = timing->tpqn * 4 * timing->numerator / timing->denominator;
  timing->stamp_ticks = 0;
}

static void
bse_song_release_children (BseContainer *container)
{
  BseSong *self = BSE_SONG (container);

  while (self->busses)
    bse_container_remove_item (container, (BseItem*) self->busses->data);
  while (self->parts)
    bse_container_remove_item (container, (BseItem*) self->parts->data);
  while (self->tracks_SL)
    bse_container_remove_item (container, (BseItem*) self->tracks_SL->data);

  /* chain parent class' handler */
  BSE_CONTAINER_CLASS (parent_class)->release_children (container);
}

static void
bse_song_finalize (GObject *object)
{
  BseSong *self = BSE_SONG (object);

  bse_container_remove_item (BSE_CONTAINER (self), BSE_ITEM (self->postprocess));
  self->postprocess = NULL;
  bse_container_remove_item (BSE_CONTAINER (self), BSE_ITEM (self->output));
  self->output = NULL;

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bse_song_get_candidates (BseItem *item, uint param_id, Bse::PropertyCandidates &pc, GParamSpec *pspec)
{
  BseSong *self = BSE_SONG (item);
  switch (param_id)
    {
    case PROP_PNET:
      pc.label = _("Available Postprocessors");
      pc.tooltip = _("List of available synthesis networks to choose a postprocessor from");
      bse_item_gather_items_typed (item, pc.items, BSE_TYPE_CSYNTH, BSE_TYPE_PROJECT, FALSE);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
song_uncross_pnet (BseItem *owner,
                   BseItem *ref_item)
{
  BseSong *self = BSE_SONG (owner);
  bse_item_set (self, "pnet", NULL, NULL);
}

static void
bse_song_set_property (GObject      *object,
		       guint         param_id,
		       const GValue *value,
		       GParamSpec   *pspec)
{
  BseSong *self = BSE_SONG (object);
  switch (param_id)
    {
    case PROP_PNET:
      if (!self->postprocess || !BSE_SOURCE_PREPARED (self->postprocess))
        {
          if (self->pnet)
            {
              bse_item_cross_unlink (BSE_ITEM (self), BSE_ITEM (self->pnet), song_uncross_pnet);
              self->pnet = NULL;
            }
          self->pnet = (BseSNet*) bse_value_get_object (value);
          if (self->pnet)
            {
              bse_item_cross_link (BSE_ITEM (self), BSE_ITEM (self->pnet), song_uncross_pnet);
            }
          if (self->postprocess)
            g_object_set (self->postprocess, /* no undo */
                          "snet", self->pnet,
                          NULL);
        }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
bse_song_get_property (GObject     *object,
		       guint        param_id,
		       GValue      *value,
		       GParamSpec  *pspec)
{
  BseSong *self = BSE_SONG (object);
  switch (param_id)
    {
    case PROP_PNET:
      bse_value_set_object (value, self->pnet);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

void
bse_song_get_timing (BseSong *self, uint tick, Bse::SongTiming *timing)
{
  assert_return (BSE_IS_SONG (self));
  assert_return (timing != NULL);

  timing->tick = 0;
  timing->bpm = self->bpm;
  timing->numerator = self->numerator;
  timing->denominator = self->denominator;
  timing->tpqn = self->tpqn;
  timing->tpt = timing->tpqn * 4 * timing->numerator / timing->denominator;
  if (!bse_engine_sample_freq())
    timing->stamp_ticks = 0;
  else /* see update_tpsi */
    timing->stamp_ticks = timing->tpqn * timing->bpm / (60.0 * bse_engine_sample_freq());
}

BseSong*
bse_song_lookup (BseProject  *project,
		 const gchar *name)
{
  BseItem *item;

  assert_return (BSE_IS_PROJECT (project), NULL);
  assert_return (name != NULL, NULL);

  item = bse_container_lookup_item (BSE_CONTAINER (project), name);

  return BSE_IS_SONG (item) ? BSE_SONG (item) : NULL;
}

static void
bse_song_set_parent (BseItem *item,
                     BseItem *parent)
{
  BseSong *self = BSE_SONG (item);

  if (self->midi_receiver_SL)
    {
      bse_midi_receiver_unref (self->midi_receiver_SL);
      self->midi_receiver_SL = NULL;
    }

  /* chain parent class' handler */
  BSE_ITEM_CLASS (parent_class)->set_parent (item, parent);

  if (parent)
    {
      BseProject *project = BSE_PROJECT (parent);
      self->midi_receiver_SL = bse_midi_receiver_ref (project->midi_receiver);
    }
}

static void
bse_song_add_item (BseContainer *container,
		   BseItem	*item)
{
  BseSong *self = BSE_SONG (container);

  BSE_SEQUENCER_LOCK ();

  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_TRACK))
    self->tracks_SL = sfi_ring_append (self->tracks_SL, item);
  else if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_PART))
    self->parts = sfi_ring_append (self->parts, item);
  else if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_BUS))
    self->busses = sfi_ring_append (self->busses, item);
  else
    /* parent class manages other BseSources */ ;

  /* chain parent class' add_item handler */
  BSE_CONTAINER_CLASS (parent_class)->add_item (container, item);

  BSE_SEQUENCER_UNLOCK ();

  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_TRACK))
    bse_track_add_modules (BSE_TRACK (item), container, self->midi_receiver_SL);
  else if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_BUS))
    {
      BseBus *bus = BSE_BUS (item);
      bse_bus_create_stack (bus);
    }
}

static void
bse_song_forall_items (BseContainer	 *container,
		       BseForallItemsFunc func,
		       gpointer		  data)
{
  BseSong *self = BSE_SONG (container);
  SfiRing *ring;

  /* iterate over non-source children */
  ring = self->parts;
  while (ring)
    {
      BseItem *item = (BseItem*) ring->data;
      ring = sfi_ring_walk (ring, self->parts);
      if (!func (item, data))
	return;
    }

  /* parent class iterates over BseSources children */
  BSE_CONTAINER_CLASS (parent_class)->forall_items (container, func, data);
}

static void
bse_song_remove_item (BseContainer *container,
		      BseItem	   *item)
{
  BseSong *self = BSE_SONG (container);

  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_TRACK))
    {
      SfiRing *ring, *tmp;
      bse_track_remove_modules (BSE_TRACK (item), BSE_CONTAINER (self));
      ring = sfi_ring_find (self->tracks_SL, item);
      for (tmp = sfi_ring_walk (ring, self->tracks_SL); tmp; tmp = sfi_ring_walk (tmp, self->tracks_SL))
	bse_item_queue_seqid_changed ((BseItem*) tmp->data);
      BSE_SEQUENCER_LOCK ();
      self->tracks_SL = sfi_ring_remove_node (self->tracks_SL, ring);
      BSE_SEQUENCER_UNLOCK ();
    }
  else if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_PART))
    {
      SfiRing *tmp, *ring = sfi_ring_find (self->parts, item);
      for (tmp = sfi_ring_walk (ring, self->parts); tmp; tmp = sfi_ring_walk (tmp, self->parts))
	bse_item_queue_seqid_changed ((BseItem*) tmp->data);
      self->parts = sfi_ring_remove_node (self->parts, ring);
    }
  else if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_BUS))
    {
      if (self->solo_bus == (BseBus*) item)
        bse_song_set_solo_bus (self, NULL);
      SfiRing *tmp, *ring = sfi_ring_find (self->busses, item);
      for (tmp = sfi_ring_walk (ring, self->busses); tmp; tmp = sfi_ring_walk (tmp, self->busses))
	bse_item_queue_seqid_changed ((BseItem*) tmp->data);
      self->busses = sfi_ring_remove_node (self->busses, ring);
    }
  else
    /* parent class manages BseSources */;

  /* chain parent class' remove_item handler */
  BSE_CONTAINER_CLASS (parent_class)->remove_item (container, item);
}

static gboolean
song_position_handler (gpointer data)
{
  BseSong *self = BSE_SONG (data);

  if (uint (self->last_position) != *self->tick_SL)
    {
      BSE_SEQUENCER_LOCK ();
      self->last_position = *self->tick_SL;
      BSE_SEQUENCER_UNLOCK ();
    }
  return TRUE;
}

static void
bse_song_update_tpsi_SL (BseSong *self)
{
  gdouble tpqn = self->tpqn;		/* ticks per quarter note */
  gdouble qnps = self->bpm / 60.;	/* quarter notes per second */
  gdouble tps = tpqn * qnps;		/* ticks per second */
  gdouble sps = bse_engine_sample_freq ();
  gdouble tpsi = tps / sps;		/* ticks per stamp increment (sample) */
  BSE_SEQUENCER_LOCK ();
  self->tpsi_SL = tpsi;
  BSE_SEQUENCER_UNLOCK ();
}

static void
bse_song_prepare (BseSource *source)
{
  BseSong *self = BSE_SONG (source);

  bse_object_lock (BSE_OBJECT (self));
  self->sequencer_underrun_detected_SL = FALSE;

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);

  bse_song_update_tpsi_SL (self);

  if (!self->position_handler)
    self->position_handler = bse_idle_timed (50000, song_position_handler, self);
}

static void
bse_song_context_create (BseSource *source,
			 guint      context_handle,
			 BseTrans  *trans)
{
  BseSong *self = BSE_SONG (source);
  BseSNet *snet = BSE_SNET (self);
  BseMidiContext mcontext = bse_snet_get_midi_context (snet, context_handle);
  SfiRing *ring;
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
  if (!bse_snet_context_is_branch (snet, context_handle))       /* catch recursion */
    for (ring = self->tracks_SL; ring; ring = sfi_ring_walk (ring, self->tracks_SL))
      bse_track_clone_voices ((BseTrack*) ring->data, snet, context_handle, mcontext, trans);
}
static void
bse_song_reset (BseSource *source)
{
  BseSong *self = BSE_SONG (source);
  Bse::Sequencer::instance().remove_song (self);
  // chain parent class' handler
  BSE_SOURCE_CLASS (parent_class)->reset (source);
  assert_return (self->sequencer_start_request_SL == 0);
  /* outside of sequencer reach, so no locks needed */
  self->sequencer_start_SL = 0;
  self->sequencer_done_SL = 0;
  if (self->position_handler)
    {
      bse_idle_remove (self->position_handler);
      self->position_handler = 0;
    }
  bse_object_unlock (BSE_OBJECT (self));
  self->as<Bse::SongImpl*>()->notify ("tick_pointer");
}
BseSource*
bse_song_create_summation (BseSong *self)
{
  GType type = g_type_from_name ("BseSummation");
  if (!g_type_is_a (type, BSE_TYPE_SOURCE))
    {
      Bse::warning ("%s: failed to resolve %s object type, probably missing or broken plugin installation", __func__, "BseSummation");
      return NULL;
    }
  BseSource *summation = (BseSource*) bse_container_new_child (BSE_CONTAINER (self), type, "uname", "Summation", NULL);
  assert_return (summation != NULL, NULL);
  bse_snet_intern_child (BSE_SNET (self), summation);
  return summation;
}

BseBus*
bse_song_find_master (BseSong *self)
{
  BseSource *osource;
  if (self->postprocess &&
      (bse_source_get_input (self->postprocess, 0, &osource, NULL) ||
       bse_source_get_input (self->postprocess, 1, &osource, NULL)) &&
      BSE_IS_BUS (osource))
    return BSE_BUS (osource);
  return NULL;
}

void
bse_song_set_solo_bus (BseSong        *self,
                       BseBus         *bus)
{
  BseBus *master = bse_song_find_master (self);
  if (bus && BSE_ITEM (bus)->parent != BSE_ITEM (self))
    bus = NULL;
  SfiRing *ring;
  self->solo_bus = bus;
  for (ring = self->busses; ring; ring = sfi_ring_walk (ring, self->busses))
    bse_bus_change_solo ((BseBus*) ring->data, self->solo_bus && ring->data != self->solo_bus && ring->data != master);
}

static void
bse_song_init (BseSong *self)
{
  bse_item_set (self, "uname", _("Song"), NULL);

  Bse::SongTiming timing;
  bse_song_timing_get_default (&timing);

  self->unset_flag (BSE_SNET_FLAG_USER_SYNTH);
  self->set_flag (BSE_SUPER_FLAG_NEEDS_CONTEXT);

  self->musical_tuning = Bse::MusicalTuning::OD_12_TET;

  self->tpqn = timing.tpqn;
  self->numerator = timing.numerator;
  self->denominator = timing.denominator;
  self->bpm = timing.bpm;

  self->parts = NULL;
  self->busses = NULL;

  self->pnet = NULL;

  self->last_position = -1;
  self->position_handler = 0;

  self->tracks_SL = NULL;
  self->loop_enabled_SL = 0;
  self->loop_left_SL = -1;
  self->loop_right_SL = -1;
}

static const gchar*
master_bus_name (void)
{
  /* TRANSLATORS: this is the name of the master mixer bus. i.e. the final audio output bus. */
  return _("Master");
}

BseSource*
bse_song_ensure_master (BseSong *self)
{
  assert_return (BSE_IS_SONG (self), NULL);
  Bse::SongImpl *this_ = self->as<Bse::SongImpl*>();
  BseSource *child = (BseSource*) bse_song_find_master (self);
  if (!child)
    {
      BseUndoStack *ustack = bse_item_undo_open (self, "Create Master");
      child = (BseSource*) bse_container_new_child_bname (BSE_CONTAINER (self), BSE_TYPE_BUS, master_bus_name(), NULL);
      Bse::BusImpl *bus = child->as<Bse::BusImpl*>();
      bus->master_output (true);
      Bse::ItemImpl::UndoDescriptor<Bse::BusImpl> bus_descriptor = this_->undo_descriptor (*bus);
      auto remove_bus_lambda = [bus_descriptor] (Bse::SongImpl &self, BseUndoStack *ustack) -> Bse::Error {
        Bse::BusImpl &bus = self.undo_resolve (bus_descriptor);
        self.remove_bus (bus);
        return Bse::Error::NONE;
      };
      this_->push_undo (__func__, *this_, remove_bus_lambda);
      bse_item_undo_close (ustack);
    }
  return child;
}

BseTrack*
bse_song_find_first_track (BseSong *self, BsePart *part)
{
  SfiRing *ring;
  /* action */
  for (ring = self->tracks_SL; ring; ring = sfi_ring_walk (ring, self->tracks_SL))
    {
      BseTrack *track = (BseTrack*) ring->data;
      guint start;
      if (bse_track_find_part (track, part, &start))
        return track;
    }
  return NULL;
}

static void
bse_song_compat_finish (BseSuper       *super,
                        guint           vmajor,
                        guint           vminor,
                        guint           vmicro)
{
  BseSong *self = BSE_SONG (super);

  /* chain parent class' handler */
  BSE_SUPER_CLASS (parent_class)->compat_finish (super, vmajor, vminor, vmicro);

  /* fixup old non-mixer songs */
  if (BSE_VERSION_CMP (vmajor, vminor, vmicro, 0, 6, 2) <= 0)
    {
      /* collect all bus inputs */
      SfiRing *node, *tracks, *inputs = NULL;
      for (node = self->busses; node; node = sfi_ring_walk (node, self->busses))
        inputs = sfi_ring_concat (inputs, bse_bus_list_inputs ((BseBus*) node->data));
      /* find tracks that are not in input list */
      tracks = sfi_ring_copy (self->tracks_SL);
      inputs = sfi_ring_sort (inputs, sfi_pointer_cmp, NULL);
      tracks = sfi_ring_sort (tracks, sfi_pointer_cmp, NULL);
      node = sfi_ring_difference (tracks, inputs, sfi_pointer_cmp, NULL);
      sfi_ring_free (inputs);
      sfi_ring_free (tracks);
      tracks = node;
      /* connect remaining tracks */
      gboolean clear_undo = FALSE;
      BseSource *master = bse_song_ensure_master (self);
      for (node = master ? tracks : NULL; node; node = sfi_ring_walk (node, tracks))
        {
          Bse::Error error = bse_bus_connect (BSE_BUS (master), (BseItem*) node->data);
          if (error != 0)
            Bse::warning ("Failed to connect track %s: %s", bse_object_debug_name (node->data), bse_error_blurb (error));
          clear_undo = TRUE;
        }
      sfi_ring_free (tracks);
      if (clear_undo)
        {
          BseProject *project = bse_item_get_project (BSE_ITEM (self));
          if (project)
            bse_project_clear_undo (project);
        }
    }
}

static void
bse_song_class_init (BseSongClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (klass);
  BseItemClass *item_class = BSE_ITEM_CLASS (klass);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (klass);
  BseContainerClass *container_class = BSE_CONTAINER_CLASS (klass);
  BseSuperClass *super_class = BSE_SUPER_CLASS (klass);
  Bse::SongTiming timing;

  parent_class = (GTypeClass*) g_type_class_peek_parent (klass);

  gobject_class->set_property = bse_song_set_property;
  gobject_class->get_property = bse_song_get_property;
  gobject_class->finalize = bse_song_finalize;

  item_class->set_parent = bse_song_set_parent;
  item_class->get_candidates = bse_song_get_candidates;

  source_class->prepare = bse_song_prepare;
  source_class->context_create = bse_song_context_create;
  source_class->reset = bse_song_reset;

  container_class->add_item = bse_song_add_item;
  container_class->remove_item = bse_song_remove_item;
  container_class->forall_items = bse_song_forall_items;
  container_class->release_children = bse_song_release_children;

  super_class->compat_finish = bse_song_compat_finish;

  bse_song_timing_get_default (&timing);

  bse_object_class_add_param (object_class, _("MIDI Instrument"),
                              PROP_PNET,
                              bse_param_spec_object ("pnet", _("Postprocessor"), _("Synthesis network to be used as postprocessor"),
                                                     BSE_TYPE_CSYNTH, SFI_PARAM_STANDARD ":unprepared"));
}

namespace Bse {

SongImpl::SongImpl (BseObject *bobj) :
  SNetImpl (bobj)
{
  shm_block_ = BSE_SERVER.allocate_shared_block (ptrdiff_t (SongTelemetry::BYTECOUNT));
}

void
SongImpl::post_init()
{
  this->SNetImpl::post_init(); // must chain
  BseSong *self = as<BseSong*>();
  self->tick_SL = (uint*) (((char*) shm_block_.mem_start) + ptrdiff_t (SongTelemetry::I32_TICK_POINTER));
  /* post processing slot */
  self->postprocess = (BseSource*) bse_container_new_child (BSE_CONTAINER (self), BSE_TYPE_SUB_SYNTH, "uname", "Postprocess", NULL);
  bse_snet_intern_child (self, self->postprocess);
  bse_sub_synth_set_null_shortcut (BSE_SUB_SYNTH (self->postprocess), TRUE);

  /* output */
  self->output = (BseSource*) bse_container_new_child (BSE_CONTAINER (self), BSE_TYPE_PCM_OUTPUT, NULL);
  bse_snet_intern_child (self, self->output);

  /* postprocess <-> output */
  bse_source_must_set_input (self->output, BSE_PCM_OUTPUT_ICHANNEL_LEFT,
			     self->postprocess, 0);
  bse_source_must_set_input (self->output, BSE_PCM_OUTPUT_ICHANNEL_RIGHT,
			     self->postprocess, 1);
}

SongImpl::~SongImpl ()
{
  BseSong *self = as<BseSong*>();
  self->tick_SL = nullptr;
  BSE_SERVER.release_shared_block (shm_block_);
}

int64_t
SongImpl::get_shm_offset (SongTelemetry fld)
{
  return shm_block_.mem_offset + ptrdiff_t (fld);
}

TrackIfaceP
SongImpl::find_any_track_for_part (PartIface &part)
{
  BseSong *self = as<BseSong*>();
  assert_return (dynamic_cast<ItemImpl*> (&part)->parent() == this, NULL);
  BsePart *bpart = part.as<BsePart*>();
  BseTrack *track = bse_song_find_first_track (self, bpart);
  return track ? track->as<TrackIfaceP> () : NULL;
}

BusIfaceP
SongImpl::create_bus ()
{
  BseSong *self = as<BseSong*>();
  if (BSE_SOURCE_PREPARED (self))
    return NULL;
  BusImpl *bus = ((BseItem*) bse_container_new_child (BSE_CONTAINER (self), BSE_TYPE_BUS, NULL))->as<BusImpl*>();
  UndoDescriptor<BusImpl> bus_descriptor = undo_descriptor (*bus);
  auto remove_bus_lambda = [bus_descriptor] (SongImpl &self, BseUndoStack *ustack) -> Error {
    BusImpl &bus = self.undo_resolve (bus_descriptor);
    self.remove_bus (bus);
    return Error::NONE;
  };
  push_undo (__func__, *this, remove_bus_lambda);
  return bus->as<BusIfaceP>();
}

void
SongImpl::remove_bus (BusIface &bus_iface)
{
  BseSong *self = as<BseSong*>();
  BusImpl &bus = dynamic_cast<BusImpl&> (bus_iface);
  assert_return (bus.parent() == this);
  return_unless (BSE_SOURCE_PREPARED (self) == false);
  BseItem *child = bus.as<BseItem*>();
  BseUndoStack *ustack = bse_item_undo_open (self, __func__);
  bus.master_output (false);
  // backup object references to undo stack
  bse_container_uncross_undoable (BSE_CONTAINER (self), child);
  // implement "undo" of bse_container_remove_backedup, i.e. redo
  UndoDescriptor<BusImpl> bus_descriptor = undo_descriptor (bus);
  auto remove_bus_lambda = [bus_descriptor] (SongImpl &self, BseUndoStack *ustack) -> Error {
    BusImpl &bus = self.undo_resolve (bus_descriptor);
    self.remove_bus (bus);
    return Error::NONE;
  };
  push_undo_to_redo (__func__, *this, remove_bus_lambda);
  // backup and remove (without redo queueing)
  bse_container_remove_backedup (BSE_CONTAINER (self), child, ustack);
  // done
  bse_item_undo_close (ustack);
}

PartIfaceP
SongImpl::create_part ()
{
  BseSong *self = as<BseSong*>();
  BseItem *child = (BseItem*) bse_container_new_child (BSE_CONTAINER (self), BSE_TYPE_PART, NULL);
  PartImpl *part = child->as<PartImpl*>();
  UndoDescriptor<PartImpl> part_descriptor = undo_descriptor (*part);
  auto remove_part_lambda = [part_descriptor] (SongImpl &self, BseUndoStack *ustack) -> Error {
    PartImpl &part = self.undo_resolve (part_descriptor);
    self.remove_part (part);
    return Error::NONE;
  };
  push_undo (__func__, *this, remove_part_lambda);
  return part->as<PartIfaceP>();
}

void
SongImpl::remove_part (PartIface &part_iface)
{
  BseSong *self = as<BseSong*>();
  PartImpl *part = dynamic_cast<PartImpl*> (&part_iface);
  return_unless (part->parent() == this);
  if (BSE_SOURCE_PREPARED (self))
    return;
  BseItem *child = part->as<BseItem*>();
  BseUndoStack *ustack = bse_item_undo_open (self, "Remove Part");
  // backup object references to undo stack
  bse_container_uncross_undoable (BSE_CONTAINER (self), child);
  // implement "undo" of bse_container_remove_backedup, i.e. redo
  UndoDescriptor<PartImpl> part_descriptor = undo_descriptor (*part);
  auto remove_part_lambda = [part_descriptor] (SongImpl &self, BseUndoStack *ustack) -> Error {
    PartImpl &part = self.undo_resolve (part_descriptor);
    self.remove_part (part);
    return Error::NONE;
  };
  push_undo_to_redo (__func__, *this, remove_part_lambda);
  // remove (without redo queueing)
  bse_container_remove_backedup (BSE_CONTAINER (self), child, ustack);
  // done
  bse_item_undo_close (ustack);
}

TrackSeq
SongImpl::list_tracks ()
{
  TrackSeq tracks;
  for (auto &childp : list_children())
    {
      ItemIface *item = childp.get();
      TrackIfaceP track = item->as<TrackIfaceP>();
      if (track)
        tracks.push_back (track);
    }
  return tracks;
}

TrackIfaceP
SongImpl::create_track ()
{
  BseSong *self = as<BseSong*>();
  return_unless (BSE_SOURCE_PREPARED (self) == false, NULL);
  BseItem *child = (BseItem*) bse_container_new_child (BSE_CONTAINER (self), BSE_TYPE_TRACK, NULL);
  TrackImpl *track = child->as<TrackImpl*>();
  UndoDescriptor<TrackImpl> track_descriptor = undo_descriptor (*track);
  auto remove_track_lambda = [track_descriptor] (SongImpl &self, BseUndoStack *ustack) -> Error {
    TrackImpl &track = self.undo_resolve (track_descriptor);
    self.remove_track (track);
    return Error::NONE;
  };
  push_undo (__func__, *this, remove_track_lambda);
  return track->as<TrackIfaceP>();
}

void
SongImpl::remove_track (TrackIface &track_iface)
{
  BseSong *self = as<BseSong*>();
  TrackImpl *track = dynamic_cast<TrackImpl*> (&track_iface);
  return_unless (track->parent() == this);
  if (BSE_SOURCE_PREPARED (self))
    return;
  BseItem *child = track->as<BseItem*>();
  BseUndoStack *ustack = bse_item_undo_open (self, "Remove Track");
  // backup object references to undo stack
  bse_container_uncross_undoable (BSE_CONTAINER (self), child);
  // implement "undo" of bse_container_remove_backedup, i.e. redo
  UndoDescriptor<TrackImpl> track_descriptor = undo_descriptor (*track);
  auto remove_track_lambda = [track_descriptor] (SongImpl &self, BseUndoStack *ustack) -> Error {
    TrackImpl &track = self.undo_resolve (track_descriptor);
    self.remove_track (track);
    return Error::NONE;
  };
  push_undo_to_redo (__func__, *this, remove_track_lambda);
  // remove (without redo queueing)
  bse_container_remove_backedup (BSE_CONTAINER (self), child, ustack);
  // done
  bse_item_undo_close (ustack);
}

SongTiming
SongImpl::get_timing (int tick)
{
  BseSong *self = as<BseSong*>();
  SongTiming timing;
  bse_song_get_timing (self, tick, &timing);
  return timing;
}

int
SongImpl::numerator() const
{
  BseSong *self = const_cast<SongImpl*> (this)->as<BseSong*>();

  return self->numerator;
}

void
SongImpl::numerator (int val)
{
  BseSong *self = as<BseSong*>();
  if (APPLY_IDL_PROPERTY (self->numerator, val))
    bse_song_update_tpsi_SL (self);
}

int
SongImpl::denominator() const
{
  BseSong *self = const_cast<SongImpl*> (this)->as<BseSong*>();

  return self->denominator;
}

void
SongImpl::denominator (int val)
{
  BseSong *self = as<BseSong*>();
  const int d = val <= 2 ? val : 1 << g_bit_storage (val - 1);
  if (APPLY_IDL_PROPERTY (self->denominator, d))
    bse_song_update_tpsi_SL (self);
}

double
SongImpl::bpm () const
{
  BseSong *self = const_cast<SongImpl*> (this)->as<BseSong*>();
  return self->bpm;
}

void
SongImpl::bpm (double val)
{
  BseSong *self = as<BseSong*>();
  if (APPLY_IDL_PROPERTY (self->bpm, float (val)))
    bse_song_update_tpsi_SL (self);
}


int
SongImpl::tpqn() const
{
  BseSong *self = const_cast<SongImpl*> (this)->as<BseSong*>();
  return self->tpqn;
}

void
SongImpl::tpqn (int val)
{
  BseSong *self = as<BseSong*>();
  if (APPLY_IDL_PROPERTY (self->tpqn, val))
    bse_song_update_tpsi_SL (self);
}

MusicalTuning
SongImpl::musical_tuning () const
{
  BseSong *self = const_cast<SongImpl*> (this)->as<BseSong*>();
  return self->musical_tuning;
}

void
SongImpl::musical_tuning (MusicalTuning tuning)
{
  if (!prepared())
    {
      BseSong *self = as<BseSong*>();
      if (APPLY_IDL_PROPERTY (self->musical_tuning, tuning))
        {
          SfiRing *ring;
          for (ring = self->parts; ring; ring = sfi_ring_walk (ring, self->parts))
            bse_part_set_semitone_table ((BsePart*) ring->data, bse_semitone_table_from_tuning (self->musical_tuning));
        }
    }
}

bool
SongImpl::loop_enabled() const
{
  BseSong *self = const_cast<SongImpl*> (this)->as<BseSong *>();

  return self->loop_enabled_SL;
}

void
SongImpl::loop_enabled (bool enabled)
{
  BseSong *self = as<BseSong*>();

  enabled = enabled && self->loop_left_SL >= 0 && self->loop_right_SL > self->loop_left_SL;
  bool value = self->loop_enabled_SL;

  if (APPLY_IDL_PROPERTY (value, enabled))
    {
      BSE_SEQUENCER_LOCK ();
      self->loop_enabled_SL = value;
      BSE_SEQUENCER_UNLOCK ();
    }
}

int
SongImpl::loop_left() const
{
  BseSong *self = const_cast<SongImpl*> (this)->as<BseSong*>();

  return self->loop_left_SL;
}

void
SongImpl::loop_left (int tick)
{
  BseSong *self = as<BseSong*>();

  if (tick != self->loop_left_SL)
    {
      // this property has no undo

      const bool loop_enabled = self->loop_enabled_SL;

      BSE_SEQUENCER_LOCK ();
      self->loop_left_SL = tick;
      self->loop_enabled_SL = (self->loop_enabled_SL &&
                               self->loop_left_SL >= 0 &&
                               self->loop_right_SL > self->loop_left_SL);
      BSE_SEQUENCER_UNLOCK ();

      notify ("loop_left");
      if (loop_enabled != self->loop_enabled_SL)
        notify ("loop_enabled");
    }
}

int
SongImpl::loop_right() const
{
  BseSong *self = const_cast<SongImpl*> (this)->as<BseSong*>();

  return self->loop_right_SL;
}

void
SongImpl::loop_right (int tick)
{
  BseSong *self = as<BseSong*>();

  if (tick != self->loop_right_SL)
    {
      // this property has no undo

      const bool loop_enabled = self->loop_enabled_SL;

      BSE_SEQUENCER_LOCK ();
      self->loop_right_SL = tick;
      self->loop_enabled_SL = (self->loop_enabled_SL &&
                               self->loop_left_SL >= 0 &&
                               self->loop_right_SL > self->loop_left_SL);
      BSE_SEQUENCER_UNLOCK ();

      notify ("loop_right");
      if (loop_enabled != self->loop_enabled_SL)
        notify ("loop_enabled");
    }
}

int
SongImpl::tick_pointer() const
{
  BseSong *self = const_cast<SongImpl*> (this)->as<BseSong*>();

  return *self->tick_SL;
}

void
SongImpl::tick_pointer (int tick)
{
  BseSong *self = as<BseSong*>();

  if (uint (tick) != *self->tick_SL)
    {
      // this property has no undo

      BSE_SEQUENCER_LOCK ();
      *self->tick_SL = tick;
      for (SfiRing *ring = self->tracks_SL; ring; ring = sfi_ring_walk (ring, self->tracks_SL))
        {
          BseTrack *track = (BseTrack*) ring->data;
          track->track_done_SL = false;	/* let sequencer recheck if playing */
        }
      BSE_SEQUENCER_UNLOCK ();

      notify ("tick_pointer");
    }
}

BusIfaceP
SongImpl::ensure_master_bus ()
{
  BseSong *self = as<BseSong*>();
  BseSource *child = bse_song_ensure_master (self);
  return child->as<BusIfaceP>();
}

TrackIfaceP
SongImpl::find_track_for_part (PartIface &part_iface)
{
  BseSong *self = as<BseSong*>();
  BsePart *part = part_iface.as<BsePart*>();
  BseTrack *track = NULL;
  uint tick = 0;
  for (SfiRing *ring = self->tracks_SL; ring; ring = sfi_ring_walk (ring, self->tracks_SL))
    {
      BseTrack *test_track = (BseTrack*) ring->data;
      uint start;
      if (bse_track_find_part (test_track, part, &start) &&
	  (!track || start < tick))
	{
	  track = test_track;
	  tick = start;
	}
    }
  return track ? track->as<TrackIfaceP>() : NULL;
}

BusIfaceP
SongImpl::get_master_bus ()
{
  BseSong *self = as<BseSong*>();
  BseBus *bus = bse_song_find_master (self);
  return bus ? bus->as<BusIfaceP>() : NULL;
}

void
SongImpl::synthesize_note (TrackIface &track_iface, int duration, int note, int fine_tune, double velocity)
{
  BseSong *self = as<BseSong*>();
  BseTrack *track = track_iface.as<BseTrack*>();
  if (BSE_SOURCE_PREPARED (self) && self->midi_receiver_SL)
    {
      double semitone_factor = bse_transpose_factor (self->musical_tuning, CLAMP (note, SFI_MIN_NOTE, SFI_MAX_NOTE) - SFI_KAMMER_NOTE);
      double freq = BSE_KAMMER_FREQUENCY * semitone_factor * bse_cent_tune_fast (fine_tune);
      SfiTime tstamp = Bse::TickStamp::current() + BSE_ENGINE_MAX_BLOCK_SIZE * 2;
      BseMidiEvent *eon, *eoff;
      eon  = bse_midi_event_note_on (track->midi_channel_SL, tstamp, freq, velocity);
      eoff = bse_midi_event_note_off (track->midi_channel_SL, tstamp + duration, freq);
      bse_midi_receiver_push_event (self->midi_receiver_SL, eon);
      bse_midi_receiver_push_event (self->midi_receiver_SL, eoff);
      bse_midi_receiver_process_events (self->midi_receiver_SL, tstamp + duration);
      bse_project_keep_activated (BSE_PROJECT (BSE_ITEM (self)->parent), tstamp + duration);
    }
}

static const gchar*
orphans_track_name (void)
{
  /* TRANSLATORS: this is the name of the track that is used to automatically
   * adopt orphan (unlinked) parts.
   */
  return _("Orphan Parts");
}

static BseTrack*
bse_song_ensure_orphans_track_noundo (BseSong *self)
{
  for (SfiRing *ring = self->tracks_SL; ring; ring = sfi_ring_walk (ring, self->tracks_SL))
    {
      BseTrack *track = (BseTrack*) ring->data;
      TrackImpl *trackimpl = track->as<TrackImpl*>();
      bool muted = trackimpl->muted();
      if (muted && g_object_get_data ((GObject*) track, "BseSong-orphan-track") == bse_song_ensure_orphans_track_noundo) /* detect orphan-parts track */
        return track;
    }
  BseTrack *child = (BseTrack*) bse_container_new_child_bname (BSE_CONTAINER (self), BSE_TYPE_TRACK, orphans_track_name(), NULL);
  TrackImpl *trackimpl = child->as<TrackImpl*>();
  trackimpl->muted (true);
  g_object_set_data ((GObject*) child, "BseSong-orphan-track", (void*) bse_song_ensure_orphans_track_noundo); /* mark orphan-parts track */
  return child;
}

void
SongImpl::ensure_track_links ()
{
  BseSong *self = as<BseSong*>();
  bool clear_undo = false;
  for (SfiRing *ring = self->parts; ring; ring = sfi_ring_walk (ring, self->parts))
    {
      BsePart *part = (BsePart*) ring->data;
      if (bse_song_find_first_track (self, part))
        continue;
      BseTrack *track = bse_song_ensure_orphans_track_noundo (self);
      TrackImpl *trackimpl = track->as<TrackImpl*>();
      trackimpl->insert_part (bse_track_get_last_tick (track), *part->as<PartImpl*>());
      clear_undo = true;
    }
  if (clear_undo)
    {
      BseProject *project = bse_item_get_project (self);
      if (project)
        bse_project_clear_undo (project);
    }
}

} // Bse
