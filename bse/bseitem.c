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
#include        "bseitem.h"

#include        "bsesuper.h"
#include        "bsestorage.h"
#include        "bseprocedure.h"



/* --- prototypes --- */
static void	bse_item_class_init		(BseItemClass	*class);
static void	bse_item_init			(BseItem		*item);
static void	bse_item_destroy		(BseObject		*object);
static void	bse_item_do_set_name		(BseObject		*object,
						 const gchar		*name);
static guint	bse_item_do_get_seqid		(BseItem		*item);
static void	bse_item_do_set_container	(BseItem                *item,
						 BseItem                *container);


/* --- variables --- */
static BseTypeClass	*parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseItem)
{
  static const BseTypeInfo item_info = {
    sizeof (BseItemClass),

    (BseClassInitBaseFunc) NULL,
    (BseClassDestroyBaseFunc) NULL,
    (BseClassInitFunc) bse_item_class_init,
    (BseClassDestroyFunc) NULL,
    NULL /* class_data */,

    sizeof (BseItem),
    0 /* n_preallocs */,
    (BseObjectInitFunc) bse_item_init,
  };

  return bse_type_register_static (BSE_TYPE_OBJECT,
				   "BseItem",
				   "Base type for objects managed by a container",
				   &item_info);
}

static void
bse_item_class_init (BseItemClass *class)
{
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);

  parent_class = bse_type_class_peek (BSE_TYPE_OBJECT);

  object_class->set_name = bse_item_do_set_name;
  object_class->destroy = bse_item_destroy;

  class->set_container = bse_item_do_set_container;
  class->get_seqid = bse_item_do_get_seqid;
  class->seqid_changed = NULL;
}

static void
bse_item_init (BseItem *item)
{
  item->container = NULL;
}

