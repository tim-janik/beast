/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-1999, 2000-2002 Tim Janik
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
#include	"bseproject.h"
#include	"bsestorage.h"
#include	"bsemain.h"
#include	"gslengine.h"
#include	<stdlib.h>
#include	<string.h>


#define parse_or_return         bse_storage_scanner_parse_or_return
#define peek_or_return          bse_storage_scanner_peek_or_return


enum
{
  SIGNAL_ITEM_ADDED,
  SIGNAL_ITEM_REMOVED,
  SIGNAL_LAST
};


/* --- prototypes --- */
static void	    bse_container_class_init		(BseContainerClass	*class);
static void	    bse_container_class_finalize	(BseContainerClass	*class);
static void	    bse_container_init			(BseContainer		*container);
static void	    bse_container_dispose		(GObject		*object);
static void	    bse_container_finalize		(GObject		*object);
static void	    bse_container_store_after		(BseObject		*object,
							 BseStorage		*storage);
static BseTokenType bse_container_try_statement		(BseObject		*object,
							 BseStorage		*storage);
static void	    bse_container_do_add_item		(BseContainer		*container,
							 BseItem		*item);
static void	    bse_container_do_remove_item	(BseContainer		*container,
							 BseItem		*item);
static BseItem*	    bse_container_real_retrieve_child	(BseContainer		*container,
							 GType			 child_type,
							 const gchar		*uname);
static void         bse_container_prepare               (BseSource              *source);
static void	    bse_container_context_create	(BseSource		*source,
							 guint			  context_handle,
							 GslTrans		*trans);
static void	    bse_container_context_connect	(BseSource		*source,
							 guint			 context_handle,
							 GslTrans		*trans);
static void	    bse_container_context_dismiss	(BseSource		*source,
							 guint			 context_handle,
							 GslTrans		*trans);
static void         bse_container_reset                 (BseSource              *source);
static GSList*	    container_context_children		(BseContainer		*container);
static void	    container_release_children		(BseContainer		*container);


/* --- variables --- */
static GTypeClass	*parent_class = NULL;
static GQuark		 quark_cross_refs = 0;
static GSList           *containers_cross_changes = NULL;
static guint             containers_cross_changes_handler = 0;
static guint		 container_signals[SIGNAL_LAST] = { 0, };


