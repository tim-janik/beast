/* BSE - Better Sound Engine
 * Copyright (C) 1998-1999, 2000-2003 Tim Janik
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
#include "bsecontainer.h"
#include "bseundostack.h"
#include "bsesource.h"
#include "bseproject.h"
#include "bsestorage.h"
#include "bsemain.h"
#include "bseengine.h"
#include <stdlib.h>
#include <string.h>


#define parse_or_return         bse_storage_scanner_parse_or_return
#define peek_or_return          bse_storage_scanner_peek_or_return


enum
{
  SIGNAL_ITEM_ADDED,
  SIGNAL_ITEM_REMOVE,
  SIGNAL_LAST
};
typedef struct _UncrossNode UncrossNode;


/* --- prototypes --- */
static void         bse_container_class_init            (BseContainerClass      *class);
static void         bse_container_class_finalize        (BseContainerClass      *class);
static void         bse_container_init                  (BseContainer           *container);
static void         bse_container_dispose               (GObject                *object);
static void         bse_container_finalize              (GObject                *object);
static void         bse_container_do_add_item           (BseContainer           *container,
                                                         BseItem                *item);
static void         bse_container_do_remove_item        (BseContainer           *container,
                                                         BseItem                *item);
static BseItem*     bse_container_real_retrieve_child   (BseContainer           *container,
                                                         GType                   child_type,
                                                         const gchar            *uname);
static void         bse_container_prepare               (BseSource              *source);
static void         bse_container_context_create        (BseSource              *source,
                                                         guint                    context_handle,
                                                         BseTrans               *trans);
static void         bse_container_context_connect       (BseSource              *source,
                                                         guint                   context_handle,
                                                         BseTrans               *trans);
static void         bse_container_context_dismiss       (BseSource              *source,
                                                         guint                   context_handle,
                                                         BseTrans               *trans);
static void         bse_container_reset                 (BseSource              *source);
static GSList*      container_context_children          (BseContainer           *container);
static void         container_release_children          (BseContainer           *container);
static gboolean     container_default_check_restore     (BseContainer           *self,
                                                         const gchar            *child_type);
static void         bse_container_uncross_descendant	(BseContainer           *container,
                                                         BseItem                *item);


/* --- variables --- */
static GTypeClass  *parent_class = NULL;
static GQuark       quark_cross_links = 0;
static GSList      *containers_cross_changes = NULL;
static guint        containers_cross_changes_handler = 0;
static guint        container_signals[SIGNAL_LAST] = { 0, };
static UncrossNode *uncross_stack = NULL;


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
  
  return bse_type_register_abstract (BSE_TYPE_SOURCE,
                                     "BseContainer",
                                     "Base type to manage BSE items",
                                     __FILE__, __LINE__,
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
  
  quark_cross_links = g_quark_from_static_string ("BseContainerCrossLinks");
  
  gobject_class->dispose = bse_container_dispose;
  gobject_class->finalize = bse_container_finalize;
  
  source_class->prepare = bse_container_prepare;
  source_class->context_create = bse_container_context_create;
  source_class->context_connect = bse_container_context_connect;
  source_class->context_dismiss = bse_container_context_dismiss;
  source_class->reset = bse_container_reset;
  
  class->add_item = bse_container_do_add_item;
  class->remove_item = bse_container_do_remove_item;
  class->forall_items = NULL;
  class->check_restore = container_default_check_restore;
  class->retrieve_child = bse_container_real_retrieve_child;
  class->context_children = container_context_children;
  class->release_children = container_release_children;
  
  container_signals[SIGNAL_ITEM_ADDED] = bse_object_class_add_signal (object_class, "item_added",
                                                                      G_TYPE_NONE, 1, BSE_TYPE_ITEM);
  container_signals[SIGNAL_ITEM_REMOVE] = bse_object_class_add_signal (object_class, "item_remove",
                                                                       G_TYPE_NONE, 2, BSE_TYPE_ITEM,
                                                                       SFI_TYPE_INT);
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
      
      /* remove any existing cross-links (with notification) */
      g_object_set_qdata (container, quark_cross_links, NULL);
    }
  
  /* chain parent class' dispose handler */
  G_OBJECT_CLASS (parent_class)->dispose (gobject);
}

