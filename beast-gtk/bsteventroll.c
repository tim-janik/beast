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
#include	"bsteventroll.h"

#include	"bstasciipixbuf.h"
#include	<gdk/gdkkeysyms.h>


/* --- defines --- */
/* helpers */
#define	STYLE(self)		(GTK_WIDGET (self)->style)
#define STATE(self)             (GTK_WIDGET (self)->state)
#define XTHICKNESS(self)        (STYLE (self)->xthickness)
#define YTHICKNESS(self)        (STYLE (self)->ythickness)
#define	ALLOCATION(self)	(&GTK_WIDGET (self)->allocation)

/* layout (requisition) */
#define	VPANEL_WIDTH(self)	((self)->vpanel_width)
#define	VPANEL_X(self)		(0)
#define	CANVAS_X(self)		(VPANEL_X (self) + VPANEL_WIDTH (self))
#define	CANVAS_Y(self)		(0)

/* layout (allocation) */
#define	CANVAS_WIDTH(self)	(ALLOCATION (self)->width - CANVAS_X (self))
#define	CANVAS_HEIGHT(self)	(ALLOCATION (self)->height - CANVAS_Y (self))

/* aliases */
#define	VPANEL_HEIGHT(self)	(CANVAS_HEIGHT (self))
#define	VPANEL_Y(self)		(CANVAS_Y (self))

/* appearance */
#define VPANEL_BG_COLOR(self)   (&STYLE (self)->bg[GTK_WIDGET_IS_SENSITIVE (self) ? GTK_STATE_NORMAL : GTK_STATE_INSENSITIVE])
#define CANVAS_BG_COLOR(self)   (&STYLE (self)->base[GTK_WIDGET_STATE (self)])
#define	QNOTE_HPIXELS		(30)	/* guideline */

/* behaviour */
#define AUTO_SCROLL_TIMEOUT	(33)
#define	AUTO_SCROLL_SCALE	(0.2)


/* --- prototypes --- */
static void     bst_event_roll_class_init             (BstEventRollClass *class);
static void     bst_event_roll_init                   (BstEventRoll      *self);
static void     bst_event_roll_dispose                (GObject           *object);
static void     bst_event_roll_destroy                (GtkObject         *object);
static void     bst_event_roll_finalize               (GObject           *object);
static void     bst_event_roll_set_scroll_adjustments (BstEventRoll      *self,
                                                       GtkAdjustment     *hadjustment,
                                                       GtkAdjustment     *vadjustment);
static void     bst_event_roll_size_request           (GtkWidget         *widget,
                                                       GtkRequisition    *requisition);
static void     bst_event_roll_size_allocate          (GtkWidget         *widget,
                                                       GtkAllocation     *allocation);
static void     bst_event_roll_style_set              (GtkWidget         *widget,
                                                       GtkStyle          *previous_style);
static void     bst_event_roll_state_changed          (GtkWidget         *widget,
                                                       guint              previous_state);
static void     bst_event_roll_add                    (GtkContainer      *container,
                                                       GtkWidget         *child);
static void     bst_event_roll_remove                 (GtkContainer      *container,
                                                       GtkWidget         *child);
static void     bst_event_roll_forall                 (GtkContainer      *container,
                                                       gboolean           include_internals,
                                                       GtkCallback        callback,
                                                       gpointer           callback_data);
static void     bst_event_roll_realize                (GtkWidget         *widget);
static void     bst_event_roll_unrealize              (GtkWidget         *widget);
static gboolean bst_event_roll_focus_in               (GtkWidget         *widget,
                                                       GdkEventFocus     *event);
static gboolean bst_event_roll_focus_out              (GtkWidget         *widget,
                                                       GdkEventFocus     *event);
static gboolean bst_event_roll_expose                 (GtkWidget         *widget,
                                                       GdkEventExpose    *event);
static gboolean bst_event_roll_key_press              (GtkWidget         *widget,
                                                       GdkEventKey       *event);
static gboolean bst_event_roll_key_release            (GtkWidget         *widget,
                                                       GdkEventKey       *event);
static gboolean bst_event_roll_button_press           (GtkWidget         *widget,
                                                       GdkEventButton    *event);
static gboolean bst_event_roll_motion                 (GtkWidget         *widget,
                                                       GdkEventMotion    *event);
static gboolean bst_event_roll_button_release         (GtkWidget         *widget,
                                                       GdkEventButton    *event);
static void     event_roll_update_adjustments         (BstEventRoll      *self,
                                                       gboolean           hadj,
                                                       gboolean           vadj);
static void     event_roll_scroll_adjustments         (BstEventRoll      *self,
                                                       gint               x_pixel,
                                                       gint               y_pixel);
static void     event_roll_adjustment_changed         (BstEventRoll      *self);
static void     event_roll_adjustment_value_changed   (BstEventRoll      *self,
                                                       GtkAdjustment     *adjustment);
static void     bst_event_roll_hsetup                 (BstEventRoll      *self,
                                                       guint              ppqn,
                                                       guint              qnpt,
                                                       guint              max_ticks,
                                                       gfloat             hzoom);

/* --- static variables --- */
static gpointer	parent_class = NULL;
static guint	signal_canvas_drag = 0;
static guint	signal_canvas_clicked = 0;
static guint	signal_vpanel_drag = 0;
static guint	signal_vpanel_clicked = 0;


/* --- functions --- */
GType
bst_event_roll_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info = {
	sizeof (BstEventRollClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) bst_event_roll_class_init,
	NULL,   /* class_finalize */
	NULL,   /* class_data */
	sizeof (BstEventRoll),
	0,      /* n_preallocs */
	(GInstanceInitFunc) bst_event_roll_init,
      };

      type = g_type_register_static (GTK_TYPE_CONTAINER,
				     "BstEventRoll",
				     &type_info, 0);
    }

  return type;
}

