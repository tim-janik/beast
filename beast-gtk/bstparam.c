/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999, 2000, 2001 Tim Janik and Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include	"bstparam.h"

#include	"bstdial.h"
#include	"bstxframe.h"
#include	"bstsequence.h"
#include	"bstlogadjustment.h"
#include	"bstapp.h"
#include	"bstrackeditor.h"
#include	<stdlib.h>
#include	<math.h>
#include	<string.h>


#undef	DEBUG_ADJUSTMENT


/* --- structures --- */
typedef struct _DotAreaData DotAreaData;
struct _DotAreaData
{
  gint cdot;
  gboolean entered;
};

/* --- prototypes --- */
static gint dots_area_configure_event	(GtkWidget	    *widget,
					 GdkEventConfigure  *event,
					 BstParam	    *bparam);
static gint dots_area_expose_event	(GtkWidget	    *widget,
					 GdkEventExpose	    *event,
					 BstParam	    *bparam);
static gint dots_area_cross_event	(GtkWidget	    *widget,
					 GdkEventCrossing   *event,
					 BstParam	    *bparam);
static gint dots_area_button_event	(GtkWidget	    *widget,
					 GdkEventButton	    *event,
					 BstParam	    *bparam);
static gint dots_area_motion_event	(GtkWidget	    *widget,
					 GdkEventMotion	    *event,
					 BstParam	    *bparam);


/* --- variables --- */
static GQuark quark_evalues = 0;
static GQuark quark_fvalues = 0;


/* --- functions --- */
static void
bst_string_toggle (GtkToggleButton *tb,
		   GtkWidget	   *widget)
{
  gtk_editable_set_editable (GTK_EDITABLE (widget), tb->active);
  if (tb->active)
    gtk_widget_grab_focus (widget);
}

static void
bst_param_gtk_changed (BstParam *bparam)
{
  if (!bparam->locked)
    bst_param_set (bparam);
}

static void
bst_param_gtk_update (BstParam *bparam)
{
  if (!bparam->locked)
    bst_param_get (bparam);
}

static void
bst_param_entry_activate (GtkWidget *widget,
			  BstParam  *bparam)
{
  static gboolean block_me = FALSE;
  GtkEntry *entry;

  entry = GTK_ENTRY (widget);
  bst_param_gtk_changed (bparam);
  if (!block_me)
    {
      block_me = TRUE;
      gtk_toplevel_activate_default (widget);
      block_me = FALSE;
    }
}

static gboolean
bst_param_focus_out (BstParam *bparam)
{
  bst_param_gtk_changed (bparam);
  return FALSE;
}

static gboolean
focus_on_event (GtkWidget *widget)
{
  gtk_widget_grab_focus (widget);
  return FALSE;
}

static gboolean
bst_entry_key_press (GtkEntry	 *entry,
		     GdkEventKey *event,
		     BstParam	 *bparam)
{
  GtkEditable *editable = GTK_EDITABLE (entry);
  gboolean intercept = FALSE;
  
  if (event->state & GDK_MOD1_MASK)
    switch (event->keyval)
      {
      case 'b': /* check gtk_move_backward_word() */
	intercept = gtk_editable_get_position (editable) <= 0;
	break;
      case 'd': /* gtk_delete_forward_word() */
	intercept = TRUE;
	break;
      case 'f': /* check gtk_move_forward_word() */
	intercept = gtk_editable_get_position (editable) >= entry->text_length;
	break;
      default:
	break;
      }
  
  return intercept;
}

static inline gchar*
str_diff (gchar *s,
	  gchar *m)
{
  while (*s && *m)
    {
      if (*s != *m)
	break;
      s++;
      m++;
    }
  return s;
}

static void
bst_param_update_clue_hunter (BstParam *bparam)
{
  gpointer group = bparam->group;
  GParamSpec *pspec = bparam->pspec;
  GtkWidget *action = bst_gmask_get_action (group);
  GtkClueHunter *ch = gtk_object_get_user_data (GTK_OBJECT (action));
  
  g_return_if_fail (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_OBJECT && GTK_IS_CLUE_HUNTER (ch));
  
  if (bparam->is_object && BSE_IS_ITEM (bparam->owner) &&
      g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (pspec), BSE_TYPE_ITEM))
    {
      BseItem *item = BSE_ITEM (bparam->owner);
      BseProject *project = bse_item_get_project (item);
      gchar *p, *prefix = NULL;
      guint l;
      BswVIter *iter;
      
      gtk_clue_hunter_remove_matches (ch, "*");

      iter = bsw_project_list_uloc_paths (BSE_OBJECT_ID (project), G_PARAM_SPEC_VALUE_TYPE (pspec));
      if (bsw_viter_n_left (iter) > 1)
	{
	  prefix = g_strdup (bsw_viter_get_string (iter));

	  for (bsw_viter_next (iter); bsw_viter_n_left (iter); bsw_viter_next (iter))
	    {
	      p = str_diff (prefix, bsw_viter_get_string (iter));
	      *p = 0;
	    }
	  p = strrchr (prefix, '.');
	  if (p)
	    *(++p) = 0;
	  else
	    {
	      g_free (prefix);
	      prefix = NULL;
	    }
	}
      l = prefix ? strlen (prefix) : 0;

      for (bsw_viter_rewind (iter); bsw_viter_n_left (iter); bsw_viter_next (iter))
	gtk_clue_hunter_add_string (ch, bsw_viter_get_string (iter) + l);
      for (bsw_viter_rewind (iter); bsw_viter_n_left (iter); bsw_viter_next (iter))
	g_print (" %s %s\n", prefix, bsw_viter_get_string (iter) + l);
      g_object_set_data_full (G_OBJECT (ch), "prefix", prefix, g_free);
      bsw_viter_free (iter);
    }
}

