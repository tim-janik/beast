/* BEAST - Bedevilled Audio System
 * Copyright (C) 1999-2002 Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bstdial.h"

#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <math.h>
#include <stdio.h>

#define SCROLL_DELAY_LENGTH	300
#define DIAL_DEFAULT_SIZE	30
#define RATIO			0.75752
#define HCENTER			(1.0 / (2 * RATIO))

static void     bst_dial_class_init                 (BstDialClass   *class);
static void     bst_dial_init                       (BstDial        *dial);
static void     bst_dial_destroy                    (GtkObject      *object);
static void     bst_dial_realize                    (GtkWidget      *widget);
static void     bst_dial_size_request               (GtkWidget      *widget,
                                                     GtkRequisition *requisition);
static void     bst_dial_size_allocate              (GtkWidget      *widget,
                                                     GtkAllocation  *allocation);
static gint     bst_dial_expose                     (GtkWidget      *widget,
                                                     GdkEventExpose *event);
static void	bst_dial_paint			    (BstDial	    *dial);
static gint     bst_dial_button_press               (GtkWidget      *widget,
                                                     GdkEventButton *event);
static gint     bst_dial_button_release             (GtkWidget      *widget,
                                                     GdkEventButton *event);
static gint     bst_dial_motion_notify              (GtkWidget      *widget,
                                                     GdkEventMotion *event);
static gboolean bst_dial_timer                      (gpointer        data);
static void     bst_dial_mouse_update               (BstDial        *dial,
                                                     gint            x,
                                                     gint            y);
static void     bst_dial_update                     (BstDial        *dial);
static void     bst_dial_adjustment_changed         (GtkAdjustment  *adjustment,
                                                     gpointer        data);
static void     bst_dial_adjustment_value_changed   (GtkAdjustment  *adjustment,
                                                     gpointer        data);

static GtkWidgetClass *parent_class = NULL;

GtkType
bst_dial_get_type (void)
{
  static GtkType dial_type = 0;
  
  if (!dial_type)
    {
      GtkTypeInfo dial_info =
      {
        "BstDial",
        sizeof (BstDial),
        sizeof (BstDialClass),
        (GtkClassInitFunc) bst_dial_class_init,
        (GtkObjectInitFunc) bst_dial_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };
      
      dial_type = gtk_type_unique (GTK_TYPE_WIDGET, &dial_info);
    }
  
  return dial_type;
}

static void
bst_dial_class_init (BstDialClass *class)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  parent_class = g_type_class_peek_parent (class);
  
  object_class->destroy = bst_dial_destroy;
  
  widget_class->size_request = bst_dial_size_request;
  widget_class->size_allocate = bst_dial_size_allocate;
  widget_class->realize = bst_dial_realize;
  widget_class->expose_event = bst_dial_expose;
  widget_class->button_press_event = bst_dial_button_press;
  widget_class->button_release_event = bst_dial_button_release;
  widget_class->motion_notify_event = bst_dial_motion_notify;
}

static void
bst_dial_init (BstDial *dial)
{
  dial->update_policy = GTK_UPDATE_CONTINUOUS;
  dial->button = 0;
  dial->radius = 0;
  dial->pointer_width = 0;
  dial->timer = 0;
  dial->angle = 0.0;
  dial->old_value = 0.0;
  dial->old_lower = 0.0;
  dial->old_upper = 0.0;
  dial->adjustment = NULL;
}

static void
bst_dial_destroy (GtkObject *object)
{
  BstDial *dial;
  
  g_return_if_fail (BST_IS_DIAL (object));
  
  dial = BST_DIAL (object);

  bst_dial_set_align_widget (dial, 0, 0, 0);

  if (dial->timer)
    {
      gtk_timeout_remove (dial->timer);
      dial->timer = 0;
    }
  
  if (dial->adjustment)
    {
      gtk_signal_disconnect_by_data (dial->adjustment, dial);
      gtk_object_unref (dial->adjustment);
      dial->adjustment = NULL;
    }
  
  /* chain parent class' handler */
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bst_dial_size_request (GtkWidget      *widget,
                       GtkRequisition *requisition)
{
  BstDial *dial = BST_DIAL (widget);

  if (dial->align_widget)
    {
      gtk_widget_size_request (dial->align_widget, requisition);
      if (dial->align_width)
	{
	  requisition->width = requisition->width;
	  requisition->height = requisition->width * RATIO + 0.5;
	}
      else
	{
	  requisition->height = requisition->height;
	  requisition->width = requisition->height / RATIO + 0.5;
	}
    }
  else
    {
      requisition->width = DIAL_DEFAULT_SIZE;
      requisition->height = DIAL_DEFAULT_SIZE * RATIO + 0.5;
    }
}

