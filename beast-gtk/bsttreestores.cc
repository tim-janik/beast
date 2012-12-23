/* BEAST - Better Audio System
 * Copyright (C) 2003 Tim Janik
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
#include "bsttreestores.hh"
#include "topconfig.h"
#include <string.h>

typedef void (*ListenerFunc) (GtkTreeModel *model, SfiProxy item, gboolean added);

/* --- functions --- */
static gboolean
file_store_idle_handler (gpointer data)
{
  GtkTreeModel *model = (GtkTreeModel*) data;
  GtkTreeStore *store = GTK_TREE_STORE (model);
  SfiFileCrawler *crawler = (SfiFileCrawler*) g_object_get_data ((GObject*) store, "file-crawler");
  gchar *file;
  gboolean busy = TRUE;
  guint n_rows = g_object_get_long (store, "n_rows");
  guint n_completed = g_object_get_long (store, "n_completed");
  GDK_THREADS_ENTER ();
  if (crawler)
    {
      SfiTime start = sfi_time_system (), now;
      /* read files */
      do
        {
          sfi_file_crawler_crawl (crawler);
          now = sfi_time_system ();
        }
      while (start <= now && /* catch time warps */
             start + 15 * 1000 > now);
      /* show files */
      file = sfi_file_crawler_pop (crawler);
      while (file)
        {
          GtkTreeIter iter;
          if (!g_utf8_validate (file, -1, NULL))
            {
              gchar *tmp = gxk_filename_to_utf8 (file);
              g_free (file);
              file = g_strconcat ("[Non-UTF8] ", tmp, NULL);
              g_free (tmp);
            }
          const gchar *dsep = strrchr (file, G_DIR_SEPARATOR);
          gtk_tree_store_append (store, &iter, NULL);
          gtk_tree_store_set (store, &iter,
                              BST_FILE_STORE_COL_ID, ++n_rows,
                              BST_FILE_STORE_COL_BASE_NAME, dsep ? dsep + 1 : file,
                              BST_FILE_STORE_COL_WAVE_NAME, dsep ? dsep + 1 : file,     /* fallback wave name */
                              BST_FILE_STORE_COL_FILE, file,
                              -1);
          g_free (file);
          /* gtk_tree_view_scroll_to_point (self->tview, 0, 2147483647); */
          file = sfi_file_crawler_pop (crawler);
        }
      g_object_set_long (store, "n_rows", n_rows);
      if (!sfi_file_crawler_needs_crawl (crawler))
        g_object_set_data ((GObject*) store, "file-crawler", NULL);
    }
  else if (n_completed < n_rows)
    {
      GtkTreePath *path = gtk_tree_path_new_from_indices (n_completed, -1);
      const gchar *filename = NULL;
      GtkTreeIter iter;
      BseSampleFileInfo *info;
      const gchar *loader, *name = NULL;
      gchar *nstr = NULL;
      gtk_tree_model_get_iter (model, &iter, path);
      gtk_tree_path_free (path);
      gtk_tree_model_get (model, &iter,
                          BST_FILE_STORE_COL_FILE, &filename,
                          -1);
      info = bse_sample_file_info (filename);
      const gchar *dsep = strrchr (filename, G_DIR_SEPARATOR);
      if (info->error == BSE_ERROR_FILE_IS_DIR)
        {
          loader = "Directory";
          name = dsep ? dsep + 1 : filename;    /* fallback wave name */
        }
      else
        {
          name = dsep ? dsep + 1 : filename;    /* fallback wave name */
          loader = info->error ? bse_error_blurb (BseErrorType (info->error)) : info->loader;
          guint l = strlen (filename);
          if (info->error == BSE_ERROR_FORMAT_UNKNOWN &&
              l >= 4 && g_strcasecmp (filename + l - 4, ".bse") == 0)
            {
              nstr = bst_file_scan_find_key (filename, "container-child", NULL);
              if (nstr)
                {
                  const gchar *col = strrchr (nstr, ':');
                  if (col > nstr && col[-1] == ':')
                    {
                      name = col + 1;
                      loader = "BSE Project";
                      info->error = 0;
                    }
                }
            }
        }
      if (info->waves->n_strings)
        name = info->waves->strings[0];
      gchar *tstr = sfi_time_to_string (info->mtime);
      gtk_tree_store_set (store, &iter,
                          BST_FILE_STORE_COL_WAVE_NAME, name,   /* real wave name */
                          BST_FILE_STORE_COL_SIZE, (guint) info->size,
                          BST_FILE_STORE_COL_TIME_USECS, info->mtime,
                          BST_FILE_STORE_COL_TIME_STR, tstr,
                          BST_FILE_STORE_COL_LOADER, loader,
                          BST_FILE_STORE_COL_LOADABLE, info->error == 0 && info->loader,
                          -1);
      g_free (tstr);
      g_free (nstr);
      g_object_set_long (store, "n_completed", n_completed + 1);
    }
  else
    busy = FALSE;
  if (!busy)
    g_object_set_long (store, "timer", 0);
  GDK_THREADS_LEAVE ();
  return busy;
}

