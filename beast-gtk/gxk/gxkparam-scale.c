/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002 Tim Janik
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


enum {
  VDIAL,
  VKNOB,
  VVSCALE,
  VHSCALE,
};

/* --- numeric parameters --- */
static void
param_scale_change_value (GtkAdjustment *adjustment,
			  BstParam      *bparam)
{
  if (!bparam->updating)
    {
      GValue dvalue = { 0, };
      g_value_init (&dvalue, G_TYPE_DOUBLE);
      g_value_set_double (&dvalue, adjustment->value);
      g_value_transform (&dvalue, &bparam->value);
      g_value_unset (&dvalue);
      bst_param_apply_value (bparam);
    }
}

static GtkWidget*
param_scale_create_widget (BstParam    *bparam,
			   const gchar *tooltip)
{
  GParamSpec *pspec = bparam->pspec;
  gboolean logarithmic = g_option_check (bparam->impl->hints, "log-scale");
  GtkWidget *action;
  gpointer linear_adjustment, adjustment = NULL;

  switch (sfi_categorize_pspec (pspec) & SFI_SCAT_TYPE_MASK)
    {
      SfiInt idefault, iminimum, imaximum, istepping;
      SfiNum ndefault, nminimum, nmaximum, nstepping;
      SfiReal rdefault, rminimum, rmaximum, rstepping;
    case SFI_SCAT_INT:
      idefault = sfi_pspec_get_int_default (pspec);
      sfi_pspec_get_int_range (pspec, &iminimum, &imaximum, &istepping);
      if (!istepping)
	istepping = 1;
      adjustment = gtk_adjustment_new (idefault, iminimum, imaximum,
				       MIN (1, istepping),
				       MAX (1, istepping),
				       0);
      break;
    case SFI_SCAT_NUM:
      ndefault = sfi_pspec_get_num_default (pspec);
      sfi_pspec_get_num_range (pspec, &nminimum, &nmaximum, &nstepping);
      if (!nstepping)
	nstepping = 1;
      adjustment = gtk_adjustment_new (ndefault, nminimum, nmaximum,
				       MIN (1, nstepping),
				       MAX (1, nstepping),
				       0);
      break;
    case SFI_SCAT_REAL:
      rdefault = sfi_pspec_get_real_default (pspec);
      sfi_pspec_get_real_range (pspec, &rminimum, &rmaximum, &rstepping);
      if (rstepping < SFI_MINREAL)
	rstepping = 1;
      adjustment = gtk_adjustment_new (rdefault, rminimum, rmaximum,
				       MIN (0.1, rstepping),
				       MAX (0.1, rstepping),
				       0);
      break;
    default:
      ;
    }

  g_object_ref (adjustment);
  gtk_object_sink (adjustment);
  linear_adjustment = adjustment;

  /* we need to be notified *after* the scale so the
   * scale's value is already updated
   */
  g_object_connect (adjustment,
		    "signal_after::value-changed", param_scale_change_value, bparam,
		    NULL);
  if (logarithmic)
    {
      SfiReal center, base, n_steps;
      if (sfi_pspec_get_log_scale (pspec, &center, &base, &n_steps))
	{
	  GtkAdjustment *log_adjustment = bst_log_adjustment_from_adj (adjustment);
	  g_object_unref (adjustment);
	  adjustment = log_adjustment;
	  g_object_ref (adjustment);
	  gtk_object_sink (adjustment);
	  bst_log_adjustment_setup (adjustment,
				    center, base, n_steps);
	}
    }
  switch (bparam->impl->variant)
    {
    case VDIAL:
      action = g_object_new (BST_TYPE_DIAL, NULL);
      bst_dial_set_adjustment (BST_DIAL (action), adjustment);
      break;
    case VKNOB:
      action = g_object_new (BST_TYPE_KNOB, NULL);
      bst_knob_set_adjustment (BST_KNOB (action), adjustment);
      break;
    case VVSCALE:
      action = g_object_new (GTK_TYPE_VSCALE,
			     "adjustment", adjustment,
			     "draw_value", FALSE,
			     NULL);
      break;
    case VHSCALE:
      action = g_object_new (GTK_TYPE_HSCALE,
			     "adjustment", adjustment,
			     "draw_value", FALSE,
			     NULL);
      break;
    default:
      action = NULL;	/* cure cc */
    }
  g_object_ref (linear_adjustment);
  g_object_set_data_full (G_OBJECT (action), "adjustment", linear_adjustment, (GDestroyNotify) g_object_unref);
  g_object_unref (adjustment);
  g_object_set (action,
		"visible", TRUE,
		"can_focus", FALSE,
		NULL);
  return action;
}

