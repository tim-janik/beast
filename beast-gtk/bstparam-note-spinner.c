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


/* --- note parameter editors --- */
static gint
param_note_spinner_input (GtkSpinButton *spin_button,
                          gdouble       *svalue,
                          GxkParam      *param)
{
  const gchar *string = gtk_entry_get_text (GTK_ENTRY (spin_button));
  gchar *error;
  *svalue = sfi_note_from_string_err (string, &error);
  g_free (error);
  return error ? GTK_INPUT_ERROR : TRUE;
}

static gint
param_note_spinner_output (GtkSpinButton *spin_button,
                           GxkParam      *param)
{
  gchar *string = sfi_note_to_string (spin_button->adjustment->value);
  gxk_param_entry_set_text (param, GTK_WIDGET (spin_button), string);
  g_free (string);
  return TRUE;
}

static GtkWidget*
param_note_spinner_create (GxkParam    *param,
                           const gchar *tooltip,
                           guint        variant)
{
  GtkAdjustment *adjustment = gxk_param_get_adjustment (param);
  GtkWidget *widget = gtk_spin_button_new (adjustment, 0, 0);
  g_object_set (widget,
                "visible", TRUE,
                "activates_default", TRUE,
                "width_chars", 0,
                NULL);
  gxk_widget_add_font_requisition (widget, 4+1, 1+1);
  gxk_param_entry_connect_handlers (param, widget, NULL);
  g_object_connect (widget,
                    "signal::input", param_note_spinner_input, param,
                    "signal::output", param_note_spinner_output, param,
                    NULL);
  gtk_tooltips_set_tip (GXK_TOOLTIPS, widget, tooltip, NULL);
  return widget;
}

static void
param_note_spinner_update (GxkParam  *param,
                           GtkWidget *widget)
{
  /* contents are updated through the adjustment */
  gtk_editable_set_editable (GTK_EDITABLE (widget), param->editable);
}

static GxkParamEditor param_note_spinner = {
  { "note-spinner",     N_("Note Entry"), },
  { G_TYPE_INT, },
  { "note",             +10,    TRUE, },        /* options, rating, editing */
  param_note_spinner_create, param_note_spinner_update,
};