GtkTreeModel*
bst_file_store_create (void)
{
  GtkTreeStore *sample_store = gtk_tree_store_new (BST_FILE_STORE_N_COLS,
                                                   G_TYPE_UINT,         // BST_FILE_STORE_COL_ID
                                                   G_TYPE_STRING,       // BST_FILE_STORE_COL_FILE
                                                   G_TYPE_STRING,       // BST_FILE_STORE_COL_BASE_NAME
                                                   G_TYPE_STRING,       // BST_FILE_STORE_COL_WAVE_NAME
                                                   G_TYPE_UINT,         // BST_FILE_STORE_COL_SIZE
                                                   SFI_TYPE_NUM,        // BST_FILE_STORE_COL_TIME_USECS
                                                   G_TYPE_STRING,       // BST_FILE_STORE_COL_TIME_STR
                                                   G_TYPE_STRING,       // BST_FILE_STORE_COL_LOADER
                                                   G_TYPE_BOOLEAN,      // BST_FILE_STORE_COL_LOADABLE
                                                   -1);
  return (GtkTreeModel*) sample_store;
}

void
bst_file_store_update_list (GtkTreeModel *model,
                            const gchar  *search_path,
                            const gchar  *filter)
{
  GtkTreeStore *store = GTK_TREE_STORE (model);
  SfiFileCrawler *crawler = sfi_file_crawler_new ();
  glong l;
  
  g_return_if_fail (search_path != NULL);
  
  sfi_file_crawler_add_search_path (crawler, search_path, filter);
  g_object_set_data_full ((GObject*) store, "file-crawler", crawler, (GDestroyNotify) sfi_file_crawler_destroy);
  l = g_timeout_add_full (G_PRIORITY_LOW + 100, 0,
                          file_store_idle_handler,
                          store, NULL);
  g_object_set_long (store, "timer", l);
}

void
bst_file_store_forget_list (GtkTreeModel *model)
{
  GtkTreeStore *store = GTK_TREE_STORE (model);
  glong l;
  g_object_set_long (model, "n_rows", 0);
  g_object_set_long (model, "n_completed", 0);
  g_object_set_data ((GObject*) model, "file-crawler", NULL);
  g_object_set_data ((GObject*) model, "proxy-seq", NULL);
  l = g_object_get_long (model, "timer");
  if (l)
    {
      g_source_remove (l);
      g_object_set_long (model, "timer", 0);
    }
  gtk_tree_store_clear (store);
}

void
bst_file_store_destroy (GtkTreeModel *model)
{
  bst_file_store_forget_list (model);
  g_object_unref (model);
}


/* --- child list wrapper --- */
typedef struct _ProxyStore ProxyStore;
struct _ProxyStore
{
  GxkListWrapper *self;
  gint (*row_from_proxy) (ProxyStore *ps, SfiProxy proxy);
  union {
    struct {
      SfiUPool  *ipool;
      SfiProxy   container;
      gchar     *child_type;
    } cl;       /* child list specific */
    struct {
      SfiRing *items;
    } pq;       /* proxy sequence specific */
  } u;
};

