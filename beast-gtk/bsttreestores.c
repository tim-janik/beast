/* BEAST - Bedevilled Audio System
 * Copyright (C) 2003 Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bsttreestores.h"
#include "topconfig.h"
#include <string.h>


/* --- functions --- */
static gboolean
file_store_idle_handler (gpointer data)
{
  GtkTreeModel *model = data;
  GtkTreeStore *store = GTK_TREE_STORE (model);
  SfiFileCrawler *crawler = g_object_get_data (store, "file-crawler");
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
          gchar *dsep;
          if (!g_utf8_validate (file, -1, NULL))
            {
              gchar *tmp = gxk_filename_to_utf8 (file);
              g_free (file);
              file = g_strconcat ("[Non-UTF8] ", tmp, NULL);
              g_free (tmp);
            }
          dsep = strrchr (file, G_DIR_SEPARATOR);
          gtk_tree_store_append (store, &iter, NULL);
          gtk_tree_store_set (store, &iter,
                              BST_FILE_STORE_COL_ID, ++n_rows,
                              BST_FILE_STORE_COL_BASE_NAME, dsep ? dsep + 1 : file,
                              BST_FILE_STORE_COL_FILE, file,
                              -1);
          g_free (file);
          /* gtk_tree_view_scroll_to_point (self->tview, 0, 2147483647); */
          file = sfi_file_crawler_pop (crawler);
        }
      g_object_set_long (store, "n_rows", n_rows);
      if (!sfi_file_crawler_needs_crawl (crawler))
        g_object_set_data (store, "file-crawler", NULL);
    }
  else if (n_completed < n_rows)
    {
      GtkTreePath *path = gtk_tree_path_new_from_indices (n_completed, -1);
      const gchar *filename = NULL;
      GtkTreeIter iter;
      BseSampleFileInfo *info;
      const gchar *loader, *name = NULL;
      gchar *tstr;
      gtk_tree_model_get_iter (model, &iter, path);
      gtk_tree_path_free (path);
      gtk_tree_model_get (model, &iter,
                          BST_FILE_STORE_COL_FILE, &filename,
                          -1);
      info = bse_sample_file_info (filename);
      if (info->error == BSE_ERROR_FILE_IS_DIR)
        loader = "Directory";
      else
        loader = info->error ? bse_error_blurb (info->error) : info->loader;
      if (info->waves->n_strings)
        name = info->waves->strings[0];
      tstr = sfi_time_to_string (info->mtime);
      gtk_tree_store_set (store, &iter,
                          BST_FILE_STORE_COL_WAVE_NAME, name,
                          BST_FILE_STORE_COL_SIZE, (guint) info->size,
                          BST_FILE_STORE_COL_TIME_USECS, info->mtime,
                          BST_FILE_STORE_COL_TIME_STR, tstr,
                          BST_FILE_STORE_COL_LOADER, loader,
                          BST_FILE_STORE_COL_LOADABLE, info->error == 0 && info->loader,
                          -1);
      g_free (tstr);
      g_object_set_long (store, "n_completed", n_completed + 1);
    }
  else
    busy = FALSE;
  if (!busy)
    g_object_set_long (store, "timer", 0);
  GDK_THREADS_LEAVE ();
  return busy;
}

static void
file_store_rebuild (GtkTreeModel *model)
{
  GtkTreeStore *store = GTK_TREE_STORE (model);
  const gchar *path = g_object_get_data (store, "search-path");
  SfiFileCrawler *crawler = sfi_file_crawler_new ();
  glong l;

  sfi_file_crawler_add_search_path (crawler, path);
  g_object_set_data_full (store, "file-crawler", crawler, sfi_file_crawler_destroy);
  l = g_timeout_add_full (G_PRIORITY_LOW + 100, 0,
                          file_store_idle_handler,
                          store, NULL);
  g_object_set_long (store, "timer", l);
}

static void
file_store_clear (GtkTreeModel *model)
{
  GtkTreeStore *store = GTK_TREE_STORE (model);
  glong l;
  g_object_set_long (model, "n_rows", 0);
  g_object_set_long (model, "n_completed", 0);
  g_object_set_data (model, "file-crawler", NULL);
  g_object_set_data (model, "proxy-seq", NULL);
  l = g_object_get_long (model, "timer");
  if (l)
    {
      g_source_remove (l);
      g_object_set_long (model, "timer", 0);
    }
  gtk_tree_store_clear (store);
}

