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


/* --- functions --- */
static void
bst_toggle_sensitive (GtkToggleButton *tb,
		      GtkWidget       *widget)
{
  gtk_widget_set_sensitive (widget, tb->active);
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

  gtk_widget_destroy (bparam->widget);
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
      if (bparam->widget)
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
      if (bparam->widget)
	bst_param_get (bparam);
    }
}

BstParam*
bst_param_create (gpointer      owner,
		  BseType	owner_type,
		  BseParamSpec *pspec,
		  GtkBox       *parent_box,
		  GtkTooltips  *tooltips)
{
  BstParam *bparam;
  GtkWidget *widget;
  GtkAdjustment *adjustment = NULL;
  guint digits = 0;
  GtkWidget *spinner = NULL;
  GtkWidget *scale = NULL;
  GtkWidget *dial = NULL;
  gboolean read_only, string_toggle;
  gchar *name, *tooltip;

  if (BSE_TYPE_IS_PROCEDURE (owner_type))
    g_return_val_if_fail (BSE_IS_PROCEDURE_CLASS (owner), NULL);
  else
    g_return_val_if_fail (BSE_IS_OBJECT (owner), NULL);
  g_return_val_if_fail (BSE_IS_PARAM_SPEC (pspec), NULL);
  g_return_val_if_fail (GTK_IS_BOX (parent_box), NULL);
  g_return_val_if_fail (GTK_IS_TOOLTIPS (tooltips), NULL);

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
  
  if (pspec->any.param_group)
    {
      GtkWidget *box;
      
      box = gtk_object_get_data_by_id (GTK_OBJECT (parent_box), pspec->any.param_group);
      if (!box || GTK_OBJECT_DESTROYED (box))
	{
	  GtkWidget *frame;

	  frame = gtk_widget_new (GTK_TYPE_FRAME,
				  "visible", TRUE,
				  "label", g_quark_to_string (pspec->any.param_group),
				  NULL),
	  box = gtk_widget_new (GTK_TYPE_VBOX,
				"visible", TRUE,
				"homogeneous", FALSE,
				"spacing", 0,
				"border_width", 5,
				"parent", frame,
				NULL);
	  gtk_box_pack_start (GTK_BOX (parent_box), frame, FALSE, TRUE, 0);
	  gtk_widget_ref (box);
	  gtk_object_set_data_by_id_full (GTK_OBJECT (parent_box),
					  pspec->any.param_group,
					  box,
					  (GtkDestroyNotify) gtk_widget_unref);
	}
      parent_box = GTK_BOX (box);
    }
  

  /* feature param hints and integral values
   */
  read_only = (pspec->any.flags & BSE_PARAM_HINT_RDONLY) != 0;
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
	  gtk_widget_set_usize (scale, 80, -2); /* FIXME: hack */
	}
      if (pspec->any.flags & BSE_PARAM_HINT_DIAL)
	{
	  dial = gtk_hscale_new (adjustment);
	  gtk_scale_set_draw_value (GTK_SCALE (dial), FALSE);
	  gtk_widget_set_usize (dial, 80, -2); /* FIXME: hack */
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
      GtkWidget *box, *button, *label, *frame, *any;
      guint width;
      DotAreaData *dot_data;

    case BSE_TYPE_PARAM_BOOL:
      widget = gtk_widget_new (GTK_TYPE_CHECK_BUTTON,
			       "label", name,
			       "visible", TRUE,
			       "sensitive", !read_only,
			       "object_signal::clicked", bst_param_gtk_changed, bparam,
			       NULL);
      gtk_misc_set_alignment (GTK_MISC (GTK_BIN (widget)->child), 0, 0.5);
      gtk_box_pack_start (parent_box, widget, FALSE, FALSE, 0);
      if (tooltip)
	gtk_tooltips_set_tip (tooltips, widget, tooltip, NULL);
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
	  width = 40;
	  break;
	default:
	  width = 3;
	  break;
	}
      if (spinner)
	width += 10;
      box = gtk_widget_new (GTK_TYPE_HBOX,
			    "homogeneous", FALSE,
			    "spacing", 10,
			    "border_width", 0,
			    "visible", TRUE,
			    NULL);
      gtk_box_pack_start (parent_box, box, FALSE, TRUE, 0);
      label = gtk_widget_new (GTK_TYPE_LABEL,
			      "label", name,
			      "justify", GTK_JUSTIFY_LEFT,
			      "visible", TRUE,
			      "sensitive", !read_only,
			      NULL);
      gtk_box_pack_start (GTK_BOX (box), label, FALSE, TRUE, 0);
      widget = spinner ? spinner : gtk_entry_new ();
      gtk_widget_set (widget,
		      "visible", TRUE,
		      "width", width,
		      "object_signal::activate", bst_param_gtk_changed, bparam,
		      "sensitive", !read_only,
		      NULL);
      gtk_box_pack_end (GTK_BOX (box), widget, FALSE, TRUE, 0);
      if (scale)
	{
	  gtk_widget_show (scale);
	  gtk_box_pack_end (GTK_BOX (box), scale, TRUE, TRUE, 0);
	  gtk_widget_set_sensitive (scale, !read_only);
	}
      if (dial)
	{
	  gtk_widget_show (dial);
	  gtk_box_pack_end (GTK_BOX (box), dial, FALSE, TRUE, 0);
	  gtk_widget_set_sensitive (dial, !read_only);
	}
      if (tooltip)
	gtk_tooltips_set_tip (tooltips, widget, tooltip, NULL);
      break;
    case BSE_TYPE_PARAM_ENUM:
    case BSE_TYPE_PARAM_FLAGS:
      widget = gtk_widget_new (GTK_TYPE_HBOX,
			       "visible", TRUE,
			       "homogeneous", FALSE,
			       "child", gtk_widget_new (GTK_TYPE_LABEL,
							"visible", TRUE,
							"sensitive", FALSE,
							"label", pspec->any.name,
							"justify", GTK_JUSTIFY_LEFT,
							"xalign", 0.0,
							NULL),
			       "child", gtk_widget_new (GTK_TYPE_LABEL,
							"visible", TRUE,
							"label", pspec->any.blurb,
							"justify", GTK_JUSTIFY_LEFT,
							"xalign", 0.0,
							NULL),
			       "parent", parent_box,
			       NULL);
      break;
    case BSE_TYPE_PARAM_STRING:
      box = gtk_widget_new (GTK_TYPE_HBOX,
			    "homogeneous", FALSE,
			    "spacing", 10,
			    "border_width", 0,
			    "visible", TRUE,
			    NULL);
      gtk_box_pack_start (parent_box, box, FALSE, TRUE, 0);
      label = gtk_widget_new (GTK_TYPE_LABEL,
			      "label", name,
			      "justify", GTK_JUSTIFY_LEFT,
			      "visible", TRUE,
			      "sensitive", !read_only,
			      NULL);
      gtk_box_pack_start (GTK_BOX (box), label, FALSE, TRUE, 0);
      if (string_toggle)
	button = gtk_widget_new (GTK_TYPE_CHECK_BUTTON,
				 "visible", !read_only,
				 "object_signal::clicked", bst_param_gtk_changed, bparam,
				 NULL);
      else
	button = NULL;
      widget = gtk_widget_new (GTK_TYPE_ENTRY,
			       "width", 200,
			       "visible", TRUE,
			       "user_data", button,
			       "object_signal::activate", bst_param_gtk_changed, bparam,
			       "object_signal::focus_out_event", bst_param_gtk_update, bparam,
			       "sensitive", !read_only,
			       NULL);
      gtk_box_pack_end (GTK_BOX (box), widget, FALSE, TRUE, 0);
      if (tooltip)
	  gtk_tooltips_set_tip (tooltips, widget, tooltip, NULL);
      if (button)
	{
	  gtk_signal_connect (GTK_OBJECT (button),
			      "clicked",
			      GTK_SIGNAL_FUNC (bst_toggle_sensitive),
			      widget);
	  gtk_box_pack_end (GTK_BOX (box), button, FALSE, FALSE, 0);
	  if (tooltip)
	    gtk_tooltips_set_tip (tooltips, button, tooltip, NULL);
	  bst_toggle_sensitive (GTK_TOGGLE_BUTTON (button), widget);
	}
      break;
    case BSE_TYPE_PARAM_DOTS:
      box = gtk_widget_new (GTK_TYPE_HBOX,
			    "homogeneous", FALSE,
			    "spacing", 10,
			    "border_width", 0,
			    "visible", TRUE,
			    NULL);
      gtk_box_pack_start (parent_box, box, FALSE, TRUE, 0);
      label = gtk_widget_new (GTK_TYPE_LABEL,
			      "label", name,
			      "justify", GTK_JUSTIFY_LEFT,
			      "visible", TRUE,
			      "sensitive", !read_only,
			      NULL);
      gtk_box_pack_start (GTK_BOX (box), label, FALSE, TRUE, 0);
      frame = gtk_widget_new (GTK_TYPE_FRAME,
			      "visible", TRUE,
			      "label", NULL,
			      "shadow", GTK_SHADOW_OUT,
			      "parent", box,
			      "border_width", 5,
			      NULL);
      dot_data = g_new0 (DotAreaData, 1);
      widget = gtk_widget_new (GTK_TYPE_DRAWING_AREA,
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
      GTK_DRAWING_AREA (widget)->draw_data = dot_data;
      if (tooltip)
	  gtk_tooltips_set_tip (tooltips, widget, tooltip, NULL);
      break;
    case BSE_TYPE_PARAM_ITEM:
      box = gtk_widget_new (GTK_TYPE_HBOX,
			    "homogeneous", FALSE,
			    "spacing", 0 /* special */,
			    "border_width", 0,
			    "visible", TRUE,
			    NULL);
      gtk_box_pack_start (parent_box, box, FALSE, TRUE, 0);
      label = gtk_widget_new (GTK_TYPE_LABEL,
			      "label", name,
			      "justify", GTK_JUSTIFY_LEFT,
			      "visible", TRUE,
			      "sensitive", !read_only,
			      NULL);
      gtk_box_pack_start (GTK_BOX (box), label, FALSE, TRUE, 0);
      widget = gtk_widget_new (GTK_TYPE_ENTRY,
			       "width", 200,
			       "visible", TRUE,
			       "object_signal::activate", bst_param_gtk_changed, bparam,
			       "object_signal::focus_out_event", bst_param_gtk_update, bparam,
			       "sensitive", !read_only,
			       NULL);
      any = gtk_widget_new (GTK_TYPE_CLUE_HUNTER,
			    "keep_history", FALSE,
			    "entry", widget,
			    "user_data", bparam,
			    NULL);
      gtk_object_set_user_data (GTK_OBJECT (widget), any);
      gtk_box_pack_end (GTK_BOX (box),
			gtk_clue_hunter_create_arrow (GTK_CLUE_HUNTER (any)),
			FALSE, FALSE, 0);
      gtk_box_pack_end (GTK_BOX (box), widget, FALSE, TRUE, 0);
      if (tooltip)
	  gtk_tooltips_set_tip (tooltips, widget, tooltip, NULL);
      break;
    default:
      g_warning ("unknown param type: `%s'", pspec->any.name);
      widget = NULL;
      break;
    }
  
  bparam->widget = widget;
  gtk_signal_connect (GTK_OBJECT (widget),
		      "destroy",
		      gtk_widget_destroyed,
		      &bparam->widget);
  gtk_signal_connect_object (GTK_OBJECT (widget),
			     "destroy",
			     bst_param_free,
			     (GtkObject*) bparam);
  gtk_signal_connect_object (GTK_OBJECT (widget),
			     "destroy",
			     bse_type_class_unref,
			     bse_type_class_peek (owner_type));
  
  gtk_widget_ensure_style (widget);
  
  return bparam;
}

