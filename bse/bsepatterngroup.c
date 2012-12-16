/* BSE - Better Sound Engine
 * Copyright (C) 2000-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include        "bsepatterngroup.h"

#include        "bseglobals.h"
#include        "bsecontainer.h"
#include        "bsestorage.h"
#include        "bsemain.h"
#include        <string.h>


enum
{
  PARAM_0,
};

enum {
  SIGNAL_PATTERN_INSERTED,
  SIGNAL_PATTERN_REMOVED,
  SIGNAL_LAST
};


/* --- prototypes --- */
static void	    bse_pattern_group_class_init	(BsePatternGroupClass	*klass);
static void	    bse_pattern_group_init		(BsePatternGroup	*pattern_group);
static void	    bse_pattern_group_destroy		(BseObject		*object);
static void         bse_pattern_group_set_property 	(BsePatternGroup	*pattern_group,
							 guint                   param_id,
							 GValue                 *value,
							 GParamSpec             *pspec,
							 const gchar            *trailer);
static void         bse_pattern_group_get_property 	(BsePatternGroup        *pattern_group,
							 guint                   param_id,
							 GValue                 *value,
							 GParamSpec             *pspec,
							 const gchar            *trailer);
static void	    bse_pattern_group_store_private	(BseObject              *object,
							 BseStorage             *storage);
static BseTokenType bse_pattern_group_restore_private	(BseObject         	*object,
							 BseStorage        	*storage);



/* --- variables --- */
static GTypeClass *parent_class = NULL;
static guint       pattern_group_signals[SIGNAL_LAST] = { 0, };


/* --- functions --- */
BSE_BUILTIN_TYPE (BsePatternGroup)
{
  static const GTypeInfo pattern_group_info = {
    sizeof (BsePatternGroupClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_pattern_group_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_group */,
    
    sizeof (BsePatternGroup),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_pattern_group_init,
  };
  
  return bse_type_register_static (BSE_TYPE_ITEM,
				   "BsePatternGroup",
				   "BSE patternary group container",
                                   __FILE__, __LINE__,
                                   &pattern_group_info);
}

static void
bse_pattern_group_class_init (BsePatternGroupClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);
  
  gobject_class->set_property = bse_pattern_group_set_property;
  gobject_class->get_property = bse_pattern_group_get_property;
  
  object_class->store_private = bse_pattern_group_store_private;
  object_class->restore_private = bse_pattern_group_restore_private;
  object_class->destroy = bse_pattern_group_destroy;
  
  pattern_group_signals[SIGNAL_PATTERN_INSERTED] = bse_object_class_add_signal (object_class, "pattern-inserted",
										G_TYPE_NONE,
										2, BSE_TYPE_PATTERN, G_TYPE_UINT);
  pattern_group_signals[SIGNAL_PATTERN_REMOVED] = bse_object_class_add_signal (object_class, "pattern-removed",
									       G_TYPE_NONE,
									       2, BSE_TYPE_PATTERN, G_TYPE_UINT);
}

static void
bse_pattern_group_init (BsePatternGroup *pgroup)
{
  pgroup->pattern_count = 0;
  pgroup->n_entries = 0;
  pgroup->entries = NULL;
}