static gboolean
xframe_check_button (BstParam *bparam,
		     guint     button)
{
  BswProxy item;

  if (!bparam->is_object || !bparam->owner)
    return FALSE;

  item = BSE_OBJECT_ID (bparam->owner);
  if (BSW_IS_ITEM (item))
    {
      BswProxy project = bsw_item_get_project (item);

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
  return FALSE;
}

static void
bst_bparam_bse_changed (BstParam   *bparam,
			GParamSpec *pspec)
{
  if (bparam->pspec == pspec && !bparam->locked)
    bst_param_get (bparam);
}

void
bst_param_destroy (BstParam *bparam)
{
  g_return_if_fail (G_IS_VALUE (bparam));
  g_return_if_fail (G_IS_PARAM_SPEC (bparam->pspec));
  g_return_if_fail (bparam->locked == 0);
  
  bst_gmask_destroy (bparam->group);
}

static void
bst_param_free (BstParam *bparam)
{
  if (bparam->is_object)
    bst_param_set_object (bparam, NULL);
  else if (bparam->is_procedure)
    bst_param_set_procedure (bparam, NULL);
  
  g_value_unset (&bparam->value);
  g_param_spec_unref (bparam->pspec);
  
  g_free (bparam);
}

void
bst_param_set_procedure (BstParam	   *bparam,
			 BseProcedureClass *proc)
{
  g_return_if_fail (G_IS_VALUE (bparam));
  g_return_if_fail (G_IS_PARAM_SPEC (bparam->pspec));
  g_return_if_fail (!bparam->locked);
  g_return_if_fail (bparam->is_procedure);
  if (proc)
    g_return_if_fail (BSE_IS_PROCEDURE_CLASS (proc));
  
  if (bparam->owner)
    bse_procedure_unref (bparam->owner);
  
  bparam->owner = proc;
  if (bparam->owner)
    {
      /* we SHOULD make sure here that bparam->param->pspec is a valid pspec
       * for proc, but actually i don't feel like writing the extra code just
       * to issue a warning
       */
      bse_procedure_ref (bparam->owner);
      if (bparam->group)
	bst_param_get (bparam);
    }
}

static void
bparam_reset_object (BstParam *bparam)
{
  bst_param_set_object (bparam, NULL);
}

static void
hscale_size_request (GtkWidget	    *scale,
		     GtkRequisition *requisition)
{
  gint slider_length, trough_border;

  gtk_widget_style_get (scale, "slider_length", &slider_length, NULL);
  gtk_widget_style_get (scale, "trough_border", &trough_border, NULL);
  requisition->width = slider_length;
  requisition->width += 2 * trough_border;
}

void
bst_param_set_object (BstParam	*bparam,
		      BseObject *object)
{
  g_return_if_fail (G_IS_VALUE (bparam));
  g_return_if_fail (G_IS_PARAM_SPEC (bparam->pspec));
  g_return_if_fail (!bparam->locked);
  g_return_if_fail (bparam->is_object);
  if (object)
    g_return_if_fail (BSE_IS_OBJECT (object));
  
  if (bparam->owner)
    g_object_disconnect (bparam->owner,
			 "any_signal", bst_bparam_bse_changed, bparam,
			 "any_signal", bparam_reset_object, bparam,
			 NULL);
  
  bparam->owner = object;
  if (bparam->owner)
    {
      /* we SHOULD make sure here that bparam->param->pspec is a valid pspec
       * for object->class (or its anchestors), but actually i don't feel
       * like writing the extra code just to issue a warning
       */
      g_object_connect (bparam->owner,
			"swapped_signal::notify", bst_bparam_bse_changed, bparam,
			"swapped_signal::destroy", bparam_reset_object, bparam,
			NULL);
      if (bparam->group)
	bst_param_get (bparam);
    }
}

BstParam*
bst_param_create (gpointer	owner,
		  GType		owner_type,
		  GParamSpec   *pspec,
		  const gchar  *param_group,
		  GtkWidget    *parent,
		  GtkTooltips  *tooltips)
{
  static GQuark null_group = 0;
  GtkAdjustment *adjustment = NULL;
  GtkWidget *parent_container;
  GtkWidget *spinner = NULL;
  GtkWidget *scale = NULL;
  GtkWidget *dial = NULL;
  gpointer group;
  GQuark param_group_quark = param_group ? g_quark_from_string (param_group) : 0;
  BstParam *bparam;
  guint digits = 0;
  gboolean read_only, string_toggle, radio, expandable;
  gchar *name, *tooltip;
  
  if (BSE_TYPE_IS_PROCEDURE (owner_type))
    g_return_val_if_fail (BSE_IS_PROCEDURE_CLASS (owner), NULL);
  else
    g_return_val_if_fail (BSE_IS_OBJECT (owner), NULL);
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (GTK_IS_TOOLTIPS (tooltips), NULL);
  
  if (!null_group)
    {
      null_group = g_quark_from_static_string ("Bst-null-group");
      quark_evalues = g_quark_from_static_string ("Bst-enum-values");
      quark_fvalues = g_quark_from_static_string ("Bst-flags-values");
    }
  
  bparam = g_new0 (BstParam, 1);
  bparam->pspec = g_param_spec_ref (pspec);
  g_value_init (&bparam->value, G_PARAM_SPEC_VALUE_TYPE (bparam->pspec));
  g_param_value_set_default (bparam->pspec, &bparam->value);
  bparam->owner = NULL;
  if (BSE_TYPE_IS_PROCEDURE (owner_type))
    {
      bparam->is_procedure = TRUE;
      bst_param_set_procedure (bparam, owner);
    }
  else
    {
      bparam->is_object = TRUE;
      bst_param_set_object (bparam, owner);
    }
  bparam->locked = 1;
  g_type_class_ref (owner_type);
  bparam->editable = TRUE;
  
  parent_container = bst_container_get_named_child (parent, param_group_quark ? param_group_quark : null_group);
  if (!parent_container || !GTK_IS_CONTAINER (parent_container))
    {
      GtkWidget *any;
      
      parent_container = bst_gmask_container_create (tooltips, param_group_quark ? 5 : 0);
      if (param_group_quark)
	any = gtk_widget_new (GTK_TYPE_FRAME,
			      "visible", TRUE,
			      "label", g_quark_to_string (param_group_quark),
			      "child", parent_container,
			      NULL);
      else
	any = parent_container;
      if (GTK_IS_BOX (parent))
	gtk_box_pack_start (GTK_BOX (parent), any, FALSE, TRUE, 0);
      else if (GTK_IS_WRAP_BOX (parent))
	gtk_container_add_with_properties (GTK_CONTAINER (parent), any,
					   "hexpand", TRUE,
					   "hfill", TRUE,
					   "vexpand", FALSE,
					   "vfill", TRUE,
					   NULL);
      else
	gtk_container_add (GTK_CONTAINER (parent), any);
      bst_container_set_named_child (parent, param_group_quark ? param_group_quark : null_group, parent_container);
    }
  parent = NULL;
  
  /* feature param hints and integral values
   */
  read_only = (pspec->flags & BSE_PARAM_HINT_RDONLY) != 0 || !(pspec->flags & BSE_PARAM_WRITABLE);
  radio = (pspec->flags & BSE_PARAM_HINT_RADIO) != 0;
  string_toggle = (pspec->flags & BSE_PARAM_HINT_CHECK_NULL) != 0;
  switch (G_TYPE_FUNDAMENTAL (G_PARAM_SPEC_VALUE_TYPE (pspec)))
    {
    case G_TYPE_INT:
      if (BSE_IS_PARAM_SPEC_INT (pspec) && BSE_PARAM_SPEC_INT (pspec)->stepping_rate != 0)
	adjustment = (GtkAdjustment*) gtk_adjustment_new (G_PARAM_SPEC_INT (pspec)->default_value,
							  G_PARAM_SPEC_INT (pspec)->minimum,
							  G_PARAM_SPEC_INT (pspec)->maximum,
							  1,
							  BSE_PARAM_SPEC_INT (pspec)->stepping_rate,
							  0);
      digits = 0;
      break;
    case G_TYPE_UINT:
      if (BSE_IS_PARAM_SPEC_UINT (pspec) && BSE_PARAM_SPEC_UINT (pspec)->stepping_rate != 0)
	adjustment = (GtkAdjustment*) gtk_adjustment_new (G_PARAM_SPEC_UINT (pspec)->default_value,
							  G_PARAM_SPEC_UINT (pspec)->minimum,
							  G_PARAM_SPEC_UINT (pspec)->maximum,
							  1,
							  BSE_PARAM_SPEC_UINT (pspec)->stepping_rate,
							  0);
      digits = 0;
      break;
    case G_TYPE_FLOAT:
      if (BSE_IS_PARAM_SPEC_FLOAT (pspec) && BSE_EPSILON_CMP (BSE_PARAM_SPEC_FLOAT (pspec)->stepping_rate, 0) != 0)
	adjustment = (GtkAdjustment*) gtk_adjustment_new (G_PARAM_SPEC_FLOAT (pspec)->default_value,
							  G_PARAM_SPEC_FLOAT (pspec)->minimum,
							  G_PARAM_SPEC_FLOAT (pspec)->maximum,
							  MIN (0.1, BSE_PARAM_SPEC_FLOAT (pspec)->stepping_rate),
							  MAX (0.1, BSE_PARAM_SPEC_FLOAT (pspec)->stepping_rate),
							  0);
      digits = 5;
      break;
    case G_TYPE_DOUBLE:
      if (BSE_IS_PARAM_SPEC_DOUBLE (pspec) && BSE_EPSILON_CMP (BSE_PARAM_SPEC_DOUBLE (pspec)->stepping_rate, 0) != 0)
	adjustment = (GtkAdjustment*) gtk_adjustment_new (G_PARAM_SPEC_DOUBLE (pspec)->default_value,
							  G_PARAM_SPEC_DOUBLE (pspec)->minimum,
							  G_PARAM_SPEC_DOUBLE (pspec)->maximum,
							  MIN (0.1, BSE_PARAM_SPEC_DOUBLE (pspec)->stepping_rate),
							  MAX (0.1, BSE_PARAM_SPEC_DOUBLE (pspec)->stepping_rate),
							  0);
      digits = 6;
      break;
    }
  if (adjustment)
    {
      if (0)
	g_print ("adjustm.increm: \"%s\": %f / %f\n", g_param_spec_get_nick (pspec),
		 adjustment->step_increment,
		 adjustment->page_increment);
      
      gtk_object_ref (GTK_OBJECT (adjustment));
      gtk_object_sink (GTK_OBJECT (adjustment));
      
      /* we need to connect *after* the spinner so the spinner's value is
       * already updated
       */
      g_object_connect (adjustment,
			"swapped_signal_after::value-changed", bst_param_gtk_changed, bparam,
			NULL);
      spinner = gtk_spin_button_new (adjustment, 0, digits);
      if (pspec->flags & BSE_PARAM_HINT_DIAL)
	{
	  dial = gtk_widget_new (BST_TYPE_DIAL,
				 "visible", TRUE,
				 "can_focus", FALSE,
				 NULL);
	  bst_dial_set_adjustment (BST_DIAL (dial), adjustment);
	}
      if (pspec->flags & (BSE_PARAM_HINT_SCALE | BSE_PARAM_HINT_DIAL))
	{
	  GtkAdjustment *scale_adjustment = adjustment;
	  BseParamLogScale lscale;

	  bse_param_spec_get_log_scale (pspec, &lscale);
	  if (lscale.n_steps)
	    {
	      scale_adjustment = bst_log_adjustment_from_adj (adjustment);
	      bst_log_adjustment_setup (BST_LOG_ADJUSTMENT (scale_adjustment),
					lscale.center,
					lscale.base,
					lscale.n_steps);
	    }
	  scale = g_object_connect (gtk_widget_new (GTK_TYPE_HSCALE,
						    "visible", TRUE,
						    "adjustment", scale_adjustment,
						    "draw_value", FALSE,
						    "can_focus", FALSE,
						    NULL),
				    "signal_after::size_request", hscale_size_request, NULL,
				    NULL);
	}
      g_object_unref (adjustment);
    }
  
  name = g_param_spec_get_nick (pspec);
  tooltip = g_param_spec_get_blurb (pspec);
  
  expandable = FALSE;
  switch (G_TYPE_FUNDAMENTAL (G_PARAM_SPEC_VALUE_TYPE (pspec)))
    {
      GtkWidget *action, *prompt, *pre_action, *post_action, *frame, *any;
      DotAreaData *dot_data;
      GEnumValue *ev;
      guint width;
      
    case G_TYPE_BOOLEAN:
      action = g_object_connect (gtk_widget_new (radio ? BST_TYPE_FREE_RADIO_BUTTON : GTK_TYPE_CHECK_BUTTON,
						 "visible", TRUE,
						 NULL),
				 "swapped_signal::clicked", bst_param_gtk_changed, bparam,
				 NULL);
      prompt = g_object_new (GTK_TYPE_LABEL,
			     "visible", TRUE,
			     "label", name,
			     "parent", g_object_connect (g_object_new (BST_TYPE_XFRAME,
								       "visible", TRUE,
								       "parent", action,
								       "cover", action,
								       "steal_button", TRUE,
								       NULL),
							 "swapped_signal::button_check", xframe_check_button, bparam,
							 NULL),
			     NULL);
      gtk_misc_set_alignment (GTK_MISC (prompt), 0, 0.5);
      group = bst_gmask_form_big (parent_container, action);
      bst_gmask_set_tip (group, tooltip);
      bst_gmask_pack (group);
      break;
    case G_TYPE_INT:
    case G_TYPE_UINT:
    case G_TYPE_FLOAT:
    case G_TYPE_DOUBLE:
    case BSE_TYPE_TIME:
    case BSE_TYPE_NOTE:
      switch (G_TYPE_FUNDAMENTAL (G_PARAM_SPEC_VALUE_TYPE (pspec)))
	{
	case G_TYPE_INT:
	case G_TYPE_UINT:
	  width = 70;
	  break;
	case G_TYPE_FLOAT:
	case G_TYPE_DOUBLE:
	  expandable = TRUE;
	  width = 80;
	  break;
	case BSE_TYPE_TIME:
	  expandable = TRUE;
	  width = 140;
	  break;
	case BSE_TYPE_NOTE:
	  width = 50;
	  break;
	default:
	  width = 3;
	  break;
	}
      if (spinner)
	width += 10;
      action = spinner ? spinner : gtk_entry_new ();
      prompt = gtk_widget_new (GTK_TYPE_LABEL,
			       "visible", TRUE,
			       "label", name,
			       "justify", GTK_JUSTIFY_LEFT,
			       "xalign", 0.0,
			       "parent", g_object_connect (g_object_new (BST_TYPE_XFRAME,
									 "visible", TRUE,
									 "cover", action,
									 NULL),
							   "swapped_signal::button_check", xframe_check_button, bparam,
							   NULL),
			       NULL);
      g_object_set (action,
		    "visible", TRUE,
		    "width_request", width,
		    "activates_default", FALSE,
		    NULL);
      g_object_connect (action,
			"signal::key_press_event", bst_entry_key_press, bparam,
			"signal::activate", bst_param_entry_activate, bparam,
			spinner ? NULL : "swapped_signal::focus_out_event", bst_param_focus_out, bparam,
			NULL);
      if (!spinner)
	g_object_connect (action,
			  "swapped_signal::focus_out_event", bst_param_focus_out, bparam,
			  NULL);
      group = bst_gmask_form (parent_container, action, expandable);
      bst_gmask_set_prompt (group, prompt);
      if (scale)
	bst_gmask_set_aux2 (group, scale);
      if (dial)
	bst_gmask_set_aux1 (group, dial);
      bst_gmask_set_tip (group, tooltip);
      bst_gmask_pack (group);
      break;
    case G_TYPE_ENUM:
      ev = G_PARAM_SPEC_ENUM (pspec)->enum_class->values;
      action = gtk_option_menu_new ();
      prompt = gtk_widget_new (GTK_TYPE_LABEL,
			       "visible", TRUE,
			       "label", name,
			       "justify", GTK_JUSTIFY_LEFT,
			       "xalign", 0.0,
			       "sensitive", !read_only && ev,
			       "parent", g_object_connect (g_object_new (BST_TYPE_XFRAME,
									 "visible", TRUE,
									 "cover", action,
									 NULL),
							   "swapped_signal::button_check", xframe_check_button, bparam,
							   NULL),
			       NULL);
      gtk_widget_set (action,
		      "visible", TRUE,
		      NULL);
      g_object_connect (action,
			"signal::button_press_event", focus_on_event, NULL,
			NULL);
      if (ev)
	{
	  GtkWidget *menu;
	  
	  menu = gtk_widget_new (GTK_TYPE_MENU,
				 NULL);
	  gtk_menu_set_accel_path (GTK_MENU (menu), "<BEAST-Param>/EnumPopup");
	  while (ev->value_nick)
	    {
	      GtkWidget *item;
	      
	      item = gtk_menu_item_new_with_label (ev->value_nick);
	      gtk_widget_show (item);
	      gtk_object_set_data_by_id (GTK_OBJECT (item), quark_evalues, ev);
	      gtk_container_add (GTK_CONTAINER (menu), item);
	      
	      ev++;
	    }
	  
	  gtk_option_menu_set_menu (GTK_OPTION_MENU (action), menu);
	  g_object_connect (action,
			    "swapped_signal::changed", bst_param_gtk_changed, bparam,
			    NULL);
	}
      group = bst_gmask_form (parent_container, action, FALSE);
      bst_gmask_set_prompt (group, prompt);
      bst_gmask_set_tip (group, tooltip);
      bst_gmask_pack (group);
      break;
    case G_TYPE_FLAGS:
      action = gtk_widget_new (GTK_TYPE_LABEL,
			       "visible", TRUE,
			       "label", g_param_spec_get_blurb (pspec),
			       "justify", GTK_JUSTIFY_LEFT,
			       "xalign", 0.0,
			       NULL);
      prompt = gtk_widget_new (GTK_TYPE_LABEL,
			       "visible", TRUE,
			       "sensitive", FALSE,
			       "label", pspec->name,
			       "justify", GTK_JUSTIFY_LEFT,
			       "xalign", 0.0,
			       "parent", g_object_connect (g_object_new (BST_TYPE_XFRAME,
									 "visible", TRUE,
									 "cover", action,
									 NULL),
							   "swapped_signal::button_check", xframe_check_button, bparam,
							   NULL),
			       NULL);
      group = bst_gmask_form (parent_container, action, FALSE);
      bst_gmask_set_prompt (group, prompt);
      bst_gmask_set_tip (group, tooltip);
      bst_gmask_pack (group);
      break;
    case G_TYPE_STRING:
      action = g_object_connect (gtk_widget_new (GTK_TYPE_ENTRY,
						 "visible", TRUE,
						 "activates_default", FALSE,
						 NULL),
				 "signal::key_press_event", bst_entry_key_press, bparam,
				 "signal::activate", bst_param_entry_activate, bparam,
				 "swapped_signal::focus_out_event", bst_param_focus_out, bparam,
				 NULL);
      prompt = gtk_widget_new (GTK_TYPE_LABEL,
			       "visible", TRUE,
			       "label", name,
			       "justify", GTK_JUSTIFY_LEFT,
			       "xalign", 0.0,
			       "parent", g_object_connect (g_object_new (BST_TYPE_XFRAME,
									 "visible", TRUE,
									 "cover", action,
									 NULL),
							   "swapped_signal::button_check", xframe_check_button, bparam,
							   NULL),
			       NULL);
      group = bst_gmask_form (parent_container, action, TRUE);
      bst_gmask_set_prompt (group, prompt);
      if (string_toggle)
	{
	  pre_action = g_object_connect (gtk_widget_new (GTK_TYPE_TOGGLE_BUTTON,
							 "visible", TRUE,
							 "can_focus", FALSE,
							 "width_request", 10,
							 "height_request", 10,
							 "parent", gtk_widget_new (GTK_TYPE_ALIGNMENT, /* don't want vexpand */
										   "visible", TRUE,
										   "xscale", 0.0,
										   "yscale", 0.0,
										   "xalign", 0.0,
										   "width_request", 10 + 3,
										   NULL),
							 NULL),
					 "swapped_signal::clicked", bst_param_gtk_changed, bparam,
					 "signal::clicked", bst_string_toggle, action,
					 NULL);
	  bst_gmask_set_ahead (group, pre_action);
	  bst_string_toggle (GTK_TOGGLE_BUTTON (pre_action), action);
	}
      bst_gmask_set_tip (group, tooltip);
      bst_gmask_pack (group);
      break;
    case BSE_TYPE_DOTS:
      prompt = gtk_widget_new (GTK_TYPE_LABEL,
			       "visible", TRUE,
			       "label", name,
			       "justify", GTK_JUSTIFY_LEFT,
			       "xalign", 0.0,
			       "parent", g_object_connect (g_object_new (BST_TYPE_XFRAME,
									 "visible", TRUE,
									 NULL),
							   "swapped_signal::button_check", xframe_check_button, bparam,
							   NULL),
			       NULL);
      frame = gtk_widget_new (GTK_TYPE_FRAME,
			      "visible", TRUE,
			      "label", NULL,
			      "shadow", GTK_SHADOW_IN,
			      "border_width", 0,
			      NULL);
      dot_data = g_new0 (DotAreaData, 1);
      action = g_object_connect (gtk_widget_new (GTK_TYPE_DRAWING_AREA,
						 "visible", TRUE,
						 "height_request", 50,
						 "parent", frame,
						 "events", (GDK_EXPOSURE_MASK |
							    GDK_ENTER_NOTIFY_MASK |
							    GDK_LEAVE_NOTIFY_MASK |
							    GDK_BUTTON_PRESS_MASK |
							    GDK_BUTTON_RELEASE_MASK |
							    GDK_BUTTON1_MOTION_MASK),
						 NULL),
				 "swapped_signal::destroy", g_free, dot_data,
				 "signal::configure_event", dots_area_configure_event, bparam,
				 "signal::expose_event", dots_area_expose_event, bparam,
				 "signal::enter_notify_event", dots_area_cross_event, bparam,
				 "signal::leave_notify_event", dots_area_cross_event, bparam,
				 "signal::button_press_event", dots_area_button_event, bparam,
				 "signal::button_release_event", dots_area_button_event, bparam,
				 "signal::motion_notify_event", dots_area_motion_event, bparam,
				 NULL);
      dot_data->cdot = -1;
      GTK_DRAWING_AREA (action)->draw_data = dot_data;
      group = bst_gmask_form_big (parent_container, action);
      bst_gmask_set_prompt (group, prompt);
      bst_gmask_set_tip (group, tooltip);
      bst_gmask_pack (group);
      break;
    case G_TYPE_OBJECT:
      action = g_object_connect (gtk_widget_new (GTK_TYPE_ENTRY,
						 "visible", TRUE,
						 "width_request", 160,
						 "activates_default", FALSE,
						 NULL),
				 "signal::key_press_event", bst_entry_key_press, bparam,
				 "signal::activate", bst_param_entry_activate, bparam,
				 "swapped_signal::focus_out_event", bst_param_focus_out, bparam,
				 "swapped_signal::grab_focus", bst_param_update_clue_hunter, bparam,
				 NULL);
      prompt = gtk_widget_new (GTK_TYPE_LABEL,
			       "visible", TRUE,
			       "label", name,
			       "justify", GTK_JUSTIFY_LEFT,
			       "xalign", 0.0,
			       "parent", g_object_connect (g_object_new (BST_TYPE_XFRAME,
									 "visible", TRUE,
									 "cover", action,
									 NULL),
							   "swapped_signal::button_check", xframe_check_button, bparam,
							   NULL),
			       NULL);
      group = bst_gmask_form (parent_container, action, TRUE);
      bst_gmask_set_prompt (group, prompt);
      any = gtk_widget_new (GTK_TYPE_CLUE_HUNTER,
			    "keep_history", FALSE,
			    "entry", action,
			    "user_data", bparam,
			    NULL);
      gtk_object_set_user_data (GTK_OBJECT (action), any);
      post_action = gtk_clue_hunter_create_arrow (GTK_CLUE_HUNTER (any));
      bst_gmask_set_atail (group, post_action);
      bst_gmask_set_tip (group, tooltip);
      bst_gmask_pack (group);
      break;
    case G_TYPE_BOXED:
      if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (pspec), BSE_TYPE_SEQUENCE))
	{
	  prompt = gtk_widget_new (GTK_TYPE_LABEL,
				   "visible", TRUE,
				   "label", name,
				   "justify", GTK_JUSTIFY_LEFT,
				   "xalign", 0.0,
				   "parent", g_object_connect (g_object_new (BST_TYPE_XFRAME,
									     "visible", TRUE,
									     NULL),
							       "swapped_signal::button_check", xframe_check_button, bparam,
							       NULL),
				   NULL);
	  action = g_object_new (BST_TYPE_SEQUENCE,
				 "visible", TRUE,
				 NULL);
	  g_object_connect (action,
			    "swapped_signal::seq-changed", bst_param_gtk_changed, bparam,
			    NULL);
	  group = bst_gmask_form_big (parent_container, action);
	  bst_gmask_set_prompt (group, prompt);
	  bst_gmask_set_tip (group, tooltip);
	  bst_gmask_pack (group);
	  break;
	}
      /* fall through */
    default:
      g_warning ("unknown param type: `%s'", pspec->name);
      group = NULL;
      break;
    }

  if (BST_IS_DIAL (dial))
    bst_dial_set_align_widget (BST_DIAL (dial), bst_gmask_get_action (group), 0, 1);
  
  bparam->group = group;
  if (bparam->group)
    g_object_connect (bparam->group,
		      "signal::destroy", gtk_widget_destroyed, &bparam->group,
		      "swapped_signal::destroy", bst_param_free, bparam,
		      "swapped_signal::destroy", g_type_class_unref, g_type_class_peek (owner_type),
		      NULL);
  else
    g_return_val_if_fail (bparam->group != NULL, bparam);
  
  bst_gmask_ensure_styles (bparam->group);
  
  return bparam;
}

