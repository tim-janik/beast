/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002-2003 Tim Janik
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


/* --- searchpath editor --- */
#include "bstauxdialogs.h"
#include "bstfiledialog.h"
static void
param_searchpath_change_value (GtkWidget *entry)
{
  GxkParam *param = g_object_get_data (entry, "beast-GxkParam");
  if (!param->updating)
    {
      const gchar *string = gtk_entry_get_text (GTK_ENTRY (entry));
      g_value_set_string (&param->value, string);
      gxk_param_apply_value (param);
    }
}

static void
param_searchpath_assign (GtkWidget  *dialog,
                         gchar     **strings,
                         gpointer    user_data)
{
  GtkWidget *widget = user_data;
  gchar *paths = g_strjoinv (G_SEARCHPATH_SEPARATOR_S, strings);
  gtk_entry_set_text (GTK_ENTRY (widget), paths);
  g_free (paths);
  param_searchpath_change_value (widget);
}

static void
param_searchpath_popup_remove (GtkWidget *widget)
{
  GtkEntry *entry = GTK_ENTRY (widget);
  gchar **paths = g_strsplit (gtk_entry_get_text (entry), G_SEARCHPATH_SEPARATOR_S, -1);
  GtkWidget *dialog = bst_list_popup_new (_("Remove Paths"), widget,
                                          param_searchpath_assign,
                                          widget, NULL);
  guint i;
  for (i = 0; paths[i]; i++)
    bst_list_popup_add (dialog, paths[i]);
  g_strfreev (paths);
  gxk_widget_showraise (dialog);
}

static void
param_searchpath_add (GtkWidget   *dialog,
                      const gchar *file,
                      gpointer     user_data)
{
  GtkWidget *widget = user_data;
  gchar *path = gtk_entry_get_text (GTK_ENTRY (widget));
  gchar *str = g_path_concat (path, file, NULL);
  gtk_entry_set_text (GTK_ENTRY (widget), str);
  g_free (str);
  param_searchpath_change_value (widget);
}

static void
param_searchpath_popup_add (GtkWidget *widget)
{
  GtkWidget *dialog = bst_file_dialog_popup_select_dir (widget);
  bst_file_dialog_set_handler (BST_FILE_DIALOG (dialog), param_searchpath_add, widget, NULL);
}

static GtkWidget*
param_searchpath_create (GxkParam    *param,
                         const gchar *tooltip,
                         guint        variant)
{
  GtkWidget *widget = gxk_param_create_editor (param, "entry");
  GtkWidget *box = gtk_hbox_new (FALSE, 0);
  GtkWidget *br = bst_stock_icon_button (BST_STOCK_REMOVE);
  GtkWidget *ba = bst_stock_icon_button (BST_STOCK_ADD);
  g_object_set_data (widget, "beast-GxkParam", param);
  gtk_box_pack_start (GTK_BOX (box), widget, TRUE, TRUE, 0);
  gtk_box_pack_end (GTK_BOX (box), br, FALSE, TRUE, 0);
  gtk_box_pack_end (GTK_BOX (box), ba, FALSE, TRUE, 0);
  g_object_connect (br, "swapped_signal::clicked", param_searchpath_popup_remove, widget, NULL);
  g_object_connect (ba, "swapped_signal::clicked", param_searchpath_popup_add, widget, NULL);
  gtk_tooltips_set_tip (GXK_TOOLTIPS, br, tooltip, NULL);
  gtk_tooltips_set_tip (GXK_TOOLTIPS, ba, tooltip, NULL);
  gtk_widget_show_all (box);
  gxk_widget_add_option (box, "hexpand", "+");
  return box;
}

static GxkParamEditor param_searchpath = {
  { "searchpath",       N_("Searchpath Text Entry"), },
  { G_TYPE_STRING, },
  { "searchpath",       +5,     TRUE, },        /* options, rating, editing */
  param_searchpath_create,  NULL,
};