static void
param_scale_update (BstParam  *bparam,
		    GtkWidget *action)
{
  GtkAdjustment *adjustment = g_object_get_data (action, "adjustment");
  GValue dvalue = { 0, };
  g_value_init (&dvalue, G_TYPE_DOUBLE);
  g_value_transform (&bparam->value, &dvalue);
  gtk_adjustment_set_value (adjustment, g_value_get_double (&dvalue));
  g_value_unset (&dvalue);
}

struct _BstParamImpl rack_knob_int = {
  "Knob",		+5 /* rating */,
  VKNOB,		BST_PARAM_EDITABLE,
  SFI_SCAT_INT,		NULL /* hints */,
  NULL, /* create_gmask */
  param_scale_create_widget,
  param_scale_update,
};
struct _BstParamImpl rack_knob_num = {
  "Knob",		+5 /* rating */,
  VKNOB,		BST_PARAM_EDITABLE,
  SFI_SCAT_NUM,		NULL /* hints */,
  NULL, /* create_gmask */
  param_scale_create_widget,
  param_scale_update,
};
struct _BstParamImpl rack_knob_real = {
  "Knob",		+5 /* rating */,
  VKNOB,		BST_PARAM_EDITABLE,
  SFI_SCAT_REAL,	NULL /* hints */,
  NULL, /* create_gmask */
  param_scale_create_widget,
  param_scale_update,
};
struct _BstParamImpl rack_log_knob_int = {
  "Knob (Logarithmic)", +5 /* rating */,
  VKNOB,		BST_PARAM_EDITABLE,
  SFI_SCAT_INT,		"log-scale",
  NULL, /* create_gmask */
  param_scale_create_widget,
  param_scale_update,
};
struct _BstParamImpl rack_log_knob_num = {
  "Knob (Logarithmic)", +5 /* rating */,
  VKNOB,		BST_PARAM_EDITABLE,
  SFI_SCAT_NUM,		"log-scale",
  NULL, /* create_gmask */
  param_scale_create_widget,
  param_scale_update,
};
struct _BstParamImpl rack_log_knob_real = {
  "Knob (Logarithmic)", +5 /* rating */,
  VKNOB,		BST_PARAM_EDITABLE,
  SFI_SCAT_REAL,	"log-scale",
  NULL, /* create_gmask */
  param_scale_create_widget,
  param_scale_update,
};

struct _BstParamImpl rack_dial_int = {
  "Dial",		+5 /* rating */,
  VDIAL,		BST_PARAM_EDITABLE,
  SFI_SCAT_INT,		NULL /* hints */,
  NULL, /* create_gmask */
  param_scale_create_widget,
  param_scale_update,
};
struct _BstParamImpl rack_dial_num = {
  "Dial",		+5 /* rating */,
  VDIAL,		BST_PARAM_EDITABLE,
  SFI_SCAT_NUM,		NULL /* hints */,
  NULL, /* create_gmask */
  param_scale_create_widget,
  param_scale_update,
};
struct _BstParamImpl rack_dial_real = {
  "Dial",		+5 /* rating */,
  VDIAL,		BST_PARAM_EDITABLE,
  SFI_SCAT_REAL,	NULL /* hints */,
  NULL, /* create_gmask */
  param_scale_create_widget,
  param_scale_update,
};
struct _BstParamImpl rack_log_dial_int = {
  "Dial (Logarithmic)", +5 /* rating */,
  VDIAL,		BST_PARAM_EDITABLE,
  SFI_SCAT_INT,		"log-scale",
  NULL, /* create_gmask */
  param_scale_create_widget,
  param_scale_update,
};
struct _BstParamImpl rack_log_dial_num = {
  "Dial (Logarithmic)", +5 /* rating */,
  VDIAL,		BST_PARAM_EDITABLE,
  SFI_SCAT_NUM,		"log-scale",
  NULL, /* create_gmask */
  param_scale_create_widget,
  param_scale_update,
};
struct _BstParamImpl rack_log_dial_real = {
  "Dial (Logarithmic)", +5 /* rating */,
  VDIAL,		BST_PARAM_EDITABLE,
  SFI_SCAT_REAL,	"log-scale",
  NULL, /* create_gmask */
  param_scale_create_widget,
  param_scale_update,
};