static void
bse_item_destroy (BseObject *object)
{
  BseItem *item;

  item = BSE_ITEM (object);

  if (item->container)
    bse_container_remove_item (BSE_CONTAINER (item->container), item);

  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_item_do_set_name (BseObject   *object,
		      const gchar *name)
{
  BseItem *item = BSE_ITEM (object);

  /* ensure that item names within this container are unique
   */
  if (!BSE_IS_CONTAINER (item->container) ||
      !bse_container_lookup_item (BSE_CONTAINER (item->container), name))
    {
      /* chain parent class' set_name handler */
      BSE_OBJECT_CLASS (parent_class)->set_name (object, name);
    }
}

static void
bse_item_do_set_container (BseItem *item,
			   BseItem *container)
{
  item->container = container;
}

void
bse_item_set_container (BseItem *item,
			BseItem *container)
{
  g_return_if_fail (BSE_IS_ITEM (item));
  if (container)
    {
      g_return_if_fail (item->container == NULL);
      g_return_if_fail (BSE_IS_CONTAINER (container));
    }
  else
    g_return_if_fail (item->container != NULL);
  g_return_if_fail (BSE_ITEM_GET_CLASS (item)->set_container != NULL); /* paranoid */

  bse_object_ref (BSE_OBJECT (item));
  if (container)
    bse_object_ref (BSE_OBJECT (container));

  BSE_NOTIFY (item, set_container, NOTIFY (OBJECT, container, DATA));

  BSE_ITEM_GET_CLASS (item)->set_container (item, container);

  bse_object_unref (BSE_OBJECT (item));
  if (container)
    bse_object_unref (BSE_OBJECT (container));
}

static guint
bse_item_do_get_seqid (BseItem *item)
{
  if (item->container)
    return bse_container_get_item_seqid (BSE_CONTAINER (item->container), item);
  else
    return 0;
}

void
bse_item_seqid_changed (BseItem *item)
{
  g_return_if_fail (BSE_IS_ITEM (item));

  if (BSE_ITEM_GET_CLASS (item)->seqid_changed)
    BSE_ITEM_GET_CLASS (item)->seqid_changed (item);

  BSE_NOTIFY (item, seqid_changed, NOTIFY (OBJECT, DATA));
}

guint
bse_item_get_seqid (BseItem *item)
{
  g_return_val_if_fail (BSE_IS_ITEM (item), 0);
  g_return_val_if_fail (BSE_ITEM_GET_CLASS (item)->get_seqid != NULL, 0); /* paranoid */

  return BSE_ITEM_GET_CLASS (item)->get_seqid (item);
}

BseSuper*
bse_item_get_super (BseItem *item)
{
  g_return_val_if_fail (BSE_IS_ITEM (item), NULL);

  while (!BSE_IS_SUPER (item) && item)
    item = item->container;
  
  return item ? BSE_SUPER (item) : NULL;
}

BseProject*
bse_item_get_project (BseItem *item)
{
  BseSuper *super;

  g_return_val_if_fail (BSE_IS_ITEM (item), NULL);

  super = bse_item_get_super (item);

  return super ? bse_super_get_project (super) : NULL;
}

gboolean
bse_item_has_ancestor (BseItem *item,
		       BseItem *ancestor)
{
  g_return_val_if_fail (BSE_IS_ITEM (item), FALSE);
  g_return_val_if_fail (BSE_IS_ITEM (ancestor), FALSE);

  while (item->container)
    {
      item = item->container;
      if (item == ancestor)
	return TRUE;
    }

  return FALSE;
}

gchar* /* free result */
bse_item_make_handle (BseItem *item,
		      gboolean persistent)
{
  g_return_val_if_fail (BSE_IS_ITEM (item), NULL);

  if (persistent)
    return g_strconcat (BSE_OBJECT_TYPE_NAME (item), "::", BSE_OBJECT_NAME (item), NULL);
  else
    {
      gchar buffer[10];
      
      g_snprintf (buffer, 10, "%u", bse_item_get_seqid (item));
      
      return g_strconcat (BSE_OBJECT_TYPE_NAME (item), ":", buffer, NULL);
    }
}

static inline BseErrorType
bse_item_execva_i (BseItem     *item,
		   const gchar *procedure,
		   va_list      var_args,
		   gboolean     skip_oparams)
{
  BseProcedureClass *proc;
  BseErrorType error;
  BseType type;
  guint l2;

  /* FIXME: we could need faster lookups here */
  type = BSE_OBJECT_TYPE (item);
  l2 = strlen (procedure);
  do
    {
      gchar *p, *name, *type_name = bse_type_name (type);
      guint l1 = strlen (type_name);

      name = strcpy (g_new (gchar, l1 + 2 + l2 + 1), type_name);
      p = name + l1;
      *(p++) = ':';
      *(p++) = ':';
      strcpy (p, procedure);
      
      proc = bse_procedure_find_ref (name);
      g_free (name);
      type = bse_type_parent (type);
    }
  while (!proc && bse_type_is_a (type, BSE_TYPE_ITEM));

  if (!BSE_IS_PROCEDURE_CLASS (proc))
    {
      g_warning ("Unable to find procedure \"%s\" for `%s'",
		 procedure,
		 BSE_OBJECT_TYPE_NAME (item));
      return BSE_ERROR_INTERNAL;
    }

  error = bse_procedure_execva_item (proc, item, var_args, FALSE);

  bse_procedure_unref (proc);

  return error;
}

BseErrorType
bse_item_exec_proc (BseItem        *item,
		    const gchar    *procedure,
		    ...)
{
  va_list var_args;
  BseErrorType error;
  
  g_return_val_if_fail (BSE_IS_ITEM (item), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (procedure != NULL, BSE_ERROR_INTERNAL);

  va_start (var_args, procedure);
  error = bse_item_execva_i (item, procedure, var_args, FALSE);
  va_end (var_args);

  return error;
}

BseErrorType
bse_item_exec_void_proc (BseItem        *item,
			 const gchar    *procedure,
			 ...)
{
  va_list var_args;
  BseErrorType error;
  
  g_return_val_if_fail (BSE_IS_ITEM (item), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (procedure != NULL, BSE_ERROR_INTERNAL);

  va_start (var_args, procedure);
  error = bse_item_execva_i (item, procedure, var_args, TRUE);
  va_end (var_args);

  return error;
}

BseStorage*
bse_item_open_undo (BseItem     *item,
		    const gchar *undo_group)
{
  g_return_val_if_fail (BSE_IS_ITEM (item), NULL);
  g_return_val_if_fail (undo_group != NULL, NULL);

  return NULL;
}

void
bse_item_close_undo (BseItem    *item,
		     BseStorage *storage)
{
  g_return_if_fail (BSE_IS_ITEM (item));
  g_return_if_fail (BSE_IS_STORAGE (storage));
}
