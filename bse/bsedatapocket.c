/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2002 Tim Janik
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
#include "bsedatapocket.h"

#include "bsemain.h"
#include "bsemarshal.h"



/* --- structures --- */
typedef struct _Notify Notify;
struct _Notify
{
  Notify        *next;
  BseDataPocket *pocket;
  guint          entry_id;
};


/* --- prototypes --- */
static void	   bse_data_pocket_init		(BseDataPocket		*pocket);
static void	   bse_data_pocket_class_init	(BseDataPocketClass	*class);
static void	   bse_data_pocket_destroy	(BseObject		*object);
static void	   bse_data_pocket_finalize	(GObject		*object);


/* --- variables --- */
static gpointer parent_class = NULL;
static guint	signal_entry_added = 0;
static guint	signal_entry_removed = 0;
static guint	signal_entry_changed = 0;
static Notify  *changed_notify_list = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseDataPocket)
{
  static const GTypeInfo data_pocket_info = {
    sizeof (BseDataPocketClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_data_pocket_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseDataPocket),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_data_pocket_init,
  };
  
  return bse_type_register_static (BSE_TYPE_ITEM,
				   "BseDataPocket",
				   "Data pocket type",
				   &data_pocket_info);
}

static void
bse_data_pocket_class_init (BseDataPocketClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->finalize = bse_data_pocket_finalize;
  
  object_class->destroy = bse_data_pocket_destroy;
  
  signal_entry_added = bse_object_class_add_signal (object_class, "entry-added",
						    bse_marshal_VOID__UINT,
						    G_TYPE_NONE, 1, G_TYPE_UINT);
  signal_entry_removed = bse_object_class_add_signal (object_class, "entry-removed",
						      bse_marshal_VOID__UINT,
						      G_TYPE_NONE, 1, G_TYPE_UINT);
  signal_entry_changed = bse_object_class_add_signal (object_class, "entry-changed",
						      bse_marshal_VOID__UINT,
						      G_TYPE_NONE, 1, G_TYPE_UINT);
}

static void
bse_data_pocket_init (BseDataPocket *pocket)
{
  pocket->free_id = 1;
  pocket->n_entries = 0;
  pocket->entries = NULL;
  pocket->need_store = 0;
  pocket->cr_items = NULL;
  BSE_OBJECT_SET_FLAGS (pocket, BSE_ITEM_FLAG_STORAGE_IGNORE);
}

