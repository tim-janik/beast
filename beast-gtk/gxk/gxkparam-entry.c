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


/* --- string parameters --- */
static void
param_entry_change_value (GtkWidget *entry,
			  BstParam  *bparam)
{
  if (!bparam->updating)
    {
      const gchar *string = gtk_entry_get_text (GTK_ENTRY (entry));
      sfi_value_set_string (&bparam->value, string);
      bst_param_apply_value (bparam);
    }
}

static gboolean
param_entry_focus_out (GtkWidget     *entry,
		       GdkEventFocus *event,
		       BstParam      *bparam)
{
  param_entry_change_value (entry, bparam);
  return FALSE;
}

static BstGMask*
param_entry_create_gmask (BstParam    *bparam,
			  const gchar *tooltip,
			  GtkWidget   *gmask_parent)
{
  GtkWidget *action, *prompt, *xframe;
  BstGMask *gmask;

  action = g_object_new (GTK_TYPE_ENTRY,
			 "visible", TRUE,
			 "activates_default", TRUE,
			 NULL);
  g_object_connect (action,
		    "signal::key_press_event", bst_param_entry_key_press, NULL,
		    "signal::activate", param_entry_change_value, bparam,
		    "signal::focus_out_event", param_entry_focus_out, bparam,
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

  gmask = bst_gmask_form (gmask_parent, action, BST_GMASK_FILL);
  bst_gmask_set_prompt (gmask, prompt);
  bst_gmask_set_tip (gmask, tooltip);

  return gmask;
}

static GtkWidget*
param_entry_create_widget (BstParam    *bparam,
			   const gchar *tooltip)
{
  GtkWidget *action;

  action = g_object_new (GTK_TYPE_ENTRY,
			 "visible", TRUE,
			 "activates_default", TRUE,
			 NULL);
  g_object_connect (action,
		    "signal::key_press_event", bst_param_entry_key_press, NULL,
		    "signal::activate", param_entry_change_value, bparam,
		    "signal::focus_out_event", param_entry_focus_out, bparam,
		    NULL);

  return action;
}

static void
param_entry_update (BstParam  *bparam,
		    GtkWidget *action)
{
  const gchar *string = sfi_value_get_string (&bparam->value);
  if (!string)
    string = "";
  if (!g_str_equal (gtk_entry_get_text (GTK_ENTRY (action)), string))
    {
      gtk_entry_set_text (GTK_ENTRY (action), string);
      gtk_editable_set_position (GTK_EDITABLE (action), G_MAXINT);
    }
}

struct _BstParamImpl param_entry = {
  "Entry",		+5 /* rating */,
  0 /* variant */,	BST_PARAM_EDITABLE,
  SFI_SCAT_STRING,	NULL /* hints */,
  param_entry_create_gmask,
  NULL, /* create_widget */
  param_entry_update,
};

struct _BstParamImpl rack_entry = {
  "Entry",		+5 /* rating */,
  0 /* variant */,	BST_PARAM_EDITABLE,
  SFI_SCAT_STRING,	NULL /* hints */,
  NULL, /* create_gmask */
  param_entry_create_widget,
  param_entry_update,
};
