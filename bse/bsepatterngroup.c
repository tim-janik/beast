/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2000 Olaf Hoehmann and Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include        "bsepatterngroup.h"

#include        "bseglobals.h"
#include        "bsecontainer.h"
#include        "bsestorage.h"
#include        "bsemarshal.h"
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
static void	    bse_pattern_group_class_init	(BsePatternGroupClass	*class);
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
				   &pattern_group_info);
}

static void
bse_pattern_group_class_init (BsePatternGroupClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = (GObjectSetPropertyFunc) bse_pattern_group_set_property;
  gobject_class->get_property = (GObjectGetPropertyFunc) bse_pattern_group_get_property;

  object_class->store_private = bse_pattern_group_store_private;
  object_class->restore_private = bse_pattern_group_restore_private;
  object_class->destroy = bse_pattern_group_destroy;

  pattern_group_signals[SIGNAL_PATTERN_INSERTED] = bse_object_class_add_signal (object_class, "pattern-inserted",
										bse_marshal_VOID__OBJECT_UINT,
										G_TYPE_NONE,
										2, BSE_TYPE_PATTERN, G_TYPE_UINT);
  pattern_group_signals[SIGNAL_PATTERN_REMOVED] = bse_object_class_add_signal (object_class, "pattern-removed",
										bse_marshal_VOID__OBJECT_UINT,
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
  
  /* chain parent class' destroy handler */
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

  bse_object_ref (BSE_OBJECT (pattern));
  g_signal_emit (pgroup, pattern_group_signals[SIGNAL_PATTERN_INSERTED], 0, pattern, position);
  bse_object_unref (BSE_OBJECT (pattern));
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
      
      bse_object_ref (BSE_OBJECT (pattern));
      g_signal_emit (pgroup, pattern_group_signals[SIGNAL_PATTERN_REMOVED], 0, pattern, position);
      bse_object_unref (BSE_OBJECT (pattern));
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
  
  bse_object_ref (BSE_OBJECT (pgroup));
  bse_object_ref (BSE_OBJECT (pattern));

  for (slist = remove_positions; slist; slist = slist->next)
    g_signal_emit (pgroup, pattern_group_signals[SIGNAL_PATTERN_REMOVED], 0, pattern, GPOINTER_TO_UINT (slist->data));
  g_slist_free (remove_positions);

  bse_object_unref (BSE_OBJECT (pattern));
  bse_object_unref (BSE_OBJECT (pgroup));
}

void
bse_pattern_group_clone_contents (BsePatternGroup *pgroup,
				  BsePatternGroup *src_pgroup)
{
  guint i;

  g_return_if_fail (BSE_IS_PATTERN_GROUP (pgroup));
  g_return_if_fail (BSE_IS_PATTERN_GROUP (src_pgroup));
  g_return_if_fail (BSE_ITEM (pgroup)->parent == BSE_ITEM (src_pgroup)->parent);

  bse_object_ref (BSE_OBJECT (pgroup));
  bse_object_ref (BSE_OBJECT (src_pgroup));

  while (pgroup->n_entries)
    bse_pattern_group_remove_entry (pgroup, 0);

  for (i = 0; i < src_pgroup->n_entries; i++)
    bse_pattern_group_insert_pattern (pgroup, src_pgroup->entries[i].pattern, i);
  
  bse_object_unref (BSE_OBJECT (pgroup));
  bse_object_unref (BSE_OBJECT (src_pgroup));
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
  BseProject *project = bse_item_get_project (BSE_ITEM (pgroup));
  guint i;

  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->store_private)
    BSE_OBJECT_CLASS (parent_class)->store_private (object, storage);

  for (i = 0; i < pgroup->n_entries; i++)
    {
      gchar *path = bse_container_make_item_path (BSE_CONTAINER (project),
						  BSE_ITEM (pgroup->entries[i].pattern),
						  FALSE);

      bse_storage_break (storage);

      bse_storage_putc (storage, '(');
      bse_storage_puts (storage, "add-pattern");
      bse_storage_printf (storage, " %s", path);
      bse_storage_handle_break (storage);
      bse_storage_putc (storage, ')');
      g_free (path);
    }
}

static BseTokenType
bse_pattern_group_restore_private (BseObject  *object,
				   BseStorage *storage)
{
  BsePatternGroup *pgroup = BSE_PATTERN_GROUP (object);
  BseProject *project = bse_item_get_project (BSE_ITEM (pgroup));
  GScanner *scanner = storage->scanner;
  GTokenType expected_token;
  gchar *pattern_path;
  BseItem *item;

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

  /* parse pgroup path */
  if (g_scanner_get_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return G_TOKEN_IDENTIFIER;
  pattern_path = g_strdup (scanner->value.v_identifier);

  /* ok, resolve and add pgroup */
  item = bse_container_item_from_path (BSE_CONTAINER (project), pattern_path);
  if (!item || !BSE_IS_PATTERN (item))
    bse_storage_warn (storage,
		      "%s: unable to determine pattern from \"%s\"",
		      BSE_OBJECT_NAME (pgroup),
		      pattern_path);
  else
    bse_pattern_group_insert_pattern (pgroup, BSE_PATTERN (item), pgroup->n_entries);
  g_free (pattern_path);

  /* read closing brace */
  return g_scanner_get_next_token (scanner) == ')' ? G_TOKEN_NONE : ')';
}
