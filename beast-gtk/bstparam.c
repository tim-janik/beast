/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include	"bstparam.h"

#include	"bstsamplerepo.h"
#include	<stdlib.h>
#include	<math.h>


#undef	DEBUG_ADJUSTMENT

/* --- structures --- */
typedef struct _DotAreaData DotAreaData;
struct _DotAreaData
{
  gint cdot;
  gboolean entered;
};

/* --- prototypes --- */
static gint dots_area_configure_event	(GtkWidget          *widget,
					 GdkEventConfigure  *event,
					 BstParam           *bparam);
static gint dots_area_expose_event	(GtkWidget          *widget,
					 GdkEventExpose     *event,
					 BstParam	    *bparam);
static gint dots_area_cross_event	(GtkWidget          *widget,
					 GdkEventCrossing   *event,
					 BstParam	    *bparam);
static gint dots_area_button_event	(GtkWidget          *widget,
					 GdkEventButton     *event,
					 BstParam	    *bparam);
static gint dots_area_motion_event	(GtkWidget          *widget,
					 GdkEventMotion     *event,
					 BstParam	    *bparam);


/* --- variables --- */
static gboolean	bst_rcarg_params_use_tip_name = TRUE;


/* --- widget group --- */
#define _GROUP_GET_NAMED_WIDGET(g,n) ((GtkWidget*) gtk_object_get_data (GTK_OBJECT (g), n))
static void
_GROUP_ADD_NAMED_OBJECT (GtkWidget   *group,
			 const gchar *name,
			 gpointer     object)
{
  g_return_if_fail (GTK_IS_WIDGET (group));
  g_return_if_fail (GTK_IS_OBJECT (object));
  
  if (group != object)
    {
      gtk_object_ref (object);
      gtk_object_sink (object);
    }
  gtk_object_set_data_full (GTK_OBJECT (group), name, object,
			    (GtkDestroyNotify) (group != object ? gtk_object_unref : NULL));
}
static void
_GROUP_CALL (GtkWidget *group,
	     gpointer   f,
	     gpointer   d)
{
  static const gchar *names[] = {
    "group_container",
    "group_prompt",
    "group_dial",
    "group_scale",
    "group_pre_action",
    "group_post_action",
    "group_action",
  };
  guint i;

  g_return_if_fail (GTK_IS_WIDGET (group));

  gtk_widget_ref (group);
  for (i = 0; i < sizeof (names) / sizeof (names[0]); i++)
    {
      GtkWidget *callee = _GROUP_GET_NAMED_WIDGET (group, names[i]);

      if (callee)
	((GtkCallback) f) (callee, d);
    }
  gtk_widget_unref (group);
}
static GtkWidget*
GROUP_FORM (GtkWidget *parent,
	    GtkWidget *action)
{
  g_return_val_if_fail (GTK_IS_CONTAINER (parent), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (action), NULL);

  _GROUP_ADD_NAMED_OBJECT (action, "group_action", action);
  _GROUP_ADD_NAMED_OBJECT (action, "group_parent", parent);

  return action;
}
static GtkWidget*
GROUP_FORM_BIG (GtkWidget *parent,
		GtkWidget *action)
{
  g_return_val_if_fail (GTK_IS_CONTAINER (parent), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (action), NULL);

  _GROUP_ADD_NAMED_OBJECT (action, "group_action", action);
  _GROUP_ADD_NAMED_OBJECT (action, "group_parent", parent);
  _GROUP_ADD_NAMED_OBJECT (action, "group_big", action);

  return action;
}
#define GROUP_ADD_POST_ACTION(g, w) (_GROUP_ADD_NAMED_OBJECT ((g), "group_post_action", (w)))
#define GROUP_ADD_PRE_ACTION(g, w)  (_GROUP_ADD_NAMED_OBJECT ((g), "group_pre_action", (w)))
#define GROUP_ADD_PROMPT(g, w)      (_GROUP_ADD_NAMED_OBJECT ((g), "group_prompt", (w)))
#define GROUP_ADD_DIAL(g, w)        (_GROUP_ADD_NAMED_OBJECT ((g), "group_dial", (w)))
#define GROUP_ADD_SCALE(g, w)       (_GROUP_ADD_NAMED_OBJECT ((g), "group_scale", (w)))
#define GROUP_PARENT_STORE_TOOLTIPS(p, t) (_GROUP_ADD_NAMED_OBJECT ((p), "group_tooltips", (t)))
#define GROUP_GET_ACTION(g)	    (GTK_WIDGET (g))
#define GROUP_GET_PRE_ACTION(g)	    (_GROUP_GET_NAMED_WIDGET ((g), "group_pre_action"))
static void
GROUP_SET_TIP (GtkWidget   *group,
	       const gchar *tip_text,
	       const gchar *tip_private)
{
  GtkWidget *parent;
  GtkTooltips *tt;

  g_return_if_fail (GTK_IS_WIDGET (group));

  parent = _GROUP_GET_NAMED_WIDGET (group, "group_parent");
  tt = gtk_object_get_data (GTK_OBJECT (parent), "group_tooltips");
  if (tt && tip_text && !GTK_WIDGET_NO_WINDOW (group))
    gtk_tooltips_set_tip (tt, group, tip_text, tip_private);
}
static GtkWidget*
GROUP_DONE (GtkWidget *group)
{
  GtkWidget *parent, *container;
  GtkWidget *post_action, *pre_action, *prompt, *dial, *scale, *big;
  GtkBox *box;

  g_return_val_if_fail (GTK_IS_WIDGET (group), NULL);

  parent = _GROUP_GET_NAMED_WIDGET (group, "group_parent");
  container = gtk_widget_new (GTK_TYPE_HBOX,
			      "visible", TRUE,
			      "homogeneous", FALSE,
			      "spacing", 0,
			      "border_width", 0,
			      NULL);
  _GROUP_ADD_NAMED_OBJECT (group, "group_container", container);
  prompt = _GROUP_GET_NAMED_WIDGET (group, "group_prompt");
  dial = _GROUP_GET_NAMED_WIDGET (group, "group_dial");
  scale = _GROUP_GET_NAMED_WIDGET (group, "group_scale");
  pre_action = _GROUP_GET_NAMED_WIDGET (group, "group_pre_action");
  big = _GROUP_GET_NAMED_WIDGET (group, "group_big");
  post_action = _GROUP_GET_NAMED_WIDGET (group, "group_post_action");
  
  if (GTK_IS_BOX (parent))
    gtk_box_pack_start (GTK_BOX (parent), container, big != NULL, TRUE, 0);
  else
    gtk_container_add (GTK_CONTAINER (parent), container);

  box = GTK_BOX (container);
  if (prompt)
    gtk_box_pack_start (box, gtk_widget_get_toplevel (prompt), FALSE, TRUE, 0);
  if (dial)
    gtk_box_pack_start (box, gtk_widget_get_toplevel (dial), FALSE, TRUE, 5);
  if (scale)
    gtk_box_pack_start (box, gtk_widget_get_toplevel (scale), TRUE, TRUE, 5);
  if (prompt && (!dial && !scale))
    gtk_box_pack_start (box,
			gtk_widget_new (GTK_TYPE_ALIGNMENT,
					"visible", TRUE,
					NULL),
			FALSE, FALSE, 5);
  if (post_action)
    gtk_box_pack_end (box, gtk_widget_get_toplevel (post_action), FALSE, FALSE, 0);
  gtk_box_pack_end (box, gtk_widget_get_toplevel (group),
		    big != NULL || !prompt, !pre_action && !post_action, 0);
  if (pre_action)
    gtk_box_pack_end (box, gtk_widget_get_toplevel (pre_action), FALSE, FALSE, 0);

  return group;
}
#define GROUP_DESTROY(g)       (_GROUP_CALL ((g), gtk_widget_destroy, NULL))
#define GROUP_ENSURE_STYLES(g) (_GROUP_CALL ((g), gtk_widget_ensure_style, NULL))


