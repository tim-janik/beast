/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
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
#include	"bsecontainer.h"

#include	"bsesource.h"
#include	"bsestorage.h"
#include	<stdlib.h>
#include	<string.h>



/* --- prototypes --- */
static void	    bse_container_class_init		(BseContainerClass	*class);
static void	    bse_container_init			(BseContainer		*container);
static void	    bse_container_destroy		(BseObject		*object);
static void	    bse_container_store_termination	(BseObject		*object,
							 BseStorage		*storage);
static BseTokenType bse_container_try_statement		(BseObject		*object,
							 BseStorage		*storage);
static void	    bse_container_do_add_item		(BseContainer		*container,
							 BseItem		*item);
static void	    bse_container_do_remove_item	(BseContainer		*container,
							 BseItem		*item);


/* --- variables --- */
static BseTypeClass	*parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseContainer)
{
  static const BseTypeInfo container_info = {
    sizeof (BseContainerClass),

    (BseClassInitBaseFunc) NULL,
    (BseClassDestroyBaseFunc) NULL,
    (BseClassInitFunc) bse_container_class_init,
    (BseClassDestroyFunc) NULL,
    NULL /* class_data */,

    sizeof (BseContainer),
    0 /* n_preallocs */,
    (BseObjectInitFunc) bse_container_init,
  };
  
  return bse_type_register_static (BSE_TYPE_SOURCE,
				   "BseContainer",
				   "Base type to manage BSE items",
				   &container_info);
}

static void
bse_container_class_init (BseContainerClass *class)
{
  BseObjectClass *object_class;
  BseItemClass *item_class;
  
  parent_class = bse_type_class_peek (BSE_TYPE_SOURCE);
  object_class = BSE_OBJECT_CLASS (class);
  item_class = BSE_ITEM_CLASS (class);
  
  object_class->store_termination = bse_container_store_termination;
  object_class->try_statement = bse_container_try_statement;
  object_class->destroy = bse_container_destroy;
  
  class->add_item = bse_container_do_add_item;
  class->remove_item = bse_container_do_remove_item;
  class->forall_items = NULL;
  class->item_seqid = NULL;
  class->get_item = NULL;
}

static void
bse_container_init (BseContainer *container)
{
  container->n_items = 0;
}

