// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GXK_SCROLL_CANVAS_H__
#define __GXK_SCROLL_CANVAS_H__

#include <gxk/gxkutils.hh>

/* --- type macros --- */
#define GXK_TYPE_SCROLL_CANVAS              (gxk_scroll_canvas_get_type ())
#define GXK_SCROLL_CANVAS(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_SCROLL_CANVAS, GxkScrollCanvas))
#define GXK_SCROLL_CANVAS_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_SCROLL_CANVAS, GxkScrollCanvasClass))
#define GXK_IS_SCROLL_CANVAS(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_SCROLL_CANVAS))
#define GXK_IS_SCROLL_CANVAS_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_SCROLL_CANVAS))
#define GXK_SCROLL_CANVAS_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GXK_TYPE_SCROLL_CANVAS, GxkScrollCanvasClass))


/* --- typedefs, enums & structures --- */
typedef enum    /*< skip >*/
{
  GXK_DRAG_AREA_RESIZE          = 1 << 0,
  GXK_DRAG_AREA_ENRICH          = 1 << 1,
  GXK_DRAG_AREA_REDUCE          = 1 << 2,
  GXK_DRAG_AREA_XOR             = 1 << 3,
  GXK_DRAG_RATIO_FIXED          = 1 << 8,
  GXK_DRAG_OFFSET_CENTERED      = 1 << 9,
} GxkDragMode;
GxkDragMode     gxk_drag_modifier_start (GdkModifierType        key_mods);
GxkDragMode     gxk_drag_modifier_next  (GdkModifierType        key_mods,
                                         GxkDragMode            last_drag_mods);