static void
bst_event_roll_class_init (BstEventRollClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  gobject_class->dispose = bst_event_roll_dispose;
  gobject_class->finalize = bst_event_roll_finalize;

  object_class->destroy = bst_event_roll_destroy;
  
  widget_class->size_request = bst_event_roll_size_request;
  widget_class->size_allocate = bst_event_roll_size_allocate;
  widget_class->realize = bst_event_roll_realize;
  widget_class->unrealize = bst_event_roll_unrealize;
  widget_class->style_set = bst_event_roll_style_set;
  widget_class->state_changed = bst_event_roll_state_changed;
  widget_class->expose_event = bst_event_roll_expose;
  widget_class->focus_in_event = bst_event_roll_focus_in;
  widget_class->focus_out_event = bst_event_roll_focus_out;
  widget_class->key_press_event = bst_event_roll_key_press;
  widget_class->key_release_event = bst_event_roll_key_release;
  widget_class->button_press_event = bst_event_roll_button_press;
  widget_class->motion_notify_event = bst_event_roll_motion;
  widget_class->button_release_event = bst_event_roll_button_release;

  container_class->add = bst_event_roll_add;
  container_class->remove = bst_event_roll_remove;
  container_class->forall = bst_event_roll_forall;

  class->set_scroll_adjustments = bst_event_roll_set_scroll_adjustments;
  class->canvas_clicked = NULL;
  
  signal_canvas_drag = g_signal_new ("canvas-drag", G_OBJECT_CLASS_TYPE (class),
				     G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstEventRollClass, canvas_drag),
				     NULL, NULL,
				     bst_marshal_NONE__POINTER,
				     G_TYPE_NONE, 1, G_TYPE_POINTER);
  signal_canvas_clicked = g_signal_new ("canvas-clicked", G_OBJECT_CLASS_TYPE (class),
					G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstEventRollClass, canvas_clicked),
					NULL, NULL,
					bst_marshal_NONE__UINT_UINT_INT_BOXED,
					G_TYPE_NONE, 4, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_FLOAT,
					GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  signal_vpanel_drag = g_signal_new ("vpanel-drag", G_OBJECT_CLASS_TYPE (class),
                                     G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstEventRollClass, vpanel_drag),
                                     NULL, NULL,
                                     bst_marshal_NONE__POINTER,
                                     G_TYPE_NONE, 1, G_TYPE_POINTER);
  signal_vpanel_clicked = g_signal_new ("vpanel-clicked", G_OBJECT_CLASS_TYPE (class),
                                        G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstEventRollClass, vpanel_clicked),
                                        NULL, NULL,
                                        bst_marshal_NONE__UINT_INT_BOXED,
                                        G_TYPE_NONE, 3, G_TYPE_UINT, G_TYPE_FLOAT,
                                        GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  widget_class->set_scroll_adjustments_signal =
    gtk_signal_new ("set_scroll_adjustments",
		    GTK_RUN_LAST,
		    GTK_CLASS_TYPE (object_class),
		    GTK_SIGNAL_OFFSET (BstEventRollClass, set_scroll_adjustments),
		    bst_marshal_NONE__OBJECT_OBJECT,
		    GTK_TYPE_NONE, 2, GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);
}

static void
bst_event_roll_init (BstEventRoll *self)
{
  GtkWidget *widget = GTK_WIDGET (self);

  GTK_WIDGET_UNSET_FLAGS (self, GTK_NO_WINDOW);
  GTK_WIDGET_SET_FLAGS (self, GTK_CAN_FOCUS);
  gtk_widget_set_double_buffered (widget, FALSE);

  self->proxy = 0;
  self->control_type = BSE_MIDI_SIGNAL_CONTINUOUS_7; /* valoume */
  self->ppqn = 384;	/* default Parts (clock ticks) Per Quarter Note */
  self->qnpt = 1;
  self->max_ticks = 1;
  self->hzoom = 1;
  self->draw_qn_grid = TRUE;
  self->draw_qqn_grid = TRUE;
  self->vpanel_width = 20;
  self->vpanel = NULL;
  self->canvas = NULL;
  self->canvas_cursor = GDK_LEFT_PTR;
  self->vpanel_cursor = GDK_HAND2;
  self->hadjustment = NULL;
  self->scroll_timer = 0;
  self->selection_tick = 0;
  self->selection_duration = 0;
  bst_event_roll_hsetup (self, 384, 4, 800 * 384, 1);
  bst_event_roll_set_hadjustment (self, NULL);
  
  bst_ascii_pixbuf_ref ();
}

