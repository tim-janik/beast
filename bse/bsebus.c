/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2004 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bsebus.h"
#include "bsecategories.h"
#include "bsetrack.h"
#include "bsesong.h"
#include "bseengine.h"
#include "bsecsynth.h"
#include "bsesubiport.h"
#include "bsesuboport.h"
#include "bseproject.h"
#include "bsestorage.h"


#define parse_or_return         bse_storage_scanner_parse_or_return
#define peek_or_return          bse_storage_scanner_peek_or_return


/* --- parameters --- */
enum
{
  PROP_0,
  PROP_INPUTS,
  PROP_SNET,
  PROP_MUTE,
  PROP_SOLO,
  PROP_SYNC,
  PROP_LEFT_VOLUME_dB,
  PROP_RIGHT_VOLUME_dB,
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
  bse_sub_synth_set_null_shortcut (BSE_SUB_SYNTH (self), TRUE);
}

static void
bse_bus_dispose (GObject *object)
{
  BseBus *self = BSE_BUS (object);
  while (self->inputs)
    bse_bus_disconnect (self, self->inputs->data);
  /* chain parent class' handler */
  G_OBJECT_CLASS (bus_parent_class)->dispose (object);
}

static void
bse_bus_finalize (GObject *object)
{
  BseBus *self = BSE_BUS (object);
  g_assert (self->inputs == NULL);
  g_assert (self->summation == NULL);
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
bus_list_candidates (BseBus     *self,
                     BseItemSeq *iseq)
{
  BseItem *item = BSE_ITEM (self);
  bse_item_gather_items_typed (item, iseq, BSE_TYPE_BUS, BSE_TYPE_SONG, FALSE);
  bse_item_gather_items_typed (item, iseq, BSE_TYPE_TRACK, BSE_TYPE_SONG, FALSE);
  BseBus *master = get_master (self);
  if (master)
    bse_item_seq_remove (iseq, BSE_ITEM (master));
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
      bus_list_candidates (self, pc->items);
      ring = bse_bus_list_inputs (self);
      while (ring)
        bse_item_seq_remove (pc->items, sfi_ring_pop_head (&ring));
      /* SYNC: type partitions */
      bse_type_seq_append (pc->partitions, "BseTrack");
      bse_type_seq_append (pc->partitions, "BseBus");
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
    case PROP_SOLO:
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
      BseErrorType error = bse_bus_disconnect (ring->data, BSE_ITEM (self));
      bse_assert_ok (error);
    }
  bse_source_clear_ochannels (BSE_SOURCE (self));       /* also disconnects master */
  g_object_notify (self, "master-output");              /* master may have changed */
  g_object_notify (self, "solo");                       /* master may have changed */
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
      g_object_notify (bus, "master-output");
      g_object_notify (bus, "solo");
    }
}