/* --- functions --- */
BSE_BUILTIN_TYPE (BseContainer)
{
  static const GTypeInfo container_info = {
    sizeof (BseContainerClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_container_class_init,
    (GClassFinalizeFunc) bse_container_class_finalize,
    NULL /* class_data */,
    
    sizeof (BseContainer),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_container_init,
  };
  
  return bse_type_register_static (BSE_TYPE_SOURCE,
				   "BseContainer",
				   "Base type to manage BSE items",
				   &container_info);
}

static void
bse_container_class_init (BseContainerClass *class)
{
  GObjectClass *gobject_class;
  BseObjectClass *object_class;
  BseItemClass *item_class;
  BseSourceClass *source_class;
  
  parent_class = g_type_class_peek_parent (class);
  gobject_class = G_OBJECT_CLASS (class);
  object_class = BSE_OBJECT_CLASS (class);
  item_class = BSE_ITEM_CLASS (class);
  source_class = BSE_SOURCE_CLASS (class);
  
  quark_cross_refs = g_quark_from_static_string ("BseContainerCrossRefs");
  
  gobject_class->dispose = bse_container_dispose;
  gobject_class->finalize = bse_container_finalize;
  
  object_class->store_after = bse_container_store_after;
  object_class->try_statement = bse_container_try_statement;
  
  source_class->prepare = bse_container_prepare;
  source_class->context_create = bse_container_context_create;
  source_class->context_connect = bse_container_context_connect;
  source_class->context_dismiss = bse_container_context_dismiss;
  source_class->reset = bse_container_reset;
  
  class->add_item = bse_container_do_add_item;
  class->remove_item = bse_container_do_remove_item;
  class->forall_items = NULL;
  class->retrieve_child = bse_container_real_retrieve_child;
  class->context_children = container_context_children;
  class->release_children = container_release_children;

  container_signals[SIGNAL_ITEM_ADDED] = bse_object_class_add_signal (object_class, "item_added",
								      G_TYPE_NONE, 1, BSE_TYPE_ITEM);
  container_signals[SIGNAL_ITEM_REMOVED] = bse_object_class_add_signal (object_class, "item_removed",
									G_TYPE_NONE, 1, BSE_TYPE_ITEM);
}

static void
bse_container_class_finalize (BseContainerClass *class)
{
}

static void
bse_container_init (BseContainer *container)
{
  container->n_items = 0;
}

static void
container_release_children (BseContainer *container)
{
  /* provide function pointer for easy chaining */
}

static void
bse_container_dispose (GObject *gobject)
{
  BseContainer *container = BSE_CONTAINER (gobject);

  if (!BSE_ITEM (container)->use_count)
    {
      BSE_CONTAINER_GET_CLASS (container)->release_children (container);

      /* remove any existing cross-references (with notification) */
      g_object_set_qdata (container, quark_cross_refs, NULL);
    }

  /* chain parent class' dispose handler */
  G_OBJECT_CLASS (parent_class)->dispose (gobject);
}

static void
bse_container_finalize (GObject *gobject)
{
  BseContainer *container = BSE_CONTAINER (gobject);

  if (container->n_items)
    g_message ("%s: finalize handlers missed to remove %u items from %s",
	       G_STRLOC,
	       container->n_items,
	       BSE_OBJECT_TYPE_NAME (container));
  
  /* chain parent class' finalize handler */
  G_OBJECT_CLASS (parent_class)->finalize (gobject);
  
  /* gobject->finalize() clears the datalist, which may cause this
   * container to end up in the containers_cross_changes list again,
   * so we make sure it is removed *after* the datalist has been
   * cleared. though gobject is an invalid pointer at this time,
   * we can still use it for list removal.
   */
  containers_cross_changes = g_slist_remove_all (containers_cross_changes, gobject);
}

static void
bse_container_do_add_item (BseContainer *container,
			   BseItem	*item)
{
  g_object_ref (item);
  container->n_items += 1;
  bse_item_set_parent (item, BSE_ITEM (container));
  
  if (BSE_IS_SOURCE (item) && BSE_SOURCE_PREPARED (container))
    {
      GslTrans *trans = gsl_trans_open ();
      guint *cids, n, c;
      
      g_return_if_fail (BSE_SOURCE_PREPARED (item) == FALSE);
      
      bse_source_prepare (BSE_SOURCE (item));
      
      /* create and connect item contexts */
      cids = bse_source_context_ids (BSE_SOURCE (container), &n);
      for (c = 0; c < n; c++)
	bse_source_create_context (BSE_SOURCE (item), cids[c], trans);
      for (c = 0; c < n; c++)
	bse_source_connect_context (BSE_SOURCE (item), cids[c], trans);
      g_free (cids);
      gsl_trans_commit (trans);
    }
}

void
bse_container_add_item (BseContainer *container,
			BseItem      *item)
{
  gchar *uname;
  
  g_return_if_fail (BSE_IS_CONTAINER (container));
  g_return_if_fail (BSE_IS_ITEM (item));
  g_return_if_fail (item->parent == NULL);
  g_return_if_fail (BSE_CONTAINER_GET_CLASS (container)->add_item != NULL); /* paranoid */
  
  g_object_ref (container);
  g_object_ref (item);
  g_object_freeze_notify (G_OBJECT (container));
  g_object_freeze_notify (G_OBJECT (item));
  
  uname = BSE_OBJECT_UNAME (item);
  
  /* ensure uniqueness of item unames within a container
   */
  if (!uname || bse_container_lookup_item (container, uname))
    {
      gchar *buffer, *p;
      guint i = 0, l;
      
      if (!uname)
	uname = BSE_OBJECT_TYPE_NAME (item);
      
      l = strlen (uname);
      buffer = g_new (gchar, l + 12);
      strcpy (buffer, uname);
      p = buffer + l;
      do
	g_snprintf (p, 11, "-%u", ++i);
      while (bse_container_lookup_item (container, buffer));
      
      g_object_set (item, "uname", buffer, NULL);
      g_free (buffer);
    }
  
  BSE_CONTAINER_GET_CLASS (container)->add_item (container, item);
  if (item->parent != NULL)
    g_signal_emit (container, container_signals[SIGNAL_ITEM_ADDED], 0, item);
  
  g_object_thaw_notify (G_OBJECT (item));
  g_object_thaw_notify (G_OBJECT (container));
  g_object_unref (item);
  g_object_unref (container);
}

gpointer
bse_container_new_item (BseContainer *container,
			GType         item_type,
			const gchar  *first_param_name,
			...)
{
  gpointer item;
  va_list var_args;
  
  g_return_val_if_fail (BSE_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (g_type_is_a (item_type, BSE_TYPE_ITEM), NULL);
  
  va_start (var_args, first_param_name);
  item = g_object_new_valist (item_type, first_param_name, var_args);
  va_end (var_args);
  bse_container_add_item (container, item);
  g_object_unref (item);
  
  return item;
}

static void
bse_container_do_remove_item (BseContainer *container,
			      BseItem	   *item)
{
  BseItem *ancestor = BSE_ITEM (container);
  
  container->n_items -= 1;
  
  do
    {
      bse_container_uncross_item (BSE_CONTAINER (ancestor), item);
      ancestor = ancestor->parent;
    }
  while (ancestor);
  
  if (BSE_IS_SOURCE (item))
    {
      /* detach item from rest of the world */
      bse_source_clear_ichannels (BSE_SOURCE (item));
      bse_source_clear_ochannels (BSE_SOURCE (item));
      /* before mudling with its state */
      if (BSE_SOURCE_PREPARED (container))
	{
	  g_return_if_fail (BSE_SOURCE_PREPARED (item) == TRUE);
	  
	  bse_source_reset (BSE_SOURCE (item));
	}
    }
  
  /* reset parent *after* uncrossing, so "release" notification
   * on item operates on sane object trees
   */
  bse_item_set_parent (item, NULL);
  
  g_object_unref (item);
}

void
bse_container_remove_item (BseContainer *container,
			   BseItem      *item)
{
  gboolean finalizing_container;

  g_return_if_fail (BSE_IS_CONTAINER (container));
  g_return_if_fail (BSE_IS_ITEM (item));
  g_return_if_fail (item->parent == BSE_ITEM (container));
  g_return_if_fail (BSE_CONTAINER_GET_CLASS (container)->remove_item != NULL); /* paranoid */
  
  finalizing_container = G_OBJECT (container)->ref_count == 0;
  if (!finalizing_container)
    g_object_ref (container);
  g_object_ref (item);
  
  BSE_CONTAINER_GET_CLASS (container)->remove_item (container, item);
  g_object_freeze_notify (G_OBJECT (container));
  g_object_freeze_notify (G_OBJECT (item));
  if (!finalizing_container)
    g_signal_emit (container, container_signals[SIGNAL_ITEM_REMOVED], 0, item);
  g_object_thaw_notify (G_OBJECT (item));
  g_object_thaw_notify (G_OBJECT (container));
  
  g_object_unref (item);
  if (!finalizing_container)
    g_object_unref (container);
}

void
bse_container_forall_items (BseContainer      *container,
			    BseForallItemsFunc func,
			    gpointer           data)
{
  g_return_if_fail (BSE_IS_CONTAINER (container));
  g_return_if_fail (func != NULL);
  
  if (container->n_items)
    {
      g_return_if_fail (BSE_CONTAINER_GET_CLASS (container)->forall_items != NULL); /* paranoid */
      
      BSE_CONTAINER_GET_CLASS (container)->forall_items (container, func, data);
    }
}

static gboolean
list_items (BseItem *item,
	    gpointer data)
{
  BseProxySeq *pseq = data;
  
  bse_proxy_seq_append (pseq, BSE_OBJECT_ID (item));
  
  return TRUE;
}

BseProxySeq*
bse_container_list_items (BseContainer *container)
{
  BseProxySeq *pseq;
  
  g_return_val_if_fail (BSE_IS_CONTAINER (container), NULL);
  
  pseq = bse_proxy_seq_new ();
  if (container->n_items)
    {
      g_return_val_if_fail (BSE_CONTAINER_GET_CLASS (container)->forall_items != NULL, NULL); /* paranoid */
      
      BSE_CONTAINER_GET_CLASS (container)->forall_items (container, list_items, pseq);
    }
  
  return pseq;
}

static gboolean
count_item_seqid (BseItem *item,
		  gpointer data_p)
{
  gpointer *data = data_p;
  
  if (G_OBJECT_TYPE (item) == (GType) data[2])
    data[0] = GUINT_TO_POINTER (GPOINTER_TO_UINT (data[0]) + 1);
  
  if (item == data[1])
    {
      data[1] = NULL;
      return FALSE;
    }
  else
    return TRUE;
}

guint
bse_container_get_item_seqid (BseContainer *container,
			      BseItem      *item)
{
  g_return_val_if_fail (BSE_IS_CONTAINER (container), 0);
  g_return_val_if_fail (BSE_IS_ITEM (item), 0);
  g_return_val_if_fail (item->parent == BSE_ITEM (container), 0);
  
  if (container->n_items)
    {
      gpointer data[3];
      
      g_return_val_if_fail (BSE_CONTAINER_GET_CLASS (container)->forall_items != NULL, 0); /* paranoid */
      
      data[0] = GUINT_TO_POINTER (0);
      data[1] = item;
      data[2] = (gpointer) G_OBJECT_TYPE (item);
      
      BSE_CONTAINER_GET_CLASS (container)->forall_items (container, count_item_seqid, data);
      
      return data[1] == NULL ? GPOINTER_TO_UINT (data[0]) : 0;
    }
  else
    return 0;
}

static gboolean
find_nth_item (BseItem *item,
	       gpointer data_p)
{
  gpointer *data = data_p;
  
  if (G_OBJECT_TYPE (item) == (GType) data[2])
    {
      data[0] = GUINT_TO_POINTER (GPOINTER_TO_UINT (data[0]) - 1);
      if (GPOINTER_TO_UINT (data[0]) == 0)
	{
	  data[1] = item;
	  return FALSE;
	}
    }
  return TRUE;
}

BseItem*
bse_container_get_item (BseContainer *container,
			GType         item_type,
			guint         seqid)
{
  g_return_val_if_fail (BSE_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (seqid > 0, NULL);
  g_return_val_if_fail (g_type_is_a (item_type, BSE_TYPE_ITEM), NULL);
  
  if (container->n_items)
    {
      gpointer data[3];
      
      g_return_val_if_fail (BSE_CONTAINER_GET_CLASS (container)->forall_items != NULL, NULL); /* paranoid */
      
      data[0] = GUINT_TO_POINTER (seqid);
      data[1] = NULL;
      data[2] = (gpointer) item_type;
      
      BSE_CONTAINER_GET_CLASS (container)->forall_items (container, find_nth_item, data);
      
      return data[1];
    }
  else
    return NULL;
}

static gboolean
store_forall (BseItem *item,
	      gpointer data_p)
{
  gpointer *data = data_p;
  // BseContainer *container = data[0];
  BseStorage *storage = data[1];
  const gchar *restore_func = data[2];
  
  if (!BSE_ITEM_AGGREGATE (item))
    {
      gchar *uname = g_strescape (BSE_OBJECT_UNAME (item), NULL);
      gchar *type_uname = g_strconcat (G_OBJECT_TYPE_NAME (item),
				       "::",
				       uname,
				       NULL);
      g_free (uname);
      bse_storage_break (storage);
      bse_storage_printf (storage, "(%s \"%s\"", restore_func, type_uname);
      bse_storage_needs_break (storage);
      g_free (type_uname);
      
      bse_storage_push_level (storage);
      bse_object_store (BSE_OBJECT (item), storage);
      bse_storage_pop_level (storage);
    }
  
  return TRUE;
}

void
bse_container_store_items (BseContainer *container,
			   BseStorage   *storage,
			   const gchar	*restore_func)
{
  gpointer data[3];
  
  g_return_if_fail (BSE_IS_CONTAINER (container));
  g_return_if_fail (BSE_IS_STORAGE (storage));
  g_return_if_fail (restore_func != NULL);
  
  g_object_ref (container);
  data[0] = container;
  data[1] = storage;
  data[2] = (gpointer) restore_func;
  bse_container_forall_items (container, store_forall, data);
  g_object_unref (container);
}

static void
bse_container_store_after (BseObject  *object,
			   BseStorage *storage)
{
  /* we *append* items to our normal container stuff, so they
   * come _after_ private stuff, stored by derived containers
   * (which usually store their stuff through store_private())
   */
  bse_container_store_items (BSE_CONTAINER (object), storage, "bse-storage-restore");
  
  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->store_after)
    BSE_OBJECT_CLASS (parent_class)->store_after (object, storage);
}

static BseTokenType
bse_container_try_statement (BseObject  *object,
			     BseStorage *storage)
{
  GScanner *scanner = storage->scanner;
  GTokenType expected_token;
  
  /* chain parent class' handler */
  expected_token = BSE_OBJECT_CLASS (parent_class)->try_statement (object, storage);
  
  /* one of the few reason to overload try_statement() is for this
   * case, where we attempt to parse items as a last resort
   */
  if (expected_token == BSE_TOKEN_UNMATCHED &&
      g_scanner_peek_next_token (scanner) == G_TOKEN_IDENTIFIER)
    {
      BseContainer *container = BSE_CONTAINER (object);
      BseItem *item;
      
      if (strcmp ("bse-container-restore", scanner->next_value.v_identifier) == 0)
	{
	  // FIXME: compat code, remove
	  bse_storage_warn (storage, "encountered deprecated statement: %s", "bse-container-restore");
	  goto storage_restore;
	}
      else if (strcmp ("bse-storage-restore", scanner->next_value.v_identifier) == 0)
	{
	storage_restore:
	  parse_or_return (scanner, G_TOKEN_IDENTIFIER);	/* eat identifier */
	  parse_or_return (scanner, G_TOKEN_STRING);		/* type_uname argument */
	  
	  item = bse_container_retrieve_child (container, scanner->value.v_string);
	  if (!item)
	    return bse_storage_warn_skip (storage, "unable to create object from (invalid) reference: %s",
					  scanner->value.v_string);
	}
      else	/* deprecated compat, the uname is an identifier */
	{
	  item = bse_container_retrieve_child (container, scanner->next_value.v_identifier);
	  if (!item)						/* probably not a compat case */
	    return expected_token;
	  parse_or_return (scanner, G_TOKEN_IDENTIFIER);	/* eat identifier */
	  bse_storage_warn (storage, "deprecated syntax: non-string uname path: %s", scanner->value.v_identifier);
	}
      
      /* now let the item restore itself
       */
      expected_token = bse_object_restore (BSE_OBJECT (item), storage);
    }
  
  return expected_token;
}

static gboolean
find_unamed_item (BseItem *item,
		  gpointer data_p)
{
  gpointer *data = data_p;
  gchar *uname = data[1];
  
  if (bse_string_equals (BSE_OBJECT_UNAME (item), uname))
    {
      data[0] = item;
      return FALSE;
    }
  return TRUE;
}

BseItem*
bse_container_lookup_item (BseContainer *container,
			   const gchar  *uname)
{
  gpointer data[2] = { NULL, };
  
  g_return_val_if_fail (BSE_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (uname != NULL, NULL);
  
  /* FIXME: better use a hashtable here */
  
  data[1] = (gpointer) uname;
  bse_container_forall_items (container, find_unamed_item, data);
  
  return data[0];
}

static BseItem*
bse_container_real_retrieve_child (BseContainer *container,
				   GType         child_type,
				   const gchar  *uname)
{
  BseItem *item;
  
  if (uname)
    {
      item = bse_container_lookup_item (container, uname);
      if (item && !g_type_is_a (G_OBJECT_TYPE (item), child_type))
	item = NULL;
      else if (!item)
	item = bse_container_new_item (container, child_type, "uname", uname, NULL);
    }
  else
    item = bse_container_new_item (container, child_type, NULL);
  
  return item;
}

BseItem*
bse_container_retrieve_child (BseContainer *container,
			      const gchar  *type_uname)
{
  BseItem *item = NULL;
  gchar *type_name, *uname;
  GType type;
  
  g_return_val_if_fail (BSE_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (type_uname != NULL, NULL);
  
  /* type_uname syntax:
   * <TYPE> [ <::> <UNAME> ]
   * examples:
   * "BseItem"		generic item	-> create item of type BseItem
   * "BseItem::foo"	unamed item	-> create/get item of type BseItem with uname "foo"
   *
   * to get unique matches for unames, items of a specific
   * container need to have unique names (enforced in bse_container_add_item()
   * and bse_item_do_set_uname()).
   */
  
  type_name = g_strdup (type_uname);
  uname = strchr (type_name, ':');
  if (uname)
    {
      if (uname[1] != ':')	/* a single colon is invalid */
	{
	  g_free (type_name);
	  return NULL;
	}
      *uname = 0;
      uname += 2;
    }
  type = g_type_from_name (type_name);
  if (g_type_is_a (type, BSE_TYPE_ITEM))
    item = BSE_CONTAINER_GET_CLASS (container)->retrieve_child (container, type, uname);
  g_free (type_name);
  
  return item;
}

BseItem*
bse_container_resolve_upath (BseContainer *container,
			     const gchar  *upath)
{
  gchar *next_uname;
  
  g_return_val_if_fail (BSE_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (upath != NULL, NULL);
  
  /* upaths consist of a colon seperated unames from the item's ancestry */
  
  next_uname = strchr (upath, ':');
  if (next_uname)
    {
      gchar *uname = g_strndup (upath, next_uname - upath);
      BseItem *item = bse_container_lookup_item (container, uname);
      
      g_free (uname);
      if (BSE_IS_CONTAINER (item))
	return bse_container_resolve_upath (BSE_CONTAINER (item), next_uname + 1);
      else
	return NULL;
    }
  else
    return bse_container_lookup_item (container, upath);
}

gchar* /* free result */
bse_container_make_upath (BseContainer *container,
			  BseItem      *item)
{
  BseItem *self_item;
  GSList *slist, *ulist = NULL;
  gchar *path, *p;
  guint n;
  
  g_return_val_if_fail (BSE_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (BSE_IS_ITEM (item), NULL);
  self_item = BSE_ITEM (container);
  g_return_val_if_fail (bse_item_has_ancestor (item, self_item), NULL);	/* item != self_item */
  
  n = 0;
  for (; item != self_item; item = item->parent)
    {
      ulist = g_slist_prepend (ulist, BSE_OBJECT_UNAME (item));
      n += strlen (ulist->data) + 1;
    }
  path = g_new (gchar, n);
  p = path;
  for (slist = ulist; slist; slist = slist->next)
    {
      strcpy (p, slist->data);
      p += strlen (p);
      if (slist->next)
	*p++ = ':';
    }
  g_slist_free (ulist);
  
  return path;
}

static gboolean
notify_cross_changes (gpointer data)
{
  BSE_THREADS_ENTER ();
  
  while (containers_cross_changes)
    {
      BseContainer *container = BSE_CONTAINER (containers_cross_changes->data);
      
      containers_cross_changes = g_slist_remove_all (containers_cross_changes, container);
      // g_signal_emit (container, container_signals[SIGNAL_CROSS_CHANGES], 0);
    }
  containers_cross_changes_handler = 0;
  
  BSE_THREADS_LEAVE ();
  
  return FALSE;
}

static inline void
container_queue_cross_changes (BseContainer *container)
{
  if (!containers_cross_changes_handler)
    containers_cross_changes_handler = bse_idle_notify (notify_cross_changes, NULL);
  containers_cross_changes = g_slist_prepend (containers_cross_changes, container);
}

typedef struct
{
  BseItem           *owner;
  BseItem           *ref_item;
  BseItemUncross     uncross;
  // gpointer         data;
} CrossRef;
typedef struct
{
  guint         n_cross_refs;
  BseContainer *container;
  CrossRef      cross_refs[1]; /* flexible array */
} BseContainerCrossRefs;

static inline void
uncross_ref (BseContainerCrossRefs *crefs,
	     guint                  n,
	     gboolean		    notify)
{
  BseItem *owner = crefs->cross_refs[n].owner;
  BseItem *ref_item = crefs->cross_refs[n].ref_item;
  BseItemUncross uncross = crefs->cross_refs[n].uncross;
  
  crefs->n_cross_refs--;
  if (n < crefs->n_cross_refs)
    crefs->cross_refs[n] = crefs->cross_refs[crefs->n_cross_refs];
  
  if (notify)
    uncross (owner, ref_item);
}

static void
destroy_crefs (gpointer data)
{
  BseContainerCrossRefs *crefs = data;
  
  if (crefs->n_cross_refs)
    container_queue_cross_changes (crefs->container);
  
  while (crefs->n_cross_refs)
    uncross_ref (crefs, crefs->n_cross_refs - 1, TRUE);
  g_free (crefs);
}

static inline void
container_set_crefs (gpointer               container,
		     BseContainerCrossRefs *crefs)
{
  g_object_steal_qdata (container, quark_cross_refs);
  g_object_set_qdata_full (container, quark_cross_refs, crefs, destroy_crefs);
}

static inline BseContainerCrossRefs*
container_get_crefs (gpointer container)
{
  return g_object_get_qdata (container, quark_cross_refs);
}

void
bse_container_cross_ref (BseContainer    *container,
			 BseItem         *owner,
			 BseItem         *ref_item,
			 BseItemUncross	  uncross)
{
  BseContainerCrossRefs *crefs;
  guint i;
  
  /* prerequisites:
   * container == bse_item_common_ancestor (owner, ref_item)
   * container != owner || container != ref_item
   */
  
  g_return_if_fail (BSE_IS_CONTAINER (container));
  g_return_if_fail (BSE_IS_ITEM (owner));
  g_return_if_fail (BSE_IS_ITEM (ref_item));
  g_return_if_fail (uncross != NULL);
  
  crefs = container_get_crefs (container);
  if (!crefs)
    {
      i = 0;
      crefs = g_realloc (crefs, sizeof (BseContainerCrossRefs));
      crefs->n_cross_refs = i + 1;
      crefs->container = container;
      container_set_crefs (container, crefs);
    }
  else
    {
      BseContainerCrossRefs *old_loc = crefs;
      
      i = crefs->n_cross_refs++;
      crefs = g_realloc (crefs, sizeof (BseContainerCrossRefs) + i * sizeof (crefs->cross_refs[0]));
      if (old_loc != crefs)
	container_set_crefs (container, crefs);
    }
  crefs->cross_refs[i].owner = owner;
  crefs->cross_refs[i].ref_item = ref_item;
  crefs->cross_refs[i].uncross = uncross;
  // crefs->cross_refs[i].data = data;
  
  container_queue_cross_changes (container);
}

void
bse_container_cross_unref (BseContainer *container,
			   BseItem      *owner,
			   BseItem      *ref_item,
			   gboolean	 notify)
{
  BseContainerCrossRefs *crefs;
  gboolean found_one = FALSE;
  
  g_return_if_fail (BSE_IS_CONTAINER (container));
  g_return_if_fail (BSE_IS_ITEM (owner));
  g_return_if_fail (BSE_IS_ITEM (ref_item));
  
  g_object_ref (container);
  g_object_ref (owner);
  g_object_ref (ref_item);
  
  crefs = container_get_crefs (container);
  if (crefs)
    {
      guint i = 0;
      
      while (i < crefs->n_cross_refs)
	{
	  if (crefs->cross_refs[i].owner == owner &&
	      crefs->cross_refs[i].ref_item == ref_item)
	    {
	      uncross_ref (crefs, i, notify);
	      container_queue_cross_changes (container);
	      found_one = TRUE;
	      break;
	    }
	  i++;
	}
    }
  
  if (!found_one)
    g_warning (G_STRLOC ": unable to find cross ref from `%s' to `%s' on `%s'",
	       G_OBJECT_TYPE_NAME (owner),
	       G_OBJECT_TYPE_NAME (ref_item),
	       G_OBJECT_TYPE_NAME (container));
  
  g_object_unref (ref_item);
  g_object_unref (owner);
  g_object_unref (container);
}

/* we could in theory use bse_item_has_ancestor() here,
 * but actually this test should be as fast as possible,
 * and we also want to catch item == container
 */
static inline gboolean
item_check_branch (BseItem *item,
		   gpointer container)
{
  BseItem *ancestor = container;
  
  do
    {
      if (item == ancestor)
	return TRUE;
      item = item->parent;
    }
  while (item);
  
  return FALSE;
}

void
bse_container_uncross_item (BseContainer *container,
			    BseItem      *item)
{
  BseContainerCrossRefs *crefs;
  gboolean found_one = FALSE;
  
  /* prerequisites:
   * bse_item_has_ancestor (item, container) == TRUE
   */
  
  g_return_if_fail (BSE_IS_CONTAINER (container));
  g_return_if_fail (BSE_IS_ITEM (item));
  
  crefs = container_get_crefs (container);
  if (crefs)
    {
      guint i = 0;
      
      g_object_ref (container);
      g_object_ref (item);
      
      /* suppress tree walks where possible
       */
      if (!BSE_IS_CONTAINER (item) || ((BseContainer*) item)->n_items == 0)
	while (i < crefs->n_cross_refs)
	  {
	    if (crefs->cross_refs[i].owner == item || crefs->cross_refs[i].ref_item == item)
	      {
		found_one = TRUE;
		uncross_ref (crefs, i, TRUE);
	      }
	    else
	      i++;
	  }
      else /* need to check whether item is ancestor of any of the cross-ref items here */
	{
	  BseItem *saved_parent, *citem = BSE_ITEM (container);
	  
	  /* we do some minor hackery here, for optimization purposes:
	   * since item is a child of container, we don't need to walk
	   * ->owner's or ->ref_item's ancestor list any further than
	   * up to reaching container.
	   * to suppress extra checks in item_check_branch() in this
	   * regard, we simply set container->parent to NULL temporarily
	   * and with that cause item_check_branch() to abort automatically
	   */
	  saved_parent = citem->parent;
	  citem->parent = NULL;
	  while (i < crefs->n_cross_refs)
	    {
	      if (item_check_branch (crefs->cross_refs[i].owner, item) ||
		  item_check_branch (crefs->cross_refs[i].ref_item, item))
		{
		  citem->parent = saved_parent;
		  
		  found_one = TRUE;
		  uncross_ref (crefs, i, TRUE);
		  
		  saved_parent = citem->parent;
		  citem->parent = NULL;
		}
	      else
		i++;
	    }
	  citem->parent = saved_parent;
	}
      
      if (found_one)
	container_queue_cross_changes (container);
      
      g_object_unref (item);
      g_object_unref (container);
    }
}

void
bse_container_cross_forall (BseContainer      *container,
			    BseForallCrossFunc func,
			    gpointer           data)
{
  BseContainerCrossRefs *crefs;
  
  g_return_if_fail (BSE_IS_CONTAINER (container));
  g_return_if_fail (func != NULL);
  
  crefs = container_get_crefs (container);
  if (crefs)
    {
      guint i;
      
      for (i = 0; i < crefs->n_cross_refs; i++)
	if (!func (crefs->cross_refs[i].owner, crefs->cross_refs[i].ref_item, data))
	  return;
    }
}

static gboolean
forall_prepare (BseItem *item,
		gpointer data)
{
  if (BSE_IS_SOURCE (item) && !BSE_SOURCE_PREPARED (item))
    bse_source_prepare (BSE_SOURCE (item));
  
  return TRUE;
}

static void
bse_container_prepare (BseSource *source)
{
  BseContainer *container = BSE_CONTAINER (source);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
  
  /* make sure all BseSource children are prepared last */
  if (container->n_items)
    {
      g_return_if_fail (BSE_CONTAINER_GET_CLASS (container)->forall_items != NULL); /* paranoid */
      
      BSE_CONTAINER_GET_CLASS (container)->forall_items (container, forall_prepare, NULL);
    }
}

static gboolean
forall_slist_prepend (BseItem *item,
		      gpointer data)
{
  GSList **slist_p = data;
  
  if (BSE_IS_SOURCE (item))
    *slist_p = g_slist_prepend (*slist_p, item);
  
  return TRUE;
}

static GSList*
container_context_children (BseContainer *container)
{
  GSList *slist = NULL;
  
  g_return_val_if_fail (BSE_CONTAINER_GET_CLASS (container)->forall_items != NULL, NULL); /* paranoid */
  
  BSE_CONTAINER_GET_CLASS (container)->forall_items (container, forall_slist_prepend, &slist);
  
  return slist;
}

static void
bse_container_context_create (BseSource	*source,
			      guint	 context_handle,
			      GslTrans  *trans)
{
  BseContainer *container = BSE_CONTAINER (source);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
  
  /* handle children */
  if (container->n_items)
    {
      GSList *node, *slist = BSE_CONTAINER_GET_CLASS (container)->context_children (container);
      
      for (node = slist; node; node = node->next)
	bse_source_create_context (node->data, context_handle, trans);
      g_slist_free (slist);
    }
}

static gboolean
forall_context_connect (BseItem *item,
			gpointer _data)
{
  gpointer *data = _data;
  guint cid = GPOINTER_TO_UINT (data[0]);
  
  if (BSE_IS_SOURCE (item))
    {
      BseSource *source = BSE_SOURCE (item);
      
      g_return_val_if_fail (BSE_SOURCE_PREPARED (item), TRUE);
      
      if (bse_source_has_context (source, cid))
	bse_source_connect_context (source, cid, data[1]);
    }
  
  return TRUE;
}

static void
bse_container_context_connect (BseSource *source,
			       guint	  context_handle,
			       GslTrans  *trans)
{
  BseContainer *container = BSE_CONTAINER (source);
  
  /* handle children */
  if (container->n_items)
    {
      gpointer data[2] = { GUINT_TO_POINTER (context_handle), trans };
      
      g_return_if_fail (BSE_CONTAINER_GET_CLASS (container)->forall_items != NULL); /* paranoid */
      
      BSE_CONTAINER_GET_CLASS (container)->forall_items (container, forall_context_connect, data);
    }
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_connect (source, context_handle, trans);
}

static gboolean
forall_context_dismiss (BseItem *item,
			gpointer _data)
{
  gpointer *data = _data;
  guint cid = GPOINTER_TO_UINT (data[0]);
  
  if (BSE_IS_SOURCE (item))
    {
      BseSource *source = BSE_SOURCE (item);
      
      g_return_val_if_fail (BSE_SOURCE_PREPARED (item), TRUE);
      
      if (bse_source_has_context (source, cid))
	bse_source_dismiss_context (source, cid, data[1]);
    }
  
  return TRUE;
}

static void
bse_container_context_dismiss (BseSource *source,
			       guint	  context_handle,
			       GslTrans  *trans)
{
  BseContainer *container = BSE_CONTAINER (source);
  
  /* handle children */
  if (container->n_items)
    {
      gpointer data[2] = { GUINT_TO_POINTER (context_handle), trans };
      
      g_return_if_fail (BSE_CONTAINER_GET_CLASS (container)->forall_items != NULL); /* paranoid */
      
      BSE_CONTAINER_GET_CLASS (container)->forall_items (container, forall_context_dismiss, data);
    }
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_dismiss (source, context_handle, trans);
}

static gboolean
forall_reset (BseItem *item,
	      gpointer data)
{
  if (BSE_IS_SOURCE (item))
    {
      g_return_val_if_fail (BSE_SOURCE_PREPARED (item), TRUE);
      
      bse_source_reset (BSE_SOURCE (item));
    }
  
  return TRUE;
}

static void
bse_container_reset (BseSource *source)
{
  BseContainer *container = BSE_CONTAINER (source);
  
  /* make sure all BseSource children are reset first */
  if (container->n_items)
    {
      g_return_if_fail (BSE_CONTAINER_GET_CLASS (container)->forall_items != NULL); /* paranoid */
      
      BSE_CONTAINER_GET_CLASS (container)->forall_items (container, forall_reset, NULL);
    }
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}
