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


/* --- pspec name display --- */
static BstGMask*
param_pspec_create_gmask (BstParam    *bparam,
			  const gchar *tooltip,
			  GtkWidget   *gmask_parent)
{
  GtkWidget *xframe, *prompt;

  xframe = g_object_new (BST_TYPE_XFRAME,
			 "visible", TRUE,
			 NULL);
  g_object_connect (xframe,
		    "swapped_signal::button_check", bst_param_xframe_check_button, bparam,
		    NULL);
  prompt = g_object_new (GTK_TYPE_LABEL,
			 "visible", TRUE,
			 "xalign", 0.0,
			 "parent", xframe,
			 NULL);
  return bst_gmask_form_big (gmask_parent, xframe);
}

static GtkWidget*
param_pspec_create_widget (BstParam    *bparam,
			   const gchar *tooltip)
{
  return g_object_new (GTK_TYPE_LABEL,
		       "visible", TRUE,
		       "xalign", 0.5,
		       NULL);
}

static void
param_pspec_update (BstParam  *bparam,
		    GtkWidget *action)
{
  if (!GTK_IS_LABEL (action))
    {
      /* label is xframe's child */
      action = GTK_BIN (action)->child;
    }
  gtk_label_set_text (GTK_LABEL (action), g_param_spec_get_nick (bparam->pspec));
}

struct _BstParamImpl param_pspec = {
  "Property Name",	-100,
  0 /* variant */,	0    /* flags */,
  0 /* scat */,		NULL /* hints */,
  param_pspec_create_gmask,
  NULL,	/* create_widget */
  param_pspec_update,
};

struct _BstParamImpl rack_pspec = {
  "Property Name",	-100,
  0 /* variant */,	0    /* flags */,
  0 /* scat */,		NULL /* hints */,
  NULL, /* create_gmask */
  param_pspec_create_widget,
  param_pspec_update,
};
