/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997, 1998, 1999 Olaf Hoehmann and Tim Janik
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
#include	"bsesong.h"

#include	"bsetrack.h"
#include	"bsepart.h"
#include	"bsecontextmerger.h"
#include	"bsepcmoutput.h"
#include	"bsesongthread.h"
#include	"bseproject.h"
#include	"bsestorage.h"
#include	"bsemarshal.h"
#include	"bsemain.h"
#include	<string.h>


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
static void	 bse_song_do_destroy		(BseObject	   *object);
static void	 bse_song_set_property		(BseSong	   *song,
						 guint              param_id,
						 GValue            *value,
						 GParamSpec        *pspec);
static void	 bse_song_get_property		(BseSong	   *song,
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
  
  gobject_class->set_property = (GObjectSetPropertyFunc) bse_song_set_property;
  gobject_class->get_property = (GObjectGetPropertyFunc) bse_song_get_property;

  object_class->store_after = bse_song_store_after;
  object_class->restore = bse_song_restore;
  object_class->restore_private = bse_song_restore_private;
  object_class->destroy = bse_song_do_destroy;
  
  source_class->prepare = bse_song_prepare;
  source_class->context_create = bse_song_context_create;
  source_class->reset = bse_song_reset;
  
  container_class->add_item = bse_song_add_item;
  container_class->remove_item = bse_song_remove_item;
  container_class->forall_items = bse_song_forall_items;
  
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_VOLUME_f,
			      bse_param_spec_float ("volume_f", "Master [float]", NULL,
						    0, bse_dB_to_factor (BSE_MAX_VOLUME_dB),
						    bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB), 0.1,
						    BSE_PARAM_STORAGE));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_VOLUME_dB,
			      bse_param_spec_float ("volume_dB", "Master [dB]", NULL,
						    BSE_MIN_VOLUME_dB, BSE_MAX_VOLUME_dB,
						    BSE_DFL_MASTER_VOLUME_dB, BSE_STP_VOLUME_dB,
						    BSE_PARAM_GUI |
						    BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_VOLUME_PERC,
			      bse_param_spec_uint ("volume_perc", "Master [%]", NULL,
						   0, bse_dB_to_factor (BSE_MAX_VOLUME_dB) * 100,
						   bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB) * 100, 1,
						   BSE_PARAM_GUI |
						   BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_BPM,
			      bse_param_spec_uint ("bpm", "Beats per minute", NULL,
						   BSE_MIN_BPM, BSE_MAX_BPM,
						   BSE_DFL_SONG_BPM, BSE_STP_BPM,
						   BSE_PARAM_DEFAULT |
						   BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Playback Settings",
			      PARAM_AUTO_ACTIVATE,
			      g_param_spec_boolean ("auto_activate", NULL, NULL,
						    TRUE, /* change default */
						    /* override parent property */ 0));
}

static void
bse_song_init (BseSong *self)
{
  BSE_OBJECT_UNSET_FLAGS (self, BSE_SNET_FLAG_USER_SYNTH);
  BSE_SUPER (self)->auto_activate = TRUE;
  self->bpm = BSE_DFL_SONG_BPM;
  self->volume_factor = bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB);
  
  self->parts = NULL;
  self->tracks = NULL;

  /* context merger */
  self->context_merger = bse_container_new_item (BSE_CONTAINER (self), BSE_TYPE_CONTEXT_MERGER, NULL);
  BSE_OBJECT_SET_FLAGS (self->context_merger, BSE_ITEM_FLAG_STORAGE_IGNORE);

  /* output */
  self->output = bse_container_new_item (BSE_CONTAINER (self), BSE_TYPE_PCM_OUTPUT, NULL);
  BSE_OBJECT_SET_FLAGS (self->output, BSE_ITEM_FLAG_STORAGE_IGNORE);

  /* context merger <-> output */
  bse_source_must_set_input (self->output, BSE_PCM_OUTPUT_ICHANNEL_LEFT,
			     self->context_merger, 0);
  bse_source_must_set_input (self->output, BSE_PCM_OUTPUT_ICHANNEL_RIGHT,
			     self->context_merger, 1);
}

static void
bse_song_do_destroy (BseObject *object)
{
  BseSong *self = BSE_SONG (object);

  bse_container_remove_item (BSE_CONTAINER (self), BSE_ITEM (self->context_merger));
  self->context_merger = NULL;
  bse_container_remove_item (BSE_CONTAINER (self), BSE_ITEM (self->output));
  self->output = NULL;

  while (self->parts)
    bse_container_remove_item (BSE_CONTAINER (self), self->parts->data);
  while (self->tracks)
    bse_container_remove_item (BSE_CONTAINER (self), self->tracks->data);
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}


