/* BEAST - Bedevilled Audio System
 * Copyright (C) 2003 Tim Janik
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
#include "bsttrackroll.h"
#include "bstsnifferscope.h"
#include <gdk/gdkkeysyms.h>


/* --- defines --- */
/* helpers */
#define	STYLE(self)		(GTK_WIDGET (self)->style)
#define	STATE(self)		(GTK_WIDGET (self)->state)
#define	SELECTED_STATE(self)	(GTK_WIDGET_IS_SENSITIVE (self) ? GTK_STATE_SELECTED : GTK_STATE_INSENSITIVE)
#define	ACTIVE_STATE(self)	(GTK_WIDGET_IS_SENSITIVE (self) ? GTK_STATE_ACTIVE : GTK_STATE_INSENSITIVE)
#define	XTHICKNESS(self)	(STYLE (self)->xthickness)
#define	YTHICKNESS(self)	(STYLE (self)->ythickness)
#define	ALLOCATION(self)	(&GTK_WIDGET (self)->allocation)

/* layout (requisition) */
#define	ROW_HEIGHT(self)	((gint) 5)
#define	HPANEL_Y(self)		(0)
#define	HPANEL_HEIGHT(self)	(self->area_offset)
#define VPANEL_X(self)		(0)
#define	VPANEL_WIDTH(self)	(HPANEL_HEIGHT (self))
#define	CANVAS_X(self)		(VPANEL_WIDTH (self))
#define	CANVAS_Y(self)		(HPANEL_HEIGHT (self))

/* layout (allocation) */
#define	CANVAS_WIDTH(self)	(ALLOCATION (self)->width - CANVAS_X (self))
#define	CANVAS_HEIGHT(self)	(ALLOCATION (self)->height - CANVAS_Y (self))
#define	MARK_OFFSET		-4
#define	MARK_WIDTH		(2 * ABS (MARK_OFFSET) + 1)

/* aliases */
#define VPANEL_HEIGHT(self)	(CANVAS_HEIGHT (self))
#define	HPANEL_WIDTH(self)	(CANVAS_WIDTH (self))
#define VPANEL_Y(self)		(CANVAS_Y (self))
#define	HPANEL_X(self)		(CANVAS_X (self))

/* appearance */
#define VPANEL_BG_GC(self)	(STYLE (self)->bg_gc[GTK_STATE_INSENSITIVE])
#define VPANEL_BG_COLOR(self)	(&STYLE (self)->bg[GTK_STATE_INSENSITIVE])
#define	HPANEL_BG_GC(self)	(STYLE (self)->bg_gc[GTK_WIDGET_IS_SENSITIVE (self) ? GTK_STATE_NORMAL : GTK_STATE_INSENSITIVE])
#define	HPANEL_BG_COLOR(self)	(&STYLE (self)->bg[GTK_WIDGET_IS_SENSITIVE (self) ? GTK_STATE_NORMAL : GTK_STATE_INSENSITIVE])
#define	CANVAS_BG_COLOR(self)	(&STYLE (self)->base[GTK_WIDGET_STATE (self)])
#define	CANVAS_BG_GC(self)	(STYLE (self)->base_gc[GTK_WIDGET_STATE (self)])
#define TACT_HPIXELS		(50)	/* guideline */

/* behaviour */
#define AUTO_SCROLL_TIMEOUT	(33)
#define	AUTO_SCROLL_SCALE	(0.2)

/* possible drag areas */
enum {
  DRAG_UNCHANGED,
  DRAG_CANVAS,
  DRAG_VPANEL,
  DRAG_HPANEL
};


/* --- prototypes --- */
static void	bst_track_roll_destroy			(GtkObject		*object);
static void	bst_track_roll_finalize			(GObject		*object);
static void	bst_track_roll_set_scroll_adjustments	(BstTrackRoll		*self,
							 GtkAdjustment		*hadjustment,
							 GtkAdjustment		*vadjustment);
static void	bst_track_roll_size_request		(GtkWidget		*widget,
							 GtkRequisition		*requisition);
static void	bst_track_roll_size_allocate		(GtkWidget		*widget,
							 GtkAllocation		*allocation);
static void	bst_track_roll_style_set		(GtkWidget		*widget,
							 GtkStyle		*previous_style);
static void	bst_track_roll_state_changed		(GtkWidget		*widget,
							 guint			 previous_state);
static void	bst_track_roll_realize			(GtkWidget		*widget);
static void	bst_track_roll_unrealize		(GtkWidget		*widget);
static gboolean	bst_track_roll_expose			(GtkWidget		*widget,
							 GdkEventExpose		*event);
static gboolean	bst_track_roll_key_press		(GtkWidget		*widget,
							 GdkEventKey		*event);
static gboolean	bst_track_roll_key_release		(GtkWidget		*widget,
							 GdkEventKey		*event);
static gboolean	bst_track_roll_button_press		(GtkWidget		*widget,
							 GdkEventButton		*event);
static gboolean	bst_track_roll_motion			(GtkWidget		*widget,
							 GdkEventMotion		*event);
static gboolean	bst_track_roll_button_release		(GtkWidget		*widget,
							 GdkEventButton		*event);
static void	track_roll_update_adjustments		(BstTrackRoll		*self,
							 gboolean		 hadj,
							 gboolean		 vadj);
static void	track_roll_scroll_adjustments		(BstTrackRoll		*self,
							 gint			 x_pixel,
							 gint			 y_pixel);
static void	track_roll_adjustment_changed		(BstTrackRoll		*self);
static void	track_roll_adjustment_value_changed	(BstTrackRoll		*self,
							 GtkAdjustment		*adjustment);
static void	bst_track_roll_hsetup			(BstTrackRoll		*self,
							 guint			 tpt,
							 guint			 max_ticks,
							 gdouble		 hzoom);
static void	bst_track_roll_allocate_ecell		(BstTrackRoll		*self);
static void     bst_track_roll_allocate_scope           (BstTrackRoll           *self,
                                                         GtkWidget              *child,
                                                         guint                   row);
static void     track_roll_forall                       (GtkContainer    *container,
                                                         gboolean         include_internals,
                                                         GtkCallback      callback,   
                                                         gpointer         callback_data);


/* --- static variables --- */
static guint	signal_select_row = 0;
static guint	signal_drag = 0;
static guint	signal_clicked = 0;
static guint	signal_stop_edit = 0;


/* --- functions --- */
G_DEFINE_TYPE (BstTrackRoll, bst_track_roll, GTK_TYPE_CONTAINER);

static void
bst_track_roll_class_init (BstTrackRollClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (class);

  gobject_class->finalize = bst_track_roll_finalize;

  object_class->destroy = bst_track_roll_destroy;
  
  widget_class->size_request = bst_track_roll_size_request;
  widget_class->size_allocate = bst_track_roll_size_allocate;
  widget_class->realize = bst_track_roll_realize;
  widget_class->unrealize = bst_track_roll_unrealize;
  widget_class->style_set = bst_track_roll_style_set;
  widget_class->state_changed = bst_track_roll_state_changed;
  widget_class->expose_event = bst_track_roll_expose;
  widget_class->key_press_event = bst_track_roll_key_press;
  widget_class->key_release_event = bst_track_roll_key_release;
  widget_class->button_press_event = bst_track_roll_button_press;
  widget_class->motion_notify_event = bst_track_roll_motion;
  widget_class->button_release_event = bst_track_roll_button_release;

  container_class->forall = track_roll_forall;

  class->set_scroll_adjustments = bst_track_roll_set_scroll_adjustments;
  class->drag = NULL;
  class->clicked = NULL;
  
  signal_select_row = g_signal_new ("select-row", G_OBJECT_CLASS_TYPE (class),
				    G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstTrackRollClass, select_row),
				    NULL, NULL,
				    bst_marshal_NONE__INT,
				    G_TYPE_NONE, 1, G_TYPE_INT);
  signal_drag = g_signal_new ("drag", G_OBJECT_CLASS_TYPE (class),
			      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstTrackRollClass, drag),
			      NULL, NULL,
			      bst_marshal_NONE__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_POINTER);
  signal_clicked = g_signal_new ("clicked", G_OBJECT_CLASS_TYPE (class),
				 G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstTrackRollClass, clicked),
				 NULL, NULL,
				 bst_marshal_NONE__UINT_UINT_INT_BOXED,
				 G_TYPE_NONE, 4, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_INT,
				 GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  signal_stop_edit = g_signal_new ("stop-edit", G_OBJECT_CLASS_TYPE (class),
				   G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstTrackRollClass, stop_edit),
				   NULL, NULL,
				   bst_marshal_NONE__BOOLEAN_OBJECT,
				   G_TYPE_NONE, 2,
				   G_TYPE_BOOLEAN, G_TYPE_OBJECT);
  widget_class->set_scroll_adjustments_signal =
    gtk_signal_new ("set_scroll_adjustments",
		    GTK_RUN_LAST,
		    GTK_CLASS_TYPE (object_class),
		    GTK_SIGNAL_OFFSET (BstTrackRollClass, set_scroll_adjustments),
		    bst_marshal_NONE__OBJECT_OBJECT,
		    GTK_TYPE_NONE, 2, GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);
}

