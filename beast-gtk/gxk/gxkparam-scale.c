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

/* --- h/v scale editors --- */
enum {
  PARAM_SCALE_HORIZONTAL,
  PARAM_LSCALE_HORIZONTAL,
  PARAM_SCALE_VERTICAL,
  PARAM_LSCALE_VERTICAL,
};

static void
param_scale_hscale_size_request (GtkWidget      *scale,
                                 GtkRequisition *requisition)
{
  gint slider_length = 0, trough_border = 0;
  /* we patch up the scale's minimum size requisition here */
  gtk_widget_style_get (scale, "slider_length", &slider_length, NULL);
  gtk_widget_style_get (scale, "trough_border", &trough_border, NULL);
  requisition->width = slider_length * 1.5;
  requisition->width += 2 * trough_border;
}

static void
param_scale_vscale_size_request (GtkWidget      *scale,
                                 GtkRequisition *requisition)
{
  gint slider_length = 0, trough_border = 0;
  /* we patch up the scale's minimum size requisition here */
  gtk_widget_style_get (scale, "slider_length", &slider_length, NULL);
  gtk_widget_style_get (scale, "trough_border", &trough_border, NULL);
  requisition->height = slider_length * 1.5;
  requisition->height += 2 * trough_border;
}

static GtkWidget*
param_scale_create (GxkParam    *param,
                    const gchar *tooltip,
                    guint        variant)
{
  GtkWidget *widget;
  GtkAdjustment *adjustment = NULL;
  if (variant == PARAM_LSCALE_HORIZONTAL || variant == PARAM_LSCALE_VERTICAL)
    adjustment = gxk_param_get_log_adjustment (param);
  if (!adjustment)
    adjustment = gxk_param_get_adjustment (param);
  widget = g_object_new (variant >= PARAM_SCALE_VERTICAL ? GTK_TYPE_VSCALE : GTK_TYPE_HSCALE,
                         "adjustment", adjustment,
                         "draw_value", FALSE,
                         "visible", TRUE,
                         "can_focus", FALSE,
                         NULL);
  g_object_connect (widget,
                    "signal_after::size_request",
                    variant >= PARAM_SCALE_VERTICAL ? param_scale_vscale_size_request : param_scale_hscale_size_request,
                    NULL, NULL);
  gtk_tooltips_set_tip (GXK_TOOLTIPS, widget, tooltip, NULL);
  if (variant >= PARAM_SCALE_VERTICAL)
    gxk_widget_add_option (widget, "vexpand", "+");
  else
    gxk_widget_add_option (widget, "hexpand", "+");
  return widget;
}

static GxkParamEditor param_scale1 = {
  { "hscale-lin",       N_("Horizontal Scale"), },
  { G_TYPE_NONE,  NULL, TRUE, TRUE, },  /* all int types and all float types */
  { NULL,         +5,   TRUE, },        /* options, rating, editing */
  param_scale_create,   NULL,   PARAM_SCALE_HORIZONTAL,
};
static GxkParamEditor param_scale2 = {
  { "vscale-lin",       N_("Vertical Scale"), },
  { G_TYPE_NONE,  NULL, TRUE, TRUE, },  /* all int types and all float types */
  { NULL,         +5,   TRUE, },        /* options, rating, editing */
  param_scale_create,   NULL,   PARAM_SCALE_VERTICAL,
};
static GxkParamEditor param_scale3 = {
  { "hscale-log",       N_("Horizontal Scale (Logarithmic)"), },
  { G_TYPE_NONE,  NULL, TRUE, TRUE, },  /* all int types and all float types */
  { "log-scale",  +5,   TRUE, },        /* options, rating, editing */
  param_scale_create,   NULL,   PARAM_LSCALE_HORIZONTAL,
};
static GxkParamEditor param_scale4 = {
  { "vscale-log",       N_("Vertical Scale (Logarithmic)"), },
  { G_TYPE_NONE,  NULL, TRUE, TRUE, },  /* all int types and all float types */
  { "log-scale",  +5,   TRUE, },        /* options, rating, editing */
  param_scale_create,   NULL,   PARAM_LSCALE_VERTICAL,
};
static const gchar *param_scale_aliases1[] = {
  "hscale",
  "hscale-lin", "hscale-log",
  NULL,
};
static const gchar *param_scale_aliases2[] = {
  "vscale",
  "vscale-lin", "vscale-log",
  NULL,
};
