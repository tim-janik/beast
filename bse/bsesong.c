/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-2003 Tim Janik
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
#include "bsesong.h"

#include "bsetrack.h"
#include "bsepart.h"
#include "bsecontextmerger.h"
#include "bsepcmoutput.h"
#include "bseproject.h"
#include "bsemidireceiver.h"
#include "bsestorage.h"
#include "bsemain.h"
#include "bsecsynth.h"
#include "bsessequencer.h"
#include "bsesubsynth.h"
#include "gslengine.h"	// FIXME: for gsl_engine_sample_freq()
#include <string.h>


enum
{
  PROP_0,
  PROP_VOLUME_f,
  PROP_VOLUME_dB,
  PROP_VOLUME_PERC,
  PROP_TPQN,
  PROP_NOMINATOR,
  PROP_DENOMINATOR,
  PROP_BPM,
  PROP_POST_NET,
  PROP_AUTO_ACTIVATE,
  PROP_LOOP_ENABLED,
  PROP_LOOP_LEFT,
  PROP_LOOP_RIGHT,
  PROP_TICK_POINTER,
};

enum {
  SIGNAL_LAST
};


/* --- prototypes --- */
static void         bse_song_class_init       (BseSongClass       *class);
static void         bse_song_init             (BseSong            *song);
static void         bse_song_finalize         (GObject            *object);
static void         bse_song_release_children (BseContainer       *container);
static BseProxySeq* bse_song_list_proxies     (BseItem            *item,
                                               guint               param_id,
                                               GParamSpec         *pspec);
static void         bse_song_set_property     (GObject            *object,
                                               guint               param_id,
                                               const GValue       *value,
                                               GParamSpec         *pspec);
static void         bse_song_get_property     (GObject            *object,
                                               guint               param_id,
                                               GValue             *value,
                                               GParamSpec         *pspec);
static void         bse_song_set_parent       (BseItem            *item,
                                               BseItem            *parent);
static void         bse_song_add_item         (BseContainer       *container,
                                               BseItem            *item);
static void         bse_song_forall_items     (BseContainer       *container,
                                               BseForallItemsFunc  func,
                                               gpointer            data);
static void         bse_song_remove_item      (BseContainer       *container,
                                               BseItem            *item);
static void         bse_song_prepare          (BseSource          *source);
static void         bse_song_context_create   (BseSource          *source,
                                               guint               context_handle,
                                               GslTrans           *trans);
static void         bse_song_reset            (BseSource          *source);
static void         bse_song_update_tpsi_SL   (BseSong            *song);



/* --- variables --- */
static GTypeClass *parent_class = NULL;
static guint       signal_pointer_changed = 0;


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
				   &song_info);
}

void
bse_song_timing_get_default (BseSongTiming *timing)
{
  g_return_if_fail (timing != NULL);

  timing->tick = 0;
  timing->bpm = 120;
  timing->nominator = 4;
  timing->denominator = 4;
  timing->tpqn = 384;
  timing->tpt = timing->tpqn * 4 * timing->nominator / timing->denominator;
}