static void
bst_track_roll_init (BstTrackRoll *self)
{
  GtkWidget *widget = GTK_WIDGET (self);

  GTK_WIDGET_UNSET_FLAGS (self, GTK_NO_WINDOW);
  gtk_widget_set_double_buffered (widget, FALSE);
  gtk_widget_set_redraw_on_allocate (widget, TRUE);
  
  self->tpt = 384 * 4;
  self->max_ticks = 1;
  self->hzoom = 1;
  self->draw_tact_grid = TRUE;
  self->x_offset = 0;
  self->y_offset = 0;
  self->prelight_row = 0;
  self->hpanel_height = 20;
  self->vpanel = NULL;
  self->hpanel = NULL;
  self->canvas = NULL;
  self->canvas_cursor = GDK_LEFT_PTR;
  self->vpanel_cursor = GDK_HAND2;
  self->hpanel_cursor = GDK_LEFT_PTR;
  self->hadjustment = NULL;
  self->vadjustment = NULL;
  self->scroll_timer = 0;
  self->area_offset = 20;
  bst_marker_init_vertical (&self->vmarker);
  self->in_drag = FALSE;
  bst_track_roll_hsetup (self, 384 * 4, 800 * 384, 100);
  bst_track_roll_set_hadjustment (self, NULL);
  bst_track_roll_set_vadjustment (self, NULL);
}

static void
bst_track_roll_destroy (GtkObject *object)
{
  BstTrackRoll *self = BST_TRACK_ROLL (object);
  guint i;

  if (self->scope_update)
    {
      g_source_remove (self->scope_update);
      self->scope_update = 0;
    }
  bst_track_roll_set_hadjustment (self, NULL);
  bst_track_roll_set_vadjustment (self, NULL);
  for (i = 0; i < self->n_scopes; i++)
    gtk_widget_unparent (self->scopes[i]);
  g_free (self->scopes);
  self->scopes = NULL;
  self->n_scopes = 0;
  
  GTK_OBJECT_CLASS (bst_track_roll_parent_class)->destroy (object);
}

static void
bst_track_roll_finalize (GObject *object)
{
  BstTrackRoll *self = BST_TRACK_ROLL (object);
  guint i;
  
  bst_track_roll_setup (self, NULL, 0);

  if (self->scope_update)
    {
      g_source_remove (self->scope_update);
      self->scope_update = 0;
    }
  g_object_unref (self->hadjustment);
  self->hadjustment = NULL;
  g_object_unref (self->vadjustment);
  self->vadjustment = NULL;

  if (self->scroll_timer)
    {
      g_source_remove (self->scroll_timer);
      self->scroll_timer = 0;
    }
  bst_marker_finalize (&self->vmarker);
  for (i = 0; i < self->n_scopes; i++)
    gtk_widget_unparent (self->scopes[i]);
  g_free (self->scopes);
  
  G_OBJECT_CLASS (bst_track_roll_parent_class)->finalize (object);
}

void
bst_track_roll_set_hadjustment (BstTrackRoll  *self,
				GtkAdjustment *adjustment)
{
  g_return_if_fail (BST_IS_TRACK_ROLL (self));
  if (adjustment)
    g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));

  if (self->hadjustment)
    {
      g_object_disconnect (self->hadjustment,
			   "any_signal", track_roll_adjustment_changed, self,
			   "any_signal", track_roll_adjustment_value_changed, self,
			   NULL);
      g_object_unref (self->hadjustment);
      self->hadjustment = NULL;
    }

  if (!adjustment)
    adjustment = g_object_new (GTK_TYPE_ADJUSTMENT, NULL);

  self->hadjustment = g_object_ref (adjustment);
  gtk_object_sink (GTK_OBJECT (adjustment));
  g_object_connect (self->hadjustment,
		    "swapped_signal::changed", track_roll_adjustment_changed, self,
		    "swapped_signal::value-changed", track_roll_adjustment_value_changed, self,
		    NULL);
}

void
bst_track_roll_set_vadjustment (BstTrackRoll  *self,
				GtkAdjustment *adjustment)
{
  g_return_if_fail (BST_IS_TRACK_ROLL (self));
  if (adjustment)
    g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));

  if (self->vadjustment)
    {
      g_object_disconnect (self->vadjustment,
			   "any_signal", track_roll_adjustment_changed, self,
			   "any_signal", track_roll_adjustment_value_changed, self,
			   NULL);
      g_object_unref (self->vadjustment);
      self->vadjustment = NULL;
    }

  if (!adjustment)
    adjustment = g_object_new (GTK_TYPE_ADJUSTMENT, NULL);

  self->vadjustment = g_object_ref (adjustment);
  gtk_object_sink (GTK_OBJECT (adjustment));
  g_object_connect (self->vadjustment,
		    "swapped_signal::changed", track_roll_adjustment_changed, self,
		    "swapped_signal::value-changed", track_roll_adjustment_value_changed, self,
		    NULL);
}

static void
bst_track_roll_set_scroll_adjustments (BstTrackRoll  *self,
				       GtkAdjustment *hadjustment,
				       GtkAdjustment *vadjustment)
{
  if (self->hadjustment != hadjustment)
    bst_track_roll_set_hadjustment (self, hadjustment);
  if (self->vadjustment != vadjustment)
    bst_track_roll_set_vadjustment (self, vadjustment);
}

static void
track_roll_forall (GtkContainer    *container,
                   gboolean         include_internals,
                   GtkCallback      callback,
                   gpointer         callback_data)
{
  BstTrackRoll *self = BST_TRACK_ROLL (container);
  guint i;
  for (i = 0; i < self->n_scopes; i++)
    if (include_internals)
      callback (self->scopes[i], callback_data);
  if (self->ecell && include_internals)
    callback ((GtkWidget*) self->ecell, callback_data);
}

static void
track_roll_reset_backgrounds (BstTrackRoll *self)
{
  GtkWidget *widget = GTK_WIDGET (self);

  if (GTK_WIDGET_REALIZED (self))
    {
      gtk_style_set_background (widget->style, widget->window, GTK_WIDGET_STATE (self));
      gdk_window_set_background (self->vpanel, VPANEL_BG_COLOR (self));
      gdk_window_set_background (self->hpanel, HPANEL_BG_COLOR (self));
      gdk_window_set_background (self->canvas, CANVAS_BG_COLOR (self));
      gdk_window_clear (widget->window);
      gdk_window_clear (self->vpanel);
      gdk_window_clear (self->hpanel);
      gdk_window_clear (self->canvas);
      gtk_widget_queue_draw (widget);
    }
}

static void
bst_track_roll_style_set (GtkWidget *widget,
			  GtkStyle  *previous_style)
{
  BstTrackRoll *self = BST_TRACK_ROLL (widget);

  self->hpanel_height = 20;
  track_roll_reset_backgrounds (self);
}

static void
bst_track_roll_state_changed (GtkWidget *widget,
			      guint	 previous_state)
{
  BstTrackRoll *self = BST_TRACK_ROLL (widget);

  track_roll_reset_backgrounds (self);
}

static void
track_roll_update_layout (BstTrackRoll *self,
			  gboolean      queue_resize)
{
  gint old_area_offset, dummy;

  old_area_offset = self->area_offset;

  if (self->tree)
    gxk_tree_view_get_bin_window_pos (self->tree, &dummy, &self->area_offset);
  else
    self->area_offset = 20;

  if (old_area_offset != self->area_offset && queue_resize)
    gtk_widget_queue_resize (GTK_WIDGET (self));
}

