/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-2002 Tim Janik
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
#include "bsestorage.h"
#include "bsemain.h"
#include "bsessequencer.h"
#include "gslengine.h"	// FIXME: for gsl_engine_sample_freq()
#include <string.h>


enum
{
  PARAM_0,
  PARAM_VOLUME_f,
  PARAM_VOLUME_dB,
  PARAM_VOLUME_PERC,
  PARAM_BPM,
  PARAM_AUTO_ACTIVATE
};

enum {
  SIGNAL_LAST
};


/* --- prototypes --- */
static void	 bse_song_class_init		(BseSongClass	   *class);
static void	 bse_song_init			(BseSong	   *song);
static void	 bse_song_finalize		(GObject	   *object);
static void	 bse_song_release_children	(BseContainer	   *container);
static void	 bse_song_set_property		(GObject           *object,
						 guint              param_id,
						 const GValue      *value,
						 GParamSpec        *pspec);
static void	 bse_song_get_property		(GObject           *object,
						 guint              param_id,
						 GValue            *value,
						 GParamSpec        *pspec);
static void	 bse_song_add_item		(BseContainer	   *container,
						 BseItem	   *item);
static void	 bse_song_forall_items		(BseContainer	   *container,
						 BseForallItemsFunc func,
						 gpointer	    data);
static void	 bse_song_remove_item		(BseContainer	   *container,
						 BseItem	   *item);
static BseTokenType bse_song_restore_private    (BseObject         *object,
						 BseStorage        *storage);
static void      bse_song_store_after		(BseObject         *object,
						 BseStorage        *storage);
static GTokenType bse_song_restore              (BseObject         *object,
						 BseStorage        *storage);
static void	 bse_song_prepare		(BseSource	   *source);
static void      bse_song_context_create        (BseSource         *source,
						 guint              context_handle,
						 GslTrans          *trans);
static void	 bse_song_reset			(BseSource	   *source);
static void	 bse_song_update_tpsi_SL	(BseSong	   *song);


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
				   &song_info);
}

static void
bse_song_class_init (BseSongClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  BseContainerClass *container_class = BSE_CONTAINER_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_song_set_property;
  gobject_class->get_property = bse_song_get_property;
  gobject_class->finalize = bse_song_finalize;
  
  object_class->store_after = bse_song_store_after;
  object_class->restore = bse_song_restore;
  object_class->restore_private = bse_song_restore_private;
  
  source_class->prepare = bse_song_prepare;
  source_class->context_create = bse_song_context_create;
  source_class->reset = bse_song_reset;
  
  container_class->add_item = bse_song_add_item;
  container_class->remove_item = bse_song_remove_item;
  container_class->forall_items = bse_song_forall_items;
  container_class->release_children = bse_song_release_children;
  
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_VOLUME_f,
			      sfi_pspec_real ("volume_f", "Master [float]", NULL,
					      bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB),
					      0, bse_dB_to_factor (BSE_MAX_VOLUME_dB),
					      0.1, SFI_PARAM_STORAGE));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_VOLUME_dB,
			      sfi_pspec_real ("volume_dB", "Master [dB]", NULL,
					      BSE_DFL_MASTER_VOLUME_dB,
					      BSE_MIN_VOLUME_dB, BSE_MAX_VOLUME_dB,
					      BSE_GCONFIG (step_volume_dB),
					      SFI_PARAM_GUI SFI_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_VOLUME_PERC,
			      sfi_pspec_int ("volume_perc", "Master [%]", NULL,
					     bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB) * 100,
					     0, bse_dB_to_factor (BSE_MAX_VOLUME_dB) * 100, 1,
					     SFI_PARAM_GUI SFI_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_BPM,
			      sfi_pspec_int ("bpm", "Beats per minute", NULL,
					     BSE_DFL_SONG_BPM,
					     BSE_MIN_BPM, BSE_MAX_BPM,
					     BSE_GCONFIG (step_bpm),
					     SFI_PARAM_DEFAULT SFI_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Playback Settings",
			      PARAM_AUTO_ACTIVATE,
			      sfi_pspec_bool ("auto_activate", NULL, NULL,
					      TRUE, /* change default */
					      /* override parent property */ 0));
}

