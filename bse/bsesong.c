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

#include	"bseinstrument.h"
#include	"bsepart.h"
#include	"bsepattern.h"
#include	"bsepatterngroup.h"
#include	"bsesongthread.h"
#include	"bseproject.h"
#include	"bsestorage.h"
#include	"bsemarshal.h"
#include	"bsemain.h"
#include	<string.h>


enum
{
  PARAM_0,
  PARAM_N_VOICES,
  PARAM_VOLUME_f,
  PARAM_VOLUME_dB,
  PARAM_VOLUME_PERC,
  PARAM_BPM,
  PARAM_AUTO_ACTIVATE
};

enum {
  SIGNAL_PATTERN_GROUP_INSERTED,
  SIGNAL_PATTERN_GROUP_REMOVED,
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
static void	 bse_song_remove_pgroup_links	(BseSong           *song,
						 BsePatternGroup   *pgroup);
static BseTokenType bse_song_restore_private    (BseObject         *object,
						 BseStorage        *storage);
static void      bse_song_store_after		(BseObject         *object,
						 BseStorage        *storage);
static GTokenType bse_song_restore              (BseObject         *object,
						 BseStorage        *storage);
static void	 bse_song_prepare		(BseSource	   *source);
static void	 bse_song_reset			(BseSource	   *source);
static void	 song_set_n_voices		(BseSong	   *song,
						 guint		    n_voices);


/* --- variables --- */
static GTypeClass *parent_class = NULL;
static guint       song_signals[SIGNAL_LAST] = { 0, };


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
  // source_class->calc_chunk = bse_song_calc_chunk;
  source_class->reset = bse_song_reset;
  
  container_class->add_item = bse_song_add_item;
  container_class->remove_item = bse_song_remove_item;
  container_class->forall_items = bse_song_forall_items;
  
  bse_object_class_add_param (object_class, NULL,
			      PARAM_N_VOICES,
			      bse_param_spec_uint ("n_voices", "Max # of Voices", NULL,
						   1, 256,
						   16, 1,
						   BSE_PARAM_DEFAULT));
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
  
  song_signals[SIGNAL_PATTERN_GROUP_INSERTED] = bse_object_class_add_signal (object_class, "pattern_group_inserted",
									     bse_marshal_VOID__OBJECT_UINT,
									     bse_marshal_VOID__POINTER_UINT,
									     G_TYPE_NONE,
									     2, BSE_TYPE_PATTERN_GROUP, G_TYPE_UINT);
  song_signals[SIGNAL_PATTERN_GROUP_REMOVED] = bse_object_class_add_signal (object_class, "pattern_group_removed",
									    bse_marshal_VOID__OBJECT_UINT,
									    bse_marshal_VOID__POINTER_UINT,
									    G_TYPE_NONE,
									    2, BSE_TYPE_PATTERN_GROUP, G_TYPE_UINT);
}

static void
bse_song_init (BseSong *song)
{
  BSE_OBJECT_UNSET_FLAGS (song, BSE_SNET_FLAG_USER_SYNTH);
  BSE_SUPER (song)->auto_activate = TRUE;
  song->bpm = BSE_DFL_SONG_BPM;
  song->volume_factor = bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB);
  song->net.n_voices = 0;
  
  song->instruments = NULL;
  song->parts = NULL;

  song_set_n_voices (song, 16);

  /* legacy */
  song->patterns = NULL;
  song->pattern_groups = NULL;
  song->n_pgroups = 0;
  song->pgroups = NULL;
}