/* --- functions --- */
static void
bst_string_toggle (GtkToggleButton *tb,
		   GtkWidget       *widget)
{
  gtk_widget_set_sensitive (widget, tb->active);
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
bst_bparam_bse_changed (BstParam     *bparam,
			BseParamSpec *pspec)
{
  if (bparam->param.pspec == pspec && !bparam->locked)
    bst_param_get (bparam);
}

#ifdef DEBUG_ADJUSTMENT
static void
debug_adjustment (GtkAdjustment *adjustment,
		  BstParam      *bparam)
{
  g_print ("adjustment changed: %f (%u)\n", adjustment->value, bparam->locked);
}
#endif

void
bst_param_destroy (BstParam *bparam)
{
  g_return_if_fail (BSE_IS_PARAM (bparam));
  g_return_if_fail (bparam->locked == 0);

  GROUP_DESTROY (bparam->group);
}

static void
bst_param_free (BstParam *bparam)
{
  if (bparam->is_object)
    bst_param_set_object (bparam, NULL);
  else if (bparam->is_procedure)
    bst_param_set_procedure (bparam, NULL);

  bse_param_free_value (&bparam->param);
  
  g_free (bparam);
}

void
bst_param_set_procedure (BstParam          *bparam,
			 BseProcedureClass *proc)
{
  g_return_if_fail (BSE_IS_PARAM (bparam));
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

void
bst_param_set_object (BstParam  *bparam,
		      BseObject *object)
{
  g_return_if_fail (BSE_IS_PARAM (bparam));
  g_return_if_fail (!bparam->locked);
  g_return_if_fail (bparam->is_object);
  if (object)
    g_return_if_fail (BSE_IS_OBJECT (object));

  if (bparam->owner)
    {
      bse_object_remove_notifier (bparam->owner, bparam->param_set_id);
      bse_object_unref (bparam->owner);
    }

  bparam->owner = object;
  if (bparam->owner)
    {
      /* we SHOULD make sure here that bparam->param->pspec is a valid pspec
       * for object->class (or its anchestors), but actually i don't feel
       * like writing the extra code just to issue a warning
       */
      bse_object_ref (bparam->owner);
      bparam->param_set_id = bse_object_add_data_notifier (object,
							   "param_changed",
							   bst_bparam_bse_changed,
							   bparam);
      if (bparam->group)
	bst_param_get (bparam);
    }
}

BstParam*
bst_param_create (gpointer      owner,
		  BseType	owner_type,
		  BseParamSpec *pspec,
		  GtkWidget    *parent,
		  GtkTooltips  *tooltips)
{
  GtkWidget *parent_container;
  gpointer widget_group;
  static GQuark null_group = 0;
  BstParam *bparam;
  GtkAdjustment *adjustment = NULL;
  guint digits = 0;
  GtkWidget *spinner = NULL;
  GtkWidget *scale = NULL;
  GtkWidget *dial = NULL;
  gboolean read_only, string_toggle, radio;
  gchar *name, *tooltip;

  if (BSE_TYPE_IS_PROCEDURE (owner_type))
    g_return_val_if_fail (BSE_IS_PROCEDURE_CLASS (owner), NULL);
  else
    g_return_val_if_fail (BSE_IS_OBJECT (owner), NULL);
  g_return_val_if_fail (BSE_IS_PARAM_SPEC (pspec), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (GTK_IS_TOOLTIPS (tooltips), NULL);
  
  if (!null_group)
    null_group = g_quark_from_static_string ("Bst-null-group");

  bparam = g_new0 (BstParam, 1);
  bse_param_init_default (&bparam->param, pspec);
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
  bse_type_class_ref (owner_type);
  
  parent_container = gtk_object_get_data_by_id (GTK_OBJECT (parent), pspec->any.param_group);
  if (!parent_container ||
      GTK_OBJECT_DESTROYED (parent_container) ||
      !GTK_IS_BOX (parent_container))
    {
      GtkWidget *any;
      
      any = gtk_widget_new (GTK_TYPE_VBOX,
			    "visible", TRUE,
			    "homogeneous", FALSE,
			    "spacing", 0,
			    "border_width", pspec->any.param_group ? 5 : 0,
			    NULL);
      parent_container = any;
      if (tooltips)
	GROUP_PARENT_STORE_TOOLTIPS (parent_container, tooltips);
      gtk_widget_ref (any);
      gtk_object_set_data_by_id_full (GTK_OBJECT (parent),
				      pspec->any.param_group ? pspec->any.param_group : null_group,
				      any,
				      (GtkDestroyNotify) gtk_widget_unref);
      if (pspec->any.param_group)
	any = gtk_widget_new (GTK_TYPE_FRAME,
			      "visible", TRUE,
			      "label", g_quark_to_string (pspec->any.param_group),
			      "child", any,
			      NULL);
      if (GTK_IS_BOX (parent))
	gtk_box_pack_start (GTK_BOX (parent), any, FALSE, TRUE, 0);
      else
	gtk_container_add (GTK_CONTAINER (parent), any);
    }
  parent = NULL;
  
  /* feature param hints and integral values
   */
  read_only = (pspec->any.flags & BSE_PARAM_HINT_RDONLY) != 0;
  radio = (pspec->any.flags & BSE_PARAM_HINT_RADIO) != 0;
  string_toggle = (pspec->any.flags & BSE_PARAM_HINT_CHECK_NULL) != 0;
  switch (pspec->type)
    {
    case BSE_TYPE_PARAM_INT:
      adjustment = (GtkAdjustment*) gtk_adjustment_new (pspec->s_int.default_value,
							pspec->s_int.minimum,
							pspec->s_int.maximum,
							1,
							pspec->s_int.stepping_rate,
							0);
      digits = 0;
      break;
    case BSE_TYPE_PARAM_UINT:
      adjustment = (GtkAdjustment*) gtk_adjustment_new (pspec->s_uint.default_value,
							pspec->s_uint.minimum,
							pspec->s_uint.maximum,
							1,
							pspec->s_uint.stepping_rate,
							0);
      digits = 0;
      break;
    case BSE_TYPE_PARAM_FLOAT:
      adjustment = (GtkAdjustment*) gtk_adjustment_new (pspec->s_float.default_value,
							pspec->s_float.minimum,
							pspec->s_float.maximum,
							MIN (0.1, pspec->s_float.stepping_rate),
							MAX (0.1, pspec->s_float.stepping_rate),
							0);
      digits = 3;
      break;
    case BSE_TYPE_PARAM_DOUBLE:
      adjustment = (GtkAdjustment*) gtk_adjustment_new (pspec->s_double.default_value,
							pspec->s_double.minimum,
							pspec->s_double.maximum,
							MIN (0.1, pspec->s_double.stepping_rate),
							MAX (0.1, pspec->s_double.stepping_rate),
							0);
      digits = 5;
      break;
    }
  if (adjustment)
    {
      gtk_object_ref (GTK_OBJECT (adjustment));
      gtk_object_sink (GTK_OBJECT (adjustment));
      
      /* we need to connect *after* the spinner so the spinner's value is
       * already updated
       */
      gtk_signal_connect_object_after (GTK_OBJECT (adjustment),
				       "value-changed",
				       bst_param_gtk_changed,
				       (GtkObject*) bparam);
#ifdef DEBUG_ADJUSTMENT
      gtk_signal_connect (GTK_OBJECT (adjustment), "value-changed", debug_adjustment, bparam);
#endif
      
      spinner = gtk_spin_button_new (adjustment, 0, digits);
      if (pspec->any.flags & BSE_PARAM_HINT_SCALE)
	{
	  scale = gtk_hscale_new (adjustment);
	  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
	}
      if (pspec->any.flags & BSE_PARAM_HINT_DIAL)
	{
	  dial = gtk_hscale_new (adjustment);
	  gtk_scale_set_draw_value (GTK_SCALE (dial), FALSE);
	  gtk_widget_set_usize (dial, 40, -2); /* FIXME: hack */
	}
      
      gtk_object_unref (GTK_OBJECT (adjustment));
    }
  
  name = pspec->any.name;
  tooltip = pspec->any.blurb;
  if (tooltip && bst_rcarg_params_use_tip_name)
    {
      name = tooltip;
      tooltip = NULL;
    }
  
  switch (pspec->type)
    {
      GtkWidget *action, *prompt, *pre_action, *post_action, *frame, *any, *group;
      guint width;
      DotAreaData *dot_data;

    case BSE_TYPE_PARAM_BOOL:
      action = gtk_widget_new (radio ? BST_TYPE_FREE_RADIO_BUTTON : GTK_TYPE_CHECK_BUTTON,
			       "visible", TRUE,
			       "label", name,
			       "sensitive", !read_only,
			       "object_signal::clicked", bst_param_gtk_changed, bparam,
			       NULL);
      gtk_misc_set_alignment (GTK_MISC (GTK_BIN (action)->child), 0, 0.5);
      group = GROUP_FORM (parent_container, action);
      GROUP_SET_TIP (group, tooltip, NULL);
      widget_group = GROUP_DONE (group);
      break;
    case BSE_TYPE_PARAM_INT:
    case BSE_TYPE_PARAM_UINT:
    case BSE_TYPE_PARAM_FLOAT:
    case BSE_TYPE_PARAM_DOUBLE:
    case BSE_TYPE_PARAM_TIME:
    case BSE_TYPE_PARAM_NOTE:
      switch (pspec->type)
	{
	case BSE_TYPE_PARAM_INT:
	case BSE_TYPE_PARAM_UINT:
	  width = 50;
	  break;
	case BSE_TYPE_PARAM_FLOAT:
	case BSE_TYPE_PARAM_DOUBLE:
	  width = 80;
	  break;
	case BSE_TYPE_PARAM_TIME:
	  width = 60;
	  break;
	case BSE_TYPE_PARAM_NOTE:
	  width = 50;
	  break;
	default:
	  width = 3;
	  break;
	}
      if (spinner)
	width += 10;
      prompt = gtk_widget_new (GTK_TYPE_LABEL,
			       "visible", TRUE,
			       "label", name,
			       "justify", GTK_JUSTIFY_LEFT,
			       "sensitive", !read_only,
			       NULL);
      action = spinner ? spinner : gtk_entry_new ();
      gtk_widget_set (action,
		      "visible", TRUE,
		      "width", width,
		      "object_signal::activate", bst_param_gtk_changed, bparam,
		      "sensitive", !read_only,
		      NULL);
      group = GROUP_FORM (parent_container, action);
      GROUP_ADD_PROMPT (group, prompt);
      if (scale)
	{
	  gtk_widget_show (scale);
	  gtk_widget_set_sensitive (scale, !read_only);
	  GROUP_ADD_SCALE (group, scale);
	}
      if (dial)
	{
	  gtk_widget_show (dial);
	  gtk_widget_set_sensitive (dial, !read_only);
	  GROUP_ADD_DIAL (group, dial);
	}
      GROUP_SET_TIP (group, tooltip, NULL);
      widget_group = GROUP_DONE (group);
      break;
    case BSE_TYPE_PARAM_ENUM:
    case BSE_TYPE_PARAM_FLAGS:
      prompt = gtk_widget_new (GTK_TYPE_LABEL,
			       "visible", TRUE,
			       "sensitive", FALSE,
			       "label", pspec->any.name,
			       "justify", GTK_JUSTIFY_LEFT,
			       "xalign", 0.0,
			       NULL);
      action = gtk_widget_new (GTK_TYPE_LABEL,
			       "visible", TRUE,
			       "label", pspec->any.blurb,
			       "justify", GTK_JUSTIFY_LEFT,
			       "xalign", 0.0,
			       NULL);
      group = GROUP_FORM (parent_container, action);
      GROUP_ADD_PROMPT (group, prompt);
      GROUP_SET_TIP (group, tooltip, NULL);
      widget_group = GROUP_DONE (group);
      break;
    case BSE_TYPE_PARAM_STRING:
      prompt = gtk_widget_new (GTK_TYPE_LABEL,
			      "visible", TRUE,
			      "label", name,
			      "justify", GTK_JUSTIFY_LEFT,
			      "sensitive", !read_only,
			      NULL);
      action = gtk_widget_new (GTK_TYPE_ENTRY,
			       "visible", TRUE,
			       "object_signal::activate", bst_param_gtk_changed, bparam,
			       "object_signal::focus_out_event", bst_param_gtk_update, bparam,
			       "sensitive", !read_only,
			       NULL);
      group = GROUP_FORM (parent_container, action);
      GROUP_ADD_PROMPT (group, prompt);
      if (string_toggle)
	{
	  pre_action = gtk_widget_new (GTK_TYPE_TOGGLE_BUTTON,
				       "visible", TRUE,
				       "can_focus", FALSE,
				       "width", 10,
				       "height", 10,
				       "parent", gtk_widget_new (GTK_TYPE_ALIGNMENT,
								 "visible", !read_only,
								 "xscale", 0.0,
								 "yscale", 0.0,
								 "xalign", 0.0,
								 "width", 13,
								 NULL),
				       "object_signal::clicked", bst_param_gtk_changed, bparam,
				       "signal::clicked", bst_string_toggle, action,
				       NULL);
	  bst_string_toggle (GTK_TOGGLE_BUTTON (pre_action), action);
	  GROUP_ADD_PRE_ACTION (group, pre_action);
	}
      GROUP_SET_TIP (group, tooltip, NULL);
      widget_group = GROUP_DONE (group);
      break;
    case BSE_TYPE_PARAM_DOTS:
      prompt = gtk_widget_new (GTK_TYPE_LABEL,
			      "visible", TRUE,
			      "label", name,
			      "justify", GTK_JUSTIFY_LEFT,
			      "sensitive", !read_only,
			      NULL);
      frame = gtk_widget_new (GTK_TYPE_FRAME,
			      "visible", TRUE,
			      "label", NULL,
			      "shadow", GTK_SHADOW_OUT,
			      "border_width", 5,
			      NULL);
      dot_data = g_new0 (DotAreaData, 1);
      action = gtk_widget_new (GTK_TYPE_DRAWING_AREA,
			       "visible", TRUE,
			       "height", 50,
			       "parent", frame,
			       "object_signal::destroy", g_free, dot_data,
			       "signal::configure_event", dots_area_configure_event, bparam,
			       "signal::expose_event", dots_area_expose_event, bparam,
			       "signal::enter_notify_event", dots_area_cross_event, bparam,
			       "signal::leave_notify_event", dots_area_cross_event, bparam,
			       "signal::button_press_event", dots_area_button_event, bparam,
			       "signal::button_release_event", dots_area_button_event, bparam,
			       "signal::motion_notify_event", dots_area_motion_event, bparam,
			       "events", (GDK_EXPOSURE_MASK |
					  GDK_ENTER_NOTIFY_MASK |
					  GDK_LEAVE_NOTIFY_MASK |
					  GDK_BUTTON_PRESS_MASK |
					  GDK_BUTTON_RELEASE_MASK |
					  GDK_BUTTON1_MOTION_MASK),
			       NULL);
      dot_data->cdot = -1;
      GTK_DRAWING_AREA (action)->draw_data = dot_data;
      group = GROUP_FORM_BIG (parent_container, action);
      GROUP_ADD_PROMPT (group, prompt);
      GROUP_SET_TIP (group, tooltip, NULL);
      widget_group = GROUP_DONE (group);
      break;
    case BSE_TYPE_PARAM_ITEM:
      prompt = gtk_widget_new (GTK_TYPE_LABEL,
			       "visible", TRUE,
			       "label", name,
			       "justify", GTK_JUSTIFY_LEFT,
			       "sensitive", !read_only,
			       NULL);
      action = gtk_widget_new (GTK_TYPE_ENTRY,
			       "visible", TRUE,
			       "width", 160,
			       "object_signal::activate", bst_param_gtk_changed, bparam,
			       "object_signal::focus_out_event", bst_param_gtk_update, bparam,
			       "sensitive", !read_only,
			       NULL);
      group = GROUP_FORM (parent_container, action);
      GROUP_ADD_PROMPT (group, prompt);
      any = gtk_widget_new (GTK_TYPE_CLUE_HUNTER,
			    "keep_history", FALSE,
			    "entry", action,
			    "user_data", bparam,
			    NULL);
      gtk_object_set_user_data (GTK_OBJECT (action), any);
      post_action = gtk_clue_hunter_create_arrow (GTK_CLUE_HUNTER (any));
      GROUP_ADD_POST_ACTION (group, post_action);
      GROUP_SET_TIP (group, tooltip, NULL);
      widget_group = GROUP_DONE (group);
      break;
    default:
      g_warning ("unknown param type: `%s'", pspec->any.name);
      widget_group = NULL;
      break;
    }
  
  bparam->group = widget_group;
  if (bparam->group)
    gtk_widget_set (bparam->group,
		    "signal::destroy", gtk_widget_destroyed, &bparam->group,
		    "object_signal::destroy", bst_param_free, bparam,
		    "object_signal::destroy", bse_type_class_unref, bse_type_class_peek (owner_type),
		    NULL);
  else
    g_return_val_if_fail (bparam->group != NULL, bparam);
  
  GROUP_ENSURE_STYLES (bparam->group);
  
  return bparam;
}

static void
bst_param_update (BstParam *bparam)
{
  GtkWidget *group = bparam->group;
  BseParam *param = &bparam->param;
  BseParamSpec *pspec = param->pspec;
  
  switch (pspec->type)
    {
      GtkWidget *action, *pre_action, *any;
      gchar *string;
      
    case BSE_TYPE_PARAM_BOOL:
      action = GROUP_GET_ACTION (group);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (action), param->value.v_bool);
      break;
    case BSE_TYPE_PARAM_INT:
    case BSE_TYPE_PARAM_UINT:
    case BSE_TYPE_PARAM_FLOAT:
    case BSE_TYPE_PARAM_DOUBLE:
    case BSE_TYPE_PARAM_TIME:
    case BSE_TYPE_PARAM_NOTE:
      action = GROUP_GET_ACTION (group);
      string = NULL; /* eek, cure stupid compiler */
      switch (pspec->type)
	{
	case BSE_TYPE_PARAM_INT:    string = g_strdup_printf ("%d", param->value.v_int);    break;
	case BSE_TYPE_PARAM_UINT:   string = g_strdup_printf ("%u", param->value.v_uint);   break;
	case BSE_TYPE_PARAM_FLOAT:  string = g_strdup_printf ("%f", param->value.v_float);  break;
	case BSE_TYPE_PARAM_DOUBLE: string = g_strdup_printf ("%f", param->value.v_double); break;
	case BSE_TYPE_PARAM_TIME:   string = bse_time_to_str (param->value.v_time);         break;
	case BSE_TYPE_PARAM_NOTE:   string = bse_note_to_string (param->value.v_note);      break;
	}
      if (!g_str_equal (gtk_entry_get_text (GTK_ENTRY (action)), string))
	{
	  gtk_entry_set_text (GTK_ENTRY (action), string);
#if 0
	  GtkRequisition requisition;
	  
	  gtk_widget_size_request (action, &requisition);
	  if (MAX (requisition.width, action->requisition.width) > action->allocation.width)
	    gtk_widget_set_usize (action,
				  MAX (requisition.width, action->requisition.width),
				  -1);
#endif
	  // gtk_entry_set_position (GTK_ENTRY (action), 0);
	  if (GTK_IS_SPIN_BUTTON (action))
	    gtk_spin_button_update (GTK_SPIN_BUTTON (action));
	}
      g_free (string);
      break;
    case BSE_TYPE_PARAM_STRING:
      action = GROUP_GET_ACTION (group);
      string = param->value.v_string;
      if (!string || !g_str_equal (gtk_entry_get_text (GTK_ENTRY (action)), string))
	{
	  gtk_entry_set_text (GTK_ENTRY (action), string ? string : "");
	  // gtk_entry_set_position (GTK_ENTRY (action), 0);
	}
      pre_action = GROUP_GET_PRE_ACTION (group);
      if (pre_action)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pre_action), string != NULL);
      break;
    case BSE_TYPE_PARAM_DOTS:
      action = GROUP_GET_ACTION (group);
      gtk_widget_queue_draw (action);
      break;
    case BSE_TYPE_PARAM_ITEM:
      action = GROUP_GET_ACTION (group);
      string = param->value.v_item ? BSE_OBJECT_NAME (param->value.v_item) : NULL;
      if (!bse_string_equals (gtk_entry_get_text (GTK_ENTRY (action)), string))
	{
	  gtk_entry_set_text (GTK_ENTRY (action), string ? string : "");
	  // gtk_entry_set_position (GTK_ENTRY (action), 0);
	}
      any = gtk_object_get_user_data (GTK_OBJECT (action));
      if (any)
	{
	  GtkClueHunter *ch = GTK_CLUE_HUNTER (any);
	  
	  if (pspec->s_item.item_type == BSE_TYPE_SAMPLE)
	    {
	      GList *free_list = bst_sample_repo_list_sample_locs (), *list;
	      
	      gtk_clue_hunter_remove_matches (ch, "*");
	      
	      for (list = free_list; list; list = list->next)
		{
		  BstSampleLoc *loc = list->data;
		  
		  /* FIXME: showing the repo as well would be nice */
		  gtk_clue_hunter_add_string (ch, loc->name);
		}
	      g_list_free (free_list);
	    }
	}
      break;
    case BSE_TYPE_PARAM_ENUM:
    case BSE_TYPE_PARAM_FLAGS:
    default:
      g_warning ("unknown param type: `%s'", pspec->any.name);
      break;
    }
}