static void
bse_pattern_group_destroy (BseObject *object)
{
  BsePatternGroup *pgroup = BSE_PATTERN_GROUP (object);
  
  pgroup->pattern_count = 0;
  pgroup->n_entries = 0;
  g_free (pgroup->entries);
  pgroup->entries = NULL;
  
  /* chain parent class' handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_pattern_group_set_property (BsePatternGroup *pgroup,
				guint            param_id,
				GValue          *value,
				GParamSpec      *pspec,
				const gchar     *trailer)
{
  switch (param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (pgroup, param_id, pspec);
      break;
    }
}

static void
bse_pattern_group_get_property (BsePatternGroup *pgroup,
				guint            param_id,
				GValue          *value,
				GParamSpec      *pspec,
				const gchar     *trailer)
{
  switch (param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (pgroup, param_id, pspec);
      break;
    }
}

static void
update_pattern_count (BsePatternGroup *pgroup)
{
  pgroup->pattern_count = pgroup->n_entries;
}

void
bse_pattern_group_insert_pattern (BsePatternGroup *pgroup,
				  BsePattern      *pattern,
				  gint             position)
{
  guint n;
  
  g_return_if_fail (BSE_IS_PATTERN_GROUP (pgroup));
  g_return_if_fail (BSE_IS_PATTERN (pattern));
  
  if (position < 0 || position > pgroup->n_entries)
    position = pgroup->n_entries;
  
  BSE_SEQUENCER_LOCK ();
  n = pgroup->n_entries++;
  pgroup->entries = g_renew (BsePatternGroupEntry, pgroup->entries, pgroup->n_entries);
  g_memmove (pgroup->entries + position + 1,
	     pgroup->entries + position,
	     sizeof (BsePatternGroupEntry) * (n - position));
  pgroup->entries[position].pattern = pattern;
  update_pattern_count (pgroup);
  BSE_SEQUENCER_UNLOCK ();
  
  g_object_ref (pattern);
  g_signal_emit (pgroup, pattern_group_signals[SIGNAL_PATTERN_INSERTED], 0, pattern, position);
  g_object_unref (pattern);
}

void
bse_pattern_group_remove_entry (BsePatternGroup *pgroup,
				gint             position)
{
  g_return_if_fail (BSE_IS_PATTERN_GROUP (pgroup));
  
  if (position < 0)
    position = pgroup->n_entries - 1;
  if (position < pgroup->n_entries)
    {
      BsePattern *pattern = pgroup->entries[position].pattern;
      
      BSE_SEQUENCER_LOCK ();
      pgroup->n_entries--;
      g_memmove (pgroup->entries + position,
		 pgroup->entries + position + 1,
		 sizeof (BsePatternGroupEntry) * (pgroup->n_entries - position));
      update_pattern_count (pgroup);
      BSE_SEQUENCER_UNLOCK ();
      
      g_object_ref (pattern);
      g_signal_emit (pgroup, pattern_group_signals[SIGNAL_PATTERN_REMOVED], 0, pattern, position);
      g_object_unref (pattern);
    }
}

void
bse_pattern_group_remove_pattern (BsePatternGroup *pgroup,
				  BsePattern      *pattern)
{
  BsePatternGroupEntry *last, *cur, *bound;
  GSList *slist, *remove_positions = NULL;
  
  g_return_if_fail (BSE_IS_PATTERN_GROUP (pgroup));
  g_return_if_fail (BSE_IS_PATTERN (pattern));
  
  cur = pgroup->entries;
  last = cur;
  bound = cur + pgroup->n_entries;
  BSE_SEQUENCER_LOCK ();
  while (cur < bound)
    {
      if (cur->pattern != pattern)
	{
	  if (last != cur)
	    {
	      last->pattern = cur->pattern;
	    }
	  last++;
	}
      else
	remove_positions = g_slist_prepend (remove_positions, GUINT_TO_POINTER (cur - pgroup->entries));
      cur++;
    }
  pgroup->n_entries = last - pgroup->entries;
  update_pattern_count (pgroup);
  BSE_SEQUENCER_UNLOCK ();
  
  g_object_ref (pgroup);
  g_object_ref (pattern);
  
  for (slist = remove_positions; slist; slist = slist->next)
    g_signal_emit (pgroup, pattern_group_signals[SIGNAL_PATTERN_REMOVED], 0, pattern, GPOINTER_TO_UINT (slist->data));
  g_slist_free (remove_positions);
  
  g_object_unref (pattern);
  g_object_unref (pgroup);
}

void
bse_pattern_group_clone_contents (BsePatternGroup *pgroup,
				  BsePatternGroup *src_pgroup)
{
  guint i;
  
  g_return_if_fail (BSE_IS_PATTERN_GROUP (pgroup));
  g_return_if_fail (BSE_IS_PATTERN_GROUP (src_pgroup));
  g_return_if_fail (BSE_ITEM (pgroup)->parent == BSE_ITEM (src_pgroup)->parent);
  
  g_object_ref (pgroup);
  g_object_ref (src_pgroup);
  
  while (pgroup->n_entries)
    bse_pattern_group_remove_entry (pgroup, 0);
  
  for (i = 0; i < src_pgroup->n_entries; i++)
    bse_pattern_group_insert_pattern (pgroup, src_pgroup->entries[i].pattern, i);
  
  g_object_unref (pgroup);
  g_object_unref (src_pgroup);
}

BsePattern*
bse_pattern_group_get_nth_pattern (BsePatternGroup *pgroup,
				   gint             index)
{
  g_return_val_if_fail (BSE_IS_PATTERN_GROUP (pgroup), NULL);
  g_return_val_if_fail (index < pgroup->pattern_count, NULL);
  
  g_return_val_if_fail (pgroup->pattern_count == pgroup->n_entries, NULL); /* current implementation */
  
  return pgroup->entries[index].pattern;
}