static void
bse_song_do_destroy (BseObject *object)
{
  BseSong *song = BSE_SONG (object);
  BseContainer *container = BSE_CONTAINER (song);
  
  while (song->pattern_groups)
    bse_container_remove_item (BSE_CONTAINER (song), song->pattern_groups->data);
  song->n_pgroups = 0;
  g_free (song->pgroups);
  song->pgroups = NULL;
  while (song->patterns)
    bse_container_remove_item (BSE_CONTAINER (song), song->patterns->data);
  while (song->parts)
    bse_container_remove_item (BSE_CONTAINER (song), song->parts->data);
  while (song->instruments)
    bse_container_remove_item (BSE_CONTAINER (song), song->instruments->data);

  song_set_n_voices (song, 0);

  if (song->net.lmixer)
    {
      BseSongNet *net = &song->net;

      bse_container_remove_item (container, BSE_ITEM (net->lmixer));
      net->lmixer = NULL;
      bse_container_remove_item (container, BSE_ITEM (net->rmixer));
      net->rmixer = NULL;
      bse_container_remove_item (container, BSE_ITEM (net->output));
      net->output = NULL;
    }
  g_free (song->net.voices);
  
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

    case PARAM_N_VOICES:
      /* we silently ignore this parameter during playing phase */
      if (!BSE_OBJECT_IS_LOCKED (song))
	{
	  song_set_n_voices (song, g_value_get_uint (value));
	}
      break;
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
    case PARAM_N_VOICES:
      g_value_set_uint (value, song->net.n_voices);
      break;
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
  
  BSE_SEQUENCER_LOCK ();

  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_INSTRUMENT))
    song->instruments = g_list_append (song->instruments, item);
  else if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_PART))
    song->parts = g_list_append (song->parts, item);
  else if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_PATTERN))
    song->patterns = g_list_append (song->patterns, item);
  else if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_PATTERN_GROUP))
    song->pattern_groups = g_list_append (song->pattern_groups, item);
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
  
  list = song->instruments;
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

  list = song->patterns;
  while (list)
    {
      BseItem *item;
      
      item = list->data;
      list = list->next;
      if (!func (item, data))
	return;
    }

  list = song->pattern_groups;
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
  
  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_INSTRUMENT))
    list_p = &song->instruments;
  else if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_PART))
    list_p = &song->parts;
  else if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_PATTERN))
    {
      BsePattern *pattern = BSE_PATTERN (item);
      guint i;

      for (i = 0; i < song->n_pgroups; i++)
	bse_pattern_group_remove_pattern (song->pgroups[i], pattern);
      list_p = &song->patterns;
    }
  else if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_PATTERN_GROUP))
    {
      bse_song_remove_pgroup_links (song, BSE_PATTERN_GROUP (item));
      list_p = &song->pattern_groups;
    }
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

BsePattern*
bse_song_get_pattern (BseSong *song,
		      guint    seqid)
{
  GList *list;
  
  g_return_val_if_fail (BSE_IS_SONG (song), NULL);
  g_return_val_if_fail (seqid > 0, NULL);
  
  list = g_list_nth (song->patterns, seqid - 1);
  if (list)
    return list->data;
  else
    return NULL;
}

BsePatternGroup*
bse_song_get_default_pattern_group (BseSong *song)
{
  g_return_val_if_fail (BSE_IS_SONG (song), NULL);

  if (song->n_pgroups)
    {
      GList *list;

      for (list = song->pattern_groups; list; list = list->next)
	if (bse_string_equals (BSE_OBJECT_UNAME (list->data), "Default"))
	  return list->data;

      return song->pgroups[song->n_pgroups - 1];
    }
  else
    {
      BseItem *item;
      BsePatternGroup *pgroup;

      item = bse_container_new_item (BSE_CONTAINER (song), BSE_TYPE_PATTERN_GROUP,
				     "uname", "Default",
				     NULL);
      pgroup = BSE_PATTERN_GROUP (item);
      bse_song_insert_pattern_group_link (song, pgroup, 0);

      return pgroup;
    }
}

void
bse_song_insert_pattern_group_link (BseSong         *song,
				    BsePatternGroup *pgroup,
				    gint             position)
{
  guint n;

  g_return_if_fail (BSE_IS_SONG (song));
  g_return_if_fail (BSE_IS_PATTERN_GROUP (pgroup));
  g_return_if_fail (BSE_ITEM (pgroup)->parent == BSE_ITEM (song));

  if (position < 0 || position > song->n_pgroups)
    position = song->n_pgroups;

  BSE_SEQUENCER_LOCK ();
  n = song->n_pgroups++;
  song->pgroups = g_renew (BsePatternGroup*, song->pgroups, song->n_pgroups);
  g_memmove (song->pgroups + position + 1,
	     song->pgroups + position,
	     sizeof (BsePatternGroup*) * (n - position));
  song->pgroups[position] = pgroup;
  BSE_SEQUENCER_UNLOCK ();
  
  bse_object_ref (BSE_OBJECT (pgroup));
  g_signal_emit (song, song_signals[SIGNAL_PATTERN_GROUP_INSERTED], 0, pgroup, position);
  bse_object_unref (BSE_OBJECT (pgroup));
}

