/* BEAST - Bedevilled Audio System
 * Copyright (C) 1999 Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bstzoomedwindow.h"
#include <gtk/gtktogglebutton.h>
#include <gtk/gtksignal.h>


/* --- signals --- */
enum {
  SIGNAL_ZOOM,
  SIGNAL_LAST
};
typedef gboolean (*SignalZoom) (GtkObject *object,
				gboolean   zoom_in,
				gpointer   func_data);


/* --- prototypes --- */
static void	bst_zoomed_window_class_init	(BstZoomedWindowClass	*klass);
static void	bst_zoomed_window_init		(BstZoomedWindow	*zoomed_window);
static void	bst_zoomed_window_destroy	(GtkObject		*object);
static void	bst_zoomed_window_finalize	(GtkObject		*object);
static void	bst_zoomed_window_map		(GtkWidget		*widget);
static void	bst_zoomed_window_unmap		(GtkWidget		*widget);
static void     bst_zoomed_window_size_request  (GtkWidget              *widget,
						 GtkRequisition         *requisition);
static void     bst_zoomed_window_size_allocate (GtkWidget              *widget,
						 GtkAllocation          *allocation);
static void	bst_zoomed_window_draw		(GtkWidget		*widget,
						 GdkRectangle           *area);
static void	bst_zoomed_window_forall	(GtkContainer           *container,
						 gboolean                include_internals,
						 GtkCallback             callback,
						 gpointer                callback_data);
static void	bst_zoomed_window_clicked	(BstZoomedWindow	*zoomed_window);


/* --- static variables --- */
static gpointer		     parent_class = NULL;
static BstZoomedWindowClass *bst_zoomed_window_class = NULL;
static guint                 zoomed_window_signals[SIGNAL_LAST] = { 0 };