typedef enum    /*< skip >*/
{
  /* drag emission state */
  GXK_DRAG_START,         /* initial drag event */
  GXK_DRAG_MOTION,        /* drag motion, pointer moved */
  GXK_DRAG_DONE,          /* final drag motion */
  GXK_DRAG_ABORT,         /* drag abortion requested */
  /* drag-action requests */
  GXK_DRAG_UNHANDLED    = GXK_DRAG_START,       /* continue with button-press or similar */
  GXK_DRAG_CONTINUE     = GXK_DRAG_MOTION,      /* request drag-motion emissions */
  GXK_DRAG_HANDLED      = GXK_DRAG_DONE,        /* no further emissions */
  GXK_DRAG_ERROR        = GXK_DRAG_ABORT        /* request abortion */
} GxkDragStatus;
typedef struct _GxkScrollCanvas GxkScrollCanvas;
struct GxkScrollCanvasDrag {
  GtkWidget       *widget = NULL;
  GdkWindow       *drawable = NULL;
  /* modifier & config determined mode */
  union {
    uint64_t bit_field = 0;
    struct {
      GxkDragMode      mode : 16;
      /* mouse button */
      uint            button : 16;
      /* emission type: start/motion/done/abort */
      GxkDragStatus    type : 16;
      uint            window_drag : 1;
      uint            canvas_drag : 1;
      uint            top_panel_drag : 1;
      uint            left_panel_drag : 1;
      uint            right_panel_drag : 1;
      uint            bottom_panel_drag : 1;
      uint            ___dummy1 : 2;
      /* whether start_x/start_y are in window */
      uint            start_confined : 1;
      /* whether current_x/current_y are in window */
      uint             current_confined : 1;
    };
  };
  int              start_x = -1, start_y = -1;
  int              current_x = -1, current_y = -1;
  /* user data: unhandled/continue/handled/error */
  GxkDragStatus        state = GXK_DRAG_ABORT;
};
typedef struct {
  guint        index;
  guint        mtype;
  GdkRectangle extends;
  GdkWindow  **windowp;
  GdkDrawable *pixmap;  /* backing store */
  GdkRectangle coords;  /* user coordinates */
  gpointer     user_data;
} GxkScrollMarker;
typedef struct {
  gint top_panel_height;
  gint left_panel_width;
  gint right_panel_width;
  gint bottom_panel_height;
  gint canvas_width, max_canvas_width;
  gint canvas_height, max_canvas_height;
} GxkScrollCanvasLayout;
struct _GxkScrollCanvas
{
  GtkContainer          parent_instance;
  GxkScrollCanvasLayout layout;
  GdkWindow            *canvas, *top_panel, *left_panel, *right_panel, *bottom_panel;
  GdkPixmap            *canvas_pixmap;
  GdkGC               **color_gc;       /* array of size class->n_colors */
  guint                 n_pango_layouts;
  PangoLayout         **pango_layouts;
  guint                 n_markers;
  GxkScrollMarker      *markers;
  /* scroll offset */
  gint                  x_offset, y_offset;
  GtkAdjustment        *hadjustment, *vadjustment;
  guint                 scroll_timer;
};
typedef struct
{
  GtkContainerClass     parent_class;
  /* widget config */
  GdkEventMask          canvas_events, top_panel_events, left_panel_events, right_panel_events, bottom_panel_events;
  guint                 double_buffer_window : 1, double_buffer_canvas : 1;
  guint                 double_buffer_top_panel : 1, double_buffer_left_panel : 1;
  guint                 double_buffer_right_panel : 1, double_buffer_bottom_panel : 1;
  guint                 auto_clear : 1; /* automatically clear non-double-buffered areas */
  guint                 grab_focus : 1; /* automatically grab focus on button-press */
  guint                 hscrollable : 1;
  guint                 vscrollable : 1;
  /* skin config */
  guint                 n_colors;       /* must be const across skin changes */
  const GdkColor       *colors;
  gchar                *image_file_name;
  GdkColor              image_tint;
  gdouble               image_saturation;
  GSList               *realized_widgets;
  /* virtual methods */
  void          (*get_layout)                   (GxkScrollCanvas        *self,
                                                 GxkScrollCanvasLayout  *layout);
  void          (*set_scroll_adjustments)       (GxkScrollCanvas        *self,
                                                 GtkAdjustment          *hadjustment,
                                                 GtkAdjustment          *vadjustment);
  void          (*update_adjustments)           (GxkScrollCanvas        *self,
                                                 gboolean                hadj,
                                                 gboolean                vadj);
  void          (*adjustment_changed)           (GxkScrollCanvas        *self,
                                                 GtkAdjustment          *adj);
  void          (*reallocate_contents)          (GxkScrollCanvas        *self,
                                                 gint                    xdiff,
                                                 gint                    ydiff);
  void          (*draw_window)                  (GxkScrollCanvas        *self,
                                                 GdkWindow              *drawable,
                                                 GdkRectangle           *area);
  void          (*draw_canvas)                  (GxkScrollCanvas        *self,
                                                 GdkWindow              *drawable,
                                                 GdkRectangle           *area);
  void          (*draw_top_panel)               (GxkScrollCanvas        *self,
                                                 GdkWindow              *drawable,
                                                 GdkRectangle           *area);
  void          (*draw_left_panel)              (GxkScrollCanvas        *self,
                                                 GdkWindow              *drawable,
                                                 GdkRectangle           *area);
  void          (*draw_right_panel)             (GxkScrollCanvas        *self,
                                                 GdkWindow              *drawable,
                                                 GdkRectangle           *area);
  void          (*draw_bottom_panel)            (GxkScrollCanvas        *self,
                                                 GdkWindow              *drawable,
                                                 GdkRectangle           *area);
  void          (*draw_marker)                  (GxkScrollCanvas        *self,
                                                 GdkWindow              *drawable,
                                                 GdkRectangle           *area,
                                                 GxkScrollMarker        *marker);
  void          (*handle_drag)                  (GxkScrollCanvas        *self,
                                                 GxkScrollCanvasDrag    *drag,
                                                 GdkEvent               *event);
} GxkScrollCanvasClass;


