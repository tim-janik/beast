/* BEAST - Bedevilled Audio System
 * Copyright (C) 2003 Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bstauxdialogs.h"


/* --- list popup dialog --- */
static GSList *list_popups = NULL;
static void
tree_view_remove_selection (GtkTreeView *tview)
{
  GtkTreeSelection *tsel = gtk_tree_view_get_selection (tview);
  GtkTreeModel *model;
  GtkTreeIter iter;
  if (gtk_tree_selection_get_selected (tsel, &model, &iter))
    gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
  else
    gdk_beep ();
}

static void
list_dialog_commit_list (GtkWidget *dialog)
{
  g_object_set_long (dialog, "list-valid", 1);
}

static void
ensure_list_popups (void)
{
  if (!list_popups)
    {
      GtkWidget *scwin = g_object_new (GTK_TYPE_SCROLLED_WINDOW,
                                       "hscrollbar_policy", GTK_POLICY_AUTOMATIC,
                                       "vscrollbar_policy", GTK_POLICY_ALWAYS,
                                       "border_width", 0,
                                       "shadow_type", GTK_SHADOW_IN,
                                       NULL);
      GtkListStore *lstore = gtk_list_store_new (1, G_TYPE_STRING);
      GtkTreeSelection *tsel;
      GtkTreeView *tview = g_object_new (GTK_TYPE_TREE_VIEW,
                                         "can_focus", TRUE,
                                         "model", lstore,
                                         "parent", scwin,
                                         "headers_visible", FALSE,
                                         "height_request", 250,
                                         "width_request", 350,
                                         "rules_hint", TRUE,
                                         "search_column", 0,
                                         NULL);
      GtkWidget *br, *bc;
      GtkWidget *content = bst_vpack (// "1:",     label = g_object_new (GTK_TYPE_LABEL, "label", "Segments:", NULL),
                                      "1:*",    scwin,
                                      "0:",     g_object_new (GTK_TYPE_HSEPARATOR, NULL),
                                      "1:",     bst_hpack ("0:H",       br = bst_stock_dbutton (BST_STOCK_REMOVE),
                                                           "0:H",       bc = bst_stock_dbutton (BST_STOCK_CLOSE),
                                                           NULL),
                                      NULL);
      GtkWidget *dialog = gxk_dialog_new (NULL, NULL,
                                          GXK_DIALOG_HIDE_ON_DELETE | GXK_DIALOG_MODAL,
                                          NULL, content);
      gxk_dialog_set_default (GXK_DIALOG (dialog), bc);
      g_object_set_data (dialog, "tree-view", tview);
      g_object_connect (br, "swapped_signal::clicked", tree_view_remove_selection, tview, NULL);
      g_object_connect (bc, "swapped_signal::clicked", list_dialog_commit_list, dialog, NULL);
      g_object_connect (bc, "swapped_signal_after::clicked", gxk_toplevel_delete, bc, NULL);
      tsel = gtk_tree_view_get_selection (tview);
      gtk_tree_selection_set_mode (tsel, GTK_SELECTION_BROWSE);
      gxk_tree_selection_force_browse (tsel, GTK_TREE_MODEL (lstore));
      gxk_tree_view_add_text_column (tview, 0, "", 0.0, NULL, NULL, NULL, NULL, 0);
      list_popups = g_slist_prepend (list_popups, dialog);
    }
}

typedef struct {
  BstListPopupHandler handler;
  gpointer            data;
  GDestroyNotify      destroy;
} BstListPopupData;

static void
bst_list_popup_hidden (GtkWidget        *dialog,
                       BstListPopupData *data)
{
  glong valid = g_object_get_long (dialog, "list-valid");
  if (valid && data->handler)
    {
      GtkTreeView *tview = g_object_get_data (dialog, "tree-view");
      GtkTreeModel *model = gtk_tree_view_get_model (tview);
      GtkTreeIter iter;
      gchar **strings = g_new0 (gchar*, 1);
      guint i = 0;
      if (gtk_tree_model_get_iter_first (model, &iter))
        do
          {
            guint p = i++;
            strings = g_renew (gchar*, strings, i + 1);
            strings[i] = NULL;
            gtk_tree_model_get (model, &iter, 0, &(strings[p]), -1);
          }
        while (gtk_tree_model_iter_next (model, &iter));
      data->handler (dialog, strings, data->data);
      g_strfreev (strings);
    }
  if (data->destroy)
    data->destroy (data->data);
  g_object_disconnect (dialog, "any_signal", bst_list_popup_hidden, data, NULL);
  g_free (data);
  list_popups = g_slist_prepend (list_popups, dialog);
}

GtkWidget*
bst_list_popup_new (const gchar              *title,
                    GtkWidget                *transient_parent,
                    BstListPopupHandler       handler,
                    gpointer                  handler_data,
                    GDestroyNotify            destroy)
{
  GtkWidget *dialog;
  ensure_list_popups ();
  dialog = g_slist_pop_head (&list_popups);
  if (dialog)
    {
      GtkTreeView *tview = g_object_get_data (dialog, "tree-view");
      GtkTreeModel *model = gtk_tree_view_get_model (tview);
      GtkListStore *lstore = GTK_LIST_STORE (model);
      BstListPopupData *data = g_new0 (BstListPopupData, 1);
      gtk_list_store_clear (lstore);
      g_object_set_long (dialog, "list-valid", 0);
      if (transient_parent)
        transient_parent = gtk_widget_get_ancestor (transient_parent, GTK_TYPE_WINDOW);
      if (transient_parent)
        gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (transient_parent));
      gxk_dialog_set_title (GXK_DIALOG (dialog), title);
      data->handler = handler;
      data->data = handler_data;
      data->destroy = destroy;
      g_object_connect (dialog, "signal_after::hide", bst_list_popup_hidden, data, NULL);
    }
  return dialog;
}

void
bst_list_popup_add (GtkWidget      *dialog,
                    const gchar    *string)
{
  GtkTreeView *tview = g_object_get_data (dialog, "tree-view");
  GtkTreeModel *model = gtk_tree_view_get_model (tview);
  GtkListStore *lstore = GTK_LIST_STORE (model);
  GtkTreeIter iter;
  gtk_list_store_append (lstore, &iter);
  gtk_list_store_set (lstore, &iter,
                      0, string,
                      -1);
}