static void
bus_volume_changed (BseBus *self)
{
  if (self->bmodule)
    {
      double db1, db2;
      if (self->muted || self->solo_muted)
        {
          db1 = BSE_MIN_VOLUME_dB;
          db2 = BSE_MIN_VOLUME_dB;
        }
      else
        {
          double lvolume = self->left_volume;
          double rvolume = self->right_volume;
          if (self->synced)
            lvolume = rvolume = (lvolume + rvolume) * 0.5;
          db1 = bse_db_from_factor (lvolume, BSE_MIN_VOLUME_dB);
          db2 = bse_db_from_factor (rvolume, BSE_MIN_VOLUME_dB);
        }
      g_object_set (self->bmodule, "volume1db", db1, "volume2db", db2, NULL);
    }
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
      BseItemSeq *iseq;
      BseItem *parent;
    case PROP_INPUTS:
      /* save user provided order */
      saved_inputs = bse_item_seq_to_ring (g_value_get_boxed (value));
      /* provide sorted rings: self->inputs, inputs */
      inputs = sfi_ring_sort (sfi_ring_copy (saved_inputs), sfi_compare_pointers, NULL);
      self->inputs = sfi_ring_sort (self->inputs, sfi_compare_pointers, NULL);
      /* get all input candidates */
      iseq = bse_item_seq_new();
      bus_list_candidates (self, iseq);
      candidates = sfi_ring_sort (bse_item_seq_to_ring (iseq), sfi_compare_pointers, NULL);
      bse_item_seq_free (iseq);
      /* constrain the new input list */
      ring = sfi_ring_intersection (inputs, candidates, sfi_compare_pointers, NULL);
      sfi_ring_free (candidates);
      sfi_ring_free (inputs);
      inputs = ring;
      /* eliminate stale inputs */
      ring = sfi_ring_difference (self->inputs, inputs, sfi_compare_pointers, NULL);
      while (ring)
        bse_bus_disconnect (self, sfi_ring_pop_head (&ring));
      /* add new inputs */
      ring = sfi_ring_difference (inputs, self->inputs, sfi_compare_pointers, NULL);
      while (ring)
        bse_bus_connect_unchecked (self, sfi_ring_pop_head (&ring));
      sfi_ring_free (inputs);
      /* restore user provided order */
      self->inputs = sfi_ring_reorder (self->inputs, saved_inputs);
      sfi_ring_free (saved_inputs);
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
      self->synced = sfi_value_get_bool (value);
      if (self->synced)
        self->right_volume = self->left_volume = (self->right_volume + self->left_volume) * 0.5;
      bus_volume_changed (self);
      g_object_notify (self, "left-volume-db");
      g_object_notify (self, "right-volume-db");
      break;
    case PROP_LEFT_VOLUME_dB:
      self->left_volume = bse_db_to_factor (sfi_value_get_real (value));
      if (self->synced)
        {
          self->right_volume = self->left_volume;
          g_object_notify (self, "right-volume-db");
        }
      bus_volume_changed (self);
      break;
    case PROP_RIGHT_VOLUME_dB:
      self->right_volume = bse_db_to_factor (sfi_value_get_real (value));
      if (self->synced)
        {
          self->left_volume = self->right_volume;
          g_object_notify (self, "left-volume-db");
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
      BseItemSeq *iseq;
      SfiRing *ring;
    case PROP_INPUTS:
      iseq = bse_item_seq_new();
      ring = bse_bus_list_inputs (self);
      while (ring)
        bse_item_seq_append (iseq, sfi_ring_pop_head (&ring));
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
    case PROP_LEFT_VOLUME_dB:
      sfi_value_set_real (value, bse_db_from_factor (self->synced ? (self->left_volume + self->right_volume) * 0.5 : self->left_volume, BSE_MIN_VOLUME_dB));
      break;
    case PROP_RIGHT_VOLUME_dB:
      sfi_value_set_real (value, bse_db_from_factor (self->synced ? (self->left_volume + self->right_volume) * 0.5 : self->right_volume, BSE_MIN_VOLUME_dB));
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
  g_object_notify (self, "solo");
  g_object_notify (self, "mute");
}

static void
bse_bus_set_parent (BseItem *item,
                    BseItem *parent)
{
  BseBus *self = BSE_BUS (item);
  self->solo_muted = FALSE;

  /* chain parent class' handler */
  BSE_ITEM_CLASS (bus_parent_class)->set_parent (item, parent);

  while (self->inputs)
    bse_bus_disconnect (self, self->inputs->data);
  
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
      g_warning ("Bus[%p] has snet[%p] in set-parent", self, BSE_SUB_SYNTH (self)->snet);
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
      g_assert (self->n_effects == 0);
      bse_bus_ensure_summation (self);
      BseSNet *snet = (BseSNet*) bse_project_create_intern_csynth (project, "%BusEffectStack");
      self->vin = bse_container_new_child_bname (BSE_CONTAINER (snet), BSE_TYPE_SUB_IPORT, "%VInput", NULL);
      bse_snet_intern_child (snet, self->vin);
      BseSource *vout = bse_container_new_child_bname (BSE_CONTAINER (snet), BSE_TYPE_SUB_OPORT, "%VOutput", NULL);
      bse_snet_intern_child (snet, vout);
      self->bmodule = bse_container_new_child_bname (BSE_CONTAINER (snet), g_type_from_name ("BseBusModule"), "%Volume", NULL);
      bse_snet_intern_child (snet, self->bmodule);
      g_object_set (self->bmodule,
                    "volume1db", bse_db_from_factor (self->left_volume, BSE_MIN_VOLUME_dB),
                    "volume2db", bse_db_from_factor (self->right_volume, BSE_MIN_VOLUME_dB),
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
          bse_source_must_set_input (BSE_SOURCE (self), 0, self->summation, 0);
          bse_source_must_set_input (BSE_SOURCE (self), 1, self->summation, 1);
        }
    }
  return self->summation != NULL;
}

static void
bus_uncross_input (BseItem *owner,
                   BseItem *item)
{
  /* delete item via procedure so deletion is recorded to undo */
  if (BSE_IS_TRACK (item))
    bse_item_exec_void (owner, "disconnect-track", item);
  else /* IS_BUS */
    bse_item_exec_void (owner, "disconnect-bus", item);
}