void
bse_song_remove_pattern_group_entry (BseSong *song,
				     gint     position)
{
  g_return_if_fail (BSE_IS_SONG (song));

  if (position < 0)
    position = song->n_pgroups - 1;
  if (position < song->n_pgroups)
    {
      BsePatternGroup *pgroup = song->pgroups[position];
      guint i;
      
      /* remove this pgroup completely from song if position is its last (only) link */
      for (i = 0; i < song->n_pgroups; i++)
	if (i != position && song->pgroups[i] == pgroup)
	  break;
      if (i >= song->n_pgroups)
	{
	  bse_container_remove_item (BSE_CONTAINER (song), BSE_ITEM (pgroup));
	  return;
	}

      /* just remove link */
      BSE_SEQUENCER_LOCK ();
      song->n_pgroups--;
      g_memmove (song->pgroups + position,
		 song->pgroups + position + 1,
		 sizeof (BsePatternGroup*) * (song->n_pgroups - position));
      BSE_SEQUENCER_UNLOCK ();
      
      bse_object_ref (BSE_OBJECT (pgroup));
      g_signal_emit (song, song_signals[SIGNAL_PATTERN_GROUP_REMOVED], 0, pgroup, position);
      bse_object_unref (BSE_OBJECT (pgroup));
    }
}

static void
bse_song_remove_pgroup_links (BseSong         *song,
			      BsePatternGroup *pgroup)
{
  BsePatternGroup **last, **cur, **bound;
  GSList *slist, *remove_positions = NULL;

  g_return_if_fail (BSE_IS_SONG (song));
  g_return_if_fail (BSE_IS_PATTERN_GROUP (pgroup));

  cur = song->pgroups;
  last = cur;
  bound = cur + song->n_pgroups;
  BSE_SEQUENCER_LOCK ();
  while (cur < bound)
    {
      if (*cur != pgroup)
	{
	  if (last != cur)
	    *last = *cur;
	  last++;
	}
      else
	remove_positions = g_slist_prepend (remove_positions, GUINT_TO_POINTER (cur - song->pgroups));
      cur++;
    }
  song->n_pgroups = last - song->pgroups;
  BSE_SEQUENCER_UNLOCK ();
  
  bse_object_ref (BSE_OBJECT (song));
  bse_object_ref (BSE_OBJECT (pgroup));

  for (slist = remove_positions; slist; slist = slist->next)
    g_signal_emit (song, song_signals[SIGNAL_PATTERN_GROUP_REMOVED], 0, pgroup, GPOINTER_TO_UINT (slist->data));
  g_slist_free (remove_positions);

  bse_object_unref (BSE_OBJECT (pgroup));
  bse_object_unref (BSE_OBJECT (song));
}

void
bse_song_insert_pattern_group_copy (BseSong         *song,
				    BsePatternGroup *src_pgroup,
				    gint             position)
{
  BseItem *item;
  BsePatternGroup *pgroup;
  gchar *blurb = NULL;

  g_return_if_fail (BSE_IS_SONG (song));
  g_return_if_fail (BSE_IS_PATTERN_GROUP (src_pgroup));
  g_return_if_fail (BSE_ITEM (src_pgroup)->parent == BSE_ITEM (song));

  bse_object_ref (BSE_OBJECT (song));
  bse_object_ref (BSE_OBJECT (src_pgroup));

  g_object_get (G_OBJECT (src_pgroup), "blurb", &blurb, NULL);
  item = bse_container_new_item (BSE_CONTAINER (song), BSE_TYPE_PATTERN_GROUP,
				 "uname", BSE_OBJECT_UNAME (src_pgroup),
				 "blurb", blurb,
				 NULL);
  pgroup = BSE_PATTERN_GROUP (item);

  bse_object_ref (BSE_OBJECT (pgroup));

  bse_song_insert_pattern_group_link (song, pgroup, position);

  bse_pattern_group_clone_contents (pgroup, src_pgroup);
  
  bse_object_unref (BSE_OBJECT (pgroup));
  bse_object_unref (BSE_OBJECT (src_pgroup));
  bse_object_unref (BSE_OBJECT (song));
}

