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
#include <gdk/gdkkeysyms.h>


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
widget_flag_data_valid (GtkWidget *dialog)
{
  g_object_set_long (dialog, "data-valid", 1);
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
      g_object_connect (bc, "swapped_signal::clicked", widget_flag_data_valid, dialog, NULL);
      g_object_connect (bc, "swapped_signal_after::clicked", gxk_toplevel_delete, bc, NULL);
      tsel = gtk_tree_view_get_selection (tview);
      gtk_tree_selection_set_mode (tsel, GTK_SELECTION_BROWSE);
      gxk_tree_selection_force_browse (tsel, GTK_TREE_MODEL (lstore));
      gxk_tree_view_add_text_column (tview, 0, "", 0.0, NULL, NULL, NULL, NULL, 0);
      list_popups = g_slist_prepend (list_popups, dialog);
    }
}

typedef struct {
  GCallback      handler;
  gpointer       data;
  GDestroyNotify destroy;
} CustomPopupData;

static void
bst_list_popup_hidden (GtkWidget       *dialog,
                       CustomPopupData *data)
{
  glong valid = g_object_get_long (dialog, "data-valid");
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
      ((BstListPopupHandler) data->handler) (dialog, strings, data->data);
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
      CustomPopupData *data = g_new0 (CustomPopupData, 1);
      gtk_list_store_clear (lstore);
      g_object_set_long (dialog, "data-valid", 0);
      if (transient_parent)
        transient_parent = gtk_widget_get_ancestor (transient_parent, GTK_TYPE_WINDOW);
      if (transient_parent)
        gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (transient_parent));
      gxk_dialog_set_title (GXK_DIALOG (dialog), title);
      data->handler = (GCallback) handler;
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


/* --- enable activates-default --- */
static void     colorsel_enable_activates_default (GtkWidget *widget);

static gboolean
idle_activate_toplevel (gpointer data)
{
  GtkWidget *widget = data;
  GDK_THREADS_ENTER ();
  gxk_toplevel_activate_default (widget);
  g_object_unref (widget);
  GDK_THREADS_LEAVE ();
  return FALSE;
}

static gboolean
colorsel_drawing_area_key_event (GtkWidget *widget,
                                 GdkEvent  *event)
{
  if (event->type == GDK_KEY_PRESS &&
      (event->key.keyval == GDK_Return ||
       event->key.keyval == GDK_KP_Enter))
    g_idle_add (idle_activate_toplevel, g_object_ref (widget));
  return FALSE;
}

static void
recurse_set_activates_default (GtkWidget *child,
                               gpointer   user_data)
{
  colorsel_enable_activates_default (child);
}

static void
colorsel_enable_activates_default (GtkWidget *widget)
{
  if (GTK_IS_DRAWING_AREA (widget))
    {
      // gtk_widget_add_events (widget, GDK_KEY_PRESS_MASK);
      g_signal_connect (widget, "event", G_CALLBACK (colorsel_drawing_area_key_event), NULL);
    }
  if (GTK_IS_ENTRY (widget))
    g_object_set (widget, "activates-default", TRUE, NULL);
  if (GTK_IS_CONTAINER (widget))
    gtk_container_forall (GTK_CONTAINER (widget), recurse_set_activates_default, NULL);
}


/* --- color popup dialog --- */
static GSList *color_popups = NULL;
static void
ensure_color_popups (void)
{
  if (!color_popups)
    {
      GtkWidget *widget = gtk_color_selection_new ();
      GtkColorSelection *csel = GTK_COLOR_SELECTION (widget);
      GtkWidget *bx, *bc;
      GtkWidget *content = bst_vpack ("1:*",    csel,
                                      "0:",     g_object_new (GTK_TYPE_HSEPARATOR, NULL),
                                      "1:",     bst_hpack ("0:H",       bx = bst_stock_dbutton (BST_STOCK_CANCEL),
                                                           "0:H",       bc = bst_stock_dbutton (BST_STOCK_CLOSE),
                                                           NULL),
                                      NULL);
      GtkWidget *dialog = gxk_dialog_new (NULL, NULL,
                                          GXK_DIALOG_HIDE_ON_DELETE | GXK_DIALOG_MODAL,
                                          NULL, content);
      g_object_set_data (dialog, "color-selection", csel);
      colorsel_enable_activates_default (widget);
      gtk_widget_show (widget);
      gxk_dialog_set_default (GXK_DIALOG (dialog), bc);
      g_object_connect (bc, "swapped_signal::clicked", widget_flag_data_valid, dialog, NULL);
      g_object_connect (bc, "swapped_signal_after::clicked", gxk_toplevel_delete, bc, NULL);
      g_object_connect (bx, "swapped_signal_after::clicked", gxk_toplevel_delete, bc, NULL);
      gtk_color_selection_set_has_opacity_control (csel, FALSE);
      gtk_color_selection_set_has_palette (csel, TRUE);
      color_popups = g_slist_prepend (color_popups, dialog);
    }
}

static void
color_popup_hidden (GtkWidget       *dialog,
                    CustomPopupData *data)
{
  glong valid = g_object_get_long (dialog, "data-valid");
  if (valid && data->handler)
    {
      GtkColorSelection *csel = g_object_get_data (dialog, "color-selection");
      GdkColor color;
      gtk_color_selection_get_current_color (csel, &color);
      ((BstColorPopupHandler) data->handler) (dialog, &color, data->data);
    }
  if (data->destroy)
    data->destroy (data->data);
  g_object_disconnect (dialog, "any_signal", color_popup_hidden, data, NULL);
  g_free (data);
  color_popups = g_slist_prepend (color_popups, dialog);
}