BseErrorType
bse_bus_connect (BseBus  *self,
                 BseItem *trackbus)
{
  /* get all input candidates */
  BseItemSeq *iseq = bse_item_seq_new();
  bus_list_candidates (self, iseq);
  /* find trackbus */
  gboolean found_candidate = FALSE;
  guint i;
  for (i = 0; i < iseq->n_items; i++)
    if (iseq->items[i] == trackbus)
      {
        found_candidate = TRUE;
        break;
      }
  bse_item_seq_free (iseq);
  /* add trackbus if valid */
  if (found_candidate)
    return bse_bus_connect_unchecked (self, trackbus);
  else
    return BSE_ERROR_SOURCE_CONNECTION_INVALID;
}

BseErrorType
bse_bus_connect_unchecked (BseBus  *self,
                           BseItem *trackbus)
{
  BseSource *osource;
  if (BSE_IS_TRACK (trackbus))
    osource = bse_track_get_output (BSE_TRACK (trackbus));
  else if (BSE_IS_BUS (trackbus))
    osource = BSE_SOURCE (trackbus);
  else
    return BSE_ERROR_SOURCE_TYPE_INVALID;
  if (!osource || !bse_bus_ensure_summation (self) ||
      BSE_ITEM (osource)->parent != BSE_ITEM (self)->parent)    /* restrict to siblings */
    return BSE_ERROR_SOURCE_PARENT_MISMATCH;
  BseErrorType error = bse_source_set_input (self->summation, 0, osource, 0);
  if (!error)
    {
      bse_source_must_set_input (self->summation, 1, osource, 1);
      self->inputs = sfi_ring_append (self->inputs, trackbus);
      bse_item_cross_link (BSE_ITEM (self), BSE_ITEM (trackbus), bus_uncross_input);
      bse_object_proxy_notifies (trackbus, self, "notify::inputs");
      g_object_notify (self, "inputs");
      // FIXME: g_object_notify (osource, "outputs");
    }
  return error;
}

BseErrorType
bse_bus_disconnect (BseBus  *self,
                    BseItem *trackbus)
{
  BseSource *osource;
  if (BSE_IS_TRACK (trackbus))
    osource = bse_track_get_output (BSE_TRACK (trackbus));
  else if (BSE_IS_BUS (trackbus))
    osource = BSE_SOURCE (trackbus);
  else
    return BSE_ERROR_SOURCE_TYPE_INVALID;
  if (!osource || !self->summation || !sfi_ring_find (self->inputs, trackbus))
    return BSE_ERROR_SOURCE_PARENT_MISMATCH;
  bse_object_unproxy_notifies (trackbus, self, "notify::inputs");
  bse_item_cross_unlink (BSE_ITEM (self), BSE_ITEM (trackbus), bus_uncross_input);
  self->inputs = sfi_ring_remove (self->inputs, trackbus);
  BseErrorType error1 = bse_source_unset_input (self->summation, 0, osource, 0);
  BseErrorType error2 = bse_source_unset_input (self->summation, 1, osource, 1);
  g_object_notify (self, "inputs");
  // FIXME: g_object_notify (osource, "outputs");
  return error1 ? error1 : error2;
}

SfiRing*
bse_bus_list_inputs (BseBus *self)
{
  return sfi_ring_copy (self->inputs);
}

SfiRing*
bse_bus_list_outputs (BseBus *self)
{
  BseItem *parent = BSE_ITEM (self)->parent;
  SfiRing *outputs = NULL;
  if (BSE_IS_SONG (parent))
    {
      BseSong *song = BSE_SONG (parent);
      SfiRing *ring;
      for (ring = song->busses; ring; ring = sfi_ring_walk (ring, song->busses))
        if (ring->data != self && sfi_ring_find (BSE_BUS (ring->data)->inputs, self))
          outputs = sfi_ring_append (outputs, ring->data);
    }
  return outputs;
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
      BseErrorType cerror;
      if (osource)
        cerror = bse_bus_connect (self, BSE_ITEM (osource));
      else
        cerror = BSE_ERROR_SOURCE_NO_SUCH_MODULE;
      if (cerror)
        bse_storage_warn (storage,
                          "failed to add input \"%s\" to mixer bus \"%s\": %s",
                          osource ? BSE_OBJECT_UNAME (osource) : ":<NULL>:",
                          BSE_OBJECT_UNAME (self),
                          bse_error_blurb (cerror));
    }
}

