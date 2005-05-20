/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2002-2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bsetrack.h"

#include "bseglobals.h"
#include "bsestorage.h"
#include "bsecsynth.h"
#include "bsewave.h"
#include "bsepart.h"
#include "bsebus.h"
#include "bsemain.h"
#include "gslcommon.h"
#include "bsesubsynth.h"
#include "bseproject.h"
#include "bsesong.h"
#include "bsemidivoice.h"
#include "bsemidireceiver.h"
#include "bsewaverepo.h"
#include <string.h>

static SFI_MSG_TYPE_DEFINE (debug_xref, "xref", SFI_MSG_DEBUG, NULL);
#define XREF_DEBUG(...) sfi_debug (debug_xref, __VA_ARGS__)

#define upper_power2(uint_n)	sfi_alloc_upper_power2 (MAX ((uint_n), 4))
#define parse_or_return		bse_storage_scanner_parse_or_return
#define peek_or_return		bse_storage_scanner_peek_or_return


enum {
  PROP_0,
  PROP_MUTED,
  PROP_SNET,
  PROP_WAVE,
  PROP_MIDI_CHANNEL,
  PROP_N_VOICES,
  PROP_PNET,
  PROP_OUTPUTS
};

/* --- prototypes --- */
static void         bse_track_class_init          (BseTrackClass *class);
static void         bse_track_init                (BseTrack      *self);
static void         bse_track_dispose             (GObject       *object);
static void         bse_track_finalize            (GObject       *object);
static void         bse_track_set_property        (GObject       *object,
                                                   guint          param_id,
                                                   const GValue  *value,
                                                   GParamSpec    *pspec);
static void         bse_track_get_property        (GObject       *object,
                                                   guint          param_id,
                                                   GValue        *value,
                                                   GParamSpec    *pspec);
static void         bse_track_store_private       (BseObject     *object,
                                                   BseStorage    *storage);
static SfiTokenType bse_track_restore_private     (BseObject     *object,
                                                   BseStorage    *storage,
                                                   GScanner      *scanner);
static void         bse_track_update_midi_channel (BseTrack      *self);


/* --- variables --- */
static GTypeClass *parent_class = NULL;
static guint	   signal_changed = 0;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseTrack)
{
  static const GTypeInfo track_info = {
    sizeof (BseTrackClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_track_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseTrack),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_track_init,
  };
  
  return bse_type_register_static (BSE_TYPE_CONTEXT_MERGER,
				   "BseTrack",
				   "BSE track type",
				   &track_info);
}

static gulong
alloc_id_above (guint n)
{
  gulong tmp, id = bse_id_alloc ();
  if (id > n)
    return id;
  tmp = id;
  id = alloc_id_above (n);
  bse_id_free (tmp);
  return id;
}

static void
bse_track_init (BseTrack *self)
{
  BSE_OBJECT_SET_FLAGS (self, BSE_SOURCE_FLAG_PRIVATE_INPUTS);
  self->snet = NULL;
  self->pnet = NULL;
  self->max_voices = 16;
  self->muted_SL = FALSE;
  self->n_entries_SL = 0;
  self->entries_SL = g_renew (BseTrackEntry, NULL, upper_power2 (self->n_entries_SL));
  self->channel_id = alloc_id_above (BSE_MIDI_MAX_CHANNELS);
  self->midi_channel_SL = self->channel_id;
  self->track_done_SL = FALSE;
}

static void
bse_track_dispose (GObject *object)
{
  BseTrack *self = BSE_TRACK (object);

  /* we may assert removal here, since if these assertions fail,
   * our parent (BseSong) doesn't properly implement track support
   */
  g_assert (self->sub_synth == NULL);
  
  /* check uncrossed references */
  g_assert (self->snet == NULL);
  g_assert (self->pnet == NULL);
  g_assert (self->n_entries_SL == 0);
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (object);

  g_assert (self->bus_outputs == NULL);
}

