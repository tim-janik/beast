/* BEAST - Better Audio System
 * Copyright (C) 2004 Tim Janik
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
#include "bstitemseqdialog.hh"
#include "bsttreestores.hh"


/* --- prototypes --- */
static void     bst_item_seq_dialog_activate          (BstItemSeqDialog *self);
static gboolean bst_item_seq_dialog_delete_event      (GtkWidget        *widget,
                                                       GdkEventAny      *event);
static void     bst_item_seq_dialog_setup             (BstItemSeqDialog *self,
                                                       gpointer          parent_widget,
                                                       const gchar      *title,
                                                       SfiProxy          proxy);


/* --- functions --- */
G_DEFINE_TYPE (BstItemSeqDialog, bst_item_seq_dialog, GXK_TYPE_DIALOG);

static void
bst_item_seq_dialog_finalize (GObject *object)
{
  BstItemSeqDialog *self = BST_ITEM_SEQ_DIALOG (object);

  bst_item_seq_dialog_setup (self, NULL, NULL, 0);

  /* chain parent class' handler */
  G_OBJECT_CLASS (bst_item_seq_dialog_parent_class)->finalize (object);
}

static gboolean
bst_item_seq_dialog_delete_event (GtkWidget   *widget,
                                  GdkEventAny *event)
{
  BstItemSeqDialog *self = BST_ITEM_SEQ_DIALOG (widget);
  GxkFreeFunc selected_cleanup = self->selected_cleanup;
  self->selected_callback = NULL;
  self->selected_cleanup = NULL;
  if (selected_cleanup)
    selected_cleanup (self->selected_data);
  if (self->candidate_store)
    bst_item_seq_store_set (self->candidate_store, NULL);
  if (self->item_store)
    bst_item_seq_store_set (self->item_store, NULL);
  /* chain parent class' handler */
  return GTK_WIDGET_CLASS (bst_item_seq_dialog_parent_class)->delete_event (widget, event);
}

static void
parent_window_destroyed (BstItemSeqDialog *self)
{
  self->selected_callback = NULL;
  GxkFreeFunc selected_cleanup = self->selected_cleanup;
  self->selected_callback = NULL;
  self->selected_cleanup = NULL;
  if (selected_cleanup)
    selected_cleanup (self->selected_data);
  gxk_toplevel_delete (GTK_WIDGET (self));
  gtk_widget_hide (GTK_WIDGET (self));
  bst_item_seq_dialog_setup (self, NULL, NULL, 0);
}

static void
bst_item_seq_dialog_setup (BstItemSeqDialog *self,
                           gpointer          parent_widget,
                           const gchar      *title,
                           SfiProxy          proxy)
{
  GtkWindow *window = GTK_WINDOW (self);
  
  g_return_if_fail (BST_IS_ITEM_SEQ_DIALOG (self));
  
  GxkFreeFunc selected_cleanup = self->selected_cleanup;
  self->selected_callback = NULL;
  self->selected_cleanup = NULL;
  if (selected_cleanup)
    selected_cleanup (self->selected_data);

  gtk_widget_hide (GTK_WIDGET (self));

  /* reset proxy handling */
  bst_window_sync_title_to_proxy (self, proxy, title);

  /* cleanup connections to old parent_window */
  if (self->parent_window)
    g_signal_handlers_disconnect_by_func (self->parent_window, (void*) parent_window_destroyed, self);
  if (window->group)
    gtk_window_group_remove_window (window->group, window);
  gtk_window_set_transient_for (window, NULL);

  self->parent_window = parent_widget ? (GtkWindow*) gtk_widget_get_ancestor ((GtkWidget*) parent_widget, GTK_TYPE_WINDOW) : NULL;

  /* setup connections to new parent_window */
  if (self->parent_window)
    {
      gtk_window_set_transient_for (window, self->parent_window);
      if (self->parent_window->group)
        gtk_window_group_add_window (self->parent_window->group, window);
      g_signal_connect_object (self->parent_window, "destroy",
                               G_CALLBACK (parent_window_destroyed),
                               self, G_CONNECT_SWAPPED);
    }

  /* allow activation */
  self->ignore_activate = FALSE;
}

static BstItemSeqDialog*
bst_item_seq_dialog_singleton (void)
{
  static BstItemSeqDialog *ts_singleton = NULL;
  if (!ts_singleton)
    ts_singleton = (BstItemSeqDialog*) g_object_new (BST_TYPE_ITEM_SEQ_DIALOG, NULL);
  return ts_singleton;
}

