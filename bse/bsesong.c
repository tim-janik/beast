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

#include	"bsesample.h"
#include	"bseinstrument.h"
#include	"bsepattern.h"
#include	"bsepatterngroup.h"
#include	"bsesongsequencer.h"
#include	"bseproject.h"
#include	"bsechunk.h"
#include	"bseheart.h"
#include	<string.h>
#include	<math.h>


/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_N_CHANNELS,
  PARAM_PATTERN_LENGTH,
  PARAM_VOLUME_f,
  PARAM_VOLUME_dB,
  PARAM_VOLUME_PERC,
  PARAM_BPM
};


/* --- prototypes --- */
static void	 bse_song_class_init		(BseSongClass	   *class);
static void	 bse_song_init			(BseSong	   *song);
static void	 bse_song_do_shutdown		(BseObject	   *object);
static void	 bse_song_set_param		(BseSong	   *song,
						 BseParam	   *param,
						 guint              param_id);
static void	 bse_song_get_param		(BseSong	   *song,
						 BseParam	   *param,
						 guint              param_id);
static void	 bse_song_add_item		(BseContainer	   *container,
						 BseItem	   *item);
static void	 bse_song_forall_items		(BseContainer	   *container,
						 BseForallItemsFunc func,
						 gpointer	    data);
static void	 bse_song_remove_item		(BseContainer	   *container,
						 BseItem	   *item);
static guint	 bse_song_item_seqid		(BseContainer	   *container,
						 BseItem	   *item);
static BseItem*	 bse_song_get_item		(BseContainer	   *container,
						 BseType	    item_type,
						 guint		    seqid);
static void	 bse_song_remove_pgroup_links	(BseSong           *song,
						 BsePatternGroup   *pgroup);
static void	 bse_song_prepare		(BseSource	   *source,
						 BseIndex	    index);
static BseChunk* bse_song_calc_chunk		(BseSource	   *source,
						 guint		    ochannel_id);
static void	 bse_song_reset			(BseSource	   *source);


/* --- variables --- */
static BseTypeClass	*parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseSong)
{
  static const BseTypeInfo song_info = {
    sizeof (BseSongClass),
    
    (BseBaseInitFunc) NULL,
    (BseBaseDestroyFunc) NULL,
    (BseClassInitFunc) bse_song_class_init,
    (BseClassDestroyFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseSong),
    4 /* n_preallocs */,
    (BseObjectInitFunc) bse_song_init,
  };
  
  return bse_type_register_static (BSE_TYPE_SUPER,
				   "BseSong",
				   "BSE Song type",
				   &song_info);
}