static void
bse_song_set_property (BseSong     *song,
		       guint        param_id,
		       GValue      *value,
		       GParamSpec  *pspec)
{
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
	  volume_factor = g_value_get_float (value);
	  break;
	case PARAM_VOLUME_dB:
	  volume_factor = bse_dB_to_factor (g_value_get_float (value));
	  break;
	case PARAM_VOLUME_PERC:
	  volume_factor = g_value_get_uint (value) / 100.0;
	  break;
	}
      BSE_SEQUENCER_LOCK ();
      song->volume_factor = volume_factor;
      BSE_SEQUENCER_UNLOCK ();
      bse_object_param_changed (BSE_OBJECT (song), "volume_dB");
      bse_object_param_changed (BSE_OBJECT (song), "volume_perc");
      bse_object_param_changed (BSE_OBJECT (song), "volume_f");
      break;
    case PARAM_BPM:
      bpm = g_value_get_uint (value);
      BSE_SEQUENCER_LOCK ();
      song->bpm = bpm;
      BSE_SEQUENCER_UNLOCK ();
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (song, param_id, pspec);
      break;
    }
}

static void
bse_song_get_property (BseSong     *song,
		       guint        param_id,
		       GValue      *value,
		       GParamSpec  *pspec)
{
  switch (param_id)
    {
    case PARAM_VOLUME_f:
      g_value_set_float (value, song->volume_factor);
      break;
    case PARAM_VOLUME_dB:
      g_value_set_float (value, bse_dB_from_factor (song->volume_factor, BSE_MIN_VOLUME_dB));
      break;
    case PARAM_VOLUME_PERC:
      g_value_set_uint (value, song->volume_factor * 100.0 + 0.5);
      break;
    case PARAM_BPM:
      g_value_set_uint (value, song->bpm);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (song, param_id, pspec);
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
  BseSong *song;
  
  song = BSE_SONG (container);
  
  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_TRACK))
    bse_track_add_modules (BSE_TRACK (item), BSE_CONTAINER (song), song->context_merger);

  BSE_SEQUENCER_LOCK ();

  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_TRACK))
    song->tracks = g_list_append (song->tracks, item);
  else if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_PART))
    song->parts = g_list_append (song->parts, item);
  else
    /* parent class manages BseSources */ ;

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
  GList *list;
  
  song = BSE_SONG (container);
  
  list = song->tracks;
  while (list)
    {
      BseItem *item;
      
      item = list->data;
      list = list->next;
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
      g_assert (!BSE_SOURCE_PREPARED (song));
      bse_track_remove_modules (BSE_TRACK (item), BSE_CONTAINER (song));
      list_p = &song->tracks;
    }
  else if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_PART))
    list_p = &song->parts;
  else
    /* parent class manages BseSources */ ;

  BSE_SEQUENCER_LOCK ();
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
  BSE_SEQUENCER_UNLOCK ();

  /* chain parent class' remove_item handler */
  BSE_CONTAINER_CLASS (parent_class)->remove_item (container, item);
}

void
bse_song_set_bpm (BseSong *song,
		  guint	    bpm)
{
  g_return_if_fail (BSE_IS_SONG (song));
  g_return_if_fail (bpm >= BSE_MIN_BPM && bpm <= BSE_MAX_BPM);
  
  bse_object_set (BSE_OBJECT (song),
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
bse_song_prepare (BseSource *source)
{
  BseSong *song = BSE_SONG (source);

  bse_object_lock (BSE_OBJECT (song));

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
  
  song->sequencer = bse_song_sequencer_setup (song);
}

static void
bse_song_context_create (BseSource *source,
			 guint      context_handle,
			 GslTrans  *trans)
{
  BseSong *self = BSE_SONG (source);
  BseSNet *snet = BSE_SNET (self);
  GList *list;
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);

  if (!bse_snet_context_is_branch (snet, context_handle))       /* catch recursion */
    for (list = self->tracks; list; list = list->next)
      bse_track_clone_voices (list->data, snet, context_handle, trans);
}

static void
bse_song_reset (BseSource *source)
{
  BseSong *song = BSE_SONG (source);

  bse_song_sequencer_destroy (song->sequencer);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);

  bse_object_unlock (BSE_OBJECT (song));
}