GtkWidget*
bst_item_seq_dialog_popup (gpointer     parent_widget,
                           SfiProxy     item,
                           const gchar *candidate_label,
                           const gchar *candidate_tooltip,
                           BseItemSeq  *candidates,
                           const gchar *item_label,
                           const gchar *item_tooltip,
                           BseItemSeq  *items,
                           BstItemSeqDialogSelected selected_callback,
                           gpointer                 selected_data,
                           GxkFreeFunc              selected_cleanup)
{
  BstItemSeqDialog *self = bst_item_seq_dialog_singleton ();
  GtkWidget *widget = GTK_WIDGET (self);
  GxkDialog *dialog = GXK_DIALOG (self);
  GtkWidget *radget = gxk_dialog_get_child (dialog);

  bst_item_seq_dialog_setup (self, NULL, NULL, 0);

  g_object_set (gxk_radget_find (radget, "candidate-label"), "label", candidate_label, NULL);
  g_object_set (gxk_radget_find (radget, "item-label"), "label", item_label, NULL);
  gxk_widget_set_tooltip (gxk_radget_find (radget, "candidate-view"), candidate_tooltip);
  gxk_widget_set_tooltip (gxk_radget_find (radget, "item-view"), item_tooltip);

  /* construct add/remove button tooltips */
  gchar *string;
  string = g_strdup_printf (_("Adds the selection from the \"%s\" list to the \"%s\" list"), candidate_label, item_label);
  gxk_widget_set_tooltip (gxk_radget_find (radget, "button-add"), string);
  g_free (string);
  string = g_strdup_printf (_("Removes the selection from the \"%s\" list"), item_label);
  gxk_widget_set_tooltip (gxk_radget_find (radget, "button-remove"), string);
  g_free (string);

  bst_item_seq_dialog_set (self, candidates, items);
  bst_item_seq_dialog_setup (self, parent_widget,
                             /* TRANSLATORS: this is a dialog title and %s is replaced by an object name */
                             _("Object Selection: %s"),
                             item);
  self->selected_callback = selected_callback;
  self->selected_data = selected_data;
  self->selected_cleanup = selected_cleanup;
  gxk_widget_showraise (widget);

  return widget;
}

void
bst_item_seq_dialog_set (BstItemSeqDialog *self,
                         BseItemSeq       *candidates,
                         BseItemSeq       *iseq)
{
  g_return_if_fail (BST_IS_ITEM_SEQ_DIALOG (self));

  bst_item_seq_store_set (self->candidate_store, candidates);
  bst_item_seq_store_set (self->item_store, iseq);
}

static gboolean
bst_item_seq_dialog_sensitize_idle (gpointer data)
{
  BstItemSeqDialog *self = BST_ITEM_SEQ_DIALOG (data);
  /* we adjust sensitivity asyncronously, to avoid resetting button focus
   * in gtk_widget_set_sensitive() due to temporary tree/selection states.
   */
  GDK_THREADS_ENTER();
  GxkRadget *radget = gxk_dialog_get_child (GXK_DIALOG (self));
  GtkWidget *button;
  button = (GtkWidget*) gxk_radget_find (radget, "button-add");
  if (button)
    gtk_widget_set_sensitive (button, gtk_tree_selection_get_selected (self->candidate_sel, NULL, NULL));
  button = (GtkWidget*) gxk_radget_find (radget, "button-remove");
  if (button)
    gtk_widget_set_sensitive (button, gtk_tree_selection_get_selected (self->item_sel, NULL, NULL));
  gboolean can_raise = FALSE, can_lower = FALSE;
  GtkTreeIter piter;
  if (self->item_sel && gtk_tree_selection_get_selected (self->item_sel, NULL, &piter))
    {
      SfiProxy proxy = bst_item_seq_store_get_from_iter (self->item_store, &piter);
      can_lower = bst_item_seq_store_can_lower (self->item_store, proxy);
      can_raise = bst_item_seq_store_can_raise (self->item_store, proxy);
    }
  button = (GtkWidget*) gxk_radget_find (radget, "button-up");
  if (button)
    gtk_widget_set_sensitive (button, can_raise);
  button = (GtkWidget*) gxk_radget_find (radget, "button-down");
  if (button)
    gtk_widget_set_sensitive (button, can_lower);
  g_object_unref (self);
  GDK_THREADS_LEAVE();
  return FALSE;
}