static void
bst_dial_size_allocate (GtkWidget     *widget,
                        GtkAllocation *allocation)
{
  BstDial *dial = BST_DIAL (widget);

  /* center widget within given allocation
   */
  widget->allocation.width = MIN (allocation->width, allocation->height / RATIO);
  widget->allocation.height = widget->allocation.width * RATIO;
  widget->allocation.x = allocation->x + (allocation->width - widget->allocation.width) / 2;
  widget->allocation.y = allocation->y + (allocation->height - widget->allocation.height) / 2;

  /* determine dial radius and pointer width from allocation
   */
  dial->radius = MAX (widget->allocation.width, 2) / 2;
  dial->pointer_width = dial->radius * 0.25;

  /* position widget's window accordingly
   */
  if (GTK_WIDGET_REALIZED (widget))
    gdk_window_move_resize (widget->window,
			    widget->allocation.x, widget->allocation.y,
			    widget->allocation.width, widget->allocation.height);
}

static void
bst_dial_realize (GtkWidget *widget)
{
  // BstDial *dial = BST_DIAL (widget);
  GdkWindowAttr attributes;
  gint attributes_mask;
  
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
  
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = (gtk_widget_get_events (widget) |
			   GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK |
			   GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK |
			   GDK_POINTER_MOTION_HINT_MASK);
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  widget->window = gdk_window_new (widget->parent->window, &attributes, attributes_mask);
  gdk_window_set_user_data (widget->window, widget);
  
  widget->style = gtk_style_attach (widget->style, widget->window);
  
  gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
}