static gboolean
bst_param_apply (BstParam *bparam,
		 gboolean *changed)
{
  GtkWidget *group = bparam->group;
  BseParam *param = &bparam->param;
  BseParamSpec *pspec = param->pspec;
  BseParam param2 = { 0, };
  guint dirty = 0;

  bse_param_init (&param2, pspec);
  bse_param_copy_value (param, &param2);

  *changed = FALSE;

  switch (pspec->type)
    {
      GtkWidget *action, *pre_action;
      gchar *string, *dummy;
      BseTime time_data;
      guint base;
      guint note_data;

    case BSE_TYPE_PARAM_BOOL:
      action = GROUP_GET_ACTION (group);
      dirty += bse_param_set_bool (param, GTK_TOGGLE_BUTTON (action)->active);
      break;
    case BSE_TYPE_PARAM_INT:
      action = GROUP_GET_ACTION (group);
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
      dirty += bse_param_set_int (param, strtol (string, &dummy, base));
      break;
    case BSE_TYPE_PARAM_UINT:
      action = GROUP_GET_ACTION (group);
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
      dirty += bse_param_set_uint (param, strtol (string, &dummy, base));
      break;
    case BSE_TYPE_PARAM_FLOAT:
      action = GROUP_GET_ACTION (group);
      dirty += bse_param_set_float (param, g_strtod (gtk_entry_get_text (GTK_ENTRY (action)), &dummy));
      break;
    case BSE_TYPE_PARAM_DOUBLE:
      action = GROUP_GET_ACTION (group);
      dirty += bse_param_set_double (param, g_strtod (gtk_entry_get_text (GTK_ENTRY (action)), &dummy));
      break;
    case BSE_TYPE_PARAM_TIME:
      action = GROUP_GET_ACTION (group);
      time_data = bse_time_from_string (gtk_entry_get_text (GTK_ENTRY (action)), NULL);
      if (time_data)
	dirty += bse_param_set_time (param, time_data);
      else
	dirty++;
      break;
    case BSE_TYPE_PARAM_NOTE:
      action = GROUP_GET_ACTION (group);
      note_data = bse_note_from_string (gtk_entry_get_text (GTK_ENTRY (action)));
      if (note_data != BSE_NOTE_UNPARSABLE)
	dirty += bse_param_set_note (param, note_data);
      else
	dirty++;
      break;
    case BSE_TYPE_PARAM_STRING:
      action = GROUP_GET_ACTION (group);
      pre_action = GROUP_GET_PRE_ACTION (group);
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
      dirty += bse_param_set_string (param, string);
      break;
    case BSE_TYPE_PARAM_DOTS:
      *changed = TRUE;
      break;
    case BSE_TYPE_PARAM_ITEM:
      action = GROUP_GET_ACTION (group);
      string = bse_strdup_stripped (gtk_entry_get_text (GTK_ENTRY (action)));
      if (string && bparam->is_object)
	{
	  GList *list, *free_list;
	  gboolean try_further = TRUE;

	  free_list = bse_objects_list_by_name (pspec->s_item.item_type, string);
	  for (list = free_list; list; list = list->next)
	    {
	      BseSuper *super = list->data;

	      if (bse_super_get_project (super) == bse_item_get_project (BSE_ITEM (bparam->owner)))
		{
		  dirty += bse_param_set_item (param, BSE_ITEM (super));
		  try_further = FALSE;
		  break;
		}
	    }
	  g_list_free (free_list);

	  if (try_further && pspec->s_item.item_type == BSE_TYPE_SAMPLE)
	    {
	      BstSampleLoc *loc = bst_sample_repo_find_sample_loc (string);

	      if (loc)
		{
		  BseSample *sample;

		  g_message ("demand load: %s (%s)", loc->name, loc->repo->name);

		  sample = bst_sample_repo_load_sample (loc,
							bse_item_get_project (BSE_ITEM (bparam->owner)));
		  dirty += bse_param_set_item (param, BSE_ITEM (sample));
		}
	    }

	  dirty += 1;
	  g_free (string);
	}
      else
	dirty += bse_param_set_item (param, NULL);
      break;
    case BSE_TYPE_PARAM_ENUM:
    case BSE_TYPE_PARAM_FLAGS:
    default:
      g_warning ("unknown param type: `%s'", pspec->any.name);
      break;
    }

  *changed |= bse_param_values_cmp (param, &param2) != 0;
  bse_param_free_value (&param2);

  dirty += *changed;

  return dirty > 0;
}

