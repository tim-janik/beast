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
#include	"bstpianoroll.h"

#include	"bstasciipixbuf.h"


/* --- defines --- */
/* helpers */
#define	STYLE(self)		(GTK_WIDGET (self)->style)
#define	ALLOCATION(self)	(&GTK_WIDGET (self)->allocation)
#define	N_OCTAVES(self)		((self)->max_octave - (self)->min_octave + 1)

/* layout (requisition) */
#define	NOTE_HEIGHT(self)	((gint) ((self)->vzoom * 1.2))		/* factor must be between 1 .. 2 */
#define	OCTAVE_HEIGHT(self)	(14 * (self)->vzoom + 7 * NOTE_HEIGHT (self))	/* coord_to_note() */
#define	VPANEL_WIDTH(self)	(35)		/* (self)->vpanel_width) */
#define	VPANEL_RATIO(self)	(2.9 / 5.)	/* black/white key ratio */
#define	HPANEL_HEIGHT(self)	((self)->hpanel_height)
#define	VPANEL_X(self)		(STYLE (self)->xthickness)
#define	HPANEL_Y(self)		(STYLE (self)->ythickness)
#define	CANVAS_X(self)		(VPANEL_X (self) + VPANEL_WIDTH (self))
#define	CANVAS_Y(self)		(HPANEL_Y (self) + HPANEL_HEIGHT (self))

/* layout (allocation) */
#define	CANVAS_WIDTH(self)	(ALLOCATION (self)->width - CANVAS_X (self) - STYLE (self)->xthickness)
#define	CANVAS_HEIGHT(self)	(ALLOCATION (self)->height - CANVAS_Y (self) - STYLE (self)->ythickness)

/* aliases */
#define	VPANEL_HEIGHT(self)	(CANVAS_HEIGHT (self))
#define	HPANEL_WIDTH(self)	(CANVAS_WIDTH (self))
#define	VPANEL_Y(self)		(CANVAS_Y (self))
#define	HPANEL_X(self)		(CANVAS_X (self))

/* appearance */
#define	VPANEL_BG_GC(self)	(&STYLE (self)->bg[GTK_STATE_INSENSITIVE])
#define	HPANEL_BG_GC(self)	(&STYLE (self)->bg[GTK_WIDGET_IS_SENSITIVE (self) ? GTK_STATE_SELECTED : GTK_STATE_INSENSITIVE])
#define	CANVAS_BG_GC(self)	(&STYLE (self)->base[GTK_WIDGET_STATE (self)])
#define	KEY_DEFAULT_VPIXELS	(4)
#define	QNOTE_HPIXELS		(30)	/* guideline */


/* --- prototypes --- */
static void	bst_piano_roll_class_init		(BstPianoRollClass	*class);
static void	bst_piano_roll_init			(BstPianoRoll		*self);
static void	bst_piano_roll_dispose			(GObject		*object);
static void	bst_piano_roll_destroy			(GtkObject		*object);
static void	bst_piano_roll_finalize			(GObject		*object);
static void	bst_piano_roll_set_scroll_adjustments	(BstPianoRoll		*self,
							 GtkAdjustment		*hadjustment,
							 GtkAdjustment		*vadjustment);
static void	bst_piano_roll_size_request		(GtkWidget		*widget,
							 GtkRequisition		*requisition);
static void	bst_piano_roll_size_allocate		(GtkWidget		*widget,
							 GtkAllocation		*allocation);
static void	bst_piano_roll_style_set		(GtkWidget		*widget,
							 GtkStyle		*previous_style);
static void	bst_piano_roll_state_changed		(GtkWidget		*widget,
							 guint			 previous_state);
static void	bst_piano_roll_realize			(GtkWidget		*widget);
static void	bst_piano_roll_unrealize		(GtkWidget		*widget);
static gboolean	bst_piano_roll_focus_in			(GtkWidget		*widget,
							 GdkEventFocus		*event);
static gboolean	bst_piano_roll_focus_out		(GtkWidget		*widget,
							 GdkEventFocus		*event);
static gboolean	bst_piano_roll_expose			(GtkWidget		*widget,
							 GdkEventExpose		*event);
static void	bst_piano_roll_draw_focus		(BstPianoRoll		*self);
static gboolean	bst_piano_roll_key_press		(GtkWidget		*widget,
							 GdkEventKey		*event);
static gboolean	bst_piano_roll_key_release		(GtkWidget		*widget,
							 GdkEventKey		*event);
static gboolean	bst_piano_roll_button_press		(GtkWidget		*widget,
							 GdkEventButton		*event);
static gboolean	bst_piano_roll_motion			(GtkWidget		*widget,
							 GdkEventMotion		*event);
static gboolean	bst_piano_roll_button_release		(GtkWidget		*widget,
							 GdkEventButton		*event);
static void	piano_roll_update_adjustments		(BstPianoRoll		*self,
							 gboolean		 hadj,
							 gboolean		 vadj);
static void	piano_roll_adjustment_changed		(BstPianoRoll		*self);
static void	piano_roll_adjustment_value_changed	(BstPianoRoll		*self,
							 GtkAdjustment		*adjustment);
static void	bst_piano_roll_hsetup			(BstPianoRoll		*self,
							 guint			 ppqn,
							 guint			 qnpt,
							 guint			 max_ticks,
							 gfloat			 hzoom);

/* --- static variables --- */
static gpointer	parent_class = NULL;
static guint	signal_canvas_press = 0;
static guint	signal_canvas_motion = 0;
static guint	signal_canvas_release = 0;


/* --- functions --- */
GType
bst_piano_roll_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info = {
	sizeof (BstPianoRollClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) bst_piano_roll_class_init,
	NULL,   /* class_finalize */
	NULL,   /* class_data */
	sizeof (BstPianoRoll),
	0,      /* n_preallocs */
	(GInstanceInitFunc) bst_piano_roll_init,
      };

      type = g_type_register_static (GTK_TYPE_CONTAINER,
				     "BstPianoRoll",
				     &type_info, 0);
    }

  return type;
}

static void
bst_piano_roll_class_init (BstPianoRollClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  gobject_class->dispose = bst_piano_roll_dispose;
  gobject_class->finalize = bst_piano_roll_finalize;

  object_class->destroy = bst_piano_roll_destroy;
  
  widget_class->size_request = bst_piano_roll_size_request;
  widget_class->size_allocate = bst_piano_roll_size_allocate;
  widget_class->realize = bst_piano_roll_realize;
  widget_class->unrealize = bst_piano_roll_unrealize;
  widget_class->style_set = bst_piano_roll_style_set;
  widget_class->state_changed = bst_piano_roll_state_changed;
  widget_class->expose_event = bst_piano_roll_expose;
  widget_class->focus_in_event = bst_piano_roll_focus_in;
  widget_class->focus_out_event = bst_piano_roll_focus_out;
  widget_class->key_press_event = bst_piano_roll_key_press;
  widget_class->key_release_event = bst_piano_roll_key_release;
  widget_class->button_press_event = bst_piano_roll_button_press;
  widget_class->motion_notify_event = bst_piano_roll_motion;
  widget_class->button_release_event = bst_piano_roll_button_release;

  class->set_scroll_adjustments = bst_piano_roll_set_scroll_adjustments;
  class->canvas_press = NULL;
  class->canvas_motion = NULL;
  class->canvas_release = NULL;
  
  signal_canvas_press = g_signal_new ("canvas-press", G_OBJECT_CLASS_TYPE (class),
				      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstPianoRollClass, canvas_press),
				      NULL, NULL,
				      bst_marshal_NONE__UINT_UINT_FLOAT,
				      G_TYPE_NONE, 3, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_FLOAT);
  signal_canvas_motion = g_signal_new ("canvas-motion", G_OBJECT_CLASS_TYPE (class),
				       G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstPianoRollClass, canvas_motion),
				       NULL, NULL,
				       bst_marshal_NONE__UINT_UINT_FLOAT,
				       G_TYPE_NONE, 3, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_FLOAT);
  signal_canvas_release = g_signal_new ("canvas-release", G_OBJECT_CLASS_TYPE (class),
					G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstPianoRollClass, canvas_release),
					NULL, NULL,
					bst_marshal_NONE__UINT_UINT_FLOAT,
					G_TYPE_NONE, 3, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_FLOAT);
  widget_class->set_scroll_adjustments_signal =
    gtk_signal_new ("set_scroll_adjustments",
		    GTK_RUN_LAST,
		    GTK_CLASS_TYPE (object_class),
		    GTK_SIGNAL_OFFSET (BstPianoRollClass, set_scroll_adjustments),
		    bst_marshal_NONE__OBJECT_OBJECT,
		    GTK_TYPE_NONE, 2, GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);
}

