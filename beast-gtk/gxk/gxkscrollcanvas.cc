// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "gxkscrollcanvas.hh"
#include "gxkimagecache.hh"
#include <gdk/gdkkeysyms.h>
#include <string.h>


/* --- defines --- */
/* accessors */
#define STYLE(self)               (GTK_WIDGET (self)->style)
#define STATE(self)               (GTK_WIDGET (self)->state)
#define SELECTED_STATE(self)      (GTK_WIDGET_IS_SENSITIVE (self) ? GTK_STATE_SELECTED : GTK_STATE_INSENSITIVE)
#define ACTIVE_STATE(self)        (GTK_WIDGET_IS_SENSITIVE (self) ? GTK_STATE_ACTIVE : GTK_STATE_INSENSITIVE)
#define XTHICKNESS(self)          (STYLE (self)->xthickness)
#define YTHICKNESS(self)          (STYLE (self)->ythickness)
#define ALLOCATION(self)          (&GTK_WIDGET (self)->allocation)
#define BORDER_WIDTH(self)        (GTK_CONTAINER (self)->border_width)
#define LAYOUT(self)              (&(self)->layout)
#define DRAG(self)                ((GxkScrollCanvasDrag*) g_object_get_data ((GObject*) self, "GxkScrollCanvas-drag"))
#define SET_DRAG(self, drag)       g_object_set_data_full ((GObject*) self, "GxkScrollCanvas-drag", drag, g_free)
/* switches */
#define WITH_TOP_PANEL(self)      (LAYOUT (self)->top_panel_height ? 1 : 0)
#define WITH_LEFT_PANEL(self)     (LAYOUT (self)->left_panel_width ? 1 : 0)
#define WITH_RIGHT_PANEL(self)    (LAYOUT (self)->right_panel_width ? 1 : 0)
#define WITH_BOTTOM_PANEL(self)   (LAYOUT (self)->bottom_panel_height ? 1 : 0)
/* layout (requisition) */
#define OFRAME_WIDTH(self)        (XTHICKNESS (self)) /* outline (frame) around panels (shadow-out) */
#define OFRAME_HEIGHT(self)       (YTHICKNESS (self))
#define CFRAME_WIDTH(self)        (XTHICKNESS (self)) /* frame around canvas, within panels (shadow-in) */
#define CFRAME_HEIGHT(self)       (YTHICKNESS (self))
#define TOP_PANEL_OFRAME(self)    (WITH_TOP_PANEL (self) * OFRAME_HEIGHT (self))
#define TOP_PANEL_HEIGHT(self)    (LAYOUT (self)->top_panel_height)
#define TOP_PANEL_CFRAME(self)    (WITH_TOP_PANEL (self) * CFRAME_HEIGHT (self))
#define LEFT_PANEL_OFRAME(self)   (WITH_LEFT_PANEL (self) * OFRAME_WIDTH (self))
#define LEFT_PANEL_WIDTH(self)    (LAYOUT (self)->left_panel_width)
#define LEFT_PANEL_CFRAME(self)   (WITH_LEFT_PANEL (self) * CFRAME_WIDTH (self))
#define RIGHT_PANEL_CFRAME(self)  (WITH_RIGHT_PANEL (self) * CFRAME_WIDTH (self))
#define RIGHT_PANEL_WIDTH(self)   (LAYOUT (self)->right_panel_width)
#define RIGHT_PANEL_OFRAME(self)  (WITH_RIGHT_PANEL (self) * OFRAME_WIDTH (self))
#define BOTTOM_PANEL_CFRAME(self) (WITH_BOTTOM_PANEL (self) * CFRAME_HEIGHT (self))
#define BOTTOM_PANEL_HEIGHT(self) (LAYOUT (self)->bottom_panel_height)
#define BOTTOM_PANEL_OFRAME(self) (WITH_BOTTOM_PANEL (self) * OFRAME_HEIGHT (self))
#define CANVAS_X(self)            (LEFT_PANEL_OFRAME (self) + LEFT_PANEL_WIDTH (self) + LEFT_PANEL_CFRAME (self))
#define CANVAS_Y(self)            (TOP_PANEL_OFRAME (self) + TOP_PANEL_HEIGHT (self) + TOP_PANEL_CFRAME (self))
#define CANVAS_HREST(self)        (RIGHT_PANEL_CFRAME (self) + RIGHT_PANEL_WIDTH (self) + RIGHT_PANEL_OFRAME (self))
#define CANVAS_VREST(self)        (BOTTOM_PANEL_CFRAME (self) + BOTTOM_PANEL_HEIGHT (self) + BOTTOM_PANEL_OFRAME (self))
#define WINDOW_WIDTH(self, rq_w)  MIN (G_MAXINT, CANVAS_X (self) + CANVAS_HREST (self) + (guint) rq_w)
#define WINDOW_HEIGHT(self, rq_h) MIN (G_MAXINT, CANVAS_Y (self) + CANVAS_VREST (self) + (guint) rq_h)
/* layout (allocation) */
#define CANVAS_WIDTH(self)        (ALLOCATION (self)->width - 2 * BORDER_WIDTH (self) - CANVAS_X (self) - CANVAS_HREST (self))
#define CANVAS_HEIGHT(self)       (ALLOCATION (self)->height - 2 * BORDER_WIDTH (self) - CANVAS_Y (self) - CANVAS_VREST (self))
#define RIGHT_PANEL_X(self)       (CANVAS_X (self) + CANVAS_WIDTH (self) + RIGHT_PANEL_CFRAME (self))
#define BOTTOM_PANEL_Y(self)      (CANVAS_Y (self) + CANVAS_HEIGHT (self) + BOTTOM_PANEL_CFRAME (self))
/* aliases */
#define TOP_PANEL_X(self)         (CANVAS_X (self))
#define TOP_PANEL_Y(self)         (TOP_PANEL_OFRAME (self))
#define TOP_PANEL_WIDTH(self)     (CANVAS_WIDTH (self))
#define LEFT_PANEL_X(self)        (LEFT_PANEL_OFRAME (self))
#define LEFT_PANEL_Y(self)        (CANVAS_Y (self))
#define LEFT_PANEL_HEIGHT(self)   (CANVAS_HEIGHT (self))
#define RIGHT_PANEL_Y(self)       (CANVAS_Y (self))
#define RIGHT_PANEL_HEIGHT(self)  (CANVAS_HEIGHT (self))
#define BOTTOM_PANEL_X(self)      (CANVAS_X (self))
#define BOTTOM_PANEL_WIDTH(self)  (CANVAS_WIDTH (self))
/* look */
#define PANEL_BG_COLOR(self)      (&STYLE (self)->bg[GTK_WIDGET_IS_SENSITIVE (self) ? GTK_STATE_NORMAL : GTK_STATE_INSENSITIVE])
#define CANVAS_BG_COLOR(self)     (&STYLE (self)->base[GTK_WIDGET_STATE (self)])
/* behaviour */
#define AUTO_SCROLL_TIMEOUT       (33)
#define AUTO_SCROLL_SCALE       (0.2)


/* --- prototypes --- */
static void scroll_canvas_adjustment_value_changed   (GxkScrollCanvas        *self,
                                                      GtkAdjustment          *adjustment);


/* --- functions --- */
#if 0
typedef enum    /*< skip >*/
{
  /* Gimp style */
  GXK_DRAG_AREA_ENRICH,         /* Start: Shift */
  GXK_DRAG_AREA_REDUCE,         /* Start: Ctrl */
  GXK_DRAG_RATIO_FIXED,         /* Motion: Shift (Fixed Aspect Ratio) */
  GXK_DRAG_OFFSET_CENTERED,     /* Motion: Ctrl  (Center Obj around Offset) */
  /* DND style */
  GXK_DRAG_ACTION_LINK,         /* Shift + Ctrl */
  GXK_DRAG_ACTION_COPY,         /* Ctrl */
  GXK_DRAG_ACTION_MOVE,         /* Shift */
  /* Gnumeric style */
  GXK_DRAG_AREA_ADD,            /* Ctrl */
  GXK_DRAG_AREA_RESIZE_LAST,    /* Shift */
  /* AbiWord Style */
  GXK_DRAG_SELECTION_RESIZE,    /* Shift */
  GXK_DRAG_SELECT_WORD,         /* Ctrl */
  /* Nautilus Style */
  GXK_DRAG_AREA_XOR,            /* Shift or Ctrl */
} GxkDragModifier;
#endif

GxkDragMode
gxk_drag_modifier_start (GdkModifierType key_mods)
{
  return GxkDragMode (0);
}

GxkDragMode
gxk_drag_modifier_next (GdkModifierType key_mods,
                        GxkDragMode     last_drag_mods)
{
  return GxkDragMode (0);
}

G_DEFINE_TYPE (GxkScrollCanvas, gxk_scroll_canvas, GTK_TYPE_CONTAINER);

static void
gxk_scroll_canvas_init (GxkScrollCanvas *self)
{
  GtkWidget *widget = GTK_WIDGET (self);

  GTK_WIDGET_UNSET_FLAGS (self, GTK_NO_WINDOW);
  gtk_widget_set_double_buffered (widget, FALSE);
  gtk_widget_set_redraw_on_allocate (widget, TRUE);
  gtk_widget_show (widget);

  gxk_scroll_canvas_set_hadjustment (self, NULL);
  gxk_scroll_canvas_set_vadjustment (self, NULL);

  gxk_scroll_canvas_get_layout (self, &self->layout);
  gxk_scroll_canvas_set_window_cursor (self, GXK_DEFAULT_CURSOR);
  gxk_scroll_canvas_set_canvas_cursor (self, GXK_DEFAULT_CURSOR);
  gxk_scroll_canvas_set_top_panel_cursor (self, GXK_DEFAULT_CURSOR);
  gxk_scroll_canvas_set_left_panel_cursor (self, GXK_DEFAULT_CURSOR);
  gxk_scroll_canvas_set_right_panel_cursor (self, GXK_DEFAULT_CURSOR);
  gxk_scroll_canvas_set_bottom_panel_cursor (self, GXK_DEFAULT_CURSOR);
}

static void
scroll_canvas_destroy (GtkObject *object)
{
  GxkScrollCanvas *self = GXK_SCROLL_CANVAS (object);

  if (self->scroll_timer)
    g_source_remove (self->scroll_timer);
  self->scroll_timer = 0;

  gxk_scroll_canvas_set_hadjustment (self, NULL);
  gxk_scroll_canvas_set_vadjustment (self, NULL);

  GTK_OBJECT_CLASS (gxk_scroll_canvas_parent_class)->destroy (object);
}

