/* BEAST - Better Audio System
 * Copyright (C) 1999-2002 Tim Janik and Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "bstutils.hh"	/* for marshallers */
#include "bstzoomedwindow.hh"

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
static void	bst_zoomed_window_finalize	(GObject		*object);
static void	bst_zoomed_window_map		(GtkWidget		*widget);
static void	bst_zoomed_window_unmap		(GtkWidget		*widget);
static void     bst_zoomed_window_size_request  (GtkWidget              *widget,
						 GtkRequisition         *requisition);
static void     bst_zoomed_window_size_allocate (GtkWidget              *widget,
						 GtkAllocation          *allocation);
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
bst_zoomed_window_class_init (BstZoomedWindowClass *klass)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);
  
  bst_zoomed_window_class = klass;
  parent_class = g_type_class_peek_parent (klass);
  
  G_OBJECT_CLASS (object_class)->finalize = bst_zoomed_window_finalize;

  object_class->destroy = bst_zoomed_window_destroy;
  
  widget_class->map = bst_zoomed_window_map;
  widget_class->unmap = bst_zoomed_window_unmap;
  widget_class->size_request = bst_zoomed_window_size_request;
  widget_class->size_allocate = bst_zoomed_window_size_allocate;
  
  container_class->forall = bst_zoomed_window_forall;
  
  klass->zoom = NULL;

  zoomed_window_signals[SIGNAL_ZOOM] =
    gtk_signal_new ("zoom",
		    GTK_RUN_LAST,
		    GTK_CLASS_TYPE (object_class),
		    GTK_SIGNAL_OFFSET (BstZoomedWindowClass, zoom),
		    bst_marshal_BOOL__BOOL,
		    GTK_TYPE_BOOL,
		    1, GTK_TYPE_BOOL);
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
                                                 NULL);
  g_object_connect (zoomed_window->toggle_button,
                    "swapped_signal::clicked", bst_zoomed_window_clicked, zoomed_window,
                    NULL);
  gtk_widget_set_parent (zoomed_window->toggle_button, GTK_WIDGET (zoomed_window));
  gtk_widget_pop_composite_child ();
}

static void
bst_zoomed_window_destroy (GtkObject *object)
{
  BstZoomedWindow *zoomed_window = BST_ZOOMED_WINDOW (object);

  if (zoomed_window->toggle_button)
    {
      gtk_widget_unparent (zoomed_window->toggle_button);
      zoomed_window->toggle_button = NULL;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bst_zoomed_window_finalize (GObject *object)
{
  /* BstZoomedWindow *zoomed_window = BST_ZOOMED_WINDOW (object); */

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bst_zoomed_window_map (GtkWidget *widget)
{
  BstZoomedWindow *zoomed_window = BST_ZOOMED_WINDOW (widget);
  
  /* chain parent class handler to map self and children */
  GTK_WIDGET_CLASS (parent_class)->map (widget);
  
  if (zoomed_window->toggle_button &&
      GTK_WIDGET_VISIBLE (zoomed_window->toggle_button) &&
      !GTK_WIDGET_MAPPED (zoomed_window->toggle_button))
    gtk_widget_map (zoomed_window->toggle_button);
}

static void
bst_zoomed_window_unmap	(GtkWidget *widget)
{
  BstZoomedWindow *zoomed_window = BST_ZOOMED_WINDOW (widget);
  
  /* chain parent class handler to unmap self and children */
  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
  
  if (zoomed_window->toggle_button &&
      GTK_WIDGET_MAPPED (zoomed_window->toggle_button))
    gtk_widget_unmap (zoomed_window->toggle_button);
}

static void
bst_zoomed_window_size_request (GtkWidget      *widget,
				GtkRequisition *requisition)
{
  BstZoomedWindow *zoomed_window = BST_ZOOMED_WINDOW (widget);

  if (zoomed_window->toggle_button)
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

  if (zoomed_window->toggle_button)
    {
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
}

static void
bst_zoomed_window_forall (GtkContainer *container,
			  gboolean      include_internals,
			  GtkCallback   callback,
			  gpointer      callback_data)
{
  BstZoomedWindow *zoomed_window = BST_ZOOMED_WINDOW (container);

  GTK_CONTAINER_CLASS (parent_class)->forall (container,
					      include_internals,
					      callback,
					      callback_data);
  if (include_internals && zoomed_window->toggle_button)
    callback (zoomed_window->toggle_button, callback_data);
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