BsePattern*
bse_song_get_pattern_from_list (BseSong	*song,
				guint	 pattern_index)
{
  BsePatternGroup *pgroup = NULL;
  gint i;
  
  g_return_val_if_fail (BSE_IS_SONG (song), NULL);

  for (i = 0; i < song->n_pgroups; i++)
    {
      if (pattern_index < song->pgroups[i]->pattern_count)
	{
	  pgroup = song->pgroups[i];
	  break;
	}
      pattern_index -= song->pgroups[i]->pattern_count;
    }

  return pgroup ? bse_pattern_group_get_nth_pattern (pgroup, pattern_index) : NULL;
}

BseInstrument*
bse_song_get_instrument (BseSong *song,
			 guint	  seqid)
{
  GList *list;
  
  g_return_val_if_fail (BSE_IS_SONG (song), NULL);
  g_return_val_if_fail (seqid > 0, NULL);
  
  list = g_list_nth (song->instruments, seqid - 1);
  if (list)
    return list->data;
  else
    return NULL;
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
  BseSong *song = BSE_SONG (object);
  guint i;

  /* we store the pattern groups after the store_after() handler,
   * which is used by BseContainer to store all children.
   * that way we can use non-persistant references to pattern groups.
   */

  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->store_after)
    BSE_OBJECT_CLASS (parent_class)->store_after (object, storage);

  for (i = 0; i < song->n_pgroups; i++)
    {
      bse_storage_break (storage);

      bse_storage_putc (storage, '(');
      bse_storage_puts (storage, "add-pattern-group");
      bse_storage_put_item_link (storage, BSE_ITEM (song), BSE_ITEM (song->pgroups[i]));
      bse_storage_putc (storage, ')');
    }
}

static void
add_resolved_pgroup (gpointer     data,
		     BseStorage  *storage,
		     BseItem     *song,
		     BseItem     *pgroup,
		     const gchar *error)
{
  if (error)
    bse_storage_warn (storage, error);
  else if (BSE_IS_PATTERN_GROUP (pgroup))
    bse_song_insert_pattern_group_link (BSE_SONG (song), BSE_PATTERN_GROUP (pgroup), BSE_SONG (song)->n_pgroups);
}

static BseTokenType
bse_song_restore_private (BseObject  *object,
			  BseStorage *storage)
{
  BseSong *song = BSE_SONG (object);
  GScanner *scanner = storage->scanner;
  GTokenType expected_token;
  
  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->restore_private)
    expected_token = BSE_OBJECT_CLASS (parent_class)->restore_private (object, storage);
  else
    expected_token = BSE_TOKEN_UNMATCHED;

  if (expected_token != BSE_TOKEN_UNMATCHED ||
      g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER ||
      !bse_string_equals ("add-pattern-group", scanner->next_value.v_identifier))
    return expected_token;

  /* eat "add-pattern-group" */
  g_scanner_get_next_token (scanner);

  /* queue pgroup resolver */
  expected_token = bse_storage_parse_item_link (storage, BSE_ITEM (song), add_resolved_pgroup, NULL);
  if (expected_token != G_TOKEN_NONE)
    return expected_token;

  /* read closing brace */
  return g_scanner_get_next_token (scanner) == ')' ? G_TOKEN_NONE : ')';
}

static GTokenType
bse_song_restore (BseObject  *object,
		  BseStorage *storage)
{
  BseSong *song = BSE_SONG (object);
  GTokenType expected_token;
  GList *list;
  
  /* chain parent class' handler */
  expected_token = BSE_OBJECT_CLASS (parent_class)->restore (object, storage);

  /* ok, parsing is done, now we just want to make
   * sure that all pattern_groups of this song are actually
   * listed in the pgroups list
   */
  for (list = song->pattern_groups; list; list = list->next)
    {
      BsePatternGroup *pgroup = list->data;
      guint i;

      for (i = 0; i < song->n_pgroups; i++)
	if (song->pgroups[i] == pgroup)
	  break;

      /* if not, default-add to pgroup list end */
      if (i >= song->n_pgroups)
	bse_song_insert_pattern_group_link (song, pgroup, song->n_pgroups);
    }

  return expected_token;
}