static void
bse_pattern_group_store_private (BseObject  *object,
				 BseStorage *storage)
{
  BsePatternGroup *pgroup = BSE_PATTERN_GROUP (object);
  guint i;
  
  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->store_private)
    BSE_OBJECT_CLASS (parent_class)->store_private (object, storage);
  
  for (i = 0; i < pgroup->n_entries; i++)
    {
      bse_storage_break (storage);
      bse_storage_putc (storage, '(');
      bse_storage_puts (storage, "add-pattern ");
      bse_storage_put_item_link (storage, BSE_ITEM (pgroup), BSE_ITEM (pgroup->entries[i].pattern));
      bse_storage_handle_break (storage);
      bse_storage_putc (storage, ')');
    }
}

static void
parser_add_pattern (gpointer     data,
		    BseStorage  *storage,
		    BseItem     *from_item,
		    BseItem     *to_item,
		    const gchar *error)
{
  BsePatternGroup *pgroup = BSE_PATTERN_GROUP (from_item);
  
  if (error)
    bse_storage_warn (storage, error);
  else if (BSE_IS_PATTERN (to_item))
    {
      bse_pattern_group_insert_pattern (pgroup, BSE_PATTERN (to_item), pgroup->n_entries);
    }
}

static BseTokenType
bse_pattern_group_restore_private (BseObject  *object,
				   BseStorage *storage)
{
  BsePatternGroup *pgroup = BSE_PATTERN_GROUP (object);
  GScanner *scanner = storage->scanner;
  GTokenType expected_token;
  gchar *pattern_path;
  
  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->restore_private)
    expected_token = BSE_OBJECT_CLASS (parent_class)->restore_private (object, storage);
  else
    expected_token = BSE_TOKEN_UNMATCHED;
  
  if (expected_token != BSE_TOKEN_UNMATCHED ||
      g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER ||
      !bse_string_equals ("add-pattern", scanner->next_value.v_identifier))
    return expected_token;
  
  /* eat "add-pattern" */
  g_scanner_get_next_token (scanner);
  
  if (g_scanner_get_next_token (scanner) != G_TOKEN_STRING)
    return G_TOKEN_STRING;
  pattern_path = g_strdup (scanner->value.v_string);
  
  /* queue resolving object link */
  expected_token = bse_storage_parse_item_link (storage, BSE_ITEM (pgroup), parser_add_pattern, NULL);
  if (expected_token != G_TOKEN_NONE)
    return expected_token;
  
  /* read closing brace */
  return g_scanner_get_next_token (scanner) == ')' ? G_TOKEN_NONE : ')';
}