static gint
bst_dial_expose (GtkWidget      *widget,
                 GdkEventExpose *event)
{
  g_return_val_if_fail (BST_IS_DIAL (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  /* since we redraw the whole widget from scratch, we ignore
   * intermediate expose events
   */
  if (event->count == 0)
    bst_dial_paint (BST_DIAL (widget));

  return TRUE;
}

static void
bst_dial_paint (BstDial *dial)
{
  GtkWidget *widget = GTK_WIDGET (dial);
  GdkPoint points[4];
  gdouble s, c;
  gdouble theta;
  gint xc, yc;
  gint tick_length, n_steps, thick_step;
  guint i, pointer_width, radius;

  xc = widget->allocation.width / 2;
  yc = (widget->allocation.height - dial->radius) / 2 + dial->radius;
  pointer_width = dial->pointer_width;
  radius = dial->radius;

  /* clear paintable area
   */
  /* fill upper part */
  gdk_draw_arc (widget->window,
		widget->style->bg_gc[GTK_WIDGET_IS_SENSITIVE (dial)
				    ? GTK_STATE_ACTIVE
				    : GTK_STATE_INSENSITIVE],
		TRUE,
                xc - radius, yc - radius,
		2 * radius, 2 * radius,
		0. * 64, 180. * 64);
  /* draw light shade on the left */
  gdk_draw_arc (widget->window,
		widget->style->light_gc[widget->state],
		FALSE,
                xc - radius, yc - radius,
		2 * radius, 2 * radius,
		60. * 64, 110. * 64);
  /* draw shadow on the right */
  gdk_draw_arc (widget->window,
		widget->style->dark_gc[widget->state],
		FALSE,
		xc - radius, yc - radius,
		2 * radius, 2 * radius,
		0.0 * 64, 30. * 64);
  radius += 1;
  /* draw bottom shadow left */
  gdk_draw_line (widget->window,
		 widget->style->dark_gc[widget->state],
		 xc - radius, yc,
		 xc + radius, yc);
  radius -= 1;

  /* draw the ticks
   */
  if (widget->allocation.width >= DIAL_DEFAULT_SIZE * 2)
    {
      n_steps = 36;
      thick_step = 6;
    }
  else
    {
      n_steps = 8;
      thick_step = 2;
    }
  for (i = 0; i < n_steps + 1; i++)
    {
      theta = M_PI - (i * M_PI / ((double) n_steps));
      s = sin (theta);
      c = cos (theta);
      
      /* draw every nth tick with doubled length */
      tick_length = (i % thick_step == 0) ? pointer_width : pointer_width / 2;
      
      /* draw the ticks as a polygon to get proper shading */
      points[0].x = xc + c * (radius - tick_length);
      points[0].y = yc - s * (radius - tick_length);
      points[1].x = xc + c * radius;
      points[1].y = yc - s * radius;
      if (widget->allocation.width >= DIAL_DEFAULT_SIZE)
	gtk_draw_polygon (widget->style,
			  widget->window,
			  widget->state,
			  GTK_SHADOW_ETCHED_IN,
			  points, 2,
			  FALSE);
      else
	gdk_draw_polygon (widget->window, widget->style->black_gc, FALSE, points, 2);
    }
  
  /* draw the pointer
   */
  s = sin (dial->angle);
  c = cos (dial->angle);
  if (widget->allocation.width >= DIAL_DEFAULT_SIZE)
    radius -= 1;
  points[0].x = xc + s * pointer_width + 0.5;
  points[0].y = yc + c * pointer_width + 0.5;
  points[1].x = xc + c * radius + 0.5;
  points[1].y = yc - s * radius + 0.5;
  points[2].x = xc - s * pointer_width + 0.5;
  points[2].y = yc - c * pointer_width + 0.5;
  points[3].x = points[0].x;
  points[3].y = points[0].y;
  if (widget->allocation.width >= DIAL_DEFAULT_SIZE)
    gtk_draw_polygon (widget->style,
		      widget->window,
		      widget->state,
		      GTK_SHADOW_OUT,
		      points, 4,
		      TRUE);
  else
    {
      gdk_draw_polygon (widget->window, widget->style->black_gc, TRUE, points, 4);
      gdk_draw_polygon (widget->window, widget->style->black_gc, FALSE, points, 4);
    }
  /*gdk_draw_line (widget->window, widget->style->black_gc,
    xc, yc, xc + c * radius, yc - s * radius);*/
}

static gint
bst_dial_button_press (GtkWidget      *widget,
                       GdkEventButton *event)
{
  BstDial *dial = BST_DIAL (widget);

  if (!dial->button)
    {
      dial->button = event->button;
      
      bst_dial_mouse_update (dial, event->x, event->y);
    }

  return TRUE;
}

static gint
bst_dial_motion_notify (GtkWidget      *widget,
                        GdkEventMotion *event)
{
  BstDial *dial = BST_DIAL (widget);
  
  if (dial->button != 0 && event->window == widget->window)
    {
      if (event->is_hint)
        gdk_window_get_pointer (widget->window, NULL, NULL, NULL);
      
      bst_dial_mouse_update (dial, event->x, event->y);
    }
  
  return TRUE;
}

static gint
bst_dial_button_release (GtkWidget      *widget,
                         GdkEventButton *event)
{
  BstDial *dial = BST_DIAL (widget);
  
  if (dial->button == event->button)
    {
      GtkAdjustment *adjustment = GTK_ADJUSTMENT (dial->adjustment);

      bst_dial_mouse_update (dial, event->x, event->y);

      dial->button = 0;
      
      if (dial->timer)
	{
	  gtk_timeout_remove (dial->timer);
	  dial->timer = 0;
	}
      
      if (dial->old_value != adjustment->value)
        gtk_adjustment_value_changed (GTK_ADJUSTMENT (dial->adjustment));
    }
  
  return TRUE;
}

static void
bst_dial_mouse_update (BstDial *dial,
                       gint     x,
                       gint     y)
{
  GtkAdjustment *adjustment;
  GtkWidget *widget;
  gint xc, yc;
  gdouble angle;
  
  g_return_if_fail (BST_IS_DIAL (dial));
  
  widget = GTK_WIDGET (dial);
  adjustment = GTK_ADJUSTMENT (dial->adjustment);

  /* figure the arc's center */
  xc = widget->allocation.width / 2;
  yc = (widget->allocation.height - dial->radius) / 2 + dial->radius;

  /* calculate the angle of the button click, constrained to the
   * viewable area of the arc
   */
  angle = atan2 (yc - y, x - xc);
  if (angle < M_PI * -0.5)
    angle = M_PI;
  if (angle <= 0)
    angle = 0;
  if (angle > M_PI)
    angle = M_PI;
  dial->angle = angle;

  /* compute new adjustment value, translated to its lower...upper range */
  adjustment->value = (adjustment->lower +
		       (1.0 - angle / M_PI) * (adjustment->upper - adjustment->lower - adjustment->page_size));
  
  /* if the adjustment value changed:
   * - for continuous updates: emit the GtkAdjustment::value_changed signal
   * - for delayed updates: install a timer to emit the changed signal, if
   *                        a timer has already been installed, reinstall a
   *                        new timer, so multiple updates get coalesced
   */
  if (adjustment->value != dial->old_value)
    {
      if (dial->update_policy == GTK_UPDATE_CONTINUOUS)
	gtk_adjustment_value_changed (GTK_ADJUSTMENT (dial->adjustment));
      else
        {
          if (dial->update_policy == GTK_UPDATE_DELAYED)
            {
	      /* restart timer, so the delay interval starts from scratch */
              if (dial->timer)
                gtk_timeout_remove (dial->timer);
              
              dial->timer = gtk_timeout_add (SCROLL_DELAY_LENGTH,
                                             bst_dial_timer,
                                             dial);
            }

	  /* immediately update the widget, so the GUI is responsive and
	   * not delayed like the ::value_changed signal
	   */
          gtk_widget_queue_draw (widget);
        }
    }
}

static gboolean
bst_dial_timer (gpointer data)
{
  BstDial *dial;
  
  GDK_THREADS_ENTER ();

  dial = BST_DIAL (data);

  gtk_adjustment_value_changed (GTK_ADJUSTMENT (dial->adjustment));

  dial->timer = 0;
  
  GDK_THREADS_LEAVE ();

  return FALSE;
}

GtkWidget*
bst_dial_new (GtkAdjustment *adjustment)
{
  GtkWidget *dial;
  
  if (adjustment)
    g_return_val_if_fail (GTK_IS_ADJUSTMENT (adjustment), NULL);
  else
    adjustment = (GtkAdjustment*) gtk_adjustment_new (0.0, 0.0, 250.0, 0.0, 0.0, 0.0);
  
  dial = gtk_widget_new (BST_TYPE_DIAL, NULL);
  
  bst_dial_set_adjustment (BST_DIAL (dial), adjustment);
  
  return dial;
}

GtkAdjustment*
bst_dial_get_adjustment (BstDial *dial)
{
  g_return_val_if_fail (BST_IS_DIAL (dial), NULL);
  
  return GTK_ADJUSTMENT (dial->adjustment);
}

void
bst_dial_set_update_policy (BstDial      *dial,
                            GtkUpdateType policy)
{
  g_return_if_fail (BST_IS_DIAL (dial));
  
  if (dial->update_policy != policy)
    {
      dial->update_policy = policy;
      
      /* remove a pending timer if necessary */
      if (dial->timer)
	{
	  gtk_timeout_remove (dial->timer);
	  dial->timer = 0;

	  /* perform pending notification */
	  if (policy == GTK_UPDATE_CONTINUOUS &&
	      dial->old_value != GTK_ADJUSTMENT (dial->adjustment)->value)
	    gtk_adjustment_value_changed (GTK_ADJUSTMENT (dial->adjustment));
	}
    }
}

void
bst_dial_set_adjustment (BstDial       *dial,
                         GtkAdjustment *adjustment)
{
  g_return_if_fail (BST_IS_DIAL (dial));
  g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));
  
  if (dial->adjustment)
    {
      gtk_signal_disconnect_by_data (dial->adjustment, dial);
      gtk_object_unref (dial->adjustment);
    }
  
  dial->adjustment = GTK_OBJECT (adjustment);
  
  gtk_object_ref (dial->adjustment);
  gtk_object_sink (dial->adjustment);
  
  gtk_signal_connect (dial->adjustment,
                      "changed",
                      GTK_SIGNAL_FUNC (bst_dial_adjustment_changed),
                      dial);
  gtk_signal_connect (dial->adjustment,
                      "value_changed",
                      GTK_SIGNAL_FUNC (bst_dial_adjustment_value_changed),
                      dial);
  
  dial->old_value = adjustment->value;
  dial->old_lower = adjustment->lower;
  dial->old_upper = adjustment->upper;
  dial->old_page_size = adjustment->page_size;
  
  bst_dial_update (dial);
}