/* --- prototypes --- */
GType            gxk_scroll_canvas_get_type                (void);
void             gxk_scroll_canvas_get_layout              (GxkScrollCanvas        *self,
                                                            GxkScrollCanvasLayout  *layout);
void             gxk_scroll_canvas_get_canvas_size         (GxkScrollCanvas        *self,
                                                            gint                   *width,
                                                            gint                   *height);
void             gxk_scroll_canvas_set_hadjustment         (GxkScrollCanvas        *self,
                                                            GtkAdjustment          *adjustment);
void             gxk_scroll_canvas_set_vadjustment         (GxkScrollCanvas        *self,
                                                            GtkAdjustment          *adjustment);
void             gxk_scroll_canvas_update_adjustments      (GxkScrollCanvas        *self,
                                                            gboolean                hadj,
                                                            gboolean                vadj);
void             gxk_scroll_canvas_scroll_to               (GxkScrollCanvas        *self,
                                                            gint                    scroll_area_x,
                                                            gint                    scroll_area_y);
void             gxk_scroll_canvas_make_visible            (GxkScrollCanvas        *self,
                                                            gint                    scroll_area_x,
                                                            gint                    scroll_area_y,
                                                            gint                    scroll_area_width,
                                                            gint                    scroll_area_height);
void             gxk_scroll_canvas_reset_pango_layouts     (GxkScrollCanvas        *self);
void             gxk_scroll_canvas_reallocate              (GxkScrollCanvas        *self);
gboolean         gxk_scroll_canvas_dragging                (GxkScrollCanvas        *self);
void             gxk_scroll_canvas_drag_abort              (GxkScrollCanvas        *self);
void             gxk_scroll_canvas_set_window_cursor       (GxkScrollCanvas        *self,
                                                            GdkCursorType           cursor);
void             gxk_scroll_canvas_set_canvas_cursor       (GxkScrollCanvas        *self,
                                                            GdkCursorType           cursor);
void             gxk_scroll_canvas_set_top_panel_cursor    (GxkScrollCanvas        *self,
                                                            GdkCursorType           cursor);
void             gxk_scroll_canvas_set_left_panel_cursor   (GxkScrollCanvas        *self,
                                                            GdkCursorType           cursor);
void             gxk_scroll_canvas_set_right_panel_cursor  (GxkScrollCanvas        *self,
                                                            GdkCursorType           cursor);
void             gxk_scroll_canvas_set_bottom_panel_cursor (GxkScrollCanvas        *self,
                                                            GdkCursorType           cursor);
PangoLayout*     gxk_scroll_canvas_get_pango_layout        (GxkScrollCanvas        *self,
                                                            guint                   nth);
PangoLayout*     gxk_scroll_canvas_peek_pango_layout       (GxkScrollCanvas        *self,
                                                            guint                   nth);
GxkScrollMarker* gxk_scroll_canvas_lookup_marker           (GxkScrollCanvas        *self,
                                                            guint                   index,
                                                            guint                  *count);
GxkScrollMarker* gxk_scroll_canvas_add_marker              (GxkScrollCanvas        *self,
                                                            guint                   index);
void             gxk_scroll_canvas_remove_marker           (GxkScrollCanvas        *self,
                                                            GxkScrollMarker        *marker);
void             gxk_scroll_canvas_setup_marker            (GxkScrollCanvas        *self,
                                                            GxkScrollMarker        *marker,
                                                            GdkWindow             **windowp,
                                                            guint                   x,
                                                            guint                   y,
                                                            guint                   width,
                                                            guint                   height);
void             gxk_scroll_canvas_move_marker             (GxkScrollCanvas        *self,
                                                            GxkScrollMarker        *marker,
                                                            guint                   x,
                                                            guint                   y);
void             gxk_scroll_canvas_class_skin_changed      (GxkScrollCanvasClass*);

#endif /* __GXK_SCROLL_CANVAS_H__ */