static void
bst_param_update (BstParam *bparam)
{
  GtkWidget *widget = bparam->widget;
  BseParam *param = &bparam->param;
  BseParamSpec *pspec = param->pspec;

  switch (pspec->type)
    {
      GtkWidget *button, *any;
      gchar *string;

    case BSE_TYPE_PARAM_BOOL:
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), param->value.v_bool);
      break;
    case BSE_TYPE_PARAM_INT:
      string = g_strdup_printf ("%d", param->value.v_int);
      if (!g_str_equal (gtk_entry_get_text (GTK_ENTRY (widget)), string))
	{
	  gtk_entry_set_text (GTK_ENTRY (widget), string);
	  // gtk_entry_set_position (GTK_ENTRY (widget), 0);
	  if (GTK_IS_SPIN_BUTTON (widget))
	    gtk_spin_button_update (GTK_SPIN_BUTTON (widget));
	}
      g_free (string);
      break;
    case BSE_TYPE_PARAM_UINT:
      string = g_strdup_printf ("%u", param->value.v_uint);
      if (!g_str_equal (gtk_entry_get_text (GTK_ENTRY (widget)), string))
	{
	  gtk_entry_set_text (GTK_ENTRY (widget), string);
	  // gtk_entry_set_position (GTK_ENTRY (widget), 0);
	  if (GTK_IS_SPIN_BUTTON (widget))
	    gtk_spin_button_update (GTK_SPIN_BUTTON (widget));
	}
      g_free (string);
      break;
    case BSE_TYPE_PARAM_FLOAT:
      string = g_strdup_printf ("%f", param->value.v_float);
      if (!g_str_equal (gtk_entry_get_text (GTK_ENTRY (widget)), string))
	{
	  gtk_entry_set_text (GTK_ENTRY (widget), string);
	  // gtk_entry_set_position (GTK_ENTRY (widget), 0);
	  if (GTK_IS_SPIN_BUTTON (widget))
	    gtk_spin_button_update (GTK_SPIN_BUTTON (widget));
	}
      g_free (string);
      break;
    case BSE_TYPE_PARAM_DOUBLE:
      string = g_strdup_printf ("%f", param->value.v_double);
      if (!g_str_equal (gtk_entry_get_text (GTK_ENTRY (widget)), string))
	{
	  gtk_entry_set_text (GTK_ENTRY (widget), string);
	  // gtk_entry_set_position (GTK_ENTRY (widget), 0);
	  if (GTK_IS_SPIN_BUTTON (widget))
	    gtk_spin_button_update (GTK_SPIN_BUTTON (widget));
	}
      g_free (string);
      break;
    case BSE_TYPE_PARAM_TIME:
      string = bse_time_to_str (param->value.v_time);
      if (!g_str_equal (gtk_entry_get_text (GTK_ENTRY (widget)), string))
	{
	  GtkRequisition requisition;

	  gtk_entry_set_text (GTK_ENTRY (widget), string);
	  gtk_widget_size_request (widget, &requisition);
	  gtk_widget_set_usize (widget,
				MAX (requisition.width, widget->requisition.width),
				-1);
	  // gtk_entry_set_position (GTK_ENTRY (widget), 0);
	  if (GTK_IS_SPIN_BUTTON (widget))
	    gtk_spin_button_update (GTK_SPIN_BUTTON (widget));
	}
      g_free (string);
      break;
    case BSE_TYPE_PARAM_NOTE:
      string = bse_note_to_string (param->value.v_note);
      if (!g_str_equal (gtk_entry_get_text (GTK_ENTRY (widget)), string))
	{
	  gtk_entry_set_text (GTK_ENTRY (widget), string);
	  // gtk_entry_set_position (GTK_ENTRY (widget), 0);
	  if (GTK_IS_SPIN_BUTTON (widget))
	    gtk_spin_button_update (GTK_SPIN_BUTTON (widget));
	}
      g_free (string);
      break;
    case BSE_TYPE_PARAM_STRING:
      string = param->value.v_string;
      if (!string || !g_str_equal (gtk_entry_get_text (GTK_ENTRY (widget)), string))
	{
	  gtk_entry_set_text (GTK_ENTRY (widget), string ? string : "");
	  // gtk_entry_set_position (GTK_ENTRY (widget), 0);
	  if (GTK_IS_SPIN_BUTTON (widget))
	    gtk_spin_button_update (GTK_SPIN_BUTTON (widget));
	}
      button = gtk_object_get_user_data (GTK_OBJECT (widget));
      if (button)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), string != NULL);
      break;
    case BSE_TYPE_PARAM_DOTS:
      gtk_widget_queue_draw (widget);
      break;
    case BSE_TYPE_PARAM_ITEM:
      string = param->value.v_item ? BSE_OBJECT_NAME (param->value.v_item) : NULL;
      if (!bse_string_equals (gtk_entry_get_text (GTK_ENTRY (widget)), string))
	{
	  gtk_entry_set_text (GTK_ENTRY (widget), string ? string : "");
	  // gtk_entry_set_position (GTK_ENTRY (widget), 0);
	}
      any = gtk_object_get_user_data (GTK_OBJECT (widget));
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
      widget = NULL;
      break;
    }
}