GtkWidget*
bst_color_popup_new (const gchar            *title,
                     GtkWidget              *transient_parent,
                     GdkColor                color,
                     BstColorPopupHandler    handler,
                     gpointer                handler_data,
                     GDestroyNotify          destroy)
{
  GtkWidget *dialog;
  ensure_color_popups ();
  dialog = g_slist_pop_head (&color_popups);
  if (dialog)
    {
      GtkColorSelection *csel = g_object_get_data (dialog, "color-selection");
      CustomPopupData *data = g_new0 (CustomPopupData, 1);
      g_object_set_long (dialog, "data-valid", 0);
      gtk_color_selection_set_current_color (csel, &color);
      if (transient_parent)
        transient_parent = gtk_widget_get_ancestor (transient_parent, GTK_TYPE_WINDOW);
      if (transient_parent)
        gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (transient_parent));
      gxk_dialog_set_title (GXK_DIALOG (dialog), title);
      data->handler = (GCallback) handler;
      data->data = handler_data;
      data->destroy = destroy;
      g_object_connect (dialog, "signal_after::hide", color_popup_hidden, data, NULL);
    }
  return dialog;
}


/* --- key grabber dialog --- */
gboolean
bst_key_combo_valid (guint              keyval,
                     GdkModifierType    modifiers)
{
  static const guint extra_valid[] = {
    GDK_Tab, GDK_ISO_Left_Tab, GDK_KP_Tab,
    GDK_Up, GDK_Down, GDK_Left, GDK_Right,
    GDK_KP_Up, GDK_KP_Down, GDK_KP_Left, GDK_KP_Right,
    0
  };
  guint i;
  if (gtk_accelerator_valid (keyval, modifiers))
    return TRUE;
  for (i = 0; extra_valid[i]; i++)
    if (extra_valid[i] == keyval)
      return TRUE;
  return FALSE;
}

static guint           grab_key_keyval;
static GdkModifierType grab_key_modifier;
static gboolean        grab_key_valid;

static gboolean
grab_key_event (GtkWidget *window,
                GdkEvent  *event)
{
  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
    case GDK_2BUTTON_PRESS:
    case GDK_3BUTTON_PRESS:
      gtk_widget_hide (window);
      break;
    case GDK_KEY_PRESS:
      grab_key_keyval = gdk_keyval_to_lower (event->key.keyval);
      grab_key_modifier = event->key.state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK);
      if (bst_key_combo_valid (grab_key_keyval, grab_key_modifier))
        {
          grab_key_valid = TRUE;
          gtk_widget_hide (window);
        }
      break;
    case GDK_DELETE:
      gtk_widget_hide (window);
      break;
    default: ;
    }
  return !GTK_WIDGET_VISIBLE (window);
}

gboolean
bst_key_combo_popup (const gchar            *function,
                     guint                  *keyval,
                     GdkModifierType        *modifier)
{
  static GtkWidget *key_window;
  static GtkWidget *label;
  if (!key_window)
    {
      key_window = g_object_new (GTK_TYPE_WINDOW,
                                 "type", GTK_WINDOW_POPUP,
                                 "modal", TRUE,
                                 "window-position", GTK_WIN_POS_CENTER_ALWAYS,
                                 "default-width", 320,
                                 "default-height", 200,
                                 NULL);
      label = g_object_new (GTK_TYPE_LABEL,
                            "visible", TRUE,
                            "wrap", TRUE,
                            "parent",
                            g_object_new (GTK_TYPE_ALIGNMENT,
                                          "visible", TRUE,
                                          "border-width", 10,
                                          "parent",
                                          g_object_new (GTK_TYPE_FRAME,
                                                        "visible", TRUE,
                                                        "parent", key_window,
                                                        "shadow-type", GTK_SHADOW_ETCHED_OUT,
                                                        NULL),
                                          NULL),
                            NULL);
      g_signal_connect (key_window, "event", G_CALLBACK (grab_key_event), NULL);
    }
  if (function && function[0])
    {
      gchar *str = g_strdup_printf (_("Please press the keyboard shortcut to be installed for function: %s"), function);
      gtk_label_set_text (GTK_LABEL (label), str);
      g_free (str);
    }
  else
    gtk_label_set_text (GTK_LABEL (label), _("Please press the keyboard shortcut to be installed..."));
  grab_key_valid = FALSE;
  gtk_widget_show (key_window);
  if (gxk_grab_pointer_and_keyboard (key_window->window, TRUE, GDK_BUTTON_PRESS_MASK,
                                     NULL, NULL, GDK_CURRENT_TIME))
    {
      while (GTK_WIDGET_VISIBLE (key_window))
        {
          GDK_THREADS_LEAVE ();
          g_main_iteration (TRUE);
          GDK_THREADS_ENTER ();
        }
      gxk_ungrab_pointer_and_keyboard (key_window->window, GDK_CURRENT_TIME);
    }
  gtk_widget_hide (key_window);
  if (keyval)
    *keyval = grab_key_valid ? grab_key_keyval : 0;
  if (modifier)
    *modifier = grab_key_valid ? grab_key_modifier : 0;
  return grab_key_valid;
}
