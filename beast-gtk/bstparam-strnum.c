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


/* --- stringified number parameters --- */
enum	/* variants */
{
  VTIME = 1,
  VNOTE = 2,
};
static void
param_strnum_change_value (GtkWidget *entry,
			 BstParam  *bparam)
{
  if (!bparam->updating)
    {
      const gchar *string = gtk_entry_get_text (GTK_ENTRY (entry));
      switch (bparam->impl->variant)
	{
	case VTIME:
	  sfi_value_set_time (&bparam->value, sfi_time_from_string (string));
	  break;
	case VNOTE:
	  sfi_value_set_note (&bparam->value, sfi_note_from_string (string));
	  break;
	}
      bst_param_apply_value (bparam);
    }
}

static guint
param_strnum_width (BstParam *bparam)
{
  switch (bparam->impl->variant)
    {
    case VTIME:	return 160;
    case VNOTE:	return 60;
    }
  return 0;
}

static gboolean
param_strnum_focus_out (GtkWidget     *entry,
		      GdkEventFocus *event,
		      BstParam      *bparam)
{
  param_strnum_change_value (entry, bparam);
  return FALSE;
}

static BstGMask*
param_strnum_create_gmask (BstParam    *bparam,
			 const gchar *tooltip,
			 GtkWidget   *gmask_parent)
{
  GtkWidget *action, *prompt, *xframe;
  BstGMask *gmask;
  
  action = g_object_new (GTK_TYPE_ENTRY,
			 "visible", TRUE,
			 "activates_default", TRUE,
			 "width_request", param_strnum_width (bparam),
			 NULL);
  g_object_connect (action,
		    "signal::key_press_event", bst_param_entry_key_press, NULL,
		    "signal::activate", param_strnum_change_value, bparam,
		    "signal::focus_out_event", param_strnum_focus_out, bparam,
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
  
  gmask = bst_gmask_form (gmask_parent, action, bparam->impl->variant == VTIME ? BST_GMASK_INTERLEAVE : 0);
  bst_gmask_set_prompt (gmask, prompt);
  bst_gmask_set_tip (gmask, tooltip);

  return gmask;
}

static GtkWidget*
param_strnum_create_widget (BstParam    *bparam,
			  const gchar *tooltip)
{
  GtkWidget *action;
  
  action = g_object_new (GTK_TYPE_ENTRY,
			 "visible", TRUE,
			 "activates_default", TRUE,
			 "width_request", param_strnum_width (bparam),
			 NULL);
  g_object_connect (action,
		    "signal::key_press_event", bst_param_entry_key_press, NULL,
		    "signal::activate", param_strnum_change_value, bparam,
		    "swapped_signal::focus_out_event", param_strnum_focus_out, bparam,
		    NULL);
  
  return action;
}

static void
param_strnum_update (BstParam  *bparam,
		   GtkWidget *action)
{
  gchar *string = NULL;
  switch (bparam->impl->variant)
    {
    case VTIME:
      string = sfi_time_to_string (sfi_value_get_time (&bparam->value));
      break;
    case VNOTE:
      string = sfi_note_to_string (sfi_value_get_note (&bparam->value));
      break;
    }
  if (!g_str_equal (gtk_entry_get_text (GTK_ENTRY (action)), string))
    {
      gtk_entry_set_text (GTK_ENTRY (action), string);
      gtk_editable_set_position (GTK_EDITABLE (action), bparam->writable ? G_MAXINT : 0);
    }
  g_free (string);
}

struct _BstParamImpl param_time = {
  "Time",		+5 /* rating */,
  VTIME /* variant */,	BST_PARAM_EDITABLE,
  SFI_SCAT_TIME,	NULL /* hints */,
  param_strnum_create_gmask,
  NULL, /* create_widget */
  param_strnum_update,
};

struct _BstParamImpl rack_time = {
  "Time",		+5 /* rating */,
  VTIME /* variant */,	BST_PARAM_EDITABLE,
  SFI_SCAT_TIME,	NULL /* hints */,
  NULL, /* create_gmask */
  param_strnum_create_widget,
  param_strnum_update,
};

struct _BstParamImpl param_note = {
  "Note",		+5 /* rating */,
  VNOTE /* variant */,	BST_PARAM_EDITABLE,
  SFI_SCAT_NOTE,	NULL /* hints */,
  param_strnum_create_gmask,
  NULL, /* create_widget */
  param_strnum_update,
};

struct _BstParamImpl rack_note = {
  "Note",		+5 /* rating */,
  VNOTE /* variant */,	BST_PARAM_EDITABLE,
  SFI_SCAT_NOTE,	NULL /* hints */,
  NULL, /* create_gmask */
  param_strnum_create_widget,
  param_strnum_update,
};