static void
scroll_canvas_finalize (GObject *object)
{
  GxkScrollCanvas *self = GXK_SCROLL_CANVAS (object);
  guint i;

  g_object_unref (self->hadjustment);
  self->hadjustment = NULL;
  g_object_unref (self->vadjustment);
  self->vadjustment = NULL;

  for (i = 0; i < self->n_pango_layouts; i++)
    if (self->pango_layouts[i])
      g_object_unref (self->pango_layouts[i]);
  g_free (self->pango_layouts);
  self->pango_layouts = NULL;
  self->n_pango_layouts = 0;

  if (self->scroll_timer)
    g_source_remove (self->scroll_timer);
  self->scroll_timer = 0;

  g_free (self->markers);
  self->markers = NULL;
  self->n_markers = 0;

  G_OBJECT_CLASS (gxk_scroll_canvas_parent_class)->finalize (object);
}

static void
scroll_canvas_adjustment_changed_handler (GxkScrollCanvas *self,
                                          GtkAdjustment   *adjustment)
{
  GxkScrollCanvasClass *klass = GXK_SCROLL_CANVAS_GET_CLASS (self);
  klass->adjustment_changed (self, adjustment);
}

void
gxk_scroll_canvas_set_hadjustment (GxkScrollCanvas *self,
                                   GtkAdjustment   *adjustment)
{
  g_return_if_fail (GXK_IS_SCROLL_CANVAS (self));
  if (adjustment)
    g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));

  if (self->hadjustment)
    {
      g_object_disconnect (self->hadjustment,
                           "any_signal", scroll_canvas_adjustment_changed_handler, self,
                           "any_signal", scroll_canvas_adjustment_value_changed, self,
                           NULL);
      g_object_unref (self->hadjustment);
      self->hadjustment = NULL;
    }

  if (!adjustment)
    adjustment = (GtkAdjustment*) g_object_new (GTK_TYPE_ADJUSTMENT, NULL);

  self->hadjustment = (GtkAdjustment*) g_object_ref (adjustment);
  gtk_object_sink (GTK_OBJECT (adjustment));
  g_object_connect (self->hadjustment,
                    "swapped_signal::changed", scroll_canvas_adjustment_changed_handler, self,
                    "swapped_signal::value-changed", scroll_canvas_adjustment_value_changed, self,
                    NULL);
}

void
gxk_scroll_canvas_set_vadjustment (GxkScrollCanvas *self,
                                   GtkAdjustment   *adjustment)
{
  g_return_if_fail (GXK_IS_SCROLL_CANVAS (self));
  if (adjustment)
    g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));

  if (self->vadjustment)
    {
      g_object_disconnect (self->vadjustment,
                           "any_signal", scroll_canvas_adjustment_changed_handler, self,
                           "any_signal", scroll_canvas_adjustment_value_changed, self,
                           NULL);
      g_object_unref (self->vadjustment);
      self->vadjustment = NULL;
    }

  if (!adjustment)
    adjustment = (GtkAdjustment*) g_object_new (GTK_TYPE_ADJUSTMENT, NULL);

  self->vadjustment = (GtkAdjustment*) g_object_ref (adjustment);
  gtk_object_sink (GTK_OBJECT (adjustment));
  g_object_connect (self->vadjustment,
                    "swapped_signal::changed", scroll_canvas_adjustment_changed_handler, self,
                    "swapped_signal::value-changed", scroll_canvas_adjustment_value_changed, self,
                    NULL);
}

static void
scroll_canvas_set_scroll_adjustments (GxkScrollCanvas *self,
                                      GtkAdjustment   *hadjustment,
                                      GtkAdjustment   *vadjustment)
{
  if (self->hadjustment != hadjustment)
    gxk_scroll_canvas_set_hadjustment (self, hadjustment);
  if (self->vadjustment != vadjustment)
    gxk_scroll_canvas_set_vadjustment (self, vadjustment);
}

