/* BEAST - Bedevilled Audio System
 * Copyright (C) 1999, 2000, 2001 Tim Janik
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
#include "bstknob.h"

#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <math.h>
#include <stdio.h>

#define SCROLL_DELAY_LENGTH	300
#define SQR(x)  ((x) * (x))

enum {
  MOUSE_SETUP,
  MOUSE_MOVE,
  MOUSE_JUMP
};


static void     bst_knob_class_init                 (BstKnobClass   *class);
static void     bst_knob_init                       (BstKnob        *knob);
static void     bst_knob_destroy                    (GtkObject      *object);
static void     bst_knob_realize                    (GtkWidget      *widget);
static void     bst_knob_unrealize                  (GtkWidget      *widget);
static void     bst_knob_map                        (GtkWidget      *widget);
static void     bst_knob_unmap                      (GtkWidget      *widget);
static void     bst_knob_size_request               (GtkWidget      *widget,
                                                     GtkRequisition *requisition);
static void     bst_knob_size_allocate              (GtkWidget      *widget,
                                                     GtkAllocation  *allocation);
static gint     bst_knob_expose                     (GtkWidget      *widget,
                                                     GdkEventExpose *event);
static void	bst_knob_paint			    (BstKnob	    *knob);
static gint     bst_knob_button_press               (GtkWidget      *widget,
                                                     GdkEventButton *event);
static gint     bst_knob_button_release             (GtkWidget      *widget,
                                                     GdkEventButton *event);
static gint     bst_knob_motion_notify              (GtkWidget      *widget,
                                                     GdkEventMotion *event);
static gboolean bst_knob_timer                      (gpointer        data);
static void     bst_knob_mouse_update               (BstKnob        *knob,
                                                     gint            x,
                                                     gint            y,
						     guint	     mode);
static void     bst_knob_update                     (BstKnob        *knob);
static void     bst_knob_adjustment_changed         (GtkAdjustment  *adjustment,
                                                     gpointer        data);
static void     bst_knob_adjustment_value_changed   (GtkAdjustment  *adjustment,
                                                     gpointer        data);

static GtkWidgetClass *parent_class = NULL;

GtkType
bst_knob_get_type (void)
{
  static GtkType knob_type = 0;
  
  if (!knob_type)
    {
      GtkTypeInfo knob_info =
      {
        "BstKnob",
        sizeof (BstKnob),
        sizeof (BstKnobClass),
        (GtkClassInitFunc) bst_knob_class_init,
        (GtkObjectInitFunc) bst_knob_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };
      
      knob_type = gtk_type_unique (GTK_TYPE_IMAGE, &knob_info);
    }
  
  return knob_type;
}

static void
bst_knob_class_init (BstKnobClass *class)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  parent_class = g_type_class_peek_parent (class);
  
  object_class->destroy = bst_knob_destroy;
  
  widget_class->size_request = bst_knob_size_request;
  widget_class->size_allocate = bst_knob_size_allocate;
  widget_class->realize = bst_knob_realize;
  widget_class->unrealize = bst_knob_unrealize;
  widget_class->map = bst_knob_map;
  widget_class->unmap = bst_knob_unmap;
  widget_class->expose_event = bst_knob_expose;
  widget_class->button_press_event = bst_knob_button_press;
  widget_class->button_release_event = bst_knob_button_release;
  widget_class->motion_notify_event = bst_knob_motion_notify;

  /* sets text[PRELIGHT] to red */
  gtk_rc_parse_string ("style'BstKnobClass-style'{text[PRELIGHT]={1.,0.,0.}}"
		       "widget_class'*BstKnob'style'BstKnobClass-style'");
}