GtkTreeModel*
bst_file_store_get_sample_list (void)
{
  static GtkTreeStore *sample_store = NULL;
  if (!sample_store)
    {
      const gchar *s = g_getenv ("BEAST_SAMPLE_PATH");
      gchar *path = g_strconcat (BST_PATH_DATA_SAMPLES, s ? G_SEARCHPATH_SEPARATOR_S : NULL, s, NULL);
      sample_store = gtk_tree_store_new (BST_FILE_STORE_N_COLS,
                                         G_TYPE_UINT,   // BST_FILE_STORE_COL_ID
                                         G_TYPE_STRING, // BST_FILE_STORE_COL_FILE
                                         G_TYPE_STRING, // BST_FILE_STORE_COL_BASE_NAME
                                         G_TYPE_STRING, // BST_FILE_STORE_COL_WAVE_NAME
                                         G_TYPE_UINT,   // BST_FILE_STORE_COL_SIZE
                                         SFI_TYPE_NUM,  // BST_FILE_STORE_COL_TIME_USECS
                                         G_TYPE_STRING, // BST_FILE_STORE_COL_TIME_STR
                                         G_TYPE_STRING, // BST_FILE_STORE_COL_LOADER
                                         G_TYPE_BOOLEAN,        // BST_FILE_STORE_COL_LOADABLE
                                         -1);
      g_object_set_data_full (sample_store, "search-path", path, g_free);
    }
  return GTK_TREE_MODEL (sample_store);
}

static guint sample_list_use_count = 0;

void
bst_file_store_use_sample_list (void)
{
  if (!sample_list_use_count)
    file_store_rebuild (bst_file_store_get_sample_list ());
  sample_list_use_count++;
}

void
bst_file_store_unuse_sample_list (void)
{
  if (sample_list_use_count)
    {
      sample_list_use_count--;
      if (!sample_list_use_count)
        file_store_clear (bst_file_store_get_sample_list ());
    }
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
      BseProxySeq *pseq;
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

static void
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
  g_object_set_data (ps->self, "ProxyStore", NULL);
}

