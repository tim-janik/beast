// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsteventroll.hh"
#include "bstasciipixbuf.hh"
#include "bstskinconfig.hh"
#include <string.h>
/* --- defines --- */
/* accessors */
#define	STYLE(self)		(GTK_WIDGET (self)->style)
#define STATE(self)             (GtkStateType (GTK_WIDGET (self)->state))
#define XTHICKNESS(self)        (STYLE (self)->xthickness)
#define YTHICKNESS(self)        (STYLE (self)->ythickness)
#define	ALLOCATION(self)	(&GTK_WIDGET (self)->allocation)
#define X_OFFSET(self)          (GXK_SCROLL_CANVAS (self)->x_offset)
#define Y_OFFSET(self)          (GXK_SCROLL_CANVAS (self)->y_offset)
#define COLOR_GC(self, i)       (GXK_SCROLL_CANVAS (self)->color_gc[i])
#define COLOR_GC_HBAR(self)     (COLOR_GC (self, INDEX_HBAR))
#define COLOR_GC_MBAR(self)     (COLOR_GC (self, INDEX_MBAR))
#define COLOR_GC_POSITIVE(self) (COLOR_GC (self, INDEX_POSITIVE))
#define COLOR_GC_ZERO(self)     (COLOR_GC (self, INDEX_ZERO))
#define COLOR_GC_NEGATIVE(self) (COLOR_GC (self, INDEX_NEGATIVE))
#define CANVAS(self)            (GXK_SCROLL_CANVAS (self)->canvas)
#define VPANEL(self)            (GXK_SCROLL_CANVAS (self)->left_panel)
/* appearance */
#define VPANEL_BG_COLOR(self)   (&STYLE (self)->bg[GTK_WIDGET_IS_SENSITIVE (self) ? GTK_STATE_NORMAL : GTK_STATE_INSENSITIVE])
#define CANVAS_BG_COLOR(self)   (&STYLE (self)->base[GTK_WIDGET_STATE (self)])
#define	QNOTE_HPIXELS		(30)	/* guideline */
/* --- prototypes --- */
static void     bst_event_roll_hsetup                 (BstEventRoll      *self,
                                                       guint              ppqn,
                                                       guint              qnpt,
                                                       guint              max_ticks,
                                                       gfloat             hzoom);