static void
bse_data_pocket_destroy (BseObject *object)
{
  BseDataPocket *pocket = BSE_DATA_POCKET (object);

  pocket->in_destroy = TRUE;

  while (pocket->n_entries)
    _bse_data_pocket_delete_entry (pocket, pocket->entries[0].id);

  /* chain parent class' handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);

  g_return_if_fail (pocket->cr_items == NULL);
}

static void
bse_data_pocket_finalize (GObject *object)
{
  BseDataPocket *pocket = BSE_DATA_POCKET (object);
  Notify *notify, *last = NULL;

  while (pocket->n_entries)
    _bse_data_pocket_delete_entry (pocket, pocket->entries[0].id);

  for (notify = changed_notify_list; notify; )
    {
      if (notify->pocket == pocket)
	{
	  Notify *tmp;

	  if (last)
	    last->next = notify->next;
	  else
	    changed_notify_list = notify->next;
	  tmp = notify;
	  notify = notify->next;
	  g_free (tmp);
	}
      else
	{
	  last = notify;
	  notify = last->next;
	}
    }

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);

  g_return_if_fail (pocket->cr_items == NULL);
}

static gboolean
changed_notify_handler (gpointer data)
{
  BSE_THREADS_ENTER ();

  while (changed_notify_list)
    {
      Notify *notify = changed_notify_list;

      changed_notify_list = notify->next;
      if (!notify->pocket->in_destroy)
	g_signal_emit (notify->pocket, signal_entry_changed, 0, notify->entry_id);
      g_free (notify);
    }

  BSE_THREADS_LEAVE ();

  return FALSE;
}

static void
notify_add (BseDataPocket *pocket,
	    guint          entry_id)
{
  Notify *notify;

  if (!changed_notify_list)
    g_idle_add_full (BSE_NOTIFY_PRIORITY, changed_notify_handler, NULL, NULL);
  for (notify = changed_notify_list; notify; notify = notify->next)
    if (notify->pocket == pocket && notify->entry_id == entry_id)
      return;
  notify = g_new (Notify, 1);
  notify->pocket = pocket;
  notify->entry_id = entry_id;
  notify->next = changed_notify_list;
  changed_notify_list = notify;
}

static void
pocket_uncross (BseItem *pitem,
		BseItem *item)
{
  BseDataPocket *pocket = BSE_DATA_POCKET (pitem);
  guint i;
  
  for (i = 0; i < pocket->n_entries; i++)
    {
      BseDataPocketEntry *entry = pocket->entries + i;
      guint n, have_this_id = 0;
      
      for (n = 0; n < entry->n_items; n++)
	if (entry->items[n].type == BSE_DATA_POCKET_OBJECT &&
	    entry->items[n].value.v_object == item)
	  {
	    if (!have_this_id++)
	      notify_add (pocket, entry->id);
	    entry->items[n].value.v_object = NULL;
	  }
    }
  
  g_object_ref (pocket);
  pocket->cr_items = g_slist_remove (pocket->cr_items, item);
  g_object_unref (pocket);
}

static void
add_cross_ref (BseDataPocket *pocket,
	       BseItem       *item)
{
  g_return_if_fail (BSE_IS_ITEM (item));
  g_return_if_fail (bse_item_common_ancestor (BSE_ITEM (pocket), item) != NULL); // FIXME: delete

  if (!g_slist_find (pocket->cr_items, item))
    {
      bse_item_cross_ref (BSE_ITEM (pocket), item, pocket_uncross);
      pocket->cr_items = g_slist_prepend (pocket->cr_items, item);
    }
}

static void
remove_cross_ref (BseDataPocket *pocket,
		  BseItem       *item)
{
  guint i;

  g_return_if_fail (BSE_IS_ITEM (item));
  g_return_if_fail (bse_item_common_ancestor (BSE_ITEM (pocket), item) != NULL); // FIXME: delete
  g_return_if_fail (g_slist_find (pocket->cr_items, item) != NULL);

  for (i = 0; i < pocket->n_entries; i++)
    {
      BseDataPocketEntry *entry = pocket->entries + i;
      guint n;

      for (n = 0; n < entry->n_items; n++)
	if (entry->items[n].type == BSE_DATA_POCKET_OBJECT &&
	    entry->items[n].value.v_object == item)
	  return;
    }

  pocket->cr_items = g_slist_remove (pocket->cr_items, item);
  bse_item_cross_unref (BSE_ITEM (pocket), item);
}

guint
_bse_data_pocket_create_entry (BseDataPocket *pocket)
{
  guint id, i;

  g_return_val_if_fail (BSE_IS_DATA_POCKET (pocket), 0);

  id = pocket->free_id++;
  g_assert (id != 0);

  i = pocket->n_entries++;
  pocket->entries = g_renew (BseDataPocketEntry, pocket->entries, pocket->n_entries);
  pocket->entries[i].id = id;
  pocket->entries[i].n_items = 0;
  pocket->entries[i].items = NULL;

  g_signal_emit (pocket, signal_entry_added, 0, id);

  return id;
}

gboolean
_bse_data_pocket_delete_entry (BseDataPocket *pocket,
			       guint          entry_id)
{
  BseDataPocketEntry *entry;
  GSList *cr_del = NULL;
  guint i, n;

  g_return_val_if_fail (BSE_IS_DATA_POCKET (pocket), FALSE);
  g_return_val_if_fail (entry_id > 0, FALSE);

  for (i = 0; i < pocket->n_entries; i++)
    if (pocket->entries[i].id == entry_id)
      break;
  if (i >= pocket->n_entries)
    return FALSE;

  entry = pocket->entries + i;
  for (n = 0; n < entry->n_items; n++)
    {
      if (entry->items[n].type == BSE_DATA_POCKET_STRING)
	g_free (entry->items[n].value.v_string);
      else if (entry->items[n].type == BSE_DATA_POCKET_OBJECT &&
	       entry->items[n].value.v_object &&
	       !g_slist_find (cr_del, entry->items[n].value.v_object))
	cr_del = g_slist_prepend (cr_del, entry->items[n].value.v_object);
    }
  g_free (entry->items);

  pocket->need_store -= entry->n_items;
  n = entry->id;

  pocket->n_entries--;
  if (i < pocket->n_entries)
    pocket->entries[i] = pocket->entries[pocket->n_entries];

  if (pocket->need_store)
    BSE_OBJECT_UNSET_FLAGS (pocket, BSE_ITEM_FLAG_STORAGE_IGNORE);
  else
    BSE_OBJECT_SET_FLAGS (pocket, BSE_ITEM_FLAG_STORAGE_IGNORE);

  while (cr_del)
    {
      GSList *tmp = cr_del;

      cr_del = tmp->next;
      remove_cross_ref (pocket, tmp->data);
      g_slist_free_1 (tmp);
    }

  if (!pocket->in_destroy)
    g_signal_emit (pocket, signal_entry_removed, 0, n);

  return TRUE;
}

gboolean
_bse_data_pocket_entry_set (BseDataPocket     *pocket,
			    guint              id,
			    GQuark             data_quark,
			    gchar              type,
			    BseDataPocketValue value)
{
  BseDataPocketEntry *entry;
  guint i, n;
  
  g_return_val_if_fail (BSE_IS_DATA_POCKET (pocket), FALSE);
  g_return_val_if_fail (id > 0, FALSE);
  g_return_val_if_fail (data_quark > 0, FALSE);
  if (type == BSE_DATA_POCKET_OBJECT && value.v_object)
    g_return_val_if_fail (BSE_IS_ITEM (value.v_object), FALSE);

  for (i = 0; i < pocket->n_entries; i++)
    if (pocket->entries[i].id == id)
      break;
  if (i >= pocket->n_entries)
    return FALSE;

  entry = pocket->entries + i;
  for (n = 0; n < entry->n_items; n++)
    if (entry->items[n].quark == data_quark)
      break;
  if (n >= entry->n_items)
    {
      n = entry->n_items++;
      entry->items = g_realloc (entry->items, sizeof (entry->items[0]) * entry->n_items);
      entry->items[n].type = 0;
      entry->items[n].quark = data_quark;
      pocket->need_store++;
    }

  if (entry->items[n].type == BSE_DATA_POCKET_STRING)
    g_free (entry->items[n].value.v_string);
  else if (entry->items[n].type == BSE_DATA_POCKET_OBJECT)
    {
      entry->items[n].type = 0;
      remove_cross_ref (pocket, value.v_object);
    }

  /* deletion */
  if ((type == BSE_DATA_POCKET_INT && value.v_int == 0) ||
      (type == BSE_DATA_POCKET_INT64 && value.v_int64 == 0) ||
      (type == BSE_DATA_POCKET_FLOAT && value.v_float == 0.0) ||
      (type == BSE_DATA_POCKET_STRING && value.v_string == NULL) ||
      (type == BSE_DATA_POCKET_OBJECT && value.v_object == NULL))
    {
      entry->n_items--;
      if (n < entry->n_items)
	entry->items[n] = entry->items[entry->n_items];
      pocket->need_store--;
    }
  else
    {
      entry->items[n].type = type;
      entry->items[n].value = value;
      if (type == BSE_DATA_POCKET_STRING)
	entry->items[n].value.v_string = g_strdup (value.v_string);
      else if (type == BSE_DATA_POCKET_OBJECT)
	add_cross_ref (pocket, value.v_object);
    }

  if (pocket->need_store)
    BSE_OBJECT_UNSET_FLAGS (pocket, BSE_ITEM_FLAG_STORAGE_IGNORE);
  else
    BSE_OBJECT_SET_FLAGS (pocket, BSE_ITEM_FLAG_STORAGE_IGNORE);

  notify_add (pocket, entry->id);

  return TRUE;
}

gchar
_bse_data_pocket_entry_get (BseDataPocket      *pocket,
			    guint               id,
			    GQuark              data_quark,
			    BseDataPocketValue *value)
{
  BseDataPocketEntry *entry;
  guint i, n;
  
  g_return_val_if_fail (BSE_IS_DATA_POCKET (pocket), 0);

  if (!data_quark)
    return 0;

  for (i = 0; i < pocket->n_entries; i++)
    if (pocket->entries[i].id == id)
      break;
  if (i >= pocket->n_entries)
    return 0;

  entry = pocket->entries + i;

  for (n = 0; n < entry->n_items; n++)
    if (entry->items[n].quark == data_quark)
      break;
  if (n >= entry->n_items)
    return 0;

  *value = entry->items[n].value;

  return entry->items[n].type;
}
