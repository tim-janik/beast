/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-2002 Tim Janik
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
#include        "bsemarshal.h"
#include	"bsemain.h"



enum {
  SIGNAL_SEQID_CHANGED,
  SIGNAL_SET_PARENT,
  SIGNAL_LAST
};


/* --- prototypes --- */
static void	bse_item_class_init		(BseItemClass	*class);
static void	bse_item_init			(BseItem		*item);
static void	bse_item_do_dispose		(GObject		*object);
static void	bse_item_do_destroy		(BseObject		*object);
static void	bse_item_do_set_uloc		(BseObject		*object,
						 const gchar		*uloc);
static guint	bse_item_do_get_seqid		(BseItem		*item);
static void	bse_item_do_set_parent		(BseItem                *item,
						 BseItem                *parent);


/* --- variables --- */
static GTypeClass *parent_class = NULL;
static GSList     *item_seqid_changed_queue = NULL;
static guint       item_signals[SIGNAL_LAST] = { 0, };


/* --- functions --- */
BSE_BUILTIN_TYPE (BseItem)
{
  static const GTypeInfo item_info = {
    sizeof (BseItemClass),

    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_item_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,

    sizeof (BseItem),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_item_init,
  };

  g_assert (BSE_ITEM_FLAGS_USHIFT < BSE_OBJECT_FLAGS_MAX_SHIFT);

  return bse_type_register_static (BSE_TYPE_OBJECT,
				   "BseItem",
				   "Base type for objects managed by a container",
				   &item_info);
}

static void
bse_item_class_init (BseItemClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  gobject_class->dispose = bse_item_do_dispose;

  object_class->set_uloc = bse_item_do_set_uloc;
  object_class->destroy = bse_item_do_destroy;

  class->set_parent = bse_item_do_set_parent;
  class->get_seqid = bse_item_do_get_seqid;

  item_signals[SIGNAL_SEQID_CHANGED] = bse_object_class_add_signal (object_class, "seqid_changed",
								    bse_marshal_VOID__NONE,
								    G_TYPE_NONE, 0);
  item_signals[SIGNAL_SET_PARENT] = bse_object_class_add_signal (object_class, "set_parent",
								 bse_marshal_VOID__OBJECT,
								 G_TYPE_NONE, 1, BSE_TYPE_CONTAINER);
}

static void
bse_item_init (BseItem *item)
{
  item->parent = NULL;
}

static void
bse_item_do_dispose (GObject *gobject)
{
  BseItem *item = BSE_ITEM (gobject);

  g_return_if_fail (item->use_count == 0);

  /* if parent could be != NULL here, we had to force removement */
  g_return_if_fail (item->parent == NULL);

  /* chain parent class' dispose handler */
  G_OBJECT_CLASS (parent_class)->dispose (gobject);
}

static void
bse_item_do_destroy (BseObject *object)
{
  BseItem *item = BSE_ITEM (object);

  item_seqid_changed_queue = g_slist_remove (item_seqid_changed_queue, item);

  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);

  g_return_if_fail (item->use_count == 0);
}

static void
bse_item_do_set_uloc (BseObject   *object,
		      const gchar *uloc)
{
  BseItem *item = BSE_ITEM (object);

  /* ensure that item ulocs within this container are unique
   */
  if (!BSE_IS_CONTAINER (item->parent) ||
      (uloc && !bse_container_lookup_item (BSE_CONTAINER (item->parent), uloc)))
    {
      /* chain parent class' set_uloc handler */
      BSE_OBJECT_CLASS (parent_class)->set_uloc (object, uloc);
    }
}

static void
bse_item_do_set_parent (BseItem *item,
			BseItem *parent)
{
  item->parent = parent;
}

void
bse_item_set_parent (BseItem *item,
		     BseItem *parent)
{
  g_return_if_fail (BSE_IS_ITEM (item));
  if (parent)
    {
      g_return_if_fail (item->parent == NULL);
      g_return_if_fail (BSE_IS_CONTAINER (parent));
    }
  else
    g_return_if_fail (item->parent != NULL);
  g_return_if_fail (BSE_ITEM_GET_CLASS (item)->set_parent != NULL); /* paranoid */

  bse_object_ref (BSE_OBJECT (item));
  if (parent)
    bse_object_ref (BSE_OBJECT (parent));

  g_signal_emit (item, item_signals[SIGNAL_SET_PARENT], 0, parent);

  BSE_ITEM_GET_CLASS (item)->set_parent (item, parent);

  bse_object_unref (BSE_OBJECT (item));
  if (parent)
    bse_object_unref (BSE_OBJECT (parent));
}

