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


/* --- numeric parameters --- */
static void
param_spinner_change_value (GtkAdjustment *adjustment,
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

static void
param_spinner_hscale_size_request (GtkWidget      *scale,
				   GtkRequisition *requisition)
{
  gint slider_length = 0, trough_border = 0;

  /* we patch up the scale's minimum size requisition here */
  gtk_widget_style_get (scale, "slider_length", &slider_length, NULL);
  gtk_widget_style_get (scale, "trough_border", &trough_border, NULL);
  requisition->width = slider_length * 1.3;
  requisition->width += 2 * trough_border;
}

static void
param_spinner_setup (BstParam   *bparam,
		     GtkWidget **spinner,
		     GtkWidget **dial,
		     GtkWidget **scale,
		     guint      *width,
		     gboolean   *expandable)
{
  GParamSpec *pspec = bparam->pspec;
  gpointer adjustment = NULL;
  guint n_digits = 0;

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
      n_digits = 0;
      *width = 80;
      *expandable = FALSE;
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
      n_digits = 0;
      *width = 130;
      *expandable = TRUE;
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
      n_digits = 6;
      *width = 90;
      *expandable = TRUE;
      break;
    default:
      ;
    }

  g_object_ref (adjustment);
  gtk_object_sink (GTK_OBJECT (adjustment));
  
  /* we need to be notified *after* the spinner so the
   * spinner's value is already updated
   */
  g_object_connect (adjustment,
		    "signal_after::value-changed", param_spinner_change_value, bparam,
		    NULL);
  *spinner = gtk_spin_button_new (adjustment, 0, n_digits);
  if (dial && sfi_pspec_test_hint (pspec, SFI_PARAM_HINT_DIAL))
    {
      *dial = g_object_new (BST_TYPE_DIAL,
			    "visible", TRUE,
			    "can_focus", FALSE,
			    NULL);
      bst_dial_set_adjustment (BST_DIAL (*dial), adjustment);
    }
  if (scale && (sfi_pspec_test_hint (pspec, SFI_PARAM_HINT_DIAL) ||
		sfi_pspec_test_hint (pspec, SFI_PARAM_HINT_SCALE)))
    {
      GtkAdjustment *scale_adjustment;
      SfiReal center, base, n_steps;
      
      if (sfi_pspec_get_log_scale (pspec, &center, &base, &n_steps))
	{
	  scale_adjustment = bst_log_adjustment_from_adj (adjustment);
	  bst_log_adjustment_setup (BST_LOG_ADJUSTMENT (scale_adjustment),
				    center, base, n_steps);
	  g_object_ref (scale_adjustment);
	  gtk_object_sink (GTK_OBJECT (scale_adjustment));
	}
      else
	scale_adjustment = g_object_ref (adjustment);
      *scale = g_object_new (GTK_TYPE_HSCALE,
			     "visible", TRUE,
			     "adjustment", scale_adjustment,
			     "draw_value", FALSE,
			     "can_focus", FALSE,
			     NULL);
      g_object_connect (*scale,
			"signal_after::size_request", param_spinner_hscale_size_request, NULL,
			NULL);
      g_object_unref (scale_adjustment);
    }
  g_object_unref (adjustment);
}