static void
bst_param_update (BstParam *bparam)
{
  gpointer group = bparam->group;
  GValue *value = &bparam->value;
  GParamSpec *pspec = bparam->pspec;
  gboolean read_only = (pspec->flags & BSE_PARAM_HINT_RDONLY) != 0 || !(pspec->flags & BSE_PARAM_WRITABLE);
  
  bst_gmask_set_sensitive (group, !read_only && bparam->editable);
  
  switch (G_TYPE_FUNDAMENTAL (G_PARAM_SPEC_VALUE_TYPE (pspec)))
    {
      GtkWidget *action, *prompt, *pre_action, *any;
      gchar *string;
      
    case G_TYPE_BOOLEAN:
      action = bst_gmask_get_action (group);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (action), g_value_get_boolean (value));
      break;
    case G_TYPE_INT:
    case G_TYPE_UINT:
    case G_TYPE_FLOAT:
    case G_TYPE_DOUBLE:
    case BSE_TYPE_TIME:
    case BSE_TYPE_NOTE:
      action = bst_gmask_get_action (group);
      string = NULL; /* eek, cure stupid compiler */
      switch (G_TYPE_FUNDAMENTAL (G_PARAM_SPEC_VALUE_TYPE (pspec)))
	{
	case G_TYPE_INT:	string = g_strdup_printf ("%d", g_value_get_int (value));	break;
	case G_TYPE_UINT:	string = g_strdup_printf ("%u", g_value_get_uint (value));	break;
	case G_TYPE_FLOAT:	string = g_strdup_printf ("%f", g_value_get_float (value));	break;
	case G_TYPE_DOUBLE:	string = g_strdup_printf ("%f", g_value_get_double (value));	break;
	case BSE_TYPE_TIME:	string = bse_time_to_str (bse_value_get_time (value));		break;
	case BSE_TYPE_NOTE:	string = bse_note_to_string (bse_value_get_note (value));	break;
	}
      if (!g_str_equal (gtk_entry_get_text (GTK_ENTRY (action)), string))
	{
	  gtk_entry_set_text (GTK_ENTRY (action), string);
	  if (GTK_IS_SPIN_BUTTON (action))
	    gtk_spin_button_update (GTK_SPIN_BUTTON (action));
	}
      g_free (string);
      break;
    case G_TYPE_STRING:
      action = bst_gmask_get_action (group);
      string = g_value_get_string (value);
      if (!string || !g_str_equal (gtk_entry_get_text (GTK_ENTRY (action)), string))
	gtk_entry_set_text (GTK_ENTRY (action), string ? string : "");
      pre_action = bst_gmask_get_ahead (group);
      if (pre_action)
	{
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pre_action), string != NULL);
	  if (!string && GTK_WIDGET_HAS_FOCUS (action))
	    {
	      GtkWidget *window = gtk_widget_get_toplevel (action);

	      if (GTK_IS_WINDOW (window))
		gtk_window_set_focus (GTK_WINDOW (window), NULL);
	    }
	  g_object_set (action, "can_focus", string != NULL, NULL);
	  g_object_set (pre_action, "can_focus", string == NULL, NULL);
	}
      break;
    case BSE_TYPE_DOTS:
      action = bst_gmask_get_action (group);
      gtk_widget_queue_draw (action);
      break;
    case G_TYPE_OBJECT:
      action = bst_gmask_get_action (group);
      string = (BSE_IS_ITEM (g_value_get_object (value))
		? bsw_item_get_uloc_path (BSE_OBJECT_ID (g_value_get_object (value)))
		: NULL);
      if (!bse_string_equals (gtk_entry_get_text (GTK_ENTRY (action)), string))
	gtk_entry_set_text (GTK_ENTRY (action), string ? string : "");
      any = gtk_object_get_user_data (GTK_OBJECT (action));
      break;
    case G_TYPE_ENUM:
      action = bst_gmask_get_action (group);
      any = gtk_option_menu_get_menu (GTK_OPTION_MENU (action));
      prompt = bst_gmask_get_prompt (group);
      gtk_widget_set_sensitive (prompt, GTK_WIDGET_IS_SENSITIVE (prompt) && G_PARAM_SPEC_ENUM (pspec)->enum_class->values);
      if (any)
	{
	  GList *list;
	  guint n = 0;
	  
	  for (list = GTK_MENU_SHELL (any)->children; list; list = list->next)
	    {
	      GtkWidget *item = list->data;
	      GEnumValue *ev = gtk_object_get_data_by_id (GTK_OBJECT (item), quark_evalues);
	      
	      if (ev->value == g_value_get_enum (value))
		{
		  gtk_option_menu_set_history (GTK_OPTION_MENU (action), n);
		  break;
		}
	      n++;
	    }
	}
      break;
    case G_TYPE_BOXED:
      if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (pspec), BSE_TYPE_SEQUENCE))
	{
	  action = bst_gmask_get_action (group);
	  bst_sequence_set_seq (BST_SEQUENCE (action),
				g_value_get_boxed (value));
	  break;
	}
      /* fall through */
    case G_TYPE_FLAGS:
    default:
      g_warning ("unknown param type: `%s'", pspec->name);
      break;
    }
}