static void
track_roll_release_proxy (BstTrackRoll *self)
{
  bse_proxy_disconnect (self->proxy,
                        "any_signal", track_roll_release_proxy, self,
                        NULL);
  bse_item_unuse (self->proxy);
  self->proxy = 0;
}

void
bst_track_roll_setup (BstTrackRoll   *self,
                      GtkTreeView    *tree,
                      SfiProxy        song)
{
  g_return_if_fail (BST_IS_TRACK_ROLL (self));
  if (tree)
    g_return_if_fail (GTK_IS_TREE_VIEW (tree));
  if (song)
    g_return_if_fail (BSE_IS_SONG (song));

  if (self->tree)
    {
      g_object_disconnect_any (self->tree, bst_track_roll_reallocate, self);
      g_object_unref (self->tree);
    }
  self->tree = tree;
  if (self->tree)
    {
      g_object_ref (tree);
      g_object_connect (self->tree,
                        "swapped_object_signal_after::size_allocate", bst_track_roll_reallocate, self,
                        NULL);
    }

  if (self->proxy)
    track_roll_release_proxy (self);
  self->proxy = song;
  if (self->proxy)
    {
      bse_item_use (self->proxy);
      bse_proxy_connect (self->proxy,
                         "swapped_signal::release", track_roll_release_proxy, self,
                         NULL);
    }
  track_roll_update_layout (self, TRUE);
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

static void
bst_track_roll_size_request (GtkWidget	    *widget,
			     GtkRequisition *requisition)
{
  BstTrackRoll *self = BST_TRACK_ROLL (widget);

  track_roll_update_layout (self, FALSE);
  requisition->width = 1 + 2 * XTHICKNESS (self) + 1;
  requisition->height = 1;
  if (self->ecell)
    gtk_widget_size_request (GTK_WIDGET (self->ecell), NULL);
}

static void
bst_track_roll_size_allocate (GtkWidget	    *widget,
			      GtkAllocation *allocation)
{
  BstTrackRoll *self = BST_TRACK_ROLL (widget);

  widget->allocation.x = allocation->x;
  widget->allocation.y = allocation->y;
  widget->allocation.width = MAX (1, allocation->width);
  widget->allocation.height = MAX (1, allocation->height);

  bst_track_roll_reallocate (self);
}

void
bst_track_roll_reselect (BstTrackRoll *self)
{
  g_return_if_fail (BST_IS_TRACK_ROLL (self));

  if (self->tree)
    {
      bst_track_roll_set_prelight_row (self, gxk_tree_view_get_selected_row (self->tree));
      if (GTK_WIDGET_DRAWABLE (self))
        gxk_window_process_next (GTK_WIDGET (self)->window, TRUE);
    }
  else
    bst_track_roll_set_prelight_row (self, 0xffffffff);
}

static void
track_roll_reallocate_children (BstTrackRoll *self)
{
  guint i;

  bst_track_roll_allocate_ecell (self);
  bst_marker_resize (&self->vmarker);
  for (i = 0; i < self->n_scopes; i++)
    bst_track_roll_allocate_scope (self, self->scopes[i], i);
  bst_track_roll_check_update_scopes (self);
  bst_track_roll_reselect (self);
}

void
bst_track_roll_reallocate (BstTrackRoll *self)
{
  g_return_if_fail (BST_IS_TRACK_ROLL (self));

  track_roll_update_layout (self, FALSE);
  if (GTK_WIDGET_REALIZED (self))
    {
      GtkWidget *widget = GTK_WIDGET (self);
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
  track_roll_update_adjustments (self, TRUE, TRUE);
  track_roll_reallocate_children (self);
}

static void
scope_set_track (GtkWidget *scope,
                 SfiProxy   track)
{
  g_object_set_long (scope, "BstTrackRoll-Track", track);
  if (BST_SNIFFER_SCOPE (scope))
    bst_sniffer_scope_set_sniffer (BST_SNIFFER_SCOPE (scope),
                                   bse_track_create_sniffer (track));
}

static SfiProxy
scope_get_track (GtkWidget *scope)
{
  return g_object_get_long (scope, "BstTrackRoll-Track");
}

static gboolean
track_roll_idle_update_scopes (gpointer data)
{
  BstTrackRoll *self = BST_TRACK_ROLL (data);
  GSList *scope_list = NULL;
  guint i;

  GDK_THREADS_ENTER ();
  self->scope_update = 0;
  
  /* save existing scopes */
  for (i = 0; i < self->n_scopes; i++)
    scope_list = g_slist_prepend (scope_list, self->scopes[i]);

  /* reset scope list */
  self->n_scopes = 0;

  /* match or create needed scopes */
  if (self->get_track && GTK_WIDGET_REALIZED (self))
    for (i = 0; ; i++)
      {
        SfiProxy track = self->get_track (self->proxy_data, i);
        GtkWidget *scope = NULL;
        GSList *slist;
        if (!track)
          break;
        for (slist = scope_list; slist; slist = slist->next)
          {
            scope = slist->data;
            if (scope_get_track (scope) == track)  /* match existing */
              {
                scope_list = g_slist_remove (scope_list, scope);
                break;
              }
          }
        if (!slist)     /* create new if not matched */
          {
            scope = g_object_new (BST_TYPE_SNIFFER_SCOPE, NULL);
            gtk_widget_show (scope);
            scope_set_track (scope, track);
            gtk_widget_set_parent_window (scope, self->vpanel);
            gtk_widget_set_parent (scope, GTK_WIDGET (self));
          }
        /* add to scope list */
        self->n_scopes++;
        self->scopes = g_renew (GtkWidget*, self->scopes, self->n_scopes);
        self->scopes[i] = scope;
      }

  /* get rid of unneeded scopes */
  while (scope_list)
    {
      GtkWidget *child = g_slist_pop_head (&scope_list);
      gtk_widget_unparent (child);
    }
  
  /* allocate scopes 'n stuff */
  bst_track_roll_reallocate (self);

  GDK_THREADS_LEAVE ();
  return FALSE;
}

static void
queue_scope_update (BstTrackRoll *self)
{
  if (!self->scope_update)
    self->scope_update = g_idle_add_full (GTK_PRIORITY_RESIZE - 1, track_roll_idle_update_scopes, self, NULL);
}

void
bst_track_roll_check_update_scopes (BstTrackRoll *self)
{
  guint i;
  g_return_if_fail (BST_IS_TRACK_ROLL (self));

  /* check whether scope update is necessary and schedule one */
  if (!GTK_WIDGET_REALIZED (self) || !self->get_track)
    {
      if (self->n_scopes)
        queue_scope_update (self);
      return;
    }
  for (i = 0; i < self->n_scopes; i++)
    {
      SfiProxy track = self->get_track (self->proxy_data, i);
      if (scope_get_track (self->scopes[i]) != track)
        {
          queue_scope_update (self);
          return;
        }
    }
  if (self->get_track (self->proxy_data, i)) /* one off, shouldn't exist */
    {
      queue_scope_update (self);
      return;
    }
}

static void
bst_track_roll_realize (GtkWidget *widget)
{
  BstTrackRoll *self = BST_TRACK_ROLL (widget);
  GdkWindowAttr attributes;
  GdkCursorType cursor_type;
  guint i, attributes_mask;
  
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
							    GDK_BUTTON_PRESS_MASK |
							    GDK_BUTTON_RELEASE_MASK |
							    GDK_BUTTON_MOTION_MASK |
							    GDK_POINTER_MOTION_HINT_MASK);
  self->vpanel = gdk_window_new (widget->window, &attributes, attributes_mask);
  gdk_window_set_user_data (self->vpanel, self);
  gdk_window_show (self->vpanel);

  /* self->hpanel */
  attributes.x = HPANEL_X (self);
  attributes.y = HPANEL_Y (self);
  attributes.width = HPANEL_WIDTH (self);
  attributes.height = HPANEL_HEIGHT (self);
  attributes.event_mask = gtk_widget_get_events (widget) | (GDK_EXPOSURE_MASK |
							    GDK_BUTTON_PRESS_MASK |
							    GDK_BUTTON_RELEASE_MASK |
							    GDK_BUTTON_MOTION_MASK |
							    GDK_POINTER_MOTION_HINT_MASK);
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
							    GDK_BUTTON_MOTION_MASK |
							    GDK_POINTER_MOTION_HINT_MASK |
							    GDK_KEY_PRESS_MASK |
							    GDK_KEY_RELEASE_MASK);
  self->canvas = gdk_window_new (widget->window, &attributes, attributes_mask);
  gdk_window_set_user_data (self->canvas, self);
  gdk_window_show (self->canvas);

  /* style setup */
  widget->style = gtk_style_attach (widget->style, widget->window);
  track_roll_reset_backgrounds (self);

  /* update cursors */
  cursor_type = self->canvas_cursor;
  self->canvas_cursor = -1;
  bst_track_roll_set_canvas_cursor (self, cursor_type);
  cursor_type = self->vpanel_cursor;
  self->vpanel_cursor = -1;
  bst_track_roll_set_vpanel_cursor (self, cursor_type);
  cursor_type = self->hpanel_cursor;
  self->hpanel_cursor = -1;
  bst_track_roll_set_hpanel_cursor (self, cursor_type);
  bst_marker_realize (&self->vmarker, self->canvas);

  /* update children */
  for (i = 0; i < self->n_scopes; i++)
    gtk_widget_set_parent_window (self->scopes[i], self->vpanel);
  if (self->ecell)
    gtk_widget_set_parent_window (GTK_WIDGET (self->ecell), self->canvas);
}

