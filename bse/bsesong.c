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
#include	"bsepattern.h"
#include	"bsepatterngroup.h"
#include	"bsesongthread.h"
#include	"bseproject.h"
#include	"bsechunk.h"
#include	"bsestorage.h"
#include	"bsemarshal.h"
#include	"bsemain.h"
#include	<string.h>


enum
{
  PARAM_0,
  PARAM_N_CHANNELS,
  PARAM_PATTERN_LENGTH,
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
static void	 song_set_n_channels		(BseSong	   *song,
						 guint		    n_channels);


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
			      PARAM_N_CHANNELS,
			      bse_param_spec_uint ("n_channels", "Number of Channels", NULL,
						 1, BSE_MAX_N_CHANNELS,
						 BSE_DFL_SONG_N_CHANNELS, BSE_STP_N_CHANNELS,
						 BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, NULL,
			      PARAM_PATTERN_LENGTH,
			      bse_param_spec_uint ("pattern_length", "Pattern length", NULL,
						 BSE_MIN_PATTERN_LENGTH, BSE_MAX_PATTERN_LENGTH,
						 BSE_DFL_SONG_PATTERN_LENGTH, BSE_STP_PATTERN_LENGTH,
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
									     G_TYPE_NONE,
									     2, BSE_TYPE_PATTERN_GROUP, G_TYPE_UINT);
  song_signals[SIGNAL_PATTERN_GROUP_REMOVED] = bse_object_class_add_signal (object_class, "pattern_group_removed",
									    bse_marshal_VOID__OBJECT_UINT,
									    G_TYPE_NONE,
									    2, BSE_TYPE_PATTERN_GROUP, G_TYPE_UINT);
}

static void
bse_song_init (BseSong *song)
{
  BSE_OBJECT_UNSET_FLAGS (song, BSE_SNET_FLAG_FINAL);
  BSE_SUPER (song)->auto_activate = TRUE;
  song->bpm = BSE_DFL_SONG_BPM;
  song->volume_factor = bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB);
  song->pattern_length = BSE_DFL_SONG_PATTERN_LENGTH;
  song->n_channels = 0;
  
  song->instruments = NULL;
  song->patterns = NULL;
  song->pattern_groups = NULL;
  
  song->n_pgroups = 0;
  song->pgroups = NULL;

  song_set_n_channels (song, BSE_DFL_SONG_N_CHANNELS);
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
  while (song->instruments)
    bse_container_remove_item (BSE_CONTAINER (song), song->instruments->data);

  song_set_n_channels (song, 0);

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
      GList *list;
      gfloat volume_factor;
      guint bpm;

