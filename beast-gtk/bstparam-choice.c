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


/* --- choice parameters --- */
static void
param_choice_change_value (GtkWidget *action,
			   BstParam  *bparam)
{
  if (!bparam->updating)
    {
      GtkWidget *item = GTK_OPTION_MENU (action)->menu_item;
      if (item)
	{
          SfiChoiceValue *cv = g_object_get_qdata (G_OBJECT (item), quark_param_choice_values);
	  sfi_value_set_choice (&bparam->value, cv->choice_name);
	}
      bst_param_apply_value (bparam);
    }
}

static GtkWidget*
param_choice_create_action (BstParam *bparam)
{
  SfiChoiceValues cvalues = sfi_pspec_get_choice_values (bparam->pspec);
  GtkWidget *action = gtk_option_menu_new ();
  GtkContainer *menu;
  guint i;

  g_object_set (action,
		"visible", TRUE,
		NULL);
  g_object_connect (action,
		    "signal::button_press_event", bst_param_ensure_focus, NULL,
		    "signal::changed", param_choice_change_value, bparam,
		    NULL);

  menu = g_object_new (GTK_TYPE_MENU,
		       NULL);
  gtk_menu_set_accel_path (GTK_MENU (menu), "<BEAST-Param>/ChoicePopup");
  for (i = 0; i < cvalues.n_values; i++)
    {
      GtkWidget *item = gtk_menu_item_new_with_label (cvalues.values[i].choice_blurb);
      gtk_widget_show (item);
      g_object_set_qdata (G_OBJECT (item), quark_param_choice_values, (gpointer) &cvalues.values[i]);
      gtk_container_add (menu, item);
    }

  gtk_option_menu_set_menu (GTK_OPTION_MENU (action), GTK_WIDGET (menu));

  return action;
}

static BstGMask*
param_choice_create_gmask (BstParam    *bparam,
			   const gchar *tooltip,
			   GtkWidget   *gmask_parent)
{
  GtkWidget *action, *prompt, *xframe;
  BstGMask *gmask;

  action = param_choice_create_action (bparam);

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
  
  gmask = bst_gmask_form (gmask_parent, action, BST_GMASK_INTERLEAVE);
  bst_gmask_set_prompt (gmask, prompt);
  bst_gmask_set_tip (gmask, tooltip);
  
  return gmask;
}

static GtkWidget*
param_choice_create_widget (BstParam    *bparam,
			    const gchar *tooltip)
{
  GtkWidget *action = param_choice_create_action (bparam);

  return action;
}

static void
param_choice_update (BstParam  *bparam,
		     GtkWidget *action)
{
  GtkWidget *menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (action));
  const gchar *string = sfi_value_get_choice (&bparam->value);

  if (menu && string)
    {
      GList *list;
      guint n = 0;
      for (list = GTK_MENU_SHELL (menu)->children; list; list = list->next)
	{
	  GtkWidget *item = list->data;
	  SfiChoiceValue *cv = g_object_get_qdata (G_OBJECT (item), quark_param_choice_values);
	  if (sfi_choice_match (cv->choice_name, string))
	    {
	      gtk_option_menu_set_history (GTK_OPTION_MENU (action), n);
	      break;
	    }
	  n++;
	}
    }
}

struct _BstParamImpl param_choice = {
  "Choice",		+5 /* rating */,
  0 /* variant */,	BST_PARAM_EDITABLE,
  SFI_SCAT_CHOICE,	NULL /* hints */,
  param_choice_create_gmask,
  NULL, /* create_widget */
  param_choice_update,
};

struct _BstParamImpl rack_choice = {
  "Choice",		+5 /* rating */,
  0 /* variant */,	BST_PARAM_EDITABLE,
  SFI_SCAT_CHOICE,	NULL /* hints */,
  NULL, /* create_gmask */
  param_choice_create_widget,
  param_choice_update,
};