static void
bse_track_finalize (GObject *object)
{
  BseTrack *self = BSE_TRACK (object);

  g_assert (self->bus_outputs == NULL);

  g_assert (self->n_entries_SL == 0);
  g_free (self->entries_SL);
  bse_id_free (self->channel_id);

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void	track_uncross_part	(BseItem *owner,
					 BseItem *ref_item);

static BseTrackEntry*
track_add_entry (BseTrack *self,
		 guint     index,
		 guint     tick,
		 BsePart  *part)
{
  guint n, size;

  g_return_val_if_fail (index <= self->n_entries_SL, NULL);
  if (index > 0)
    g_return_val_if_fail (self->entries_SL[index - 1].tick < tick, NULL);
  if (index < self->n_entries_SL)
    g_return_val_if_fail (self->entries_SL[index].tick > tick, NULL);

  BSE_SEQUENCER_LOCK ();
  n = self->n_entries_SL++;
  size = upper_power2 (self->n_entries_SL);
  if (size > upper_power2 (n))
    self->entries_SL = g_renew (BseTrackEntry, self->entries_SL, size);
  g_memmove (self->entries_SL + index + 1, self->entries_SL + index, (n - index) * sizeof (self->entries_SL[0]));
  self->entries_SL[index].tick = tick;
  self->entries_SL[index].id = bse_id_alloc ();
  self->entries_SL[index].part = part;
  self->track_done_SL = FALSE;	/* let sequencer recheck if playing */
  BSE_SEQUENCER_UNLOCK ();
  bse_item_cross_link (BSE_ITEM (self), BSE_ITEM (part), track_uncross_part);
  XREF_DEBUG ("cross-link: %p %p", self, part);
  bse_object_proxy_notifies (part, self, "changed");
  bse_object_reemit_signal (part, "notify::last-tick", self, "changed");
  return self->entries_SL + index;
}

static void
track_delete_entry (BseTrack *self,
		    guint     index)
{
  guint n;
  BsePart *part;

  g_return_if_fail (index < self->n_entries_SL);

  part = self->entries_SL[index].part;
  bse_object_remove_reemit (part, "notify::last-tick", self, "changed");
  bse_object_unproxy_notifies (part, self, "changed");
  XREF_DEBUG ("cross-unlink: %p %p", self, part);
  bse_item_cross_unlink (BSE_ITEM (self), BSE_ITEM (part), track_uncross_part);
  BSE_SEQUENCER_LOCK ();
  n = self->n_entries_SL--;
  bse_id_free (self->entries_SL[index].id);
  g_memmove (self->entries_SL + index, self->entries_SL + index + 1, (self->n_entries_SL - index) * sizeof (self->entries_SL[0]));
  BSE_SEQUENCER_UNLOCK ();
}

static BseTrackEntry*
track_lookup_entry (BseTrack *self,
		    guint     tick)
{
  BseTrackEntry *nodes = self->entries_SL;
  guint n = self->n_entries_SL, offs = 0, i = 0;
  
  while (offs < n)
    {
      gint cmp;
      i = (offs + n) >> 1;
      cmp = tick > nodes[i].tick ? +1 : tick < nodes[i].tick ? -1 : 0;
      if (!cmp)
	return nodes + i;
      else if (cmp < 0)
	n = i;
      else /* (cmp > 0) */
	offs = i + 1;
    }
  
  /* return the closest entry with tick <= requested tick if possible */
  if (!self->n_entries_SL)
    return NULL;
  if (nodes[i].tick > tick)
    return i > 0 ? nodes + i - 1 : NULL;	/* previous entry */
  else
    return nodes + i;				/* closest match */
}

static void
track_uncross_part (BseItem *owner,
		    BseItem *item)
{
  BseTrack *self = BSE_TRACK (owner);
  BsePart *part = BSE_PART (item);
  guint i;
  for (i = 0; i < self->n_entries_SL; i++)
    if (self->entries_SL[i].part == part)
      {
        guint tick = self->entries_SL[i].tick;
	XREF_DEBUG ("uncrossing[start]: %p %p (%d)", self, part, tick);
        /* delete track via procedure so deletion is recorded to undo */
        bse_item_exec_void (owner, "remove-tick", tick);
	XREF_DEBUG ("uncrossing[done]: %p %p (%d)", self, part, tick);
	return;
      }
}

static void
bse_track_get_candidates (BseItem               *item,
                          guint                  param_id,
                          BsePropertyCandidates *pc,
                          GParamSpec            *pspec)
{
  BseTrack *self = BSE_TRACK (item);
  switch (param_id)
    {
      BseProject *project;
      SfiRing *ring;
    case PROP_WAVE:
      bse_property_candidate_relabel (pc, _("Available Waves"), _("List of available waves to choose as track instrument"));
      project = bse_item_get_project (item);
      if (project)
	{
	  BseWaveRepo *wrepo = bse_project_get_wave_repo (project);
	  bse_item_gather_items_typed (BSE_ITEM (wrepo), pc->items, BSE_TYPE_WAVE, BSE_TYPE_WAVE_REPO, FALSE);
	}
      break;
    case PROP_SNET:
      bse_property_candidate_relabel (pc, _("Available Synthesizers"), _("List of available synthesis networks to choose a track instrument from"));
      bse_item_gather_items_typed (item, pc->items, BSE_TYPE_CSYNTH, BSE_TYPE_PROJECT, FALSE);
      break;
    case PROP_PNET:
      bse_property_candidate_relabel (pc, _("Available Postprocessors"), _("List of available synthesis networks to choose a postprocessor from"));
      bse_item_gather_items_typed (item, pc->items, BSE_TYPE_CSYNTH, BSE_TYPE_PROJECT, FALSE);
      break;
    case PROP_OUTPUTS:
      bse_property_candidate_relabel (pc, _("Available Outputs"), _("List of available mixer busses to be used as track output"));
      bse_bus_or_track_list_output_candidates (BSE_ITEM (self), pc->items);
      /* remove existing outputs */
      for (ring = self->bus_outputs; ring; ring = sfi_ring_walk (ring, self->bus_outputs))
        bse_item_seq_remove (pc->items, ring->data);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
track_uncross_snet (BseItem *owner,
		    BseItem *ref_item)
{
  BseTrack *self = BSE_TRACK (owner);
  bse_item_set (self, "snet", NULL, NULL);
}

static void
track_uncross_pnet (BseItem *owner,
		    BseItem *ref_item)
{
  BseTrack *self = BSE_TRACK (owner);
  bse_item_set (self, "pnet", NULL, NULL);
}

static void
track_uncross_wave (BseItem *owner,
		    BseItem *ref_item)
{
  BseTrack *self = BSE_TRACK (owner);
  bse_item_set (self, "wave", NULL, NULL);
}

static void
create_wnet (BseTrack *self,
	     BseWave  *wave)
{
  g_return_if_fail (self->wnet == NULL);

  const gchar *play_type = bse_xinfos_get_value (wave->xinfos, "play-type");
  const gchar *synthesis_network = play_type ? play_type : "wave-mono";

  self->wnet = bse_project_create_intern_synth (bse_item_get_project (BSE_ITEM (self)),
						synthesis_network,
						BSE_TYPE_SNET);

  bse_item_cross_link (BSE_ITEM (self), BSE_ITEM (self->wnet), track_uncross_wave);

  if (self->sub_synth)
    g_object_set (self->sub_synth, /* no undo */
		  "snet", self->wnet,
		  NULL);

  if (strcmp (synthesis_network, "wave-mono") == 0)
    {
      g_object_set (bse_container_resolve_upath (BSE_CONTAINER (self->wnet), "wave-osc"), /* no undo */
						 "wave", wave,
					         NULL);
    }
  else if (strcmp (synthesis_network, "gus-patch") == 0)
    {
      g_object_set (bse_container_resolve_upath (BSE_CONTAINER (self->wnet), "wave-osc"), /* no undo */
						 "wave", wave,
					         NULL);
      g_object_set (bse_container_resolve_upath (BSE_CONTAINER (self->wnet), "gus-patch-envelope"), /* no undo */
						 "wave", wave,
					         NULL);
    }
  else
    {
      g_warning ("track: waves with the play-type \"%s\" are not supported by this version of beast\n",
	         synthesis_network);
    }

}

static void
clear_snet_and_wave (BseTrack *self)
{
  g_return_if_fail (!self->sub_synth || !BSE_SOURCE_PREPARED (self->sub_synth));

  if (self->sub_synth)
    g_object_set (self->sub_synth, /* no undo */
		  "snet", NULL,
		  NULL);
  if (self->snet)
    {
      bse_object_unproxy_notifies (self->snet, self, "changed");
      bse_item_cross_unlink (BSE_ITEM (self), BSE_ITEM (self->snet), track_uncross_snet);
      self->snet = NULL;
      g_object_notify (self, "snet");
    }
  if (self->wave)
    {
      bse_object_unproxy_notifies (self->wave, self, "changed");
      bse_item_cross_unlink (BSE_ITEM (self), BSE_ITEM (self->wave), track_uncross_wave);
      self->wave = NULL;
      g_object_notify (self, "wave");
    }
  if (self->wnet)
    {
      BseSNet *wnet = self->wnet;
      bse_item_cross_unlink (BSE_ITEM (self), BSE_ITEM (self->wnet), track_uncross_wave);
      self->wnet = NULL;
      bse_container_remove_item (BSE_CONTAINER (bse_item_get_project (BSE_ITEM (self))), BSE_ITEM (wnet));
    }
}

static void
bse_track_set_property (GObject      *object,
			guint         param_id,
			const GValue *value,
			GParamSpec   *pspec)
{
  BseTrack *self = BSE_TRACK (object);
  
  switch (param_id)
    {
      guint i;
    case PROP_MUTED:
      BSE_SEQUENCER_LOCK ();
      self->muted_SL = sfi_value_get_bool (value);
      BSE_SEQUENCER_UNLOCK ();
      break;
    case PROP_SNET:
      if (!self->sub_synth || !BSE_SOURCE_PREPARED (self))
	{
	  BseSNet *snet = bse_value_get_object (value);
	  if (snet || self->snet)
	    {
	      clear_snet_and_wave (self);
	      self->snet = snet;
	      if (self->snet)
		{
		  bse_item_cross_link (BSE_ITEM (self), BSE_ITEM (self->snet), track_uncross_snet);
		  bse_object_proxy_notifies (self->snet, self, "changed");
		}
	      if (self->sub_synth)
		g_object_set (self->sub_synth, /* no undo */
			      "snet", self->snet,
			      NULL);
	    }
	}
      break;
    case PROP_WAVE:
      if (!self->sub_synth || !BSE_SOURCE_PREPARED (self->sub_synth))
	{
	  BseWave *wave = bse_value_get_object (value);
	  if (wave || self->wave)
	    {
	      clear_snet_and_wave (self);

	      self->wave = wave;
	      if (self->wave)
		{
		  create_wnet (self, wave);
		  bse_item_cross_link (BSE_ITEM (self), BSE_ITEM (self->wave), track_uncross_wave);
		  bse_object_proxy_notifies (self->wave, self, "changed");
		}
	    }
	}
      break;
    case PROP_N_VOICES:
      if (!self->postprocess || !BSE_SOURCE_PREPARED (self->postprocess))
        self->max_voices = sfi_value_get_int (value);
      break;
    case PROP_MIDI_CHANNEL:
      i = sfi_value_get_int (value);
      if (i != self->midi_channel_SL && !BSE_SOURCE_PREPARED (self))
        {
          self->midi_channel_SL = i > 0 ? i : self->channel_id;
          bse_track_update_midi_channel (self);
        }
      break;
    case PROP_PNET:
      if (!self->postprocess || !BSE_SOURCE_PREPARED (self))
	{
          if (self->pnet)
            {
              bse_object_unproxy_notifies (self->pnet, self, "notify::pnet");
              bse_item_cross_unlink (BSE_ITEM (self), BSE_ITEM (self->pnet), track_uncross_pnet);
              self->pnet = NULL;
            }
          self->pnet = bse_value_get_object (value);
          if (self->pnet)
            {
              bse_item_cross_link (BSE_ITEM (self), BSE_ITEM (self->pnet), track_uncross_pnet);
              bse_object_proxy_notifies (self->pnet, self, "notify::pnet");
            }
          if (self->postprocess)
            g_object_set (self->postprocess, /* no undo */
                          "snet", self->pnet,
                          NULL);
	}
      break;
    case PROP_OUTPUTS:
      bse_bus_or_track_set_outputs (BSE_ITEM (self), g_value_get_boxed (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_track_get_property (GObject    *object,
			guint       param_id,
			GValue     *value,
			GParamSpec *pspec)
{
  BseTrack *self = BSE_TRACK (object);
  
  switch (param_id)
    {
      BseItemSeq *iseq;
      SfiRing *ring;
    case PROP_MUTED:
      sfi_value_set_bool (value, self->muted_SL);
      break;
    case PROP_SNET:
      bse_value_set_object (value, self->snet);
      break;
    case PROP_PNET:
      bse_value_set_object (value, self->pnet);
      break;
    case PROP_OUTPUTS:
      iseq = bse_item_seq_new();
      for (ring = self->bus_outputs; ring; ring = sfi_ring_walk (ring, self->bus_outputs))
        bse_item_seq_append (iseq, ring->data);
      g_value_take_boxed (value, iseq);
      break;
    case PROP_WAVE:
      bse_value_set_object (value, self->wave);
      break;
    case PROP_N_VOICES:
      sfi_value_set_int (value, self->max_voices);
      break;
    case PROP_MIDI_CHANNEL:
      sfi_value_set_int (value, self->midi_channel_SL <= BSE_MIDI_MAX_CHANNELS ? self->midi_channel_SL : 0);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

guint
bse_track_insert_part (BseTrack *self,
		       guint     tick,
		       BsePart  *part)
{
  BseTrackEntry *entry;

  g_return_val_if_fail (BSE_IS_TRACK (self), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_IS_PART (part), BSE_ERROR_INTERNAL);

  entry = track_lookup_entry (self, tick);
  if (entry && entry->tick == tick)
    return 0;
  else
    {
      gint i = entry ? entry - self->entries_SL : 0;
      entry = track_add_entry (self, entry ? i + 1 : 0, tick, part);
    }
  bse_part_links_changed (part);
  g_signal_emit (self, signal_changed, 0);
  return entry ? entry->id : 0;
}

void
bse_track_remove_tick (BseTrack *self,
		       guint     tick)
{
  BseTrackEntry *entry;

  g_return_if_fail (BSE_IS_TRACK (self));

  entry = track_lookup_entry (self, tick);
  if (entry && entry->tick == tick)
    {
      BsePart *part = entry->part;
      track_delete_entry (self, entry - self->entries_SL);
      bse_part_links_changed (part);
      g_signal_emit (self, signal_changed, 0);
    }
}

static BseTrackPartSeq*
bse_track_list_parts_intern (BseTrack *self,
                             BsePart  *part)
{
  BseItem *item = BSE_ITEM (self);
  BseSong *song = NULL;
  if (BSE_IS_SONG (item->parent))
    song = BSE_SONG (item->parent);
  BseSongTiming timing;
  bse_song_timing_get_default (&timing);
  BseTrackPartSeq *tps = bse_track_part_seq_new ();
  gint i;
  for (i = 0; i < self->n_entries_SL; i++)
    {
      BseTrackEntry *entry = self->entries_SL + i;
      if (entry->part && (entry->part == part || !part))
	{
	  BseTrackPart tp = { 0, };
	  tp.tick = entry->tick;
	  tp.part = entry->part;
	  if (song)
	    bse_song_get_timing (song, tp.tick, &timing);
	  tp.duration = MAX (timing.tpt, entry->part->last_tick_SL);
	  if (i + 1 < self->n_entries_SL)
	    tp.duration = MIN (tp.duration, entry[1].tick - entry->tick);
	  bse_track_part_seq_append (tps, &tp);
	}
    }
  return tps;
}

BseTrackPartSeq*
bse_track_list_parts (BseTrack *self)
{
  g_return_val_if_fail (BSE_IS_TRACK (self), NULL);
  return bse_track_list_parts_intern (self, NULL);
}

BseTrackPartSeq*
bse_track_list_part (BseTrack *self,
                     BsePart  *part)
{
  g_return_val_if_fail (BSE_IS_TRACK (self), NULL);
  g_return_val_if_fail (BSE_IS_PART (part), NULL);
  return bse_track_list_parts_intern (self, part);
}

gboolean
bse_track_find_part (BseTrack *self,
		     BsePart  *part,
		     guint    *start_p)
{
  guint i;

  g_return_val_if_fail (BSE_IS_TRACK (self), FALSE);
  g_return_val_if_fail (BSE_IS_PART (part), FALSE);

  for (i = 0; i < self->n_entries_SL; i++)
    if (self->entries_SL[i].part == part)
      {
	if (start_p)
	  *start_p = self->entries_SL[i].tick;
	return TRUE;
      }
  return FALSE;
}

BseTrackEntry*
bse_track_lookup_tick (BseTrack               *self,
		       guint                   tick)
{
  BseTrackEntry *entry;

  g_return_val_if_fail (BSE_IS_TRACK (self), NULL);

  entry = track_lookup_entry (self, tick);
  if (entry && entry->tick == tick)
    return entry;
  return NULL;
}

BseTrackEntry*
bse_track_find_link (BseTrack *self,
                     guint     id)
{
  guint i;

  g_return_val_if_fail (BSE_IS_TRACK (self), NULL);

  for (i = 0; i < self->n_entries_SL; i++)
    if (self->entries_SL[i].id == id)
      return self->entries_SL + i;
  return NULL;
}

BsePart*
bse_track_get_part_SL (BseTrack *self,
		       guint     tick,
		       guint    *start,
		       guint    *next)
{
  BseTrackEntry *entry;

  g_return_val_if_fail (BSE_IS_TRACK (self), NULL);

  /* we return the nearest part with start <= tick and
   * set *next to the start of the following part if any
   */

  entry = track_lookup_entry (self, tick);
  if (entry)
    {
      guint i = entry - self->entries_SL;
      if (i + 1 < self->n_entries_SL)
	*next = self->entries_SL[i + 1].tick;
      else
	*next = 0;	/* no next part */
      *start = entry->tick;
      return entry->part;
    }
  *start = 0;
  *next = self->n_entries_SL ? self->entries_SL[0].tick : 0;
  return NULL;
}

void
bse_track_add_modules (BseTrack        *self,
		       BseContainer    *container,
                       BseMidiReceiver *midi_receiver)
{
  g_return_if_fail (BSE_IS_TRACK (self));
  g_return_if_fail (BSE_IS_CONTAINER (container));
  g_return_if_fail (self->sub_synth == NULL);
  g_return_if_fail (midi_receiver != NULL);

  /* midi voice input */
  self->voice_input = bse_container_new_child (container, BSE_TYPE_MIDI_VOICE_INPUT, NULL);
  bse_item_set_internal (self->voice_input, TRUE);
  
  /* sub synth */
  self->sub_synth = bse_container_new_child_bname (container, BSE_TYPE_SUB_SYNTH, "Track-Instrument",
                                                   "in_port_1", "frequency",
                                                   "in_port_2", "gate",
                                                   "in_port_3", "velocity",
                                                   "in_port_4", "aftertouch",
                                                   "out_port_1", "left-audio",
                                                   "out_port_2", "right-audio",
                                                   "out_port_3", "unused",
                                                   "out_port_4", "synth-done",
                                                   "snet", self->snet,
                                                   NULL);
  bse_item_set_internal (self->sub_synth, TRUE);
  
  /* voice input <-> sub-synth */
  bse_source_must_set_input (self->sub_synth, 0,
			     self->voice_input, BSE_MIDI_VOICE_INPUT_OCHANNEL_FREQUENCY);
  bse_source_must_set_input (self->sub_synth, 1,
			     self->voice_input, BSE_MIDI_VOICE_INPUT_OCHANNEL_GATE);
  bse_source_must_set_input (self->sub_synth, 2,
			     self->voice_input, BSE_MIDI_VOICE_INPUT_OCHANNEL_VELOCITY);
  bse_source_must_set_input (self->sub_synth, 3,
			     self->voice_input, BSE_MIDI_VOICE_INPUT_OCHANNEL_AFTERTOUCH);
  
  /* midi voice switch */
  self->voice_switch = bse_container_new_child (container, BSE_TYPE_MIDI_VOICE_SWITCH, NULL);
  bse_item_set_internal (self->voice_switch, TRUE);
  bse_midi_voice_input_set_voice_switch (BSE_MIDI_VOICE_INPUT (self->voice_input), BSE_MIDI_VOICE_SWITCH (self->voice_switch));

  /* sub-synth <-> voice switch */
  bse_source_must_set_input (self->voice_switch, BSE_MIDI_VOICE_SWITCH_ICHANNEL_LEFT,
			     self->sub_synth, 0);
  bse_source_must_set_input (self->voice_switch, BSE_MIDI_VOICE_SWITCH_ICHANNEL_RIGHT,
			     self->sub_synth, 1);
  bse_source_must_set_input (self->voice_switch, BSE_MIDI_VOICE_SWITCH_ICHANNEL_DISCONNECT,
			     self->sub_synth, 3);
  
  /* midi voice switch <-> context merger */
  bse_source_must_set_input (BSE_SOURCE (self), 0,
			     self->voice_switch, BSE_MIDI_VOICE_SWITCH_OCHANNEL_LEFT);
  bse_source_must_set_input (BSE_SOURCE (self), 1,
			     self->voice_switch, BSE_MIDI_VOICE_SWITCH_OCHANNEL_RIGHT);
  
  /* postprocess */
  self->postprocess = bse_container_new_child_bname (container, BSE_TYPE_SUB_SYNTH, "Track-Postprocess", NULL);
  bse_item_set_internal (self->postprocess, TRUE);
  bse_sub_synth_set_null_shortcut (BSE_SUB_SYNTH (self->postprocess), TRUE);
  
  /* context merger <-> postprocess */
  bse_source_must_set_input (self->postprocess, 0, BSE_SOURCE (self), 0);
  bse_source_must_set_input (self->postprocess, 1, BSE_SOURCE (self), 1);

  /* propagate midi channel to modules */
  bse_track_update_midi_channel (self);
}

BseSource*
bse_track_get_output (BseTrack *self)
{
  return self->postprocess;
}

guint
bse_track_get_last_tick (BseTrack *self)
{
  int last_tick = 0;

  /* find last part */
  BsePart *part = NULL;
  guint i, offset = 0;
  for (i = 0; i < self->n_entries_SL; i++)
    if (self->entries_SL[i].part)
      {
        part = self->entries_SL[i].part;
        offset = self->entries_SL[i].tick;
      }
  if (part)
    {
      BseItem *item = BSE_ITEM (self);
      BseSongTiming timing;
      g_object_get (part, "last-tick", &last_tick, NULL);
      if (BSE_IS_SONG (item->parent))
        bse_song_get_timing (BSE_SONG (item->parent), offset, &timing);
      else
        bse_song_timing_get_default (&timing);
      last_tick = MAX (last_tick, timing.tpt);  /* MAX duration with tact size, see bse_track_list_parts() */
      last_tick += offset;
    }
  else
    last_tick += 1;     /* always return one after */
  
  return last_tick;
}

static void
bse_track_update_midi_channel (BseTrack *self)
{
  if (self->voice_switch)
    {
      bse_sub_synth_set_midi_channel (BSE_SUB_SYNTH (self->sub_synth), self->midi_channel_SL);
      bse_sub_synth_set_midi_channel (BSE_SUB_SYNTH (self->postprocess), self->midi_channel_SL);
      bse_midi_voice_switch_set_midi_channel (BSE_MIDI_VOICE_SWITCH (self->voice_switch), self->midi_channel_SL);
    }
}

static void
bse_track_context_create (BseSource      *source,
                          guint           context_handle,
                          BseTrans       *trans)
{
  BseTrack *self = BSE_TRACK (source);
  BseMidiContext mcontext = bse_snet_get_midi_context (bse_item_get_snet (BSE_ITEM (self)), context_handle);
  if (self->snet || self->wave)
    bse_midi_receiver_channel_enable_poly (mcontext.midi_receiver, self->midi_channel_SL);
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

static void
bse_track_context_dismiss (BseSource      *source,
                           guint           context_handle,
                           BseTrans       *trans)
{
  BseTrack *self = BSE_TRACK (source);
  BseMidiContext mcontext = bse_snet_get_midi_context (bse_item_get_snet (BSE_ITEM (self)), context_handle);
  if (self->snet || self->wave)
    bse_midi_receiver_channel_disable_poly (mcontext.midi_receiver, self->midi_channel_SL);
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_dismiss (source, context_handle, trans);
}

void
bse_track_remove_modules (BseTrack     *self,
			  BseContainer *container)
{
  g_return_if_fail (BSE_IS_TRACK (self));
  g_return_if_fail (BSE_IS_CONTAINER (container));
  g_return_if_fail (self->sub_synth != NULL);
  
  bse_container_remove_item (container, BSE_ITEM (self->sub_synth));
  self->sub_synth = NULL;
  bse_container_remove_item (container, BSE_ITEM (self->voice_input));
  self->voice_input = NULL;
  bse_container_remove_item (container, BSE_ITEM (self->voice_switch));
  self->voice_switch = NULL;
  bse_container_remove_item (container, BSE_ITEM (self->postprocess));
  self->postprocess = NULL;
}

void
bse_track_clone_voices (BseTrack       *self,
			BseSNet        *snet,
			guint           context,
                        BseMidiContext  mcontext,
			BseTrans       *trans)
{
  guint i;
  
  g_return_if_fail (BSE_IS_TRACK (self));
  g_return_if_fail (BSE_IS_SNET (snet));
  g_return_if_fail (trans != NULL);

  for (i = 0; i < self->max_voices - 1; i++)
    bse_snet_context_clone_branch (snet, context, BSE_SOURCE (self), mcontext, trans);
}

static void
bse_track_store_private (BseObject  *object,
			 BseStorage *storage)
{
  BseTrack *self = BSE_TRACK (object);
  BseItem *item = BSE_ITEM (self);
  guint i;

  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->store_private)
    BSE_OBJECT_CLASS (parent_class)->store_private (object, storage);

  for (i = 0; i < self->n_entries_SL; i++)
    {
      BseTrackEntry *e = self->entries_SL + i;
      if (e->part)
	{
	  bse_storage_break (storage);
	  bse_storage_printf (storage, "(insert-part %u ",
			      e->tick);
	  bse_storage_put_item_link (storage, item, BSE_ITEM (e->part));
	  bse_storage_putc (storage, ')');
	}
    }
}

static void
part_link_resolved (gpointer        data,
		    BseStorage     *storage,
		    BseItem        *from_item,
		    BseItem        *to_item,
		    const gchar    *error)
{
  BseTrack *self = BSE_TRACK (from_item);

  if (error)
    bse_storage_warn (storage, error);
  else if (!BSE_IS_PART (to_item))
    bse_storage_warn (storage, "skipping invalid part reference: %s", bse_object_debug_name (to_item));
  else if (to_item->parent != from_item->parent)
    bse_storage_warn (storage, "skipping out-of-branch part reference: %s", bse_object_debug_name (to_item));
  else
    {
      guint tick = GPOINTER_TO_UINT (data);
      if (bse_track_insert_part (self, tick, BSE_PART (to_item)) < 1)
	bse_storage_warn (storage, "failed to insert part reference to %s",
			  bse_object_debug_name (to_item));
    }
}

static SfiTokenType
bse_track_restore_private (BseObject  *object,
			   BseStorage *storage,
                           GScanner   *scanner)
{
  BseTrack *self = BSE_TRACK (object);

  if (g_scanner_peek_next_token (scanner) == G_TOKEN_IDENTIFIER &&
      bse_string_equals ("insert-part", scanner->next_value.v_identifier))
    {
      GTokenType token;
      guint tick;
      
      g_scanner_get_next_token (scanner);       /* eat quark */
      
      parse_or_return (scanner, G_TOKEN_INT);
      tick = scanner->value.v_int64;
      token = bse_storage_parse_item_link (storage, BSE_ITEM (self), part_link_resolved, GUINT_TO_POINTER (tick));
      if (token != G_TOKEN_NONE)
	return token;
      parse_or_return (scanner, ')');
      return G_TOKEN_NONE;
    }
  else /* chain parent class' handler */
    return BSE_OBJECT_CLASS (parent_class)->restore_private (object, storage, scanner);
}

static void
bse_track_class_init (BseTrackClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseItemClass *item_class = BSE_ITEM_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_track_set_property;
  gobject_class->get_property = bse_track_get_property;
  gobject_class->dispose = bse_track_dispose;
  gobject_class->finalize = bse_track_finalize;

  object_class->store_private = bse_track_store_private;
  object_class->restore_private = bse_track_restore_private;
  
  item_class->get_candidates = bse_track_get_candidates;
  
  source_class->context_create = bse_track_context_create;
  source_class->context_dismiss = bse_track_context_dismiss;

  bse_source_class_inherit_channels (BSE_SOURCE_CLASS (class));

  bse_object_class_add_param (object_class, _("Adjustments"),
			      PROP_MUTED,
			      sfi_pspec_bool ("muted", _("Muted"), NULL,
					      FALSE, SFI_PARAM_STANDARD ":skip-default"));
  bse_object_class_add_param (object_class, _("Synth Input"),
			      PROP_SNET,
			      bse_param_spec_object ("snet", _("Synthesizer"), _("Synthesis network to be used as instrument"),
						     BSE_TYPE_CSYNTH,
						     SFI_PARAM_STANDARD ":unprepared"));
  bse_object_class_add_param (object_class, _("Synth Input"),
			      PROP_WAVE,
			      bse_param_spec_object ("wave", _("Wave"), _("Wave to be used as instrument"),
						     BSE_TYPE_WAVE,
						     SFI_PARAM_STANDARD ":unprepared"));
  bse_object_class_add_param (object_class, _("Synth Input"),
			      PROP_N_VOICES,
			      sfi_pspec_int ("n_voices", _("Max Voixes"), _("Maximum number of voices for simultaneous playback"),
					     16, 1, 256, 1,
					     SFI_PARAM_GUI SFI_PARAM_STORAGE ":scale:unprepared"));
  bse_object_class_add_param (object_class, _("MIDI Instrument"),
                              PROP_MIDI_CHANNEL,
                              sfi_pspec_int ("midi_channel", _("MIDI Channel"),
                                             _("Midi channel assigned to this track, 0 uses internal per-track channel"),
                                             0, 0, BSE_MIDI_MAX_CHANNELS, 1,
                                             SFI_PARAM_GUI SFI_PARAM_STORAGE ":scale:skip-default:unprepared"));
  bse_object_class_add_param (object_class, _("MIDI Instrument"),
			      PROP_PNET,
			      bse_param_spec_object ("pnet", _("Postprocessor"), _("Synthesis network to be used as postprocessor"),
						     BSE_TYPE_CSYNTH, SFI_PARAM_STANDARD ":unprepared"));
  bse_object_class_add_param (object_class, _("Signal Outputs"),
                              PROP_OUTPUTS,
                              bse_param_spec_boxed ("outputs", _("Output Signals"),
                                                    _("Mixer busses used as output for this track"),
                                                    BSE_TYPE_ITEM_SEQ, SFI_PARAM_GUI ":item-sequence"));
  signal_changed = bse_object_class_add_asignal (object_class, "changed", G_TYPE_NONE, 0);
}