    case PARAM_N_CHANNELS:
      /* we silently ignore this parameter during playing phase */
      if (!BSE_OBJECT_IS_LOCKED (song))
	{
	  song_set_n_channels (song, g_value_get_uint (value));
	  for (list = song->patterns; list; list = list->next)
	    bse_pattern_set_n_channels (list->data, song->n_channels);
	}
      break;
    case PARAM_PATTERN_LENGTH:
      /* we silently ignore this parameter during playing phase */
      if (!BSE_OBJECT_IS_LOCKED (song))
	{
	  song->pattern_length = g_value_get_uint (value);
	  for (list = song->patterns; list; list = list->next)
	    bse_pattern_set_n_rows (list->data, song->pattern_length);
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
    case PARAM_PATTERN_LENGTH:
      g_value_set_uint (value, song->pattern_length);
      break;
    case PARAM_N_CHANNELS:
      g_value_set_uint (value, song->n_channels);
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

void
bse_song_set_pattern_length (BseSong *song,
			     guint    pattern_length)
{
  g_return_if_fail (BSE_IS_SONG (song));
  g_return_if_fail (pattern_length >= BSE_MIN_PATTERN_LENGTH);
  g_return_if_fail (pattern_length <= BSE_MAX_PATTERN_LENGTH);
  
  bse_object_set (BSE_OBJECT (song),
		  "pattern_length", pattern_length,
		  NULL);
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
	if (bse_string_equals (BSE_OBJECT_ULOC (list->data), "Default"))
	  return list->data;

      return song->pgroups[song->n_pgroups - 1];
    }
  else
    {
      BseItem *item;
      BsePatternGroup *pgroup;

      item = bse_container_new_item (BSE_CONTAINER (song), BSE_TYPE_PATTERN_GROUP,
				     "name", "Default",
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
				 "name", BSE_OBJECT_ULOC (src_pgroup),
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





#if 0
BsePattern*
bse_song_get_pattern_from_list (BseSong	*song,
				guint	 pattern_index)
{
  GList *list;
  
  g_return_val_if_fail (BSE_IS_SONG (song), NULL);
  
  // FIXME (bse_song_get_pattern_from_list(): test implementation);
  
  list = g_list_nth (song->patterns, pattern_index);
  
  if (list)
    return list->data;
  else
    return NULL;
}
#endif

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
  BseProject *project = bse_item_get_project (BSE_ITEM (song));
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
      gchar *path = bse_container_make_item_path (BSE_CONTAINER (project),
						  BSE_ITEM (song->pgroups[i]),
						  FALSE);

      bse_storage_break (storage);

      bse_storage_putc (storage, '(');
      bse_storage_puts (storage, "add-pattern-group");
      bse_storage_printf (storage, " %s", path);
      bse_storage_handle_break (storage);
      bse_storage_putc (storage, ')');
      g_free (path);
    }
}

static BseTokenType
bse_song_restore_private (BseObject  *object,
			  BseStorage *storage)
{
  BseSong *song = BSE_SONG (object);
  BseProject *project = bse_item_get_project (BSE_ITEM (song));
  GScanner *scanner = storage->scanner;
  GTokenType expected_token;
  gchar *pgroup_path;
  BseItem *item;
  
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

  /* parse pgroup path */
  if (g_scanner_get_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return G_TOKEN_IDENTIFIER;
  pgroup_path = g_strdup (scanner->value.v_identifier);

  /* ok, resolve and add pgroup */
  item = bse_container_item_from_path (BSE_CONTAINER (project), pgroup_path);
  if (!item || !BSE_IS_PATTERN_GROUP (item))
    bse_storage_warn (storage,
		      "%s: unable to determine pattern group from \"%s\"",
		      BSE_OBJECT_ULOC (song),
		      pgroup_path);
  else
    bse_song_insert_pattern_group_link (song, BSE_PATTERN_GROUP (item), song->n_pgroups);
  g_free (pgroup_path);

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
   * usre that all pattern_groups of this song are actually
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

#if 0
#include	"bseio.h"
void
bse_song_reload_instrument_samples (BseSong	     *song,
				    BseSampleLookupCB cb_func,
				    gpointer	      cb_data)
{
  GList *list;
  
  g_return_if_fail (BSE_IS_SONG (song));
  
  for (list = song->instruments; list; list = list->next)
    {
      BseInstrument *instrument;
      
      instrument = list->data;
      if (instrument->type == BSE_INSTRUMENT_SAMPLE &&
	  instrument->deferred_sample_name)
	{
	  BseSample *sample;
	  
	  sample = bse_sample_lookup (BSE_SUPER_PROJECT (song), instrument->deferred_sample_name);
	  if (sample && sample != instrument->sample)
	    bse_instrument_set_sample (instrument, sample);
	  else if (!sample && cb_func)
	    {
	      BseIoData *io_data;
	      
	      io_data = cb_func (cb_data,
				 instrument->deferred_sample_name,
				 instrument->deferred_sample_path);
	      sample = bse_sample_lookup (BSE_SUPER_PROJECT (song), instrument->deferred_sample_name);
	      if (!sample)
		{
		  gchar *path;
		  
		  if (io_data)
		    bse_io_data_destroy (io_data);
		  io_data = NULL;
		  
		  path = NULL; FIXME (bse_sample_lookup_path (instrument->deferred_sample_name););
		  if (path && !g_str_equal (path, instrument->deferred_sample_path))
		    io_data = cb_func (cb_data,
				       instrument->deferred_sample_name,
				       path);
		  sample = bse_sample_lookup (BSE_SUPER_PROJECT (song), instrument->deferred_sample_name);
		}
	      if (sample && sample != instrument->sample)
		bse_instrument_set_sample (instrument, sample);
	      if (io_data)
		bse_io_data_destroy (io_data);
	    }
	  if (sample)
	    {
	      g_free (instrument->deferred_sample_name);
	      instrument->deferred_sample_name = NULL;
	      g_free (instrument->deferred_sample_path);
	      instrument->deferred_sample_path = NULL;
	    }
	}
    }
}
#endif

static void
song_set_n_channels (BseSong *song,
		     guint    n_channels)
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

      _bse_source_set_input (net->output, 0, net->lmixer, 0);
      _bse_source_set_input (net->output, 1, net->rmixer, 0);
    }

  for (i = n_channels; i < song->n_channels; i++)
    {
      BseItem *item;

      item = BSE_ITEM (net->voices[i].ofreq);
      bse_container_remove_item (container, item);

      item = BSE_ITEM (net->voices[i].synth);
      bse_container_remove_item (container, item);
    }

  i = song->n_channels;
  song->n_channels = n_channels;
  net->voices = g_renew (BseSongVoice, net->voices, song->n_channels);

  for (; i < song->n_channels; i++)
    {
      BseSource *source;

      source = g_object_new (g_type_from_name ("BseConstant"),
			     "value_1", 0.0,
			     "value_2", 0.0,
			     "value_3", 0.0,
			     "value_4", 0.0,
			     NULL);
      BSE_OBJECT_SET_FLAGS (source, BSE_ITEM_FLAG_STORAGE_IGNORE);
      net->voices[i].ofreq = source;
      bse_container_add_item (container, BSE_ITEM (source));
      g_object_unref (source);
      
      source = g_object_new (g_type_from_name ("BseSubSynth"),
			     "in_port_1", "frequency",
			     "in_port_2", "gate",
			     "in_port_3", "velocity",
			     "in_port_4", "aftertouch",
			     "out_port_1", "left_out",
			     "out_port_2", "right_out",
			     "out_port_3", "unused",
			     "out_port_4", "synth_done",
			     NULL);
      BSE_OBJECT_SET_FLAGS (source, BSE_ITEM_FLAG_STORAGE_IGNORE);
      net->voices[i].synth = source;
      bse_container_add_item (container, BSE_ITEM (source));
      g_object_unref (source);

      _bse_source_set_input (net->voices[i].synth, 0, net->voices[i].ofreq, 0);
      _bse_source_set_input (net->voices[i].synth, 1, net->voices[i].ofreq, 1);
      _bse_source_set_input (net->voices[i].synth, 2, net->voices[i].ofreq, 2);
      _bse_source_set_input (net->voices[i].synth, 3, net->voices[i].ofreq, 3);

      if (i < 4) // FIXME
	{
	  _bse_source_set_input (net->lmixer, i, net->voices[i].synth, 0);
	  _bse_source_set_input (net->rmixer, i, net->voices[i].synth, 1);
	}
    }
}

static void
bse_song_prepare (BseSource *source)
{
  BseSong *song = BSE_SONG (source);

  bse_object_lock (BSE_OBJECT (song));
  
  song->sequencer_index = 0;
  song->sequencer = bse_song_sequencer_setup (song);

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

#if 0
void
bse_song_update_sequencer (BseSong *song)
{
  g_return_if_fail (BSE_IS_SONG (song));

  if (song->sequencer)
    {
      BseSource *source = BSE_SOURCE (song);

      if (song->sequencer_index < 0) // FIXME: source->index
	{
	  song->sequencer_index++;
	  bse_song_sequencer_step (song);
	}
    }
}

static BseChunk*
bse_song_calc_chunk (BseSource *source,
		     guint	ochannel_id)
{
  BseSong *song = BSE_SONG (source);
  BseSampleValue *hunk;
  
  g_return_val_if_fail (ochannel_id == BSE_SONG_OCHANNEL_STEREO, NULL);
  
  /* FIXME: we need some kinda notification mechanism on sources when
   * the source stopped playing
   */
  if (song->sequencer_index < source->index)
    {
      song->sequencer_index++;
      bse_song_sequencer_step (song);
    }
  
  hunk = bse_hunk_alloc0 (2);
  bse_song_sequencer_fill_hunk (song, hunk);
  
  return bse_chunk_new_orphan (2, hunk);
}
#endif

static void
bse_song_reset (BseSource *source)
{
  BseSong *song = BSE_SONG (source);

  song->sequencer_index = 0;
  bse_song_sequencer_destroy (song->sequencer);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);

  bse_object_unlock (BSE_OBJECT (song));
}