static void
bst_event_roll_destroy (GtkObject *object)
{
  BstEventRoll *self = BST_EVENT_ROLL (object);

  bst_event_roll_set_proxy (self, 0);
  bst_event_roll_set_hadjustment (self, NULL);
  
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bst_event_roll_dispose (GObject *object)
{
  BstEventRoll *self = BST_EVENT_ROLL (object);

  bst_event_roll_set_proxy (self, 0);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
bst_event_roll_finalize (GObject *object)
{
  BstEventRoll *self = BST_EVENT_ROLL (object);

  bst_event_roll_set_proxy (self, 0);

  g_object_unref (self->hadjustment);
  self->hadjustment = NULL;

  if (self->scroll_timer)
    {
      g_source_remove (self->scroll_timer);
      self->scroll_timer = 0;
    }
  bst_ascii_pixbuf_unref ();

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

void
bst_event_roll_set_hadjustment (BstEventRoll  *self,
				GtkAdjustment *adjustment)
{
  g_return_if_fail (BST_IS_EVENT_ROLL (self));
  if (adjustment)
    g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));

  if (self->hadjustment)
    {
      g_object_disconnect (self->hadjustment,
			   "any_signal", event_roll_adjustment_changed, self,
			   "any_signal", event_roll_adjustment_value_changed, self,
			   NULL);
      g_object_unref (self->hadjustment);
      self->hadjustment = NULL;
    }

  if (!adjustment)
    adjustment = g_object_new (GTK_TYPE_ADJUSTMENT, NULL);

  self->hadjustment = g_object_ref (adjustment);
  gtk_object_sink (GTK_OBJECT (adjustment));
  g_object_connect (self->hadjustment,
		    "swapped_signal::changed", event_roll_adjustment_changed, self,
		    "swapped_signal::value-changed", event_roll_adjustment_value_changed, self,
		    NULL);
}

static void
bst_event_roll_set_scroll_adjustments (BstEventRoll  *self,
				       GtkAdjustment *hadjustment,
				       GtkAdjustment *vadjustment)
{
  if (self->hadjustment != hadjustment)
    bst_event_roll_set_hadjustment (self, hadjustment);
}

static void
event_roll_reset_backgrounds (BstEventRoll *self)
{
  GtkWidget *widget = GTK_WIDGET (self);

  if (GTK_WIDGET_REALIZED (self))
    {
      GdkColor colors[BST_EVENT_ROLL_N_COLORS] = {
	{ 0, 0x0000, 0x0000, 0xffff },	/* blue */
	{ 0, 0xffff, 0x0000, 0x0000 },	/* red */
      };
      guint i;

      for (i = 0; i < BST_EVENT_ROLL_N_COLORS; i++)
	gdk_gc_set_rgb_fg_color (self->color_gc[i], colors + i);

      gtk_style_set_background (widget->style, widget->window, GTK_WIDGET_STATE (self));
      gdk_window_set_background (self->vpanel, VPANEL_BG_COLOR (self));
      gdk_window_set_background (self->canvas, CANVAS_BG_COLOR (self));
      gdk_window_clear (widget->window);
      gdk_window_clear (self->vpanel);
      gdk_window_clear (self->canvas);
      gtk_widget_queue_draw (widget);
    }
}

static void
bst_event_roll_style_set (GtkWidget *widget,
			  GtkStyle  *previous_style)
{
  BstEventRoll *self = BST_EVENT_ROLL (widget);

  if (self->fetch_vpanel_width)
    self->vpanel_width = self->fetch_vpanel_width (self->fetch_vpanel_width_data);
  event_roll_reset_backgrounds (self);
}

static void
bst_event_roll_state_changed (GtkWidget *widget,
			      guint	 previous_state)
{
  BstEventRoll *self = BST_EVENT_ROLL (widget);

  event_roll_reset_backgrounds (self);
}

static void
bst_event_roll_size_request (GtkWidget	    *widget,
			     GtkRequisition *requisition)
{
  BstEventRoll *self = BST_EVENT_ROLL (widget);

  if (self->child && GTK_WIDGET_VISIBLE (self->child))
    {
      GtkRequisition child_requisition;
      gtk_widget_size_request (self->child, &child_requisition);
    }

  if (self->fetch_vpanel_width)
    self->vpanel_width = self->fetch_vpanel_width (self->fetch_vpanel_width_data);

  requisition->width = CANVAS_X (self) + 500 + XTHICKNESS (self);
  requisition->height = CANVAS_Y (self) + 128 * YTHICKNESS (self);
}

static void
bst_event_roll_size_allocate (GtkWidget	    *widget,
			      GtkAllocation *allocation)
{
  BstEventRoll *self = BST_EVENT_ROLL (widget);
  guint real_width = allocation->width;
  guint real_height = allocation->height;
  
  widget->allocation.x = allocation->x;
  widget->allocation.y = allocation->y;
  widget->allocation.width = MAX (CANVAS_X (self) + 1 + XTHICKNESS (self), allocation->width);
  widget->allocation.height = MAX (CANVAS_Y (self) + 1 + YTHICKNESS (self), allocation->height);
  // widget->allocation.height = MIN (CANVAS_Y (self) + YTHICKNESS (self), widget->allocation.height);

  if (self->fetch_vpanel_width)
    self->vpanel_width = self->fetch_vpanel_width (self->fetch_vpanel_width_data);
  
  if (GTK_WIDGET_REALIZED (widget))
    {
      gdk_window_move_resize (widget->window,
			      widget->allocation.x, widget->allocation.y,
			      real_width, real_height);
      gdk_window_move_resize (self->vpanel,
			      VPANEL_X (self), VPANEL_Y (self),
			      VPANEL_WIDTH (self), VPANEL_HEIGHT (self));
      gdk_window_move_resize (self->canvas,
			      CANVAS_X (self), CANVAS_Y (self),
			      CANVAS_WIDTH (self), CANVAS_HEIGHT (self));
    }
  if (self->child)
    {
      GtkAllocation child_allocation;
      child_allocation.x = XTHICKNESS (self);
      child_allocation.y = 0;
      child_allocation.width = MAX (VPANEL_WIDTH (self) - 2 * XTHICKNESS (self), 1);
      child_allocation.height = VPANEL_HEIGHT (self);
      gtk_widget_size_allocate (self->child, &child_allocation);
    }
  event_roll_update_adjustments (self, TRUE, TRUE);
}

static void
bst_event_roll_add (GtkContainer *container,
                    GtkWidget    *child)
{
  BstEventRoll *self = BST_EVENT_ROLL (container);

  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (self->child == NULL);

  gtk_widget_set_parent_window (child, self->vpanel);
  self->child = child;
  gtk_widget_set_parent (child, GTK_WIDGET (self));
}

static void
bst_event_roll_remove (GtkContainer *container,
                       GtkWidget    *child)
{
  BstEventRoll *self = BST_EVENT_ROLL (container);

  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (self->child == child);

  gtk_widget_unparent (child);
  self->child = NULL;
}

static void
bst_event_roll_forall (GtkContainer *container,
                       gboolean      include_internals,
                       GtkCallback   callback,
                       gpointer      callback_data)
{
  BstEventRoll *self = BST_EVENT_ROLL (container);

  g_return_if_fail (callback != NULL);

  if (self->child)
    callback (self->child, callback_data);
}

static void
bst_event_roll_realize (GtkWidget *widget)
{
  BstEventRoll *self = BST_EVENT_ROLL (widget);
  GdkWindowAttr attributes;
  GdkCursorType cursor_type;
  guint attributes_mask, i;
  
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  if (self->fetch_vpanel_width)
    self->vpanel_width = self->fetch_vpanel_width (self->fetch_vpanel_width_data);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

  /* widget->window */
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.event_mask = gtk_widget_get_events (widget) | (GDK_EXPOSURE_MASK |
							    GDK_ENTER_NOTIFY_MASK |
							    GDK_LEAVE_NOTIFY_MASK);
  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
  gdk_window_set_user_data (widget->window, self);

  /* self->vpanel */
  attributes.x = VPANEL_X (self);
  attributes.y = VPANEL_Y (self);
  attributes.width = VPANEL_WIDTH (self);
  attributes.height = VPANEL_HEIGHT (self);
  attributes.event_mask = gtk_widget_get_events (widget) | (GDK_EXPOSURE_MASK |
							    GDK_BUTTON_PRESS_MASK |
							    GDK_BUTTON_RELEASE_MASK |
							    GDK_BUTTON_MOTION_MASK |
							    GDK_POINTER_MOTION_HINT_MASK);
  self->vpanel = gdk_window_new (widget->window, &attributes, attributes_mask);
  gdk_window_set_user_data (self->vpanel, self);
  gdk_window_show (self->vpanel);
  
  /* self->canvas */
  attributes.x = CANVAS_X (self);
  attributes.y = CANVAS_Y (self);
  attributes.width = CANVAS_WIDTH (self);
  attributes.height = CANVAS_HEIGHT (self);
  attributes.event_mask = gtk_widget_get_events (widget) | (GDK_EXPOSURE_MASK |
							    GDK_BUTTON_PRESS_MASK |
							    GDK_BUTTON_RELEASE_MASK |
							    GDK_BUTTON_MOTION_MASK |
							    GDK_POINTER_MOTION_HINT_MASK |
							    GDK_KEY_PRESS_MASK |
							    GDK_KEY_RELEASE_MASK);
  self->canvas = gdk_window_new (widget->window, &attributes, attributes_mask);
  gdk_window_set_user_data (self->canvas, self);
  gdk_window_show (self->canvas);

  /* allocate color GCs */
  for (i = 0; i < BST_EVENT_ROLL_N_COLORS; i++)
    self->color_gc[i] = gdk_gc_new (self->canvas);

  /* style setup */
  widget->style = gtk_style_attach (widget->style, widget->window);
  event_roll_reset_backgrounds (self);

  /* update cursors */
  cursor_type = self->canvas_cursor;
  self->canvas_cursor = -1;
  bst_event_roll_set_canvas_cursor (self, cursor_type);
  cursor_type = self->vpanel_cursor;
  self->vpanel_cursor = -1;
  bst_event_roll_set_vpanel_cursor (self, cursor_type);

  if (self->child)
    gtk_widget_set_parent_window (self->child, self->vpanel);
}

static void
bst_event_roll_unrealize (GtkWidget *widget)
{
  BstEventRoll *self = BST_EVENT_ROLL (widget);
  guint i;

  for (i = 0; i < BST_EVENT_ROLL_N_COLORS; i++)
    {
      g_object_unref (self->color_gc[i]);
      self->color_gc[i] = NULL;
    }
  gdk_window_set_user_data (self->canvas, NULL);
  gdk_window_destroy (self->canvas);
  self->canvas = NULL;
  gdk_window_set_user_data (self->vpanel, NULL);
  gdk_window_destroy (self->vpanel);
  self->vpanel = NULL;

  if (GTK_WIDGET_CLASS (parent_class)->unrealize)
    GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static gint
ticks_to_pixels (BstEventRoll *self,
		 gint	       ticks)
{
  gfloat ppqn = self->ppqn;
  gfloat tpixels = QNOTE_HPIXELS;

  /* compute pixel span of a tick range */

  tpixels *= self->hzoom / ppqn * (gfloat) ticks;
  if (ticks)
    tpixels = MAX (tpixels, 1);
  return tpixels;
}

static gint
pixels_to_ticks (BstEventRoll *self,
		 gint	       pixels)
{
  gfloat ppqn = self->ppqn;
  gfloat ticks = 1.0 / (gfloat) QNOTE_HPIXELS;

  /* compute tick span of a pixel range */

  ticks = ticks * ppqn / self->hzoom * (gfloat) pixels;
  if (pixels > 0)
    ticks = MAX (ticks, 1);
  else
    ticks = 0;
  return ticks;
}

static gint
tick_to_coord (BstEventRoll *self,
	       gint	     tick)
{
  return ticks_to_pixels (self, tick) - self->x_offset;
}

static gint
coord_to_tick (BstEventRoll *self,
	       gint	     x,
	       gboolean	     right_bound)
{
  guint tick;

  x += self->x_offset;
  tick = pixels_to_ticks (self, x);
  if (right_bound)
    {
      guint tick2 = pixels_to_ticks (self, x + 1);

      if (tick2 > tick)
	tick = tick2 - 1;
    }
  return tick;
}

static void
bst_event_roll_overlap_grow_vpanel_area (BstEventRoll *self,
					 GdkRectangle *area)
{
  // FIXME: grow vpanel exposes?
}

static void
bst_event_roll_draw_vpanel (BstEventRoll *self,
			    gint	  y,
			    gint	  ybound)
{
  GdkWindow *drawable = self->vpanel;

  /* outer vpanel shadow */
  gtk_paint_shadow (STYLE (self), drawable,
                    STATE (self), GTK_SHADOW_OUT,
                    NULL, NULL, NULL,
                    0, -YTHICKNESS (self),
                    VPANEL_WIDTH (self), VPANEL_HEIGHT (self) + 2 * YTHICKNESS (self));
}

static void
bst_event_roll_draw_canvas (BstEventRoll *self,
			    gint          x,
			    gint	  y,
			    gint          xbound,
			    gint          ybound)
{
  guint8 *dash, dashes[3] = { 1, 1, 0 }; // long: { 2, 2, 0 };
  GdkWindow *drawable = self->canvas;
  GdkGC *light_gc, *dark_gc = STYLE (self)->dark_gc[GTK_STATE_NORMAL];
  gint i, dlen, line_width = 0; /* line widths != 0 interfere with dash-settings on some X servers */
  gint mid = (CANVAS_HEIGHT (self) - 1) / 2, range = mid;
  BsePartControlSeq *cseq;

  /* draw selection */
  if (self->selection_duration)
    {
      gint x1, x2;
      x1 = tick_to_coord (self, self->selection_tick);
      x2 = tick_to_coord (self, self->selection_tick + self->selection_duration);
      gdk_draw_rectangle (drawable, GTK_WIDGET (self)->style->bg_gc[GTK_STATE_SELECTED], TRUE,
			  x1, 0, MAX (x2 - x1, 0), CANVAS_HEIGHT (self));
    }

  /* draw horizontal grid lines */
  gdk_gc_set_line_attributes (dark_gc, line_width, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER);
  dash = dashes;
  dlen = dash[0] + dash[1];
  gdk_gc_set_dashes (dark_gc, (self->x_offset + x + 1) % dlen, dash, 2);
  gdk_draw_line (drawable, dark_gc, x, mid + range / 2, xbound - 1, mid + range / 2);
  gdk_draw_line (drawable, dark_gc, x, mid - range / 2, xbound - 1, mid - range / 2);
  gdk_gc_set_line_attributes (dark_gc, line_width, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
  gdk_draw_line (drawable, dark_gc, x, mid, xbound - 1, mid);

  /* draw notes */
  light_gc = STYLE (self)->light_gc[GTK_STATE_NORMAL];
  dark_gc = STYLE (self)->dark_gc[GTK_STATE_NORMAL];
  cseq = self->proxy ? bse_part_list_controls (self->proxy,
                                               coord_to_tick (self, x, FALSE),
                                               coord_to_tick (self, xbound, FALSE),
                                               self->control_type) : NULL;
  for (i = 0; cseq && i < cseq->n_pcontrols; i++)
    {
      BsePartControl *pctrl = cseq->pcontrols[i];
      guint tick = pctrl->tick;
      GdkGC *xdark_gc, *xlight_gc, *xval_gc;
      gint x1, x2, y1, y2;
      gboolean selected = pctrl->selected;

      selected |= (pctrl->tick >= self->selection_tick &&
		   pctrl->tick < self->selection_tick + self->selection_duration);
      if (selected)
	{
	  xdark_gc = STYLE (self)->bg_gc[GTK_STATE_SELECTED];
	  xval_gc = STYLE (self)->fg_gc[GTK_STATE_SELECTED];
	  xlight_gc = STYLE (self)->bg_gc[GTK_STATE_SELECTED];
	}
      else
	{
	  xdark_gc = dark_gc;
          if (ABS (pctrl->value) < 0.00001)
            xval_gc = self->color_gc[1];
          else
            xval_gc = self->color_gc[0];
	  xlight_gc = dark_gc;
	}
      x1 = tick_to_coord (self, tick);
      x2 = x1 + XTHICKNESS (self);
      x1 = MAX (0, x1 - XTHICKNESS (self));
      if (pctrl->value * range > 0)
        {
          y1 = mid - MAX (YTHICKNESS (self), range * pctrl->value);
          y2 = mid + YTHICKNESS (self);
        }
      else if (pctrl->value * range < 0)
        {
          y1 = mid - YTHICKNESS (self);
          y2 = mid + MAX (YTHICKNESS (self), range * -pctrl->value);
        }
      else /* pctrl->value * range == 0 */
        {
          y1 = mid - YTHICKNESS (self);
          y2 = mid + YTHICKNESS (self);
        }

      gdk_draw_rectangle (drawable, xval_gc, TRUE, x1, y1, MAX (x2 - x1, 1), MAX (y2 - y1, 1));
      gdk_draw_line (drawable, xdark_gc, x1, y2, x2, y2);
      gdk_draw_line (drawable, xdark_gc, x2, y1, x2, y2);
      gdk_draw_line (drawable, xlight_gc, x1, y1, x2, y1);
      gdk_draw_line (drawable, xlight_gc, x1, y1, x1, y2);
#define TICK_WIDTH(self)        (pixels_to_ticks (self, XTHICKNESS (self) * 2 + 1))
    }
  // FIXME: redraw area needs to be fudged by value-rectangle width around edges */
}

static void
bst_event_roll_overlap_grow_canvas_area (BstEventRoll *self,
					 GdkRectangle *area)
{
  gint i, x = area->x, xbound = x + area->width;

  /* grow canvas paint area by value bar width, so we don't clear out parts of neighbouring bars */
  x -= TICK_WIDTH (self);
  xbound += TICK_WIDTH (self);

  area->x = MAX (x, 0);
  area->width = xbound - area->x;
}

static void
bst_event_roll_draw_window (BstEventRoll *self,
                            gint          x,
                            gint          y,
                            gint          xbound,
                            gint          ybound)
{
  GdkWindow *drawable = GTK_WIDGET (self)->window;
  GtkWidget *widget = GTK_WIDGET (self);
  GdkGC *bg_gc = STYLE (self)->bg_gc[GTK_WIDGET_STATE (self)];
  gint width = 0;
  gint height = VPANEL_Y (self);

  gdk_draw_rectangle (drawable, bg_gc, TRUE,
                      0, 0, width, height);

  /* draw hpanel and vpanel scrolling boundaries */
  gtk_paint_shadow (widget->style, drawable,
                    widget->state, GTK_SHADOW_ETCHED_IN,
                    NULL, NULL, NULL,
                    - XTHICKNESS (self), - YTHICKNESS (self),
                    width + XTHICKNESS (self), height + YTHICKNESS (self));
  /* draw outer scrollpanel shadow */
  gtk_paint_shadow (widget->style, drawable,
                    widget->state, GTK_SHADOW_OUT,
                    NULL, NULL, NULL,
                    0, 0, width + XTHICKNESS (self), height + YTHICKNESS (self));
  /* draw inner scrollpanel corner */
  gtk_paint_shadow (widget->style, drawable,
                    widget->state, GTK_SHADOW_IN,
                    NULL, NULL, NULL,
                    width - XTHICKNESS (self), height - YTHICKNESS (self),
                    2 * XTHICKNESS (self), 2 * YTHICKNESS (self));
}

static gboolean
bst_event_roll_expose (GtkWidget      *widget,
		       GdkEventExpose *event)
{
  BstEventRoll *self = BST_EVENT_ROLL (widget);
  GdkRectangle area = event->area;
  
  /* with gtk_widget_set_double_buffered (self, FALSE) in init and
   * with gdk_window_begin_paint_region()/gdk_window_end_paint()
   * around our redraw functions, we can decide on our own on what
   * windows we want double buffering.
   */
  if (event->window == widget->window)
    {
      gdk_window_begin_paint_rect (event->window, &area);
      bst_event_roll_draw_window (self, area.x, area.y, area.x + area.width, area.y + area.height);
      gdk_window_end_paint (event->window);
    }
  else if (event->window == self->vpanel)
    {
      bst_event_roll_overlap_grow_vpanel_area (self, &area);
      gdk_window_begin_paint_rect (event->window, &area);
      bst_event_roll_overlap_grow_vpanel_area (self, &area);
      bst_event_roll_draw_vpanel (self, area.y, area.y + area.height);
      gdk_window_end_paint (event->window);
    }
  else if (event->window == self->canvas)
    {
      gdk_window_begin_paint_rect (event->window, &area);
      bst_event_roll_overlap_grow_canvas_area (self, &area);
      bst_event_roll_draw_canvas (self, area.x, area.y, area.x + area.width, area.y + area.height);
      gdk_window_end_paint (event->window);
    }
  return GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);
}

static void
event_roll_adjustment_changed (BstEventRoll *self)
{
}

static void
event_roll_adjustment_value_changed (BstEventRoll  *self,
				     GtkAdjustment *adjustment)
{
  if (adjustment == self->hadjustment)
    {
      gint x = self->x_offset, diff;

      self->x_offset = ticks_to_pixels (self, adjustment->value);
      diff = x - self->x_offset;
      if (diff && GTK_WIDGET_DRAWABLE (self))
	{
	  GdkRectangle area = { 0, };

	  gdk_window_scroll (self->canvas, diff, 0);
	  area.x = diff < 0 ? CANVAS_WIDTH (self) + diff : 0;
	  area.y = 0;
	  area.width = ABS (diff);
	  area.height = CANVAS_HEIGHT (self);
	  gdk_window_invalidate_rect (self->canvas, &area, TRUE);
	}
    }
}

static void
event_roll_update_adjustments (BstEventRoll *self,
			       gboolean      hadj,
			       gboolean      vadj)
{
  gdouble hv = self->hadjustment->value;

  if (hadj)
    {
      self->hadjustment->lower = 0;
      self->hadjustment->upper = self->max_ticks;
      self->hadjustment->page_size = pixels_to_ticks (self, CANVAS_WIDTH (self));
      self->hadjustment->step_increment = self->ppqn;
      self->hadjustment->page_increment = self->ppqn * self->qnpt;
      self->hadjustment->value = CLAMP (self->hadjustment->value,
					self->hadjustment->lower,
					self->hadjustment->upper - self->hadjustment->page_size);
      gtk_adjustment_changed (self->hadjustment);
    }
  if (hv != self->hadjustment->value)
    gtk_adjustment_value_changed (self->hadjustment);
}

static void
event_roll_scroll_adjustments (BstEventRoll *self,
			       gint          x_pixel,
			       gint          y_pixel)
{
  gdouble hv = self->hadjustment->value;
  gint xdiff, ydiff;
  gint ticks;

  xdiff = x_pixel * AUTO_SCROLL_SCALE;
  ydiff = y_pixel * AUTO_SCROLL_SCALE;

  ticks = pixels_to_ticks (self, ABS (xdiff));
  if (x_pixel > 0)
    ticks = MAX (ticks, 1);
  else if (x_pixel < 0)
    ticks = MIN (-1, -ticks);
  self->hadjustment->value += ticks;
  self->hadjustment->value = CLAMP (self->hadjustment->value,
				    self->hadjustment->lower,
				    self->hadjustment->upper - self->hadjustment->page_size);
  if (y_pixel > 0)
    ydiff = MAX (ydiff, 1);
  else if (y_pixel < 0)
    ydiff = MIN (-1, ydiff);
  if (hv != self->hadjustment->value)
    gtk_adjustment_value_changed (self->hadjustment);
}

static void
bst_event_roll_hsetup (BstEventRoll *self,
		       guint	     ppqn,
		       guint	     qnpt,
		       guint	     max_ticks,
		       gfloat	     hzoom)
{
  guint old_ppqn = self->ppqn;
  guint old_qnpt = self->qnpt;
  guint old_max_ticks = self->max_ticks;
  gfloat old_hzoom = self->hzoom;

  /* here, we setup all things necessary to determine our
   * horizontal layout. we have to avoid resizes at
   * least if just max_ticks changes, since the tick range
   * might need to grow during pointer grabs
   */
  self->ppqn = MAX (ppqn, 1);
  self->qnpt = CLAMP (qnpt, 3, 4);
  self->max_ticks = MAX (max_ticks, 1);
  self->hzoom = CLAMP (hzoom, 0.01, 100);

  if (old_ppqn != self->ppqn ||
      old_qnpt != self->qnpt ||
      old_max_ticks != self->max_ticks ||	// FIXME: shouldn't always cause a redraw
      old_hzoom != self->hzoom)
    {
      if (self->hadjustment)
	{
	  self->x_offset = ticks_to_pixels (self, self->hadjustment->value);
	  event_roll_update_adjustments (self, TRUE, FALSE);
	}
      self->draw_qn_grid = ticks_to_pixels (self, self->ppqn) >= 3;
      self->draw_qqn_grid = ticks_to_pixels (self, self->ppqn / 4) >= 5;
      gtk_widget_queue_draw (GTK_WIDGET (self));
    }
}

gfloat
bst_event_roll_set_hzoom (BstEventRoll *self,
			  gfloat        hzoom)
{
  g_return_val_if_fail (BST_IS_EVENT_ROLL (self), 0);

  bst_event_roll_hsetup (self, self->ppqn, self->qnpt, self->max_ticks, hzoom);

  return self->hzoom;
}

static gboolean
bst_event_roll_focus_in (GtkWidget     *widget,
			 GdkEventFocus *event)
{
  BstEventRoll *self = BST_EVENT_ROLL (widget);

  GTK_WIDGET_SET_FLAGS (self, GTK_HAS_FOCUS);
  // bst_event_roll_draw_focus (self);
  return TRUE;
}

static gboolean
bst_event_roll_focus_out (GtkWidget	*widget,
			  GdkEventFocus *event)
{
  BstEventRoll *self = BST_EVENT_ROLL (widget);

  GTK_WIDGET_UNSET_FLAGS (self, GTK_HAS_FOCUS);
  // bst_event_roll_draw_focus (self);
  return TRUE;
}

static void
bst_event_roll_canvas_drag_abort (BstEventRoll *self)
{
  if (self->canvas_drag)
    {
      self->canvas_drag = FALSE;
      self->drag.type = BST_DRAG_ABORT;
      g_signal_emit (self, signal_canvas_drag, 0, &self->drag);
    }
}

static gboolean
bst_event_roll_canvas_drag (BstEventRoll *self,
			    gint	  coord_x,
			    gint	  coord_y,
			    gboolean      initial)
{
  if (self->canvas_drag)
    {
      gint mid = (CANVAS_HEIGHT (self) - 1) / 2, range = mid;
      self->drag.current_tick = coord_to_tick (self, MAX (coord_x, 0), FALSE);
      self->drag.current_value = mid - coord_y;
      self->drag.current_value /= range;
      self->drag.current_value = CLAMP (self->drag.current_value, -1, +1);
      self->drag.current_valid = ABS (self->drag.current_value) <= 1;
      if (initial)
	{
          self->drag.tick_width = TICK_WIDTH (self);
	  self->drag.start_tick = self->drag.current_tick;
	  self->drag.start_value = self->drag.current_value;
	  self->drag.start_valid = self->drag.current_valid;
	}
      g_signal_emit (self, signal_canvas_drag, 0, &self->drag);
      if (self->drag.state == BST_DRAG_HANDLED)
	self->canvas_drag = FALSE;
      else if (self->drag.state == BST_DRAG_ERROR)
	bst_event_roll_canvas_drag_abort (self);
      else if (initial && self->drag.state == BST_DRAG_UNHANDLED)
	return TRUE;
    }
  return FALSE;
}

static void
bst_event_roll_vpanel_drag_abort (BstEventRoll *self)
{
  if (self->vpanel_drag)
    {
      self->vpanel_drag = FALSE;
      self->drag.type = BST_DRAG_ABORT;
      g_signal_emit (self, signal_vpanel_drag, 0, &self->drag);
    }
}

static gboolean
bst_event_roll_vpanel_drag (BstEventRoll *self,
			   gint	         coord_x,
			   gint	         coord_y,
			   gboolean      initial)
{
  if (self->vpanel_drag)
    {
      self->drag.current_tick = 0;
      self->drag.current_value = coord_y;
      self->drag.current_valid = ABS (self->drag.current_value) <= 1;
      if (initial)
	{
          self->drag.tick_width = TICK_WIDTH (self);
	  self->drag.start_tick = self->drag.current_tick;
	  self->drag.start_value = self->drag.current_value;
	  self->drag.start_valid = self->drag.current_valid;
	}
      g_signal_emit (self, signal_vpanel_drag, 0, &self->drag);
      if (self->drag.state == BST_DRAG_HANDLED)
	self->vpanel_drag = FALSE;
      else if (self->drag.state == BST_DRAG_ERROR)
	bst_event_roll_vpanel_drag_abort (self);
      else if (initial && self->drag.state == BST_DRAG_UNHANDLED)
	return TRUE;
    }
  return FALSE;
}

static gboolean
bst_event_roll_button_press (GtkWidget	    *widget,
			     GdkEventButton *event)
{
  BstEventRoll *self = BST_EVENT_ROLL (widget);
  gboolean handled = FALSE;
  
  if (!GTK_WIDGET_HAS_FOCUS (widget))
    gtk_widget_grab_focus (widget);

  if (event->window == self->canvas && !self->canvas_drag)
    {
      handled = TRUE;
      self->drag.eroll = self;
      self->drag.type = BST_DRAG_START;
      self->drag.mode = bst_drag_modifier_start (event->state);
      self->drag.button = event->button;
      self->drag.state = BST_DRAG_UNHANDLED;
      self->canvas_drag = TRUE;
      if (bst_event_roll_canvas_drag (self, event->x, event->y, TRUE) == TRUE)
	{
	  self->canvas_drag = FALSE;
	  g_signal_emit (self, signal_canvas_clicked, 0, self->drag.button, self->drag.start_tick, self->drag.start_value, event);
	}
    }
  else if (event->window == self->vpanel && !self->vpanel_drag)
    {
      handled = TRUE;
      self->drag.eroll = self;
      self->drag.type = BST_DRAG_START;
      self->drag.mode = bst_drag_modifier_start (event->state);
      self->drag.button = event->button;
      self->drag.state = BST_DRAG_UNHANDLED;
      self->vpanel_drag = TRUE;
      if (bst_event_roll_vpanel_drag (self, event->x, event->y, TRUE) == TRUE)
	{
	  self->vpanel_drag = FALSE;
	  g_signal_emit (self, signal_vpanel_clicked, 0, self->drag.button, self->drag.start_tick, self->drag.start_value, event);
	}
    }

  return handled;
}

static gboolean
timeout_scroller (gpointer data)
{
  BstEventRoll *self;
  guint remain = 1;

  GDK_THREADS_ENTER ();
  self = BST_EVENT_ROLL (data);
  if (self->canvas_drag && GTK_WIDGET_DRAWABLE (self))
    {
      gint x, y, width, height, xdiff = 0, ydiff = 0;
      GdkModifierType modifiers;

      gdk_window_get_size (self->canvas, &width, &height);
      gdk_window_get_pointer (self->canvas, &x, &y, &modifiers);
      if (x < 0)
	xdiff = x;
      else if (x >= width)
	xdiff = x - width + 1;
      if (y < 0)
	ydiff = y;
      else if (y >= height)
	ydiff = y - height + 1;
      if (xdiff || ydiff)
	{
	  event_roll_scroll_adjustments (self, xdiff, ydiff);
	  self->drag.type = BST_DRAG_MOTION;
	  self->drag.mode = bst_drag_modifier_next (modifiers, self->drag.mode);
	  bst_event_roll_canvas_drag (self, x, y, FALSE);
	}
      else
	self->scroll_timer = remain = 0;
    }
  else if (self->vpanel_drag && GTK_WIDGET_DRAWABLE (self))
    {
      gint x, y, height, ydiff = 0;
      GdkModifierType modifiers;

      gdk_window_get_size (self->vpanel, NULL, &height);
      gdk_window_get_pointer (self->vpanel, &x, &y, &modifiers);
      if (y < 0)
	ydiff = y;
      else if (y >= height)
	ydiff = y - height + 1;
      if (ydiff)
	{
	  event_roll_scroll_adjustments (self, 0, ydiff);
	  self->drag.type = BST_DRAG_MOTION;
	  self->drag.mode = bst_drag_modifier_next (modifiers, self->drag.mode);
	  bst_event_roll_vpanel_drag (self, x, y, FALSE);
	}
      else
	self->scroll_timer = remain = 0;
    }
  else
    self->scroll_timer = remain = 0;
  GDK_THREADS_LEAVE ();

  return remain;
}

static gboolean
bst_event_roll_motion (GtkWidget      *widget,
		       GdkEventMotion *event)
{
  BstEventRoll *self = BST_EVENT_ROLL (widget);
  gboolean handled = FALSE;

  if (event->window == self->canvas && self->canvas_drag)
    {
      gint width, height;
      handled = TRUE;
      self->drag.type = BST_DRAG_MOTION;
      self->drag.mode = bst_drag_modifier_next (event->state, self->drag.mode);
      bst_event_roll_canvas_drag (self, event->x, event->y, FALSE);
      gdk_window_get_size (self->canvas, &width, &height);
      if (!self->scroll_timer && (event->x < 0 || event->x >= width ||
				  event->y < 0 || event->y >= height))
	self->scroll_timer = g_timeout_add_full (G_PRIORITY_DEFAULT,
						 AUTO_SCROLL_TIMEOUT,
						 timeout_scroller,
						 self, NULL);
      /* trigger motion events (since we use motion-hint) */
      gdk_window_get_pointer (self->canvas, NULL, NULL, NULL);
    }
  if (event->window == self->vpanel && self->vpanel_drag)
    {
      gint height;
      handled = TRUE;
      self->drag.type = BST_DRAG_MOTION;
      self->drag.mode = bst_drag_modifier_next (event->state, self->drag.mode);
      bst_event_roll_vpanel_drag (self, event->x, event->y, FALSE);
      gdk_window_get_size (self->vpanel, NULL, &height);
      if (!self->scroll_timer && (event->y < 0 || event->y >= height))
	self->scroll_timer = g_timeout_add_full (G_PRIORITY_DEFAULT,
						 AUTO_SCROLL_TIMEOUT,
						 timeout_scroller,
						 self, NULL);
      /* trigger motion events (since we use motion-hint) */
      gdk_window_get_pointer (self->vpanel, NULL, NULL, NULL);
    }

  return handled;
}

static gboolean
bst_event_roll_button_release (GtkWidget      *widget,
			       GdkEventButton *event)
{
  BstEventRoll *self = BST_EVENT_ROLL (widget);
  gboolean handled = FALSE;

  if (event->window == self->canvas && self->canvas_drag && event->button == self->drag.button)
    {
      handled = TRUE;
      self->drag.type = BST_DRAG_DONE;
      self->drag.mode = bst_drag_modifier_next (event->state, self->drag.mode);
      bst_event_roll_canvas_drag (self, event->x, event->y, FALSE);
      self->canvas_drag = FALSE;
    }
  else if (event->window == self->vpanel && self->vpanel_drag && event->button == self->drag.button)
    {
      handled = TRUE;
      self->drag.type = BST_DRAG_DONE;
      self->drag.mode = bst_drag_modifier_next (event->state, self->drag.mode);
      bst_event_roll_vpanel_drag (self, event->x, event->y, FALSE);
      self->vpanel_drag = FALSE;
    }

  return handled;
}

static gboolean
bst_event_roll_key_press (GtkWidget   *widget,
			  GdkEventKey *event)
{
  BstEventRoll *self = BST_EVENT_ROLL (widget);
  gboolean handled = TRUE;

  if (event->keyval == GDK_Escape)
    {
      bst_event_roll_canvas_drag_abort (self);
      bst_event_roll_vpanel_drag_abort (self);
    }

  return handled;
}

static gboolean
bst_event_roll_key_release (GtkWidget   *widget,
			    GdkEventKey *event)
{
  // BstEventRoll *self = BST_EVENT_ROLL (widget);
  gboolean handled = TRUE;

  return handled;
}

static void
event_roll_update (BstEventRoll *self,
		   guint         tick_start,
		   guint         duration)
{
  guint tick_end = tick_start + MAX (duration, 1) - 1;
  gint x1 = tick_to_coord (self, tick_start);
  gint x2 = tick_to_coord (self, tick_end);
  GdkRectangle area;

  area.x = x1 - 3;		/* add fudge */
  area.y = 0;
  area.width = x2 - x1 + 3 + 3;	/* add fudge */
  area.height = CANVAS_HEIGHT (self);
  gdk_window_invalidate_rect (self->canvas, &area, TRUE);
}

static void
event_roll_unset_proxy (BstEventRoll *self)
{
  bst_event_roll_set_proxy (self, 0);
}

void
bst_event_roll_set_proxy (BstEventRoll *self,
			  SfiProxy      proxy)
{
  g_return_if_fail (BST_IS_EVENT_ROLL (self));
  if (proxy)
    {
      g_return_if_fail (BSE_IS_ITEM (proxy));
      g_return_if_fail (bse_item_get_project (proxy) != 0);
    }

  if (self->proxy)
    {
      bse_proxy_disconnect (self->proxy,
			    "any_signal", event_roll_unset_proxy, self,
			    "any_signal", event_roll_update, self,
			    NULL);
      bse_item_unuse (self->proxy);
    }
  self->proxy = proxy;
  if (self->proxy)
    {
      bse_item_use (self->proxy);
      bse_proxy_connect (self->proxy,
			 "swapped_signal::release", event_roll_unset_proxy, self,
			 // "swapped_signal::property-notify::uname", event_roll_update_name, self,
			 "swapped_signal::range-changed", event_roll_update, self,
			 NULL);
      self->max_ticks = bse_part_get_max_tick (self->proxy);
      bst_event_roll_hsetup (self, self->ppqn, self->qnpt, self->max_ticks, self->hzoom);
    }
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

void
bst_event_roll_set_vpanel_width_hook (BstEventRoll   *self,
                                      gint          (*fetch_vpanel_width) (gpointer data),
                                      gpointer        data)
{
  g_return_if_fail (BST_IS_EVENT_ROLL (self));

  self->fetch_vpanel_width = fetch_vpanel_width;
  self->fetch_vpanel_width_data = data;
}

static void
event_roll_queue_region (BstEventRoll *self,
			 guint         tick,
			 guint         duration)
{
  if (self->proxy && duration)
    bse_part_queue_controls (self->proxy, tick, duration);
  event_roll_update (self, tick, duration);
}

void
bst_event_roll_set_view_selection (BstEventRoll *self,
				   guint         tick,
				   guint         duration)
{
  g_return_if_fail (BST_IS_EVENT_ROLL (self));
  
  if (!duration)	/* invalid selection */
    {
      tick = 0;
      duration = 0;
    }
  
  if (self->selection_duration && duration)
    {
      /* if at least one corner of the old an the new selection
       * matches, it's probably worth updating only diff-regions
       */
      if ((tick == self->selection_tick ||
	   tick + duration == self->selection_tick + self->selection_duration))
	{
	  guint start, end;
	  /* difference on the left */
	  start = MIN (tick, self->selection_tick);
	  end = MAX (tick, self->selection_tick);
	  if (end != start)
	    event_roll_queue_region (self, start, end - start);
	  /* difference on the right */
	  start = MIN (tick + duration, self->selection_tick + self->selection_duration);
	  end = MAX (tick + duration, self->selection_tick + self->selection_duration);
	  if (end != start)
	    event_roll_queue_region (self, start, end - start);
	  start = MIN (tick, self->selection_tick);
	  end = MAX (tick + duration, self->selection_tick + self->selection_duration);
	}
      else
	{
	  /* simply update new and old selection */
	  event_roll_queue_region (self, self->selection_tick, self->selection_duration);
	  event_roll_queue_region (self, tick, duration);
	}
    }
  else if (self->selection_duration)
    event_roll_queue_region (self, self->selection_tick, self->selection_duration);
  else /* duration != 0 */
    event_roll_queue_region (self, tick, duration);
  self->selection_tick = tick;
  self->selection_duration = duration;
}

void
bst_event_roll_set_quantization (BstEventRoll *self,
				 guint         note_fraction)
{
  g_return_if_fail (BST_IS_EVENT_ROLL (self));

  switch (note_fraction)
    {
    case 1:
    case 2:
    case 4:
    case 8:
    case 16:
    case 32:
    case 64:
    case 128:
    case 256:
    case 512:
      self->quantization = note_fraction;
      break;
    default:
      self->quantization = 0;
      break;
    }
}

guint
bst_event_roll_quantize (BstEventRoll *self,
			 guint         fine_tick)
{
  g_return_val_if_fail (BST_IS_EVENT_ROLL (self), fine_tick);

  /* quantize tick */
  if (self->quantization)
    {
      guint quant = self->ppqn * 4 / self->quantization;
      guint qtick = fine_tick / quant;
      qtick *= quant;
      if (fine_tick - qtick > quant / 2 &&
	  qtick + quant > fine_tick)
	fine_tick = qtick + quant;
      else
	fine_tick = qtick;
    }
  return fine_tick;
}

void
bst_event_roll_set_control_type (BstEventRoll   *self,
                                 guint           control_type)
{
  g_return_if_fail (BST_IS_EVENT_ROLL (self));

  self->control_type = control_type;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

void
bst_event_roll_set_canvas_cursor (BstEventRoll *self,
				  GdkCursorType cursor_type)
{
  g_return_if_fail (BST_IS_EVENT_ROLL (self));

  if (cursor_type != self->canvas_cursor)
    {
      self->canvas_cursor = cursor_type;
      if (GTK_WIDGET_REALIZED (self))
	gxk_window_set_cursor_type (self->canvas, self->canvas_cursor);
    }
}

void
bst_event_roll_set_vpanel_cursor (BstEventRoll *self,
				  GdkCursorType cursor_type)
{
  g_return_if_fail (BST_IS_EVENT_ROLL (self));

  if (cursor_type != self->vpanel_cursor)
    {
      self->vpanel_cursor = cursor_type;
      if (GTK_WIDGET_REALIZED (self))
	gxk_window_set_cursor_type (self->vpanel, self->vpanel_cursor);
    }
}
