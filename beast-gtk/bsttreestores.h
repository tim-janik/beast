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
#ifndef __BST_TREE_STORES_H__
#define __BST_TREE_STORES_H__

#include "bstutils.h"

G_BEGIN_DECLS


/* --- file store --- */
enum {
  BST_FILE_STORE_COL_ID,
  BST_FILE_STORE_COL_FILE,
  BST_FILE_STORE_COL_BASE_NAME,
  BST_FILE_STORE_COL_WAVE_NAME,
  BST_FILE_STORE_COL_SIZE,
  BST_FILE_STORE_COL_TIME_USECS,
  BST_FILE_STORE_COL_TIME_STR,
  BST_FILE_STORE_COL_LOADER,
  BST_FILE_STORE_COL_LOADABLE,
  BST_FILE_STORE_N_COLS
};
GtkTreeModel*   bst_file_store_create           (void);
void            bst_file_store_update_list      (GtkTreeModel *model,
                                                 const gchar  *search_path,
                                                 const gchar  *filter);
void            bst_file_store_forget_list      (GtkTreeModel *model);
void            bst_file_store_destroy          (GtkTreeModel *model);


/* --- proxy stores --- */
enum {
  BST_PROXY_STORE_SEQID,
  BST_PROXY_STORE_NAME,
  BST_PROXY_STORE_BLURB,
  BST_PROXY_STORE_TYPE,
  BST_PROXY_STORE_N_COLS
};
/* store based on a BseProxySeq */
GtkTreeModel*   bst_proxy_seq_store_new                 (void);
void            bst_proxy_seq_store_set                 (GtkTreeModel   *self,
                                                         BseProxySeq    *pseq);
SfiProxy        bst_proxy_seq_store_get_proxy           (GtkTreeModel   *self,
                                                         gint            row);
SfiProxy        bst_proxy_seq_store_get_from_iter       (GtkTreeModel   *self,
                                                         GtkTreeIter    *iter);
gboolean        bst_proxy_seq_store_get_iter            (GtkTreeModel   *self,
                                                         GtkTreeIter    *iter,
                                                         SfiProxy        proxy);
/* store based on the child list of a container */
GxkListWrapper* bst_child_list_wrapper_store_new        (void);


/* --- generic child list wrapper --- */
void     bst_child_list_wrapper_setup           (GxkListWrapper *self,
                                                 SfiProxy        parent,
                                                 const gchar    *child_type);
void     bst_child_list_wrapper_set_listener    (GxkListWrapper *self,
                                                 void          (*listener) (GtkTreeModel *model,
                                                                            SfiProxy      item,
                                                                            gboolean      added));
void     bst_child_list_wrapper_rebuild         (GxkListWrapper *self);
SfiProxy bst_child_list_wrapper_get_proxy       (GxkListWrapper *self,
                                                 gint            row);
SfiProxy bst_child_list_wrapper_get_from_iter   (GxkListWrapper *self,
                                                 GtkTreeIter    *iter);
gboolean bst_child_list_wrapper_get_iter        (GxkListWrapper *self,
                                                 GtkTreeIter    *iter,
                                                 SfiProxy        proxy);
void     bst_child_list_wrapper_proxy_changed   (GxkListWrapper *self,
                                                 SfiProxy        item);


G_END_DECLS

#endif /* __BST_TREE_STORES_H__ */