static void
bst_piano_roll_init (BstPianoRoll *self)
{
  GtkWidget *widget = GTK_WIDGET (self);

  GTK_WIDGET_UNSET_FLAGS (self, GTK_NO_WINDOW);
  GTK_WIDGET_SET_FLAGS (self, GTK_CAN_FOCUS);
  gtk_widget_set_double_buffered (widget, FALSE);

  self->proxy = 0;
  self->vzoom = KEY_DEFAULT_VPIXELS;
  self->ppqn = 384;	/* default Parts (clock ticks) Per Quarter Note */
  self->qnpt = 1;
  self->max_ticks = 1;
  self->hzoom = 1;
  self->draw_qn_grid = TRUE;
  self->draw_qqn_grid = TRUE;
  self->min_octave = -1;
  self->max_octave = 2;
  self->hpanel_height = 20;
  self->vpanel = NULL;
  self->hpanel = NULL;
  self->canvas = NULL;
  self->canvas_cursor = GDK_LEFT_PTR;
  self->vpanel_cursor = GDK_HAND2;
  self->hpanel_cursor = GDK_LEFT_PTR;
  self->hadjustment = NULL;
  self->vadjustment = NULL;
  bst_piano_roll_hsetup (self, 384, 4, 800 * 384, 1);
  bst_piano_roll_set_hadjustment (self, NULL);
  bst_piano_roll_set_vadjustment (self, NULL);

  bst_ascii_pixbuf_ref ();
}

static void
bst_piano_roll_destroy (GtkObject *object)
{
  BstPianoRoll *self = BST_PIANO_ROLL (object);

  bst_piano_roll_set_proxy (self, 0);
  bst_piano_roll_set_hadjustment (self, NULL);
  bst_piano_roll_set_vadjustment (self, NULL);
  
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bst_piano_roll_dispose (GObject *object)
{
  BstPianoRoll *self = BST_PIANO_ROLL (object);

  bst_piano_roll_set_proxy (self, 0);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
bst_piano_roll_finalize (GObject *object)
{
  BstPianoRoll *self = BST_PIANO_ROLL (object);

  bst_piano_roll_set_proxy (self, 0);

  g_object_unref (self->hadjustment);
  self->hadjustment = NULL;
  g_object_unref (self->vadjustment);
  self->vadjustment = NULL;
  
  bst_ascii_pixbuf_unref ();

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

gfloat
bst_piano_roll_set_vzoom (BstPianoRoll *self,
			  gfloat        vzoom)
{
  g_return_val_if_fail (BST_IS_PIANO_ROLL (self), 0);

  self->vzoom = vzoom; //  * KEY_DEFAULT_VPIXELS;
  self->vzoom = CLAMP (self->vzoom, 1, 16);

  gtk_widget_queue_resize (GTK_WIDGET (self));

  return self->vzoom;
}

void
bst_piano_roll_set_hadjustment (BstPianoRoll  *self,
				GtkAdjustment *adjustment)
{
  g_return_if_fail (BST_IS_PIANO_ROLL (self));
  if (adjustment)
    g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));

  if (self->hadjustment)
    {
      g_object_disconnect (self->hadjustment,
			   "any_signal", piano_roll_adjustment_changed, self,
			   "any_signal", piano_roll_adjustment_value_changed, self,
			   NULL);
      g_object_unref (self->hadjustment);
      self->hadjustment = NULL;
    }

  if (!adjustment)
    adjustment = g_object_new (GTK_TYPE_ADJUSTMENT, NULL);

  self->hadjustment = g_object_ref (adjustment);
  gtk_object_sink (GTK_OBJECT (adjustment));
  g_object_connect (self->hadjustment,
		    "swapped_signal::changed", piano_roll_adjustment_changed, self,
		    "swapped_signal::value-changed", piano_roll_adjustment_value_changed, self,
		    NULL);
  bst_piano_roll_adjust_scroll_area (self);
}

void
bst_piano_roll_set_vadjustment (BstPianoRoll  *self,
				GtkAdjustment *adjustment)
{
  g_return_if_fail (BST_IS_PIANO_ROLL (self));
  if (adjustment)
    g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));

  if (self->vadjustment)
    {
      g_object_disconnect (self->vadjustment,
			   "any_signal", piano_roll_adjustment_changed, self,
			   "any_signal", piano_roll_adjustment_value_changed, self,
			   NULL);
      g_object_unref (self->vadjustment);
      self->vadjustment = NULL;
    }

  if (!adjustment)
    adjustment = g_object_new (GTK_TYPE_ADJUSTMENT, NULL);

  self->vadjustment = g_object_ref (adjustment);
  gtk_object_sink (GTK_OBJECT (adjustment));
  g_object_connect (self->vadjustment,
		    "swapped_signal::changed", piano_roll_adjustment_changed, self,
		    "swapped_signal::value-changed", piano_roll_adjustment_value_changed, self,
		    NULL);
  bst_piano_roll_adjust_scroll_area (self);
}

static void
bst_piano_roll_set_scroll_adjustments (BstPianoRoll  *self,
				       GtkAdjustment *hadjustment,
				       GtkAdjustment *vadjustment)
{
  if (self->hadjustment != hadjustment)
    bst_piano_roll_set_hadjustment (self, hadjustment);
  if (self->vadjustment != vadjustment)
    bst_piano_roll_set_vadjustment (self, vadjustment);
}