static void
bse_song_class_init (BseSongClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseItemClass *item_class = BSE_ITEM_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  BseContainerClass *container_class = BSE_CONTAINER_CLASS (class);
  BseSongTiming timing;

  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_song_set_property;
  gobject_class->get_property = bse_song_get_property;
  gobject_class->finalize = bse_song_finalize;
  
  item_class->set_parent = bse_song_set_parent;
  item_class->list_proxies = bse_song_list_proxies;

  source_class->prepare = bse_song_prepare;
  source_class->context_create = bse_song_context_create;
  source_class->reset = bse_song_reset;
  
  container_class->add_item = bse_song_add_item;
  container_class->remove_item = bse_song_remove_item;
  container_class->forall_items = bse_song_forall_items;
  container_class->release_children = bse_song_release_children;

  bse_song_timing_get_default (&timing);

  bse_object_class_add_param (object_class, "Adjustments",
			      PROP_VOLUME_f,
			      sfi_pspec_real ("volume_f", "Master [float]", NULL,
					      bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB),
					      0, bse_dB_to_factor (BSE_MAX_VOLUME_dB),
					      0.1, SFI_PARAM_STORAGE));
  bse_object_class_add_param (object_class, "Adjustments",
			      PROP_VOLUME_dB,
			      sfi_pspec_real ("volume_dB", "Master [dB]", NULL,
					      BSE_DFL_MASTER_VOLUME_dB,
					      BSE_MIN_VOLUME_dB, BSE_MAX_VOLUME_dB,
					      BSE_GCONFIG (step_volume_dB),
					      SFI_PARAM_GUI SFI_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Adjustments",
			      PROP_VOLUME_PERC,
			      sfi_pspec_int ("volume_perc", "Master [%]", NULL,
					     bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB) * 100,
					     0, bse_dB_to_factor (BSE_MAX_VOLUME_dB) * 100, 1,
					     SFI_PARAM_GUI SFI_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Timing",
			      PROP_TPQN,
			      sfi_pspec_int ("tpqn", "Ticks", "Number of ticks per quarter note",
					     timing.tpqn, 384, 384, 0, SFI_PARAM_DEFAULT_RDONLY));
  bse_object_class_add_param (object_class, "Timing",
			      PROP_NOMINATOR,
			      sfi_pspec_int ("nominator", "Nominator", "Measure nominator",
					     timing.nominator, 1, 256, 1, SFI_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Timing",
			      PROP_DENOMINATOR,
			      sfi_pspec_int ("denominator", "Denominator", "Measure denominator, must be a power of 2",
					     timing.denominator, 1, 256, 0, SFI_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Timing",
			      PROP_BPM,
			      sfi_pspec_real ("bpm", "Beats per minute", NULL,
					      timing.bpm,
					      BSE_MIN_BPM, BSE_MAX_BPM,
					      BSE_GCONFIG (step_bpm),
					      SFI_PARAM_DEFAULT SFI_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Synth Postprocess",
                              PROP_POST_NET,
                              bse_param_spec_object ("pnet", "Custom Postprocess Net",
                                                     "Synthesis network to postprocess song sound",
                                                     BSE_TYPE_CSYNTH,
                                                     SFI_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Playback Settings",
			      PROP_AUTO_ACTIVATE,
			      sfi_pspec_bool ("auto_activate", NULL, NULL,
					      TRUE, /* change default */
					      /* override parent property */ 0));
  bse_object_class_add_param (object_class, "Looping",
			      PROP_LOOP_ENABLED,
			      sfi_pspec_bool ("loop_enabled", NULL, NULL,
					      FALSE, SFI_PARAM_READWRITE));
  bse_object_class_add_param (object_class, "Looping",
			      PROP_LOOP_LEFT,
			      sfi_pspec_int ("loop_left", NULL, NULL,
					     -1, -1, G_MAXINT, 384,
					     SFI_PARAM_READWRITE));
  bse_object_class_add_param (object_class, "Looping",
			      PROP_LOOP_RIGHT,
			      sfi_pspec_int ("loop_right", NULL, NULL,
					     -1, -1, G_MAXINT, 384,
					     SFI_PARAM_READWRITE));
  bse_object_class_add_param (object_class, "Looping",
			      PROP_TICK_POINTER,
			      sfi_pspec_int ("tick_pointer", NULL, NULL,
					     -1, -1, G_MAXINT, 384,
					     SFI_PARAM_READWRITE));

  signal_pointer_changed = bse_object_class_add_signal (object_class, "pointer-changed",
							G_TYPE_NONE, 1, SFI_TYPE_INT);
}

static void
bse_song_init (BseSong *self)
{
  BseSNet *snet = BSE_SNET (self);
  BseSongTiming timing;

  bse_song_timing_get_default (&timing);

  BSE_OBJECT_UNSET_FLAGS (self, BSE_SNET_FLAG_USER_SYNTH);
  BSE_OBJECT_SET_FLAGS (self, BSE_SUPER_FLAG_NEEDS_CONTEXT | BSE_SUPER_FLAG_NEEDS_SEQUENCER);

  self->tpqn = timing.tpqn;
  self->nominator = timing.nominator;
  self->denominator = timing.denominator;
  self->bpm = timing.bpm;
  self->volume_factor = bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB);
  
  self->parts = NULL;

  self->pnet = NULL;

  self->last_position = -1;
  self->position_handler = 0;

  self->tracks_SL = NULL;
  self->loop_enabled_SL = 0;
  self->loop_left_SL = -1;
  self->loop_right_SL = -1;

  /* context merger */
  self->context_merger = bse_container_new_child (BSE_CONTAINER (self), BSE_TYPE_CONTEXT_MERGER,
                                                  "uname", "Track-Merger",
                                                  NULL);
  bse_snet_intern_child (snet, self->context_merger);

  /* post processing slot */
  self->postprocess = bse_container_new_child (BSE_CONTAINER (self), BSE_TYPE_SUB_SYNTH,
                                               "uname", "Postprocess",
                                               NULL);
  bse_snet_intern_child (snet, self->postprocess);
  bse_sub_synth_set_null_shortcut (BSE_SUB_SYNTH (self->postprocess), TRUE);

  /* context merger <-> postprocess */
  bse_source_must_set_input (self->postprocess, 0,
			     self->context_merger, 0);
  bse_source_must_set_input (self->postprocess, 1,
			     self->context_merger, 1);

  /* output */
  self->output = bse_container_new_child (BSE_CONTAINER (self), BSE_TYPE_PCM_OUTPUT, NULL);
  bse_snet_intern_child (snet, self->output);

  /* postprocess <-> output */
  bse_source_must_set_input (self->output, BSE_PCM_OUTPUT_ICHANNEL_LEFT,
			     self->postprocess, 0);
  bse_source_must_set_input (self->output, BSE_PCM_OUTPUT_ICHANNEL_RIGHT,
			     self->postprocess, 1);
}

static void
bse_song_release_children (BseContainer *container)
{
  BseSong *self = BSE_SONG (container);

  while (self->parts)
    bse_container_remove_item (container, self->parts->data);
  while (self->tracks_SL)
    bse_container_remove_item (container, self->tracks_SL->data);

  /* chain parent class' handler */
  BSE_CONTAINER_CLASS (parent_class)->release_children (container);
}

static void
bse_song_finalize (GObject *object)
{
  BseSong *self = BSE_SONG (object);

  bse_container_remove_item (BSE_CONTAINER (self), BSE_ITEM (self->context_merger));
  self->context_merger = NULL;
  bse_container_remove_item (BSE_CONTAINER (self), BSE_ITEM (self->postprocess));
  self->postprocess = NULL;
  bse_container_remove_item (BSE_CONTAINER (self), BSE_ITEM (self->output));
  self->output = NULL;
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static BseProxySeq*
bse_song_list_proxies (BseItem    *item,
                       guint       param_id,
                       GParamSpec *pspec)
{
  BseSong *self = BSE_SONG (item);
  BseProxySeq *pseq = bse_proxy_seq_new ();
  switch (param_id)
    {
    case PROP_POST_NET:
      bse_item_gather_proxies_typed (item, pseq, BSE_TYPE_CSYNTH, BSE_TYPE_PROJECT, FALSE);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
  return pseq;
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
      gfloat volume_factor;
      gboolean vbool;
      SfiInt vint;
      SfiRing *ring;
    case PROP_VOLUME_f:
    case PROP_VOLUME_dB:
    case PROP_VOLUME_PERC:
      volume_factor = 0; /* silence gcc */
      switch (param_id)
	{
	case PROP_VOLUME_f:
	  volume_factor = sfi_value_get_real (value);
	  break;
	case PROP_VOLUME_dB:
	  volume_factor = bse_dB_to_factor (sfi_value_get_real (value));
	  break;
	case PROP_VOLUME_PERC:
	  volume_factor = sfi_value_get_int (value) / 100.0;
	  break;
	}
      BSE_SEQUENCER_LOCK ();
      self->volume_factor = volume_factor;
      BSE_SEQUENCER_UNLOCK ();
      g_object_notify (self, "volume_dB");
      g_object_notify (self, "volume_perc");
      g_object_notify (self, "volume_f");
      break;
    case PROP_BPM:
      self->bpm = sfi_value_get_real (value);
      bse_song_update_tpsi_SL (self);
      break;
    case PROP_POST_NET:
      if (!self->postprocess || !BSE_SOURCE_PREPARED (self->postprocess))
        {
          if (self->pnet)
            {
              bse_object_unproxy_notifies (self->pnet, self, "notify::pnet");
              bse_item_cross_unlink (BSE_ITEM (self), BSE_ITEM (self->pnet), song_uncross_pnet);
              self->pnet = NULL;
            }
          self->pnet = bse_value_get_object (value);
          if (self->pnet)
            {
              bse_item_cross_link (BSE_ITEM (self), BSE_ITEM (self->pnet), song_uncross_pnet);
              bse_object_proxy_notifies (self->pnet, self, "notify::pnet");
            }
          if (self->postprocess)
            g_object_set (self->postprocess, /* no undo */
                          "snet", self->pnet,
                          NULL);
        }
      break;
    case PROP_NOMINATOR:
      self->nominator = sfi_value_get_int (value);
      bse_song_update_tpsi_SL (self);
      break;
    case PROP_DENOMINATOR:
      vint = sfi_value_get_int (value);
      self->denominator = vint <= 2 ? vint : 1 << g_bit_storage (vint - 1);
      bse_song_update_tpsi_SL (self);
      break;
    case PROP_TPQN:
      self->tpqn = sfi_value_get_int (value);
      bse_song_update_tpsi_SL (self);
      break;
    case PROP_LOOP_ENABLED:
      vbool = sfi_value_get_bool (value);
      vbool = vbool && self->loop_left_SL >= 0 && self->loop_right_SL > self->loop_left_SL;
      if (vbool != self->loop_enabled_SL)
	{
	  BSE_SEQUENCER_LOCK ();
	  self->loop_enabled_SL = vbool;
	  BSE_SEQUENCER_UNLOCK ();
	}
      break;
    case PROP_LOOP_LEFT:
      vint = sfi_value_get_int (value);
      if (vint != self->loop_left_SL)
	{
	  gboolean loop_enabled = self->loop_enabled_SL;
	  BSE_SEQUENCER_LOCK ();
	  self->loop_left_SL = vint;
	  self->loop_enabled_SL = (self->loop_enabled_SL &&
				   self->loop_left_SL >= 0 &&
				   self->loop_right_SL > self->loop_left_SL);
	  BSE_SEQUENCER_UNLOCK ();
	  if (loop_enabled != self->loop_enabled_SL)
	    g_object_notify (self, "loop_enabled");
	}
      break;
    case PROP_LOOP_RIGHT:
      vint = sfi_value_get_int (value);
      if (vint != self->loop_right_SL)
	{
          gboolean loop_enabled = self->loop_enabled_SL;
	  BSE_SEQUENCER_LOCK ();
	  self->loop_right_SL = vint;
	  self->loop_enabled_SL = (self->loop_enabled_SL &&
				   self->loop_left_SL >= 0 &&
				   self->loop_right_SL > self->loop_left_SL);
	  BSE_SEQUENCER_UNLOCK ();
          if (loop_enabled != self->loop_enabled_SL)
	    g_object_notify (self, "loop_enabled");
	}
      break;
    case PROP_TICK_POINTER:
      vint = sfi_value_get_int (value);
      if (vint != self->tick_SL)
	{
	  BSE_SEQUENCER_LOCK ();
	  self->tick_SL = vint;
	  for (ring = self->tracks_SL; ring; ring = sfi_ring_walk (ring, self->tracks_SL))
	    {
	      BseTrack *track = ring->data;
	      track->track_done_SL = FALSE;	/* let sequencer recheck if playing */
	    }
	  BSE_SEQUENCER_UNLOCK ();
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
    case PROP_VOLUME_f:
      sfi_value_set_real (value, self->volume_factor);
      break;
    case PROP_VOLUME_dB:
      sfi_value_set_real (value, bse_dB_from_factor (self->volume_factor, BSE_MIN_VOLUME_dB));
      break;
    case PROP_VOLUME_PERC:
      sfi_value_set_int (value, self->volume_factor * 100.0 + 0.5);
      break;
    case PROP_BPM:
      sfi_value_set_real (value, self->bpm);
      break;
    case PROP_POST_NET:
      bse_value_set_object (value, self->pnet);
      break;
    case PROP_NOMINATOR:
      sfi_value_set_int (value, self->nominator);
      break;
    case PROP_DENOMINATOR:
      sfi_value_set_int (value, self->denominator);
      break;
    case PROP_TPQN:
      sfi_value_set_int (value, self->tpqn);
      break;
    case PROP_LOOP_ENABLED:
      sfi_value_set_bool (value, self->loop_enabled_SL);
      break;
    case PROP_LOOP_LEFT:
      sfi_value_set_int (value, self->loop_left_SL);
      break;
    case PROP_LOOP_RIGHT:
      sfi_value_set_int (value, self->loop_right_SL);
      break;
    case PROP_TICK_POINTER:
      sfi_value_set_int (value, self->tick_SL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

void
bse_song_get_timing (BseSong       *self,
		     guint          tick,
		     BseSongTiming *timing)
{
  g_return_if_fail (BSE_IS_SONG (self));
  g_return_if_fail (timing != NULL);

  timing->tick = 0;
  timing->bpm = self->bpm;
  timing->nominator = self->nominator;
  timing->denominator = self->denominator;
  timing->tpqn = self->tpqn;
  timing->tpt = timing->tpqn * 4 * timing->nominator / timing->denominator;
}

BseSong*
bse_song_lookup (BseProject  *project,
		 const gchar *name)
{
  BseItem *item;
  
  g_return_val_if_fail (BSE_IS_PROJECT (project), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  
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

  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_TRACK))
    bse_track_add_modules (BSE_TRACK (item), container, self->midi_receiver_SL, self->context_merger);

  BSE_SEQUENCER_LOCK ();

  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_TRACK))
    self->tracks_SL = sfi_ring_append (self->tracks_SL, item);
  else if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_PART))
    self->parts = g_list_append (self->parts, item);
  else
    /* parent class manages BseSources */ ;
  self->song_done_SL = FALSE;	/* let sequencer recheck if playing */

  /* chain parent class' add_item handler */
  BSE_CONTAINER_CLASS (parent_class)->add_item (container, item);

  BSE_SEQUENCER_UNLOCK ();
}

static void
bse_song_forall_items (BseContainer	 *container,
		       BseForallItemsFunc func,
		       gpointer		  data)
{
  BseSong *song;
  SfiRing *ring;
  GList *list;
  
  song = BSE_SONG (container);
  
  ring = song->tracks_SL;
  while (ring)
    {
      BseItem *item = ring->data;
      ring = sfi_ring_walk (ring, song->tracks_SL);
      if (!func (item, data))
	return;
    }

  list = song->parts;
  while (list)
    {
      BseItem *item;
      
      item = list->data;
      list = list->next;
      if (!func (item, data))
	return;
    }

  /* parent class manages BseSources */
  BSE_CONTAINER_CLASS (parent_class)->forall_items (container, func, data);
}

static void
bse_song_remove_item (BseContainer *container,
		      BseItem	   *item)
{
  BseSong *song;
  GList **list_p = NULL;
  
  song = BSE_SONG (container);
  
  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_TRACK))
    {
      SfiRing *ring, *tmp;
      bse_track_remove_modules (BSE_TRACK (item), BSE_CONTAINER (song));
      ring = sfi_ring_find (song->tracks_SL, item);
      for (tmp = sfi_ring_walk (ring, song->tracks_SL); tmp; tmp = sfi_ring_walk (tmp, song->tracks_SL))
	bse_item_queue_seqid_changed (tmp->data);
      BSE_SEQUENCER_LOCK ();
      song->tracks_SL = sfi_ring_remove_node (song->tracks_SL, ring);
      BSE_SEQUENCER_UNLOCK ();
    }
  else if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_PART))
    list_p = &song->parts;
  else
    /* parent class manages BseSources */;

  if (list_p)
    {
      GList *list, *tmp;
      for (list = *list_p; list; list = list->next)
	if (list->data == (gpointer) item)
	  break;
      (list->prev ? list->prev->next : *list_p) = list->next;
      if (list->next)
	list->next->prev = list->prev;
      tmp = list;
      list = list->next;
      g_list_free_1 (tmp);
      for (; list; list = list->next)
	bse_item_queue_seqid_changed (list->data);
    }

  /* chain parent class' remove_item handler */
  BSE_CONTAINER_CLASS (parent_class)->remove_item (container, item);
}