/* --- functions --- */
GtkType
bst_zoomed_window_get_type (void)
{
  static GtkType zoomed_window_type = 0;
  
  if (!zoomed_window_type)
    {
      GtkTypeInfo zoomed_window_info =
      {
	"BstZoomedWindow",
	sizeof (BstZoomedWindow),
	sizeof (BstZoomedWindowClass),
	(GtkClassInitFunc) bst_zoomed_window_class_init,
	(GtkObjectInitFunc) bst_zoomed_window_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      zoomed_window_type = gtk_type_unique (GTK_TYPE_SCROLLED_WINDOW, &zoomed_window_info);
    }
  
  return zoomed_window_type;
}

static void
bst_zoomed_window_marshal_zoom (GtkObject    *object,
				GtkSignalFunc func,
				gpointer      func_data,
				GtkArg       *args)
{
  SignalZoom rfunc = (SignalZoom) func;
  gboolean *return_val;

  return_val = GTK_RETLOC_BOOL (args[1]);
  *return_val = (*rfunc) (object,
			  GTK_VALUE_BOOL (args[0]),
			  func_data);
}

static void
bst_zoomed_window_class_init (BstZoomedWindowClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;
  
  object_class = GTK_OBJECT_CLASS (class);
  widget_class = GTK_WIDGET_CLASS (class);
  container_class = GTK_CONTAINER_CLASS (class);
  
  bst_zoomed_window_class = class;
  parent_class = gtk_type_class (GTK_TYPE_SCROLLED_WINDOW);
  
  object_class->destroy = bst_zoomed_window_destroy;
  object_class->finalize = bst_zoomed_window_finalize;
  
  widget_class->map = bst_zoomed_window_map;
  widget_class->unmap = bst_zoomed_window_unmap;
  widget_class->size_request = bst_zoomed_window_size_request;
  widget_class->size_allocate = bst_zoomed_window_size_allocate;
  widget_class->draw = bst_zoomed_window_draw;
  
  container_class->forall = bst_zoomed_window_forall;
  
  class->zoom = NULL;

  zoomed_window_signals[SIGNAL_ZOOM] =
    gtk_signal_new ("zoom",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (BstZoomedWindowClass, zoom),
		    bst_zoomed_window_marshal_zoom,
		    GTK_TYPE_BOOL,
		    1, GTK_TYPE_BOOL);
  gtk_object_class_add_signals (object_class, zoomed_window_signals, SIGNAL_LAST);
}

static void
bst_zoomed_window_init (BstZoomedWindow *zoomed_window)
{
  GtkScrolledWindow *scrolled_window = GTK_SCROLLED_WINDOW (zoomed_window);

  /* default construct */
  gtk_scrolled_window_set_hadjustment (scrolled_window, NULL);
  gtk_scrolled_window_set_vadjustment (scrolled_window, NULL);
  
  gtk_widget_push_composite_child ();
  zoomed_window->toggle_button = gtk_widget_new (GTK_TYPE_TOGGLE_BUTTON,
						 "visible", TRUE,
						 "can_focus", FALSE,
						 "object_signal::clicked", bst_zoomed_window_clicked, zoomed_window,
						 NULL);
  gtk_widget_set_parent (zoomed_window->toggle_button, GTK_WIDGET (zoomed_window));
  gtk_widget_ref (zoomed_window->toggle_button);
  gtk_widget_pop_composite_child ();
}

static void
bst_zoomed_window_destroy (GtkObject *object)
{
  BstZoomedWindow *zoomed_window = BST_ZOOMED_WINDOW (object);

  gtk_widget_unparent (zoomed_window->toggle_button);
  gtk_widget_destroy (zoomed_window->toggle_button);
  
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bst_zoomed_window_finalize (GtkObject *object)
{
  BstZoomedWindow *zoomed_window = BST_ZOOMED_WINDOW (object);
  
  gtk_widget_unref (zoomed_window->toggle_button);

  GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bst_zoomed_window_map (GtkWidget *widget)
{
  BstZoomedWindow *zoomed_window = BST_ZOOMED_WINDOW (widget);
  
  /* chain parent class handler to map self and children */
  GTK_WIDGET_CLASS (parent_class)->map (widget);

  if (GTK_WIDGET_VISIBLE (zoomed_window->toggle_button) &&
      !GTK_WIDGET_MAPPED (zoomed_window->toggle_button))
    gtk_widget_map (zoomed_window->toggle_button);
}

static void
bst_zoomed_window_unmap	(GtkWidget *widget)
{
  BstZoomedWindow *zoomed_window = BST_ZOOMED_WINDOW (widget);
  
  /* chain parent class handler to unmap self and children */
  GTK_WIDGET_CLASS (parent_class)->unmap (widget);

  if (GTK_WIDGET_MAPPED (zoomed_window->toggle_button))
    gtk_widget_unmap (zoomed_window->toggle_button);
}

static void
bst_zoomed_window_size_request (GtkWidget      *widget,
				GtkRequisition *requisition)
{
  BstZoomedWindow *zoomed_window = BST_ZOOMED_WINDOW (widget);

  gtk_widget_size_request (zoomed_window->toggle_button, NULL);

  /* chain parent class handler for requisition */
  GTK_WIDGET_CLASS (parent_class)->size_request (widget, requisition);
}

static void
bst_zoomed_window_size_allocate (GtkWidget     *widget,
				 GtkAllocation *allocation)
{
  GtkScrolledWindow *scrolled_window = GTK_SCROLLED_WINDOW (widget);
  BstZoomedWindow *zoomed_window = BST_ZOOMED_WINDOW (widget);

  /* chain parent class handler to layout children */
  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  if (scrolled_window->hscrollbar && GTK_WIDGET_VISIBLE (scrolled_window->hscrollbar) &&
      scrolled_window->vscrollbar && GTK_WIDGET_VISIBLE (scrolled_window->vscrollbar))
    {
      GtkAllocation child_allocation;

      child_allocation.x = scrolled_window->vscrollbar->allocation.x;
      child_allocation.y = scrolled_window->hscrollbar->allocation.y;
      child_allocation.width = scrolled_window->vscrollbar->allocation.width;
      child_allocation.height = scrolled_window->hscrollbar->allocation.height;

      gtk_widget_size_allocate (zoomed_window->toggle_button, &child_allocation);
      gtk_widget_show (zoomed_window->toggle_button);
    }
  else
    gtk_widget_hide (zoomed_window->toggle_button);
}

static void
bst_zoomed_window_draw (GtkWidget    *widget,
			GdkRectangle *area)
{
  BstZoomedWindow *zoomed_window = BST_ZOOMED_WINDOW (widget);
  GdkRectangle child_area;
  
  /* chain parent class handler to map self and children */
  GTK_WIDGET_CLASS (parent_class)->draw (widget, area);

  if (GTK_WIDGET_DRAWABLE (zoomed_window->toggle_button) &&
      gtk_widget_intersect (zoomed_window->toggle_button, area, &child_area))
    gtk_widget_draw (zoomed_window->toggle_button, &child_area);
}

static void
bst_zoomed_window_forall (GtkContainer *container,
			  gboolean      include_internals,
			  GtkCallback   callback,
			  gpointer      callback_data)
{
  GTK_CONTAINER_CLASS (parent_class)->forall (container,
					      include_internals,
					      callback,
					      callback_data);
  if (include_internals)
    {
      BstZoomedWindow *zoomed_window = BST_ZOOMED_WINDOW (container);

      callback (zoomed_window->toggle_button, callback_data);
    }
}

static void
bst_zoomed_window_clicked (BstZoomedWindow *zoomed_window)
{
  gboolean stay_active = GTK_TOGGLE_BUTTON (zoomed_window->toggle_button)->active;

  gtk_widget_ref (GTK_WIDGET (zoomed_window));
  gtk_signal_emit (GTK_OBJECT (zoomed_window),
		   zoomed_window_signals[SIGNAL_ZOOM],
		   stay_active,
		   &stay_active);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (zoomed_window->toggle_button), stay_active);
  gtk_widget_unref (GTK_WIDGET (zoomed_window));
}
