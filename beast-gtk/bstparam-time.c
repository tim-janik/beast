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

/* --- time parameter editor --- */
static void
param_time_changed (GtkWidget *entry,
                    GxkParam  *param)
{
  const gchar *string = gtk_entry_get_text (GTK_ENTRY (entry));
  sfi_value_set_time (&param->value, sfi_time_from_string (string));
  gxk_param_apply_value (param);
}

static GtkWidget*
param_time_create (GxkParam    *param,
                   const gchar *tooltip,
                   guint        variant)
{
  GtkWidget *widget = g_object_new (GTK_TYPE_ENTRY,
                                    "visible", TRUE,
                                    "activates_default", TRUE,
                                    "width_chars", 0,
                                    NULL);
  gxk_widget_add_font_requisition (widget, 5, 14);
  gxk_param_entry_connect_handlers (param, widget, param_time_changed);
  gxk_widget_set_tooltip (widget, tooltip);
  return widget;
}

static void
param_time_update (GxkParam  *param,
		   GtkWidget *widget)
{
  gchar *string = sfi_time_to_string (sfi_value_get_time (&param->value));
  gxk_param_entry_set_text (param, widget, string);
  g_free (string);
}

static GxkParamEditor param_time = {
  { "time",             N_("Time Entry"), },
  { G_TYPE_INT64, },
  { "time",             +10,    TRUE, },        /* options, rating, editing */
  param_time_create,    param_time_update,
};
