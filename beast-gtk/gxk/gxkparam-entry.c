/* GXK - Gtk+ Extension Kit
 * Copyright (C) 2002-2003 Tim Janik
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

/* --- string parameters --- */
static void
param_entry_changed (GtkWidget *entry,
                     GxkParam  *param)
{
  const gchar *string = gtk_entry_get_text (GTK_ENTRY (entry));
  g_value_set_string (&param->value, string);
  gxk_param_apply_value (param);
}

static GtkWidget*
param_entry_create (GxkParam    *param,
                    const gchar *tooltip,
                    guint        variant)
{
  GtkWidget *widget = g_object_new (GTK_TYPE_ENTRY,
                                    "visible", TRUE,
                                    "activates_default", TRUE,
                                    "width_chars", 0,
                                    NULL);
  const GxkParamEditorSizes *esizes = gxk_param_get_editor_sizes ();
  gxk_widget_add_font_requisition (widget, esizes->string_chars, esizes->string_digits);
  gxk_param_entry_connect_handlers (param, widget, param_entry_changed);
  gtk_tooltips_set_tip (GXK_TOOLTIPS, widget, tooltip, NULL);
  gxk_widget_add_option (widget, "hexpand", "+");
  return widget;
}

static void
param_entry_update (GxkParam  *param,
		    GtkWidget *widget)
{
  gxk_param_entry_set_text (param, widget, g_value_get_string (&param->value));
}

static GxkParamEditor param_entry = {
  { "entry",            N_("Text Entry"), },
  { G_TYPE_STRING, },
  { NULL,       +5,     TRUE, },        /* options, rating, editing */
  param_entry_create,   param_entry_update,
};