static void
proxy_store_item_property_notify (SfiProxy     item,
                                  const gchar *property_name,
                                  ProxyStore  *ps)
{
  gint row = ps->row_from_proxy (ps, item);
  if (row >= 0) /* the item can be removed already */
    gxk_list_wrapper_notify_change (ps->self, row);
}

static gint
proxy_store_item_listen_on (ProxyStore *ps,
                            SfiProxy    item)
{
  gint row = ps->row_from_proxy (ps, item);
  bse_proxy_connect (item,
                     "signal::property-notify::seqid", proxy_store_item_property_notify, ps,
                     "signal::property-notify::uname", proxy_store_item_property_notify, ps,
                     "signal::property-notify::blurb", proxy_store_item_property_notify, ps,
                     NULL);
  gxk_list_wrapper_notify_insert (ps->self, row);
  return row;
}

static void
proxy_store_item_unlisten_on (ProxyStore *ps,
                              SfiProxy    item,
                              gint        row)
{
  bse_proxy_disconnect (item,
                        "any_signal", proxy_store_item_property_notify, ps,
                        NULL);
  if (row >= 0) /* special case ipool foreach destroy */
    gxk_list_wrapper_notify_delete (ps->self, row);
}

static gboolean
proxy_store_get_iter (ProxyStore  *ps,
                      GtkTreeIter *iter,
                      SfiProxy     item)
{
  gboolean isset = FALSE;
  gint row = ps->row_from_proxy (ps, item);
  GtkTreePath *path = gtk_tree_path_new_from_indices (row, -1);
  isset = row >= 0 && gtk_tree_model_get_iter (GTK_TREE_MODEL (ps->self), iter, path);
  gtk_tree_path_free (path);
  return isset;
}

static void
child_list_wrapper_release_container (SfiProxy    container,
                                      ProxyStore *ps)
{
  g_object_set_data ((GObject*) ps->self, "ProxyStore", NULL);
}

static void
child_list_wrapper_item_added (SfiProxy    container,
                               SfiProxy    item,
                               ProxyStore *ps)
{
  if (BSE_IS_ITEM (item) && bse_proxy_is_a (item, ps->u.cl.child_type))
    {
      ListenerFunc listener = (ListenerFunc) g_object_get_data ((GObject*) ps->self, "listener");
      sfi_upool_set (ps->u.cl.ipool, item);
      proxy_store_item_listen_on (ps, item);
      if (listener)
        listener (GTK_TREE_MODEL (ps->self), item, TRUE);
    }
}

static void
child_list_wrapper_item_removed (SfiProxy    container,
                                 SfiProxy    item,
                                 gint        seqid,
                                 ProxyStore *ps)
{
  if (BSE_IS_ITEM (item) && bse_proxy_is_a (item, ps->u.cl.child_type))
    {
      ListenerFunc listener = (ListenerFunc) g_object_get_data ((GObject*) ps->self, "listener");
      gint row = seqid - 1;
      proxy_store_item_unlisten_on (ps, item, row);
      if (row >= 0)     /* special case ipool foreach destroy */
        sfi_upool_unset (ps->u.cl.ipool, item);
      if (listener)
        listener (GTK_TREE_MODEL (ps->self), item, FALSE);
    }
}

static gint
child_list_wrapper_row_from_proxy (ProxyStore *ps,
                                   SfiProxy    proxy)
{
  return bse_item_get_seqid (proxy) - 1;
}

static gboolean
child_list_wrapper_foreach (gpointer data,
                            gulong   unique_id)
{
  ProxyStore *ps = (ProxyStore*) data;
  child_list_wrapper_item_removed (0, unique_id, 0, ps);
  return TRUE;
}