static void
bst_track_roll_unrealize (GtkWidget *widget)
{
  BstTrackRoll *self = BST_TRACK_ROLL (widget);
  guint i;

  bst_track_roll_abort_edit (self);
  gdk_window_set_user_data (self->canvas, NULL);
  gdk_window_destroy (self->canvas);
  self->canvas = NULL;
  gdk_window_set_user_data (self->hpanel, NULL);
  gdk_window_destroy (self->hpanel);
  self->hpanel = NULL;
  gdk_window_set_user_data (self->vpanel, NULL);
  gdk_window_destroy (self->vpanel);
  self->vpanel = NULL;
  bst_marker_unrealize (&self->vmarker);

  /* update children */
  for (i = 0; i < self->n_scopes; i++)
    gtk_widget_set_parent_window (self->scopes[i], self->vpanel);
  if (self->ecell)
    gtk_widget_set_parent_window (GTK_WIDGET (self->ecell), self->canvas);

  if (GTK_WIDGET_CLASS (bst_track_roll_parent_class)->unrealize)
    GTK_WIDGET_CLASS (bst_track_roll_parent_class)->unrealize (widget);
}

static gint
ticks_to_pixels (BstTrackRoll *self,
		 gint	       ticks)
{
  gdouble tpt = self->tpt;
  gdouble tpixels = TACT_HPIXELS;

  /* compute pixel span of a tick range */

  tpixels *= self->hzoom / tpt * (gdouble) ticks;
  if (ticks)
    tpixels = MAX (tpixels, 1);
  return tpixels;
}

static gint
pixels_to_ticks (BstTrackRoll *self,
		 gint	       pixels)
{
  gdouble tpt = self->tpt;
  gdouble ticks = 1.0 / (gdouble) TACT_HPIXELS;

  /* compute tick span of a pixel range */

  ticks = ticks * tpt / self->hzoom * (gdouble) pixels;
  if (pixels > 0)
    ticks = MAX (ticks, 1);
  else
    ticks = 0;
  return ticks;
}

static gint
tick_to_coord (BstTrackRoll *self,
	       gint	     tick)
{
  return ticks_to_pixels (self, tick) - self->x_offset;
}

static gint
coord_to_tick (BstTrackRoll *self,
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

#define	CROSSING_TACT4		(1)
#define	CROSSING_TACT		(2)

static guint
coord_check_crossing (BstTrackRoll *self,
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
    case CROSSING_TACT4:
      lq = ltick / (self->tpt * 4);
      rq = rtick / (self->tpt * 4);
      break;
    case CROSSING_TACT:
      lq = ltick / self->tpt;
      rq = rtick / self->tpt;
      break;
    }

  return lq != rq;
}

static gint
coord_to_row (BstTrackRoll *self,
	      gint          y,
	      gboolean     *is_valid)
{
  gint row;
  if (self->tree && is_valid)
    *is_valid = gxk_tree_view_get_row_from_coord (self->tree, y, &row);
  else if (self->tree)
    gxk_tree_view_get_row_from_coord (self->tree, y, &row);
  else
    row = y / 15; /* uneducated guess */
  return row;
}

static gboolean
row_to_coords (BstTrackRoll *self,
	       gint          row,
	       gint         *y_p,
	       gint         *height_p)
{
  if (self->tree)
    return gxk_tree_view_get_row_area (self->tree, row, y_p, height_p);
  else
    {
      if (y_p)
	*y_p = row * 15;
      if (height_p)
	*height_p = 15;
      return TRUE;
    }
}

static SfiProxy
row_to_track (BstTrackRoll *self,
	      gint          row)
{
  if (self->get_track)
    return self->get_track (self->proxy_data, row);
  else
    return 0;
}

static void
bst_track_roll_draw_canvas (BstTrackRoll *self,
			    gint          x,
			    gint	  y,
			    gint          xbound,
			    gint          ybound)
{
  GdkWindow *drawable = self->canvas;
  GtkWidget *widget = GTK_WIDGET (self);
  GdkFont *font = gtk_style_get_font (STYLE (self));
  GdkGC *fg_gc = widget->style->fg_gc[widget->state];
  GdkGC *bg2_gc = widget->style->bg_gc[widget->state];
  GdkGC *bg1_gc = STYLE (self)->bg_gc[GTK_STATE_ACTIVE];
  GdkGC *bgp_gc = widget->style->bg_gc[GTK_STATE_PRELIGHT];
  GdkGC *dark_gc = widget->style->dark_gc[widget->state];
  GdkGC *light_gc = widget->style->light_gc[widget->state];
  gint row = coord_to_row (self, y, NULL);
  gint ry, rheight, validrow;
  // gint line_width = 0; /* line widths != 0 interfere with dash-settings on some X servers */

  validrow = row_to_coords (self, row, &ry, &rheight);
  while (validrow && ry < ybound)
    {
      SfiProxy track = row_to_track (self, row);
      if (row == self->ecell_row)
	bst_track_roll_allocate_ecell (self);
      gdk_draw_rectangle (drawable,
			  row == self->prelight_row ? bgp_gc : bg1_gc, // row & 1 ? bgd_gc : bg_gc,
			  TRUE,
			  0, ry, CANVAS_WIDTH (self), rheight);
      if (track)
	{
	  BseTrackPartSeq *tps = bse_track_list_parts (track);
	  gint i;

	  for (i = 0; i < tps->n_tparts; i++)
	    {
	      guint tick = tps->tparts[i]->tick;
	      SfiProxy part = tps->tparts[i]->part;
	      guint duration = tps->tparts[i]->duration;
	      const gchar *name = bse_item_get_name (part);
	      GdkRectangle area, carea;
	      gint ascent, descent, sh;
	      gdk_string_extents (font, name, NULL, NULL, NULL, &ascent, &descent);
	      sh = ascent + descent;
	      area.x = tick_to_coord (self, tick);
	      area.y = ry + 1;
	      area.width = ticks_to_pixels (self, duration);
	      area.height = rheight;
	      area.height = MAX (area.height, 2) - 2;
	      carea = area;
	      carea.x += XTHICKNESS (self);
	      carea.y += YTHICKNESS (self);
	      carea.width = MAX (carea.width - 2 * XTHICKNESS (self), 0);
	      carea.height = MAX (carea.height - 2 * YTHICKNESS (self), 0);
	      gdk_draw_rectangle (drawable,
				  bg2_gc, // row == self->prelight_row ? bgp_gc : bg2_gc,
				  TRUE,
				  carea.x, carea.y, carea.width, carea.height);
	      gdk_gc_set_clip_rectangle (fg_gc, &carea);
	      gdk_draw_string (drawable, font, fg_gc,
			       area.x + XTHICKNESS (self) + 3,
			       area.y - descent + area.height - (MAX (area.height, sh) - sh) / 2,
			       name);
	      gdk_gc_set_clip_rectangle (fg_gc, NULL);
	      gtk_paint_shadow (widget->style, drawable,
				widget->state, // row == self->prelight_row ? GTK_STATE_PRELIGHT : widget->state,
				GTK_SHADOW_OUT, NULL, NULL, NULL,
				area.x, area.y, area.width, area.height);
	    }
	}
      gdk_draw_line (drawable, light_gc, 0, ry, CANVAS_WIDTH (self), ry);
      gdk_draw_line (drawable, dark_gc, 0, ry + rheight - 1, CANVAS_WIDTH (self), ry + rheight - 1);
      validrow = row_to_coords (self, ++row, &ry, &rheight);
    }
}