static SfiTokenType
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
      BseSource *osource = sfi_ring_pop_head (&inputs);
      bse_storage_break (storage);
      bse_storage_printf (storage, "(bus-input ");
      bse_storage_put_item_link (storage, BSE_ITEM (self), BSE_ITEM (osource));
      bse_storage_printf (storage, ")");
    }
}

static void
bse_bus_class_init (BseBusClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseItemClass *item_class = BSE_ITEM_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint channel_id;
  
  bus_parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_bus_set_property;
  gobject_class->get_property = bse_bus_get_property;
  gobject_class->dispose = bse_bus_dispose;
  gobject_class->finalize = bse_bus_finalize;
  
  object_class->editable_property = bse_bus_editable_property;
  object_class->store_private = bus_store_private;
  object_class->restore_private = bus_restore_private;

  item_class->set_parent = bse_bus_set_parent;
  item_class->get_candidates = bse_bus_get_candidates;
  
  source_class->prepare = bse_bus_prepare;
  source_class->context_create = bse_bus_context_create;
  source_class->context_connect = bse_bus_context_connect;
  source_class->reset = bse_bus_reset;
  
  bse_object_class_add_param (object_class, _("Adjustments"), PROP_MUTE,
                              sfi_pspec_bool ("mute", _("Mute"), _("Mute: turn off the bus volume"), FALSE, SFI_PARAM_STANDARD));
  bse_object_class_add_param (object_class, _("Adjustments"), PROP_SOLO,
                              sfi_pspec_bool ("solo", _("Solo"), _("Solo: mute all other busses"), FALSE, SFI_PARAM_STANDARD));
  bse_object_class_add_param (object_class, _("Adjustments"), PROP_SYNC,
                              sfi_pspec_bool ("sync", _("Sync"), _("Sync: enforce the same volume left and right"), TRUE, SFI_PARAM_STANDARD));
  bse_object_class_add_param (object_class, _("Adjustments"), PROP_LEFT_VOLUME_dB,
			      sfi_pspec_real ("left-volume-db", _("Left Volume [dB]"), _("Volume adjustment of left bus channel"),
                                              0, BSE_MIN_VOLUME_dB, BSE_MAX_VOLUME_dB,
					      0.1, SFI_PARAM_GUI ":dial"));
  bse_object_class_add_param (object_class, _("Adjustments"), PROP_RIGHT_VOLUME_dB,
			      sfi_pspec_real ("right-volume-db", _("Right Volume [dB]"), _("Volume adjustment of right bus channel"),
                                              0, BSE_MIN_VOLUME_dB, BSE_MAX_VOLUME_dB,
					      0.1, SFI_PARAM_GUI ":dial"));
  bse_object_class_add_param (object_class, _("Signal Inputs"),
                              PROP_INPUTS,
                              /* SYNC: type partitions determine the order of displayed objects */
                              bse_param_spec_boxed ("inputs", _("Input Signals"),
                                                    /* TRANSLATORS: the "tracks and busses" order in this tooltip needs
                                                     * to be preserved to match the GUI order of displayed objects.
                                                     */
                                                    _("Synthesis signals (from tracks and busses) used as bus input"),
                                                    BSE_TYPE_ITEM_SEQ, SFI_PARAM_GUI ":item-sequence"));
  bse_object_class_add_param (object_class, NULL, PROP_SNET, bse_param_spec_object ("snet", NULL, NULL, BSE_TYPE_CSYNTH, SFI_PARAM_READWRITE ":skip-undo"));
  bse_object_class_add_param (object_class, _("Internals"),
			      PROP_MASTER_OUTPUT,
			      sfi_pspec_bool ("master-output", _("Master Output"), NULL,
                                              FALSE, SFI_PARAM_STORAGE ":skip-default"));

  channel_id = bse_source_class_add_ichannel (source_class, "left-audio-in", _("Left Audio In"), _("Left channel input"));
  g_assert (channel_id == BSE_BUS_ICHANNEL_LEFT);
  channel_id = bse_source_class_add_ichannel (source_class, "right-audio-in", _("Right Audio In"), _("Right channel input"));
  g_assert (channel_id == BSE_BUS_ICHANNEL_RIGHT);
  channel_id = bse_source_class_add_ochannel (source_class, "left-audio-out", _("Left Audio Out"), _("Left channel output"));
  g_assert (channel_id == BSE_BUS_OCHANNEL_LEFT);
  channel_id = bse_source_class_add_ochannel (source_class, "right-audio-out", _("Right Audio Out"), _("Right channel output"));
  g_assert (channel_id == BSE_BUS_OCHANNEL_RIGHT);
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
                                         &bus_info);
  return type;
}