static void
child_list_wrapper_destroy_data (gpointer data)
{
  ProxyStore *ps = (ProxyStore*) data;
  bse_proxy_disconnect (ps->u.cl.container,
                        "any_signal", child_list_wrapper_release_container, ps,
                        "any_signal", child_list_wrapper_item_added, ps,
                        "any_signal", child_list_wrapper_item_removed, ps,
                        NULL);
  sfi_upool_foreach (ps->u.cl.ipool, child_list_wrapper_foreach, ps);
  sfi_upool_destroy (ps->u.cl.ipool);
  gxk_list_wrapper_notify_clear (ps->self);
  g_free (ps->u.cl.child_type);
  g_free (ps);
}

void
bst_child_list_wrapper_setup (GxkListWrapper *self,
                              SfiProxy        parent,
                              const gchar    *child_type)
{
  g_object_set_data ((GObject*) self, "ProxyStore", NULL);
  if (parent)
    {
      ProxyStore *ps = g_new0 (ProxyStore, 1);
      BseItemSeq *iseq;
      guint i;
      ps->self = self;
      ps->row_from_proxy = child_list_wrapper_row_from_proxy;
      ps->u.cl.ipool = sfi_upool_new ();
      ps->u.cl.container = parent;
      ps->u.cl.child_type = g_strdup (child_type ? child_type : "BseItem");
      bse_proxy_connect (ps->u.cl.container,
                         "signal::release", child_list_wrapper_release_container, ps,
                         "signal::item_added", child_list_wrapper_item_added, ps,
                         "signal::item_remove", child_list_wrapper_item_removed, ps,
                         NULL);
      iseq = bse_container_list_children (ps->u.cl.container);
      for (i = 0; i < iseq->n_items; i++)
        child_list_wrapper_item_added (ps->u.cl.container, iseq->items[i], ps);
      g_object_set_data_full ((GObject*) self, "ProxyStore", ps, child_list_wrapper_destroy_data);
    }
}

static void
child_list_wrapper_rebuild_with_listener (GxkListWrapper *self,
                                          gpointer        listener,
                                          gboolean        set_listener)
{
  ProxyStore *ps = (ProxyStore*) g_object_get_data ((GObject*) self, "ProxyStore");
  SfiProxy parent = ps ? ps->u.cl.container : 0;
  gchar *child_type = ps ? g_strdup (ps->u.cl.child_type) : NULL;
  /* clear list with old listener */
  g_object_set_data ((GObject*) self, "ProxyStore", NULL);
  /* rebuild with new listener */
  if (set_listener)
    g_object_set_data ((GObject*) self, "listener", listener);
  bst_child_list_wrapper_setup (self, parent, child_type);
  g_free (child_type);
}

void
bst_child_list_wrapper_set_listener (GxkListWrapper *self,
                                     ListenerFunc   listener)
{
  child_list_wrapper_rebuild_with_listener (self, (void*) listener, TRUE);
}

void
bst_child_list_wrapper_rebuild (GxkListWrapper *self)
{
  child_list_wrapper_rebuild_with_listener (self, NULL, FALSE);
}

SfiProxy
bst_child_list_wrapper_get_from_iter (GxkListWrapper *self,
                                      GtkTreeIter    *iter)
{
  gint row = gxk_list_wrapper_get_index (self, iter);
  return bst_child_list_wrapper_get_proxy (self, row);
}

SfiProxy
bst_child_list_wrapper_get_proxy (GxkListWrapper *self,
                                  gint            row)
{
  ProxyStore *ps = (ProxyStore*) g_object_get_data ((GObject*) self, "ProxyStore");
  if (ps && row >= 0)
    {
      guint seqid = row + 1;
      return bse_container_get_item (ps->u.cl.container, ps->u.cl.child_type, seqid);
    }
  return 0;
}

gboolean
bst_child_list_wrapper_get_iter (GxkListWrapper *self,
                                 GtkTreeIter    *iter,
                                 SfiProxy        proxy)
{
  ProxyStore *ps = (ProxyStore*) g_object_get_data ((GObject*) self, "ProxyStore");
  return ps ? proxy_store_get_iter (ps, iter, proxy) : FALSE;
}