static guint
bse_item_do_get_seqid (BseItem *item)
{
  if (item->parent)
    return bse_container_get_item_seqid (BSE_CONTAINER (item->parent), item);
  else
    return 0;
}

static gboolean
idle_handler_seqid_changed (gpointer data)
{
  BSE_THREADS_ENTER ();
  
  while (item_seqid_changed_queue)
    {
      GSList *slist = item_seqid_changed_queue;
      BseItem *item = slist->data;

      item_seqid_changed_queue = slist->next;
      g_slist_free_1 (slist);
      if (item->parent)
	g_signal_emit (item, item_signals[SIGNAL_SEQID_CHANGED], 0);
    }

  BSE_THREADS_LEAVE ();
  
  return FALSE;
}

void
bse_item_queue_seqid_changed (BseItem *item)
{
  g_return_if_fail (BSE_IS_ITEM (item));
  g_return_if_fail (BSE_ITEM (item)->parent != NULL);

  if (!item_seqid_changed_queue)
    g_idle_add_full (BSE_NOTIFY_PRIORITY, idle_handler_seqid_changed, NULL, NULL);

  if (!g_slist_find (item_seqid_changed_queue, item))
    item_seqid_changed_queue = g_slist_prepend (item_seqid_changed_queue, item);
}

guint
bse_item_get_seqid (BseItem *item)
{
  g_return_val_if_fail (BSE_IS_ITEM (item), 0);
  g_return_val_if_fail (BSE_ITEM_GET_CLASS (item)->get_seqid != NULL, 0); /* paranoid */

  return BSE_ITEM_GET_CLASS (item)->get_seqid (item);
}

BseItem*
bse_item_common_ancestor (BseItem *item1,
			  BseItem *item2)
{
  g_return_val_if_fail (BSE_IS_ITEM (item1), NULL);
  g_return_val_if_fail (BSE_IS_ITEM (item2), NULL);

  do
    {
      BseItem *item = item2;

      do
	{
	  if (item == item1)
	    return item;
	  item = item->parent;
	}
      while (item);
      item1 = item1->parent;
    }
  while (item1);

  return NULL;
}

void
bse_item_cross_ref (BseItem         *owner,
		    BseItem         *ref_item,
		    BseItemUncross   uncross_func)
{
  BseItem *container;
  
  g_return_if_fail (BSE_IS_ITEM (owner));
  g_return_if_fail (BSE_IS_ITEM (ref_item));
  g_return_if_fail (uncross_func != NULL);
  
  container = bse_item_common_ancestor (owner, ref_item);
  
  if (container)
    bse_container_cross_ref (BSE_CONTAINER (container), owner, ref_item, uncross_func);
  else
    g_warning ("%s: `%s' and `%s' have no common anchestor", G_STRLOC,
	       BSE_OBJECT_TYPE_NAME (owner),
	       BSE_OBJECT_TYPE_NAME (ref_item));
}

void
bse_item_cross_unref (BseItem *owner,
		      BseItem *ref_item)
{
  BseItem *container;

  g_return_if_fail (BSE_IS_ITEM (owner));
  g_return_if_fail (BSE_IS_ITEM (ref_item));

  container = bse_item_common_ancestor (owner, ref_item);

  if (container)
    bse_container_cross_unref (BSE_CONTAINER (container), owner, ref_item);
  else
    g_warning ("%s: `%s' and `%s' have no common anchestor", G_STRLOC,
	       BSE_OBJECT_TYPE_NAME (owner),
	       BSE_OBJECT_TYPE_NAME (ref_item));
}

static gboolean
cross_list_func (BseItem *owner,
		 BseItem *ref_item,
		 gpointer data_p)
{
  gpointer *data = data_p;
  BseItem *item = data[0];

  if (item == ref_item)
    data[1] = g_list_prepend (data[1], owner);

  return TRUE;
}

