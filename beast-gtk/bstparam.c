/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002-2004 Tim Janik
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
#include "bstparam.h"
#include "bstxframe.h"


/* --- prototypes --- */
static gboolean bst_param_xframe_check_button (GxkParam *param,
                                               guint     button);


/* --- variable --- */
static GQuark quark_null_group = 0;
static GQuark quark_param_choice_values = 0;
static guint  param_size_group = 0;


/* --- gmask parameters --- */
static GtkWidget*
param_get_gmask_container (GtkWidget *parent,
                           GQuark     quark_group)
{
  GtkWidget *container = bst_container_get_named_child (parent, quark_group ? quark_group : quark_null_group);
  if (!container || !GTK_IS_CONTAINER (container))
    {
      GtkWidget *any;
      container = bst_gmask_container_create (quark_group ? 5 : 0, FALSE);
      if (quark_group)
        any = g_object_new (GTK_TYPE_FRAME,
                            "visible", TRUE,
                            "child", container,
                            "label-widget", g_object_new (GXK_TYPE_SIMPLE_LABEL,
                                                          "label", g_quark_to_string (quark_group),
                                                          NULL),
                            NULL);
      else
	any = container;
      if (GTK_IS_BOX (parent))
	gtk_box_pack_start (GTK_BOX (parent), any, FALSE, TRUE, 0);
      else if (GTK_IS_WRAP_BOX (parent))
	gtk_container_add_with_properties (GTK_CONTAINER (parent), any,
					   "hexpand", TRUE, "hfill", TRUE,
					   "vexpand", FALSE, "vfill", TRUE,
					   NULL);
      else
	gtk_container_add (GTK_CONTAINER (parent), any);
      bst_container_set_named_child (parent, quark_group ? quark_group : quark_null_group, container);
    }
  return container;
}

static const GxkParamEditorSizes param_editor_homogeneous_sizes = {
  FALSE,        /* may_resize */
  FALSE,        /* request_fractions */
  2, 8,         /* char */
  2, 8,         /* uchar */
  2, 8,         /* int */
  2, 8,         /* uint */
  2, 8,         /* long */
  2, 8,         /* ulong */
  2, 8,         /* int64 */
  2, 8,         /* uint64 */
  2, 8,         /* float */
  2, 8,         /* double */
  9, 3,         /* string */
};

BstGMask*
bst_param_create_gmask (GxkParam    *param,
                        const gchar *editor_name,
                        GtkWidget   *parent)
{
  SfiProxy proxy = bst_param_get_proxy (param);
  const gchar *group;
  GtkWidget *xframe, *action, *prompt = NULL;
  BstGMask *gmask;
  gboolean expand_action;
  gchar *tooltip;
  
  g_return_val_if_fail (GXK_IS_PARAM (param), NULL);
  g_return_val_if_fail (GTK_IS_CONTAINER (parent), NULL);
  
  gxk_param_set_sizes (param_size_group, BST_GCONFIG (size_group_input_fields) ? &param_editor_homogeneous_sizes : NULL);
  group = sfi_pspec_get_group (param->pspec);
  parent = param_get_gmask_container (parent, group ? g_quark_from_string (group) : 0);
  
  action = gxk_param_create_editor (param, editor_name);
  
  xframe = g_object_new (BST_TYPE_XFRAME, "cover", action, NULL);
  g_object_connect (xframe,
                    "swapped_signal::button_check", bst_param_xframe_check_button, param,
                    NULL);
  
  if (GTK_IS_TOGGLE_BUTTON (action))
    {
      /* if there's a prompt widget inside the button already, sneak in xframe */
      if (GTK_BIN (action)->child)
        {
          gtk_widget_reparent (GTK_BIN (action)->child, xframe);
          g_object_set (xframe, "parent", action, "steal_button", TRUE, NULL);
        }
    }
  else
    {
      prompt = g_object_new (GTK_TYPE_LABEL,
                             "visible", TRUE,
                             "label", g_param_spec_get_nick (param->pspec),
                             "xalign", 0.0,
                             "parent", xframe,
                             NULL);
      gxk_param_add_object (param, GTK_OBJECT (prompt));
    }

  expand_action = !prompt || gxk_widget_check_option (action, "hexpand");
  gmask = bst_gmask_form (parent, action, expand_action ? BST_GMASK_BIG : BST_GMASK_INTERLEAVE);
  if (BSE_IS_SOURCE (proxy) && sfi_pspec_check_option (param->pspec, "automate"))
    {
      GtkWidget *automation = gxk_param_create_editor (param, "automation");
      if (prompt)
        {
          GtkBox *hbox = g_object_new (GTK_TYPE_HBOX, "visible", TRUE, NULL);
          gtk_box_pack_start (hbox, gtk_widget_get_toplevel (prompt), FALSE, TRUE, 0);
          gtk_box_pack_end (hbox, automation, FALSE, TRUE, 0);
        }
      else
        prompt = automation;
    }
  if (prompt)
    bst_gmask_set_prompt (gmask, prompt);
  if (sfi_pspec_check_option (param->pspec, "dial"))
    {
      GtkWidget *dial = gxk_param_create_editor (param, "dial");
      bst_gmask_set_aux1 (gmask, dial);
    }
  if (sfi_pspec_check_option (param->pspec, "scale") ||
      sfi_pspec_check_option (param->pspec, "dial") ||
      sfi_pspec_check_option (param->pspec, "note"))
    {
      GtkWidget *scale = gxk_param_create_editor (param, "hscale");
      bst_gmask_set_aux2 (gmask, scale);
    }
  
  tooltip = gxk_param_dup_tooltip (param);
  bst_gmask_set_tip (gmask, tooltip);
  g_free (tooltip);
  bst_gmask_pack (gmask);
  gxk_param_update (param);
  return gmask;
}