static void
bse_song_class_init (BseSongClass *class)
{
  BseObjectClass *object_class;
  BseSourceClass *source_class;
  BseContainerClass *container_class;
  BseSuperClass *super_class;
  guint ochannel_id;
  
  parent_class = bse_type_class_peek (BSE_TYPE_SUPER);
  object_class = BSE_OBJECT_CLASS (class);
  source_class = BSE_SOURCE_CLASS (class);
  container_class = BSE_CONTAINER_CLASS (class);
  super_class = BSE_SUPER_CLASS (class);
  
  object_class->set_param = (BseObjectSetParamFunc) bse_song_set_param;
  object_class->get_param = (BseObjectGetParamFunc) bse_song_get_param;
  object_class->shutdown = bse_song_do_shutdown;
  
  source_class->prepare = bse_song_prepare;
  source_class->calc_chunk = bse_song_calc_chunk;
  source_class->reset = bse_song_reset;
  
  container_class->add_item = bse_song_add_item;
  container_class->remove_item = bse_song_remove_item;
  container_class->forall_items = bse_song_forall_items;
  container_class->item_seqid = bse_song_item_seqid;
  container_class->get_item = bse_song_get_item;
  
  bse_object_class_add_param (object_class, NULL,
			      PARAM_N_CHANNELS,
			      bse_param_spec_uint ("n_channels", "Number of Channels", NULL,
						   1, BSE_MAX_N_CHANNELS,
						   BSE_STP_N_CHANNELS,
						   BSE_DFL_SONG_N_CHANNELS,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, NULL,
			      PARAM_PATTERN_LENGTH,
			      bse_param_spec_uint ("pattern_length", "Pattern length", NULL,
						   BSE_MIN_PATTERN_LENGTH, BSE_MAX_PATTERN_LENGTH,
						   BSE_STP_PATTERN_LENGTH,
						   BSE_DFL_SONG_PATTERN_LENGTH,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_VOLUME_f,
			      bse_param_spec_float ("volume_f", "Master [float]", NULL,
						    0, bse_dB_to_factor (BSE_MAX_VOLUME_dB),
						    0.1,
						    bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB),
						    BSE_PARAM_STORAGE));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_VOLUME_dB,
			      bse_param_spec_float ("volume_dB", "Master [dB]", NULL,
						    BSE_MIN_VOLUME_dB, BSE_MAX_VOLUME_dB,
						    BSE_STP_VOLUME_dB,
						    BSE_DFL_MASTER_VOLUME_dB,
						    BSE_PARAM_GUI |
						    BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_VOLUME_PERC,
			      bse_param_spec_uint ("volume_perc", "Master [%]", NULL,
						   0, bse_dB_to_factor (BSE_MAX_VOLUME_dB) * 100,
						   1,
						   bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB) * 100,
						   BSE_PARAM_GUI |
						   BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_BPM,
			      bse_param_spec_uint ("bpm", "Beats per minute", NULL,
						   BSE_MIN_BPM, BSE_MAX_BPM,
						   BSE_STP_BPM,
						   BSE_DFL_SONG_BPM,
						   BSE_PARAM_DEFAULT |
						   BSE_PARAM_HINT_SCALE));
  ochannel_id = bse_source_class_add_ochannel (source_class,
					       "Stereo Out", "Stereo Output",
					       2);
  g_assert (ochannel_id == BSE_SONG_OCHANNEL_STEREO);
}

static void
bse_song_init (BseSong *song)
{
  song->bpm = BSE_DFL_SONG_BPM;
  song->volume_factor = bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB);
  song->pattern_length = BSE_DFL_SONG_PATTERN_LENGTH;
  song->n_channels = BSE_DFL_SONG_N_CHANNELS;
  
  song->instruments = NULL;
  song->patterns = NULL;
  song->pattern_groups = NULL;
  
  song->n_pgroups = 0;
  song->pgroups = NULL;

  song->sequencer = NULL;
  song->sequencer_index = 0;

  song->pattern_list_length = 0;
  song->pattern_list = NULL;
}

static void
bse_song_do_shutdown (BseObject *object)
{
  BseSong *song;
  
  song = BSE_SONG (object);
  
  while (song->pattern_groups)
    bse_container_remove_item (BSE_CONTAINER (song), song->pattern_groups->data);
  song->n_pgroups = 0;
  g_free (song->pgroups);
  song->pgroups = NULL;
  while (song->patterns)
    bse_container_remove_item (BSE_CONTAINER (song), song->patterns->data);
  while (song->instruments)
    bse_container_remove_item (BSE_CONTAINER (song), song->instruments->data);
  
  /* chain parent class' shutdown handler */
  BSE_OBJECT_CLASS (parent_class)->shutdown (object);
}


static void
bse_song_set_param (BseSong  *song,
		    BseParam *param,
		    guint     param_id)
{
  switch (param_id)
    {
      GList *list;
      
    case PARAM_N_CHANNELS:
      /* we silently ignore this parameter during playing phase */
      if (!BSE_OBJECT_IS_LOCKED (song))
	{
	  song->n_channels = param->value.v_uint;
	  for (list = song->patterns; list; list = list->next)
	    bse_pattern_set_n_channels (list->data, song->n_channels);
	}
      break;
    case PARAM_PATTERN_LENGTH:
      /* we silently ignore this parameter during playing phase */
      if (!BSE_OBJECT_IS_LOCKED (song))
	{
	  song->pattern_length = param->value.v_uint;
	  for (list = song->patterns; list; list = list->next)
	    bse_pattern_set_n_rows (list->data, song->pattern_length);
	}
      break;
    case PARAM_VOLUME_f:
      song->volume_factor = param->value.v_float;
      if (song->sequencer)
	bse_song_sequencer_recalc (song);
      bse_object_param_changed (BSE_OBJECT (song), "volume_dB");
      bse_object_param_changed (BSE_OBJECT (song), "volume_perc");
      break;
    case PARAM_VOLUME_dB:
      song->volume_factor = bse_dB_to_factor (param->value.v_float);
      if (song->sequencer)
	bse_song_sequencer_recalc (song);
      bse_object_param_changed (BSE_OBJECT (song), "volume_f");
      bse_object_param_changed (BSE_OBJECT (song), "volume_perc");
      break;
    case PARAM_VOLUME_PERC:
      song->volume_factor = param->value.v_uint / 100.0;
      if (song->sequencer)
	bse_song_sequencer_recalc (song);
      bse_object_param_changed (BSE_OBJECT (song), "volume_f");
      bse_object_param_changed (BSE_OBJECT (song), "volume_dB");
      break;
    case PARAM_BPM:
      song->bpm = param->value.v_uint;
      if (song->sequencer)
	bse_song_sequencer_recalc (song);
      break;
      
    default:
      BSE_UNHANDLED_PARAM_ID (song, param, param_id);
      break;
    }
}