static void
song_set_n_voices (BseSong *song,
		   guint    n_voices)
{
  BseContainer *container = BSE_CONTAINER (song);
  BseSongNet *net = &song->net;
  guint i;
  
  if (!net->lmixer)
    {
      /* initial setup */
      net->lmixer = g_object_new (g_type_from_name ("BseMixer"), NULL);	// FIXME
      BSE_OBJECT_SET_FLAGS (net->lmixer, BSE_ITEM_FLAG_STORAGE_IGNORE);
      bse_container_add_item (container, BSE_ITEM (net->lmixer));
      g_object_unref (net->lmixer);

      net->rmixer = g_object_new (g_type_from_name ("BseMixer"), NULL);	// FIXME
      BSE_OBJECT_SET_FLAGS (net->rmixer, BSE_ITEM_FLAG_STORAGE_IGNORE);
      bse_container_add_item (container, BSE_ITEM (net->rmixer));
      g_object_unref (net->rmixer);

      net->output = g_object_new (g_type_from_name ("BsePcmOutput"), NULL);	// FIXME
      BSE_OBJECT_SET_FLAGS (net->output, BSE_ITEM_FLAG_STORAGE_IGNORE);
      bse_container_add_item (container, BSE_ITEM (net->output));
      g_object_unref (net->output);

      bse_source_set_input (net->output, 0, net->lmixer, 0);
      bse_source_set_input (net->output, 1, net->rmixer, 0);
    }

  for (i = n_voices; i < song->net.n_voices; i++)
    {
      BseItem *item;

      item = BSE_ITEM (net->voices[i].constant);
      bse_container_remove_item (container, item);

      item = BSE_ITEM (net->voices[i].sub_synth);
      bse_container_remove_item (container, item);
    }

  i = song->net.n_voices;
  song->net.n_voices = n_voices;
  net->voices = g_renew (BseSongVoice, net->voices, song->net.n_voices);

  for (; i < song->net.n_voices; i++)
    {
      BseSource *source;

      source = g_object_new (g_type_from_name ("BseConstant"),
			     "value_1", 0.0,
			     "value_2", 0.0,
			     "value_3", 0.0,
			     "value_4", 0.0,
			     NULL);
      BSE_OBJECT_SET_FLAGS (source, BSE_ITEM_FLAG_STORAGE_IGNORE);
      net->voices[i].constant = source;
      bse_container_add_item (container, BSE_ITEM (source));
      g_object_unref (source);
      
      source = g_object_new (g_type_from_name ("BseSubSynth"),
			     "in_port_1", "frequency",
			     "in_port_2", "gate",
			     "in_port_3", "velocity",
			     "in_port_4", "aftertouch",
			     "out_port_1", "left-audio",
			     "out_port_2", "right-audio",
			     "out_port_3", "unused",
			     "out_port_4", "synth_done",
			     NULL);
      BSE_OBJECT_SET_FLAGS (source, BSE_ITEM_FLAG_STORAGE_IGNORE);
      net->voices[i].sub_synth = source;
      bse_container_add_item (container, BSE_ITEM (source));
      g_object_unref (source);

      bse_source_set_input (net->voices[i].sub_synth, 0, net->voices[i].constant, 0);
      bse_source_set_input (net->voices[i].sub_synth, 1, net->voices[i].constant, 1);
      bse_source_set_input (net->voices[i].sub_synth, 2, net->voices[i].constant, 2);
      bse_source_set_input (net->voices[i].sub_synth, 3, net->voices[i].constant, 3);

      if (i < 4) // FIXME
	{
	  bse_source_set_input (net->lmixer, i, net->voices[i].sub_synth, 0);
	  bse_source_set_input (net->rmixer, i, net->voices[i].sub_synth, 1);
	}
    }
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
bse_song_reset (BseSource *source)
{
  BseSong *song = BSE_SONG (source);

  bse_song_sequencer_destroy (song->sequencer);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);

  bse_object_unlock (BSE_OBJECT (song));
}