static void
bst_track_roll_overlap_grow_hpanel_area (BstTrackRoll *self,
					 GdkRectangle *area)
{
  gint i, x = area->x, xbound = x + area->width;

  /* grow hpanel exposes by surrounding tacts */
  i = coord_to_tick (self, x, FALSE);
  i /= self->tpt;
  if (i > 0)
    i -= 1;		/* fudge 1 tact to the left */
  i *= self->tpt;
  x = tick_to_coord (self, i);
  i = coord_to_tick (self, xbound + 1, TRUE);
  i /= self->tpt;
  i += 1;		/* fudge 1 tact to the right */
  i *= self->tpt;
  xbound = tick_to_coord (self, i);

  area->x = x;
  area->width = xbound - area->x;
}

static void
bst_track_roll_draw_hpanel (BstTrackRoll *self,
			    gint	  x,
			    gint	  xbound)
{
  GdkWindow *drawable = self->hpanel;
  GtkWidget *widget = GTK_WIDGET (self);
  GdkFont *font = gtk_style_get_font (STYLE (self));
  GdkGC *bg_gc = HPANEL_BG_GC (self);
  GdkGC *fg_gc = STYLE (self)->fg_gc[widget->state]; // GTK_WIDGET_IS_SENSITIVE (self) ? GTK_STATE_SELECTED : GTK_STATE_INSENSITIVE];
  gint text_y = HPANEL_HEIGHT (self) - (HPANEL_HEIGHT (self) - gdk_string_height (font, "0123456789:")) / 2;
  gchar buffer[64];
  gint i;

  /* clear background */
  gdk_draw_rectangle (drawable, bg_gc, TRUE,
		      0, 0, HPANEL_WIDTH (self), HPANEL_HEIGHT (self));

  /* tact numbers */
  for (i = x; i < xbound; i++)
    {
      guint next_pixel, width;

      /* drawing tact numbers is not of much use if we can't even draw
       * the tact grid, so we special case draw_tact_grid here
       */

      if (coord_check_crossing (self, i, CROSSING_TACT4))
	{
	  guint tact4 = coord_to_tick (self, i, TRUE) + 1;

	  tact4 /= (self->tpt * 4);
	  next_pixel = tick_to_coord (self, (tact4 + 1) * (self->tpt * 4));

	  g_snprintf (buffer, 64, "%u", tact4 + 1);

	  /* draw this tact if there's enough space */
	  width = gdk_string_width (font, buffer);
	  if (i + width / 2 < (i + next_pixel) / 2)
	    gdk_draw_string (self->hpanel, font, fg_gc,
			     i - width / 2, text_y,
			     buffer);
	}
      else if (self->draw_tact_grid && coord_check_crossing (self, i, CROSSING_TACT))
	{
          guint tact = coord_to_tick (self, i, TRUE) + 1;

	  tact /= self->tpt;
	  next_pixel = tick_to_coord (self, (tact + 1) * self->tpt);

	  g_snprintf (buffer, 64, ":%u", tact % 4 + 1);

	  /* draw this tact if there's enough space */
	  width = gdk_string_width (font, buffer);
	  if (i + width < (i + next_pixel) / 2)		/* don't half width, leave some more space */
	    gdk_draw_string (self->hpanel, font, fg_gc,
			     i - width / 2, text_y,
			     buffer);
	}
    }

  /* draw outer hpanel shadow */
  gtk_paint_shadow (widget->style, drawable,
		    widget->state, GTK_SHADOW_OUT,
		    NULL, NULL, NULL,
		    -XTHICKNESS (self), 0,
		    HPANEL_WIDTH (self) + 2 * XTHICKNESS (self), HPANEL_HEIGHT (self));

  /* draw markers */
  for (i = 0; i < self->vmarker.n_marks; i++)
    {
      GdkGC *gc = bst_marker_get_gc (&self->vmarker, self->vmarker.marks + i);
      gint x = self->vmarker.marks[i].pixoffset;
      gdk_draw_rectangle (drawable, gc, TRUE,
			  x + MARK_OFFSET,
			  2 * YTHICKNESS (self),
			  MARK_WIDTH,
			  HPANEL_HEIGHT (self) - 4 * YTHICKNESS (self));
      gtk_paint_shadow (widget->style, drawable,
			widget->state, GTK_SHADOW_IN,
			NULL, NULL, NULL,
			x + MARK_OFFSET - XTHICKNESS (self),
			YTHICKNESS (self),
			MARK_WIDTH + 2 * XTHICKNESS (self),
			HPANEL_HEIGHT (self) - 2 * YTHICKNESS (self));
      gtk_paint_shadow (widget->style, drawable,
			widget->state, GTK_SHADOW_OUT,
			NULL, NULL, NULL,
			x + MARK_OFFSET,
			2 * YTHICKNESS (self),
			MARK_WIDTH,
			HPANEL_HEIGHT (self) - 4 * YTHICKNESS (self));
    }
}

static void
bst_track_roll_expose_mark (BstTrackRoll *self,
			    BstMarker    *mark)
{
  if (GTK_WIDGET_DRAWABLE (self))
    {
      GdkRectangle area;
      area.x = mark->pixoffset + MARK_OFFSET - XTHICKNESS (self);
      area.y = 0;
      area.width = MARK_WIDTH + 2 * XTHICKNESS (self);
      area.height = HPANEL_HEIGHT (self);
      gdk_window_invalidate_rect (self->hpanel, &area, TRUE);
      gxk_window_process_next (self->hpanel, TRUE);
    }
}

static void
bst_track_roll_allocate_scope (BstTrackRoll *self,
                               GtkWidget    *child,
                               guint         row)
{
  GtkAllocation allocation;
  gint ry, rheight, validrow;
  gtk_widget_size_request (child, NULL);
  validrow = GTK_WIDGET_REALIZED (self) ? row_to_coords (self, row, &ry, &rheight) : FALSE;
  if (!validrow)
    {
      allocation.x = allocation.y = -10;
      allocation.width = allocation.height = 1;
    }
  else
    {
      allocation.x = 2 * XTHICKNESS (self) + 1;
      allocation.y = ry + YTHICKNESS (self) + 1;
      allocation.width = VPANEL_WIDTH (self) - 4 * XTHICKNESS (self) - 2;
      allocation.height = rheight - 2 * YTHICKNESS (self) - 2;
    }
  gtk_widget_size_allocate (child, &allocation);
}

static void
bst_track_roll_draw_vpanel (BstTrackRoll *self,
			    gint	  y,
			    gint	  ybound)
{
  GdkWindow *drawable = self->vpanel;
  GtkWidget *widget = GTK_WIDGET (self);
  GdkGC *bg_gc = VPANEL_BG_GC (self);
  gint row = coord_to_row (self, y, NULL);
  gint ry, rheight, validrow;
  validrow = row_to_coords (self, row, &ry, &rheight);

  gdk_draw_rectangle (drawable, bg_gc, TRUE,
		      0, 0, VPANEL_WIDTH (self), VPANEL_HEIGHT (self));
  while (validrow && ry < ybound)
    {
      gdk_draw_rectangle (drawable,
                          STYLE (self)->base_gc[row == self->prelight_row ? ACTIVE_STATE (self) : STATE (self)],
			  TRUE,
			  2 * XTHICKNESS (self) + 1,
			  ry + YTHICKNESS (self) + 1,
			  VPANEL_WIDTH (self) - 4 * XTHICKNESS (self) - 2,
			  rheight - 2 * YTHICKNESS (self) - 2);
      gtk_paint_shadow (widget->style, drawable,
			widget->state, GTK_SHADOW_IN,
			NULL, NULL, NULL,
			XTHICKNESS (self) + 1,
			ry + 1,
			VPANEL_WIDTH (self) - 2 * XTHICKNESS (self) - 2,
			rheight - 2);
      validrow = row_to_coords (self, ++row, &ry, &rheight);
    }
  gtk_paint_shadow (widget->style, drawable,
		    widget->state, GTK_SHADOW_OUT,
		    NULL, NULL, NULL,
		    0, -YTHICKNESS (self),
		    VPANEL_WIDTH (self), VPANEL_HEIGHT (self) + 2 * YTHICKNESS (self));
}