static gboolean
bst_param_apply (BstParam *bparam,
		 gboolean *changed)
{
  gpointer group = bparam->group;
  GValue *value = &bparam->value;
  GParamSpec *pspec = bparam->pspec;
  GValue tmp_value = { 0, };
  gchar *dummy = NULL;
  guint dirty = 0;
  
  g_value_init (&tmp_value, G_VALUE_TYPE (value));
  g_value_copy (value, &tmp_value);
  
  *changed = FALSE;
  
  switch (G_TYPE_FUNDAMENTAL (G_PARAM_SPEC_VALUE_TYPE (pspec)))
    {
      GtkWidget *action, *pre_action, *any;
      gchar *string;
      BseTime time_data;
      guint base;
      gint note_data;
      
    case G_TYPE_BOOLEAN:
      action = bst_gmask_get_action (group);
      g_value_set_boolean (value, GTK_TOGGLE_BUTTON (action)->active);
      break;
    case G_TYPE_INT:
      action = bst_gmask_get_action (group);
      string = gtk_entry_get_text (GTK_ENTRY (action));
      if (string && string[0] == '0')
	{
	  base = 8;
	  string++;
	  if (string[0] == 'x' || string[0] == 'X')
	    {
	      base = 16;
	      string++;
	    }
	}
      else
	base = 10;
      g_value_set_int (value, strtol (string, &dummy, base));
      dirty += dummy != NULL && (*dummy != 0 || dummy == string);
      break;
    case G_TYPE_UINT:
      action = bst_gmask_get_action (group);
      string = gtk_entry_get_text (GTK_ENTRY (action));
      if (string && string[0] == '0')
	{
	  base = 8;
	  string++;
	  if (string[0] == 'x' || string[0] == 'X')
	    {
	      base = 16;
	      string++;
	    }
	}
      else
	base = 10;
      g_value_set_uint (value, strtol (string, &dummy, base));
      dirty += dummy != NULL && (*dummy != 0 || dummy == string);
      break;
    case G_TYPE_FLOAT:
      action = bst_gmask_get_action (group);
      g_value_set_float (value, g_strtod (gtk_entry_get_text (GTK_ENTRY (action)), &dummy));
      break;
    case G_TYPE_DOUBLE:
      action = bst_gmask_get_action (group);
      g_value_set_double (value, g_strtod (gtk_entry_get_text (GTK_ENTRY (action)), &dummy));
      break;
    case BSE_TYPE_TIME:
      action = bst_gmask_get_action (group);
      time_data = bse_time_from_string (gtk_entry_get_text (GTK_ENTRY (action)), NULL);
      if (time_data)
	bse_value_set_time (value, time_data);
      else
	dirty++;
      break;
    case BSE_TYPE_NOTE:
      action = bst_gmask_get_action (group);
      note_data = bse_note_from_string (gtk_entry_get_text (GTK_ENTRY (action)));
      if (note_data != BSE_NOTE_UNPARSABLE)
	bse_value_set_note (value, note_data);
      else
	dirty++;
      break;
    case G_TYPE_STRING:
      action = bst_gmask_get_action (group);
      pre_action = bst_gmask_get_ahead (group);
      if (!pre_action)
	string = gtk_entry_get_text (GTK_ENTRY (action));
      else if (GTK_TOGGLE_BUTTON (pre_action)->active)
	{
	  string = gtk_entry_get_text (GTK_ENTRY (action));
	  if (!string)
	    string = "";
	}
      else
	string = NULL;
      g_value_set_string (value, string);
      break;
    case BSE_TYPE_DOTS:
      *changed = TRUE;
      break;
    case G_TYPE_OBJECT:
      action = bst_gmask_get_action (group);
      string = bse_strdup_stripped (gtk_entry_get_text (GTK_ENTRY (action)));
      if (string && bparam->is_object && BSE_IS_ITEM (bparam->owner))
	{
	  BswProxy item = 0, project = bsw_item_get_project (BSE_OBJECT_ID (bparam->owner));
	  
	  /* check whether this is an uloc path */
	  if (!item && strchr (string, '.'))
	    {
	      item = bsw_project_find_item (project, string);
	      
	      if (item && !g_type_is_a (bsw_proxy_type (item), G_PARAM_SPEC_VALUE_TYPE (pspec)))
		item = 0;
	    }
	  else if (!item) /* try generic lookup for uloc (ambiguous lookup) */
	    {
	      BswVIter *iter = bsw_project_list_items_by_uloc (project,
							       G_PARAM_SPEC_VALUE_TYPE (pspec),
							       string);
	      for (bsw_viter_rewind (iter); bsw_viter_n_left (iter); bsw_viter_next (iter))
		if (bsw_item_get_project (bsw_viter_get_proxy (iter)) == project)
		  {
		    item = bsw_viter_get_proxy (iter);
		    break;
		  }
	      bsw_viter_free (iter);
	    }

	  /* ok, found one or giving up */
	  g_value_set_object (value, bse_object_from_id (item));
	  g_free (string);
	  
	  /* enforce redisplay of the entry's string with the correct name */
	  dirty += 1;
	}
      else
	g_value_set_object (value, NULL);
      break;
    case G_TYPE_ENUM:
      action = bst_gmask_get_action (group);
      any = GTK_OPTION_MENU (action)->menu_item;
      if (any)
	{
	  GEnumValue *ev = gtk_object_get_data_by_id (GTK_OBJECT (any), quark_evalues);
	  
	  g_value_set_enum (value, ev->value);
	}
      break;
    case G_TYPE_BOXED:
      if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (pspec), BSE_TYPE_SEQUENCE))
	{
	  action = bst_gmask_get_action (group);
	  g_value_set_boxed (value, BST_SEQUENCE (action)->sd);
	  break;
	}
      /* fall through */
    case G_TYPE_FLAGS:
    default:
      g_warning ("unknown param type: `%s'", pspec->name);
      break;
    }
  
  dirty += g_param_value_validate (pspec, value);
  
  *changed |= g_param_values_cmp (pspec, value, &tmp_value) != 0;
  g_value_unset (&tmp_value);
  
  dirty += *changed;
  
  return dirty > 0;
}