static void
piano_roll_reset_backgrounds (BstPianoRoll *self)
{
  GtkWidget *widget = GTK_WIDGET (self);

  if (GTK_WIDGET_REALIZED (self))
    {
      GdkColor colors[12] = {
	/* C: */
	{ 0, 0x8080, 0x0000, 0x0000 },	/* dark red */
	{ 0, 0xa000, 0x0000, 0xa000 },	/* dark magenta */
	/* D: */
	{ 0, 0x0000, 0x8000, 0x8000 },	/* dark turquoise */
	{ 0, 0x0000, 0xff00, 0xff00 },	/* light turquoise */
	/* E: */
	{ 0, 0xff00, 0xff00, 0x0000 },	/* bright yellow */
	/* F: */
	{ 0, 0x0000, 0x7000, 0x0000 },	/* dark green */
	{ 0, 0x4000, 0xff00, 0x4000 },	/* bright green */
	/* G: */
	{ 0, 0xff00, 0x4000, 0x4000 },	/* light red */
	{ 0, 0xff00, 0x8000, 0x0000 },	/* bright orange */
	/* A: */
	{ 0, 0x0000, 0x0000, 0x9000 },	/* dark blue */
	{ 0, 0x4000, 0x4000, 0xff00 },	/* light blue */
	/* B: */
	{ 0, 0xff00, 0x0000, 0x8000 },	/* light pink */
      };
      guint i;

      for (i = 0; i < 12; i++)
	gdk_gc_set_rgb_fg_color (self->color_gc[i], colors + i);

      gtk_style_set_background (widget->style, widget->window, GTK_WIDGET_STATE (self));
      gdk_window_set_background (self->vpanel,
				 (GTK_WIDGET_IS_SENSITIVE (self)
				  ? &STYLE (self)->bg[GTK_STATE_NORMAL]
				  : &STYLE (self)->bg[GTK_STATE_INSENSITIVE]));
      gdk_window_set_background (self->hpanel,
				 (GTK_WIDGET_IS_SENSITIVE (self)
				  ? &STYLE (self)->bg[GTK_STATE_SELECTED]
				  : &STYLE (self)->bg[GTK_STATE_INSENSITIVE]));
      gdk_window_set_background (self->canvas, &STYLE (self)->base[GTK_WIDGET_STATE (self)]);
      gdk_window_clear (widget->window);
      gdk_window_clear (self->vpanel);
      gdk_window_clear (self->hpanel);
      gdk_window_clear (self->canvas);
      gtk_widget_queue_draw (widget);
    }
}

static void
bst_piano_roll_style_set (GtkWidget *widget,
			  GtkStyle  *previous_style)
{
  BstPianoRoll *self = BST_PIANO_ROLL (widget);

  self->hpanel_height = 20;
  piano_roll_reset_backgrounds (self);
}

static void
bst_piano_roll_state_changed (GtkWidget *widget,
			      guint	 previous_state)
{
  BstPianoRoll *self = BST_PIANO_ROLL (widget);

  piano_roll_reset_backgrounds (self);
}

static void
bst_piano_roll_size_request (GtkWidget	    *widget,
			     GtkRequisition *requisition)
{
  BstPianoRoll *self = BST_PIANO_ROLL (widget);

  requisition->width = CANVAS_X (self) + 500 + STYLE (self)->xthickness;
  requisition->height = CANVAS_Y (self) + N_OCTAVES (self) * OCTAVE_HEIGHT (self) + STYLE (self)->ythickness;
  g_print ("request: n: %d (%d %d)\n", N_OCTAVES (self), self->min_octave, self->max_octave);
}

static void
bst_piano_roll_size_allocate (GtkWidget	    *widget,
			      GtkAllocation *allocation)
{
  BstPianoRoll *self = BST_PIANO_ROLL (widget);

  widget->allocation.x = allocation->x;
  widget->allocation.y = allocation->y;
  widget->allocation.width = MAX (CANVAS_X (self) + 1 + STYLE (self)->xthickness, allocation->width);
  widget->allocation.height = MAX (CANVAS_Y (self) + 1 + STYLE (self)->ythickness, allocation->height);
  widget->allocation.height = MIN (CANVAS_Y (self) + N_OCTAVES (self) * OCTAVE_HEIGHT (self) + STYLE (self)->ythickness,
				   widget->allocation.height);

  if (GTK_WIDGET_REALIZED (widget))
    {
      gdk_window_move_resize (widget->window,
			      widget->allocation.x, widget->allocation.y,
			      widget->allocation.width, widget->allocation.height);
      gdk_window_move_resize (self->vpanel,
			      VPANEL_X (self), VPANEL_Y (self),
			      VPANEL_WIDTH (self), VPANEL_HEIGHT (self));
      gdk_window_move_resize (self->hpanel,
			      HPANEL_X (self), HPANEL_Y (self),
			      HPANEL_WIDTH (self), HPANEL_HEIGHT (self));
      gdk_window_move_resize (self->canvas,
			      CANVAS_X (self), CANVAS_Y (self),
			      CANVAS_WIDTH (self), CANVAS_HEIGHT (self));
    }
  piano_roll_update_adjustments (self, TRUE, TRUE);
  bst_piano_roll_adjust_scroll_area (self);
}

static void
bst_piano_roll_realize (GtkWidget *widget)
{
  BstPianoRoll *self = BST_PIANO_ROLL (widget);
  GdkWindowAttr attributes;
  GdkCursorType cursor_type;
  guint attributes_mask, i;
  
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

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
							    GDK_POINTER_MOTION_MASK);
  self->vpanel = gdk_window_new (widget->window, &attributes, attributes_mask);
  gdk_window_set_user_data (self->vpanel, self);
  gdk_window_show (self->vpanel);
  
  /* self->hpanel */
  attributes.x = HPANEL_X (self);
  attributes.y = HPANEL_Y (self);
  attributes.width = HPANEL_WIDTH (self);
  attributes.height = HPANEL_HEIGHT (self);
  attributes.event_mask = gtk_widget_get_events (widget) | (GDK_EXPOSURE_MASK);
  self->hpanel = gdk_window_new (widget->window, &attributes, attributes_mask);
  gdk_window_set_user_data (self->hpanel, self);
  gdk_window_show (self->hpanel);

  /* self->canvas */
  attributes.x = CANVAS_X (self);
  attributes.y = CANVAS_Y (self);
  attributes.width = CANVAS_WIDTH (self);
  attributes.height = CANVAS_HEIGHT (self);
  attributes.event_mask = gtk_widget_get_events (widget) | (GDK_EXPOSURE_MASK |
							    GDK_BUTTON_PRESS_MASK |
							    GDK_BUTTON_RELEASE_MASK |
							    GDK_BUTTON1_MOTION_MASK |
							    GDK_POINTER_MOTION_MASK |
							    GDK_KEY_PRESS_MASK |
							    GDK_KEY_RELEASE_MASK);
  self->canvas = gdk_window_new (widget->window, &attributes, attributes_mask);
  gdk_window_set_user_data (self->canvas, self);
  gdk_window_show (self->canvas);

  /* allocate color GCs */
  for (i = 0; i < 12; i++)
    self->color_gc[i] = gdk_gc_new (self->canvas);

  /* style setup */
  widget->style = gtk_style_attach (widget->style, widget->window);
  piano_roll_reset_backgrounds (self);
  bst_piano_roll_adjust_scroll_area (self);

  /* update cursors */
  cursor_type = self->canvas_cursor++;
  bst_piano_roll_set_canvas_cursor (self, cursor_type);
  cursor_type = self->vpanel_cursor++;
  bst_piano_roll_set_vpanel_cursor (self, cursor_type);
  cursor_type = self->hpanel_cursor++;
  bst_piano_roll_set_hpanel_cursor (self, cursor_type);
}