static void
bse_container_finalize (GObject *gobject)
{
  BseContainer *container = BSE_CONTAINER (gobject);
  
  if (container->n_items)
    g_warning ("%s: finalize handlers missed to remove %u items from %s",
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
                           BseItem      *item)
{
  g_object_ref (item);
  container->n_items += 1;
  bse_item_set_parent (item, BSE_ITEM (container));
  
  if (BSE_IS_SOURCE (item) && BSE_SOURCE_PREPARED (container))
    {
      BseTrans *trans = bse_trans_open ();
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
      bse_trans_commit (trans);
    }
}

void
bse_container_add_item (BseContainer *container,
                        BseItem      *item)
{
  BseUndoStack *ustack;
  gchar *uname;
  
  g_return_if_fail (BSE_IS_CONTAINER (container));
  g_return_if_fail (BSE_IS_ITEM (item));
  g_return_if_fail (item->parent == NULL);
  g_return_if_fail (BSE_CONTAINER_GET_CLASS (container)->add_item != NULL); /* paranoid */
  
  g_object_ref (container);
  g_object_ref (item);
  ustack = bse_item_undo_open (container, "add-child-noundo");
  bse_undo_stack_ignore_steps (ustack);
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
        uname = g_object_get_data (container, "BseContainer-base-name");
      if (!uname)
        {
          uname = BSE_OBJECT_TYPE_NAME (item);
          if (strncmp (uname, "BseContrib", 10) == 0 && uname[10] >= 'A' && uname[10] <= 'Z')
            uname += 10;                /* strip BseContrib namespace for convenient naming */
          else if (strncmp (uname, "Bse", 3) == 0 && uname[3] >= 'A' && uname[3] <= 'Z')
            uname += 3;                 /* strip Bse namespace for convenient naming */
        }

      l = strlen (uname);
      buffer = g_new (gchar, l + 12);
      strcpy (buffer, uname);
      p = buffer + l;
      do
        g_snprintf (p, 11, "-%u", ++i);
      while (bse_container_lookup_item (container, buffer));
      
      g_object_set (item, "uname", buffer, NULL); /* no undo */
      g_free (buffer);
    }
  g_object_set_data (container, "BseContainer-base-name", NULL);

  BSE_CONTAINER_GET_CLASS (container)->add_item (container, item);
  if (item->parent != NULL)
    g_signal_emit (container, container_signals[SIGNAL_ITEM_ADDED], 0, item);
  
  g_object_thaw_notify (G_OBJECT (item));
  g_object_thaw_notify (G_OBJECT (container));
  bse_undo_stack_unignore_steps (ustack);
  bse_item_undo_close (ustack);
  g_object_unref (item);
  g_object_unref (container);
}