void
bst_param_get (BstParam *bparam)
{
  g_return_if_fail (G_IS_VALUE (bparam));
  g_return_if_fail (G_IS_PARAM_SPEC (bparam->pspec));
  
  bparam->locked++;
  if (bparam->is_object && bparam->owner)
    g_object_get_property (G_OBJECT (bparam->owner), bparam->pspec->name, &bparam->value);
  else
    {
      /* bse_param_reset_value (&bparam->param); */
    }
  bst_param_update (bparam);
  bparam->locked = 0;
}

void
bst_param_set (BstParam *bparam)
{
  gboolean dirty, changed;
  
  g_return_if_fail (G_IS_VALUE (bparam));
  g_return_if_fail (G_IS_PARAM_SPEC (bparam->pspec));
  
  bparam->locked++;
  dirty = bst_param_apply (bparam, &changed);
  if (changed && bparam->is_object && bparam->owner)
    g_object_set_property (G_OBJECT (bparam->owner), bparam->pspec->name, &bparam->value);
  
  if (dirty)
    bst_param_get (bparam);
  else
    bparam->locked = 0;
}

void
bst_param_reset (BstParam *bparam)
{
  g_return_if_fail (G_IS_VALUE (bparam));
  g_return_if_fail (G_IS_PARAM_SPEC (bparam->pspec));
  
  bparam->locked++;
  g_value_reset (&bparam->value);
  bst_param_update (bparam);
  bparam->locked = 0;
}