static gboolean
song_position_handler (gpointer data)
{
  BseSong *self = BSE_SONG (data);

  if (self->last_position != self->tick_SL)
    {
      BSE_SEQUENCER_LOCK ();
      self->last_position = self->tick_SL;
      BSE_SEQUENCER_UNLOCK ();
      g_signal_emit (self, signal_pointer_changed, 0, self->last_position);
    }
  return TRUE;
}

static void
bse_song_update_tpsi_SL (BseSong *self)
{
  gdouble tpqn = self->tpqn;		/* ticks per quarter note */
  gdouble qnps = self->bpm / 60.;	/* quarter notes per second */
  gdouble tps = tpqn * qnps;		/* ticks per second */
  gdouble sps = gsl_engine_sample_freq ();
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
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);

  bse_song_update_tpsi_SL (self);

  if (!self->position_handler)
    self->position_handler = bse_idle_timed (50000, song_position_handler, self);
}

static void
bse_song_context_create (BseSource *source,
			 guint      context_handle,
			 GslTrans  *trans)
{
  BseSong *self = BSE_SONG (source);
  BseSNet *snet = BSE_SNET (self);
  BseMidiContext mcontext = bse_snet_get_midi_context (snet, context_handle);
  SfiRing *ring;

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);

  if (!bse_snet_context_is_branch (snet, context_handle))       /* catch recursion */
    for (ring = self->tracks_SL; ring; ring = sfi_ring_walk (ring, self->tracks_SL))
      bse_track_clone_voices (ring->data, snet, context_handle, mcontext, trans);
}

static void
bse_song_reset (BseSource *source)
{
  BseSong *self = BSE_SONG (source);

  bse_ssequencer_handle_jobs (sfi_ring_prepend (NULL, bse_ssequencer_job_stop_super (BSE_SUPER (self))));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);

  if (self->position_handler)
    {
      bse_idle_remove (self->position_handler);
      self->position_handler = 0;
    }

  bse_object_unlock (BSE_OBJECT (self));

  g_object_notify (self, "tick-pointer");
}

void
bse_song_stop_sequencing_SL (BseSong *self)
{
  BseItem *item;

  g_return_if_fail (BSE_IS_SONG (self));

  bse_ssequencer_remove_super_SL (BSE_SUPER (self));
  item = BSE_ITEM (self);
  while (item->parent)
    item = item->parent;
  bse_project_queue_auto_stop_SL (BSE_PROJECT (item));
}