gpointer
bse_container_new_child_bname (BseContainer *container,
                               GType         child_type,
                               const gchar  *base_name,
                               const gchar  *first_param_name,
                               ...)
{
  gpointer child;
  va_list var_args;
  
  g_return_val_if_fail (BSE_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (g_type_is_a (child_type, BSE_TYPE_ITEM), NULL);
  g_return_val_if_fail (!G_TYPE_IS_ABSTRACT (child_type), NULL);

  g_object_set_data_full (container, "BseContainer-base-name", g_strdup (base_name), g_free);
  va_start (var_args, first_param_name);
  child = g_object_new_valist (child_type, first_param_name, var_args);
  va_end (var_args);
  if (base_name)
    g_object_set (child, "uname", NULL, NULL); /* no undo */
  bse_container_add_item (container, child);
  g_object_unref (child);
  
  return child;
}

static void
bse_container_do_remove_item (BseContainer *container,
                              BseItem      *item)
{
  BseItem *ancestor = BSE_ITEM (container);
  
  do
    {
      bse_container_uncross_descendant (BSE_CONTAINER (ancestor), item);
      ancestor = ancestor->parent;
    }
  while (ancestor);
  
  container->n_items -= 1;
  
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
  BseUndoStack *ustack;
  guint seqid;
  
  g_return_if_fail (BSE_IS_CONTAINER (container));
  g_return_if_fail (BSE_IS_ITEM (item));
  g_return_if_fail (item->parent == BSE_ITEM (container));
  g_return_if_fail (BSE_CONTAINER_GET_CLASS (container)->remove_item != NULL); /* paranoid */
  
  finalizing_container = G_OBJECT (container)->ref_count == 0;
  if (!finalizing_container)
    g_object_ref (container);
  g_object_ref (item);
  ustack = bse_item_undo_open (container, "remove-child-noundo");
  bse_undo_stack_ignore_steps (ustack);
  
  seqid = bse_container_get_item_seqid (container, item);
  g_object_freeze_notify (G_OBJECT (container));
  g_object_freeze_notify (G_OBJECT (item));
  if (!finalizing_container)
    g_signal_emit (container, container_signals[SIGNAL_ITEM_REMOVE], 0, item, seqid);
  BSE_CONTAINER_GET_CLASS (container)->remove_item (container, item);
  g_object_thaw_notify (G_OBJECT (item));
  g_object_thaw_notify (G_OBJECT (container));
  
  bse_undo_stack_unignore_steps (ustack);
  bse_item_undo_close (ustack);
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
  BseItemSeq *iseq = data;
  
  bse_item_seq_append (iseq, item);
  
  return TRUE;
}

BseItemSeq*
bse_container_list_children (BseContainer *container)
{
  BseItemSeq *iseq;
  
  g_return_val_if_fail (BSE_IS_CONTAINER (container), NULL);
  
  iseq = bse_item_seq_new ();
  if (container->n_items)
    {
      g_return_val_if_fail (BSE_CONTAINER_GET_CLASS (container)->forall_items != NULL, NULL); /* paranoid */
      
      BSE_CONTAINER_GET_CLASS (container)->forall_items (container, list_items, iseq);
    }
  
  return iseq;
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
              gpointer data)
{
  BseStorage *storage = data;
  if (!BSE_ITEM_INTERNAL (item) &&
      bse_item_needs_storage (item, storage))
    bse_storage_store_child (storage, item);
  return TRUE;
}

void
bse_container_store_children (BseContainer *container,
                              BseStorage   *storage)
{
  g_return_if_fail (BSE_IS_CONTAINER (container));
  g_return_if_fail (BSE_IS_STORAGE (storage));
  
  g_object_ref (container);
  bse_container_forall_items (container, store_forall, storage);
  g_object_unref (container);
}

static gboolean
container_default_check_restore (BseContainer   *self,
                                 const gchar    *child_type)
{
  GType type = g_type_from_name (child_type);
  return g_type_is_a (type, BSE_TYPE_ITEM) && !G_TYPE_IS_ABSTRACT (type);
}

gboolean
bse_container_check_restore (BseContainer   *self,
                             const gchar    *child_type)
{
  g_return_val_if_fail (BSE_IS_CONTAINER (self), FALSE);
  g_return_val_if_fail (child_type != NULL, FALSE);

  return BSE_CONTAINER_GET_CLASS (self)->check_restore (self, child_type);
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
  return bse_container_new_child (container, child_type,
                                  uname ? "uname" : NULL, uname,
                                  NULL);
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
   * "BseItem"          generic item    -> create item of type BseItem
   * "BseItem::foo"     unamed item     -> create/get item of type BseItem with uname "foo"
   *
   * to get unique matches for unames, items of a container need
   * to have unique names (enforced in bse_container_add_item()
   * and bse_item_do_set_uname()).
   */
  
  type_name = g_strdup (type_uname);
  uname = strchr (type_name, ':');
  if (uname)
    {
      if (uname[1] != ':')      /* a single colon is invalid */
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
  
  /* upaths consist of colon seperated unames from the item's ancestry */
  
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
  g_return_val_if_fail (bse_item_has_ancestor (item, self_item), NULL); /* item != self_item */
  
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
  BseItem           *link;
  BseItemUncross     uncross;
  // gpointer         data;
} CrossLink;
typedef struct
{
  guint         n_cross_links;
  BseContainer *container;
  CrossLink     cross_links[1]; /* flexible array */
} BseContainerCrossLinks;
struct _UncrossNode
{
  UncrossNode   *next;
  BseContainer  *container;
  BseItem       *owner;
  BseItem       *link;
  BseItemUncross uncross;
};

static inline void
uncross_link_R (BseContainerCrossLinks *clinks,
                guint                   n,
                gboolean                notify)
{
  UncrossNode unode;
  unode.owner = clinks->cross_links[n].owner;
  unode.link = clinks->cross_links[n].link;
  unode.uncross = clinks->cross_links[n].uncross;
  
  clinks->n_cross_links--;
  if (n < clinks->n_cross_links)
    clinks->cross_links[n] = clinks->cross_links[clinks->n_cross_links];
  
  if (notify)
    {
      /* record recursion */
      unode.container = clinks->container;
      unode.next = uncross_stack;
      uncross_stack = &unode;
      unode.uncross (unode.owner, unode.link); /* may recurse */
      g_assert (uncross_stack == &unode); /* paranoid */
      uncross_stack = unode.next;
    }
}

static void
destroy_clinks (gpointer data)
{
  BseContainerCrossLinks *clinks = data;
  
  if (clinks->n_cross_links)
    container_queue_cross_changes (clinks->container);
  
  while (clinks->n_cross_links)
    uncross_link_R (clinks, clinks->n_cross_links - 1, TRUE);
  g_free (clinks);
}

static inline void
container_set_clinks (gpointer                container,
                      BseContainerCrossLinks *clinks)
{
  g_object_steal_qdata (container, quark_cross_links);
  g_object_set_qdata_full (container, quark_cross_links, clinks, destroy_clinks);
}

static inline BseContainerCrossLinks*
container_get_clinks (gpointer container)
{
  return g_object_get_qdata (container, quark_cross_links);
}

void
_bse_container_cross_link (BseContainer    *container,
                           BseItem         *owner,
                           BseItem         *link,
                           BseItemUncross   uncross)
{
  BseContainerCrossLinks *clinks;
  guint i;
  
  /* prerequisites:
   * - container == bse_item_common_ancestor (owner, link)
   * - container != owner || container != link
   * + implies: owner != link
   */
  
  g_return_if_fail (BSE_IS_CONTAINER (container));
  g_return_if_fail (BSE_IS_ITEM (owner));
  g_return_if_fail (BSE_IS_ITEM (link));
  g_return_if_fail (uncross != NULL);
  
  clinks = container_get_clinks (container);
  if (!clinks)
    {
      i = 0;
      clinks = g_realloc (clinks, sizeof (BseContainerCrossLinks));
      clinks->n_cross_links = i + 1;
      clinks->container = container;
      container_set_clinks (container, clinks);
    }
  else
    {
      BseContainerCrossLinks *old_loc = clinks;
      
      i = clinks->n_cross_links++;
      clinks = g_realloc (clinks, sizeof (BseContainerCrossLinks) + i * sizeof (clinks->cross_links[0]));
      if (old_loc != clinks)
        container_set_clinks (container, clinks);
    }
  clinks->cross_links[i].owner = owner;
  clinks->cross_links[i].link = link;
  clinks->cross_links[i].uncross = uncross;
  // clinks->cross_links[i].data = data;
  
  container_queue_cross_changes (container);
}

void
_bse_container_cross_unlink (BseContainer  *container,
                             BseItem       *owner,
                             BseItem       *link,
                             BseItemUncross uncross)
{
  UncrossNode *unode;
  gboolean found_one = FALSE;
  
  g_return_if_fail (BSE_IS_CONTAINER (container));
  g_return_if_fail (BSE_IS_ITEM (owner));
  g_return_if_fail (BSE_IS_ITEM (link));
  g_return_if_fail (uncross != NULL);
  
  g_object_ref (container);
  g_object_ref (owner);
  g_object_ref (link);
  
  /* _first_ check whether a currently uncrossing (in recursion)
   * link is to be unlinked
   */
  for (unode = uncross_stack; unode; unode = unode->next)
    if (unode->container == container &&
        unode->owner == owner &&
        unode->link == link &&
        unode->uncross == uncross)
      {
        unode->container = NULL;
        unode->owner = NULL;
        unode->link = NULL;
        unode->uncross = NULL;
        found_one = TRUE;
        break;
      }
  if (!found_one)
    {
      BseContainerCrossLinks *clinks = container_get_clinks (container);
      if (clinks)
        {
          guint i = 0;
          
          while (i < clinks->n_cross_links)
            {
              if (clinks->cross_links[i].owner == owner &&
                  clinks->cross_links[i].link == link &&
                  clinks->cross_links[i].uncross == uncross)
                {
                  uncross_link_R (clinks, i, FALSE);    /* clinks invalid */
                  container_queue_cross_changes (container);
                  found_one = TRUE;
                  break;
                }
              i++;
            }
        }
    }
  if (!found_one)
    g_warning ("no cross link from `%s' to `%s' on `%s' to remove",
               G_OBJECT_TYPE_NAME (owner),
               G_OBJECT_TYPE_NAME (link),
               G_OBJECT_TYPE_NAME (container));
  
  g_object_unref (link);
  g_object_unref (owner);
  g_object_unref (container);
}

void
_bse_container_uncross (BseContainer  *container,
                        BseItem       *owner,
                        BseItem       *link)
{
  BseContainerCrossLinks *clinks;
  
  g_return_if_fail (BSE_IS_CONTAINER (container));
  g_return_if_fail (BSE_IS_ITEM (owner));
  g_return_if_fail (BSE_IS_ITEM (link));
  
  g_object_ref (container);
  g_object_ref (owner);
  g_object_ref (link);
  
  clinks = container_get_clinks (container);
  if (clinks)
    {
      guint i = 0;
      while (i < clinks->n_cross_links)
        {
          if (clinks->cross_links[i].owner == owner &&
              clinks->cross_links[i].link == link)
            {
              uncross_link_R (clinks, i, TRUE);
              container_queue_cross_changes (container);
              clinks = container_get_clinks (container);
              i = 0;
            }
          i++;
        }
    }
  
  g_object_unref (link);
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

static void
bse_container_uncross_descendant (BseContainer *container,
                                  BseItem      *item)
{
  BseContainerCrossLinks *clinks;
  gboolean found_one = FALSE;
  
  /* prerequisites:
   * bse_item_has_ancestor (item, container) == TRUE
   */
  
  g_return_if_fail (BSE_IS_CONTAINER (container));
  g_return_if_fail (BSE_IS_ITEM (item));
  
  clinks = container_get_clinks (container);
  if (clinks)
    {
      guint i = 0;
      
      g_object_ref (container);
      g_object_ref (item);
      
      /* suppress tree walks where possible
       */
      if (!BSE_IS_CONTAINER (item) || ((BseContainer*) item)->n_items == 0)
        while (i < clinks->n_cross_links)
          {
            if (clinks->cross_links[i].owner == item || clinks->cross_links[i].link == item)
              {
                found_one = TRUE;
                uncross_link_R (clinks, i, TRUE);
                clinks = container_get_clinks (container);
                i = 0;
              }
            else
              i++;
          }
      else /* need to check whether item is ancestor of any of the cross-link items here */
        {
          BseItem *saved_parent, *citem = BSE_ITEM (container);
          
          /* we do some minor hackery here, for optimization purposes:
           * since item is a child of container, we don't need to walk
           * ->owner's or ->link's ancestor list any further than
           * up to reaching container.
           * to suppress extra checks in item_check_branch() in this
           * regard, we simply set container->parent to NULL temporarily
           * and with that cause item_check_branch() to abort automatically
           */
          saved_parent = citem->parent;
          citem->parent = NULL;
          while (i < clinks->n_cross_links)
            {
              if (item_check_branch (clinks->cross_links[i].owner, item) ||
                  item_check_branch (clinks->cross_links[i].link, item))
                {
                  citem->parent = saved_parent;
                  
                  found_one = TRUE;
                  uncross_link_R (clinks, i, TRUE);
                  clinks = container_get_clinks (container);
                  i = 0;
                  
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
bse_container_context_create (BseSource *source,
                              guint      context_handle,
                              BseTrans  *trans)
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
                               guint      context_handle,
                               BseTrans  *trans)
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
                               guint      context_handle,
                               BseTrans  *trans)
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

static void
undo_remove_child (BseUndoStep  *ustep,
                   BseUndoStack *ustack)
{
  BseItem *child = g_object_new (ustep->data[0].v_ulong,
                                 "uname", ustep->data[1].v_pointer,
                                 NULL);
  bse_container_add_item (bse_undo_pointer_unpack (ustep->data[2].v_pointer, ustack), child);
  g_object_unref (child);
}

static void
unde_free_remove_child (BseUndoStep *ustep)
{
  g_free (ustep->data[1].v_pointer);
  g_free (ustep->data[2].v_pointer);
}

void
bse_container_uncross_undoable (BseContainer *container,
                                BseItem      *child)
{
  BseItem *ancestor;

  g_return_if_fail (BSE_IS_CONTAINER (container));
  g_return_if_fail (BSE_IS_ITEM (child));
  g_return_if_fail (child->parent == (BseItem*) container);

  /* backup source channels state */
  if (BSE_IS_SOURCE (child))
    {
      bse_source_backup_ochannels_to_undo (BSE_SOURCE (child));
      bse_source_clear_ochannels (BSE_SOURCE (child));
      bse_source_backup_ichannels_to_undo (BSE_SOURCE (child));
      bse_source_clear_ichannels (BSE_SOURCE (child));
    }
  /* dispose cross references, those backup themselves */
  ancestor = BSE_ITEM (container);
  do
    {
      bse_container_uncross_descendant (BSE_CONTAINER (ancestor), child);
      ancestor = ancestor->parent;
    }
  while (ancestor);
}

void
bse_container_remove_backedup (BseContainer *container,
                               BseItem      *child,
                               BseUndoStack *ustack)
{
  g_return_if_fail (BSE_IS_CONTAINER (container));
  g_return_if_fail (BSE_IS_ITEM (child));
  g_return_if_fail (child->parent == (BseItem*) container);

  /* _no_ redo facility is queued by this function */

  /* backup item state */
  bse_item_backup_to_undo (child, ustack);
  /* make sure item is recreated upon undo */
  if (!BSE_UNDO_STACK_VOID (ustack))
    {
      BseUndoStep *ustep = bse_undo_step_new (undo_remove_child, unde_free_remove_child, 3);
      ustep->data[0].v_ulong = G_OBJECT_TYPE (child);
      ustep->data[1].v_pointer = g_strdup (BSE_OBJECT_UNAME (child));
      ustep->data[2].v_pointer = bse_undo_pointer_pack (container, ustack);
      bse_undo_stack_push (ustack, ustep);
    }

  /* check for automation properties */
  BseProject *deactivate_project = NULL;
  if (!BSE_UNDO_STACK_VOID (ustack) && BSE_IS_SOURCE (child) && BSE_SOURCE_PREPARED (child))
    {
      BseSourceClass *class = BSE_SOURCE_GET_CLASS (child);
      if (class->automation_properties)
        {
          /* automation properties can't be setup on prepared children */
          deactivate_project = bse_item_get_project (child);
        }
    }

  /* actual removal */
  bse_undo_stack_ignore_steps (ustack);
  bse_container_remove_item (container, child);
  bse_undo_stack_unignore_steps (ustack);

  /* certain things work only (can only be undone) in deactivated projects */
  if (deactivate_project)
    bse_project_push_undo_silent_deactivate (deactivate_project);
}

#include <stdio.h>

static gboolean
container_debug_tree_forall (BseItem *item,
                             gpointer data_p)
{
  gpointer *data = data_p;
  BseContainer *container = data[0];
  FILE *file = data[1];

  fprintf (file, "  %s (%s)\n", bse_container_make_upath (container, item), bse_object_debug_name (item));
  if (BSE_IS_CONTAINER (item))
    BSE_CONTAINER_GET_CLASS (item)->forall_items (BSE_CONTAINER (item), container_debug_tree_forall, data_p);
  return TRUE;
}

void
bse_container_debug_tree (BseContainer *container)
{
  FILE *file = stderr;
  gpointer data[2];

  fprintf (file, "%s:\n", bse_object_debug_name (container));
  data[0] = container;
  data[1] = file;
  if (BSE_IS_CONTAINER (container))
    BSE_CONTAINER_GET_CLASS (container)->forall_items (container, container_debug_tree_forall, data);
}