void
bst_param_set_default (BstParam *bparam)
{
  g_return_if_fail (G_IS_VALUE (bparam));
  g_return_if_fail (G_IS_PARAM_SPEC (bparam->pspec));
  
  bparam->locked++;
  g_param_value_set_default (bparam->pspec, &bparam->value);
  bst_param_update (bparam);
  bparam->locked = 0;
}

gboolean
bst_param_set_value (BstParam	  *bparam,
		     const GValue *value)
{
  gboolean success;
  
  g_return_val_if_fail (G_IS_VALUE (bparam), FALSE);
  g_return_val_if_fail (G_IS_PARAM_SPEC (bparam->pspec), FALSE);
  g_return_val_if_fail (G_IS_VALUE (value), FALSE);
  
  success = g_param_value_convert (bparam->pspec, value, &bparam->value, FALSE);
  
  if (success)
    {
      if (bparam->is_object && bparam->owner)
	g_object_set_property (G_OBJECT (bparam->owner), bparam->pspec->name, &bparam->value);
      bst_param_get (bparam);
    }
  else
    bparam->locked = 0;
  
  return success;
}

void
bst_param_set_editable (BstParam *bparam,
			gboolean  editable)
{
  g_return_if_fail (G_IS_VALUE (bparam));
  g_return_if_fail (G_IS_PARAM_SPEC (bparam->pspec));
  
  editable = editable != FALSE;
  if (bparam->editable != editable)
    {
      bparam->editable = editable;
      bst_param_update (bparam);
    }
}