/* --- value binding --- */
GxkParam*
bst_param_new_value (GParamSpec          *pspec,
                     GxkParamValueNotify  notify,
                     gpointer             notify_data)
{
  GxkParam *param = gxk_param_new_value (pspec, notify, notify_data);
  if (param)
    gxk_param_set_size_group (param, param_size_group);
  return param;
}

/* --- proxy binding --- */
static void
proxy_binding_set_value (GxkParam     *param,
                         const GValue *value)
{
  SfiProxy proxy = param->bdata[0].v_long;
  if (proxy)
    sfi_glue_proxy_set_property (param->bdata[0].v_long, param->pspec->name, value);
}

static void
proxy_binding_get_value (GxkParam *param,
                         GValue   *value)
{
  SfiProxy proxy = param->bdata[0].v_long;
  if (proxy)
    {
      const GValue *cvalue = sfi_glue_proxy_get_property (param->bdata[0].v_long, param->pspec->name);
      if (cvalue)
	g_value_transform (cvalue, value);
      else
	g_value_reset (value);
    }
  else
    g_value_reset (value);
}

static void
proxy_binding_weakref (gpointer data,
                       SfiProxy junk)
{
  GxkParam *param = data;
  param->bdata[0].v_long = 0;
  param->bdata[1].v_long = 0;	/* already disconnected */
}

static void
proxy_binding_destroy (GxkParam *param)
{
  SfiProxy proxy = param->bdata[0].v_long;
  if (proxy)
    {
      sfi_glue_signal_disconnect (proxy, param->bdata[1].v_long);
      sfi_glue_proxy_weak_unref (proxy, proxy_binding_weakref, param);
      param->bdata[0].v_long = 0;
      param->bdata[1].v_long = 0;
    }
}

static void
proxy_binding_start_grouping (GxkParam *param)
{
  SfiProxy proxy = param->bdata[0].v_long;
  gchar *ustr = g_strconcat ("Modify ", g_param_spec_get_nick (param->pspec), NULL);
  if (proxy)
    bse_item_group_undo (proxy, ustr);
  g_free (ustr);
}

static void
proxy_binding_stop_grouping (GxkParam *param)
{
  SfiProxy proxy = param->bdata[0].v_long;
  if (proxy)
    bse_item_ungroup_undo (proxy);
}

static gboolean
proxy_binding_check_writable (GxkParam *param)
{
  SfiProxy proxy = param->bdata[0].v_long;
  if (proxy)
    return bse_item_editable_property (proxy, param->pspec->name);
  else
    return FALSE;
}

static GxkParamBinding proxy_binding = {
  .n_data_fields        = 2,
  .set_value            = proxy_binding_set_value,
  .get_value            = proxy_binding_get_value,
  .destroy              = proxy_binding_destroy,
  .check_writable       = proxy_binding_check_writable,
  .start_grouping       = proxy_binding_start_grouping,
  .stop_grouping        = proxy_binding_stop_grouping,
};

GxkParam*
bst_param_new_proxy (GParamSpec *pspec,
                     SfiProxy    proxy)
{
  GxkParam *param = gxk_param_new (pspec, &proxy_binding, (gpointer) FALSE);
  bst_param_set_proxy (param, proxy);
  gxk_param_set_size_group (param, param_size_group);
  return param;
}

