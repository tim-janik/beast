/* BEAST - Bedevilled Audio System
 * Copyright (C) 2004 Tim Janik
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


/* --- color parameter editors --- */
#include "bstauxdialogs.h"
static gint
param_color_spinner_input (GtkSpinButton *spin_button,
                          gdouble       *svalue,
                          GxkParam      *param)
{
  const gchar *string = gtk_entry_get_text (GTK_ENTRY (spin_button));
  GdkColor c = { 0 };
  if (gdk_color_parse (string, &c))
    {
      guint n = ((c.red >> 8) << 16) + ((c.green >> 8) << 8) + (c.blue >> 8);
      *svalue = n;
      return TRUE;
    }
  else
    return GTK_INPUT_ERROR;
}

static gint
param_color_spinner_output (GtkSpinButton *spin_button,
                            GxkParam      *param)
{
  guint n = spin_button->adjustment->value;
  gchar *string = g_strdup_printf ("#%06x", n);
  gxk_param_entry_set_text (param, GTK_WIDGET (spin_button), string);
  g_free (string);
  return TRUE;
}

static void
param_color_assign (GtkWidget *dialog,
                    GdkColor  *color,
                    gpointer   user_data)
{
  GtkSpinButton *spin_button = user_data;
  GdkColor c = *color;
  guint n = ((c.red >> 8) << 16) + ((c.green >> 8) << 8) + (c.blue >> 8);
  gtk_adjustment_set_value (spin_button->adjustment, n);
}

static void
param_color_popup_selector (GtkSpinButton *spin_button)
{
  gtk_widget_grab_focus (GTK_WIDGET (spin_button));
  if (gtk_editable_get_editable (GTK_EDITABLE (spin_button)))
    {
      GtkWidget *dialog = bst_color_popup_new (_("Select Color"), GTK_WIDGET (spin_button),
                                               gdk_color_from_rgb (spin_button->adjustment->value),
                                               param_color_assign,
                                               spin_button, NULL);
      gxk_widget_showraise (dialog);
    }
}

static GtkWidget*
param_color_spinner_create (GxkParam    *param,
                           const gchar *tooltip,
                           guint        variant)
{
  GtkAdjustment *adjustment = gxk_param_get_adjustment_with_stepping (param, 0x101010);
  GtkWidget *widget = gtk_spin_button_new (adjustment, 0, 0);
  GtkWidget *btn, *box;
  g_object_set_data (widget, "beast-GxkParam", param);
  g_object_set (widget,
                "visible", TRUE,
                "activates_default", TRUE,
                "width_chars", 0,
                NULL);
  gxk_widget_add_font_requisition (widget, 1+1, 6);
  gxk_param_entry_connect_handlers (param, widget, NULL);
  g_object_connect (widget,
                    "signal::input", param_color_spinner_input, param,
                    "signal::output", param_color_spinner_output, param,
                    NULL);
  gtk_tooltips_set_tip (GXK_TOOLTIPS, widget, tooltip, NULL);
  box = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), widget, TRUE, TRUE, 0);
  btn = bst_stock_icon_button (BST_STOCK_COLOR_SELECTOR);
  gtk_box_pack_end (GTK_BOX (box), btn, FALSE, TRUE, 0);
  gtk_tooltips_set_tip (GXK_TOOLTIPS, btn, tooltip, NULL);
  g_object_connect (btn, "swapped_signal::clicked", param_color_popup_selector, widget, NULL);
  gtk_widget_show_all (box);
  gxk_widget_add_option (box, "hexpand", "+");
  return widget;
}

static void
param_color_spinner_update (GxkParam  *param,
                           GtkWidget *widget)
{
  /* contents are updated through the adjustment */
  gtk_editable_set_editable (GTK_EDITABLE (widget), param->editable);
}

static GxkParamEditor param_color_spinner = {
  { "color-spinner",    N_("Color Entry"), },
  { G_TYPE_INT, },
  { "rgb",              +10,    TRUE, },        /* options, rating, editing */
  param_color_spinner_create, param_color_spinner_update,
};