static gint
dots_area_configure_event (GtkWidget	     *widget,
			   GdkEventConfigure *event,
			   BstParam	     *bparam)
{
  gdk_window_set_background (widget->window, &widget->style->base[GTK_WIDGET_STATE (widget)]);
  
  return TRUE;
}

static gint
dots_area_cross_event (GtkWidget	*widget,
		       GdkEventCrossing *event,
		       BstParam		*bparam)
{
  DotAreaData *data = GTK_DRAWING_AREA (widget)->draw_data;
  
  if (event->type == GDK_ENTER_NOTIFY)
    data->entered = TRUE;
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      if (data->entered)
	data->entered = FALSE;
      else
	data->cdot = -1;
    }
  
  gtk_widget_queue_draw (widget);
  
  return TRUE;
}

static gint
dots_area_expose_event (GtkWidget      *widget,
			GdkEventExpose *event,
			BstParam       *bparam)
{
  guint n_dots;
  BseDot *dots = bse_value_get_dots (&bparam->value, &n_dots);
  GdkDrawable *drawable = widget->window;
  GdkGC *fg_gc = widget->style->black_gc;
  GdkGC *bg_gc = widget->style->base_gc[GTK_WIDGET_STATE (widget)];
  GdkGC *hl_gc = widget->style->bg_gc[GTK_STATE_SELECTED];
  DotAreaData *data = GTK_DRAWING_AREA (widget)->draw_data;
  gint width, height, maxx, maxy;
  guint i;
  
  gdk_window_get_size (widget->window, &width, &height);
  maxx = width - 1;
  maxy = height - 1;
  
  /* clear background
   */
  gdk_draw_rectangle (drawable, bg_gc,
		      TRUE,
		      0,
		      0,
		      width,
		      height);
  
  /* draw lines
   */
  for (i = 0; i < n_dots - 1; i++)
    gdk_draw_line (drawable, fg_gc,
		   maxx * dots[i].x,
		   maxy * (1.0 - dots[i].y),
		   maxx * dots[i + 1].x,
		   maxy * (1.0 - dots[i + 1].y));
  
  
  /* draw circles
   */
  if (data->entered || data->cdot >= 0)
    for (i = 0; i < n_dots; i++)
      gdk_draw_arc (drawable, hl_gc, FALSE,
		    maxx * dots[i].x - BST_TAG_DIAMETER / 2,
		    maxy * (1.0 - dots[i].y) - BST_TAG_DIAMETER / 2,
		    BST_TAG_DIAMETER, BST_TAG_DIAMETER,
		    0 * 64, 360 *64);
  
  return TRUE;
}