void
bst_param_set_proxy (GxkParam *param,
                     SfiProxy  proxy)
{
  g_return_if_fail (GXK_IS_PARAM (param));
  g_return_if_fail (param->binding == &proxy_binding);
  
  proxy_binding_destroy (param);
  param->bdata[0].v_long = proxy;
  if (proxy)
    {
      gchar *sig = g_strconcat ("property-notify::", param->pspec->name, NULL);
      param->bdata[1].v_long = sfi_glue_signal_connect_swapped (proxy, sig, gxk_param_update, param);
      g_free (sig);
      sfi_glue_proxy_weak_ref (proxy, proxy_binding_weakref, param);
    }
}

SfiProxy
bst_param_get_proxy (GxkParam *param)
{
  g_return_val_if_fail (GXK_IS_PARAM (param), 0);
  
  if (param->binding == &proxy_binding)
    return param->bdata[0].v_long;
  return 0;
}


/* --- record binding --- */
static void
record_binding_set_value (GxkParam     *param,
			  const GValue *value)
{
  sfi_rec_set (param->bdata[0].v_pointer, param->pspec->name, value);
}

static void
record_binding_get_value (GxkParam *param,
			  GValue   *value)
{
  const GValue *cvalue = sfi_rec_get (param->bdata[0].v_pointer, param->pspec->name);
  if (cvalue)
    g_value_transform (cvalue, value);
  else
    g_value_reset (value);
}

static void
record_binding_destroy (GxkParam *param)
{
  sfi_rec_unref (param->bdata[0].v_pointer);
  param->bdata[0].v_pointer = NULL;
}

static GxkParamBinding record_binding = {
  1, NULL,
  record_binding_set_value,
  record_binding_get_value,
  record_binding_destroy,
  NULL,	/* check_writable */
};

GxkParam*
bst_param_new_rec (GParamSpec *pspec,
                   SfiRec     *rec)
{
  GxkParam *param = gxk_param_new (pspec, &record_binding, (gpointer) FALSE);
  g_return_val_if_fail (rec != NULL, NULL);
  param->bdata[0].v_pointer = sfi_rec_ref (rec);
  gxk_param_set_size_group (param, param_size_group);
  return param;
}


/* --- param implementation utils --- */
static gboolean
bst_param_xframe_check_button (GxkParam *param,
                               guint     button)
{
  g_return_val_if_fail (GXK_IS_PARAM (param), FALSE);
#if 0
  if (bparam->binding->rack_item)
    {
      SfiProxy item = bparam->binding->rack_item (bparam);
      if (BSE_IS_ITEM (item))
	{
	  SfiProxy project = bse_item_get_project (item);
	  if (project)
	    {
	      BstApp *app = bst_app_find (project);
	      if (app && app->rack_editor && BST_RACK_EDITOR (app->rack_editor)->rtable->edit_mode)
		{
		  if (button == 1)
		    bst_rack_editor_add_property (BST_RACK_EDITOR (app->rack_editor), item, bparam->pspec->name);
		  return TRUE;
		}
	    }
	}
    }
#endif
  return FALSE;
}


/* --- param editor registration --- */
#include "bstparam-choice.c"
#include "bstparam-color-spinner.c"
#include "bstparam-note-sequence.c"
#include "bstparam-note-spinner.c"
#include "bstparam-proxy.c"
#include "bstparam-item-seq.c"
#include "bstparam-scale.c"
#include "bstparam-searchpath.c"
#include "bstparam-time.c"
#include "bstparam-automation.c"
void
_bst_init_params (void)
{
  g_assert (quark_null_group == 0);

  quark_null_group = g_quark_from_static_string ("bst-param-null-group");
  quark_param_choice_values = g_quark_from_static_string ("bst-param-choice-values");
  param_size_group = gxk_param_create_size_group ();
  gxk_param_register_editor (&param_choice1, NULL);
  gxk_param_register_editor (&param_choice2, NULL);
  gxk_param_register_editor (&param_choice3, NULL);
  gxk_param_register_editor (&param_choice4, NULL);
  gxk_param_register_aliases (param_choice_aliases1);
  gxk_param_register_editor (&param_color_spinner, NULL);
  gxk_param_register_editor (&param_note_sequence, NULL);
  gxk_param_register_editor (&param_note_spinner, NULL);
  gxk_param_register_editor (&param_proxy, NULL);
  gxk_param_register_editor (&param_item_seq, NULL);
  gxk_param_register_editor (&param_automation, NULL);
  gxk_param_register_editor (&param_scale1, NULL);
  gxk_param_register_editor (&param_scale2, NULL);
  gxk_param_register_editor (&param_scale3, NULL);
  gxk_param_register_editor (&param_scale4, NULL);
  gxk_param_register_aliases (param_scale_aliases1);
  gxk_param_register_aliases (param_scale_aliases2);
  gxk_param_register_editor (&param_searchpath, NULL);
  gxk_param_register_editor (&param_filename, NULL);
  gxk_param_register_editor (&param_time, NULL);
}