static void
bst_piano_roll_unrealize (GtkWidget *widget)
{
  BstPianoRoll *self = BST_PIANO_ROLL (widget);
  guint i;

  for (i = 0; i < 12; i++)
    {
      g_object_unref (self->color_gc[i]);
      self->color_gc[i] = NULL;
    }
  gdk_window_set_user_data (self->canvas, NULL);
  gdk_window_destroy (self->canvas);
  self->canvas = NULL;
  gdk_window_set_user_data (self->hpanel, NULL);
  gdk_window_destroy (self->hpanel);
  self->hpanel = NULL;
  gdk_window_set_user_data (self->vpanel, NULL);
  gdk_window_destroy (self->vpanel);
  self->vpanel = NULL;

  if (GTK_WIDGET_CLASS (parent_class)->unrealize)
    GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static gint
ticks_to_pixel (BstPianoRoll *self,
		gint	      ticks)
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
pixels_to_ticks (BstPianoRoll *self,
		 gint	       pixels)
{
  gfloat ppqn = self->ppqn;
  gfloat ticks = 1.0 / (gfloat) QNOTE_HPIXELS;

  /* compute tick span of a pixel range */

  ticks = ticks * ppqn / self->hzoom * (gfloat) pixels;
  if (pixels)
    ticks = MAX (ticks, 1);
  return ticks;
}

static gint
tick_to_coord (BstPianoRoll *self,
	       gint	     tick)
{
  return ticks_to_pixel (self, tick) - self->x_offset;
}

static gint
coord_to_tick (BstPianoRoll *self,
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

#define	CROSSING_TACT		(1)
#define	CROSSING_QNOTE		(2)
#define	CROSSING_QNOTE_Q	(3)

static guint
coord_check_crossing (BstPianoRoll *self,
		      gint	    x,
		      guint	    crossing)
{
  guint ltick = coord_to_tick (self, x, FALSE);
  guint rtick = coord_to_tick (self, x, TRUE);
  guint lq = 0, rq = 0;

  /* catch _at_ tick boundary as well */
  rtick += 1;

  switch (crossing)
    {
    case CROSSING_TACT:
      lq = ltick / (self->ppqn * self->qnpt);
      rq = rtick / (self->ppqn * self->qnpt);
      break;
    case CROSSING_QNOTE:
      lq = ltick / self->ppqn;
      rq = rtick / self->ppqn;
      break;
    case CROSSING_QNOTE_Q:
      lq = ltick * 4 / self->ppqn;
      rq = rtick * 4 / self->ppqn;
      break;
    }

  return lq != rq;
}

#define	DRAW_NONE	(0)
#define	DRAW_START	(1)
#define	DRAW_MIDDLE	(2)
#define	DRAW_END	(3)

typedef struct {
  gint  octave;
  guint note;		/* 0 .. 11    within octave */
  guint key;		/* 0 .. 6     index of white key */
  guint key_frac;	/* 0 .. 4*z-1 fractional pixel index into key */
  guint wstate;		/* DRAW_ START/MIDDLE/END of white key */
  guint bstate;		/* DRAW_ NONE/START/MIDDLE/END of black key */
  guint bmatch;		/* TRUE if on black key (differs from bstate!=NONE) */
} NoteInfo;

static gint
note_to_coord (BstPianoRoll *self,
	       guint         note,
	       gint	     octave,
	       gint	    *height_p)
{
  gint ythickness = 1, z = self->vzoom, h = NOTE_HEIGHT (self);
  gint oheight = OCTAVE_HEIGHT (self), y, zz = z + z, offs = 0, height = h;

  switch (note)
    {
    case 10:	offs += zz + h;
    case  8:	offs += zz + h;
    case  6:	offs += zz + h + zz + h;
    case  3:	offs += zz + h;
    case  1:	offs += z + h + (zz - h) / 2;
      break;
    case 11:	offs += h + zz;
    case  9:	offs += h + zz;
    case  7:	offs += h + zz;
    case  5:	offs += h + zz;
    case  4:	offs += h + zz;
    case  2:	offs += h + zz;
    case  0:	offs += z;
      break;
    }
  octave = N_OCTAVES (self) - 1 - octave + self->min_octave;
  y = octave * oheight;
  y += oheight - offs - h;

  /* spacing out by a bit looks nicer */
  if (z >= 4)
    {
      height += ythickness;
    }

  if (height_p)
    *height_p = height;

  return y - self->y_offset;
}

static gboolean
coord_to_note (BstPianoRoll *self,
	       gint          y,
	       NoteInfo	    *info)
{
  gint ythickness = 1, i, z = self->vzoom, h = NOTE_HEIGHT (self);
  gint end_shift, start_shift, black_shift = 0;
  gint oheight = OCTAVE_HEIGHT (self), kheight = 2 * z + h;

  y += self->y_offset;
  info->octave = y / oheight;
  i = y - info->octave * oheight;
  i = oheight - 1 - i;		/* octave increases with decreasing y */
  info->key = i / kheight;
  info->key_frac = i - info->key * kheight;
  i = info->key_frac;
  info->octave = N_OCTAVES (self) - 1 - info->octave + self->min_octave;

  /* pixel layout and note numbers:
   * Iz|h|zIz|h|zIz|h|zIz|h|zIz|h|zIz|h|zIz|h|zI
   * I 0 |#1#|2|#3#|4  I  5|#6#|7|#8#|9|#10|11 I
   * I   |###| |###|   I   |###| |###| |###|   I
   * I   +-+-+ +-+-+   I   +-+-+ +-+-+ +-+-+   I
   * I     I     I     I     I     I     I     I
   * +--0--+--1--+--2--+--3--+--4--+--5--+--6--+
   */

  /* figure black notes */
  end_shift = i >= z + h;
  start_shift = i < z + ythickness;
  info->note = 0;
  switch (info->key)
    {
    case 3:	info->note += 5;
    case 0:
      info->note += 0 + end_shift;
      black_shift = end_shift;
      break;
    case 5:	info->note += 2;
    case 4:	info->note += 5;
    case 1:
      info->note += 2 + (start_shift ? -1 : end_shift);
      black_shift = start_shift || end_shift;
      break;
    case 6:	info->note += 7;
    case 2:
      info->note += 4 - start_shift;
      black_shift = start_shift;
      break;
    }

  /* figure draw states */
  if (i < ythickness)
    info->wstate = DRAW_START;
  else if (i < kheight - ythickness)
    info->wstate = DRAW_MIDDLE;
  else
    info->wstate = DRAW_END;
  if (!black_shift)
    info->bstate = DRAW_NONE;
  else if (i < z)
    info->bstate = DRAW_MIDDLE;
  else if (i < z + ythickness)
    info->bstate = DRAW_END;
  else if (i < z + h + ythickness)
    info->bstate = DRAW_START;
  else
    info->bstate = DRAW_MIDDLE;

  /* behaviour fixup, ignore black note borders */
  if (black_shift && info->bstate == DRAW_END)
    {
      info->bmatch = FALSE;
      info->note += 1;
    }
  else if (black_shift && info->bstate == DRAW_START)
    {
      info->bmatch = FALSE;
      info->note -= 1;
    }
  else
    info->bmatch = TRUE;

  return info->bmatch != 0;
}

static void
bst_piano_roll_overlap_grow_vpanel_area (BstPianoRoll *self,
					 GdkRectangle *area)
{
  /* grow hpanel exposes by surrounding white keys */
  area->y -= OCTAVE_HEIGHT (self) / 7;			/* fudge 1 key upwards */
  area->height += OCTAVE_HEIGHT (self) / 7;
  area->height += OCTAVE_HEIGHT (self) / 7;		/* fudge 1 key downwards */
}

static void
bst_piano_roll_draw_vpanel (BstPianoRoll *self,
			    gint	  y,
			    gint	  ybound)
{
  GdkWindow *window = self->vpanel;
  GdkGC *black_gc = STYLE (self)->fg_gc[GTK_STATE_NORMAL];
  GdkGC *dark_gc = STYLE (self)->dark_gc[GTK_STATE_NORMAL];
  GdkGC *light_gc = STYLE (self)->light_gc[GTK_STATE_NORMAL];
  gint i, white_x = VPANEL_WIDTH (self) - 1, black_x = white_x * VPANEL_RATIO (self);

  y = MAX (y, 0);

  /* draw vertical frame lines */
  gdk_draw_line (window, dark_gc, white_x, y, white_x, ybound - 1);

  /* draw horizontal lines */
  for (i = y; i < ybound; i++)
    {
      gint x = black_x + 1;
      NoteInfo info;

      coord_to_note (self, i, &info);
      switch (info.bstate)
	{
	case DRAW_START:
	  gdk_draw_line (window, dark_gc, 0, i, black_x, i);
	  break;
	case DRAW_MIDDLE:
	  gdk_draw_line (window, black_gc, 0, i, black_x - 1, i);
	  gdk_draw_line (window, dark_gc, black_x, i, black_x, i);
	  break;
	case DRAW_END:
	  gdk_draw_line (window, light_gc, 0, i, black_x, i);
	  break;
	default:
	  x = 0;
	}
      switch (info.wstate)
	{
	case DRAW_START:
	  gdk_draw_line (window, dark_gc, x, i, white_x, i);
	  if (info.note == 0)	/* C */
	    {
	      gint pbheight, ypos, ythickness = 1, overlap = 1;
	      gint pbwidth = white_x - black_x + overlap;
	      GdkPixbuf *pixbuf;

	      pbheight = OCTAVE_HEIGHT (self) / 7;
	      pbwidth /= 2;
	      ypos = i - pbheight + ythickness;
	      pixbuf = bst_ascii_pixbuf_new ('C', pbwidth, pbheight);
	      gdk_pixbuf_render_to_drawable (pixbuf, window, light_gc, 0, 0,
					     black_x, ypos, -1, -1,
					     GDK_RGB_DITHER_MAX, 0, 0);
	      g_object_unref (pixbuf);
	      if (info.octave < 0)
		{
		  guint indent = pbwidth * 0.4;

		  /* render a minus '-' for negative octaves into the 'C' */
		  pixbuf = bst_ascii_pixbuf_new ('-', pbwidth - indent, pbheight - 1);
		  gdk_pixbuf_render_to_drawable (pixbuf, window, light_gc, 0, 0,
						 black_x + indent, ypos, -1, -1,
						 GDK_RGB_DITHER_MAX, 0, 0);
		  g_object_unref (pixbuf);
		}
	      pixbuf = bst_ascii_pixbuf_new (ABS (info.octave) + '0', pbwidth, pbheight);
	      gdk_pixbuf_render_to_drawable (pixbuf, window, light_gc, 0, 0,
					     black_x + pbwidth - overlap, ypos, -1, -1,
					     GDK_RGB_DITHER_MAX, 0, 0);
	      g_object_unref (pixbuf);
	    }
	  break;
	case DRAW_MIDDLE:
	  // gdk_draw_line (window, white_gc, x, i, white_x, i);
	  break;
	case DRAW_END:
	  gdk_draw_line (window, light_gc, x, i, white_x, i);
	  break;
	}
    }
}

static void
bst_piano_roll_draw_canvas (BstPianoRoll *self,
			    gint          x,
			    gint	  y,
			    gint          xbound,
			    gint          ybound)
{
  GdkWindow *window = self->canvas;
  GdkGC *light_gc, *note_gc, *dark_gc = STYLE (self)->dark_gc[GTK_STATE_NORMAL];
  gint i, dlen, line_width = 0; /* line widths != 0 interfere with dash-settings on some X servers */
  BswVIter *iter;

  /* draw horizontal grid lines */
  for (i = y; i < ybound; i++)
    {
      NoteInfo info;

      coord_to_note (self, i, &info);
      if (info.wstate != DRAW_START)
	continue;

      if (info.note == 0)
	{
	  gdk_gc_set_line_attributes (dark_gc, line_width, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
	  gdk_draw_line (window, dark_gc, x, i, xbound - 1, i);
	}
      else if (info.note == 5)
	{
	  guint8 dash[3] = { 2, 2, 0 };
	  
	  gdk_gc_set_line_attributes (dark_gc, line_width, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER);
	  dlen = dash[0] + dash[1];
	  gdk_gc_set_dashes (dark_gc, (self->x_offset + x + 1) % dlen, dash, 2);
	  gdk_draw_line (window, dark_gc, x, i, xbound - 1, i);
	  gdk_gc_set_line_attributes (dark_gc, 0, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
	}
      else
	{
	  guint8 dash[3] = { 1, 1, 0 };
	  
	  gdk_gc_set_line_attributes (dark_gc, line_width, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER);
	  dlen = dash[0] + dash[1];
	  gdk_gc_set_dashes (dark_gc, (self->x_offset + x + 1) % dlen, dash, 2);
	  gdk_draw_line (window, dark_gc, x, i, xbound - 1, i);
	  gdk_gc_set_line_attributes (dark_gc, 0, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
	}
    }

  /* draw vertical grid lines */
  for (i = x; i < xbound; i++)
    {
      if (coord_check_crossing (self, i, CROSSING_TACT))
	{
	  gdk_gc_set_line_attributes (dark_gc, line_width, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
	  gdk_draw_line (window, dark_gc, i, y, i, ybound - 1);
	}
      else if (self->draw_qn_grid && coord_check_crossing (self, i, CROSSING_QNOTE))
	{
	  guint8 dash[3] = { 2, 2, 0 };

	  gdk_gc_set_line_attributes (dark_gc, line_width, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER);
	  dlen = dash[0] + dash[1];
	  gdk_gc_set_dashes (dark_gc, (self->y_offset + y + 1) % dlen, dash, 2);
	  gdk_draw_line (window, dark_gc, i, y, i, ybound - 1);
	  gdk_gc_set_line_attributes (dark_gc, 0, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
	}
      else if (self->draw_qqn_grid && coord_check_crossing (self, i, CROSSING_QNOTE_Q))
	{
	  guint8 dash[3] = { 1, 1, 0 };

	  gdk_gc_set_line_attributes (dark_gc, line_width, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER);
	  dlen = dash[0] + dash[1];
	  gdk_gc_set_dashes (dark_gc, (self->y_offset + y + 1) % dlen, dash, 2);
	  gdk_draw_line (window, dark_gc, i, y, i, ybound - 1);
	  gdk_gc_set_line_attributes (dark_gc, 0, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
	}
    }

  /* draw notes */
  light_gc = STYLE (self)->light_gc[GTK_STATE_NORMAL];
  note_gc = STYLE (self)->bg_gc[GTK_STATE_SELECTED];
  dark_gc = STYLE (self)->dark_gc[GTK_STATE_NORMAL];
  iter = self->proxy ? bsw_part_find_notes (self->proxy,
					    coord_to_tick (self, x, FALSE),
					    coord_to_tick (self, xbound, FALSE)) : NULL;
  for (; iter && bsw_viter_n_left (iter); bsw_viter_next (iter))
    {
      BswPartNote *pnote = bsw_viter_get_boxed (iter);
      BswNoteDescription *info = bsw_server_note_from_freq (BSW_SERVER, pnote->freq);
      guint start = pnote->tick, end = start + pnote->duration;
      gint x1, x2, y1, y2, height;

      x1 = tick_to_coord (self, start);
      x2 = tick_to_coord (self, end);

      y1 = note_to_coord (self, info->half_tone, info->octave, &height);
      y2 = y1 + height - 1;
      gdk_draw_line (window, dark_gc, x1, y2, x2, y2);
      gdk_draw_line (window, dark_gc, x2, y1, x2, y2);
      gdk_draw_rectangle (window, self->color_gc[info->half_tone], TRUE, x1, y1, MAX (x2 - x1, 1), MAX (y2 - y1, 1));
      if (y2 - y1 >= 3)	/* work for zoom to micro size */
	{
	  if (0) /* skip this with white canvas */
	    {
	      gdk_draw_line (window, light_gc, x1, y1, x2, y1);
	      gdk_draw_line (window, light_gc, x1, y1, x1, y2);
	    }
	}
      bsw_note_description_free (info);
    }
  if (iter)
    bsw_viter_free (iter);
}

static void
bst_piano_roll_overlap_grow_hpanel_area (BstPianoRoll *self,
					 GdkRectangle *area)
{
  gint i, x = area->x, xbound = x + area->width;

  /* grow hpanel exposes by surrounding tacts */
  i = coord_to_tick (self, x, FALSE);
  i /= self->ppqn * self->qnpt;
  i -= 1;		/* fudge 1 tact to the left */
  i *= self->ppqn * self->qnpt;
  x = tick_to_coord (self, i);
  i = coord_to_tick (self, xbound + 1, TRUE);
  i /= self->ppqn * self->qnpt;
  i += 1;		/* fudge 1 tact to the right */
  i *= self->ppqn * self->qnpt;
  xbound = tick_to_coord (self, i);

  area->x = x;
  area->width = xbound - area->x;
}

static void
bst_piano_roll_draw_hpanel (BstPianoRoll *self,
			    gint	  x,
			    gint	  xbound)
{
  GdkFont *font = gtk_style_get_font (STYLE (self));
  GdkGC *fg_gc = STYLE (self)->fg_gc[GTK_WIDGET_IS_SENSITIVE (self) ? GTK_STATE_SELECTED : GTK_STATE_INSENSITIVE];
  gint text_y = HPANEL_HEIGHT (self) - (HPANEL_HEIGHT (self) - gdk_string_height (font, "0123456789:")) / 2;
  gchar buffer[64];
  gint i;

  for (i = x; i < xbound; i++)
    {
      guint next_pixel, width;

      /* drawing qnote numbers is not of much use if we can't even draw
       * the qnote quarter grid, so we special case draw_qqn_grid here
       */

      if (coord_check_crossing (self, i, CROSSING_TACT))
	{
	  guint tact = coord_to_tick (self, i, TRUE) + 1;

	  tact /= (self->ppqn * self->qnpt);
	  next_pixel = tick_to_coord (self, (tact + 1) * (self->ppqn * self->qnpt));

	  g_snprintf (buffer, 64, "%u", tact + 1);

	  /* draw this tact if there's enough space */
	  width = gdk_string_width (font, buffer);
	  if (i + width / 2 < (i + next_pixel) / 2)
	    gdk_draw_string (self->hpanel, font, fg_gc,
			     i - width / 2, text_y,
			     buffer);
	}
      else if (self->draw_qqn_grid && coord_check_crossing (self, i, CROSSING_QNOTE))
	{
          guint tact = coord_to_tick (self, i, TRUE) + 1, qn = tact;

	  tact /= (self->ppqn * self->qnpt);
	  qn /= self->ppqn;
	  next_pixel = tick_to_coord (self, (qn + 1) * self->ppqn);

	  g_snprintf (buffer, 64, ":%u", /* tact + 1, */ qn % self->qnpt + 1);

	  /* draw this tact if there's enough space */
	  width = gdk_string_width (font, buffer);
	  if (i + width < (i + next_pixel) / 2)		/* don't half width, leave some more space */
	    gdk_draw_string (self->hpanel, font, fg_gc,
			     i - width / 2, text_y,
			     buffer);
	}
    }
}

static void
bst_piano_roll_draw_focus (BstPianoRoll *self)
{
  if (GTK_WIDGET_DRAWABLE (self))
    {
    }
}

static gboolean
bst_piano_roll_expose (GtkWidget      *widget,
		       GdkEventExpose *event)
{
  BstPianoRoll *self = BST_PIANO_ROLL (widget);
  GdkRectangle area = event->area;
  
  /* with gtk_widget_set_double_buffered (self, FALSE) in init and
   * with gdk_window_begin_paint_region()/gdk_window_end_paint()
   * around our redraw functions, we can decide on our own on what
   * windows we want double buffering.
   */
  if (event->window == widget->window)
    {
      gdk_window_begin_paint_rect (event->window, &area);
      // gdk_window_clear_area (widget->window, area.x, area.y, area.width, area.height);
      gtk_draw_shadow (widget->style, widget->window,
		       GTK_WIDGET_STATE (self), GTK_SHADOW_IN,
		       0, 0,
		       widget->allocation.width,
		       widget->allocation.height);
      gdk_window_end_paint (event->window);
    }
  else if (event->window == self->vpanel)
    {
      bst_piano_roll_overlap_grow_vpanel_area (self, &area);
      gdk_window_begin_paint_rect (event->window, &area);
      bst_piano_roll_overlap_grow_vpanel_area (self, &area);
      bst_piano_roll_draw_vpanel (self, area.y, area.y + area.height);
      gdk_window_end_paint (event->window);
    }
  else if (event->window == self->hpanel)
    {
      bst_piano_roll_overlap_grow_hpanel_area (self, &area);
      gdk_window_begin_paint_rect (event->window, &area);
      bst_piano_roll_overlap_grow_hpanel_area (self, &area);
      bst_piano_roll_draw_hpanel (self, area.x, area.x + area.width);
      gdk_window_end_paint (event->window);
    }
  else if (event->window == self->canvas)
    {
      gdk_window_begin_paint_rect (event->window, &area);
      // gdk_window_clear_area (widget->window, area.x, area.y, area.width, area.height);
      bst_piano_roll_draw_canvas (self, area.x, area.y, area.x + area.width, area.y + area.height);
      gdk_window_end_paint (event->window);
    }
  return FALSE;
}

static void
piano_roll_queue_expose (BstPianoRoll *self,
			 GdkWindow    *window,
			 guint	       note,
			 gint	       octave,
			 guint	       tick_start,
			 guint	       tick_end)
{
  gint x1 = tick_to_coord (self, tick_start);
  gint x2 = tick_to_coord (self, tick_end);
  gint height, y1 = note_to_coord (self, note, octave, &height);
  GdkRectangle area;

  area.x = x1 - 3;		/* add fudge */
  area.y = y1;
  area.width = x2 - x1 + 3 + 3;	/* add fudge */
  area.height = height;
  if (window == self->vpanel)
    {
      area.x = 0;
      area.width = VPANEL_WIDTH (self);
    }
  else if (window == self->hpanel)
    {
      area.y = 0;
      area.height = HPANEL_HEIGHT (self);
    }
  gdk_window_invalidate_rect (window, &area, TRUE);
}

static void
piano_roll_adjustment_changed (BstPianoRoll *self)
{
}

static void
piano_roll_adjustment_value_changed (BstPianoRoll  *self,
				     GtkAdjustment *adjustment)
{
  if (adjustment == self->hadjustment)
    {
      gint x = self->x_offset, diff;

      self->x_offset = ticks_to_pixel (self, adjustment->value);
      diff = x - self->x_offset;
      if (diff && GTK_WIDGET_DRAWABLE (self))
	{
	  GdkRectangle area = { 0, };

	  gdk_window_scroll (self->hpanel, diff, 0);
	  gdk_window_scroll (self->canvas, diff, 0);
	  area.x = diff < 0 ? CANVAS_WIDTH (self) + diff : 0;
	  area.y = 0;
	  area.width = ABS (diff);
	  area.height = CANVAS_HEIGHT (self);
	  gdk_window_invalidate_rect (self->canvas, &area, TRUE);
	  area.x = diff < 0 ? HPANEL_WIDTH (self) + diff : 0;
	  area.y = 0;
	  area.width = ABS (diff);
	  area.height = HPANEL_HEIGHT (self);
	  gdk_window_invalidate_rect (self->hpanel, &area, TRUE);
	}
    }
  if (adjustment == self->vadjustment)
    {
      gint y = self->y_offset, diff;

      self->y_offset = adjustment->value;
      diff = y - self->y_offset;
      if (diff && GTK_WIDGET_DRAWABLE (self))
	{
	  GdkRectangle area = { 0, };

	  gdk_window_scroll (self->vpanel, 0, diff);
	  gdk_window_scroll (self->canvas, 0, diff);
	  area.x = 0;
	  area.y = diff < 0 ? VPANEL_HEIGHT (self) + diff : 0;
	  area.width = VPANEL_WIDTH (self);
	  area.height = ABS (diff);
	  gdk_window_invalidate_rect (self->vpanel, &area, TRUE);
	  area.x = 0;
	  area.y = diff < 0 ? CANVAS_HEIGHT (self) + diff : 0;
	  area.width = CANVAS_WIDTH (self);
	  area.height = ABS (diff);
	  gdk_window_invalidate_rect (self->canvas, &area, TRUE);
	}
    }
}

static void
piano_roll_update_adjustments (BstPianoRoll *self,
			       gboolean      hadj,
			       gboolean      vadj)
{
  gdouble hv = self->hadjustment->value;
  gdouble vv = self->vadjustment->value;

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
  if (vadj)
    {
      self->vadjustment->lower = 0;
      self->vadjustment->upper = OCTAVE_HEIGHT (self) * N_OCTAVES (self);
      self->vadjustment->page_size = CANVAS_HEIGHT (self);
      self->vadjustment->page_increment = self->vadjustment->page_size / 2;
      self->vadjustment->step_increment = OCTAVE_HEIGHT (self) / 7;
      self->vadjustment->value = CLAMP (self->vadjustment->value,
					self->vadjustment->lower,
					self->vadjustment->upper - self->vadjustment->page_size);
      gtk_adjustment_changed (self->vadjustment);
    }
  if (hv != self->hadjustment->value)
    gtk_adjustment_value_changed (self->hadjustment);
  if (vv != self->vadjustment->value)
    gtk_adjustment_value_changed (self->vadjustment);
}

void
bst_piano_roll_adjust_scroll_area (BstPianoRoll *self)
{
}

static void
bst_piano_roll_hsetup (BstPianoRoll *self,
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
      old_max_ticks != self->max_ticks ||	// FIME: shouldn't always cause a redraw
      old_hzoom != self->hzoom)
    {
      if (self->hadjustment)
	{
	  self->x_offset = ticks_to_pixel (self, self->hadjustment->value);
	  piano_roll_update_adjustments (self, TRUE, FALSE);
	}
      self->draw_qn_grid = ticks_to_pixel (self, self->ppqn) >= 3;
      self->draw_qqn_grid = ticks_to_pixel (self, self->ppqn / 4) >= 5;
      gtk_widget_queue_draw (GTK_WIDGET (self));
    }
}

gfloat
bst_piano_roll_set_hzoom (BstPianoRoll *self,
			  gfloat        hzoom)
{
  g_return_val_if_fail (BST_IS_PIANO_ROLL (self), 0);

  bst_piano_roll_hsetup (self, self->ppqn, self->qnpt, self->max_ticks, hzoom);

  return self->hzoom;
}

static gboolean
bst_piano_roll_focus_in (GtkWidget     *widget,
			 GdkEventFocus *event)
{
  BstPianoRoll *self = BST_PIANO_ROLL (widget);

  GTK_WIDGET_SET_FLAGS (self, GTK_HAS_FOCUS);
  bst_piano_roll_draw_focus (self);
  return TRUE;
}

static gboolean
bst_piano_roll_focus_out (GtkWidget	*widget,
			  GdkEventFocus *event)
{
  BstPianoRoll *self = BST_PIANO_ROLL (widget);

  GTK_WIDGET_UNSET_FLAGS (self, GTK_HAS_FOCUS);
  bst_piano_roll_draw_focus (self);
  return TRUE;
}

static gboolean
bst_piano_roll_button_press (GtkWidget	    *widget,
			     GdkEventButton *event)
{
  BstPianoRoll *self = BST_PIANO_ROLL (widget);
  gboolean handled = FALSE;
  
  if (!GTK_WIDGET_HAS_FOCUS (widget))
    gtk_widget_grab_focus (widget);

  if (self->drag_button == 0)
    {
      handled = TRUE;
      if (event->window == self->canvas)
	{
	  BswNoteDescription *desc;
	  NoteInfo info;

	  self->drag_button = event->button;
	  self->drag_type = BST_PIANO_ROLL_POINTER_MOTION;
	  self->drag_tick0 = coord_to_tick (self, event->x, FALSE);
	  coord_to_note (self, event->y, &info);
	  desc = bsw_server_construct_note (BSW_SERVER, info.note, info.octave, 0);
	  self->drag_freq0 = desc->freq;
	  bsw_note_description_free (desc);
	  g_signal_emit (self, signal_canvas_press, 0, self->drag_button, self->drag_tick0, self->drag_freq0);
	}
    }

  return handled;
}

static gboolean
bst_piano_roll_motion (GtkWidget      *widget,
		       GdkEventMotion *event)
{
  BstPianoRoll *self = BST_PIANO_ROLL (widget);
  gboolean handled = FALSE;
  
  if (self->drag_button)
    {
      handled = TRUE;
      if (event->window == self->canvas && self->drag_type == BST_PIANO_ROLL_POINTER_MOTION)
	{
          BswNoteDescription *desc;
	  NoteInfo info;
	  guint tick = coord_to_tick (self, event->x, FALSE);
	  gfloat freq;
	  
	  coord_to_note (self, event->y, &info);
	  desc = bsw_server_construct_note (BSW_SERVER, info.note, info.octave, 0);
	  freq = desc->freq;
	  bsw_note_description_free (desc);
	  g_signal_emit (self, signal_canvas_motion, 0, self->drag_button, tick, freq);
	}
    }
  if (event->window == self->vpanel)
    {
      NoteInfo info;
      coord_to_note (self, event->y, &info);
      g_print ("motion: note=%d octave=%d note_step=%d step_frac=%d\n",
	       info.note, info.octave, info.key, info.key_frac);
    }
  
  return handled;
}

static gboolean
bst_piano_roll_button_release (GtkWidget      *widget,
			       GdkEventButton *event)
{
  BstPianoRoll *self = BST_PIANO_ROLL (widget);
  gboolean handled = FALSE;

  if (self->drag_button == event->button)
    {
      self->drag_button = 0;
      handled = TRUE;
      if (event->window == self->canvas && self->drag_type >= BST_PIANO_ROLL_POINTER_RELEASE)
	{
	  BswNoteDescription *desc;
	  NoteInfo info;
	  guint tick = coord_to_tick (self, event->x, FALSE);
	  gfloat freq;

	  coord_to_note (self, event->y, &info);
	  desc = bsw_server_construct_note (BSW_SERVER, info.note, info.octave, 0);
	  freq = desc->freq;
	  bsw_note_description_free (desc);
	  g_signal_emit (self, signal_canvas_release, 0, event->button, tick, freq);
	}
    }

  return handled;
}

static gboolean
bst_piano_roll_key_press (GtkWidget   *widget,
			  GdkEventKey *event)
{
  // BstPianoRoll *self = BST_PIANO_ROLL (widget);
  gboolean handled = FALSE;

  return handled;
}

static gboolean
bst_piano_roll_key_release (GtkWidget   *widget,
			    GdkEventKey *event)
{
  // BstPianoRoll *self = BST_PIANO_ROLL (widget);
  gboolean handled = FALSE;

  return handled;
}

static void
piano_roll_update (BstPianoRoll *self,
		   guint         tick,
		   guint         duration,
		   gfloat        min_freq,
		   gfloat        max_freq)
{
  gint note, octave;

  g_return_if_fail (duration > 0);

  // FIXME: freq->note/octave translation
  for (note = 0; note < 12; note++)
    for (octave = self->min_octave; octave <= self->max_octave; octave++)
      piano_roll_queue_expose (self, self->canvas, note, octave, tick, tick + duration - 1);
}

static void
piano_roll_unset_proxy (BstPianoRoll *self)
{
  bst_piano_roll_set_proxy (self, 0);
}

void
bst_piano_roll_set_proxy (BstPianoRoll *self,
			  BswProxy      proxy)
{
  g_return_if_fail (BST_IS_PIANO_ROLL (self));
  if (proxy)
    {
      g_return_if_fail (BSW_IS_ITEM (proxy));
      g_return_if_fail (bsw_item_get_project (proxy) != 0);
    }

  if (self->proxy)
    {
      bsw_proxy_disconnect (self->proxy,
			    "any_signal", piano_roll_unset_proxy, self,
			    "any_signal", piano_roll_update, self,
			    NULL);
      bsw_item_unuse (self->proxy);
    }
  self->proxy = proxy;
  if (self->proxy)
    {
      bsw_item_use (self->proxy);
      bsw_proxy_connect (self->proxy,
			 "swapped_signal::set_parent", piano_roll_unset_proxy, self,
			 // "swapped_signal::notify::name", piano_roll_update_name, self,
			 "swapped_signal::range-changed", piano_roll_update, self,
			 NULL);
      self->min_octave = bsw_part_get_min_octave (self->proxy);
      self->max_octave = bsw_part_get_max_octave (self->proxy);
    }
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

void
bst_piano_roll_set_pointer (BstPianoRoll           *self,
			    BstPianoRollPointerType type)
{
  g_return_if_fail (BST_IS_PIANO_ROLL (self));
  g_return_if_fail (type >= BST_PIANO_ROLL_POINTER_IGNORE && type <= BST_PIANO_ROLL_POINTER_MOTION);

  self->drag_type = type;
}

void
bst_piano_roll_set_drag_data1 (BstPianoRoll *self,
			       guint         tick,
                               guint         duration,
			       gfloat        freq)
{
  g_return_if_fail (BST_IS_PIANO_ROLL (self));

  self->drag_tick1 = tick;
  self->drag_duration1 = duration;
  self->drag_freq1 = freq;
}

void
bst_piano_roll_set_drag_data2 (BstPianoRoll *self,
			       guint         tick,
			       guint	     duration,
			       gfloat        freq)
{
  g_return_if_fail (BST_IS_PIANO_ROLL (self));

  self->drag_tick2 = tick;
  self->drag_duration2 = duration;
  self->drag_freq2 = freq;
}

guint
bst_piano_roll_get_drag_data0 (BstPianoRoll *self,
			       guint        *tick,
			       gfloat       *freq)
{
  g_return_val_if_fail (BST_IS_PIANO_ROLL (self), 0);

  if (tick)
    *tick = self->drag_tick0;
  if (freq)
    *freq = self->drag_freq0;
  return self->drag_button;
}

guint
bst_piano_roll_get_drag_data1 (BstPianoRoll *self,
			       guint        *tick,
			       guint	    *duration,
			       gfloat       *freq)
{
  g_return_val_if_fail (BST_IS_PIANO_ROLL (self), 0);

  if (tick)
    *tick = self->drag_tick1;
  if (duration)
    *duration = self->drag_duration1;
  if (freq)
    *freq = self->drag_freq1;
  return self->drag_button;
}

guint
bst_piano_roll_get_drag_data2 (BstPianoRoll *self,
			       guint        *tick,
			       guint	    *duration,
			       gfloat       *freq)
{
  g_return_val_if_fail (BST_IS_PIANO_ROLL (self), 0);

  if (tick)
    *tick = self->drag_tick2;
  if (duration)
    *duration = self->drag_duration2;
  if (freq)
    *freq = self->drag_freq2;
  return self->drag_button;
}

void
bst_piano_roll_set_canvas_cursor (BstPianoRoll *self,
				  GdkCursorType cursor_type)
{
  GdkCursor *cursor;

  g_return_if_fail (BST_IS_PIANO_ROLL (self));

  if (cursor_type != self->canvas_cursor)
    {
      self->canvas_cursor = cursor_type;
      if (GTK_WIDGET_REALIZED (self))
	{
	  cursor = gdk_cursor_new (self->canvas_cursor);
	  gdk_window_set_cursor (self->canvas, cursor);
	  gdk_cursor_unref (cursor);
	}
    }
}

void
bst_piano_roll_set_vpanel_cursor (BstPianoRoll *self,
				  GdkCursorType cursor_type)
{
  GdkCursor *cursor;

  g_return_if_fail (BST_IS_PIANO_ROLL (self));

  if (cursor_type != self->vpanel_cursor)
    {
      self->vpanel_cursor = cursor_type;
      if (GTK_WIDGET_REALIZED (self))
	{
	  cursor = gdk_cursor_new (self->vpanel_cursor);
	  gdk_window_set_cursor (self->vpanel, cursor);
	  gdk_cursor_unref (cursor);
	}
    }
}

void
bst_piano_roll_set_hpanel_cursor (BstPianoRoll *self,
				  GdkCursorType cursor_type)
{
  GdkCursor *cursor;

  g_return_if_fail (BST_IS_PIANO_ROLL (self));

  if (cursor_type != self->hpanel_cursor)
    {
      self->hpanel_cursor = cursor_type;
      if (GTK_WIDGET_REALIZED (self))
	{
	  cursor = gdk_cursor_new (self->hpanel_cursor);
	  gdk_window_set_cursor (self->hpanel, cursor);
	  gdk_cursor_unref (cursor);
	}
    }
}
