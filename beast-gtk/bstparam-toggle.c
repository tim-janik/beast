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

enum {
  VRADIOBUTTON,
  VCHECKBUTTON,
  VTOGGLEBUTTON,
};


/* --- boolean parameters --- */
static void
param_toggle_change_value (GtkWidget *toggle,
			   BstParam  *bparam)
{
  if (!bparam->updating)
    {
      sfi_value_set_bool (&bparam->value, GTK_TOGGLE_BUTTON (toggle)->active);
      bst_param_apply_value (bparam);
    }
}

static BstGMask*
param_toggle_create_gmask (BstParam    *bparam,
			   const gchar *tooltip,
			   GtkWidget   *gmask_parent)
{
  GtkWidget *action, *prompt, *xframe;
  BstGMask *gmask;
  gboolean radio = sfi_pspec_test_hint (bparam->pspec, SFI_PARAM_HINT_RADIO);

  action = g_object_new (radio ? BST_TYPE_FREE_RADIO_BUTTON : GTK_TYPE_CHECK_BUTTON,
			 "visible", TRUE,
			 NULL);
  g_object_connect (action,
		    "signal::clicked", param_toggle_change_value, bparam,
		    NULL);
  xframe = g_object_new (BST_TYPE_XFRAME,
			 "visible", TRUE,
			 "parent", action,
			 "cover", action,
			 "steal_button", TRUE,
			 NULL);
  g_object_connect (xframe,
		    "swapped_signal::button_check", bst_param_xframe_check_button, bparam,
		    NULL);
  prompt = g_object_new (GTK_TYPE_LABEL,
			 "visible", TRUE,
			 "label", g_param_spec_get_nick (bparam->pspec),
			 "parent", xframe,
			 NULL);
  gtk_misc_set_alignment (GTK_MISC (prompt), 0, 0.5);
  gmask = bst_gmask_form_big (gmask_parent, action);
  bst_gmask_set_tip (gmask, tooltip);

  return gmask;
}

static GtkWidget*
param_toggle_create_widget (BstParam    *bparam,
			    const gchar *tooltip)
{
  GtkWidget *action, *prompt;

  action = g_object_new (bparam->impl->variant == VCHECKBUTTON ? GTK_TYPE_CHECK_BUTTON :
			 bparam->impl->variant == VRADIOBUTTON ? GTK_TYPE_RADIO_BUTTON :
			 GTK_TYPE_TOGGLE_BUTTON,
			 "visible", TRUE,
			 NULL);
  g_object_connect (action,
		    "signal::clicked", param_toggle_change_value, bparam,
		    NULL);
  prompt = g_object_new (GTK_TYPE_LABEL,
			 "visible", TRUE,
			 "label", bparam->pspec->name,
			 "parent", action,
			 NULL);
  return action;
}

static void
param_toggle_update (BstParam  *bparam,
		     GtkWidget *action)
{
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (action), sfi_value_get_bool (&bparam->value));
}

struct _BstParamImpl param_check_button = {
  "CheckButton",	0 /* rating */,
  0 /* variant */,	BST_PARAM_EDITABLE,
  SFI_SCAT_BOOL,	NULL /* hints */,
  param_toggle_create_gmask,
  NULL, /* create_widget */
  param_toggle_update,
};

struct _BstParamImpl rack_toggle_button = {
  "ToggleButton",	0 /* rating */,
  VTOGGLEBUTTON,	BST_PARAM_EDITABLE,
  SFI_SCAT_BOOL,	NULL /* hints */,
  NULL, /* create_gmask */
  param_toggle_create_widget,
  param_toggle_update,
};

struct _BstParamImpl rack_check_button = {
  "CheckButton",	+1 /* rating */,
  VCHECKBUTTON,		BST_PARAM_EDITABLE,
  SFI_SCAT_BOOL,	NULL /* hints */,
  NULL, /* create_gmask */
  param_toggle_create_widget,
  param_toggle_update,
};

struct _BstParamImpl rack_radio_button = {
  "RadioButton",	0 /* rating */,
  VRADIOBUTTON,		BST_PARAM_EDITABLE,
  SFI_SCAT_BOOL,	"radio" /* hints */,
  NULL, /* create_gmask */
  param_toggle_create_widget,
  param_toggle_update,
};