static void
bse_song_get_param (BseSong  *song,
		    BseParam *param,
		    guint     param_id)
{
  switch (param_id)
    {
    case PARAM_PATTERN_LENGTH:
      param->value.v_uint = song->pattern_length;
      break;
    case PARAM_N_CHANNELS:
      param->value.v_uint = song->n_channels;
      break;
    case PARAM_VOLUME_f:
      param->value.v_float = song->volume_factor;
      break;
    case PARAM_VOLUME_dB:
      param->value.v_float = bse_dB_from_factor (song->volume_factor, BSE_MIN_VOLUME_dB);
      break;
    case PARAM_VOLUME_PERC:
      param->value.v_uint = song->volume_factor * 100.0 + 0.5;
      break;
    case PARAM_BPM:
      param->value.v_uint = song->bpm;
      break;
    default:
      BSE_UNHANDLED_PARAM_ID (song, param, param_id);
      break;
    }
}

BseSong*
bse_song_new (BseProject *project,
	      guint	  n_channels)
{
  register BseSong *song;
  
  if (project)
    g_return_val_if_fail (BSE_IS_PROJECT (project), NULL);
  g_return_val_if_fail (n_channels >= 1 && n_channels <= BSE_MAX_N_CHANNELS, NULL);
  
  song = bse_object_new (BSE_TYPE_SONG,
			 "n-channels", n_channels,
			 NULL);
  if (project)
    bse_project_add_super (project, BSE_SUPER (song));
  
  return song;
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
  
  if (bse_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_INSTRUMENT))
    song->instruments = g_list_append (song->instruments, item);
  else if (bse_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_PATTERN))
    song->patterns = g_list_append (song->patterns, item);
  else if (bse_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_PATTERN_GROUP))
    song->pattern_groups = g_list_append (song->pattern_groups, item);
  else
    g_warning ("BseSong: cannot add unknown item type `%s'",
	       BSE_OBJECT_TYPE_NAME (item));
  
  /* chain parent class' add_item handler */
  BSE_CONTAINER_CLASS (parent_class)->add_item (container, item);
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
}

static void
bse_song_remove_item (BseContainer *container,
		      BseItem	   *item)
{
  BseSong *song;
  GList **list_p = NULL;
  
  song = BSE_SONG (container);
  
  if (bse_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_INSTRUMENT))
    list_p = &song->instruments;
  else if (bse_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_PATTERN))
    list_p = &song->patterns;
  else if (bse_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_PATTERN_GROUP))
    {
      bse_song_remove_pgroup_links (song, BSE_PATTERN_GROUP (item));
      list_p = &song->pattern_groups;
    }
  else
    g_warning ("BseSong: cannot remove unknown item type `%s'",
	       BSE_OBJECT_TYPE_NAME (item));

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

static guint
bse_song_item_seqid (BseContainer *container,
		     BseItem	  *item)
     
{
  BseSong *song = BSE_SONG (container);
  
  if (bse_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_INSTRUMENT))
    return 1 + g_list_index (song->instruments, item);
  else if (bse_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_PATTERN))
    return 1 + g_list_index (song->patterns, item);
  else if (bse_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_PATTERN_GROUP))
    return 1 + g_list_index (song->pattern_groups, item);
  else
    return 0;
}