/* --- static variables --- */
static guint	signal_canvas_drag = 0;
static guint	signal_canvas_clicked = 0;
static guint	signal_vpanel_drag = 0;
static guint	signal_vpanel_clicked = 0;
/* --- functions --- */
G_DEFINE_TYPE (BstEventRoll, bst_event_roll, GXK_TYPE_SCROLL_CANVAS);
enum {
  INDEX_HBAR,
  INDEX_MBAR,
  INDEX_POSITIVE,
  INDEX_ZERO,
  INDEX_NEGATIVE,
  INDEX_LAST
};
static void
event_roll_class_setup_skin (BstEventRollClass *klass)
{
  static GdkColor colors[INDEX_LAST];
  GxkScrollCanvasClass *scroll_canvas_class = GXK_SCROLL_CANVAS_CLASS (klass);
  scroll_canvas_class->n_colors = G_N_ELEMENTS (colors);
  scroll_canvas_class->colors = colors;
  colors[INDEX_HBAR] = gdk_color_from_rgb (BST_SKIN_CONFIG (controls_hbar));
  colors[INDEX_MBAR] = gdk_color_from_rgb (BST_SKIN_CONFIG (controls_mbar));
  colors[INDEX_POSITIVE] = gdk_color_from_rgb (BST_SKIN_CONFIG (controls_positive));
  colors[INDEX_ZERO] = gdk_color_from_rgb (BST_SKIN_CONFIG (controls_zero));
  colors[INDEX_NEGATIVE] = gdk_color_from_rgb (BST_SKIN_CONFIG (controls_negative));
  g_free (scroll_canvas_class->image_file_name);
  scroll_canvas_class->image_file_name = BST_SKIN_CONFIG_STRDUP_PATH (controls_image);
  scroll_canvas_class->image_tint = gdk_color_from_rgb (BST_SKIN_CONFIG (controls_color));
  scroll_canvas_class->image_saturation = BST_SKIN_CONFIG (controls_shade) * 0.01;
  gxk_scroll_canvas_class_skin_changed (scroll_canvas_class);
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
  self->selection_tick = 0;
  self->selection_duration = 0;
  bst_event_roll_hsetup (self, 384, 4, 800 * 384, 1);
  bst_ascii_pixbuf_ref ();
  gxk_scroll_canvas_set_canvas_cursor (GXK_SCROLL_CANVAS (self), GDK_LEFT_PTR);
  gxk_scroll_canvas_set_left_panel_cursor (GXK_SCROLL_CANVAS (self), GDK_HAND2);
}
static void
bst_event_roll_destroy (GtkObject *object)
{
  BstEventRoll *self = BST_EVENT_ROLL (object);
  bst_event_roll_set_proxy (self, 0);
  GTK_OBJECT_CLASS (bst_event_roll_parent_class)->destroy (object);
}
static void
bst_event_roll_dispose (GObject *object)
{
  BstEventRoll *self = BST_EVENT_ROLL (object);
  bst_event_roll_set_proxy (self, 0);
  G_OBJECT_CLASS (bst_event_roll_parent_class)->dispose (object);
}
static void
bst_event_roll_finalize (GObject *object)
{
  BstEventRoll *self = BST_EVENT_ROLL (object);
  bst_event_roll_set_proxy (self, 0);
  bst_ascii_pixbuf_unref ();
  G_OBJECT_CLASS (bst_event_roll_parent_class)->finalize (object);
}
static void
event_roll_get_layout (GxkScrollCanvas       *scc,
                       GxkScrollCanvasLayout *layout)
{
  BstEventRoll *self = BST_EVENT_ROLL (scc);
  layout->top_panel_height = 0;
  if (self->fetch_vpanel_width)
    layout->left_panel_width = self->fetch_vpanel_width (self->fetch_vpanel_width_data);
  layout->right_panel_width = 0;
  layout->bottom_panel_height = 0;
  layout->canvas_width = 500 + XTHICKNESS (self);
  layout->canvas_height = 64 + YTHICKNESS (self);
}
static void
event_roll_reallocate_contents (GxkScrollCanvas *scc,
                                gint             xdiff,
                                gint             ydiff)
{
  BstEventRoll *self = BST_EVENT_ROLL (scc);
  if (!xdiff && !ydiff && self->child)  /* real size-allocate */
    {
      GtkAllocation child_allocation;
      gint w, h;
      gxk_scroll_canvas_get_canvas_size (scc, &w, &h);
      child_allocation.x = 0;
      child_allocation.y = 0;
      child_allocation.width = scc->layout.left_panel_width;
      child_allocation.height = h;
      gtk_widget_size_allocate (self->child, &child_allocation);
    }
  if (bst_segment_initialized (&self->segment))
    bst_segment_translate (&self->segment, xdiff, 0);
}
static void
bst_event_roll_add (GtkContainer *container,
                    GtkWidget    *child)
{
  BstEventRoll *self = BST_EVENT_ROLL (container);
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (self->child == NULL);
  gtk_widget_set_parent_window (child, VPANEL (self));
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
  GTK_WIDGET_CLASS (bst_event_roll_parent_class)->realize (widget);
  if (self->child)
    gtk_widget_set_parent_window (self->child, VPANEL (self));
}
static gint
ticks_to_pixels (BstEventRoll *self,
		 gint	       ticks)
{
  gdouble ppqn = self->ppqn;
  gdouble tpixels = QNOTE_HPIXELS;
  /* compute pixel span of a tick range */
  tpixels *= self->hzoom / ppqn * (gdouble) ticks;
  if (ticks)
    tpixels = MAX (tpixels, 1);
  return MIN (G_MAXINT, tpixels);
}
static gint
pixels_to_ticks (BstEventRoll *self,
		 gint	       pixels)
{
  gdouble ppqn = self->ppqn;
  gdouble ticks = 1.0 / (gdouble) QNOTE_HPIXELS;
  /* compute tick span of a pixel range */
  ticks = ticks * ppqn / self->hzoom * (gdouble) pixels;
  if (pixels > 0)
    ticks = MAX (ticks, 1);
  else
    ticks = 0;
  return MIN (G_MAXINT, ticks);
}
static gint
tick_to_coord (BstEventRoll *self,
	       gint	     tick)
{
  return ticks_to_pixels (self, tick) - X_OFFSET (self);
}
static gint
coord_to_tick (BstEventRoll *self,
	       gint	     x,
	       gboolean	     right_bound)
{
  guint tick;
  x += X_OFFSET (self);
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
event_roll_draw_vpanel (GxkScrollCanvas *scc,
                        GdkWindow       *drawable,
                        GdkRectangle    *area)
{
  BstEventRoll *self = BST_EVENT_ROLL (scc);
  gint width, height;
  gdk_window_get_size (drawable, &width, &height);
  if (0)
    {
      /* outer vpanel shadow */
      gtk_paint_shadow (STYLE (self), drawable,
                        STATE (self), GTK_SHADOW_OUT,
                        NULL, NULL, NULL,
                        0, -YTHICKNESS (self),
                        width, height + 2 * YTHICKNESS (self));
    }
}
static gint
event_roll_scale_range (BstEventRoll *self,
                        gint         *rangep)
{
  GxkScrollCanvas *scc = GXK_SCROLL_CANVAS (self);
  gint width, height, mid, range;
  gxk_scroll_canvas_get_canvas_size (scc, &width, &height);
  mid = (height - 1) / 2;
  range = mid;
  if (rangep)
    *rangep = range;
  return mid;
}
#define TICK_WIDTH(self)        (pixels_to_ticks (self, XTHICKNESS (self) * 2 + 1))
static void
bst_event_roll_overlap_grow_canvas_area (BstEventRoll *self,
					 GdkRectangle *area)
{
  gint x = area->x, xbound = x + area->width;
  /* grow canvas paint area by value bar width, so we don't clear out parts of neighbouring bars */
  x -= TICK_WIDTH (self);
  xbound += TICK_WIDTH (self);
  area->x = MAX (x, 0);
  area->width = xbound - area->x;
}
static void
event_roll_draw_canvas (GxkScrollCanvas *scc,
                        GdkWindow       *drawable,
                        GdkRectangle    *area)
{
  BstEventRoll *self = BST_EVENT_ROLL (scc);
  guint8 *dash, dashes[3] = { 1, 1, 0 }; // long: { 2, 2, 0 };
  GdkGC *draw_gc, *dark_gc;
  int dlen, line_width = 0; /* line widths != 0 interfere with dash-settings on some X servers */
  BsePartControlSeq *cseq;
  gint range, mid = event_roll_scale_range (self, &range);
  gint x, xbound, width, height;
  GXK_SCROLL_CANVAS_CLASS (bst_event_roll_parent_class)->draw_canvas (scc, drawable, area);
  gdk_window_get_size (drawable, &width, &height);
  bst_event_roll_overlap_grow_canvas_area (self, area);
  x = area->x;
  xbound = x + area->width;
  /* draw selection */
  if (self->selection_duration)
    {
      gint x1, x2;
      x1 = tick_to_coord (self, self->selection_tick);
      x2 = tick_to_coord (self, self->selection_tick + self->selection_duration);
      /* confine to 16bit coordinates for gdk to handle correctly */
      x1 = MAX (x1, 0);
      x2 = MIN (x2, width);
      gdk_draw_rectangle (drawable, GTK_WIDGET (self)->style->bg_gc[GTK_STATE_SELECTED], TRUE,
			  x1, 0, MAX (x2 - x1, 0), height);
    }
  /* draw horizontal grid lines */
  draw_gc = COLOR_GC_MBAR (self);
  gdk_gc_set_line_attributes (draw_gc, line_width, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER);
  dash = dashes;
  dlen = dash[0] + dash[1];
  gdk_gc_set_dashes (draw_gc, (X_OFFSET (self) + x + 1) % dlen, (gint8*) dash, 2);
  gdk_draw_line (drawable, draw_gc, x, mid + range / 2, xbound - 1, mid + range / 2);
  gdk_draw_line (drawable, draw_gc, x, mid - range / 2, xbound - 1, mid - range / 2);
  gdk_gc_set_line_attributes (draw_gc, line_width, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
  draw_gc = COLOR_GC_HBAR (self);
  gdk_draw_line (drawable, draw_gc, x, mid, xbound - 1, mid);
  /* draw controls */
  dark_gc = STYLE (self)->dark_gc[GTK_STATE_NORMAL];
  cseq = self->proxy ? bse_part_list_controls (self->proxy,
                                               coord_to_tick (self, x, FALSE),
                                               coord_to_tick (self, xbound, FALSE),
                                               self->control_type) : NULL;
  for (uint i = 0; cseq && i < cseq->n_pcontrols; i++)
    {
      BsePartControl *pctrl = cseq->pcontrols[i];
      guint tick = pctrl->tick;
      GdkGC *xdark_gc, *xlight_gc, *xval_gc;
      gint x1, x2, y1, y2;
      gboolean selected = pctrl->selected;
      selected |= (uint (pctrl->tick) >= self->selection_tick &&
		   uint (pctrl->tick) < self->selection_tick + self->selection_duration);
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
            xval_gc = COLOR_GC_ZERO (self);
          else if (pctrl->value < 0)
            xval_gc = COLOR_GC_NEGATIVE (self);
          else
            xval_gc = COLOR_GC_POSITIVE (self);
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
    }
  if (bst_segment_initialized (&self->segment))
    bst_segment_draw (&self->segment, STYLE (self));
}
static void
event_roll_update_adjustments (GxkScrollCanvas *scc,
			       gboolean         hadj,
			       gboolean         vadj)
{
  // BstEventRoll *self = BST_EVENT_ROLL (scc);
  if (hadj)
    {
      scc->hadjustment->lower = 0;
      /* we're not adjusting adj->upper, adj->step_increment and adj->page_increment here,
       * because doing so requires notification based max_ticks updating. the piano roll
       * gets this right, and as long as this widget is scroll-aligned with a piano roll
       * we'd create races if we tried updating anyway. see piano_roll_update_adjustments()
       * and piano_roll_adjustment_changed().
       */
    }
  GXK_SCROLL_CANVAS_CLASS (bst_event_roll_parent_class)->update_adjustments (scc, hadj, vadj);
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
      old_hzoom != self->hzoom)
    {
      self->draw_qn_grid = ticks_to_pixels (self, self->ppqn) >= 3;
      self->draw_qqn_grid = ticks_to_pixels (self, self->ppqn / 4) >= 5;
      gtk_widget_queue_draw (GTK_WIDGET (self));
      X_OFFSET (self) = GXK_SCROLL_CANVAS (self)->hadjustment->value;
      gxk_scroll_canvas_update_adjustments (GXK_SCROLL_CANVAS (self), TRUE, FALSE);
    }
  else if (old_max_ticks != self->max_ticks)
    {
      X_OFFSET (self) = GXK_SCROLL_CANVAS (self)->hadjustment->value;
      gxk_scroll_canvas_update_adjustments (GXK_SCROLL_CANVAS (self), TRUE, FALSE);
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
static void
event_roll_handle_drag (GxkScrollCanvas     *scc,
                        GxkScrollCanvasDrag *scc_drag,
                        GdkEvent            *event)
{
  BstEventRoll *self = BST_EVENT_ROLL (scc);
  BstEventRollDrag drag_mem = { 0 }, *drag = &drag_mem;
  /* copy over drag setup */
  memcpy (drag, scc_drag, sizeof (*scc_drag));  /* sizeof (*scc_drag) < sizeof (*drag) */
  drag->eroll = self;
  drag->tick_width = TICK_WIDTH (self);
  if (drag->canvas_drag)
    {
      gint range, mid = event_roll_scale_range (self, &range);
      drag->current_tick = coord_to_tick (self, MAX (drag->current_x, 0), FALSE);
      drag->current_value_raw = mid - drag->current_y;
      drag->current_value_raw /= range;
      drag->current_value = CLAMP (drag->current_value_raw, -1, +1);
      drag->current_valid = ABS (drag->current_value) <= 1;
    }
  else if (drag->left_panel_drag)
    {
      drag->current_tick = 0;
      drag->current_value_raw = drag->current_y;
      drag->current_value = drag->current_value_raw;
      drag->current_valid = ABS (drag->current_value) <= 1;
    }
  /* sync start-position fields */
  if (drag->type == GXK_DRAG_START)
    {
      drag->start_tick = self->start_tick = drag->current_tick;
      drag->start_value = self->start_value = drag->current_value;
      drag->start_valid = self->start_valid = drag->current_valid;
    }
  else
    {
      drag->start_tick = self->start_tick;
      drag->start_value = self->start_value;
      drag->start_valid = self->start_valid;
    }
  /* handle drag */
  if (drag->canvas_drag)
    g_signal_emit (self, signal_canvas_drag, 0, drag);
  else if (drag->left_panel_drag)
    g_signal_emit (self, signal_vpanel_drag, 0, drag);
  /* copy over drag reply */
  scc_drag->state = drag->state;
  /* resort to clicks for unhandled button presses */
  if (drag->type == GXK_DRAG_START && drag->state == GXK_DRAG_UNHANDLED &&
      event && event->type == GDK_BUTTON_PRESS)
    {
      drag->state = GXK_DRAG_HANDLED;
      if (drag->canvas_drag)
        g_signal_emit (self, signal_canvas_clicked, 0, drag->button, drag->start_tick, drag->start_value, event);
      else if (drag->left_panel_drag)
        g_signal_emit (self, signal_vpanel_clicked, 0, drag->button, drag->start_tick, drag->start_value, event);
    }
}
static void
event_roll_range_changed (BstEventRoll *self)
{
  guint max_ticks;
  bse_proxy_get (self->proxy, "last-tick", &max_ticks, NULL);
  bst_event_roll_hsetup (self, self->ppqn, self->qnpt, self->max_ticks, self->hzoom);
}
static void
event_roll_update (BstEventRoll *self,
		   guint         tick_start,
		   guint         duration)
{
  guint tick_end = tick_start + MAX (duration, 1) - 1;
  gint x1 = tick_to_coord (self, tick_start);
  gint x2 = tick_to_coord (self, tick_end);
  if (GTK_WIDGET_DRAWABLE (self))
    {
      gint width, height;
      GdkRectangle area;
      gdk_window_get_size (CANVAS (self), &width, &height);
      area.x = x1 - 3;		/* add fudge */
      area.y = 0;
      area.width = x2 - x1 + 3 + 3;	/* add fudge */
      area.height = height;
      gdk_window_invalidate_rect (CANVAS (self), &area, TRUE);
    }
  gxk_widget_update_actions (self); /* update controllers */
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
			    "any-signal", event_roll_unset_proxy, self,
                            "any-signal", event_roll_range_changed, self,
			    "any-signal", event_roll_update, self,
			    NULL);
      bse_item_unuse (self->proxy);
    }
  self->proxy = proxy;
  if (self->proxy)
    {
      bse_item_use (self->proxy);
      bse_proxy_connect (self->proxy,
			 "swapped-signal::release", event_roll_unset_proxy, self,
                         "swapped-signal::property-notify::last-tick", event_roll_range_changed, self,
			 "swapped-signal::range-changed", event_roll_update, self,
			 NULL);
      event_roll_range_changed (self);
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
bst_event_roll_set_control_type (BstEventRoll *self, BseMidiSignalType control_type)
{
  g_return_if_fail (BST_IS_EVENT_ROLL (self));
  self->control_type = control_type;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}
void
bst_event_roll_init_segment (BstEventRoll   *self,
                             BstSegmentType  type)
{
  bst_event_roll_clear_segment (self);
  bst_segment_init (&self->segment, type, CANVAS (self));
}
void
bst_event_roll_segment_start (BstEventRoll   *self,
                              guint           tick,
                              gfloat          value)
{
  gint range, mid = event_roll_scale_range (self, &range);
  if (bst_segment_initialized (&self->segment))
    bst_segment_start (&self->segment,
                       tick_to_coord (self, tick),
                       mid - value * range);
}
void
bst_event_roll_segment_move_to (BstEventRoll   *self,
                                guint           tick,
                                gfloat          value)
{
  gint range, mid = event_roll_scale_range (self, &range);
  if (bst_segment_initialized (&self->segment))
    bst_segment_move_to (&self->segment,
                         tick_to_coord (self, tick),
                         mid - value * range);
}
void
bst_event_roll_segment_tick_range (BstEventRoll   *self,
                                   guint          *tick_p,
                                   guint          *duration_p)
{
  gdouble x = 0, w = 0;
  guint t;
  if (bst_segment_initialized (&self->segment))
    bst_segment_xrange (&self->segment, &x, &w);
  t = coord_to_tick (self, x, FALSE);
  if (tick_p)
    *tick_p = t;
  if (duration_p)
    *duration_p = coord_to_tick (self, x + w, TRUE) - t;
}
gdouble
bst_event_roll_segment_value (BstEventRoll   *self,
                              guint           tick)
{
  gint range, mid = event_roll_scale_range (self, &range);
  gdouble y = 0, v;
  if (bst_segment_initialized (&self->segment))
    y = bst_segment_calcy (&self->segment, tick_to_coord (self, tick));
  v = mid - y;
  v /= range;
  v = CLAMP (v, -1, +1);
  return v;
}
void
bst_event_roll_clear_segment (BstEventRoll *self)
{
  if (bst_segment_initialized (&self->segment))
    {
      bst_segment_expose (&self->segment);
      bst_segment_clear (&self->segment);
    }
}
static void
bst_event_roll_class_init (BstEventRollClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);
  GxkScrollCanvasClass *scroll_canvas_class = GXK_SCROLL_CANVAS_CLASS (klass);
  gobject_class->dispose = bst_event_roll_dispose;
  gobject_class->finalize = bst_event_roll_finalize;
  object_class->destroy = bst_event_roll_destroy;
  widget_class->realize = bst_event_roll_realize;
  container_class->add = bst_event_roll_add;
  container_class->remove = bst_event_roll_remove;
  container_class->forall = bst_event_roll_forall;
  scroll_canvas_class->hscrollable = TRUE;
  scroll_canvas_class->get_layout = event_roll_get_layout;
  scroll_canvas_class->update_adjustments = event_roll_update_adjustments;
  scroll_canvas_class->reallocate_contents = event_roll_reallocate_contents;
  scroll_canvas_class->draw_left_panel = event_roll_draw_vpanel;
  scroll_canvas_class->draw_canvas = event_roll_draw_canvas;
  scroll_canvas_class->handle_drag = event_roll_handle_drag;
  bst_skin_config_add_notify ((BstSkinConfigNotify) event_roll_class_setup_skin, klass);
  event_roll_class_setup_skin (klass);
  klass->canvas_clicked = NULL;
  signal_canvas_drag = g_signal_new ("canvas-drag", G_OBJECT_CLASS_TYPE (klass),
				     G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstEventRollClass, canvas_drag),
				     NULL, NULL,
				     bst_marshal_NONE__POINTER,
				     G_TYPE_NONE, 1, G_TYPE_POINTER);
  signal_canvas_clicked = g_signal_new ("canvas-clicked", G_OBJECT_CLASS_TYPE (klass),
					G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstEventRollClass, canvas_clicked),
					NULL, NULL,
					bst_marshal_NONE__UINT_UINT_FLOAT_BOXED,
					G_TYPE_NONE, 4, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_FLOAT,
					GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  signal_vpanel_drag = g_signal_new ("vpanel-drag", G_OBJECT_CLASS_TYPE (klass),
                                     G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstEventRollClass, vpanel_drag),
                                     NULL, NULL,
                                     bst_marshal_NONE__POINTER,
                                     G_TYPE_NONE, 1, G_TYPE_POINTER);
  signal_vpanel_clicked = g_signal_new ("vpanel-clicked", G_OBJECT_CLASS_TYPE (klass),
                                        G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstEventRollClass, vpanel_clicked),
                                        NULL, NULL,
                                        bst_marshal_NONE__UINT_INT_BOXED,
                                        G_TYPE_NONE, 3, G_TYPE_UINT, G_TYPE_FLOAT,
                                        GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
}
