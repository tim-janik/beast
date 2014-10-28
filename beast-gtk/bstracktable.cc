// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstracktable.hh"
#if 0
#include "bstrackitem.hh"
#endif
#include "bstutils.hh"
#include <gtk/gtkalignment.h>
#include <gtk/gtklabel.h>

typedef enum
{
  EXPOSE_AREA,
  FOCUS_AREA,
  DRAW_ACTIVE,
  DRAW_INACTIVE,
} AreaAction;


/* --- prototypes --- */
static void		bst_rack_table_class_init	(BstRackTableClass	*klass);
static void		bst_rack_table_init		(BstRackTable		*rtable);
static void		bst_rack_table_destroy		(GtkObject		*object);
static void		bst_rack_table_finalize		(GObject		*object);
static void		bst_rack_table_style_set	(GtkWidget		*widget,
							 GtkStyle		*previous_style);
static void     	bst_rack_table_size_request	(GtkWidget              *widget,
							 GtkRequisition         *requisition);
static void     	bst_rack_table_size_allocate	(GtkWidget              *widget,
							 GtkAllocation          *allocation);
static void		bst_rack_table_realize		(GtkWidget		*widget);
static void		bst_rack_table_unrealize	(GtkWidget		*widget);
static void		bst_rack_table_map		(GtkWidget		*widget);
static void		bst_rack_table_unmap		(GtkWidget		*widget);
static gint     	bst_rack_table_button_press     (GtkWidget		*widget,
							 GdkEventButton		*event);
static gint     	bst_rack_table_button_release   (GtkWidget		*widget,
							 GdkEventButton		*event);
static gint		bst_rack_table_motion_notify	(GtkWidget		*widget,
							 GdkEventMotion		*event);
static gint		bst_rack_table_expose		(GtkWidget		*widget,
							 GdkEventExpose		*event);
static void		bst_rack_table_add		(GtkContainer		*container,
							 GtkWidget		*child);
static void		bst_rack_table_remove		(GtkContainer		*container,
							 GtkWidget		*child);
static gboolean		bst_rack_table_iwindow_translate(BstRackTable		*rtable,
							 gint			 x,
							 gint			 y,
							 guint			*hcell,
							 guint			*vcell);
static GtkWidget*	bst_rack_table_find_child	(BstRackTable		*rtable,
							 guint			 hcell,
							 guint			 vcell);
static void		bst_rack_table_on_area		(BstRackTable		*rtable,
							 AreaAction		 action,
							 guint			 hcell1,
							 guint			 vcell1,
							 guint			 hspan,
							 guint			 vspan);
static void		bst_rtable_update_child_map	(BstRackTable		*rtable);
static void		widget_reparent			(GtkWidget		*child,
							 GtkWidget		*parent);
static void		rtable_abort_drag		(BstRackTable		*rtable,
							 guint32		 etime);


/* --- static variables --- */
static gpointer	parent_class = NULL;
static GQuark   quark_rack_info = 0;
static guint	signal_edit_mode_changed = 0;


/* --- functions --- */
GtkType
bst_rack_table_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info = {
	sizeof (BstRackTableClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) bst_rack_table_class_init,
	NULL,   /* class_finalize */
	NULL,   /* class_data */
	sizeof (BstRackTable),
	0,      /* n_preallocs */
	(GInstanceInitFunc) bst_rack_table_init,
      };

      object_type = g_type_register_static (GTK_TYPE_TABLE,
					    "BstRackTable",
					    &object_info, GTypeFlags (0));
    }

  return object_type;
}

