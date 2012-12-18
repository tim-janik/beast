/* GXK - Gtk+ Extension Kit
 * Copyright (C) 2002-2003 Tim Janik
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

/* --- label display --- */
enum {
  PARAM_LABEL,
  PARAM_LABEL_IDENT,
  PARAM_LABEL_NAME,
};

static GtkWidget*
param_label_create (GxkParam    *param,
                    const gchar *tooltip,
                    guint        variant)
{
  GtkWidget *widget, *parent = (GtkWidget*) g_object_new (GTK_TYPE_ALIGNMENT,
                                                          "visible", TRUE,
                                                          "xalign", 0.5,
                                                          "yalign", 0.5,
                                                          "xscale", 0.0,
                                                          "yscale", 0.0,
                                                          NULL);
  if (tooltip)
    {
      parent = (GtkWidget*) g_object_new (GTK_TYPE_EVENT_BOX,
                                          "visible", TRUE,
                                          "parent", parent,
                                          NULL);
      gxk_widget_set_tooltip (parent, tooltip);
    }
  widget = (GtkWidget*) g_object_new (GTK_TYPE_LABEL,
                                      "visible", TRUE,
                                      "parent", parent,
                                      NULL);
  if (variant == PARAM_LABEL_IDENT)
    gtk_label_set_text (GTK_LABEL (widget), g_param_spec_get_name (param->pspec));
  if (variant == PARAM_LABEL_NAME)
    gtk_label_set_text (GTK_LABEL (widget), g_param_spec_get_nick (param->pspec));
  return widget;
}

static void
param_label_update (GxkParam  *param,
		    GtkWidget *widget)
{
  gtk_label_set_text (GTK_LABEL (widget), g_value_get_string (&param->value));
}

static GxkParamEditor param_label1 = {
  { "ident",            N_("Property Identifier"), },
  { 0, },
  { NULL,       -101,   FALSE, },       /* options, rating, editing */
  param_label_create,   NULL,   PARAM_LABEL_IDENT,
};
static GxkParamEditor param_label2 = {
  { "name",             N_("Property Name"), },
  { 0, },
  { NULL,       -100,   FALSE, },       /* options, rating, editing */
  param_label_create,   NULL,   PARAM_LABEL_NAME,
};
static GxkParamEditor param_label3 = {
  { "label",            N_("Label"), },
  { G_TYPE_STRING, },
  { NULL,         +0,   FALSE, },       /* options, rating, editing */
  param_label_create,   param_label_update,     PARAM_LABEL,
};