GList*
bse_item_list_cross_owners (BseItem *item)
{
  gpointer data[2] = { item, NULL };

  g_return_val_if_fail (BSE_IS_ITEM (item), NULL);

  do
    {
      if (BSE_IS_CONTAINER (item))
	bse_container_cross_forall (BSE_CONTAINER (item), cross_list_func, data);
      item = item->parent;
    }
  while (item);

  return data[1];
}

static gboolean
cross_check_func (BseItem *owner,
		  BseItem *ref_item,
		  gpointer data_p)
{
  gpointer *data = data_p;
  BseItem *item = data[0];

  if (item == ref_item)
    {
      data[1] = GINT_TO_POINTER (TRUE);

      return FALSE;
    }
  else
    return TRUE;
}

gboolean
bse_item_has_cross_owners (BseItem *item)
{
  gpointer data[2] = { item, GINT_TO_POINTER (FALSE) };

  g_return_val_if_fail (BSE_IS_ITEM (item), FALSE);

  do
    {
      if (BSE_IS_CONTAINER (item))
	bse_container_cross_forall (BSE_CONTAINER (item), cross_check_func, data);
      item = item->parent;
    }
  while (item);

  return GPOINTER_TO_INT (data[1]);
}

BseSuper*
bse_item_get_super (BseItem *item)
{
  g_return_val_if_fail (BSE_IS_ITEM (item), NULL);

  while (!BSE_IS_SUPER (item) && item)
    item = item->parent;
  
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

  while (item->parent)
    {
      item = item->parent;
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
    return g_strconcat (BSE_OBJECT_TYPE_NAME (item), "::", BSE_OBJECT_ULOC (item), NULL);
  else
    {
      gchar buffer[10];
      
      g_snprintf (buffer, 10, "%u", bse_item_get_seqid (item));
      
      return g_strconcat (BSE_OBJECT_TYPE_NAME (item), ":", buffer, NULL);
    }
}

gchar* /* free result */
bse_item_make_nick_path (BseItem *item)
{
  BseItem *project;
  gchar *nick = NULL;

  g_return_val_if_fail (BSE_IS_ITEM (item), NULL);

  project = (BseItem*) bse_item_get_project (item);

  while (item && item != project)
    {
      gchar *string = nick;

      nick = g_strconcat (BSE_OBJECT_ULOC (item), string ? "." : NULL, string, NULL);
      g_free (string);
      item = item->parent;
    }

  return nick;
}

static inline BseErrorType
bse_item_execva_i (BseItem     *item,
		   const gchar *procedure,
		   va_list      var_args,
		   gboolean     skip_oparams)
{
  BseProcedureClass *proc;
  BseErrorType error;
  GType   type;
  guint l2;

  /* FIXME: we could need faster lookups here */
  type = BSE_OBJECT_TYPE (item);
  l2 = strlen (procedure);
  do
    {
      gchar *p, *name, *type_name = g_type_name (type);
      guint l1 = strlen (type_name);

      name = strcpy (g_new (gchar, l1 + 1 + l2 + 1), type_name);
      p = name + l1;
      *(p++) = '+';
      strcpy (p, procedure);
      
      proc = bse_procedure_find_ref (name);
      g_free (name);
      type = g_type_parent (type);
    }
  while (!proc && g_type_is_a (type, BSE_TYPE_ITEM));

  if (!BSE_IS_PROCEDURE_CLASS (proc))
    {
      g_warning ("Unable to find procedure \"%s\" for `%s'",
		 procedure,
		 BSE_OBJECT_TYPE_NAME (item));
      return BSE_ERROR_INTERNAL;
    }

  error = bse_procedure_execva_object (proc, BSE_OBJECT (item), var_args, FALSE);

  bse_procedure_unref (proc);

  return error;
}

BseErrorType
bse_item_exec_proc (gpointer	 _item,
		    const gchar *procedure,
		    ...)
{
  BseItem *item = _item;
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
bse_item_exec_void_proc (gpointer     _item,
			 const gchar *procedure,
			 ...)
{
  BseItem *item = _item;
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
