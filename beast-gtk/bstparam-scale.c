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
#include "bstdial.h"
#include "bstknob.h"

/* --- scale-alike parameter editor --- */
enum {
  PARAM_SCALE_DIAL,
  PARAM_SCALE_KNOB,
  PARAM_SCALE_LOGARITHMIC       = 0x10000
};

static GtkWidget*
param_scale_create (GxkParam    *param,
                    const gchar *tooltip,
                    guint        variant)
{
  GtkWidget *widget = NULL;
  guint svariant = variant & 0xffff;
  guint logarithmic = variant & PARAM_SCALE_LOGARITHMIC;
  GtkAdjustment *adjustment = logarithmic ? gxk_param_get_log_adjustment (param) : NULL;
  if (!logarithmic &&
      (g_param_spec_check_option (param->pspec, "db-volume") ||
       g_param_spec_check_option (param->pspec, "db-range")) &&
      !g_param_spec_check_option (param->pspec, "db-value"))
    adjustment = gxk_param_get_decibel_adjustment (param);
  if (!adjustment)
    adjustment = gxk_param_get_adjustment (param);

  switch (svariant)
    {
    case PARAM_SCALE_DIAL:
      widget = g_object_new (BST_TYPE_DIAL, NULL);
      bst_dial_set_adjustment (BST_DIAL (widget), adjustment);
      break;
    case PARAM_SCALE_KNOB:
      widget = g_object_new (BST_TYPE_KNOB, NULL);
      bst_knob_set_adjustment (BST_KNOB (widget), adjustment);
      break;
    }
  g_object_set (widget,
		"visible", TRUE,
		"can_focus", FALSE,
		NULL);
  gxk_param_add_grab_widget (param, widget);
  gtk_tooltips_set_tip (GXK_TOOLTIPS, widget, tooltip, NULL);
  return widget;
}

static GxkParamEditor param_scale1 = {
  { "knob-lin",         N_("Knob"), },
  { G_TYPE_NONE,  NULL, TRUE, TRUE, },  /* all int types and all float types */
  { NULL,         +5,   TRUE, },        /* options, rating, editing */
  param_scale_create,   NULL,   PARAM_SCALE_KNOB,
};
static GxkParamEditor param_scale2 = {
  { "knob-log",         N_("Knob (Logarithmic)"), },
  { G_TYPE_NONE,  NULL, TRUE, TRUE, },  /* all int types and all float types */
  { "log-scale",  +5,   TRUE, },        /* options, rating, editing */
  param_scale_create,   NULL,   PARAM_SCALE_KNOB | PARAM_SCALE_LOGARITHMIC,
};
static const gchar *param_scale_aliases1[] = {
  "knob",
  "knob-lin", "knob-log",
  NULL,
};

static GxkParamEditor param_scale3 = {
  { "dial-lin",         N_("Dial"), },
  { G_TYPE_NONE,  NULL, TRUE, TRUE, },  /* all int types and all float types */
  { NULL,         +5,   TRUE, },        /* options, rating, editing */
  param_scale_create,   NULL,   PARAM_SCALE_DIAL,
};
static GxkParamEditor param_scale4 = {
  { "dial-log",         N_("Dial (Logarithmic)"), },
  { G_TYPE_NONE,  NULL, TRUE, TRUE, },  /* all int types and all float types */
  { "log-scale",  +5,   TRUE, },        /* options, rating, editing */
  param_scale_create,   NULL,   PARAM_SCALE_DIAL | PARAM_SCALE_LOGARITHMIC,
};
static const gchar *param_scale_aliases2[] = {
  "dial",
  "dial-lin", "dial-log",
  NULL,
};