static void
child_list_wrapper_item_added (SfiProxy    container,
                               SfiProxy    item,
                               ProxyStore *ps)
{
  if (BSE_IS_ITEM (item) && bse_proxy_is_a (item, ps->u.cl.child_type))
    {
      void (*listener) (GtkTreeModel *model,
                        SfiProxy      item,
                        gboolean      added) = g_object_get_data (ps->self, "listener");
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
      void (*listener) (GtkTreeModel *model,
                        SfiProxy      item,
                        gboolean      added) = g_object_get_data (ps->self, "listener");
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
  ProxyStore *ps = data;
  child_list_wrapper_item_removed (0, unique_id, 0, ps);
  return TRUE;
}

static void
child_list_wrapper_destroy_data (gpointer data)
{
  ProxyStore *ps = data;
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
  g_object_set_data (self, "ProxyStore", NULL);
  if (parent)
    {
      ProxyStore *ps = g_new0 (ProxyStore, 1);
      BseProxySeq *pseq;
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
      pseq = bse_container_list_items (ps->u.cl.container);
      for (i = 0; i < pseq->n_proxies; i++)
        child_list_wrapper_item_added (ps->u.cl.container, pseq->proxies[i], ps);
      g_object_set_data_full (self, "ProxyStore", ps, child_list_wrapper_destroy_data);
    }
}

static void
child_list_wrapper_rebuild_with_listener (GxkListWrapper *self,
                                          gpointer        listener,
                                          gboolean        set_listener)
{
  ProxyStore *ps = g_object_get_data (self, "ProxyStore");
  SfiProxy parent = ps ? ps->u.cl.container : 0;
  gchar *child_type = ps ? g_strdup (ps->u.cl.child_type) : NULL;
  /* clear list with old listener */
  g_object_set_data (self, "ProxyStore", NULL);
  /* rebuild with new listener */
  if (set_listener)
    g_object_set_data (self, "listener", listener);
  bst_child_list_wrapper_setup (self, parent, child_type);
  g_free (child_type);
}

void
bst_child_list_wrapper_set_listener (GxkListWrapper *self,
                                     void          (*listener) (GtkTreeModel *model,
                                                                SfiProxy      item,
                                                                gboolean      added))
{
  child_list_wrapper_rebuild_with_listener (self, listener, TRUE);
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
  ProxyStore *ps = g_object_get_data (self, "ProxyStore");
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
  ProxyStore *ps = g_object_get_data (self, "ProxyStore");
  return ps ? proxy_store_get_iter (ps, iter, proxy) : FALSE;
}

void
bst_child_list_wrapper_proxy_changed (GxkListWrapper *self,
                                      SfiProxy        item)
{
  ProxyStore *ps = g_object_get_data (self, "ProxyStore");
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
proxy_seq_store_fill_value (GxkListWrapper *self,
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
      item = bst_proxy_seq_store_get_proxy (model, row);
      g_value_set_string_take_ownership (value, g_strdup_printf ("%03u", bse_item_get_seqid (item)));
      break;
    case BST_PROXY_STORE_NAME:
      item = bst_proxy_seq_store_get_proxy (model, row);
      g_value_set_string (value, bse_item_get_name (item));
      break;
    case BST_PROXY_STORE_BLURB:
      item = bst_proxy_seq_store_get_proxy (model, row);
      bse_proxy_get (item, "blurb", &string, NULL);
      g_value_set_string (value, string ? string : "");
      break;
    case BST_PROXY_STORE_TYPE:
      item = bst_proxy_seq_store_get_proxy (model, row);
      g_value_set_string (value, bse_item_get_type (item));
      break;
    }
}

GtkTreeModel*
bst_proxy_seq_store_new (void)
{
  GxkListWrapper *self;
  self = gxk_list_wrapper_new (BST_PROXY_STORE_N_COLS,
                               G_TYPE_STRING,   // BST_PROXY_STORE_SEQID
                               G_TYPE_STRING,   // BST_PROXY_STORE_NAME
                               G_TYPE_STRING,   // BST_PROXY_STORE_BLURB
                               G_TYPE_STRING,   // BST_PROXY_STORE_TYPE
                               -1);
  g_signal_connect_object (self, "fill-value",
                           G_CALLBACK (proxy_seq_store_fill_value),
                           self, G_CONNECT_SWAPPED);
  return GTK_TREE_MODEL (self);
}

static gint
proxy_seq_store_row_from_proxy (ProxyStore *ps,
                                SfiProxy    proxy)
{
  BseProxySeq *pseq = ps->u.pq.pseq;
  guint i;
  for (i = 0; i < pseq->n_proxies; i++)
    if (proxy == pseq->proxies[i])
      return i;
  return -1;
}

static void
proxy_seq_store_destroy_data (gpointer data)
{
  ProxyStore *ps = data;
  guint i;
  gxk_list_wrapper_notify_clear (ps->self);
  for (i = 0; i < ps->u.pq.pseq->n_proxies; i++)
    proxy_store_item_unlisten_on (ps, ps->u.pq.pseq->proxies[i], -1);
  bse_proxy_seq_free (ps->u.pq.pseq);
  g_free (ps);
}

void
bst_proxy_seq_store_set (GtkTreeModel   *model,
                         BseProxySeq    *pseq)
{
  g_object_set_data (model, "ProxyStore", NULL);
  if (pseq && pseq->n_proxies)
    {
      ProxyStore *ps = g_new0 (ProxyStore, 1);
      guint i;
      ps->self = GXK_LIST_WRAPPER (model);
      ps->row_from_proxy = proxy_seq_store_row_from_proxy;
      ps->u.pq.pseq = bse_proxy_seq_copy_shallow (pseq);
      for (i = 0; i < pseq->n_proxies; i++)
        proxy_store_item_listen_on (ps, pseq->proxies[i]);
      g_object_set_data_full (model, "ProxyStore", ps, proxy_seq_store_destroy_data);
    }
}

SfiProxy
bst_proxy_seq_store_get_from_iter (GtkTreeModel *model,
                                   GtkTreeIter  *iter)
{
  GxkListWrapper *self = GXK_LIST_WRAPPER (model);
  gint row = gxk_list_wrapper_get_index (self, iter);
  return bst_proxy_seq_store_get_proxy (model, row);
}

SfiProxy
bst_proxy_seq_store_get_proxy (GtkTreeModel *model,
                               gint          row)
{
  ProxyStore *ps = g_object_get_data (model, "ProxyStore");
  if (ps && row >= 0)
    return row < ps->u.pq.pseq->n_proxies ? ps->u.pq.pseq->proxies[row] : 0;
  return 0;
}

gboolean
bst_proxy_seq_store_get_iter (GtkTreeModel *model,
                              GtkTreeIter  *iter,
                              SfiProxy      proxy)
{
  ProxyStore *ps = g_object_get_data (model, "ProxyStore");
  return ps ? proxy_store_get_iter (ps, iter, proxy) : FALSE;
}