static void
bst_track_roll_draw_window (BstTrackRoll *self,
			    gint          x,
			    gint          y,
			    gint          xbound,
			    gint          ybound)
{
  GdkWindow *drawable = GTK_WIDGET (self)->window;
  GtkWidget *widget = GTK_WIDGET (self);
  GdkGC *bg_gc = STYLE (self)->bg_gc[GTK_WIDGET_STATE (self)];
  gint width = HPANEL_X (self);
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
bst_track_roll_expose (GtkWidget      *widget,
		       GdkEventExpose *event)
{
  BstTrackRoll *self = BST_TRACK_ROLL (widget);
  GdkRectangle area = event->area;
  
  /* with gtk_widget_set_double_buffered (self, FALSE) in init and
   * with gdk_window_begin_paint_region()/gdk_window_end_paint()
   * around our redraw functions, we can decide on our own on what
   * windows we want double buffering.
   */
  if (event->window == widget->window)
    {
      gdk_window_begin_paint_rect (event->window, &area);
      bst_track_roll_draw_window (self, area.x, area.y, area.x + area.width, area.y + area.height);
      GTK_WIDGET_CLASS (bst_track_roll_parent_class)->expose_event (widget, event);
      gdk_window_end_paint (event->window);
    }
  else if (event->window == self->vpanel)
    {
      gdk_window_begin_paint_rect (event->window, &area);
      bst_track_roll_draw_vpanel (self, area.y, area.y + area.height);
      GTK_WIDGET_CLASS (bst_track_roll_parent_class)->expose_event (widget, event);
      gdk_window_end_paint (event->window);
    }
  else if (event->window == self->hpanel)
    {
      bst_track_roll_overlap_grow_hpanel_area (self, &area);
      gdk_window_begin_paint_rect (event->window, &area);
      bst_track_roll_overlap_grow_hpanel_area (self, &area);
      bst_track_roll_draw_hpanel (self, area.x, area.x + area.width);
      GTK_WIDGET_CLASS (bst_track_roll_parent_class)->expose_event (widget, event);
      gdk_window_end_paint (event->window);
    }
  else if (event->window == self->canvas)
    {
      // gdk_window_clear_area (widget->window, area.x, area.y, area.width, area.height);
      gdk_window_begin_paint_rect (event->window, &area);
      bst_track_roll_draw_canvas (self, area.x, area.y, area.x + area.width, area.y + area.height);
      bst_marker_save_backing (&self->vmarker, &area);
      bst_marker_expose (&self->vmarker, &area);
      GTK_WIDGET_CLASS (bst_track_roll_parent_class)->expose_event (widget, event);
      gdk_window_end_paint (event->window);
    }
  else
    GTK_WIDGET_CLASS (bst_track_roll_parent_class)->expose_event (widget, event);
  return FALSE;
}

static void
track_roll_adjustment_changed (BstTrackRoll *self)
{
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
track_roll_adjustment_value_changed (BstTrackRoll  *self,
				     GtkAdjustment *adjustment)
{
  gboolean need_realloc = FALSE;
  if (adjustment == self->hadjustment)
    {
      gint x = self->x_offset, diff;
      self->x_offset = ticks_to_pixels (self, adjustment->value);
      diff = x - self->x_offset;
      if (diff && GTK_WIDGET_DRAWABLE (self))
	{
	  GdkRectangle area = { 0, };

	  gdk_window_scroll (self->hpanel, diff, 0);
	  gdk_window_scroll (self->canvas, diff, 0);
	  bst_marker_scroll (&self->vmarker, diff, 0);
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
          need_realloc = TRUE;
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
	  bst_marker_scroll (&self->vmarker, 0, diff);
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
          need_realloc = TRUE;
	}
    }
  if (need_realloc)
    track_roll_reallocate_children (self);
}

static void
track_roll_update_adjustments (BstTrackRoll *self,
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
      self->hadjustment->step_increment = self->tpt;
      self->hadjustment->page_increment = self->tpt * 4;
      self->hadjustment->value = CLAMP (self->hadjustment->value,
					self->hadjustment->lower,
					self->hadjustment->upper - self->hadjustment->page_size);
      gtk_adjustment_changed (self->hadjustment);
    }
  if (vadj)
    {
#if 0	// FIXME: remove
      self->vadjustment->lower = 0;
      self->vadjustment->upper = OCTAVE_HEIGHT (self) * N_OCTAVES (self);
      self->vadjustment->page_size = CANVAS_HEIGHT (self);
      self->vadjustment->page_increment = self->vadjustment->page_size / 2;
      self->vadjustment->step_increment = OCTAVE_HEIGHT (self) / 7;
      gtk_adjustment_changed (self->vadjustment);
#endif
      self->vadjustment->value = CLAMP (self->vadjustment->value,
					self->vadjustment->lower,
					self->vadjustment->upper - self->vadjustment->page_size);
    }
  if (hv != self->hadjustment->value)
    gtk_adjustment_value_changed (self->hadjustment);
  if (vv != self->vadjustment->value)
    gtk_adjustment_value_changed (self->vadjustment);
}

static void
track_roll_scroll_adjustments (BstTrackRoll *self,
			       gint          x_pixel,
			       gint          y_pixel)
{
  gdouble hv = self->hadjustment->value;
  gdouble vv = self->vadjustment->value;
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
  self->vadjustment->value += ydiff;
  self->vadjustment->value = CLAMP (self->vadjustment->value,
				    self->vadjustment->lower,
				    self->vadjustment->upper - self->vadjustment->page_size);
  if (hv != self->hadjustment->value)
    gtk_adjustment_value_changed (self->hadjustment);
  if (vv != self->vadjustment->value)
    gtk_adjustment_value_changed (self->vadjustment);
}

static void
bst_track_roll_hsetup (BstTrackRoll *self,
		       guint	     tpt,
		       guint	     max_ticks,
		       gdouble	     hzoom)
{
  guint old_tpt = self->tpt;
  guint old_max_ticks = self->max_ticks;
  gdouble old_hzoom = self->hzoom;

  /* here, we setup all things necessary to determine our
   * horizontal layout. we have to avoid resizes at
   * least if just max_ticks changes, since the tick range
   * might need to grow during pointer grabs
   */
  self->tpt = MAX (tpt, 1);
  self->max_ticks = MAX (max_ticks, 1);
  self->hzoom = hzoom / 50;

  if (old_tpt != self->tpt ||
      old_max_ticks != self->max_ticks ||	// FIXME: shouldn't always cause a redraw
      old_hzoom != self->hzoom)
    {
      if (self->hadjustment)
	{
	  self->x_offset = ticks_to_pixels (self, self->hadjustment->value);
	  track_roll_update_adjustments (self, TRUE, FALSE);
	}
      self->draw_tact_grid = ticks_to_pixels (self, self->tpt) >= 3;
      gtk_widget_queue_draw (GTK_WIDGET (self));
    }
}

gdouble
bst_track_roll_set_hzoom (BstTrackRoll *self,
			  gdouble       hzoom)
{
  guint n;

  g_return_val_if_fail (BST_IS_TRACK_ROLL (self), 0);

  hzoom = CLAMP (hzoom, 0.1, 100);
  bst_track_roll_hsetup (self, self->tpt, self->max_ticks, hzoom);
  /* readjust marks */
  for (n = 0; n < self->vmarker.n_marks; n++)
    {
      BstMarker *mark = self->vmarker.marks + n;
      bst_marker_set (&self->vmarker, mark, mark->type, tick_to_coord (self, mark->position));
    }

  return self->hzoom * 50;
}

static void
bst_track_roll_drag_abort (BstTrackRoll *self)
{
  if (self->in_drag)
    {
      self->in_drag = FALSE;
      self->drag.type = BST_DRAG_ABORT;
      g_signal_emit (self, signal_drag, 0, &self->drag);
    }
}

static gboolean
bst_track_roll_drag (BstTrackRoll *self,
		     gint	   coord_x,
		     gint	   coord_y,
		     guint         drag_area)
{
  if (self->in_drag)
    {
      self->drag.current_row = coord_to_row (self, coord_y, &self->drag.current_valid);
      self->drag.current_track = row_to_track (self, self->drag.current_row);
      self->drag.current_tick = coord_to_tick (self, MAX (coord_x, 0), FALSE);
      if (drag_area != DRAG_UNCHANGED)
	{
	  self->drag.canvas_drag = drag_area == DRAG_CANVAS;
	  self->drag.hpanel_drag = drag_area == DRAG_HPANEL;
	  self->drag.vpanel_drag = drag_area == DRAG_VPANEL;
	}
      if (self->drag.type == BST_DRAG_START)
	{
	  self->drag.start_row = self->drag.current_row;
	  self->drag.start_valid = self->drag.current_valid;
	  self->drag.start_track = self->drag.current_track;
	  self->drag.start_tick = self->drag.current_tick;
	  if (self->drag.current_valid && (drag_area == DRAG_CANVAS || drag_area == DRAG_VPANEL))
	    g_signal_emit (self, signal_select_row, 0, self->drag.current_row);
	}
      g_signal_emit (self, signal_drag, 0, &self->drag);
      if (self->drag.state == BST_DRAG_HANDLED)
	self->in_drag = FALSE;
      else if (self->drag.state == BST_DRAG_ERROR)
	bst_track_roll_drag_abort (self);
      else if (self->drag.type == BST_DRAG_START && self->drag.state == BST_DRAG_UNHANDLED)
	return TRUE;
    }
  return FALSE;
}

static gboolean
bst_track_roll_button_press (GtkWidget	    *widget,
			     GdkEventButton *event)
{
  BstTrackRoll *self = BST_TRACK_ROLL (widget);
  gboolean handled = FALSE;

  if (self->ecell)
    {
      bst_track_roll_stop_edit (self);
      return TRUE;
    }
  if (self->in_drag)
    return TRUE;

  if (event->window == self->canvas)
    {
      handled = TRUE;
      self->drag.troll = self;
      self->drag.type = BST_DRAG_START;
      self->drag.mode = bst_drag_modifier_start (event->state);
      self->drag.button = event->button;
      self->drag.state = BST_DRAG_UNHANDLED;
      self->in_drag = TRUE;
      if (bst_track_roll_drag (self, event->x, event->y, DRAG_CANVAS) == TRUE)
	{
	  self->in_drag = FALSE;
	  g_signal_emit (self, signal_clicked, 0, self->drag.button, self->drag.start_row, self->drag.start_tick, event);
	}
    }
  else if (event->window == self->vpanel)
    {
      gboolean is_valid;
      gint current_row = coord_to_row (self, event->y, &is_valid);
      if (is_valid)
	g_signal_emit (self, signal_select_row, 0, current_row);
      handled = TRUE;
    }
  else if (event->window == self->hpanel)
    {
      handled = TRUE;
      self->drag.troll = self;
      self->drag.type = BST_DRAG_START;
      self->drag.mode = bst_drag_modifier_start (event->state);
      self->drag.button = event->button;
      self->drag.state = BST_DRAG_UNHANDLED;
      self->in_drag = TRUE;
      if (bst_track_roll_drag (self, event->x, event->y, DRAG_HPANEL) == TRUE)
	{
          self->in_drag = FALSE;
	  g_signal_emit (self, signal_clicked, 0, self->drag.button, self->drag.start_row, self->drag.start_tick, event);
	}
    }

  return handled;
}

static gboolean
timeout_scroller (gpointer data)
{
  BstTrackRoll *self;
  guint remain = 1;

  GDK_THREADS_ENTER ();
  self = BST_TRACK_ROLL (data);
  if (self->in_drag && self->drag.canvas_drag && GTK_WIDGET_DRAWABLE (self))
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
	  track_roll_scroll_adjustments (self, xdiff, ydiff);
	  self->drag.type = BST_DRAG_MOTION;
	  self->drag.mode = bst_drag_modifier_next (modifiers, self->drag.mode);
	  bst_track_roll_drag (self, x, y, DRAG_UNCHANGED);
	}
      else
	self->scroll_timer = remain = 0;
    }
  else if (self->in_drag && self->drag.vpanel_drag && GTK_WIDGET_DRAWABLE (self))
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
          track_roll_scroll_adjustments (self, 0, ydiff);
          self->drag.type = BST_DRAG_MOTION;
	  self->drag.mode = bst_drag_modifier_next (modifiers, self->drag.mode);
	  bst_track_roll_drag (self, x, y, DRAG_UNCHANGED);
	}
      else
	self->scroll_timer = remain = 0;
    }
  else if (self->in_drag && self->drag.hpanel_drag && GTK_WIDGET_DRAWABLE (self))
    {
      gint x, y, width, xdiff = 0;
      GdkModifierType modifiers;

      gdk_window_get_size (self->hpanel, &width, NULL);
      gdk_window_get_pointer (self->hpanel, &x, &y, &modifiers);
      if (x < 0)
	xdiff = x;
      else if (x >= width)
	xdiff = x - width + 1;
      if (xdiff)
	{
	  track_roll_scroll_adjustments (self, xdiff, 0);
	  self->drag.type = BST_DRAG_MOTION;
	  self->drag.mode = bst_drag_modifier_next (modifiers, self->drag.mode);
	  bst_track_roll_drag (self, x, y, DRAG_UNCHANGED);
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
bst_track_roll_motion (GtkWidget      *widget,
		       GdkEventMotion *event)
{
  BstTrackRoll *self = BST_TRACK_ROLL (widget);
  gboolean handled = FALSE;

  if (self->in_drag && event->window == self->canvas)
    {
      gint width, height;
      handled = TRUE;
      self->drag.type = BST_DRAG_MOTION;
      self->drag.mode = bst_drag_modifier_next (event->state, self->drag.mode);
      bst_track_roll_drag (self, event->x, event->y, DRAG_UNCHANGED);
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
  if (self->in_drag && event->window == self->vpanel)
    {
      gint height;
      handled = TRUE;
      self->drag.type = BST_DRAG_MOTION;
      self->drag.mode = bst_drag_modifier_next (event->state, self->drag.mode);
      bst_track_roll_drag (self, event->x, event->y, DRAG_UNCHANGED);
      gdk_window_get_size (self->vpanel, NULL, &height);
      if (!self->scroll_timer && (event->y < 0 || event->y >= height))
	self->scroll_timer = g_timeout_add_full (G_PRIORITY_DEFAULT,
						 AUTO_SCROLL_TIMEOUT,
						 timeout_scroller,
						 self, NULL);
      /* trigger motion events (since we use motion-hint) */
      gdk_window_get_pointer (self->vpanel, NULL, NULL, NULL);
    }
  if (self->in_drag && event->window == self->hpanel)
    {
      gint width;
      handled = TRUE;
      self->drag.type = BST_DRAG_MOTION;
      self->drag.mode = bst_drag_modifier_next (event->state, self->drag.mode);
      bst_track_roll_drag (self, event->x, event->y, DRAG_UNCHANGED);
      gdk_window_get_size (self->hpanel, &width, NULL);
      if (!self->scroll_timer && (event->x < 0 || event->x >= width))
	self->scroll_timer = g_timeout_add_full (G_PRIORITY_DEFAULT,
						 AUTO_SCROLL_TIMEOUT,
						 timeout_scroller,
						 self, NULL);
      /* trigger motion events (since we use motion-hint) */
      gdk_window_get_pointer (self->hpanel, NULL, NULL, NULL);
    }

  return handled;
}

static gboolean
bst_track_roll_button_release (GtkWidget      *widget,
			       GdkEventButton *event)
{
  BstTrackRoll *self = BST_TRACK_ROLL (widget);
  gboolean handled = FALSE;

  if (self->in_drag && event->button == self->drag.button)
    {
      handled = TRUE;
      self->drag.type = BST_DRAG_DONE;
      self->drag.mode = bst_drag_modifier_next (event->state, self->drag.mode);
      bst_track_roll_drag (self, event->x, event->y, DRAG_UNCHANGED);
      self->in_drag = FALSE;
    }

  return handled;
}

static gboolean
bst_track_roll_key_press (GtkWidget   *widget,
			  GdkEventKey *event)
{
  BstTrackRoll *self = BST_TRACK_ROLL (widget);
  gboolean handled = FALSE;

  if (event->keyval == GDK_Escape)
    {
      bst_track_roll_drag_abort (self);
      handled = TRUE;
    }
  return handled;
}

static gboolean
bst_track_roll_key_release (GtkWidget   *widget,
			    GdkEventKey *event)
{
  // BstTrackRoll *self = BST_TRACK_ROLL (widget);
  gboolean handled = FALSE;

  return handled;
}

void
bst_track_roll_set_canvas_cursor (BstTrackRoll *self,
				  GdkCursorType cursor_type)
{
  g_return_if_fail (BST_IS_TRACK_ROLL (self));

  if (cursor_type != self->canvas_cursor)
    {
      self->canvas_cursor = cursor_type;
      if (GTK_WIDGET_REALIZED (self))
	gxk_window_set_cursor_type (self->canvas, self->canvas_cursor);
    }
}

void
bst_track_roll_set_vpanel_cursor (BstTrackRoll *self,
				  GdkCursorType cursor_type)
{
  g_return_if_fail (BST_IS_TRACK_ROLL (self));

  if (cursor_type != self->vpanel_cursor)
    {
      self->vpanel_cursor = cursor_type;
      if (GTK_WIDGET_REALIZED (self))
	gxk_window_set_cursor_type (self->vpanel, self->vpanel_cursor);
    }
}

void
bst_track_roll_set_hpanel_cursor (BstTrackRoll *self,
				  GdkCursorType cursor_type)
{
  g_return_if_fail (BST_IS_TRACK_ROLL (self));

  if (cursor_type != self->hpanel_cursor)
    {
      self->hpanel_cursor = cursor_type;
      if (GTK_WIDGET_REALIZED (self))
        gxk_window_set_cursor_type (self->hpanel, self->hpanel_cursor);
    }
}

void
bst_track_roll_set_track_callback (BstTrackRoll   *self,
				   gpointer        data,
				   BstTrackRollTrackFunc get_track)
{
  g_return_if_fail (BST_IS_TRACK_ROLL (self));

  self->proxy_data = data;
  self->get_track = get_track;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

void
bst_track_roll_queue_draw_row (BstTrackRoll *self,
			       guint         row)
{
  GdkRectangle rect;

  g_return_if_fail (BST_IS_TRACK_ROLL (self));

  rect.x = 0;
  rect.width = CANVAS_WIDTH (self);
  if (GTK_WIDGET_DRAWABLE (self) &&
      row_to_coords (self, row, &rect.y, &rect.height))
    {
      gdk_window_invalidate_rect (self->canvas, &rect, TRUE);
      rect.x = 0;
      rect.width = VPANEL_WIDTH (self);
      gdk_window_invalidate_rect (self->vpanel, &rect, TRUE);
    }
}

void
bst_track_roll_set_prelight_row (BstTrackRoll *self,
				 guint         row)
{
  g_return_if_fail (BST_IS_TRACK_ROLL (self));

  if (self->prelight_row != row)
    {
      gint clear_row = self->prelight_row;
      self->prelight_row = row;
      bst_track_roll_queue_draw_row (self, clear_row);
      bst_track_roll_queue_draw_row (self, self->prelight_row);
    }
}

static void
bst_track_roll_allocate_ecell (BstTrackRoll *self)
{
  if (GTK_WIDGET_REALIZED (self) && self->ecell)
    {
      GtkAllocation allocation;
      gint ry, rheight, validrow;
      gtk_widget_size_request (GTK_WIDGET (self->ecell), NULL);
      validrow = row_to_coords (self, self->ecell_row, &ry, &rheight);
      if (!validrow)
	{
	  allocation.x = allocation.y = -10;
	  allocation.width = allocation.height = 1;
	}
      else
	{
	  gint w = ticks_to_pixels (self, self->ecell_duration);
	  allocation.x = tick_to_coord (self, self->ecell_tick) + XTHICKNESS (self);
	  allocation.width = MAX (w - 2 * XTHICKNESS (self), 0);
	  allocation.y = ry + 1 + YTHICKNESS (self);
	  allocation.height = MAX (rheight - 2 - 2 * YTHICKNESS (self), 1);
	}
      gtk_widget_size_allocate (GTK_WIDGET (self->ecell), &allocation);
    }
}

void
bst_track_roll_start_edit (BstTrackRoll    *self,
			   guint            row,
			   guint            tick,
			   guint            duration,
			   GtkCellEditable *ecell)
{
  gint ry, rheight, validrow;

  g_return_if_fail (BST_IS_TRACK_ROLL (self));
  g_return_if_fail (GTK_WIDGET_REALIZED (self));
  g_return_if_fail (GTK_IS_CELL_EDITABLE (ecell));
  g_return_if_fail (GTK_WIDGET_CAN_FOCUS (ecell));
  g_return_if_fail (GTK_WIDGET (ecell)->parent == NULL);
  g_return_if_fail (self->ecell == NULL);

  validrow = row_to_coords (self, row, &ry, &rheight);
  if (!validrow)
    {
      gtk_object_sink (GTK_OBJECT (ecell));
      return;
    }
  self->ecell = ecell;
  g_object_connect (self->ecell,
		    "signal::notify::is-focus", gxk_cell_editable_is_focus_handler, self,
		    "swapped_signal::editing_done", bst_track_roll_stop_edit, self,
		    NULL);
  self->ecell_row = row;
  self->ecell_tick = tick;
  self->ecell_duration = duration;
  gtk_widget_set_parent_window (GTK_WIDGET (self->ecell), self->canvas);
  gtk_widget_set_parent (GTK_WIDGET (self->ecell), GTK_WIDGET (self));
  gtk_cell_editable_start_editing (self->ecell, NULL);
  gtk_widget_grab_focus (GTK_WIDGET (self->ecell));
}

static void
track_roll_stop_edit (BstTrackRoll *self,
		      gboolean      canceled)
{
  g_return_if_fail (BST_IS_TRACK_ROLL (self));
  if (self->ecell)
    {
      g_signal_emit (self, signal_stop_edit, 0,
		     canceled || gxk_cell_editable_canceled (self->ecell),
		     self->ecell);
      g_object_disconnect (self->ecell,
			   "any_signal::notify::is-focus", gxk_cell_editable_is_focus_handler, self,
			   "any_signal::editing_done", bst_track_roll_stop_edit, self,
			   NULL);
      gtk_widget_unparent (GTK_WIDGET (self->ecell));
      self->ecell = NULL;
    }
}

void
bst_track_roll_abort_edit (BstTrackRoll *self)
{
  g_return_if_fail (BST_IS_TRACK_ROLL (self));

  track_roll_stop_edit (self, TRUE);
}

void
bst_track_roll_stop_edit (BstTrackRoll *self)
{
  g_return_if_fail (BST_IS_TRACK_ROLL (self));

  track_roll_stop_edit (self, FALSE);
}

void
bst_track_roll_set_mark (BstTrackRoll *self,
                         guint         mark_index,
                         guint         position,
                         BstMarkerType type)
{
  BstMarker *mark;

  g_return_if_fail (BST_IS_TRACK_ROLL (self));
  g_return_if_fail (mark_index > 0);
  if (type)
    g_return_if_fail (position < position + 1);       /* catch guint wraps */

  mark = bst_marker_get (&self->vmarker, mark_index);
  if (!type)
    {
      if (mark)
	{
	  bst_track_roll_expose_mark (self, mark);
	  bst_marker_delete (&self->vmarker, mark);
	}
      return;
    }
  if (!mark)
    mark = bst_marker_add (&self->vmarker, mark_index);
  else
    bst_track_roll_expose_mark (self, mark);
  mark->position = position;
  bst_marker_set (&self->vmarker, mark, type, tick_to_coord (self, mark->position));
  bst_track_roll_expose_mark (self, mark);
}