static BstGMask*
param_spinner_create_gmask (BstParam    *bparam,
			    const gchar *tooltip,
			    GtkWidget   *gmask_parent)
{
  GtkWidget *action = NULL, *scale = NULL, *dial = NULL, *prompt, *xframe;
  gboolean expandable = FALSE;
  guint width = 10;
  BstGMask *gmask;

  param_spinner_setup (bparam, &action, &dial, &scale, &width, &expandable);

  g_object_set (action,
		"visible", TRUE,
		"width_request", width,
		"activates_default", TRUE,
		NULL);
  g_object_connect (action,
		    "signal::key_press_event", bst_param_entry_key_press, NULL,
		    NULL);

  xframe = g_object_new (BST_TYPE_XFRAME,
			 "visible", TRUE,
			 "cover", action,
			 NULL);
  g_object_connect (xframe,
		    "swapped_signal::button_check", bst_param_xframe_check_button, bparam,
		    NULL);
  prompt = g_object_new (GTK_TYPE_LABEL,
			 "visible", TRUE,
			 "label", g_param_spec_get_nick (bparam->pspec),
			 "xalign", 0.0,
			 "parent", xframe,
			 NULL);

  gmask = bst_gmask_form (gmask_parent, action, expandable ? BST_GMASK_FILL : 0);
  bst_gmask_set_prompt (gmask, prompt);
  if (scale)
    bst_gmask_set_aux2 (gmask, scale);
  if (dial)
    bst_gmask_set_aux1 (gmask, dial);
  bst_gmask_set_tip (gmask, tooltip);
  
  return gmask;
}

static GtkWidget*
param_spinner_create_widget (BstParam    *bparam,
			     const gchar *tooltip)
{
  GtkWidget *action;
  gboolean expandable = FALSE;
  guint width = 10;

  param_spinner_setup (bparam, &action, NULL, NULL, &width, &expandable);

  g_object_set (action,
		"visible", TRUE,
		"width_request", width,
		"activates_default", TRUE,
		NULL);
  g_object_connect (action,
		    "signal::key_press_event", bst_param_entry_key_press, bparam,
		    NULL);

  return action;
}

static void
param_spinner_update (BstParam  *bparam,
		      GtkWidget *action)
{
  GtkAdjustment *adjustment = GTK_SPIN_BUTTON (action)->adjustment;
  GValue dvalue = { 0, };
  g_value_init (&dvalue, G_TYPE_DOUBLE);
  g_value_transform (&bparam->value, &dvalue);
  gtk_adjustment_set_value (adjustment, g_value_get_double (&dvalue));
  g_value_unset (&dvalue);
}

struct _BstParamImpl param_spinner_int = {
  "Spinner",		+5 /* rating */,
  0 /* variant */,	BST_PARAM_EDITABLE,
  SFI_SCAT_INT,		NULL /* hints */,
  param_spinner_create_gmask,
  NULL, /* create_widget */
  param_spinner_update,
};

struct _BstParamImpl rack_spinner_int = {
  "Spinner",		+5 /* rating */,
  0 /* variant */,	BST_PARAM_EDITABLE,
  SFI_SCAT_INT,		NULL /* hints */,
  NULL, /* create_gmask */
  param_spinner_create_widget,
  param_spinner_update,
};

struct _BstParamImpl param_spinner_num = {
  "Spinner",		+5 /* rating */,
  0 /* variant */,	BST_PARAM_EDITABLE,
  SFI_SCAT_NUM,		NULL /* hints */,
  param_spinner_create_gmask,
  NULL, /* create_widget */
  param_spinner_update,
};

struct _BstParamImpl rack_spinner_num = {
  "Spinner",		+5 /* rating */,
  0 /* variant */,	BST_PARAM_EDITABLE,
  SFI_SCAT_NUM,		NULL /* hints */,
  NULL, /* create_gmask */
  param_spinner_create_widget,
  param_spinner_update,
};

struct _BstParamImpl param_spinner_real = {
  "Spinner",		+5 /* rating */,
  0 /* variant */,	BST_PARAM_EDITABLE,
  SFI_SCAT_REAL,	NULL /* hints */,
  param_spinner_create_gmask,
  NULL, /* create_widget */
  param_spinner_update,
};

struct _BstParamImpl rack_spinner_real = {
  "Spinner",		+5 /* rating */,
  0 /* variant */,	BST_PARAM_EDITABLE,
  SFI_SCAT_REAL,	NULL /* hints */,
  NULL, /* create_gmask */
  param_spinner_create_widget,
  param_spinner_update,
};
