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

/* --- spinnbutton editors --- */
enum {
  PARAM_SPINNER_LINEAR,
  PARAM_SPINNER_LOGARITHMIC,
};

static GtkWidget*
param_spinner_create (GxkParam    *param,
                      const gchar *tooltip,
                      guint        variant)
{
  GtkWidget *widget;
  GtkAdjustment *adjustment = NULL;
  const GxkParamEditorSizes *esize = gxk_param_get_editor_sizes ();
  guint chars = 1, digits = 0, fracts = 0;
  switch (G_TYPE_FUNDAMENTAL (G_VALUE_TYPE (&param->value)))
    {
    case G_TYPE_CHAR:   chars = esize->char_chars;   digits = esize->char_digits;   break;
    case G_TYPE_UCHAR:  chars = esize->uchar_chars;  digits = esize->uchar_digits;  break;
    case G_TYPE_INT:    chars = esize->int_chars;    digits = esize->int_digits;    break;
    case G_TYPE_UINT:   chars = esize->uint_chars;   digits = esize->uint_digits;   break;
    case G_TYPE_LONG:   chars = esize->long_chars;   digits = esize->long_digits;   break;
    case G_TYPE_ULONG:  chars = esize->ulong_chars;  digits = esize->ulong_digits;  break;
    case G_TYPE_INT64:  chars = esize->int64_chars;  digits = esize->int64_digits;  break;
    case G_TYPE_UINT64: chars = esize->uint64_chars; digits = esize->uint64_digits; break;
    case G_TYPE_FLOAT:  chars = esize->float_chars;  digits = esize->float_digits;  fracts = 7;  break;
    case G_TYPE_DOUBLE: chars = esize->double_chars; digits = esize->double_digits; fracts = 17; break;
    }
  if (variant == PARAM_SPINNER_LOGARITHMIC)
    adjustment = gxk_param_get_log_adjustment (param);
  if (!adjustment)
    adjustment = gxk_param_get_adjustment (param);
  widget = g_object_new (GTK_TYPE_SPIN_BUTTON,
                         "visible", TRUE,
                         "activates_default", TRUE,
                         "adjustment", adjustment,
                         "digits", fracts,
                         "width_chars", 0,
                         NULL);
  gxk_widget_add_font_requisition (widget, chars, digits);
  gxk_param_entry_connect_handlers (param, widget, NULL);
  gtk_tooltips_set_tip (GXK_TOOLTIPS, widget, tooltip, NULL);
  return widget;
}

static GxkParamEditor param_spinner1 = {
  { "spinner",          N_("Spin Button"), },
  { G_TYPE_NONE,  NULL, TRUE, TRUE, },  /* all int types and all float types */
  { NULL,         +9,   TRUE, },        /* options, rating, editing */
  param_spinner_create, NULL,   PARAM_SPINNER_LINEAR,
};
static GxkParamEditor param_spinner2 = {
  { "spinner-log",      N_("Spin Button (Logarithmic)"), },
  { G_TYPE_NONE,  NULL, TRUE, TRUE, },  /* all int types and all float types */
  { "log-scale",  +4,   TRUE, },        /* options, rating, editing */
  param_spinner_create, NULL,   PARAM_SPINNER_LINEAR,
};