static void
bst_dial_adjustment_changed (GtkAdjustment *adjustment,
                             gpointer       data)
{
  BstDial *dial;
  
  g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));
  g_return_if_fail (data != NULL);
  
  dial = BST_DIAL (data);
  
  if (dial->old_value != adjustment->value ||
      dial->old_lower != adjustment->lower ||
      dial->old_upper != adjustment->upper ||
      dial->old_page_size != adjustment->page_size)
    {
      dial->old_value = adjustment->value;
      dial->old_lower = adjustment->lower;
      dial->old_upper = adjustment->upper;
      dial->old_page_size = adjustment->page_size;

      bst_dial_update (dial);
    }
}

static void
bst_dial_adjustment_value_changed (GtkAdjustment *adjustment,
                                   gpointer       data)
{
  BstDial *dial;
  
  g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));
  g_return_if_fail (data != NULL);
  
  dial = BST_DIAL (data);
  
  if (dial->old_value != adjustment->value)
    {
      dial->old_value = adjustment->value;

      bst_dial_update (dial);
    }
}

static void
bst_dial_update (BstDial *dial)
{
  GtkAdjustment *adjustment;
  GtkWidget *widget;
  gdouble new_value;
  
  g_return_if_fail (BST_IS_DIAL (dial));
  
  widget = GTK_WIDGET (dial);
  adjustment = GTK_ADJUSTMENT (dial->adjustment);
  
  new_value = CLAMP (adjustment->value, adjustment->lower, adjustment->upper - adjustment->page_size);

  if (new_value != adjustment->value)
    {
      if (0)
	g_print ("dial-adjustment: %f <= %f <= %f, adjust: %f\n",
		 adjustment->lower, adjustment->value, adjustment->upper, new_value);
      adjustment->value = new_value;
      gtk_adjustment_value_changed (GTK_ADJUSTMENT (dial->adjustment));
    }
  
  dial->angle = (M_PI - M_PI * (adjustment->value - adjustment->lower) /
		 (adjustment->upper - adjustment->page_size - adjustment->lower));
  
  gtk_widget_queue_draw (widget);
}

void
bst_dial_set_align_widget (BstDial   *dial,
			   GtkWidget *widget,
			   gboolean   width_align,
			   gboolean   height_align)
{
  g_return_if_fail (BST_IS_DIAL (dial));
  if (widget)
    {
      g_return_if_fail (GTK_IS_WIDGET (widget));
      g_return_if_fail (widget != GTK_WIDGET (dial));
      width_align = width_align != FALSE;
      height_align = height_align != FALSE;
      g_return_if_fail (width_align ^ height_align);
    }

  if (dial->align_widget)
    g_object_unref (dial->align_widget);
  dial->align_widget = widget;
  if (dial->align_widget)
    {
      g_object_ref (dial->align_widget);
      dial->align_width = width_align;
    }
}