static gint
dots_area_button_event (GtkWidget      *widget,
			GdkEventButton *event,
			BstParam       *bparam)
{
  guint n_dots;
  BseDot *dots = bse_value_get_dots (&bparam->value, &n_dots);
  gint maxx, maxy;
  DotAreaData *data = GTK_DRAWING_AREA (widget)->draw_data;
  
  if (bparam->locked)
    return TRUE;
  
  data->cdot = -1;
  
  gdk_window_get_size (widget->window, &maxx, &maxy);
  maxx -= 1; maxy -= 1;
  
  if (event->button == 1 &&
      event->type == GDK_BUTTON_PRESS)
    {
      guint i;
      gfloat min = BST_TAG_DIAMETER / 2 + 1;
      
      for (i = 0; i < n_dots; i++)
	{
	  gfloat dx = event->x - maxx * dots[i].x;
	  gfloat dy = event->y - maxy * (1.0 - dots[i].y);
	  gfloat dist;
	  
	  dist = sqrt (dx * dx + dy * dy);
	  if (dist < min || (dist == min && i < n_dots / 2))
	    {
	      min = dist;
	      data->cdot = i;
	    }
	}
      
      if (data->cdot >= 0)
	{
	  gfloat y = event->y ? event->y / maxy : 0;
	  gfloat x = event->x ? event->x / maxx : 0;
	  
	  bse_value_set_dot (&bparam->value,
			   data->cdot,
			   x,
			   1.0 - y);
	  
	  bst_param_set (bparam);
	}
    }
  
  return TRUE;
}

static gint
dots_area_motion_event (GtkWidget      *widget,
			GdkEventMotion *event,
			BstParam       *bparam)
{
  gint maxx, maxy;
  DotAreaData *data = GTK_DRAWING_AREA (widget)->draw_data;
  
  gdk_window_get_size (widget->window, &maxx, &maxy);
  maxx -= 1; maxy -= 1;
  
  if (bparam->locked)
    return TRUE;
  
  if (data->cdot >= 0 &&
      event->type == GDK_MOTION_NOTIFY &&
      !event->is_hint)
    {
      gfloat y = event->y ? event->y / maxy : 0;
      gfloat x = event->x ? event->x / maxx : 0;
      
      bse_value_set_dot (&bparam->value,
		       data->cdot,
		       x,
		       1.0 - y);
      
      bst_param_set (bparam);
    }
  
  return TRUE;
}