void
bst_param_get (BstParam *bparam)
{
  g_return_if_fail (BSE_IS_PARAM (bparam));

  bparam->locked++;
  if (bparam->is_object && bparam->owner)
    bse_object_get_param (bparam->owner, &bparam->param);
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

  g_return_if_fail (BSE_IS_PARAM (bparam));

  bparam->locked++;
  dirty = bst_param_apply (bparam, &changed);
  if (changed && bparam->is_object && bparam->owner)
    bse_object_set_param (bparam->owner, &bparam->param);

  if (dirty)
    bst_param_get (bparam);
  else
    bparam->locked = 0;
}

gboolean
bst_param_set_from_other (BstParam *bparam,
			  BseParam *src_param)
{
  gboolean success;

  g_return_val_if_fail (BSE_IS_PARAM (bparam), FALSE);
  g_return_val_if_fail (BSE_IS_PARAM (src_param), FALSE);

  success = bse_param_value_convert (src_param, &bparam->param);

  if (success)
    {
      if (bparam->is_object && bparam->owner)
	bse_object_set_param (bparam->owner, &bparam->param);
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
  g_return_if_fail (BSE_IS_PARAM (bparam));

  if (bparam->group)
    {
      GtkWidget *action = GROUP_GET_ACTION (bparam->group);
      
      if (GTK_IS_EDITABLE (action))
	gtk_editable_set_editable (GTK_EDITABLE (action), editable);
      else
	gtk_widget_set_sensitive (action, editable);
    }
}

static gint
dots_area_configure_event (GtkWidget         *widget,
			   GdkEventConfigure *event,
			   BstParam          *bparam)
{
  gdk_window_set_background (widget->window, &widget->style->base[GTK_WIDGET_STATE (widget)]);

  return TRUE;
}

static gint
dots_area_cross_event (GtkWidget        *widget,
		       GdkEventCrossing *event,
		       BstParam         *bparam)
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
  guint n_dots = bparam->param.pspec->s_dots.n_dots;
  BseDot *dots = bparam->param.value.v_dots;
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
  guint n_dots = bparam->param.pspec->s_dots.n_dots;
  BseDot *dots = bparam->param.value.v_dots;
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
	  
	  bse_param_set_dot (&bparam->param, data->cdot,
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
      
      bse_param_set_dot (&bparam->param, data->cdot,
			 x,
			 1.0 - y);
      
      bst_param_set (bparam);
    }
  
  return TRUE;
}
