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
#include "bsesnet.h"
#include "bsepart.h"
#include "bsemain.h"
#include "gslcommon.h"
#include "bsesubsynth.h"
#include "bseproject.h"
#include "bsesong.h"
#include "bsemidivoice.h"
#include "bsecontextmerger.h"
#include "bsemidireceiver.h"
#include <string.h>


#define upper_power2(uint_n)	sfi_alloc_upper_power2 (MAX ((uint_n), 4))
#define parse_or_return		bse_storage_scanner_parse_or_return
#define peek_or_return		bse_storage_scanner_peek_or_return


enum {
  PROP_0,
  PROP_PART,
  PROP_MUTED,
  PROP_SYNTH_NET,
  PROP_N_SYNTH_VOICES
};

/* --- prototypes --- */
static void	      bse_track_class_init	(BseTrackClass		*class);
static void	      bse_track_init		(BseTrack		*self);
static void	      bse_track_dispose		(GObject		*object);
static void	      bse_track_finalize	(GObject		*object);
static void	      bse_track_set_property	(GObject		*object,
						 guint                   param_id,
						 const GValue           *value,
						 GParamSpec             *pspec);
static void	      bse_track_get_property	(GObject		*object,
						 guint                   param_id,
						 GValue                 *value,
						 GParamSpec             *pspec);
static BseProxySeq*   bse_track_list_proxies	(BseItem        	*item,
						 guint          	 param_id,
						 GParamSpec     	*pspec);
static void	      bse_track_store_private	(BseObject		*object,
						 BseStorage		*storage);
static BseTokenType   bse_track_restore_private	(BseObject		*object,
						 BseStorage		*storage);


/* --- variables --- */
static GTypeClass *parent_class = NULL;
static guint	   signal_changed = 0;
static GQuark      quark_insert_part = 0;


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
  
  return bse_type_register_static (BSE_TYPE_ITEM,
				   "BseTrack",
				   "BSE track type",
				   &track_info);
}