void
bst_child_list_wrapper_proxy_changed (GxkListWrapper *self,
                                      SfiProxy        item)
{
  ProxyStore *ps = (ProxyStore*) g_object_get_data ((GObject*) self, "ProxyStore");
  gint row = ps->row_from_proxy (ps, item);
  if (row >= 0)
    gxk_list_wrapper_notify_change (ps->self, row);
}


/* --- proxy stores --- */
static void
child_list_wrapper_fill_value (GxkListWrapper *self,
                               guint           column,
                               guint           row,
                               GValue         *value)
{
  guint seqid = row + 1;
  switch (column)
    {
      const gchar *string;
      SfiProxy item;
    case BST_PROXY_STORE_SEQID:
      g_value_set_string_take_ownership (value, g_strdup_printf ("%03u", seqid));
      break;
    case BST_PROXY_STORE_NAME:
      item = bst_child_list_wrapper_get_proxy (self, row);
      g_value_set_string (value, bse_item_get_name (item));
      break;
    case BST_PROXY_STORE_BLURB:
      item = bst_child_list_wrapper_get_proxy (self, row);
      bse_proxy_get (item, "blurb", &string, NULL);
      g_value_set_string (value, string ? string : "");
      break;
    case BST_PROXY_STORE_TYPE:
      item = bst_child_list_wrapper_get_proxy (self, row);
      g_value_set_string (value, bse_item_get_type (item));
      break;
    }
}

GxkListWrapper*
bst_child_list_wrapper_store_new (void)
{
  GxkListWrapper *self;
  self = gxk_list_wrapper_new (BST_PROXY_STORE_N_COLS,
                               G_TYPE_STRING,   // BST_PROXY_STORE_SEQID
                               G_TYPE_STRING,   // BST_PROXY_STORE_NAME
                               G_TYPE_STRING,   // BST_PROXY_STORE_BLURB
                               G_TYPE_STRING,   // BST_PROXY_STORE_TYPE
                               -1);
  g_signal_connect_object (self, "fill-value",
                           G_CALLBACK (child_list_wrapper_fill_value),
                           self, G_CONNECT_SWAPPED);
  return self;
}

static void
item_seq_store_fill_value (GxkListWrapper *self,
                           guint           column,
                           guint           row,
                           GValue         *value)
{
  GtkTreeModel *model = GTK_TREE_MODEL (self);
  switch (column)
    {
      const gchar *string;
      SfiProxy item;
    case BST_PROXY_STORE_SEQID:
      item = bst_item_seq_store_get_proxy (model, row);
      g_value_set_string_take_ownership (value, g_strdup_printf ("%03u", bse_item_get_seqid (item)));
      break;
    case BST_PROXY_STORE_NAME:
      item = bst_item_seq_store_get_proxy (model, row);
      g_value_set_string (value, bse_item_get_name (item));
      break;
    case BST_PROXY_STORE_BLURB:
      item = bst_item_seq_store_get_proxy (model, row);
      bse_proxy_get (item, "blurb", &string, NULL);
      g_value_set_string (value, string ? string : "");
      break;
    case BST_PROXY_STORE_TYPE:
      item = bst_item_seq_store_get_proxy (model, row);
      g_value_set_string (value, bse_item_get_type (item));
      break;
    }
}

GtkTreeModel*
bst_item_seq_store_new (gboolean sorted)
{
  GxkListWrapper *self;
  self = gxk_list_wrapper_new (BST_PROXY_STORE_N_COLS,
                               G_TYPE_STRING,   // BST_PROXY_STORE_SEQID
                               G_TYPE_STRING,   // BST_PROXY_STORE_NAME
                               G_TYPE_STRING,   // BST_PROXY_STORE_BLURB
                               G_TYPE_STRING,   // BST_PROXY_STORE_TYPE
                               -1);
  g_signal_connect_object (self, "fill-value",
                           G_CALLBACK (item_seq_store_fill_value),
                           self, G_CONNECT_SWAPPED);
  g_object_set_long (self, "sorted-proxies", sorted != FALSE);
  return GTK_TREE_MODEL (self);
}