static void
bse_song_init (BseSong *self)
{
  BSE_OBJECT_UNSET_FLAGS (self, BSE_SNET_FLAG_USER_SYNTH);
  BSE_OBJECT_SET_FLAGS (self, BSE_SUPER_FLAG_NEEDS_CONTEXT | BSE_SUPER_FLAG_NEEDS_SEQUENCER);
  self->bpm = BSE_DFL_SONG_BPM;
  self->volume_factor = bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB);
  
  self->parts = NULL;
  self->tracks_SL = NULL;

  /* context merger */
  self->context_merger = bse_container_new_item (BSE_CONTAINER (self), BSE_TYPE_CONTEXT_MERGER, NULL);
  BSE_OBJECT_SET_FLAGS (self->context_merger, BSE_ITEM_FLAG_AGGREGATE);

  /* output */
  self->output = bse_container_new_item (BSE_CONTAINER (self), BSE_TYPE_PCM_OUTPUT, NULL);
  BSE_OBJECT_SET_FLAGS (self->output, BSE_ITEM_FLAG_AGGREGATE);

  /* context merger <-> output */
  bse_source_must_set_input (self->output, BSE_PCM_OUTPUT_ICHANNEL_LEFT,
			     self->context_merger, 0);
  bse_source_must_set_input (self->output, BSE_PCM_OUTPUT_ICHANNEL_RIGHT,
			     self->context_merger, 1);
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
  bse_container_remove_item (BSE_CONTAINER (self), BSE_ITEM (self->output));
  self->output = NULL;
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
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
      guint bpm;
    case PARAM_VOLUME_f:
    case PARAM_VOLUME_dB:
    case PARAM_VOLUME_PERC:
      volume_factor = 0; /* silence gcc */
      switch (param_id)
	{
	case PARAM_VOLUME_f:
	  volume_factor = sfi_value_get_real (value);
	  break;
	case PARAM_VOLUME_dB:
	  volume_factor = bse_dB_to_factor (sfi_value_get_real (value));
	  break;
	case PARAM_VOLUME_PERC:
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
    case PARAM_BPM:
      bpm = sfi_value_get_int (value);
      self->bpm = bpm;
      bse_song_update_tpsi_SL (self);
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
    case PARAM_VOLUME_f:
      sfi_value_set_real (value, self->volume_factor);
      break;
    case PARAM_VOLUME_dB:
      sfi_value_set_real (value, bse_dB_from_factor (self->volume_factor, BSE_MIN_VOLUME_dB));
      break;
    case PARAM_VOLUME_PERC:
      sfi_value_set_int (value, self->volume_factor * 100.0 + 0.5);
      break;
    case PARAM_BPM:
      sfi_value_set_int (value, self->bpm);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
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
bse_song_add_item (BseContainer *container,
		   BseItem	*item)
{
  BseSong *self = BSE_SONG (container);

  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_TRACK))
    bse_track_add_modules (BSE_TRACK (item), container, self->context_merger);

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

void
bse_song_set_bpm (BseSong *self,
		  guint	   bpm)
{
  g_return_if_fail (BSE_IS_SONG (self));
  g_return_if_fail (bpm >= BSE_MIN_BPM && bpm <= BSE_MAX_BPM);
  
  g_object_set (self,
		"bpm", bpm,
		NULL);
}

static void
bse_song_ht_foreach (gpointer key,
		     gpointer value,
		     gpointer user_data)
{
  GList **list_p;
  
  list_p = user_data;
  *list_p = g_list_prepend (*list_p, value);
}

static void
bse_song_store_after (BseObject  *object,
		      BseStorage *storage)
{
  // BseSong *song = BSE_SONG (object);

  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->store_after)
    BSE_OBJECT_CLASS (parent_class)->store_after (object, storage);
}

static BseTokenType
bse_song_restore_private (BseObject  *object,
			  BseStorage *storage)
{
  // BseSong *song = BSE_SONG (object);
  GScanner *scanner = storage->scanner;
  GTokenType expected_token;
  
  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->restore_private)
    expected_token = BSE_OBJECT_CLASS (parent_class)->restore_private (object, storage);
  else
    expected_token = BSE_TOKEN_UNMATCHED;

  if (expected_token != BSE_TOKEN_UNMATCHED ||
      g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return expected_token;

  return expected_token;
}

static GTokenType
bse_song_restore (BseObject  *object,
		  BseStorage *storage)
{
  // BseSong *song = BSE_SONG (object);
  GTokenType expected_token;
  
  /* chain parent class' handler */
  expected_token = BSE_OBJECT_CLASS (parent_class)->restore (object, storage);

  return expected_token;
}

static void
bse_song_update_tpsi_SL (BseSong *self)
{
  gdouble tpqn = 384; 			/* ticks per quarter note */
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
  BseSong *song = BSE_SONG (source);

  bse_object_lock (BSE_OBJECT (song));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);

  bse_song_update_tpsi_SL (song);
}

static void
bse_song_context_create (BseSource *source,
			 guint      context_handle,
			 GslTrans  *trans)
{
  BseSong *self = BSE_SONG (source);
  BseSNet *snet = BSE_SNET (self);
  SfiRing *ring;

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);

  if (!bse_snet_context_is_branch (snet, context_handle))       /* catch recursion */
    for (ring = self->tracks_SL; ring; ring = sfi_ring_walk (ring, self->tracks_SL))
      bse_track_clone_voices (ring->data, snet, context_handle, trans);
}

static void
bse_song_reset (BseSource *source)
{
  BseSong *song = BSE_SONG (source);

  bse_ssequencer_handle_jobs (sfi_ring_prepend (NULL, bse_ssequencer_job_stop_super (BSE_SUPER (song))));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);

  bse_object_unlock (BSE_OBJECT (song));
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