static void
bst_item_seq_dialog_queue_sensitize (BstItemSeqDialog *self)
{
  g_idle_add (bst_item_seq_dialog_sensitize_idle, g_object_ref (self));
}

static void
bst_item_seq_dialog_up (BstItemSeqDialog *self)
{
  if (self->ignore_activate)
    return;
  GtkTreeIter piter;
  GtkTreeModel *model;
  if (gtk_tree_selection_get_selected (self->item_sel, &model, &piter))
    {
      SfiProxy proxy = bst_item_seq_store_get_from_iter (model, &piter);
      gint row = bst_item_seq_store_raise (self->item_store, proxy);
      gxk_tree_view_select_index (gtk_tree_selection_get_tree_view (self->item_sel), row);
    }
}

static void
bst_item_seq_dialog_down (BstItemSeqDialog *self)
{
  if (self->ignore_activate)
    return;
  GtkTreeIter piter;
  GtkTreeModel *model;
  if (gtk_tree_selection_get_selected (self->item_sel, &model, &piter))
    {
      SfiProxy proxy = bst_item_seq_store_get_from_iter (model, &piter);
      gint row = bst_item_seq_store_lower (self->item_store, proxy);
      gxk_tree_view_select_index (gtk_tree_selection_get_tree_view (self->item_sel), row);
    }
}

static void
bst_item_seq_dialog_add (BstItemSeqDialog *self)
{
  if (self->ignore_activate)
    return;
  GtkTreeIter piter;
  GtkTreeModel *model;
  if (gtk_tree_selection_get_selected (self->candidate_sel, &model, &piter))
    {
      SfiProxy proxy = bst_item_seq_store_get_from_iter (model, &piter);
      bst_item_seq_store_remove (self->candidate_store, proxy);
      gint row = bst_item_seq_store_add (self->item_store, proxy);
      gxk_tree_view_select_index (gtk_tree_selection_get_tree_view (self->item_sel), row);
    }
}

static void
bst_item_seq_dialog_remove (BstItemSeqDialog *self)
{
  if (self->ignore_activate)
    return;
  GtkTreeIter piter;
  GtkTreeModel *model;
  if (gtk_tree_selection_get_selected (self->item_sel, &model, &piter))
    {
      SfiProxy proxy = bst_item_seq_store_get_from_iter (model, &piter);
      bst_item_seq_store_remove (self->item_store, proxy);
      gint row = bst_item_seq_store_add (self->candidate_store, proxy);
      gxk_tree_view_select_index (gtk_tree_selection_get_tree_view (self->candidate_sel), row);
    }
}

static void
bst_item_seq_dialog_activate (BstItemSeqDialog *self)
{
  if (self->ignore_activate)
    return;

  /* ignore_activate guards against multiple clicks */
  self->ignore_activate = TRUE;
  BstItemSeqDialogSelected selected_callback = self->selected_callback;
  gpointer                 selected_data = self->selected_data;
  GxkFreeFunc              selected_cleanup = self->selected_cleanup;
  self->selected_callback = NULL;
  self->selected_cleanup = NULL;
  if (selected_callback)        /* notify popup caller */
    {
      BseItemSeq *iseq = bst_item_seq_store_dup (self->item_store);
      selected_callback (selected_data, iseq, self);
      bse_item_seq_free (iseq);
    }
  if (selected_cleanup)
    selected_cleanup (selected_data);

  gxk_toplevel_delete (GTK_WIDGET (self));
}