static void
bst_knob_init (BstKnob *knob)
{
#include "icons/knob.c"
  gfloat w, h, radius;

  knob->update_policy = GTK_UPDATE_CONTINUOUS;
  knob->button = 0;
  knob->arc_start = 1.25 * M_PI;	/* 0 .. 2*M_PI */
  knob->arc_dist  = 1.5 * M_PI;		/* 0 .. 2*M_PI */
  knob->timer = 0;
  knob->angle_range = 0.0;
  knob->old_value = 0.0;
  knob->old_lower = 0.0;
  knob->old_upper = 0.0;
  knob->old_page_size = 0.0;
  knob->adjustment = NULL;
  knob->pixbuf = gdk_pixbuf_new_from_inline (-1, knob_pixbuf, FALSE, NULL);
  g_assert (knob->pixbuf);

  gtk_image_set_from_pixbuf (GTK_IMAGE (knob), knob->pixbuf);
  w = gdk_pixbuf_get_width (knob->pixbuf);
  h = gdk_pixbuf_get_height (knob->pixbuf);
  radius = (w + h) / 4.;
  knob->furrow_radius = radius * 0.65;
  knob->dot_radius = radius * 0.81;
  knob->xofs = 1;
  knob->yofs = 0;
}

static void
bst_knob_destroy (GtkObject *object)
{
  BstKnob *knob = BST_KNOB (object);

  if (knob->timer)
    {
      gtk_timeout_remove (knob->timer);
      knob->timer = 0;
    }
  
  if (knob->adjustment)
    {
      gtk_object_unref (knob->adjustment);
      knob->adjustment = NULL;
    }

  if (knob->pixbuf)
    {
      g_object_unref (knob->pixbuf);
      knob->pixbuf = NULL;
    }
  
  /* chain parent class' handler */
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bst_knob_size_request (GtkWidget      *widget,
			GtkRequisition *requisition)
{
  // BstKnob *knob = BST_KNOB (widget);

  /* chain parent class' handler */
  GTK_WIDGET_CLASS (parent_class)->size_request (widget, requisition);
}

static void
bst_knob_size_allocate (GtkWidget     *widget,
                        GtkAllocation *allocation)
{
  BstKnob *knob = BST_KNOB (widget);

  /* chain parent class' handler */
  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  /* position widget's window accordingly
   */
  if (GTK_WIDGET_REALIZED (widget))
    gdk_window_move_resize (knob->iwindow,
			    widget->allocation.x, widget->allocation.y,
			    widget->allocation.width, widget->allocation.height);
}

static void
bst_knob_realize (GtkWidget *widget)
{
  BstKnob *knob = BST_KNOB (widget);
  GdkWindowAttr attributes;
  gint attributes_mask;

  /* chain parent class' handler */
  GTK_WIDGET_CLASS (parent_class)->realize (widget);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= (GDK_BUTTON_PRESS_MASK |
			    GDK_BUTTON_RELEASE_MASK |
			    GDK_POINTER_MOTION_MASK |
			    GDK_POINTER_MOTION_HINT_MASK);
  attributes_mask = GDK_WA_X | GDK_WA_Y;

  knob->iwindow = gdk_window_new (gtk_widget_get_parent_window (widget),
				  &attributes, attributes_mask);
  gdk_window_set_user_data (knob->iwindow, knob);
}

static void
bst_knob_unrealize (GtkWidget *widget)
{
  BstKnob *knob = BST_KNOB (widget);

  gdk_window_set_user_data (knob->iwindow, NULL);
  gdk_window_destroy (knob->iwindow);
  knob->iwindow = NULL;

  /* chain parent class' handler */
  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
bst_knob_map (GtkWidget *widget)
{
  BstKnob *knob = BST_KNOB (widget);

  GTK_WIDGET_CLASS (parent_class)->map (widget);

  gdk_window_show (knob->iwindow);
}

static void
bst_knob_unmap (GtkWidget *widget)
{
  BstKnob *knob = BST_KNOB (widget);

  gdk_window_hide (knob->iwindow);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static gint
bst_knob_expose (GtkWidget      *widget,
		 GdkEventExpose *event)
{
  BstKnob *knob = BST_KNOB (widget);

  /* chain parent class' handler */
  GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);

  /* we ignore intermediate expose events
   */
  if (event->count == 0)
    bst_knob_paint (knob);

  return TRUE;
}

static void
bst_knob_paint (BstKnob *knob)
{
  GtkWidget *widget = GTK_WIDGET (knob);
  GdkGC *gc = widget->style->text_gc[GTK_STATE_PRELIGHT];
  gfloat xc, yc, s, c, angle, radius, dw;
  gint pointer_width = 3;
  gint line_width = 1;
  GdkPoint points[5];
  
  /* compute center */
  xc = widget->allocation.width / 2 + widget->allocation.x + knob->xofs;
  yc = widget->allocation.height / 2 + widget->allocation.y + knob->yofs;

  /* compute pointer coords */
  angle = knob->arc_start - knob->arc_dist * knob->angle_range;
  s = sin (angle);
  c = cos (angle);

  /* draw the pointer */
  radius = knob->furrow_radius;
  gdk_gc_set_line_attributes (gc, 2, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
  gdk_draw_line (widget->window,
		 widget->style->dark_gc[GTK_STATE_NORMAL],
		 xc + c * radius, yc - s * radius,
		 xc, yc);
  gdk_gc_set_line_attributes (gc, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
  gdk_draw_line (widget->window,
		 widget->style->light_gc[GTK_STATE_NORMAL],
		 xc + c * radius, yc - s * radius,
		 xc, yc);
  gdk_gc_set_line_attributes (gc, 0, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);

  radius = knob->dot_radius;
  dw = (knob->dot_radius - knob->furrow_radius) * 0.75;
  gdk_draw_arc (widget->window, gc,
		TRUE,
		xc + c * radius - dw, yc - s * radius - dw,
		2 * dw, 2 * dw,
		0.0 * 64, 360. * 64);
  

  if (0)
    {
      points[0].x = xc + s * pointer_width + 0.5;
      points[0].y = yc + c * pointer_width + 0.5;
      points[1].x = xc - s * pointer_width + 0.5;
      points[1].y = yc - c * pointer_width + 0.5;
      points[2].x = points[1].x + c * radius + 0.5;
      points[2].y = points[1].y - s * radius + 0.5;
      points[3].x = points[0].x + c * radius + 0.5;
      points[3].y = points[0].y - s * radius + 0.5;
      points[4].x = points[0].x;
      points[4].y = points[0].y;
      
      gdk_gc_set_line_attributes (gc, line_width, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
      gdk_draw_polygon (widget->window, gc, TRUE, points, 5);
      gdk_gc_set_line_attributes (gc, 0, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
    }
}

static gint
bst_knob_button_press (GtkWidget      *widget,
			GdkEventButton *event)
{
  BstKnob *knob = BST_KNOB (widget);

  if (!knob->button)
    {
      if (event->button == 1)
	{
	  knob->button = event->button;
	  bst_knob_mouse_update (knob, event->x, event->y, MOUSE_SETUP);
	}
      else if (event->button == 2)
	{
	  knob->button = event->button;
	  bst_knob_mouse_update (knob, event->x, event->y, MOUSE_JUMP);
	}
    }

  return TRUE;
}

static gint
bst_knob_motion_notify (GtkWidget      *widget,
			 GdkEventMotion *event)
{
  BstKnob *knob = BST_KNOB (widget);
  
  if (knob->button != 0 && event->window == knob->iwindow)
    {
      if (event->is_hint)
        gdk_window_get_pointer (widget->window, NULL, NULL, NULL);
      
      bst_knob_mouse_update (knob, event->x, event->y, knob->button == 1 ? MOUSE_MOVE : MOUSE_JUMP);
    }
  
  return TRUE;
}

static gint
bst_knob_button_release (GtkWidget      *widget,
			  GdkEventButton *event)
{
  BstKnob *knob = BST_KNOB (widget);
  
  if (knob->button == event->button)
    {
      GtkAdjustment *adjustment = GTK_ADJUSTMENT (knob->adjustment);

      bst_knob_mouse_update (knob, event->x, event->y, knob->button == 1 ? MOUSE_MOVE : MOUSE_JUMP);

      knob->button = 0;
      
      if (knob->timer)
	{
	  gtk_timeout_remove (knob->timer);
	  knob->timer = 0;
	}
      
      if (knob->old_value != adjustment->value)
        gtk_adjustment_value_changed (GTK_ADJUSTMENT (knob->adjustment));
    }
  
  return TRUE;
}

static double   /* expresses arc2 relative to arc1 */
arc_diff (double arc1,
	  double arc2)
{
  const double _2pi = M_PI * 2.0;
  double diff;

  diff = arc2 - arc1;

  if (diff > _2pi)
    {
      double f = floor (diff / _2pi);
      diff -= f * _2pi;
    }
  else if (diff < 0)
    {
      double f = floor (diff / -_2pi + 1);
      diff += f * _2pi;
    }
  if (diff > M_PI)
    diff -= _2pi;
  else if (diff < -M_PI)
    diff += _2pi;

  return diff;
}

static void
bst_knob_mouse_update (BstKnob *knob,
		       gint     x,
		       gint     y,
		       guint    mode)
{
  GtkAdjustment *adjustment = GTK_ADJUSTMENT (knob->adjustment);
  GtkWidget *widget = GTK_WIDGET (knob);
  gdouble angle, diff, dist, sensitivity = 0.001;
  gint xc, yc;
  
  /* figure the arc's center */
  xc = widget->allocation.width / 2;
  yc = widget->allocation.height / 2;
  
  /* calculate the angle of the pointer */
  angle = atan2 (yc - y, x - xc);
  
  switch (mode)
    {
      gdouble sdiff, ediff;
    case MOUSE_SETUP:
      knob->pangle = angle;
      knob->px = x;
      knob->py = y;
      return;
    case MOUSE_MOVE:
      dist = sqrt (SQR (x - knob->px) + SQR (y - knob->py));
      diff = arc_diff (angle, knob->pangle);
      if (diff > 0)
	dist = -dist;
      knob->pangle = angle;
      knob->px = x;
      knob->py = y;
      knob->angle_range -= dist * sensitivity;	/* turn direction sign */
      break;
    case MOUSE_JUMP:
      sdiff = arc_diff (knob->arc_start, angle);
      ediff = arc_diff (knob->arc_start - knob->arc_dist, angle); /* turn direction sign */
      if (sdiff > 0 && ediff < 0)
	angle = knob->arc_start - (fabs (sdiff) < fabs (ediff) ? 0 : knob->arc_dist); /* turn direction sign */
      angle = -arc_diff (knob->arc_start, angle);
      if (angle < 0)
	angle += 2 * M_PI;
      knob->angle_range = angle / knob->arc_dist;
      break;
    }
  
  /* constrained to the valid area of the arc */
  knob->angle_range = CLAMP (knob->angle_range, 0, 1.0);
  
  /* compute new adjustment value, translated to its lower...upper range */
  adjustment->value = adjustment->lower + knob->angle_range * (adjustment->upper - adjustment->page_size - adjustment->lower);
  
  /* if the adjustment value changed:
   * - for continuous updates: emit the GtkAdjustment::value_changed signal
   * - for delayed updates: install a timer to emit the changed signal, if
   *                        a timer has already been installed, reinstall a
   *                        new timer, so multiple updates get coalesced
   */
  if (adjustment->value != knob->old_value)
    {
      if (knob->update_policy == GTK_UPDATE_CONTINUOUS)
	gtk_adjustment_value_changed (GTK_ADJUSTMENT (knob->adjustment));
      else
        {
          if (knob->update_policy == GTK_UPDATE_DELAYED)
            {
	      /* restart timer, so the delay interval starts from scratch */
              if (knob->timer)
                gtk_timeout_remove (knob->timer);
              
              knob->timer = gtk_timeout_add (SCROLL_DELAY_LENGTH,
                                             bst_knob_timer,
                                             knob);
            }
	  
	  /* immediately update the widget, so the GUI is responsive and
	   * not delayed like the ::value_changed signal
	   */
          gtk_widget_queue_draw (widget);
        }
    }
}

static gboolean
bst_knob_timer (gpointer data)
{
  BstKnob *knob;
  
  GDK_THREADS_ENTER ();
  
  knob = BST_KNOB (data);
  
  gtk_adjustment_value_changed (GTK_ADJUSTMENT (knob->adjustment));
  
  knob->timer = 0;
  
  GDK_THREADS_LEAVE ();
  
  return FALSE;
}

GtkWidget*
bst_knob_new (GtkAdjustment *adjustment)
{
  GtkWidget *knob;
  
  if (adjustment)
    g_return_val_if_fail (GTK_IS_ADJUSTMENT (adjustment), NULL);
  else
    adjustment = (GtkAdjustment*) gtk_adjustment_new (0.0, 0.0, 250.0, 0.0, 0.0, 0.0);
  
  knob = gtk_widget_new (BST_TYPE_KNOB, NULL);
  
  bst_knob_set_adjustment (BST_KNOB (knob), adjustment);
  
  return knob;
}

GtkAdjustment*
bst_knob_get_adjustment (BstKnob *knob)
{
  g_return_val_if_fail (BST_IS_KNOB (knob), NULL);
  
  return GTK_ADJUSTMENT (knob->adjustment);
}

void
bst_knob_set_update_policy (BstKnob      *knob,
			     GtkUpdateType policy)
{
  g_return_if_fail (BST_IS_KNOB (knob));
  
  if (knob->update_policy != policy)
    {
      knob->update_policy = policy;
      
      /* remove a pending timer if necessary */
      if (knob->timer)
	{
	  gtk_timeout_remove (knob->timer);
	  knob->timer = 0;

	  /* perform pending notification */
	  if (policy == GTK_UPDATE_CONTINUOUS &&
	      knob->old_value != GTK_ADJUSTMENT (knob->adjustment)->value)
	    gtk_adjustment_value_changed (GTK_ADJUSTMENT (knob->adjustment));
	}
    }
}

void
bst_knob_set_adjustment (BstKnob      *knob,
			  GtkAdjustment *adjustment)
{
  g_return_if_fail (BST_IS_KNOB (knob));
  g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));
  
  if (knob->adjustment)
    {
      gtk_signal_disconnect_by_data (knob->adjustment, knob);
      gtk_object_unref (knob->adjustment);
    }
  
  knob->adjustment = GTK_OBJECT (adjustment);
  
  gtk_object_ref (knob->adjustment);
  gtk_object_sink (knob->adjustment);
  
  gtk_signal_connect (knob->adjustment,
                      "changed",
                      GTK_SIGNAL_FUNC (bst_knob_adjustment_changed),
                      knob);
  gtk_signal_connect (knob->adjustment,
                      "value_changed",
                      GTK_SIGNAL_FUNC (bst_knob_adjustment_value_changed),
                      knob);
  
  knob->old_value = adjustment->value;
  knob->old_lower = adjustment->lower;
  knob->old_upper = adjustment->upper;
  knob->old_page_size = adjustment->page_size;
  
  bst_knob_update (knob);
}

static void
bst_knob_adjustment_changed (GtkAdjustment *adjustment,
			      gpointer       data)
{
  BstKnob *knob;
  
  g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));
  g_return_if_fail (data != NULL);
  
  knob = BST_KNOB (data);
  
  if (knob->old_value != adjustment->value ||
      knob->old_lower != adjustment->lower ||
      knob->old_upper != adjustment->upper ||
      knob->old_page_size != adjustment->page_size)
    {
      knob->old_value = adjustment->value;
      knob->old_lower = adjustment->lower;
      knob->old_upper = adjustment->upper;
      knob->old_page_size = adjustment->page_size;

      bst_knob_update (knob);
    }
}

static void
bst_knob_adjustment_value_changed (GtkAdjustment *adjustment,
				    gpointer       data)
{
  BstKnob *knob;
  
  g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));
  g_return_if_fail (data != NULL);
  
  knob = BST_KNOB (data);
  
  if (knob->old_value != adjustment->value)
    {
      knob->old_value = adjustment->value;

      bst_knob_update (knob);
    }
}

static void
bst_knob_update (BstKnob *knob)
{
  GtkAdjustment *adjustment;
  GtkWidget *widget;
  gdouble new_value;
  
  g_return_if_fail (BST_IS_KNOB (knob));
  
  widget = GTK_WIDGET (knob);
  adjustment = GTK_ADJUSTMENT (knob->adjustment);
  
  new_value = CLAMP (adjustment->value, adjustment->lower, adjustment->upper - adjustment->page_size);

  if (new_value != adjustment->value)
    {
      if (0)
	g_print ("knob-adjustment: %f <= %f <= %f, adjust: %f\n",
		 adjustment->lower, adjustment->value, adjustment->upper, new_value);
      adjustment->value = new_value;
      gtk_adjustment_value_changed (GTK_ADJUSTMENT (knob->adjustment));
    }
  
  knob->angle_range = ((adjustment->value - adjustment->lower) /
		       (adjustment->upper - adjustment->page_size - adjustment->lower));
  
  gtk_widget_queue_draw (widget);
}