static inline SfiProxy
get_proxy (const SfiRing *ring)
{
  return (SfiProxy) (ring ? ring->data : NULL);
}

static gint
item_seq_store_row_from_proxy (ProxyStore *ps,
                               SfiProxy    proxy)
{
  return sfi_ring_index (ps->u.pq.items, (gpointer) proxy);
}

static void
item_seq_store_destroy_data (gpointer data)
{
  ProxyStore *ps = (ProxyStore*) data;
  gxk_list_wrapper_notify_clear (ps->self);
  SfiRing *ring;
  for (ring = ps->u.pq.items; ring; ring = sfi_ring_walk (ring, ps->u.pq.items))
    proxy_store_item_unlisten_on (ps, get_proxy (ring), -1);
  sfi_ring_free (ps->u.pq.items);
  g_free (ps);
}

static gint
proxy_cmp_sorted (gconstpointer   value1,
                  gconstpointer   value2,
                  gpointer        data)
{
  SfiProxy p1 = (SfiProxy) value1;
  SfiProxy p2 = (SfiProxy) value2;
  if (!p1 || !p2)
    return p2 ? -1 : p1 != 0;
  const gchar *s1 = bse_item_get_type (p1);
  const gchar *s2 = bse_item_get_type (p2);
  if (!s1 || !s2)
    return s2 ? -1 : s1 != 0;
  gint cmp = strcmp (s1, s2);
  if (cmp)
    return cmp;
  s1 = bse_item_get_name (p1);
  s2 = bse_item_get_name (p2);
  if (!s1 || !s2)
    return s2 ? -1 : s1 != 0;
  return strcmp (s1, s2);
}

void
bst_item_seq_store_set (GtkTreeModel   *model,
                        BseItemSeq     *iseq)
{
  g_object_set_data ((GObject*) model, "ProxyStore", NULL);
  if (iseq)
    {
      ProxyStore *ps = g_new0 (ProxyStore, 1);
      ps->self = GXK_LIST_WRAPPER (model);
      ps->row_from_proxy = item_seq_store_row_from_proxy;
      guint i;
      for (i = 0; i < iseq->n_items; i++)
        ps->u.pq.items = sfi_ring_append (ps->u.pq.items, (gpointer) iseq->items[i]);
      if (g_object_get_long (model, "sorted-proxies"))
        ps->u.pq.items = sfi_ring_sort (ps->u.pq.items, proxy_cmp_sorted, NULL);
      SfiRing *ring;
      for (ring = ps->u.pq.items; ring; ring = sfi_ring_walk (ring, ps->u.pq.items))
        proxy_store_item_listen_on (ps, get_proxy (ring));
      g_object_set_data_full ((GObject*) model, "ProxyStore", ps, item_seq_store_destroy_data);
    }
}

gint
bst_item_seq_store_add (GtkTreeModel   *model,
                        SfiProxy        proxy)
{
  ProxyStore *ps = (ProxyStore*) g_object_get_data ((GObject*) model, "ProxyStore");
  if (g_object_get_long (model, "sorted-proxies"))
    ps->u.pq.items = sfi_ring_insert_sorted (ps->u.pq.items, (gpointer) proxy, proxy_cmp_sorted, NULL);
  else
    ps->u.pq.items = sfi_ring_append (ps->u.pq.items, (gpointer) proxy);
  gint row = proxy_store_item_listen_on (ps, proxy);
  return row;
}

gint
bst_item_seq_store_remove (GtkTreeModel   *model,
                           SfiProxy        proxy)
{
  ProxyStore *ps = (ProxyStore*) g_object_get_data ((GObject*) model, "ProxyStore");
  gint row = item_seq_store_row_from_proxy (ps, proxy);
  if (row >= 0)
    {
      ps->u.pq.items = sfi_ring_remove (ps->u.pq.items, (gpointer) proxy);
      proxy_store_item_unlisten_on (ps, proxy, row);
    }
  return row;
}