static void
bst_item_seq_dialog_init (BstItemSeqDialog *self)
{
  GtkTreeView *tview;

  /* configure self */
  g_object_set (self,
                "flags", (GXK_DIALOG_HIDE_ON_DELETE |
                          GXK_DIALOG_PRESERVE_STATE |
                          GXK_DIALOG_POPUP_POS |
                          GXK_DIALOG_MODAL),
                NULL);
  gxk_dialog_set_sizes (GXK_DIALOG (self), 550, 300, 600, 320);
  
  /* dialog contents */
  GxkRadget *radget = gxk_radget_create ("beast", "item-seq-box", NULL);
  gxk_dialog_set_child (GXK_DIALOG (self), (GtkWidget*) radget);

  /* candidate store and selection setup */
  self->candidate_store = bst_item_seq_store_new (TRUE);
  tview = (GtkTreeView*) gxk_radget_find (radget, "candidate-view");
  g_object_set (tview, "model", self->candidate_store, "search_column", BST_PROXY_STORE_NAME, NULL);
  self->candidate_sel = gtk_tree_view_get_selection (tview);
  gtk_tree_selection_set_mode (self->candidate_sel, GTK_SELECTION_BROWSE);
  gxk_tree_selection_force_browse (self->candidate_sel, self->candidate_store);
  g_signal_connect_swapped (self->candidate_sel, "changed", G_CALLBACK (bst_item_seq_dialog_queue_sensitize), self);
  /* candidate store tree columns */
  gxk_tree_view_add_text_column (tview, BST_PROXY_STORE_NAME, "S",
                                 0.0, "Name", NULL,
                                 NULL, NULL, G_CONNECT_SWAPPED);
  gxk_tree_view_add_text_column (tview, BST_PROXY_STORE_BLURB, "",
                                 0.0, "Comment", NULL,
                                 NULL, NULL, G_CONNECT_SWAPPED);
  if (BST_DVL_HINTS)
    gxk_tree_view_add_text_column (tview, BST_PROXY_STORE_TYPE, "",
                                   0.0, "Type", NULL,
                                   NULL, NULL, G_CONNECT_SWAPPED);
  /* make row connections */
  g_signal_connect_object (tview, "row_activated", G_CALLBACK (gtk_button_clicked), gxk_radget_find (radget, "button-add"), G_CONNECT_SWAPPED);
  
  /* item store and selection setup */
  self->item_store = bst_item_seq_store_new (FALSE);
  tview = (GtkTreeView*) gxk_radget_find (radget, "item-view");
  g_object_set (tview, "model", self->item_store, "search_column", BST_PROXY_STORE_NAME, NULL);
  self->item_sel = gtk_tree_view_get_selection (tview);
  gtk_tree_selection_set_mode (self->item_sel, GTK_SELECTION_BROWSE);
  gxk_tree_selection_force_browse (self->item_sel, self->item_store);
  g_signal_connect_swapped (self->item_sel, "changed", G_CALLBACK (bst_item_seq_dialog_queue_sensitize), self);
  /* item store tree columns */
  gxk_tree_view_add_text_column (tview, BST_PROXY_STORE_NAME, "S",
                                 0.0, "Name", NULL,
                                 NULL, NULL, G_CONNECT_SWAPPED);
  gxk_tree_view_add_text_column (tview, BST_PROXY_STORE_BLURB, "",
                                 0.0, "Comment", NULL,
                                 NULL, NULL, G_CONNECT_SWAPPED);
  if (BST_DVL_HINTS)
    gxk_tree_view_add_text_column (tview, BST_PROXY_STORE_TYPE, "",
                                   0.0, "Type", NULL,
                                   NULL, NULL, G_CONNECT_SWAPPED);
  /* make row connections */
  g_signal_connect_object (tview, "row_activated", G_CALLBACK (gtk_button_clicked), gxk_radget_find (radget, "button-remove"), G_CONNECT_SWAPPED);

  /* provide buttons */
  self->ok = gxk_dialog_default_action_swapped (GXK_DIALOG (self), BST_STOCK_OK, (void*) bst_item_seq_dialog_activate, self);
  gxk_dialog_action (GXK_DIALOG (self), BST_STOCK_CANCEL, (void*) gxk_toplevel_delete, self);

  /* connect buttons */
  g_signal_connect_object (gxk_radget_find (radget, "button-add"), "clicked", G_CALLBACK (bst_item_seq_dialog_add), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (gxk_radget_find (radget, "button-remove"), "clicked", G_CALLBACK (bst_item_seq_dialog_remove), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (gxk_radget_find (radget, "button-up"), "clicked", G_CALLBACK (bst_item_seq_dialog_up), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (gxk_radget_find (radget, "button-down"), "clicked", G_CALLBACK (bst_item_seq_dialog_down), self, G_CONNECT_SWAPPED);
  bst_item_seq_dialog_queue_sensitize (self);
}

static void
bst_item_seq_dialog_class_init (BstItemSeqDialogClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  
  gobject_class->finalize = bst_item_seq_dialog_finalize;

  widget_class->delete_event = bst_item_seq_dialog_delete_event;
}