void
gxk_scroll_canvas_reset_pango_layouts (GxkScrollCanvas *self)
{
  g_return_if_fail (GXK_IS_SCROLL_CANVAS (self));
  /* resort to destroying the layouts to force recreation,
   * so things like merging a font_desc with STYLE->font_desc
   * take effect.
   */
  for (uint i = 0; i < self->n_pango_layouts; i++)
    if (self->pango_layouts[i])
      {
        g_object_unref (self->pango_layouts[i]);
        self->pango_layouts[i] = NULL;
      }
  /* force get_layout() size request to recreate required pango_layouts */
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

static void
scroll_canvas_reset_skin_and_style (GxkScrollCanvas *self,
                                    gboolean         relayout)
{
  GxkScrollCanvasClass *klass = GXK_SCROLL_CANVAS_GET_CLASS (self);
  GtkWidget *widget = GTK_WIDGET (self);
  if (GTK_WIDGET_REALIZED (self))
    {
      guint i;
      /* reset backgrounds */
      gdk_window_set_background (widget->window, PANEL_BG_COLOR (self));
      if (self->top_panel)
        gdk_window_set_background (self->top_panel, PANEL_BG_COLOR (self));
      if (self->left_panel)
        gdk_window_set_background (self->left_panel, PANEL_BG_COLOR (self));
      if (self->right_panel)
        gdk_window_set_background (self->right_panel, PANEL_BG_COLOR (self));
      if (self->bottom_panel)
        gdk_window_set_background (self->bottom_panel, PANEL_BG_COLOR (self));
      gdk_window_set_background (self->canvas, CANVAS_BG_COLOR (self));
      gtk_widget_queue_draw (widget);
      /* reset background pixmap */
      if (self->canvas_pixmap)
        gxk_image_cache_unuse_pixmap (self->canvas_pixmap);
      self->canvas_pixmap = gxk_image_cache_use_pixmap (klass->image_file_name,
                                                        klass->image_tint, klass->image_saturation,
                                                        gdk_drawable_get_colormap (self->canvas));
      /* revert to an empty pixbuf to let the shading still take effect */
      if (!self->canvas_pixmap)
        self->canvas_pixmap = gxk_image_cache_use_pixmap (GXK_IMAGE_BLACK32,
                                                          klass->image_tint, klass->image_saturation,
                                                          gdk_drawable_get_colormap (self->canvas));
      /* reset colors */
      for (i = 0; i < klass->n_colors; i++)
        {
          GdkColor color = klass->colors[i];
          gdk_gc_set_rgb_fg_color (self->color_gc[i], &color);
        }
      /* reset pango layouts */
      if (relayout)
        gxk_scroll_canvas_reset_pango_layouts (self);
      else
        {
          for (i = 0; i < self->n_pango_layouts; i++)
            if (self->pango_layouts[i])
              pango_layout_context_changed (self->pango_layouts[i]);
        }
    }
}

static void
scroll_canvas_style_set (GtkWidget *widget,
                         GtkStyle  *previous_style)
{
  GxkScrollCanvas *self = GXK_SCROLL_CANVAS (widget);
  scroll_canvas_reset_skin_and_style (self, TRUE);
  GTK_WIDGET_CLASS (gxk_scroll_canvas_parent_class)->style_set (widget, previous_style);
}

static void
scroll_canvas_state_changed (GtkWidget   *widget,
                             GtkStateType previous_state)
{
  GxkScrollCanvas *self = GXK_SCROLL_CANVAS (widget);
  scroll_canvas_reset_skin_and_style (self, TRUE);
  GTK_WIDGET_CLASS (gxk_scroll_canvas_parent_class)->state_changed (widget, previous_state);
}

static void
scroll_canvas_direction_changed (GtkWidget        *widget,
                                 GtkTextDirection  previous_direction)
{
  GxkScrollCanvas *self = GXK_SCROLL_CANVAS (widget);
  scroll_canvas_reset_skin_and_style (self, TRUE);
  GTK_WIDGET_CLASS (gxk_scroll_canvas_parent_class)->direction_changed (widget, previous_direction);
}

void
gxk_scroll_canvas_get_layout (GxkScrollCanvas       *self,
                              GxkScrollCanvasLayout *layout)
{
  GxkScrollCanvasClass *klass = GXK_SCROLL_CANVAS_GET_CLASS (self);
  GxkScrollCanvasLayout tlayout;
  tlayout.top_panel_height = 10;
  tlayout.left_panel_width = 10;
  tlayout.right_panel_width = 10;
  tlayout.bottom_panel_height = 10;
  tlayout.canvas_width = 20;
  tlayout.canvas_height = 20;
  tlayout.max_canvas_width = G_MAXINT;
  tlayout.max_canvas_height = G_MAXINT;
  if (klass->get_layout)
    {
      klass->get_layout (self, &tlayout);
      tlayout.top_panel_height = MAX (0, tlayout.top_panel_height);
      tlayout.left_panel_width = MAX (0, tlayout.left_panel_width);
      tlayout.right_panel_width = MAX (0, tlayout.right_panel_width);
      tlayout.bottom_panel_height = MAX (0, tlayout.bottom_panel_height);
      tlayout.canvas_width = MAX (1, tlayout.canvas_width);
      tlayout.canvas_height = MAX (1, tlayout.canvas_height);
      if (tlayout.max_canvas_width < 1)
        tlayout.max_canvas_width = G_MAXINT;
      if (tlayout.max_canvas_height < 1)
        tlayout.max_canvas_height = G_MAXINT;
    }
  *layout = tlayout;
}

static void
child_request_size (GtkWidget *child,
                    gpointer   data)
{
  gtk_widget_size_request (child, NULL);
}

static void
scroll_canvas_size_request (GtkWidget      *widget,
                            GtkRequisition *requisition)
{
  GxkScrollCanvas *self = GXK_SCROLL_CANVAS (widget);
  gtk_container_forall (GTK_CONTAINER (self), child_request_size, NULL);
  gxk_scroll_canvas_get_layout (self, &self->layout);
  requisition->width = 2 * BORDER_WIDTH (self) + WINDOW_WIDTH (self, LAYOUT (self)->canvas_width);
  requisition->height = 2 * BORDER_WIDTH (self) + WINDOW_HEIGHT (self, LAYOUT (self)->canvas_height);
}

static void
scroll_canvas_size_allocate (GtkWidget     *widget,
                             GtkAllocation *allocation)
{
  GxkScrollCanvas *self = GXK_SCROLL_CANVAS (widget);
  widget->allocation.x = allocation->x;
  widget->allocation.y = allocation->y;
  widget->allocation.width = MAX (allocation->width, 2 * BORDER_WIDTH (self) + WINDOW_WIDTH (self, 1));
  widget->allocation.width = MIN (widget->allocation.width,
                                  2 * BORDER_WIDTH (self) +
                                  WINDOW_WIDTH (self, LAYOUT (self)->max_canvas_width));
  widget->allocation.height = MAX (allocation->height, 2 * BORDER_WIDTH (self) + WINDOW_HEIGHT (self, 1));
  widget->allocation.height = MIN (widget->allocation.height,
                                   2 * BORDER_WIDTH (self) +
                                   WINDOW_HEIGHT (self, LAYOUT (self)->max_canvas_height));
  gxk_scroll_canvas_reallocate (self);
}

void
gxk_scroll_canvas_get_canvas_size (GxkScrollCanvas       *self,
                                   gint                  *width,
                                   gint                  *height)
{
  if (self->canvas)                     /* realized */
    gdk_window_get_size (self->canvas, width, height);
  if (ALLOCATION (self)->width > 1 ||
      ALLOCATION (self)->height > 1)    /* allocated */
    {
      if (width)
        *width = CANVAS_WIDTH (self);
      if (height)
        *height = CANVAS_HEIGHT (self);
    }
  else                  /* all we have is a requisition */
    {
      if (width)
        *width = self->layout.canvas_width;
      if (height)
        *height = self->layout.canvas_height;
    }
}

void
gxk_scroll_canvas_reallocate (GxkScrollCanvas *self)
{
  GxkScrollCanvasClass *klass;
  g_return_if_fail (GXK_IS_SCROLL_CANVAS (self));
  klass = GXK_SCROLL_CANVAS_GET_CLASS (self);

  /* one reason we could be called is simply that self->layout has changed */
  gxk_scroll_canvas_get_layout (self, &self->layout);
  if (GTK_WIDGET_REALIZED (self))
    {
      GtkWidget *widget = GTK_WIDGET (self);
      gdk_window_move_resize (widget->window,
                              widget->allocation.x + BORDER_WIDTH (self),
                              widget->allocation.y + BORDER_WIDTH (self),
                              widget->allocation.width - 2 * BORDER_WIDTH (self),
                              widget->allocation.height - 2 * BORDER_WIDTH (self));
      if (self->top_panel)
        gdk_window_move_resize (self->top_panel,
                                TOP_PANEL_X (self), TOP_PANEL_Y (self),
                                TOP_PANEL_WIDTH (self), TOP_PANEL_HEIGHT (self));
      if (self->left_panel)
        gdk_window_move_resize (self->left_panel,
                                LEFT_PANEL_X (self), LEFT_PANEL_Y (self),
                                LEFT_PANEL_WIDTH (self), LEFT_PANEL_HEIGHT (self));
      if (self->right_panel)
        gdk_window_move_resize (self->right_panel,
                                RIGHT_PANEL_X (self), RIGHT_PANEL_Y (self),
                                RIGHT_PANEL_WIDTH (self), RIGHT_PANEL_HEIGHT (self));
      if (self->bottom_panel)
        gdk_window_move_resize (self->bottom_panel,
                                BOTTOM_PANEL_X (self), BOTTOM_PANEL_Y (self),
                                BOTTOM_PANEL_WIDTH (self), BOTTOM_PANEL_HEIGHT (self));
      gdk_window_move_resize (self->canvas,
                              CANVAS_X (self), CANVAS_Y (self),
                              CANVAS_WIDTH (self), CANVAS_HEIGHT (self));
    }
  if (klass->reallocate_contents)
    klass->reallocate_contents (self, 0, 0);
  gxk_scroll_canvas_update_adjustments (self, TRUE, TRUE);
}

static void
scroll_canvas_realize_marker (GxkScrollCanvas *self,
                              GxkScrollMarker *marker)
{
  if (GTK_WIDGET_REALIZED (self) && !marker->pixmap && marker->windowp &&
      *marker->windowp && marker->extends.width && marker->extends.height)
    {
      marker->pixmap = gdk_pixmap_new (*marker->windowp,
                                       marker->extends.width,
                                       marker->extends.height, -1);
      gdk_window_invalidate_rect (*marker->windowp, &marker->extends, TRUE);
    }
}

static void
scroll_canvas_realize (GtkWidget *widget)
{
  GxkScrollCanvas *self = GXK_SCROLL_CANVAS (widget);
  GxkScrollCanvasClass *klass = GXK_SCROLL_CANVAS_GET_CLASS (self);
  GdkWindowAttr attributes;
  guint i, attributes_mask;
  GdkEventMask panel_events = ((GdkEventMask) gtk_widget_get_events (widget) |
                               GDK_EXPOSURE_MASK |
                               GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK |
                               GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                               GDK_BUTTON_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);
  panel_events &= ~(GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

  /* widget->window */
  attributes.x = widget->allocation.x + BORDER_WIDTH (self);
  attributes.y = widget->allocation.y + BORDER_WIDTH (self);
  attributes.width = widget->allocation.width - 2 * BORDER_WIDTH (self);
  attributes.height = widget->allocation.height - 2 * BORDER_WIDTH (self);
  attributes.event_mask = gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK;
  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
  gdk_window_set_user_data (widget->window, self);
  gxk_window_set_cursor_type (widget->window, (GdkCursorType) g_object_get_long (self, "window_cursor"));

  /* self->top_panel */
  if (LAYOUT (self)->top_panel_height)
    {
      attributes.x = TOP_PANEL_X (self);
      attributes.y = TOP_PANEL_Y (self);
      attributes.width = TOP_PANEL_WIDTH (self);
      attributes.height = TOP_PANEL_HEIGHT (self);
      attributes.event_mask = panel_events | klass->top_panel_events;
      self->top_panel = gdk_window_new (widget->window, &attributes, attributes_mask);
      gdk_window_set_user_data (self->top_panel, self);
      gxk_window_set_cursor_type (self->top_panel, (GdkCursorType) g_object_get_long (self, "top_panel_cursor"));
      gdk_window_show (self->top_panel);
    }

  /* self->left_panel */
  if (LAYOUT (self)->left_panel_width)
    {
      attributes.x = LEFT_PANEL_X (self);
      attributes.y = LEFT_PANEL_Y (self);
      attributes.width = LEFT_PANEL_WIDTH (self);
      attributes.height = LEFT_PANEL_HEIGHT (self);
      attributes.event_mask = panel_events | klass->left_panel_events;
      self->left_panel = gdk_window_new (widget->window, &attributes, attributes_mask);
      gdk_window_set_user_data (self->left_panel, self);
      gxk_window_set_cursor_type (self->left_panel, (GdkCursorType) g_object_get_long (self, "left_panel_cursor"));
      gdk_window_show (self->left_panel);
    }

  /* self->right_panel */
  if (LAYOUT (self)->right_panel_width)
    {
      attributes.x = RIGHT_PANEL_X (self);
      attributes.y = RIGHT_PANEL_Y (self);
      attributes.width = RIGHT_PANEL_WIDTH (self);
      attributes.height = RIGHT_PANEL_HEIGHT (self);
      attributes.event_mask = panel_events | klass->right_panel_events;
      self->right_panel = gdk_window_new (widget->window, &attributes, attributes_mask);
      gdk_window_set_user_data (self->right_panel, self);
      gxk_window_set_cursor_type (self->right_panel, (GdkCursorType) g_object_get_long (self, "right_panel_cursor"));
      gdk_window_show (self->right_panel);
    }

  /* self->bottom_panel */
  if (LAYOUT (self)->bottom_panel_height)
    {
      attributes.x = BOTTOM_PANEL_X (self);
      attributes.y = BOTTOM_PANEL_Y (self);
      attributes.width = BOTTOM_PANEL_WIDTH (self);
      attributes.height = BOTTOM_PANEL_HEIGHT (self);
      attributes.event_mask = panel_events | klass->bottom_panel_events;
      self->bottom_panel = gdk_window_new (widget->window, &attributes, attributes_mask);
      gdk_window_set_user_data (self->bottom_panel, self);
      gxk_window_set_cursor_type (self->bottom_panel, (GdkCursorType) g_object_get_long (self, "bottom_panel_cursor"));
      gdk_window_show (self->bottom_panel);
    }

  /* self->canvas */
  attributes.x = CANVAS_X (self);
  attributes.y = CANVAS_Y (self);
  attributes.width = CANVAS_WIDTH (self);
  attributes.height = CANVAS_HEIGHT (self);
  attributes.event_mask = panel_events | klass->canvas_events;
  self->canvas = gdk_window_new (widget->window, &attributes, attributes_mask);
  gdk_window_set_user_data (self->canvas, self);
  gxk_window_set_cursor_type (self->canvas, (GdkCursorType) g_object_get_long (self, "canvas_cursor"));
  gdk_window_show (self->canvas);

  /* setup style, allocate colors */
  widget->style = gtk_style_attach (widget->style, widget->window);
  self->color_gc = g_new0 (GdkGC*, klass->n_colors);
  for (i = 0; i < klass->n_colors; i++)
    self->color_gc[i] = gdk_gc_new (widget->window);

  /* catch skin changes */
  klass->realized_widgets = g_slist_prepend (klass->realized_widgets, self);

  /* setup backgrounds, colors and pango layouts */
  scroll_canvas_reset_skin_and_style (self, FALSE);

  /* realize markers */
  for (i = 0; i < self->n_markers; i++)
    scroll_canvas_realize_marker (self, self->markers + i);

  /* update sizes */
  gxk_scroll_canvas_reallocate (self);
}

static void
scroll_canvas_unrealize_marker (GxkScrollCanvas *self,
                                GxkScrollMarker *marker,
                                gboolean         restore_background)
{
  if (marker->pixmap)
    {
      if (restore_background)
        gxk_scroll_canvas_move_marker (self, marker, -marker->extends.width, -marker->extends.height);
      if (*marker->windowp)
        gdk_window_invalidate_rect (*marker->windowp, &marker->extends, TRUE);
      g_object_unref (marker->pixmap);
      marker->pixmap = NULL;
    }
}

static void
scroll_canvas_unrealize (GtkWidget *widget)
{
  GxkScrollCanvas *self = GXK_SCROLL_CANVAS (widget);
  GxkScrollCanvasClass *klass = GXK_SCROLL_CANVAS_GET_CLASS (self);
  guint i;

  /* ignore skin changes */
  klass->realized_widgets = g_slist_remove (klass->realized_widgets, self);

  /* release color GCs */
  for (i = 0; i < klass->n_colors; i++)
    g_object_unref (self->color_gc[i]);
  g_free (self->color_gc);
  self->color_gc = NULL;

  /* release pango layouts */
  for (i = 0; i < self->n_pango_layouts; i++)
    if (self->pango_layouts[i])
      {
        g_object_unref (self->pango_layouts[i]);
        self->pango_layouts[i] = NULL;
      }

  /* unrealize markers */
  for (i = 0; i < self->n_markers; i++)
    scroll_canvas_unrealize_marker (self, self->markers + i, FALSE);

  /* destroy windows */
  gdk_window_set_user_data (self->canvas, NULL);
  gdk_window_destroy (self->canvas);
  self->canvas = NULL;
  if (self->top_panel)
    {
      gdk_window_set_user_data (self->top_panel, NULL);
      gdk_window_destroy (self->top_panel);
      self->top_panel = NULL;
    }
  if (self->left_panel)
    {
      gdk_window_set_user_data (self->left_panel, NULL);
      gdk_window_destroy (self->left_panel);
      self->left_panel = NULL;
    }
  if (self->right_panel)
    {
      gdk_window_set_user_data (self->right_panel, NULL);
      gdk_window_destroy (self->right_panel);
      self->right_panel = NULL;
    }
  if (self->bottom_panel)
    {
      gdk_window_set_user_data (self->bottom_panel, NULL);
      gdk_window_destroy (self->bottom_panel);
      self->bottom_panel = NULL;
    }
  if (self->canvas_pixmap)
    {
      gxk_image_cache_unuse_pixmap (self->canvas_pixmap);
      self->canvas_pixmap = NULL;
    }

  /* unset parent windows of children */
  gtk_container_forall (GTK_CONTAINER (self), (GtkCallback) gtk_widget_set_parent_window, NULL);

  GTK_WIDGET_CLASS (gxk_scroll_canvas_parent_class)->unrealize (widget);
}

static void
scroll_canvas_draw_window (GxkScrollCanvas        *self,
                           GdkWindow              *drawable,
                           GdkRectangle           *area)
{
  GtkWidget *widget = GTK_WIDGET (self);
  // GdkGC *fg_gc = STYLE (self)->fg_gc[STATE (self)];
  gint x, y, width, height, xb, yb;

  /* frame: upper-left */
  x = 0, y = 0, xb = CANVAS_X (self), yb = CANVAS_Y (self);
  gtk_paint_shadow (widget->style, drawable, GtkStateType (widget->state), GTK_SHADOW_ETCHED_IN,
                    NULL, NULL, NULL, x, y, xb - x, yb - y);
  /* frame: upper-right */
  x = CANVAS_X (self) + CANVAS_WIDTH (self), y = 0, width = CANVAS_HREST (self), yb = CANVAS_Y (self);
  gtk_paint_shadow (widget->style, drawable, GtkStateType (widget->state), GTK_SHADOW_ETCHED_IN,
                    NULL, NULL, NULL, x, y, width, yb - y);
  /* frame: lower-left */
  x = 0, y = CANVAS_Y (self) + CANVAS_HEIGHT (self), xb = CANVAS_X (self), height = CANVAS_VREST (self);
  gtk_paint_shadow (widget->style, drawable, GtkStateType (widget->state), GTK_SHADOW_ETCHED_IN,
                    NULL, NULL, NULL, x, y, xb - x, height);
  /* frame: lower-right */
  x = CANVAS_X (self) + CANVAS_WIDTH (self), y = CANVAS_Y (self) + CANVAS_HEIGHT (self);
  width = CANVAS_HREST (self), height = CANVAS_VREST (self);
  gtk_paint_shadow (widget->style, drawable, GtkStateType (widget->state), GTK_SHADOW_ETCHED_IN,
                    NULL, NULL, NULL, x, y, width, height);
  /* frame: top-panel */
  x = TOP_PANEL_X (self), y = TOP_PANEL_Y (self), width = TOP_PANEL_WIDTH (self), height = TOP_PANEL_HEIGHT (self);
  gtk_paint_shadow (widget->style, drawable, GtkStateType (widget->state), GTK_SHADOW_OUT,
                    NULL, NULL, NULL, x, y - YTHICKNESS (self), width, height + 2 * YTHICKNESS (self));
  /* frame: left-panel */
  x = LEFT_PANEL_X (self), y = LEFT_PANEL_Y (self), width = LEFT_PANEL_WIDTH (self), height = LEFT_PANEL_HEIGHT (self);
  gtk_paint_shadow (widget->style, drawable, GtkStateType (widget->state), GTK_SHADOW_OUT,
                    NULL, NULL, NULL, x - XTHICKNESS (self), y, width + 2 * XTHICKNESS (self), height);
  /* frame: right-panel */
  x = RIGHT_PANEL_X (self), y = RIGHT_PANEL_Y (self), width = RIGHT_PANEL_WIDTH (self), height = RIGHT_PANEL_HEIGHT (self);
  gtk_paint_shadow (widget->style, drawable, GtkStateType (widget->state), GTK_SHADOW_OUT,
                    NULL, NULL, NULL, x - XTHICKNESS (self), y, width + 2 * XTHICKNESS (self), height);
  /* frame: bottom-panel */
  x = BOTTOM_PANEL_X (self), y = BOTTOM_PANEL_Y (self), width = BOTTOM_PANEL_WIDTH (self), height = BOTTOM_PANEL_HEIGHT (self);
  gtk_paint_shadow (widget->style, drawable, GtkStateType (widget->state), GTK_SHADOW_OUT,
                    NULL, NULL, NULL, x, y - YTHICKNESS (self), width, height + 2 * YTHICKNESS (self));
  /* frame */
  x = CANVAS_X (self) - XTHICKNESS (self), y = CANVAS_Y (self) - YTHICKNESS (self);
  width = CANVAS_WIDTH (self) + 2 * XTHICKNESS (self), height = CANVAS_HEIGHT (self) + 2 * YTHICKNESS (self);
  gtk_paint_shadow (widget->style, drawable, GtkStateType (widget->state),
                    GTK_SHADOW_IN, NULL, NULL, NULL,
                    x, y, width, height);
  /* outline */
  x = CANVAS_X (self) - LEFT_PANEL_CFRAME (self) - LEFT_PANEL_WIDTH (self) - XTHICKNESS (self);
  y = CANVAS_Y (self) - TOP_PANEL_CFRAME (self) - TOP_PANEL_HEIGHT (self) - YTHICKNESS (self);
  xb = CANVAS_X (self) + CANVAS_WIDTH (self) + RIGHT_PANEL_CFRAME (self) + RIGHT_PANEL_WIDTH (self) + XTHICKNESS (self);
  yb = CANVAS_Y (self) + CANVAS_HEIGHT (self) + BOTTOM_PANEL_CFRAME (self) + BOTTOM_PANEL_HEIGHT (self) + YTHICKNESS (self);
  gtk_paint_shadow (widget->style, drawable, GtkStateType (widget->state),
                    GTK_SHADOW_OUT, NULL, NULL, NULL,
                    x, y, xb - x, yb - y);
}

static void
scroll_canvas_draw_canvas (GxkScrollCanvas        *self,
                           GdkWindow              *drawable,
                           GdkRectangle           *area)
{
  GdkGC *bg_gc = STYLE (self)->bg_gc[STATE (self)];

  if (self->canvas_pixmap)
    {
      gint x, y, pwidth, pheight;
      gdk_gc_set_clip_rectangle (bg_gc, area);
      gdk_drawable_get_size (self->canvas_pixmap, &pwidth, &pheight);
      for (y = area->y; y < area->y + area->height; )
        {
          gint dy = (y + self->y_offset) % pheight;
          gint dheight = pheight - dy;
          for (x = area->x; x < area->x + area->width; )
            {
              gint dx = (x + self->x_offset) % pwidth;
              gint dwidth = pwidth - dx;
              gdk_draw_drawable (drawable, bg_gc, self->canvas_pixmap,
                                 dx, dy, x, y, dwidth, dheight);
              x += dwidth;
            }
          y += dheight;
        }
      gdk_gc_set_clip_rectangle (bg_gc, NULL);
    }
}

static void
scroll_canvas_draw_panel (GxkScrollCanvas        *self,
                          GdkWindow              *drawable,
                          GdkRectangle           *area)
{
  // GtkWidget *widget = GTK_WIDGET (self);
  GdkGC *fg_gc = STYLE (self)->fg_gc[STATE (self)];
  GdkGC *bg_gc = STYLE (self)->bg_gc[STATE (self)];
  gint width, height;
  gdk_drawable_get_size (drawable, &width, &height);

  /* clear background */
  gdk_draw_rectangle (drawable, bg_gc, TRUE, 0, 0, width, height);

  /* debugging stuff */
  if (1)
    {
      gdk_draw_line (drawable, fg_gc, 0, 0, width-1, height-1);
      gdk_draw_line (drawable, fg_gc, width-1, 0, 0, height-1);
    }
}

static void
scroll_canvas_redraw_markers (GxkScrollCanvas *self,
                              GdkWindow       *drawable,
                              GdkRectangle     area)
{
  GxkScrollCanvasClass *klass = GXK_SCROLL_CANVAS_GET_CLASS (self);
  guint i;
  for (i = 0; i < self->n_markers; i++)
    {
      GxkScrollMarker *marker = self->markers + i;
      if (marker->pixmap && *marker->windowp == drawable)
        {
          GdkRectangle isec;
          if (gdk_rectangle_intersect (&area, &marker->extends, &isec))
            {
              gdk_window_begin_paint_rect (*marker->windowp, &isec);
              klass->draw_marker (self, drawable, &isec, marker);
              gdk_window_end_paint (*marker->windowp);
            }
        }
    }
}

static void
scroll_canvas_update_markers (GxkScrollCanvas *self,
                              GdkWindow       *drawable,
                              GdkRectangle     area)
{
  guint i;
  /* save backing */
  for (i = 0; i < self->n_markers; i++)
    {
      GxkScrollMarker *marker = self->markers + i;
      if (marker->pixmap && *marker->windowp == drawable)
        {
          GdkRectangle isec;
          if (gdk_rectangle_intersect (&area, &marker->extends, &isec))
            {
              GdkGC *draw_gc = STYLE (self)->bg_gc[STATE (self)];
              gdk_draw_drawable (marker->pixmap, draw_gc, drawable,
                                 isec.x, isec.y,
                                 isec.x - marker->extends.x,
                                 isec.y - marker->extends.y,
                                 isec.width, isec.height);
            }
        }
    }
  scroll_canvas_redraw_markers (self, drawable, area);
}

static gboolean
scroll_canvas_expose (GtkWidget      *widget,
                      GdkEventExpose *event)
{
  GxkScrollCanvas *self = GXK_SCROLL_CANVAS (widget);
  GxkScrollCanvasClass *klass = GXK_SCROLL_CANVAS_GET_CLASS (self);
  GdkRectangle *areas;
  gint j, n_areas;

  if (!GTK_WIDGET_DRAWABLE (widget))
    return FALSE;

  if (event->region)
    gdk_region_get_rectangles (event->region, &areas, &n_areas);
  else
    {
      n_areas = 1;
      areas = (GdkRectangle*) g_memdup (&event->area, sizeof (event->area));
    }

  /* with gtk_widget_set_double_buffered (self, FALSE) in init and
   * with gdk_window_begin_paint_region()/gdk_window_end_paint()
   * around our redraw functions, we can decide on our own what
   * windows we want double buffering on.
   */
  for (j = 0; j < n_areas; j++)
    {
      GdkRectangle area = areas[j];
      if (event->window == widget->window)
        {
          if (klass->double_buffer_window)
            gdk_window_begin_paint_rect (event->window, &area);
          else if (klass->auto_clear)
            gdk_window_clear_area (event->window, area.x, area.y, area.width, area.height);
          klass->draw_window (self, event->window, &area);
          GTK_WIDGET_CLASS (gxk_scroll_canvas_parent_class)->expose_event (widget, event);
          scroll_canvas_update_markers (self, event->window, areas[j]);
          if (klass->double_buffer_window)
            gdk_window_end_paint (event->window);
        }
      else if (event->window == self->canvas)
        {
          // g_print ("%s: canvas-expose: %d %d %d %d\n", G_OBJECT_TYPE_NAME (self), area.x, area.y, area.width, area.height);
          if (klass->double_buffer_canvas)
            gdk_window_begin_paint_rect (event->window, &area);
          else if (klass->auto_clear)
            gdk_window_clear_area (event->window, area.x, area.y, area.width, area.height);
          klass->draw_canvas (self, event->window, &area);
          GTK_WIDGET_CLASS (gxk_scroll_canvas_parent_class)->expose_event (widget, event);
          scroll_canvas_update_markers (self, event->window, areas[j]);
          if (klass->double_buffer_canvas)
            gdk_window_end_paint (event->window);
        }
      else if (event->window == self->top_panel)
        {
          if (klass->double_buffer_top_panel)
            gdk_window_begin_paint_rect (event->window, &area);
          else if (klass->auto_clear)
            gdk_window_clear_area (event->window, area.x, area.y, area.width, area.height);
          klass->draw_top_panel (self, event->window, &area);
          GTK_WIDGET_CLASS (gxk_scroll_canvas_parent_class)->expose_event (widget, event);
          scroll_canvas_update_markers (self, event->window, areas[j]);
          if (klass->double_buffer_top_panel)
            gdk_window_end_paint (event->window);
        }
      else if (event->window == self->left_panel)
        {
          if (klass->double_buffer_left_panel)
            gdk_window_begin_paint_rect (event->window, &area);
          else if (klass->auto_clear)
            gdk_window_clear_area (event->window, area.x, area.y, area.width, area.height);
          klass->draw_left_panel (self, event->window, &area);
          GTK_WIDGET_CLASS (gxk_scroll_canvas_parent_class)->expose_event (widget, event);
          scroll_canvas_update_markers (self, event->window, areas[j]);
          if (klass->double_buffer_left_panel)
            gdk_window_end_paint (event->window);
        }
      else if (event->window == self->right_panel)
        {
          if (klass->double_buffer_right_panel)
            gdk_window_begin_paint_rect (event->window, &area);
          else if (klass->auto_clear)
            gdk_window_clear_area (event->window, area.x, area.y, area.width, area.height);
          klass->draw_right_panel (self, event->window, &area);
          GTK_WIDGET_CLASS (gxk_scroll_canvas_parent_class)->expose_event (widget, event);
          scroll_canvas_update_markers (self, event->window, areas[j]);
          if (klass->double_buffer_right_panel)
            gdk_window_end_paint (event->window);
        }
      else if (event->window == self->bottom_panel)
        {
          if (klass->double_buffer_bottom_panel)
            gdk_window_begin_paint_rect (event->window, &area);
          else if (klass->auto_clear)
            gdk_window_clear_area (event->window, area.x, area.y, area.width, area.height);
          klass->draw_bottom_panel (self, event->window, &area);
          GTK_WIDGET_CLASS (gxk_scroll_canvas_parent_class)->expose_event (widget, event);
          scroll_canvas_update_markers (self, event->window, areas[j]);
          if (klass->double_buffer_bottom_panel)
            gdk_window_end_paint (event->window);
        }
      else
        {
          gdk_window_begin_paint_rect (event->window, &area);
          GTK_WIDGET_CLASS (gxk_scroll_canvas_parent_class)->expose_event (widget, event);
          scroll_canvas_update_markers (self, event->window, areas[j]);
          gdk_window_end_paint (event->window);
        }
    }
  g_free (areas);
  return FALSE;
}

static void
scroll_canvas_draw_focus (GxkScrollCanvas *self)
{
  if (GTK_WIDGET_DRAWABLE (self))
    {
      GtkWidget *widget = GTK_WIDGET (self);
      GdkRectangle rect = { 0, 0 };
      gdk_drawable_get_size (widget->window, &rect.width, &rect.height);
      scroll_canvas_redraw_markers (self, widget->window, rect);
      gdk_drawable_get_size (self->canvas, &rect.width, &rect.height);
      scroll_canvas_redraw_markers (self, self->canvas, rect);
      if (self->top_panel)
        {
          gdk_drawable_get_size (self->top_panel, &rect.width, &rect.height);
          scroll_canvas_redraw_markers (self, self->top_panel, rect);
        }
      if (self->left_panel)
        {
          gdk_drawable_get_size (self->left_panel, &rect.width, &rect.height);
          scroll_canvas_redraw_markers (self, self->left_panel, rect);
        }
      if (self->right_panel)
        {
          gdk_drawable_get_size (self->right_panel, &rect.width, &rect.height);
          scroll_canvas_redraw_markers (self, self->right_panel, rect);
        }
      if (self->bottom_panel)
        {
          gdk_drawable_get_size (self->bottom_panel, &rect.width, &rect.height);
          scroll_canvas_redraw_markers (self, self->bottom_panel, rect);
        }
    }
}

static gboolean
scroll_canvas_focus_in (GtkWidget     *widget,
                        GdkEventFocus *event)
{
  GxkScrollCanvas *self = GXK_SCROLL_CANVAS (widget);
  GTK_WIDGET_SET_FLAGS (self, GTK_HAS_FOCUS);
  scroll_canvas_draw_focus (self);
  return TRUE;
}

static gboolean
scroll_canvas_focus_out (GtkWidget     *widget,
                         GdkEventFocus *event)
{
  GxkScrollCanvas *self = GXK_SCROLL_CANVAS (widget);
  GTK_WIDGET_UNSET_FLAGS (self, GTK_HAS_FOCUS);
  scroll_canvas_draw_focus (self);
  return TRUE;
}

void
gxk_scroll_canvas_set_window_cursor (GxkScrollCanvas *self,
                                     GdkCursorType    cursor)
{
  GdkCursorType ocursor;
  g_return_if_fail (GXK_IS_SCROLL_CANVAS (self));
  ocursor = (GdkCursorType) g_object_get_long (self, "window_cursor");
  if (cursor != ocursor)
    {
      GtkWidget *widget = GTK_WIDGET (self);
      g_object_set_long (self, "window_cursor", cursor);
      if (widget->window)
        gxk_window_set_cursor_type (widget->window, cursor);
    }
}

void
gxk_scroll_canvas_set_canvas_cursor (GxkScrollCanvas *self,
                                     GdkCursorType    cursor)
{
  GdkCursorType ocursor;
  g_return_if_fail (GXK_IS_SCROLL_CANVAS (self));
  ocursor = (GdkCursorType) g_object_get_long (self, "canvas_cursor");
  if (cursor != ocursor)
    {
      g_object_set_long (self, "canvas_cursor", cursor);
      if (self->canvas)
        gxk_window_set_cursor_type (self->canvas, cursor);
    }
}

void
gxk_scroll_canvas_set_top_panel_cursor (GxkScrollCanvas *self,
                                        GdkCursorType    cursor)
{
  GdkCursorType ocursor;
  g_return_if_fail (GXK_IS_SCROLL_CANVAS (self));
  ocursor = (GdkCursorType) g_object_get_long (self, "top_panel_cursor");
  if (cursor != ocursor)
    {
      g_object_set_long (self, "top_panel_cursor", cursor);
      if (self->top_panel)
        gxk_window_set_cursor_type (self->top_panel, cursor);
    }
}

void
gxk_scroll_canvas_set_left_panel_cursor (GxkScrollCanvas *self,
                                         GdkCursorType    cursor)
{
  GdkCursorType ocursor;
  g_return_if_fail (GXK_IS_SCROLL_CANVAS (self));
  ocursor = (GdkCursorType) g_object_get_long (self, "left_panel_cursor");
  if (cursor != ocursor)
    {
      g_object_set_long (self, "left_panel_cursor", cursor);
      if (self->left_panel)
        gxk_window_set_cursor_type (self->left_panel, cursor);
    }
}

void
gxk_scroll_canvas_set_right_panel_cursor (GxkScrollCanvas *self,
                                          GdkCursorType    cursor)
{
  GdkCursorType ocursor;
  g_return_if_fail (GXK_IS_SCROLL_CANVAS (self));
  ocursor = (GdkCursorType) g_object_get_long (self, "right_panel_cursor");
  if (cursor != ocursor)
    {
      g_object_set_long (self, "right_panel_cursor", cursor);
      if (self->right_panel)
        gxk_window_set_cursor_type (self->right_panel, cursor);
    }
}

void
gxk_scroll_canvas_set_bottom_panel_cursor (GxkScrollCanvas *self,
                                           GdkCursorType    cursor)
{
  GdkCursorType ocursor;
  g_return_if_fail (GXK_IS_SCROLL_CANVAS (self));
  ocursor = (GdkCursorType) g_object_get_long (self, "bottom_panel_cursor");
  if (cursor != ocursor)
    {
      g_object_set_long (self, "bottom_panel_cursor", cursor);
      if (self->bottom_panel)
        gxk_window_set_cursor_type (self->bottom_panel, cursor);
    }
}

PangoLayout*
gxk_scroll_canvas_get_pango_layout (GxkScrollCanvas        *self,
                                    guint                   nth)
{
  guint i;
  g_return_val_if_fail (GXK_IS_SCROLL_CANVAS (self), NULL);

  if (nth >= self->n_pango_layouts)
    {
      i = self->n_pango_layouts;
      self->n_pango_layouts = nth + 1;
      self->pango_layouts = g_renew (PangoLayout*, self->pango_layouts, self->n_pango_layouts);
      for (; i < self->n_pango_layouts; i++)
        self->pango_layouts[i] = NULL;
    }
  if (!self->pango_layouts[nth])
    {
      self->pango_layouts[nth] = gtk_widget_create_pango_layout (GTK_WIDGET (self), "");
      pango_layout_set_single_paragraph_mode (self->pango_layouts[nth], TRUE);
    }
  return self->pango_layouts[nth];
}

PangoLayout*
gxk_scroll_canvas_peek_pango_layout (GxkScrollCanvas        *self,
                                     guint                   nth)
{
  g_return_val_if_fail (GXK_IS_SCROLL_CANVAS (self), NULL);
  return nth < self->n_pango_layouts ? self->pango_layouts[nth] : NULL;
}

static void
scroll_canvas_adjustment_changed (GxkScrollCanvas *self,
                                  GtkAdjustment   *adjustment)
{
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
scroll_canvas_adjustment_value_changed (GxkScrollCanvas *self,
                                        GtkAdjustment   *adjustment)
{
  GxkScrollCanvasClass *klass = GXK_SCROLL_CANVAS_GET_CLASS (self);
  gint xdiff = 0, ydiff = 0;
  if (klass->hscrollable)
    {
      gint x = self->x_offset;
      self->x_offset = self->hadjustment->value;
      xdiff = x - self->x_offset;
    }
  if (klass->vscrollable)
    {
      gint y = self->y_offset;
      self->y_offset = self->vadjustment->value;
      ydiff = y - self->y_offset;
    }
  if ((xdiff || ydiff) && GTK_WIDGET_DRAWABLE (self))
    {
      if (self->top_panel)
        gdk_window_scroll (self->top_panel, xdiff, 0);
      if (self->left_panel)
        gdk_window_scroll (self->left_panel, 0, ydiff);
      gdk_window_scroll (self->canvas, xdiff, ydiff);
      if (self->right_panel)
        gdk_window_scroll (self->right_panel, 0, ydiff);
      if (self->bottom_panel)
        gdk_window_scroll (self->bottom_panel, xdiff, 0);
    }
  if (xdiff || ydiff)
    {
      /* adjust marker coordinates */
      for (uint i = 0; i < self->n_markers; i++)
        {
          GxkScrollMarker *marker = self->markers + i;
          if (xdiff && (marker->windowp == &self->canvas ||
                        marker->windowp == &self->top_panel ||
                        marker->windowp == &self->bottom_panel))
            marker->extends.x += xdiff;
          if (ydiff && (marker->windowp == &self->canvas ||
                        marker->windowp == &self->left_panel ||
                        marker->windowp == &self->right_panel))
            marker->extends.y += ydiff;
        }
      if (klass->reallocate_contents)
        klass->reallocate_contents (self, xdiff, ydiff);
    }
}

void
gxk_scroll_canvas_update_adjustments (GxkScrollCanvas *self,
                                      gboolean         hadj,
                                      gboolean         vadj)
{
  GxkScrollCanvasClass *klass = GXK_SCROLL_CANVAS_GET_CLASS (self);
  if (hadj && klass->hscrollable)
    {
      self->hadjustment->lower = MAX (self->hadjustment->lower, 0);
      self->hadjustment->upper = CLAMP (self->hadjustment->upper, self->hadjustment->lower, G_MAXINT);
      self->hadjustment->page_size = CANVAS_WIDTH (self);
      self->hadjustment->page_size = CLAMP (self->hadjustment->page_size, 0,
                                            self->hadjustment->upper - self->hadjustment->lower);
      self->hadjustment->page_increment = self->hadjustment->page_size / 2;
      self->hadjustment->step_increment = CLAMP (self->hadjustment->step_increment, 0, self->hadjustment->page_increment);
    }
  if (vadj && klass->vscrollable)
    {
      self->vadjustment->lower = MAX (self->vadjustment->lower, 0);
      self->vadjustment->upper = CLAMP (self->vadjustment->upper, self->vadjustment->lower, G_MAXINT);
      self->vadjustment->page_size = CANVAS_HEIGHT (self);
      self->vadjustment->page_size = CLAMP (self->vadjustment->page_size, 0,
                                            self->vadjustment->upper - self->vadjustment->lower);
      self->vadjustment->page_increment = self->vadjustment->page_size / 2;
      self->vadjustment->step_increment = CLAMP (self->vadjustment->step_increment, 0, self->vadjustment->page_increment);
    }
  GXK_SCROLL_CANVAS_GET_CLASS (self)->update_adjustments (self, hadj, vadj);
}

static void
scroll_canvas_update_adjustments (GxkScrollCanvas *self,
                                  gboolean         hadj,
                                  gboolean         vadj)
{
  GxkScrollCanvasClass *klass = GXK_SCROLL_CANVAS_GET_CLASS (self);
  gdouble hv = self->hadjustment->value;
  gdouble vv = self->vadjustment->value;

  if (hadj && klass->hscrollable)
    {
      self->hadjustment->lower = MAX (self->hadjustment->lower, 0);
      self->hadjustment->upper = CLAMP (self->hadjustment->upper, self->hadjustment->lower, G_MAXINT);
      self->hadjustment->page_size = CANVAS_WIDTH (self);
      self->hadjustment->page_size = CLAMP (self->hadjustment->page_size, 0,
                                            self->hadjustment->upper - self->hadjustment->lower);
      if (self->hadjustment->page_increment <= 0 ||
          self->hadjustment->page_increment > self->hadjustment->page_size)
        self->hadjustment->page_increment = self->hadjustment->page_size / 2;
      self->hadjustment->step_increment = CLAMP (self->hadjustment->step_increment, 0, self->hadjustment->page_increment);
    }
  if (klass->hscrollable)
    self->hadjustment->value = CLAMP (self->hadjustment->value,
                                      self->hadjustment->lower,
                                      MAX (self->hadjustment->upper - self->hadjustment->page_size,
                                           self->hadjustment->lower));
  if (vadj && klass->vscrollable)
    {
      self->vadjustment->lower = MAX (self->vadjustment->lower, 0);
      self->vadjustment->upper = CLAMP (self->vadjustment->upper, self->vadjustment->lower, G_MAXINT);
      self->vadjustment->page_size = CANVAS_HEIGHT (self);
      self->vadjustment->page_size = CLAMP (self->vadjustment->page_size, 0,
                                            self->vadjustment->upper - self->vadjustment->lower);
      if (self->vadjustment->page_increment <= 0 ||
          self->vadjustment->page_increment > self->vadjustment->page_size)
        self->vadjustment->page_increment = self->vadjustment->page_size / 2;
      self->vadjustment->step_increment = CLAMP (self->vadjustment->step_increment, 0, self->vadjustment->page_increment);
    }
  if (klass->vscrollable)
    self->vadjustment->value = CLAMP (self->vadjustment->value,
                                      self->vadjustment->lower,
                                      MAX (self->vadjustment->upper - self->vadjustment->page_size,
                                           self->hadjustment->lower));
  if (hadj && klass->hscrollable)
    gtk_adjustment_changed (self->hadjustment);
  if (vadj && klass->vscrollable)
    gtk_adjustment_changed (self->vadjustment);
  if (hv != self->hadjustment->value && klass->hscrollable)
    gtk_adjustment_value_changed (self->hadjustment);
  if (vv != self->vadjustment->value && klass->vscrollable)
    gtk_adjustment_value_changed (self->vadjustment);
}

static void
scroll_canvas_change_adjustments (GxkScrollCanvas *self,
                                  gint             xval,
                                  gint             yval)
{
  GxkScrollCanvasClass *klass = GXK_SCROLL_CANVAS_GET_CLASS (self);
  gdouble hv = self->hadjustment->value;
  gdouble vv = self->vadjustment->value;

  if (klass->hscrollable)
    {
      self->hadjustment->value = xval;
      self->hadjustment->value = CLAMP (self->hadjustment->value,
                                        self->hadjustment->lower,
                                        MAX (self->hadjustment->upper - self->hadjustment->page_size,
                                             self->hadjustment->lower));
    }
  if (klass->vscrollable)
    {
      self->vadjustment->value = yval;
      self->vadjustment->value = CLAMP (self->vadjustment->value,
                                        self->vadjustment->lower,
                                        MAX (self->vadjustment->upper - self->vadjustment->page_size,
                                             self->vadjustment->lower));
    }
  if (klass->hscrollable && hv != self->hadjustment->value)
    gtk_adjustment_value_changed (self->hadjustment);
  if (klass->vscrollable && vv != self->vadjustment->value)
    gtk_adjustment_value_changed (self->vadjustment);
}

static void
scroll_canvas_scroll_adjustments (GxkScrollCanvas *self,
                                  gint             x_pixel,
                                  gint             y_pixel)
{
  gint xdiff, ydiff;

  xdiff = x_pixel * AUTO_SCROLL_SCALE;
  if (x_pixel > 0)
    xdiff = MAX (xdiff, 1);
  else if (x_pixel < 0)
    xdiff = MIN (-1, xdiff);

  ydiff = y_pixel * AUTO_SCROLL_SCALE;
  if (y_pixel > 0)
    ydiff = MAX (ydiff, 1);
  else if (y_pixel < 0)
    ydiff = MIN (-1, ydiff);

  scroll_canvas_change_adjustments (self, self->hadjustment->value + xdiff, self->vadjustment->value + ydiff);
}

gboolean
gxk_scroll_canvas_dragging (GxkScrollCanvas *self)
{
  g_return_val_if_fail (GXK_IS_SCROLL_CANVAS (self), FALSE);
  GxkScrollCanvasDrag *drag = DRAG (self);
  return drag != NULL;
}

void
gxk_scroll_canvas_drag_abort (GxkScrollCanvas *self)
{
  g_return_if_fail (GXK_IS_SCROLL_CANVAS (self));
  GxkScrollCanvasDrag *drag = DRAG (self);
  if (drag)
    {
      GxkScrollCanvasDrag dcopy = *drag;
      SET_DRAG (self, NULL);
      dcopy.type = GXK_DRAG_ABORT;
      GXK_SCROLL_CANVAS_GET_CLASS (self)->handle_drag (self, &dcopy, NULL);
    }
}

static gboolean
scroll_canvas_drag (GxkScrollCanvas *self,
                    gint             coord_x,
                    gint             coord_y,
                    GdkEvent        *event)
{
  GxkScrollCanvasDrag *drag = DRAG (self);
  if (drag)
    {
      gint width, height;
      gdk_window_get_position (drag->drawable, &width, &height);
      drag->current_x = coord_x;
      drag->current_y = coord_y;
      drag->current_confined = (drag->current_x >= 0 && drag->current_y >= 0 &&
                                drag->current_x < width && drag->current_y < height);
      if (drag->type == GXK_DRAG_START)
        {
          drag->start_x = drag->current_x;
          drag->start_y = drag->current_y;
          drag->start_confined = drag->current_confined;
        }
      GXK_SCROLL_CANVAS_GET_CLASS (self)->handle_drag (self, drag, event);
      if (drag->type == GXK_DRAG_START && drag->state == GXK_DRAG_UNHANDLED)
        {
          SET_DRAG (self, NULL);
          return FALSE; /* unhandled */
        }
      if (drag->state == GXK_DRAG_HANDLED)
        SET_DRAG (self, NULL);
      else if (drag->state == GXK_DRAG_ERROR || !GTK_WIDGET_DRAWABLE (self))
        gxk_scroll_canvas_drag_abort (self);
      return TRUE;
    }
  return FALSE;
}

static gboolean
scroll_canvas_button_press (GtkWidget      *widget,
                            GdkEventButton *event)
{
  GxkScrollCanvas *self = GXK_SCROLL_CANVAS (widget);
  GxkScrollCanvasDrag *drag = DRAG (self);
  gboolean handled = FALSE;

  if (drag)
    return TRUE;

  if (GTK_WIDGET_CAN_FOCUS (self) && !GTK_WIDGET_HAS_FOCUS (self) &&
      GXK_SCROLL_CANVAS_GET_CLASS (self)->grab_focus)
    gtk_widget_grab_focus (widget);

  /* start new drag */
  drag = g_new0 (GxkScrollCanvasDrag, 1);
  drag->widget = widget;
  drag->type = GXK_DRAG_START;
  drag->mode = gxk_drag_modifier_start (GdkModifierType (event->state));
  drag->button = event->button;
  drag->state = GXK_DRAG_UNHANDLED;
  drag->drawable = event->window;
  drag->window_drag = drag->drawable == widget->window;
  drag->canvas_drag = drag->drawable == self->canvas;
  drag->top_panel_drag = drag->drawable == self->top_panel;
  drag->left_panel_drag = drag->drawable == self->left_panel;
  drag->right_panel_drag = drag->drawable == self->right_panel;
  drag->bottom_panel_drag = drag->drawable == self->bottom_panel;

  /* check drag validity */
  if (drag->window_drag || drag->canvas_drag ||
      drag->top_panel_drag || drag->left_panel_drag ||
      drag->right_panel_drag || drag->bottom_panel_drag)
    {
      /* process initial drag event */
      SET_DRAG (self, drag);
      handled = scroll_canvas_drag (self, event->x, event->y, (GdkEvent*) event);
    }
  else  /* very unlikely */
    g_free (drag);

  return handled;
}

static gboolean
scroll_canvas_auto_scroller (gpointer data)
{
  GxkScrollCanvas *self = GXK_SCROLL_CANVAS (data);
  GxkScrollCanvasClass *klass = GXK_SCROLL_CANVAS_GET_CLASS (self);
  guint remain = 1;

  GDK_THREADS_ENTER ();
  GxkScrollCanvasDrag *drag = DRAG (self);
  if (drag && GTK_WIDGET_DRAWABLE (self))
    {
      gboolean hdrag = klass->hscrollable && (drag->canvas_drag || drag->top_panel_drag || drag->bottom_panel_drag);
      gboolean vdrag = klass->vscrollable && (drag->canvas_drag || drag->left_panel_drag || drag->right_panel_drag);
      gint x, y, width, height, xdiff = 0, ydiff = 0;
      GdkModifierType modifiers;
      gdk_window_get_size (drag->drawable, &width, &height);
      gdk_window_get_pointer (drag->drawable, &x, &y, &modifiers);

      if (hdrag && x < 0)
        xdiff = x;
      else if (hdrag && x >= width)
        xdiff = x - width + 1;
      if (vdrag && y < 0)
        ydiff = y;
      else if (vdrag && y >= height)
        ydiff = y - height + 1;

      if (xdiff || ydiff)
        {
          scroll_canvas_scroll_adjustments (self, xdiff, ydiff);
          drag->type = GXK_DRAG_MOTION;
          drag->mode = gxk_drag_modifier_next (modifiers, drag->mode);
          scroll_canvas_drag (self, x, y, NULL);
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
scroll_canvas_motion (GtkWidget      *widget,
                      GdkEventMotion *event)
{
  GxkScrollCanvas *self = GXK_SCROLL_CANVAS (widget);
  GxkScrollCanvasDrag *drag = DRAG (self);
  gboolean handled = FALSE;

  /* process drag event */
  if (drag)
    {
      handled = TRUE;
      drag->type = GXK_DRAG_MOTION;
      drag->mode = gxk_drag_modifier_next (GdkModifierType (event->state), drag->mode);
      scroll_canvas_drag (self, event->x, event->y, (GdkEvent*) event);
      drag = DRAG (self);
    }

  /* start up auto-scroll timer */
  if (drag && !drag->current_confined && !self->scroll_timer)
    {
      self->scroll_timer = g_timeout_add_full (G_PRIORITY_DEFAULT,
                                               AUTO_SCROLL_TIMEOUT,
                                               scroll_canvas_auto_scroller,
                                               self, NULL);
      /* trigger motion events (since we use motion-hint) */
      gdk_window_get_pointer (drag->drawable, NULL, NULL, NULL);
    }

  return handled;
}

static gboolean
scroll_canvas_button_release (GtkWidget      *widget,
                              GdkEventButton *event)
{
  GxkScrollCanvas *self = GXK_SCROLL_CANVAS (widget);
  GxkScrollCanvasDrag *drag = DRAG (self);
  gboolean handled = FALSE;

  if (drag && event->button == drag->button)
    {
      handled = TRUE;
      drag->type = GXK_DRAG_DONE;
      drag->mode = gxk_drag_modifier_next (GdkModifierType (event->state), drag->mode);
      scroll_canvas_drag (self, event->x, event->y, (GdkEvent*) event);
      drag = DRAG (self);
      SET_DRAG (self, NULL);
    }
  else if (drag)
    handled = TRUE;

  return handled;
}

static gboolean
scroll_canvas_key_press (GtkWidget   *widget,
                         GdkEventKey *event)
{
  GxkScrollCanvas *self = GXK_SCROLL_CANVAS (widget);
  GxkScrollCanvasDrag *drag = DRAG (self);
  gboolean handled = FALSE;

  if (drag && event->keyval == GDK_Escape)
    {
      gxk_scroll_canvas_drag_abort (self);
      handled = TRUE;
    }
  return handled;
}

void
gxk_scroll_canvas_scroll_to (GxkScrollCanvas *self,
                             gint             scroll_area_x,
                             gint             scroll_area_y)
{
  g_return_if_fail (GXK_IS_SCROLL_CANVAS (self));

  if (GTK_WIDGET_REALIZED (self))
    {
      GxkScrollCanvasClass *klass = GXK_SCROLL_CANVAS_GET_CLASS (self);
      gint ihv = self->hadjustment->value;
      gint ivv = self->vadjustment->value;
      if ((klass->hscrollable && ihv != scroll_area_x) ||
          (klass->vscrollable && ivv != scroll_area_y))
        {
          GxkScrollCanvasDrag *drag = DRAG (self);
          scroll_canvas_change_adjustments (self, scroll_area_x, scroll_area_y);
          if (drag)
            {
              GdkModifierType modifiers;
              gint x, y;
              gdk_window_get_pointer (drag->drawable, &x, &y, &modifiers);
              drag->type = GXK_DRAG_MOTION;
              drag->mode = gxk_drag_modifier_next (modifiers, drag->mode);
              scroll_canvas_drag (self, x, y, NULL);
            }
        }
    }
}

void
gxk_scroll_canvas_make_visible (GxkScrollCanvas *self,
                                gint             scroll_area_x,
                                gint             scroll_area_y,
                                gint             scroll_area_width,
                                gint             scroll_area_height)
{
  g_return_if_fail (GXK_IS_SCROLL_CANVAS (self));
  if (GTK_WIDGET_REALIZED (self))
    {
      gint ihv = self->hadjustment->value;
      gint ivv = self->vadjustment->value;
      gint p;
      /* scroll to make right boundary visible */
      p = scroll_area_x + scroll_area_width - 1;
      ihv = CLAMP (ihv, p - CANVAS_WIDTH (self) + 1, p);
      /* scroll to make left boundary visible */
      p = scroll_area_x;
      ihv = CLAMP (ihv, p - CANVAS_WIDTH (self) + 1, p);
      /* scroll to make lower boundary visible */
      p = scroll_area_y + scroll_area_height - 1;
      ivv = CLAMP (ivv, p - CANVAS_HEIGHT (self) + 1, p);
      /* scroll to make upper boundary visible */
      p = scroll_area_y;
      ivv = CLAMP (ivv, p - CANVAS_HEIGHT (self) + 1, p);
      gxk_scroll_canvas_scroll_to (self, ihv, ivv);
    }
}

GxkScrollMarker*
gxk_scroll_canvas_lookup_marker (GxkScrollCanvas *self,
                                 guint            index,
                                 guint           *countp)
{
  guint i;
  g_return_val_if_fail (GXK_IS_SCROLL_CANVAS (self), NULL);

  if (countp)
    *countp = 0;
  for (i = 0; i < self->n_markers; i++)
    if (self->markers[i].index == index)
      {
        if (countp)
          {
            GxkScrollMarker *marker = self->markers + i;
            while (marker < self->markers + self->n_markers &&
                   marker->index == index)
              (*countp)++, marker++;
          }
        return self->markers + i;
      }
  return NULL;
}

GxkScrollMarker*
gxk_scroll_canvas_add_marker (GxkScrollCanvas *self,
                              guint            index)
{
  GxkScrollMarker *marker;
  guint i;
  g_return_val_if_fail (GXK_IS_SCROLL_CANVAS (self), NULL);
  g_return_val_if_fail (index > 0, NULL);

  for (i = 0; i < self->n_markers; i++)
    if (index < self->markers[i].index)
      break;
  self->markers = g_renew (GxkScrollMarker, self->markers, self->n_markers + 1);
  g_memmove (self->markers + i + 1, self->markers + i, sizeof (marker[0]) * (self->n_markers - i));
  self->n_markers += 1;
  marker = self->markers + i;
  memset (marker, 0, sizeof (marker[0]));
  marker->index = index;
  return marker;
}

void
gxk_scroll_canvas_remove_marker (GxkScrollCanvas *self,
                                 GxkScrollMarker *marker)
{
  guint i;
  g_return_if_fail (GXK_IS_SCROLL_CANVAS (self));
  g_return_if_fail (marker != NULL);

  i = marker - self->markers;
  gxk_scroll_canvas_setup_marker (self, marker, NULL, 0, 0, 0, 0);
  self->n_markers -= 1;
  g_memmove (self->markers + i, self->markers + i + 1, sizeof (marker[0]) * (self->n_markers - i));
}

void
gxk_scroll_canvas_setup_marker (GxkScrollCanvas *self,
                                GxkScrollMarker *marker,
                                GdkWindow      **windowp,
                                guint            x,
                                guint            y,
                                guint            width,
                                guint            height)
{
  GtkWidget *widget;
  g_return_if_fail (GXK_IS_SCROLL_CANVAS (self));
  g_return_if_fail (marker != NULL);
  widget = GTK_WIDGET (self);
  if (windowp)
    g_return_if_fail (windowp == &widget->window || windowp == &self->canvas ||
                      windowp == &self->top_panel || windowp == &self->left_panel ||
                      windowp == &self->right_panel || windowp == &self->bottom_panel);

  scroll_canvas_unrealize_marker (self, marker, TRUE);
  marker->windowp = windowp;
  marker->extends.x = -width;
  marker->extends.y = -height;
  marker->extends.width = width;
  marker->extends.height = height;
  scroll_canvas_realize_marker (self, marker);
  gxk_scroll_canvas_move_marker (self, marker, x, y);
}

void
gxk_scroll_canvas_move_marker (GxkScrollCanvas        *self,
                               GxkScrollMarker        *marker,
                               guint                   x,
                               guint                   y)
{
  g_return_if_fail (GXK_IS_SCROLL_CANVAS (self));
  g_return_if_fail (marker != NULL);

  uint ox = marker->extends.x;
  uint oy = marker->extends.y;
  if (x == ox && y == oy)
    return;
  if (!GTK_WIDGET_DRAWABLE (self) || !marker->pixmap || !*marker->windowp)
    {
      marker->extends.x = x;
      marker->extends.y = y;
      return;
    }

  /* restore backing under marker */
  GdkGC *draw_gc = STYLE (self)->bg_gc[STATE (self)];
  gdk_window_begin_paint_rect (*marker->windowp, &marker->extends);
  gdk_draw_drawable (*marker->windowp, draw_gc, marker->pixmap,
                     0, 0, marker->extends.x, marker->extends.y,
                     marker->extends.width, marker->extends.height);
  /* restore overlapping markers */
  GxkScrollCanvasClass *klass = GXK_SCROLL_CANVAS_GET_CLASS (self);
  for (uint i = 0; i < self->n_markers; i++)
    {
      GxkScrollMarker *marker2 = self->markers + i;
      if (marker2 != marker && marker2->windowp == marker->windowp && marker2->pixmap)
        {
          GdkRectangle isec, nsec = marker->extends;
          if (gdk_rectangle_intersect (&marker->extends, &marker2->extends, &isec))
            klass->draw_marker (self, *marker2->windowp, &isec, marker2);
          nsec.x = x, nsec.y = y;       /* check against new region */
          if (gdk_rectangle_intersect (&nsec, &marker2->extends, &isec))
            klass->draw_marker (self, *marker2->windowp, &isec, marker2);
        }
    }

  /* move */
  marker->extends.x = x;
  marker->extends.y = y;

  /* save new backing */
  gdk_draw_drawable (marker->pixmap, draw_gc, *marker->windowp,
                     marker->extends.x, marker->extends.y, 0, 0,
                     marker->extends.width, marker->extends.height);
  /* save backing from overlapping markers */
  for (uint i = 0; i < self->n_markers; i++)
    {
      GxkScrollMarker *marker2 = self->markers + i;
      if (marker2 != marker && marker2->windowp == marker->windowp && marker2->pixmap)
        {
          GdkRectangle isec = marker->extends;
          if (gdk_rectangle_intersect (&isec, &marker2->extends, &isec))
            gdk_draw_drawable (marker->pixmap, draw_gc, marker2->pixmap,
                               isec.x - marker2->extends.x, isec.y - marker2->extends.y,
                               isec.x - marker->extends.x, isec.y - marker->extends.y,
                               isec.width, isec.height);
        }
    }

  /* redraw old and new marker area */
  gdk_window_begin_paint_rect (*marker->windowp, &marker->extends);
  gdk_draw_drawable (*marker->windowp, draw_gc, marker->pixmap,
                     0, 0, marker->extends.x, marker->extends.y,
                     marker->extends.width, marker->extends.height);
  for (uint i = 0; i < self->n_markers; i++)
    {
      GxkScrollMarker *marker2 = self->markers + i;
      if (marker2->windowp == marker->windowp && marker2->pixmap)
        {
          GdkRectangle isec, osec = marker->extends;
          if (gdk_rectangle_intersect (&marker->extends, &marker2->extends, &isec))
            klass->draw_marker (self, *marker2->windowp, &isec, marker2);
          osec.x = ox, osec.y = oy;     /* check against old region */
          if (gdk_rectangle_intersect (&osec, &marker2->extends, &isec))
            klass->draw_marker (self, *marker2->windowp, &isec, marker2);
        }
    }

  /* update onscreen */
  gdk_window_end_paint (*marker->windowp);
  gdk_window_end_paint (*marker->windowp);
}

static void
dummy_handler (void)
{
}

static void
gxk_scroll_canvas_class_init (GxkScrollCanvasClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  // GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  gobject_class->finalize = scroll_canvas_finalize;

  object_class->destroy = scroll_canvas_destroy;

  widget_class->size_request = scroll_canvas_size_request;
  widget_class->size_allocate = scroll_canvas_size_allocate;
  widget_class->realize = scroll_canvas_realize;
  widget_class->unrealize = scroll_canvas_unrealize;
  widget_class->style_set = scroll_canvas_style_set;
  widget_class->state_changed = scroll_canvas_state_changed;
  widget_class->direction_changed = scroll_canvas_direction_changed;
  widget_class->expose_event = scroll_canvas_expose;
  widget_class->button_press_event = scroll_canvas_button_press;
  widget_class->motion_notify_event = scroll_canvas_motion;
  widget_class->button_release_event = scroll_canvas_button_release;
  widget_class->key_press_event = scroll_canvas_key_press;
  widget_class->focus_in_event = scroll_canvas_focus_in;
  widget_class->focus_out_event = scroll_canvas_focus_out;

  /* widget config */
  klass->double_buffer_top_panel = TRUE;
  klass->double_buffer_left_panel = TRUE;
  klass->double_buffer_right_panel = TRUE;
  klass->double_buffer_bottom_panel = TRUE;
  klass->double_buffer_canvas = TRUE;
  klass->double_buffer_window = TRUE;
  klass->auto_clear = TRUE;
  klass->grab_focus = TRUE;
  klass->hscrollable = FALSE;
  klass->vscrollable = FALSE;
  /* skin config */
  klass->image_file_name = NULL;
  klass->image_tint.red = 0xff00;
  klass->image_tint.green = 0xff00;
  klass->image_tint.blue = 0xff00;
  klass->image_saturation = 0.9;
  /* virtual methods */
  klass->get_layout = (void (*) (GxkScrollCanvas*, GxkScrollCanvasLayout*)) dummy_handler;
  klass->set_scroll_adjustments = scroll_canvas_set_scroll_adjustments;
  klass->update_adjustments = scroll_canvas_update_adjustments;
  klass->adjustment_changed = scroll_canvas_adjustment_changed;
  klass->draw_window = scroll_canvas_draw_window;
  klass->draw_canvas = scroll_canvas_draw_canvas;
  klass->draw_top_panel = scroll_canvas_draw_panel;
  klass->draw_left_panel = scroll_canvas_draw_panel;
  klass->draw_right_panel = scroll_canvas_draw_panel;
  klass->draw_bottom_panel = scroll_canvas_draw_panel;
  klass->handle_drag = (void (*) (GxkScrollCanvas*, GxkScrollCanvasDrag*, GdkEvent*)) dummy_handler;

  widget_class->set_scroll_adjustments_signal =
    gtk_signal_new ("set_scroll_adjustments",
                    GTK_RUN_LAST,
                    GTK_CLASS_TYPE (object_class),
                    GTK_SIGNAL_OFFSET (GxkScrollCanvasClass, set_scroll_adjustments),
                    gxk_marshal_NONE__OBJECT_OBJECT,
                    GTK_TYPE_NONE, 2, GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);
}

void
gxk_scroll_canvas_class_skin_changed (GxkScrollCanvasClass *klass)
{
  GSList *slist;
  g_return_if_fail (GXK_IS_SCROLL_CANVAS_CLASS (klass));
  for (slist = klass->realized_widgets; slist; slist = slist->next)
    scroll_canvas_reset_skin_and_style ((GxkScrollCanvas*) slist->data, TRUE);
}