static void
bse_track_class_init (BseTrackClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseItemClass *item_class = BSE_ITEM_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  quark_insert_part = g_quark_from_static_string ("insert-part");

  gobject_class->set_property = bse_track_set_property;
  gobject_class->get_property = bse_track_get_property;
  gobject_class->dispose = bse_track_dispose;
  gobject_class->finalize = bse_track_finalize;

  object_class->store_private = bse_track_store_private;
  object_class->restore_private = bse_track_restore_private;
  
  item_class->list_proxies = bse_track_list_proxies;
  
  bse_object_class_add_param (object_class, "Play List",
			      PROP_PART,
			      bse_param_spec_object ("part", "Part", NULL,
						     BSE_TYPE_PART, SFI_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Adjustments",
			      PROP_MUTED,
			      sfi_pspec_bool ("muted", "Muted", NULL,
					      FALSE, SFI_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Synth Input",
			      PROP_SYNTH_NET,
			      bse_param_spec_object ("snet", "Custom Synth Net", "Synthesis network to be used as instrument",
						     BSE_TYPE_SNET,
						     SFI_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Synth Input",
			      PROP_N_SYNTH_VOICES,
			      sfi_pspec_int ("n_voices", "Max Voixes", "Maximum number of voices for simultaneous playback",
					     8, 1, 256, 1,
					     SFI_PARAM_GUI SFI_PARAM_STORAGE SFI_PARAM_HINT_SCALE));
  signal_changed = bse_object_class_add_asignal (object_class, "changed", G_TYPE_NONE, 0);
}

static void
bse_track_init (BseTrack *self)
{
  self->snet = NULL;
  self->max_voices = 8;
  self->muted_SL = FALSE;
  self->n_entries_SL = 0;
  self->entries_SL = g_renew (BseTrackEntry, NULL, upper_power2 (self->n_entries_SL));
  self->midi_receiver_SL = bse_midi_receiver_new ("intern");
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
  g_assert (self->n_entries_SL == 0);
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
bse_track_finalize (GObject *object)
{
  BseTrack *self = BSE_TRACK (object);

  g_assert (self->n_entries_SL == 0);
  g_free (self->entries_SL);
  bse_midi_receiver_unref (self->midi_receiver_SL);
  self->midi_receiver_SL = NULL;
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void	track_uncross_part	(BseItem *owner,
					 BseItem *ref_item);

static void
track_add_entry (BseTrack *self,
		 guint     index,
		 guint     tick,
		 BsePart  *part)
{
  guint n, size;

  g_return_if_fail (index <= self->n_entries_SL);
  if (index > 0)
    g_return_if_fail (self->entries_SL[index - 1].tick < tick);
  if (index < self->n_entries_SL)
    g_return_if_fail (self->entries_SL[index].tick > tick);

  BSE_SEQUENCER_LOCK ();
  n = self->n_entries_SL++;
  size = upper_power2 (self->n_entries_SL);
  if (size > upper_power2 (n))
    self->entries_SL = g_renew (BseTrackEntry, self->entries_SL, size);
  g_memmove (self->entries_SL + index + 1, self->entries_SL + index, (n - index) * sizeof (self->entries_SL[0]));
  self->entries_SL[index].tick = tick;
  self->entries_SL[index].part = part;
  self->track_done_SL = FALSE;	/* let sequencer recheck if playing */
  BSE_SEQUENCER_UNLOCK ();
  bse_item_cross_link (BSE_ITEM (self), BSE_ITEM (part), track_uncross_part);
  bse_object_proxy_notifies (part, self, "changed");
  bse_object_reemit_signal (part, "notify::last-tick", self, "changed");
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
  bse_item_cross_unlink (BSE_ITEM (self), BSE_ITEM (part), track_uncross_part);
  BSE_SEQUENCER_LOCK ();
  n = self->n_entries_SL--;
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
	track_delete_entry (self, i);
	g_signal_emit (self, signal_changed, 0);
      }
}

static gboolean
check_project (BseItem *item)
{
  return BSE_IS_PROJECT (item);
}

static gboolean
check_song (BseItem *item)
{
  return BSE_IS_SONG (item);
}

static gboolean
check_part (BseItem *item)
{
  return BSE_IS_PART (item);
}

static gboolean
check_synth (BseItem *item)
{
  // FIXME: we check for non-derived snets here because snet is base type for midisnets and songs
  return G_OBJECT_TYPE (item) == BSE_TYPE_SNET;
}

static BseProxySeq*
bse_track_list_proxies (BseItem    *item,
			guint       param_id,
			GParamSpec *pspec)
{
  BseTrack *self = BSE_TRACK (item);
  BseProxySeq *pseq = bse_proxy_seq_new ();
  switch (param_id)
    {
    case PROP_PART:
      bse_item_gather_proxies (item, pseq, BSE_TYPE_PART,
			       (BseItemCheckContainer) check_song,
			       (BseItemCheckProxy) NULL,
			       NULL);
      break;
    case PROP_SYNTH_NET:
      bse_item_gather_proxies (item, pseq, BSE_TYPE_SNET,
			       (BseItemCheckContainer) check_project,
			       (BseItemCheckProxy) check_synth,
			       NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
  return pseq;
}

static void
track_uncross_snet (BseItem *owner,
		    BseItem *ref_item)
{
  BseTrack *self = BSE_TRACK (owner);
  g_object_set (self, "snet", NULL, NULL);
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
    case PROP_MUTED:
      BSE_SEQUENCER_LOCK ();
      self->muted_SL = sfi_value_get_bool (value);
      BSE_SEQUENCER_UNLOCK ();
      break;
    case PROP_PART:
      if (!self->n_entries_SL)
	bse_track_insert_part (self, 0, bse_value_get_object (value));
      break;
    case PROP_SYNTH_NET:
      if (self->snet)
	{
	  bse_object_unproxy_notifies (self->snet, self, "changed");
	  bse_item_cross_unlink (BSE_ITEM (self), BSE_ITEM (self->snet), track_uncross_snet);
	}
      self->snet = bse_value_get_object (value);
      if (self->snet)
	{
	  bse_item_cross_link (BSE_ITEM (self), BSE_ITEM (self->snet), track_uncross_snet);
	  bse_object_proxy_notifies (self->snet, self, "changed");
	}
      if (self->sub_synth)
	g_object_set (self->sub_synth,
		      "snet", self->snet,
		      NULL);
      break;
    case PROP_N_SYNTH_VOICES:
      self->max_voices = sfi_value_get_int (value);
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
    case PROP_MUTED:
      sfi_value_set_bool (value, self->muted_SL);
      break;
    case PROP_PART:
      bse_value_set_object (value, NULL);
      break;
    case PROP_SYNTH_NET:
      bse_value_set_object (value, self->snet);
      break;
    case PROP_N_SYNTH_VOICES:
      sfi_value_set_int (value, self->max_voices);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

BseErrorType
bse_track_insert_part (BseTrack *self,
		       guint     tick,
		       BsePart  *part)
{
  BseTrackEntry *entry;
  gint i;

  g_return_val_if_fail (BSE_IS_TRACK (self), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_IS_PART (part), BSE_ERROR_INTERNAL);

  entry = track_lookup_entry (self, tick);
  if (entry && entry->tick == tick)
    return BSE_ERROR_POS_ALLOC;
  else
    {
      i = entry ? entry - self->entries_SL : 0;
      track_add_entry (self, entry ? i + 1 : 0, tick, part);
    }
  g_signal_emit (self, signal_changed, 0);
  return BSE_ERROR_NONE;
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
      track_delete_entry (self, entry - self->entries_SL);
      g_signal_emit (self, signal_changed, 0);
    }
}

BseTrackPartSeq*
bse_track_list_parts (BseTrack *self)
{
  BseTrackPartSeq *tps;
  BseItem *item;
  guint mindur = 384 * 4;
  gint i;

  g_return_val_if_fail (BSE_IS_TRACK (self), NULL);

  item = BSE_ITEM (self);
  if (BSE_IS_SONG (item->parent))
    {
      BseSong *song = BSE_SONG (item->parent);
      mindur = song->tpqn * song->qnpt;
    }
  tps = bse_track_part_seq_new ();
  for (i = 0; i < self->n_entries_SL; i++)
    {
      BseTrackEntry *entry = self->entries_SL + i;
      if (entry->part)
	{
	  BseTrackPart tp = { 0, };
	  tp.tick = entry->tick;
	  tp.part = BSE_OBJECT_ID (entry->part);
	  tp.duration = MAX (mindur, entry->part->last_tick_SL);
	  if (i + 1 < self->n_entries_SL)
	    tp.duration = MIN (tp.duration, entry[1].tick - entry->tick);
	  bse_track_part_seq_append (tps, &tp);
	}
    }
  return tps;
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
bse_track_add_modules (BseTrack     *self,
		       BseContainer *container,
		       BseSource    *merger)
{
  g_return_if_fail (BSE_IS_TRACK (self));
  g_return_if_fail (BSE_IS_CONTAINER (container));
  g_return_if_fail (BSE_IS_CONTEXT_MERGER (merger));
  g_return_if_fail (self->sub_synth == NULL);
  
  /* midi voice input */
  self->voice_input = bse_container_new_item (container, BSE_TYPE_MIDI_VOICE_INPUT, NULL);
  BSE_OBJECT_SET_FLAGS (self->voice_input, BSE_ITEM_FLAG_AGGREGATE);
  bse_midi_voice_input_set_midi_receiver (BSE_MIDI_VOICE_INPUT (self->voice_input), self->midi_receiver_SL, 0);
  
  /* sub synth */
  self->sub_synth = bse_container_new_item (container, BSE_TYPE_SUB_SYNTH,
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
  BSE_OBJECT_SET_FLAGS (self->sub_synth, BSE_ITEM_FLAG_AGGREGATE);
  bse_sub_synth_set_midi_receiver (BSE_SUB_SYNTH (self->sub_synth), self->midi_receiver_SL, 0);
  
  /* midi voice switch */
  self->voice_switch = bse_container_new_item (container, BSE_TYPE_MIDI_VOICE_SWITCH, NULL);
  BSE_OBJECT_SET_FLAGS (self->voice_switch, BSE_ITEM_FLAG_AGGREGATE);
  bse_midi_voice_switch_set_voice_input (BSE_MIDI_VOICE_SWITCH (self->voice_switch), BSE_MIDI_VOICE_INPUT (self->voice_input));
  
  /* context merger */
  self->context_merger = bse_container_new_item (container, BSE_TYPE_CONTEXT_MERGER, NULL);
  BSE_OBJECT_SET_FLAGS (self->context_merger, BSE_ITEM_FLAG_AGGREGATE);
  
  /* voice input <-> sub-synth */
  bse_source_must_set_input (self->sub_synth, 0,
			     self->voice_input, BSE_MIDI_VOICE_INPUT_OCHANNEL_FREQUENCY);
  bse_source_must_set_input (self->sub_synth, 1,
			     self->voice_input, BSE_MIDI_VOICE_INPUT_OCHANNEL_GATE);
  bse_source_must_set_input (self->sub_synth, 2,
			     self->voice_input, BSE_MIDI_VOICE_INPUT_OCHANNEL_VELOCITY);
  bse_source_must_set_input (self->sub_synth, 3,
			     self->voice_input, BSE_MIDI_VOICE_INPUT_OCHANNEL_AFTERTOUCH);
  
  /* sub-synth <-> voice switch */
  bse_source_must_set_input (self->voice_switch, BSE_MIDI_VOICE_SWITCH_ICHANNEL_LEFT,
			     self->sub_synth, 0);
  bse_source_must_set_input (self->voice_switch, BSE_MIDI_VOICE_SWITCH_ICHANNEL_RIGHT,
			     self->sub_synth, 1);
  bse_source_must_set_input (self->voice_switch, BSE_MIDI_VOICE_SWITCH_ICHANNEL_DISCONNECT,
			     self->sub_synth, 3);
  
  /* midi voice switch <-> context merger */
  bse_source_must_set_input (self->context_merger, 0,
			     self->voice_switch, BSE_MIDI_VOICE_SWITCH_OCHANNEL_LEFT);
  bse_source_must_set_input (self->context_merger, 1,
			     self->voice_switch, BSE_MIDI_VOICE_SWITCH_OCHANNEL_RIGHT);
  
  /* context merger <-> container's merger */
  bse_source_must_set_input (merger, 0,
			     self->context_merger, 0);
  bse_source_must_set_input (merger, 1,
			     self->context_merger, 1);
}

void
bse_track_remove_modules (BseTrack     *self,
			  BseContainer *container)
{
  g_return_if_fail (BSE_IS_TRACK (self));
  g_return_if_fail (BSE_IS_CONTAINER (container));
  g_return_if_fail (self->sub_synth != NULL);
  
  bse_container_remove_item (container, BSE_ITEM (self->voice_input));
  self->voice_input = NULL;
  bse_container_remove_item (container, BSE_ITEM (self->sub_synth));
  self->sub_synth = NULL;
  bse_container_remove_item (container, BSE_ITEM (self->voice_switch));
  self->voice_switch = NULL;
  bse_container_remove_item (container, BSE_ITEM (self->context_merger));
  self->context_merger = NULL;
}

void
bse_track_clone_voices (BseTrack *self,
			BseSNet  *snet,
			guint     context,
			GslTrans *trans)
{
  guint i;
  
  g_return_if_fail (BSE_IS_TRACK (self));
  g_return_if_fail (BSE_IS_SNET (snet));
  g_return_if_fail (trans != NULL);
  
  for (i = 0; i < self->max_voices - 1; i++)
    bse_snet_context_clone_branch (snet, context, self->context_merger, self->midi_receiver_SL, 0, trans);
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
      BseErrorType error = bse_track_insert_part (self, tick, BSE_PART (to_item));
      if (error)
	bse_storage_warn (storage, "failed to insert part reference to \"%s\": %s",
			  bse_object_debug_name (to_item),
			  bse_error_blurb (error));
    }
}

static BseTokenType
bse_track_restore_private (BseObject  *object,
			   BseStorage *storage)
{
  BseTrack *self = BSE_TRACK (object);
  GScanner *scanner = storage->scanner;
  GTokenType token;
  GQuark token_quark;
  
  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->restore_private)
    token = BSE_OBJECT_CLASS (parent_class)->restore_private (object, storage);
  else
    token = BSE_TOKEN_UNMATCHED;
  
  if (token != BSE_TOKEN_UNMATCHED ||
      g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return token;
  
  token_quark = g_quark_try_string (scanner->next_value.v_identifier);
  
  if (token_quark == quark_insert_part)
    {
      guint tick;

      g_scanner_get_next_token (scanner);       /* eat quark */
      
      parse_or_return (scanner, G_TOKEN_INT);
      tick = scanner->value.v_int;
      token = bse_storage_parse_item_link (storage, BSE_ITEM (self), part_link_resolved, GUINT_TO_POINTER (tick));
      if (token != G_TOKEN_NONE)
	return token;
      if (g_scanner_get_next_token (scanner) != ')')
	return ')';
      return G_TOKEN_NONE;
    }
  else
    return BSE_TOKEN_UNMATCHED;
}
