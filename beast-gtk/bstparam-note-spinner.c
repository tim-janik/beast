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
param_note_spinner_change_value (GtkAdjustment *adjustment,
				 BstParam      *bparam)
{
  if (!bparam->updating)
    {
      sfi_value_set_note (&bparam->value, adjustment->value);
      bst_param_apply_value (bparam);
    }
}

static GtkWidget*
param_note_spinner_setup (BstParam *bparam)
{
  GParamSpec *pspec = bparam->pspec;
  SfiInt iminimum, imaximum, istepping;
  gpointer adjustment;
  GtkWidget *spinner;
  
  sfi_pspec_get_int_range (pspec, &iminimum, &imaximum, &istepping);
  adjustment = gtk_adjustment_new (sfi_pspec_get_int_default (pspec),
				   iminimum, imaximum,
				   MIN (1, istepping),
				   MAX (12, istepping),
				   0);
  spinner = gtk_spin_button_new (adjustment, 0, 0);
  
  /* we need to be notified *after* the spinner so the
   * spinner's value is already updated
   */
  g_object_connect (adjustment,
		    "signal_after::value-changed", param_note_spinner_change_value, bparam,
		    NULL);
  return spinner;
}

static BstGMask*
param_note_spinner_create_gmask (BstParam    *bparam,
				 const gchar *tooltip,
				 GtkWidget   *gmask_parent)
{
  GtkWidget *action = NULL, *prompt, *xframe;
  BstGMask *gmask;
  
  action = param_note_spinner_setup (bparam);
  g_object_set (action,
		"visible", TRUE,
		"width_request", 60,
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
  
  gmask = bst_gmask_form (gmask_parent, action, 0);
  bst_gmask_set_prompt (gmask, prompt);
  bst_gmask_set_tip (gmask, tooltip);
  
  return gmask;
}

static GtkWidget*
param_note_spinner_create_widget (BstParam    *bparam,
				  const gchar *tooltip)
{
  GtkWidget *action = param_note_spinner_setup (bparam);
  
  g_object_set (action,
		"visible", TRUE,
		"width_request", 60,
		"activates_default", TRUE,
		NULL);
  g_object_connect (action,
		    "signal::key_press_event", bst_param_entry_key_press, bparam,
		    NULL);
  
  return action;
}

static void
param_note_spinner_update (BstParam  *bparam,
			   GtkWidget *action)
{
  GtkAdjustment *adjustment = GTK_SPIN_BUTTON (action)->adjustment;
  gtk_adjustment_set_value (adjustment, sfi_value_get_note (&bparam->value));
}

struct _BstParamImpl param_note_spinner = {
  "NoteSpinner",	+4 /* rating */,
  0 /* variant */,	BST_PARAM_EDITABLE,
  SFI_SCAT_NOTE,	NULL /* hints */,
  param_note_spinner_create_gmask,
  NULL, /* create_widget */
  param_note_spinner_update,
};

struct _BstParamImpl rack_note_spinner = {
  "NoteSpinner",	+4 /* rating */,
  0 /* variant */,	BST_PARAM_EDITABLE,
  SFI_SCAT_NOTE,	NULL /* hints */,
  NULL, /* create_gmask */
  param_note_spinner_create_widget,
  param_note_spinner_update,
};