static gboolean
bst_param_apply (BstParam *bparam,
		 gboolean *changed)
{
  GtkWidget *widget = bparam->widget;
  BseParam *param = &bparam->param;
  BseParamSpec *pspec = param->pspec;
  BseParam param2 = { 0, };
  guint dirty = 0;

  bse_param_init (&param2, pspec);
  bse_param_copy_value (param, &param2);

  *changed = FALSE;

  switch (pspec->type)
    {
      GtkWidget *button;
      gchar *string, *dummy;
      BseTime time_data;
      guint base;
      guint note_data;

    case BSE_TYPE_PARAM_BOOL:
      dirty += bse_param_set_bool (param, GTK_TOGGLE_BUTTON (widget)->active);
      break;
    case BSE_TYPE_PARAM_INT:
      string = gtk_entry_get_text (GTK_ENTRY (widget));
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
      string = gtk_entry_get_text (GTK_ENTRY (widget));
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
      dirty += bse_param_set_float (param, g_strtod (gtk_entry_get_text (GTK_ENTRY (widget)), &dummy));
      break;
    case BSE_TYPE_PARAM_DOUBLE:
      dirty += bse_param_set_double (param, g_strtod (gtk_entry_get_text (GTK_ENTRY (widget)), &dummy));
      break;
    case BSE_TYPE_PARAM_TIME:
      time_data = bse_time_from_string (gtk_entry_get_text (GTK_ENTRY (widget)), NULL);
      if (time_data)
	dirty += bse_param_set_time (param, time_data);
      else
	dirty++;
      break;
    case BSE_TYPE_PARAM_NOTE:
      note_data = bse_note_from_string (gtk_entry_get_text (GTK_ENTRY (widget)));
      if (note_data != BSE_NOTE_UNPARSABLE)
	dirty += bse_param_set_note (param, note_data);
      else
	dirty++;
      break;
    case BSE_TYPE_PARAM_STRING:
      button = gtk_object_get_user_data (GTK_OBJECT (widget));
      if (!button || GTK_TOGGLE_BUTTON (button)->active)
	string = gtk_entry_get_text (GTK_ENTRY (widget));
      else
	string = button ? NULL : "";
      dirty += bse_param_set_string (param, string);
      break;
    case BSE_TYPE_PARAM_DOTS:
      *changed = TRUE;
      break;
    case BSE_TYPE_PARAM_ITEM:
      string = bse_strdup_stripped (gtk_entry_get_text (GTK_ENTRY (widget)));
      if (string && bparam->is_object)
	{
	  GList *list, *free_list;
	  gboolean try_further = TRUE;

	  free_list = bse_objects_list_by_name (pspec->s_item.item_type, string);
	  for (list = free_list; list; list = list->next)
	    {
	      BseSuper *super = list->data;

	      if (bse_super_get_project (super) ==
		  bse_item_get_project (BSE_ITEM (bparam->owner)))
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

  if (bparam->widget)
    {
      if (GTK_IS_EDITABLE (bparam->widget))
	gtk_editable_set_editable (GTK_EDITABLE (bparam->widget), editable);
      else
	gtk_widget_set_sensitive (bparam->widget, editable);
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

  /* draw background
   */
  gdk_draw_rectangle (drawable, bg_gc,
		      TRUE,
		      0,
		      0,
		      width,
		      height);

  for (i = 0; i < n_dots - 1; i++)
    gdk_draw_line (drawable, fg_gc,
		   maxx * dots[i].x,
		   maxy * (1.0 - dots[i].y),
		   maxx * dots[i + 1].x,
		   maxy * (1.0 - dots[i + 1].y));
  
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