gboolean
bst_item_seq_store_can_raise (GtkTreeModel   *model,
                              SfiProxy        proxy)
{
  ProxyStore *ps = (ProxyStore*) g_object_get_data ((GObject*) model, "ProxyStore");
  gint row = item_seq_store_row_from_proxy (ps, proxy);
  return row > 0;
}

gint
bst_item_seq_store_raise (GtkTreeModel   *model,
                          SfiProxy        proxy)
{
  ProxyStore *ps = (ProxyStore*) g_object_get_data ((GObject*) model, "ProxyStore");
  gint row = item_seq_store_row_from_proxy (ps, proxy);
  if (row > 0)
    {
      ps->u.pq.items = sfi_ring_remove (ps->u.pq.items, (gpointer) proxy);
      proxy_store_item_unlisten_on (ps, proxy, row);
      row -= 1;
      ps->u.pq.items = sfi_ring_insert (ps->u.pq.items, (gpointer) proxy, row);
      proxy_store_item_listen_on (ps, proxy);
    }
  return row;
}

gboolean
bst_item_seq_store_can_lower (GtkTreeModel   *model,
                              SfiProxy        proxy)
{
  ProxyStore *ps = (ProxyStore*) g_object_get_data ((GObject*) model, "ProxyStore");
  gint row = item_seq_store_row_from_proxy (ps, proxy);
  return row >= 0 && sfi_ring_tail (ps->u.pq.items)->data != (gpointer) proxy;
}

gint
bst_item_seq_store_lower (GtkTreeModel   *model,
                          SfiProxy        proxy)
{
  ProxyStore *ps = (ProxyStore*) g_object_get_data ((GObject*) model, "ProxyStore");
  gint row = item_seq_store_row_from_proxy (ps, proxy);
  if (row >= 0 && sfi_ring_tail (ps->u.pq.items)->data != (gpointer) proxy)
    {
      ps->u.pq.items = sfi_ring_remove (ps->u.pq.items, (gpointer) proxy);
      proxy_store_item_unlisten_on (ps, proxy, row);
      row += 1;
      ps->u.pq.items = sfi_ring_insert (ps->u.pq.items, (gpointer) proxy, row);
      proxy_store_item_listen_on (ps, proxy);
    }
  return row;
}

BseItemSeq*
bst_item_seq_store_dup (GtkTreeModel   *model)
{
  ProxyStore *ps = (ProxyStore*) g_object_get_data ((GObject*) model, "ProxyStore");
  BseItemSeq *iseq = bse_item_seq_new ();
  SfiRing *ring;
  for (ring = ps->u.pq.items; ring; ring = sfi_ring_walk (ring, ps->u.pq.items))
    bse_item_seq_append (iseq, (SfiProxy) ring->data);
  return iseq;
}

SfiProxy
bst_item_seq_store_get_from_iter (GtkTreeModel *model,
                                  GtkTreeIter  *iter)
{
  GxkListWrapper *self = GXK_LIST_WRAPPER (model);
  gint row = gxk_list_wrapper_get_index (self, iter);
  return bst_item_seq_store_get_proxy (model, row);
}

SfiProxy
bst_item_seq_store_get_proxy (GtkTreeModel *model,
                              gint          row)
{
  ProxyStore *ps = (ProxyStore*) g_object_get_data ((GObject*) model, "ProxyStore");
  if (ps && row >= 0)
    return get_proxy (sfi_ring_nth (ps->u.pq.items, row));
  return 0;
}

gboolean
bst_item_seq_store_get_iter (GtkTreeModel *model,
                             GtkTreeIter  *iter,
                             SfiProxy      proxy)
{
  ProxyStore *ps = (ProxyStore*) g_object_get_data ((GObject*) model, "ProxyStore");
  return ps ? proxy_store_get_iter (ps, iter, proxy) : FALSE;
}