static void
bse_container_destroy (BseObject *object)
{
  BseContainer *container;
  
  container = BSE_CONTAINER (object);

  if (container->n_items)
    g_warning ("%s: destroy handlers missed to remove %u items",
	       BSE_OBJECT_TYPE_NAME (container),
	       container->n_items);
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_container_do_add_item (BseContainer *container,
			   BseItem	*item)
{
  bse_object_ref (BSE_OBJECT (item));
  container->n_items += 1;
  bse_item_set_container (item, BSE_ITEM (container));
}

void
bse_container_add_item (BseContainer *container,
			BseItem      *item)
{
  g_return_if_fail (BSE_IS_CONTAINER (container));
  g_return_if_fail (BSE_IS_ITEM (item));
  g_return_if_fail (item->container == NULL);
  g_return_if_fail (BSE_CONTAINER_GET_CLASS (container)->add_item != NULL); /* paranoid */

  bse_object_ref (BSE_OBJECT (container));
  bse_object_ref (BSE_OBJECT (item));

  /* ensure that item names per container are unique
   */
  if (!BSE_OBJECT_NAME (item) || bse_container_lookup_item (container, BSE_OBJECT_NAME (item)))
    {
      gchar *name = BSE_OBJECT_NAME (item);
      gchar *buffer, *p;
      guint i = 0, l;
      
      if (!name)
	name = BSE_OBJECT_TYPE_NAME (item);
      
      l = strlen (name);
      buffer = g_new (gchar, l + 12);
      strcpy (buffer, name);
      p = buffer + l;
      do
	g_snprintf (p, 11, "-%u", ++i);
      while (bse_container_lookup_item (container, buffer));
      
      bse_object_set_name (BSE_OBJECT (item), buffer);
      g_free (buffer);
    }

  BSE_CONTAINER_GET_CLASS (container)->add_item (container, item);
  BSE_NOTIFY (container, item_added, NOTIFY (OBJECT, item, DATA));

  bse_object_unref (BSE_OBJECT (item));
  bse_object_unref (BSE_OBJECT (container));
}

BseItem*
bse_container_new_item (BseContainer *container,
			BseType       item_type,
			const gchar  *first_param_name,
			...)
{
  BseItem *item;
  va_list var_args;

  g_return_val_if_fail (BSE_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (bse_type_is_a (item_type, BSE_TYPE_ITEM), NULL);

  va_start (var_args, first_param_name);
  item = bse_object_new_valist (item_type, first_param_name, var_args);
  va_end (var_args);
  bse_container_add_item (container, item);
  bse_object_unref (BSE_OBJECT (item));

  return item;
}

static void
bse_container_do_remove_item (BseContainer *container,
			      BseItem	   *item)
{
  container->n_items -= 1;
  bse_item_set_container (item, NULL);
  bse_object_unref (BSE_OBJECT (item));
}

void
bse_container_remove_item (BseContainer *container,
			   BseItem      *item)
{
  g_return_if_fail (BSE_IS_CONTAINER (container));
  g_return_if_fail (BSE_IS_ITEM (item));
  g_return_if_fail (item->container == BSE_ITEM (container));
  g_return_if_fail (BSE_CONTAINER_GET_CLASS (container)->remove_item != NULL); /* paranoid */

  bse_object_ref (BSE_OBJECT (container));
  bse_object_ref (BSE_OBJECT (item));

  BSE_CONTAINER_GET_CLASS (container)->remove_item (container, item);
  BSE_NOTIFY (container, item_removed, NOTIFY (OBJECT, item, DATA));

  bse_object_unref (BSE_OBJECT (item));
  bse_object_unref (BSE_OBJECT (container));
}

void
bse_container_forall_items (BseContainer      *container,
			    BseForallItemsFunc func,
			    gpointer           data)
{
  g_return_if_fail (BSE_IS_CONTAINER (container));
  g_return_if_fail (func != NULL);

  if (container->n_items && BSE_CONTAINER_GET_CLASS (container)->forall_items)
    BSE_CONTAINER_GET_CLASS (container)->forall_items (container, func, data);
}

static gboolean
list_items (BseItem *item,
	    gpointer data)
{
  GList **list_p = data;

  *list_p = g_list_prepend (*list_p, item);

  return TRUE;
}

GList*
bse_container_list_items (BseContainer *container)
{
  g_return_val_if_fail (BSE_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (BSE_CONTAINER_GET_CLASS (container)->forall_items != NULL, NULL); /* paranoid */

  if (container->n_items)
    {
      GList *list = NULL;

      BSE_CONTAINER_GET_CLASS (container)->forall_items (container, list_items, &list);

      return list;
    }
  else
    return NULL;
}

guint
bse_container_get_item_seqid (BseContainer *container,
			      BseItem      *item)
{
  g_return_val_if_fail (BSE_IS_CONTAINER (container), 0);
  g_return_val_if_fail (BSE_IS_ITEM (item), 0);
  g_return_val_if_fail (item->container == BSE_ITEM (container), 0);
  g_return_val_if_fail (BSE_CONTAINER_GET_CLASS (container)->item_seqid != NULL, 0); /* paranoid */

  return BSE_CONTAINER_GET_CLASS (container)->item_seqid (container, item);
}

BseItem*
bse_container_get_item (BseContainer *container,
			BseType       item_type,
			guint         seqid)
{
  g_return_val_if_fail (BSE_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (seqid > 0, NULL);
  g_return_val_if_fail (bse_type_is_a (item_type, BSE_TYPE_ITEM), NULL);
  g_return_val_if_fail (BSE_CONTAINER_GET_CLASS (container)->get_item != NULL, NULL); /* paranoid */

  return BSE_CONTAINER_GET_CLASS (container)->get_item (container, item_type, seqid);
}

static gboolean
store_forall (BseItem *item,
	      gpointer data_p)
{
  gpointer *data = data_p;
  BseContainer *container = data[0];
  BseStorage *storage = data[1];
  gchar *path;

  bse_storage_break (storage);
  bse_storage_putc (storage, '(');

  path = bse_container_make_item_path (container, item, TRUE);
  bse_storage_puts (storage, path);
  g_free (path);

  bse_storage_push_level (storage);
  bse_object_store (BSE_OBJECT (item), storage);
  bse_storage_pop_level (storage);
  
  return TRUE;
}

void
bse_container_store_items (BseContainer *container,
			   BseStorage   *storage)
{
  gpointer data[2];

  g_return_if_fail (BSE_IS_CONTAINER (container));
  g_return_if_fail (BSE_IS_STORAGE (storage));

  bse_object_ref (BSE_OBJECT (container));
  data[0] = container;
  data[1] = storage;
  bse_container_forall_items (container, store_forall, data);
  bse_object_unref (BSE_OBJECT (container));
}

static void
bse_container_store_termination (BseObject  *object,
				 BseStorage *storage)
{
  /* we *append* items to our normal container stuff,
   * so they even come *after* private stuff, stored by derived containers
   * (which store their stuff through store_private())
   */
  bse_container_store_items (BSE_CONTAINER (object), storage);

  bse_storage_handle_break (storage);
  bse_storage_putc (storage, ')');
}

static BseTokenType
bse_container_try_statement (BseObject  *object,
			     BseStorage *storage)
{
  GScanner *scanner = storage->scanner;
  GTokenType expected_token;

  /* chain parent class' handler */
  expected_token = BSE_OBJECT_CLASS (parent_class)->try_statement (object, storage);

  /* pretty much the only valid reason to overload try_statement() is for this
   * case, where we attempt to parse items as a last resort
   */
  if (expected_token == BSE_TOKEN_UNMATCHED &&
      g_scanner_peek_next_token (scanner) == G_TOKEN_IDENTIFIER)
    {
      BseItem *item;

      item = bse_container_item_from_path (BSE_CONTAINER (object),
					   scanner->next_value.v_identifier);
      if (item)
	{
	  g_scanner_get_next_token (scanner); /* eat the path */

	  /* now let the item restore itself
	   */
	  expected_token = bse_object_restore (BSE_OBJECT (item), storage);
	}
    }

  return expected_token;
}

static gboolean
find_named_item (BseItem *item,
		 gpointer data_p)
{
  gpointer *data = data_p;
  gchar *name = data[1];

  if (bse_string_equals (BSE_OBJECT_NAME (item), name))
    {
      data[0] = item;
      return FALSE;
    }
  return TRUE;
}

BseItem*
bse_container_lookup_item (BseContainer *container,
			   const gchar  *name)
{
  gpointer data[2] = { NULL, };

  g_return_val_if_fail (BSE_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  /* FIXME: better use a hashtable here */
  
  data[1] = (gpointer) name;
  bse_container_forall_items (container, find_named_item, data);

  return data[0];
}

static gboolean
find_named_typed_item (BseItem *item,
		       gpointer data_p)
{
  gpointer *data = data_p;
  gchar *name = data[1];
  BseType type = GPOINTER_TO_UINT (data[2]);

  if (bse_type_is_a (BSE_OBJECT_TYPE (item), type) &&
      bse_string_equals (BSE_OBJECT_NAME (item), name))
    {
      data[0] = item;
      return FALSE;
    }
  return TRUE;
}

BseItem*
bse_container_item_from_handle (BseContainer *container,
				const gchar  *handle)
{
  gchar *type_name, *ident, *name = NULL;
  BseItem *item = NULL;
  BseType type;

  g_return_val_if_fail (BSE_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (handle != NULL, NULL);

  /* handle syntax:
   * <TYPE> [ <:> { <SeqId> | <:> <NAME> } ]
   * examples:
   * "BseItem"	     generic handle    -> create item of type BseItem
   * "BseItem:3"     sequential handle -> get third item of type BseItem
   * "BseItem::foo"  named handle      -> create/get item of type BseItem, named "foo"
   *
   * to get unique matches for named handles, items of a specific
   * container need to have unique names (enforced in bse_container_add_item()
   * and bse_item_do_set_name()).
   */

  type_name = g_strdup (handle);
  ident = strchr (type_name, ':');
  if (ident)
    {
      *(ident++) = 0;
      if (*ident == ':')
	name = ident + 1;
    }
  type = bse_type_from_name (type_name);
  if (bse_type_is_a (type, BSE_TYPE_ITEM))
    {
      if (name)
	{
	  gpointer data[3] = { NULL, };
	  
	  data[1] = name;
	  data[2] = GUINT_TO_POINTER (type);
	  bse_container_forall_items (container, find_named_typed_item, data);
	  item = data[0];

	  if (!item)
	    item = bse_container_new_item (container, type, "name", name, NULL);
	}
      else if (ident)
	{
	  gchar *f = NULL;
	  guint seq_id = strtol (ident, &f, 10);
	  
	  if (seq_id > 0 && (!f || *f == 0))
	    item = bse_container_get_item (container, type, seq_id);
	}
      else
	item = bse_container_new_item (container, type, NULL);
    }
  g_free (type_name);

  return item;
}

BseItem*
bse_container_item_from_path (BseContainer *container,
			      const gchar  *path)
{
  gchar *handle, *next_handle;
  BseItem *item = NULL;

  g_return_val_if_fail (BSE_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (path != NULL, NULL);

  handle = g_strdup (path);
  next_handle = strchr (handle, '.');
  if (next_handle)
    {
      *(next_handle++) = 0;
      item = bse_container_item_from_handle (container, handle);
      if (BSE_IS_CONTAINER (item))
	item = bse_container_item_from_handle (BSE_CONTAINER (item), next_handle);
      else
	item = NULL;
    }
  else
    item = bse_container_item_from_handle (container, handle);
  g_free (handle);

  return item;
}

gchar*
bse_container_make_item_path (BseContainer *container,
			      BseItem      *item,
			      gboolean      persistent)
{
  gchar *path;

  g_return_val_if_fail (BSE_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (BSE_IS_ITEM (item), NULL);
  g_return_val_if_fail (bse_item_has_ancestor (item, BSE_ITEM (container)), NULL);

  path = bse_item_make_handle (item, persistent);

  while (item->container != BSE_ITEM (container))
    {
      gchar *string, *handle;

      item = item->container;
      string = path;
      handle = bse_item_make_handle (item, persistent);
      path = g_strconcat (handle, ".", string, NULL);
      g_free (string);
      g_free (handle);
    }

  return path;
}