static BseItem*
bse_song_get_item (BseContainer *container,
		   BseType	 item_type,
		   guint	 seqid)
{
  BseSong *song = BSE_SONG (container);
  GList *list;
  
  if (bse_type_is_a (item_type, BSE_TYPE_INSTRUMENT))
    list = g_list_nth (song->instruments, seqid - 1);
  else if (bse_type_is_a (item_type, BSE_TYPE_PATTERN))
    list = g_list_nth (song->patterns, seqid - 1);
  else if (bse_type_is_a (item_type, BSE_TYPE_PATTERN_GROUP))
    list = g_list_nth (song->pattern_groups, seqid - 1);
  else
    list = NULL;
  
  return list ? list->data : NULL;
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
	if (bse_string_equals (BSE_OBJECT_NAME (list->data), "Default"))
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

  n = song->n_pgroups++;
  song->pgroups = g_renew (BsePatternGroup*, song->pgroups, song->n_pgroups);
  g_memmove (song->pgroups + position + 1,
	     song->pgroups + position,
	     sizeof (BsePatternGroup*) * (n - position));
  song->pgroups[position] = pgroup;

  bse_object_ref (BSE_OBJECT (pgroup));
  BSE_NOTIFY (song, pattern_group_inserted, NOTIFY (OBJECT, pgroup, position, DATA));
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
      song->n_pgroups--;
      g_memmove (song->pgroups + position,
		 song->pgroups + position + 1,
		 sizeof (BsePatternGroup*) * (song->n_pgroups - position));
      bse_object_ref (BSE_OBJECT (pgroup));
      BSE_NOTIFY (song, pattern_group_removed, NOTIFY (OBJECT, pgroup, position, DATA));
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

  bse_object_ref (BSE_OBJECT (song));
  bse_object_ref (BSE_OBJECT (pgroup));

  for (slist = remove_positions; slist; slist = slist->next)
    BSE_NOTIFY (song, pattern_group_removed, NOTIFY (OBJECT, pgroup, GPOINTER_TO_UINT (slist->data), DATA));
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

  g_return_if_fail (BSE_IS_SONG (song));
  g_return_if_fail (BSE_IS_PATTERN_GROUP (src_pgroup));
  g_return_if_fail (BSE_ITEM (src_pgroup)->parent == BSE_ITEM (song));

  bse_object_ref (BSE_OBJECT (song));
  bse_object_ref (BSE_OBJECT (src_pgroup));

  item = bse_container_new_item (BSE_CONTAINER (song), BSE_TYPE_PATTERN_GROUP,
				 "name", BSE_OBJECT_NAME (src_pgroup),
				 "blurb", bse_object_get_blurb (BSE_OBJECT (src_pgroup)),
				 NULL);
  pgroup = BSE_PATTERN_GROUP (item);

  bse_object_ref (BSE_OBJECT (pgroup));

  bse_song_insert_pattern_group_link (song, pgroup, position);

  bse_pattern_group_copy_contents (pgroup, src_pgroup);
  
  bse_object_unref (BSE_OBJECT (pgroup));
  bse_object_unref (BSE_OBJECT (src_pgroup));
  bse_object_unref (BSE_OBJECT (song));
}






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
bse_song_prepare (BseSource *source,
		  BseIndex   index)
{
  BseSong *song = BSE_SONG (source);

  bse_object_lock (BSE_OBJECT (song));
  
  song->sequencer_index = index;
  bse_song_sequencer_setup (song, 2);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source, index);

  /* FIXME: odevice hack */
  bse_heart_source_add_odevice (source, bse_heart_get_device (bse_heart_get_default_odevice ()));
}

void
bse_song_update_sequencer (BseSong *song)
{
  g_return_if_fail (BSE_IS_SONG (song));

  if (song->sequencer)
    {
      BseSource *source = BSE_SOURCE (song);

      if (song->sequencer_index < source->index)
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

static void
bse_song_reset (BseSource *source)
{
  BseSong *song = BSE_SONG (source);

  song->sequencer_index = 0;
  bse_song_sequencer_destroy (song);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);

  bse_object_unlock (BSE_OBJECT (song));
}