static void
bst_rack_table_class_init (BstRackTableClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  quark_rack_info = g_quark_from_static_string ("BstRackChildInfo");

  gobject_class->finalize = bst_rack_table_finalize;

  object_class->destroy = bst_rack_table_destroy;

  widget_class->style_set = bst_rack_table_style_set;
  widget_class->size_request = bst_rack_table_size_request;
  widget_class->size_allocate = bst_rack_table_size_allocate;
  widget_class->realize = bst_rack_table_realize;
  widget_class->unrealize = bst_rack_table_unrealize;
  widget_class->map = bst_rack_table_map;
  widget_class->unmap = bst_rack_table_unmap;
  widget_class->button_press_event = bst_rack_table_button_press;
  widget_class->button_release_event = bst_rack_table_button_release;
  widget_class->motion_notify_event = bst_rack_table_motion_notify;
  widget_class->expose_event = bst_rack_table_expose;

  container_class->add = bst_rack_table_add;
  container_class->remove = bst_rack_table_remove;

  signal_edit_mode_changed = g_signal_new ("edit-mode-changed",
					   G_OBJECT_CLASS_TYPE (klass),
					   G_SIGNAL_RUN_FIRST,
					   G_STRUCT_OFFSET (BstRackTableClass, edit_mode_changed),
					   NULL, NULL,
					   bst_marshal_NONE__BOOLEAN,
					   G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
}

static void
bst_rack_table_init (BstRackTable *rtable)
{
  gtk_widget_set_redraw_on_allocate (GTK_WIDGET (rtable), TRUE);
  gtk_container_set_reallocate_redraws (GTK_CONTAINER (rtable), TRUE);
  rtable->iwindow = NULL;
  g_object_set (rtable,
		"homogeneous", TRUE,
		"column_spacing", 0,
		"row_spacing", 0,
		NULL);
  rtable->cell_request_width = 0;
  rtable->cell_request_height = 0;

  rtable->drag_window = (GtkWidget*) g_object_new (GTK_TYPE_WINDOW,
                                                   "type", GTK_WINDOW_POPUP,
                                                   "width_request", rtable->cell_request_width,
                                                   "height_request", rtable->cell_request_height,
                                                   NULL);
  if (0)
    g_object_new (GTK_TYPE_LABEL,
		"parent", rtable->drag_window,
		"label", "foo",
		"visible", TRUE,
		NULL);
}

static void
bst_rack_table_destroy (GtkObject *object)
{
  BstRackTable *rtable = BST_RACK_TABLE (object);

  rtable_abort_drag (rtable, GDK_CURRENT_TIME);

  if (rtable->drag_window)
    {
      gtk_widget_destroy (rtable->drag_window);
      rtable->drag_window = NULL;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bst_rack_table_finalize (GObject *object)
{
  BstRackTable *rtable = BST_RACK_TABLE (object);

  g_free (rtable->child_map);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bst_rack_table_realize (GtkWidget *widget)
{
  BstRackTable *rtable = BST_RACK_TABLE (widget);
  GdkWindowAttr attributes;
  gint attributes_mask;

  GTK_WIDGET_CLASS (parent_class)->realize (widget);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= (GDK_EXPOSURE_MASK |
			    GDK_BUTTON_PRESS_MASK |
			    GDK_BUTTON_RELEASE_MASK |
			    GDK_ENTER_NOTIFY_MASK |
			    GDK_LEAVE_NOTIFY_MASK |
			    GDK_POINTER_MOTION_MASK |
			    GDK_POINTER_MOTION_HINT_MASK);
  attributes_mask = GDK_WA_X | GDK_WA_Y;

  rtable->iwindow = gdk_window_new (gtk_widget_get_parent_window (widget),
				    &attributes, attributes_mask);
  gdk_window_set_user_data (rtable->iwindow, rtable);
}

static void
bst_rack_table_style_set (GtkWidget *widget,
			  GtkStyle  *previous_style)
{
  BstRackTable *rtable = BST_RACK_TABLE (widget);
  GdkFont *font = gtk_style_get_font (widget->style);
  guint i, x = 0;

  for (i = 0; i < 256; i++)
    {
      guint width = gdk_char_width (font, i);

      x = MAX (x, width);
    }
  rtable->cell_request_width = x;
  rtable->cell_request_height = font->ascent + font->descent;
}

static void
bst_rack_table_size_request (GtkWidget      *widget,
			     GtkRequisition *requisition)
{
  BstRackTable *rtable = BST_RACK_TABLE (widget);
  GtkTable *table = GTK_TABLE (rtable);
  GList *list;
  guint i, j;

  for (list = table->children; list; list = list->next)
    {
      GtkTableChild *child = (GtkTableChild*) list->data;

      if (GTK_WIDGET_VISIBLE (child->widget))
	gtk_widget_size_request (child->widget, NULL);
    }

  requisition->width = GTK_CONTAINER (rtable)->border_width * 2;
  requisition->height = GTK_CONTAINER (rtable)->border_width * 2;
  for (i = 0; i < table->ncols; i++)
    {
      table->cols[i].requisition = rtable->cell_request_width + table->cols[i].spacing;
      requisition->width += table->cols[i].requisition;
    }
  for (j = 0; j < table->nrows; j++)
    {
      table->rows[j].requisition = rtable->cell_request_height + table->rows[j].spacing;
      requisition->height += table->rows[j].requisition;
    }
}

static void
bst_rack_table_size_allocate (GtkWidget     *widget,
			      GtkAllocation *allocation)
{
  BstRackTable *rtable = BST_RACK_TABLE (widget);
  GtkTable *table = GTK_TABLE (rtable);

  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  bst_rtable_update_child_map (rtable);

  rtable->cell_width = table->cols[0].allocation;
  rtable->cell_height = table->rows[0].allocation;

  if (GTK_WIDGET_REALIZED (rtable))
    {
      gdk_window_move_resize (rtable->iwindow,
			      widget->allocation.x, widget->allocation.y,
			      widget->allocation.width, widget->allocation.height);
      gdk_window_raise (rtable->iwindow);
    }
}

static void
bst_rack_table_unrealize (GtkWidget *widget)
{
  BstRackTable *rtable = BST_RACK_TABLE (widget);

  gdk_window_set_user_data (rtable->iwindow, NULL);
  gdk_window_destroy (rtable->iwindow);
  rtable->iwindow = NULL;

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
bst_rack_table_map (GtkWidget *widget)
{
  BstRackTable *rtable = BST_RACK_TABLE (widget);

  GTK_WIDGET_CLASS (parent_class)->map (widget);

  if (rtable->edit_mode)
    gdk_window_show (rtable->iwindow);
}

static void
bst_rack_table_unmap (GtkWidget *widget)
{
  BstRackTable *rtable = BST_RACK_TABLE (widget);

  rtable_abort_drag (rtable, GDK_CURRENT_TIME);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);

  gdk_window_hide (rtable->iwindow);
}

static void
widget_reparent (GtkWidget *child,
		 GtkWidget *parent)
{
  g_object_ref (child);
  gtk_container_remove (GTK_CONTAINER (child->parent), child);
  gtk_container_add (GTK_CONTAINER (parent), child);
  g_object_unref (child);
}

static gint
bst_rack_table_button_press (GtkWidget      *widget,
			     GdkEventButton *event)
{
  BstRackTable *rtable = BST_RACK_TABLE (widget);
  GtkTable *table = GTK_TABLE (rtable);

  if (!rtable->edit_mode)
    return FALSE;

  if (event->type == GDK_BUTTON_PRESS && event->button == 2)
    {
      bool was_dragging = rtable->in_drag;
      uint h, v;

      if (bst_rack_table_iwindow_translate (rtable, event->x, event->y, &h, &v))
	rtable->child = bst_rack_table_find_child (rtable, h, v);
      rtable->in_drag = rtable->child != NULL;
      if (rtable->in_drag)
	{
	  if (!was_dragging)
	    {
	      GdkCursor *cursor = gdk_cursor_new (GDK_FLEUR);

	      rtable->in_drag_and_grabbing = gdk_pointer_grab (rtable->iwindow, FALSE,
							       (GDK_BUTTON_PRESS_MASK |
								GDK_BUTTON_RELEASE_MASK |
								GDK_ENTER_NOTIFY_MASK |
								GDK_LEAVE_NOTIFY_MASK |
								GDK_POINTER_MOTION_MASK |
								GDK_POINTER_MOTION_HINT_MASK),
							       NULL, cursor, event->time) == GDK_GRAB_SUCCESS;
	      gdk_cursor_destroy (cursor);
	    }
	  bst_rack_child_get_info (rtable->child, &rtable->drag_info);
	  rtable->xofs = event->x - GTK_CONTAINER (rtable)->border_width;
	  rtable->yofs = event->y - GTK_CONTAINER (rtable)->border_width;
	  for (int i = 0; i < rtable->drag_info.col; i++)
	    rtable->xofs -= table->cols[i].allocation + table->cols[i].spacing;
	  for (int i = 0; i < rtable->drag_info.row; i++)
	    rtable->yofs -= table->rows[i].allocation + table->rows[i].spacing;
	  widget_reparent (rtable->child, rtable->drag_window);
	  rtable->drag_col = rtable->drag_info.col;
	  rtable->drag_row = rtable->drag_info.row;
          h = table->cols[0].allocation + table->cols[0].spacing;
          v = table->rows[0].allocation + table->rows[0].spacing;
	  h *= rtable->drag_info.hspan - 1;
	  v *= rtable->drag_info.vspan - 1;
	  h += table->cols[0].allocation;
	  v += table->rows[0].allocation;
	  gtk_window_resize (GTK_WINDOW (rtable->drag_window), h, v);
	  h = event->x_root - rtable->xofs;
	  v = event->y_root - rtable->yofs;
	  gtk_window_move (GTK_WINDOW (rtable->drag_window), h, v);
	  gtk_widget_show (rtable->drag_window);
	  bst_rack_table_on_area (rtable, EXPOSE_AREA,
				  rtable->drag_info.col,
				  rtable->drag_info.row,
				  rtable->drag_info.hspan,
				  rtable->drag_info.vspan);
	}
    }
  else if (event->type == GDK_BUTTON_PRESS && event->button == 3 && !rtable->in_drag)
    {
      uint h, v;
      GtkWidget *child;
      if (bst_rack_table_iwindow_translate (rtable, event->x, event->y, &h, &v))
	child = bst_rack_table_find_child (rtable, h, v);
#if 0
      if (BST_IS_RACK_ITEM (child))
	{
	  /* proxy button presses */
	  g_signal_emit_by_name (child, "button_press", event);
	}
#endif
      (void) child;
    }

  return TRUE;
}

static gint
bst_rack_table_motion_notify (GtkWidget      *widget,
			      GdkEventMotion *event)
{
  BstRackTable *rtable = BST_RACK_TABLE (widget);
  GtkTable *table = GTK_TABLE (rtable);

  if (rtable->in_drag)
    {
      guint h, v;
      gint x, y;

      gtk_window_move (GTK_WINDOW (rtable->drag_window), event->x_root - rtable->xofs, event->y_root - rtable->yofs);

      if (event->is_hint)	/* trigger new events */
	gdk_window_get_pointer (widget->window, NULL, NULL, NULL);

      /* translate x/y to first cell mid-point */
      x = event->x - rtable->xofs;
      y = event->y - rtable->yofs;
      x += rtable->cell_width / 2;
      y += rtable->cell_height / 2;

      if (!bst_rack_table_iwindow_translate (rtable, x, y, &h, &v) ||
	  h >= table->ncols || v >= table->nrows ||
	  h + rtable->drag_info.hspan > table->ncols ||
	  v + rtable->drag_info.vspan > table->nrows ||
	  bst_rack_table_check_area (rtable, h, v, rtable->drag_info.hspan, rtable->drag_info.vspan))
	{
	  h = rtable->drag_col;
	  v = rtable->drag_row;
	}
      if (h != uint (rtable->drag_info.col) || v != uint (rtable->drag_info.row))
	{
	  bst_rack_table_on_area (rtable, EXPOSE_AREA,
				  rtable->drag_info.col,
				  rtable->drag_info.row,
				  rtable->drag_info.hspan,
				  rtable->drag_info.vspan);
	  rtable->drag_info.col = h;
	  rtable->drag_info.row = v;
          bst_rack_table_on_area (rtable, EXPOSE_AREA,
				  rtable->drag_info.col,
				  rtable->drag_info.row,
                                  rtable->drag_info.hspan,
				  rtable->drag_info.vspan);
	}
    }

  return TRUE;
}

static void
rtable_abort_drag (BstRackTable *rtable,
		   guint32       etime)
{
  if (rtable->in_drag)
    {
      if (rtable->in_drag_and_grabbing && rtable->iwindow)
	gdk_pointer_ungrab (etime);
      rtable->in_drag = FALSE;
      rtable->in_drag_and_grabbing = FALSE;
      gtk_widget_hide (rtable->drag_window);
      bst_rack_child_set_info (rtable->child,
			       rtable->drag_info.col,
			       rtable->drag_info.row,
			       rtable->drag_info.hspan,
			       rtable->drag_info.vspan);
      widget_reparent (rtable->child, GTK_WIDGET (rtable));
      rtable->child = NULL;
    }
}

static gint
bst_rack_table_button_release (GtkWidget      *widget,
			       GdkEventButton *event)
{
  BstRackTable *rtable = BST_RACK_TABLE (widget);

  if (rtable->in_drag && event->button == 2)
    rtable_abort_drag (rtable, event->time);

  return TRUE;
}

static void
bst_rack_table_on_area (BstRackTable *rtable,
			AreaAction    action,
			guint	      hcell1,
			guint	      vcell1,
			guint	      hspan,
			guint	      vspan)
{
  GtkWidget *widget = GTK_WIDGET (rtable);
  GtkTable *table = GTK_TABLE (rtable);
  guint i, x, y, width, height, hcell2 = hcell1 + hspan, vcell2 = vcell1 + vspan;

  g_return_if_fail (hspan > 0 && hcell2 <= table->ncols);
  g_return_if_fail (vspan > 0 && vcell2 <= table->nrows);

  x = GTK_CONTAINER (widget)->border_width + widget->allocation.x;
  width = 0;
  for (i = 0; i < hcell2; i++)
    {
      guint bound = table->cols[i].allocation + table->cols[i].spacing;

      if (i < hcell1)
	x += bound;
      else
	width += bound;
    }
  y = GTK_CONTAINER (widget)->border_width + widget->allocation.y;
  height = 0;
  for (i = 0; i < vcell2; i++)
    {
      guint bound = table->rows[i].allocation + table->rows[i].spacing;

      if (i < vcell1)
	y += bound;
      else
	height += bound;
    }

  switch (action)
    {
      GdkGC *bg_gc, *dark_gc, *light_gc;
      guint r;
    case EXPOSE_AREA:
      gtk_widget_queue_draw_area (widget, x, y, width, height);
      break;
    case DRAW_INACTIVE:
    case DRAW_ACTIVE:
      if (action == DRAW_INACTIVE)
	{
	  bg_gc = widget->style->bg_gc[GTK_STATE_ACTIVE];
	  light_gc = widget->style->light_gc[widget->state];
	  dark_gc = widget->style->dark_gc[widget->state];
	}
      else
	{
	  bg_gc = widget->style->bg_gc[GTK_STATE_PRELIGHT];
	  dark_gc = widget->style->light_gc[widget->state];
	  light_gc = widget->style->dark_gc[widget->state];
	}
      r = MIN (width, height);
      if (width > height)
	x += (width - height) / 2;
      if (height > width)
	y += (height - width) / 2;
      gdk_draw_arc (widget->window,
		    bg_gc,
		    TRUE,
		    x + r / 4, y + r / 4, r / 2, r / 2,
		    0. * 64, 360. * 64);
      gdk_draw_arc (widget->window,
		    light_gc,
		    FALSE,
		    x + r / 4, y + r / 4, r / 2, r / 2,
		    225. * 64, 180. * 64);
      gdk_draw_arc (widget->window,
		    dark_gc,
		    FALSE,
		    x + r / 4, y + r / 4, r / 2, r / 2,
		    45. * 64, 180. * 64);
      break;
    case FOCUS_AREA:
      gdk_draw_rectangle (widget->window,
			  widget->style->black_gc,
			  FALSE,
			  x, y, width, height);
      break;
    }
}

static gint
bst_rack_table_expose (GtkWidget      *widget,
		       GdkEventExpose *event)
{
  BstRackTable *rtable = BST_RACK_TABLE (widget);
  GtkTable *table = GTK_TABLE (rtable);

  if (rtable->edit_mode)
    {
      uint h1 = rtable->drag_info.col, h2 = h1 + rtable->drag_info.hspan;
      uint v1 = rtable->drag_info.row, v2 = v1 + rtable->drag_info.vspan;
      int x = GTK_CONTAINER (rtable)->border_width + GTK_WIDGET (rtable)->allocation.x, bx = 0;
      uint i, j;

      for (i = 0; i < table->ncols; i++)
	{
	  int y = GTK_CONTAINER (rtable)->border_width + GTK_WIDGET (rtable)->allocation.y, by = 0;

	  bx = table->cols[i].allocation + table->cols[i].spacing;
	  if (x > event->area.x + event->area.width || x + bx < event->area.x)
	    {
	      x += bx;
	      bst_rack_table_on_area (rtable, FOCUS_AREA, i, 0, 1, table->nrows);
	      continue;
	    }
	  for (j = 0; j < table->nrows; j++)
	    {
	      by = table->rows[j].allocation + table->rows[j].spacing;
	      if (y > event->area.y + event->area.height || y + by < event->area.y)
		{
		  y += by;
		  bst_rack_table_on_area (rtable, FOCUS_AREA, i, j, 1, 1);
		  continue;
		}
	      if (bst_rack_table_check_cell (rtable, i, j))
		continue;
	      if (rtable->in_drag &&
		  i >= h1 && i < h2 &&
		  j >= v1 && j < v2)
		bst_rack_table_on_area (rtable, DRAW_ACTIVE, i, j, 1, 1);
	      else
		bst_rack_table_on_area (rtable, DRAW_INACTIVE, i, j, 1, 1);
	      y += by;
	    }
	  x += bx;
	}
    }

  GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);

  return FALSE;
}

static void
bst_rack_table_add (GtkContainer *container,
		    GtkWidget    *child)
{
  BstRackTable *rtable = BST_RACK_TABLE (container);
  GtkTable *table = GTK_TABLE (rtable);
  BstRackChildInfo rinfo = { 0, };

  bst_rack_child_get_info (child, &rinfo);
  if (rinfo.hspan < 1)
    rinfo.hspan = 4;
  rinfo.hspan = CLAMP (rinfo.hspan, 1, table->ncols);
  if (rinfo.vspan < 1)
    rinfo.vspan = 4;
  rinfo.vspan = CLAMP (rinfo.vspan, 1, table->nrows);
  if (rinfo.col < 0 || rinfo.row < 0 ||
      bst_rack_table_check_area (rtable, rinfo.col, rinfo.row, rinfo.hspan, rinfo.vspan))
    {
      guint row, col = 0; /* silence compiler */

      for (row = 0; row < table->nrows; row++)
	for (col = 0; col + rinfo.hspan <= table->ncols; col++)
	  if (!bst_rack_table_check_area (rtable, col, row, rinfo.hspan, rinfo.vspan))
	    goto found_position;
      if (row >= table->nrows)
	col = 0;
    found_position:
      rinfo.col = col;
      rinfo.row = row;
    }
  bst_rack_child_set_info (child, rinfo.col, rinfo.row, rinfo.hspan, rinfo.vspan);
  gtk_table_attach (table, child,
		    rinfo.col, rinfo.col + rinfo.hspan,
		    rinfo.row, rinfo.row + rinfo.vspan,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    0, 0);
  bst_rtable_update_child_map (rtable);
#if 0
  if (BST_IS_RACK_ITEM (child))
    bst_rack_item_gui_changed (BST_RACK_ITEM (child));
#endif
}

static void
bst_rack_table_remove (GtkContainer *container,
		       GtkWidget    *child)
{
  BstRackTable *rtable = BST_RACK_TABLE (container);

  /* cause a map-update */
  rtable->map_cols = 0;
  rtable->map_rows = 0;

  /* chain parent class' handler */
  GTK_CONTAINER_CLASS (parent_class)->remove (container, child);
}

static gboolean
bst_rack_table_iwindow_translate (BstRackTable		*rtable,
				  gint			 x,
				  gint			 y,
				  guint			*hcell,
				  guint			*vcell)
{
  GtkTable *table = GTK_TABLE (rtable);
  guint i;

  x -= GTK_CONTAINER (rtable)->border_width;
  *hcell = 0;
  for (i = 0; i < table->ncols; i++)
    {
      int bound = table->cols[i].allocation + table->cols[i].spacing;

      if (x < bound)
	{
	  *hcell = i;
	  break;
	}
      x -= bound;
    }
  if (i >= table->ncols)
    *hcell = table->ncols;

  y -= GTK_CONTAINER (rtable)->border_width;
  *vcell = 0;
  for (i = 0; i < table->nrows; i++)
    {
      int bound = table->rows[i].allocation + table->rows[i].spacing;

      if (y < bound)
	{
	  *vcell = i;
	  break;
	}
      y -= bound;
    }
  if (i >= table->nrows)
    *vcell = table->nrows;

  return x >= 0 && *hcell < table->ncols && y >= 0 && *vcell < table->nrows;
}

static GtkWidget*
bst_rack_table_find_child (BstRackTable *rtable,
			   guint         hcell,
			   guint         vcell)
{
  GtkTable *table = GTK_TABLE (rtable);
  GList *list;

  for (list = table->children; list; list = list->next)
    {
      GtkTableChild *child = (GtkTableChild*) list->data;

      if (hcell >= child->left_attach && hcell < child->right_attach &&
	  vcell >= child->top_attach && vcell < child->bottom_attach)
	{
#if 0
	  if (rtable->edit_mode &&
	      BST_IS_RACK_ITEM (child->widget) &&
	      BST_RACK_ITEM (child->widget)->empty_frame &&
	      hcell > child->left_attach && hcell + 1 < child->right_attach &&
	      vcell > child->top_attach && vcell + 1 < child->bottom_attach)
	    continue;
	  return child->widget;
#endif
	}
    }

  return NULL;
}

static void
bst_rtable_update_child_map (BstRackTable *rtable)
{
  GtkTable *table = GTK_TABLE (rtable);
  guint32 *bits;
  guint i, j, n = 0;

  rtable->map_cols = table->ncols;
  rtable->map_rows = table->nrows;
  g_free (rtable->child_map);
  rtable->child_map = g_new0 (guint32, rtable->map_cols * rtable->map_rows);
  bits = rtable->child_map;
  for (j = 0; j < table->nrows; j++)
    for (i = 0; i < table->ncols; i++)
      {
	if (bst_rack_table_find_child (rtable, i, j))
	  *bits |= 1 << n;
	if (++n == 32)
	  {
	    n = 0;
	    bits++;
	  }
      }
}

static inline guint
test_cell (guint32 *bits,
	   guint    col,
	   guint    row_offset)
{
  guint i, n;

  i = row_offset + col;
  n = i >> 5;
  bits += n;
  i &= 0x1f;
  return *bits & (1 << i);
}

gboolean
bst_rack_table_check_cell (BstRackTable *rtable,
			   guint         col,
			   guint         row)
{
  GtkTable *table;

  g_return_val_if_fail (BST_IS_RACK_TABLE (rtable), FALSE);

  table = GTK_TABLE (rtable);
  if (col >= table->ncols || row >= table->nrows)
    return FALSE;

  if (table->ncols != rtable->map_cols || table->nrows != rtable->map_rows)
    bst_rtable_update_child_map (rtable);

  return test_cell (rtable->child_map, col, row * table->ncols) > 0;
}

gboolean
bst_rack_table_expand_rect (BstRackTable *rtable,
			    guint	  col,
			    guint	  row,
			    guint        *hspan,
			    guint        *vspan)
{
  GtkTable *table;
  guint i, j, f;

  g_return_val_if_fail (BST_IS_RACK_TABLE (rtable), FALSE);

  table = GTK_TABLE (rtable);
  if (col + 1 >= table->ncols || row + 1 >= table->nrows ||
      bst_rack_table_check_cell (rtable, col, row))
    return FALSE;

  /* h/v expand */
  for (i = 1; col + i < table->ncols; i++)
    if (test_cell (rtable->child_map, col + i, row * table->ncols))
      break;
  for (j = 1; row + j < table->nrows; j++)
    for (f = 0; f < i; f++)
      if (test_cell (rtable->child_map, col + f, (row + j) * table->ncols))
	goto last_row_break;
 last_row_break:
  *hspan = i;
  *vspan = j;

  /* v/h expand */
  for (j = j; row + j < table->nrows; j++)
    if (test_cell (rtable->child_map, col, (row + j) * table->ncols))
      break;
  if (j == *vspan)
    return TRUE;
  for (i = 1; col + i < table->ncols; i++)
    for (f = 0; f < j; f++)
      if (test_cell (rtable->child_map, col + i, (row + f) * table->ncols))
	goto last_col_break;
 last_col_break:
  if (i * j >= *hspan * *vspan)
    {
      *hspan = i;
      *vspan = j;
    }
  return TRUE;
}

gboolean
bst_rack_table_check_area (BstRackTable *rtable,
			   guint         col,
			   guint         row,
			   guint         hspan,
			   guint         vspan)
{
  GtkTable *table;
  guint i, j;

  g_return_val_if_fail (BST_IS_RACK_TABLE (rtable), FALSE);

  table = GTK_TABLE (rtable);
  if (col >= table->ncols || row >= table->nrows)
    return FALSE;

  for (i = 0; i < hspan; i++)
    for (j = 0; j < vspan; j++)
      if (bst_rack_table_check_cell (rtable, col + i, row + j))
	return TRUE;
  return FALSE;
}

void
bst_rack_table_set_edit_mode (BstRackTable *rtable,
			      gboolean      enable_editing)
{
  g_return_if_fail (BST_IS_RACK_TABLE (rtable));

  rtable->edit_mode = enable_editing != FALSE;
  if (GTK_WIDGET_REALIZED (rtable))
    {
      if (rtable->edit_mode)
	gdk_window_show (rtable->iwindow);
      else
	{
	  rtable_abort_drag (rtable, GDK_CURRENT_TIME);
	  gdk_window_hide (rtable->iwindow);
	}
      gtk_widget_queue_draw (GTK_WIDGET (rtable));
    }
  g_signal_emit (rtable, signal_edit_mode_changed, 0, rtable->edit_mode);
}

void
bst_rack_child_get_info (GtkWidget         *widget,
			 BstRackChildInfo *info)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (info != NULL);

#if 0
  if (BST_IS_RACK_ITEM (widget))
    *info = BST_RACK_ITEM (widget)->rack_child_info;
  else
    {
      BstRackChildInfo *rinfo = g_object_get_qdata (G_OBJECT (widget), quark_rack_info);
      if (!rinfo)
	{
	  info->col = -1;
	  info->row = -1;
	  info->hspan = -1;
	  info->vspan = -1;
	}
      else
	*info = *rinfo;
    }
#endif
}

void
bst_rack_child_set_info (GtkWidget *widget,
			 gint       col,
			 gint       row,
			 gint       hspan,
			 gint       vspan)
{
  GtkWidget *parent = NULL;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  g_object_ref (widget);
  if (BST_IS_RACK_TABLE (widget->parent))
    {
      parent = widget->parent;
      gtk_container_remove (GTK_CONTAINER (parent), widget);
    }

#if 0
  BstRackChildInfo *rinfo;
  if (BST_IS_RACK_ITEM (widget))
    rinfo = &BST_RACK_ITEM (widget)->rack_child_info;
  else
    {
      rinfo = g_object_get_qdata (G_OBJECT (widget), quark_rack_info);
      if (!rinfo)
	{
	  rinfo = g_new (BstRackChildInfo, 1);
	  g_object_set_qdata_full (G_OBJECT (widget), quark_rack_info, rinfo, g_free);
	  rinfo->col = -1;
	  rinfo->row = -1;
	  rinfo->hspan = 1;
	  rinfo->vspan = 1;
	}
    }
  rinfo->col = col >= 0 ? col : rinfo->col;
  rinfo->row = row >= 0 ? row : rinfo->row;
  rinfo->hspan = hspan > 1 ? hspan : rinfo->hspan;
  rinfo->vspan = vspan > 1 ? vspan : rinfo->vspan;
#endif
  if (parent)
    gtk_container_add (GTK_CONTAINER (parent), widget);
  g_object_unref (widget);
}