struct _BstParamImpl rack_vscale_int = {
  "VScale",		+5 /* rating */,
  VVSCALE,		BST_PARAM_EDITABLE,
  SFI_SCAT_INT,		NULL /* hints */,
  NULL, /* create_gmask */
  param_scale_create_widget,
  param_scale_update,
};
struct _BstParamImpl rack_vscale_num = {
  "VScale",		+5 /* rating */,
  VVSCALE,		BST_PARAM_EDITABLE,
  SFI_SCAT_NUM,		NULL /* hints */,
  NULL, /* create_gmask */
  param_scale_create_widget,
  param_scale_update,
};
struct _BstParamImpl rack_vscale_real = {
  "VScale",		+5 /* rating */,
  VVSCALE,		BST_PARAM_EDITABLE,
  SFI_SCAT_REAL,	NULL /* hints */,
  NULL, /* create_gmask */
  param_scale_create_widget,
  param_scale_update,
};
struct _BstParamImpl rack_log_vscale_int = {
  "VScale (Logarithmic)", +5 /* rating */,
  VVSCALE,		BST_PARAM_EDITABLE,
  SFI_SCAT_INT,		"log-scale",
  NULL, /* create_gmask */
  param_scale_create_widget,
  param_scale_update,
};
struct _BstParamImpl rack_log_vscale_num = {
  "VScale (Logarithmic)", +5 /* rating */,
  VVSCALE,		BST_PARAM_EDITABLE,
  SFI_SCAT_NUM,		"log-scale",
  NULL, /* create_gmask */
  param_scale_create_widget,
  param_scale_update,
};
struct _BstParamImpl rack_log_vscale_real = {
  "VScale (Logarithmic)", +5 /* rating */,
  VVSCALE,		BST_PARAM_EDITABLE,
  SFI_SCAT_REAL,	"log-scale",
  NULL, /* create_gmask */
  param_scale_create_widget,
  param_scale_update,
};

struct _BstParamImpl rack_hscale_int = {
  "HScale",		+5 /* rating */,
  VHSCALE,		BST_PARAM_EDITABLE,
  SFI_SCAT_INT,		NULL /* hints */,
  NULL, /* create_gmask */
  param_scale_create_widget,
  param_scale_update,
};
struct _BstParamImpl rack_hscale_num = {
  "HScale",		+5 /* rating */,
  VHSCALE,		BST_PARAM_EDITABLE,
  SFI_SCAT_NUM,		NULL /* hints */,
  NULL, /* create_gmask */
  param_scale_create_widget,
  param_scale_update,
};
struct _BstParamImpl rack_hscale_real = {
  "HScale",		+5 /* rating */,
  VHSCALE,		BST_PARAM_EDITABLE,
  SFI_SCAT_REAL,	NULL /* hints */,
  NULL, /* create_gmask */
  param_scale_create_widget,
  param_scale_update,
};
struct _BstParamImpl rack_log_hscale_int = {
  "HScale (Logarithmic)", +5 /* rating */,
  VHSCALE,		BST_PARAM_EDITABLE,
  SFI_SCAT_INT,		"log-scale",
  NULL, /* create_gmask */
  param_scale_create_widget,
  param_scale_update,
};
struct _BstParamImpl rack_log_hscale_num = {
  "HScale (Logarithmic)", +5 /* rating */,
  VHSCALE,		BST_PARAM_EDITABLE,
  SFI_SCAT_NUM,		"log-scale",
  NULL, /* create_gmask */
  param_scale_create_widget,
  param_scale_update,
};
struct _BstParamImpl rack_log_hscale_real = {
  "HScale (Logarithmic)", +5 /* rating */,
  VHSCALE,		BST_PARAM_EDITABLE,
  SFI_SCAT_REAL,	"log-scale",
  NULL, /* create_gmask */
  param_scale_create_widget,
  param_scale_update,
};
