// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstsegment.hh"
#include <string.h>
/* --- functions --- */
void
bst_segment_expose (BstSegment *self)
{
  GdkRectangle area;
  bst_segment_bbox (self, &area);
  gdk_window_invalidate_rect (self->any.drawable, &area, TRUE);
}
void
bst_segment_bbox (BstSegment   *self,
                  GdkRectangle *area)
{
  gdouble x, w, y, h;
  bst_segment_xrange (self, &x, &w);
  bst_segment_yrange (self, &y, &h);
  area->x = x - 1;
  area->y = y - 1;
  area->width = w + 2.5;
  area->height = h + 2.5;
}
gint
bst_segment_initialized (BstSegment     *self)
{
  return self->type && self->any.drawable;
}
void
bst_segment_init (BstSegment     *self,
                  BstSegmentType  type,
                  GdkDrawable    *drawable)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->type == 0);
  g_return_if_fail (GDK_IS_DRAWABLE (drawable));
  self->type = type;
  self->any.drawable = (GdkDrawable*) g_object_ref (drawable);
  switch (self->type)
    {
    case BST_SEGMENT_LINE:
      self->line.x1 = -1;
      self->line.y1 = -1;
      self->line.x2 = -1;
      self->line.y2 = -1;
      break;
    }
}
void
bst_segment_clear (BstSegment *self)
{
  bst_segment_expose (self);
  switch (self->type)
    {
    case BST_SEGMENT_LINE:
      break;
    }
  g_object_unref (self->any.drawable);
  memset (self, 0, sizeof (*self));
}
void
bst_segment_xrange (BstSegment     *self,
                    gdouble        *x_p,
                    gdouble        *width_p)
{
  gdouble x1 = 0, x2 = 0;
  switch (self->type)
    {
    case BST_SEGMENT_LINE:
      x1 = self->line.x1;
      x2 = self->line.x2;
      break;
    }
  if (x_p)
    *x_p = MIN (x1, x2);
  if (width_p)
    *width_p = MAX (x1, x2) - MIN (x1, x2) + 1;
}
void
bst_segment_yrange (BstSegment     *self,
                    gdouble        *y_p,
                    gdouble        *height_p)
{
  gdouble y1 = 0, y2 = 0;
  switch (self->type)
    {
    case BST_SEGMENT_LINE:
      y1 = self->line.y1;
      y2 = self->line.y2;
      break;
    }
  if (y_p)
    *y_p = MIN (y1, y2);
  if (height_p)
    *height_p = MAX (y1, y2) - MIN (y1, y2) + 1;
}
gdouble
bst_segment_calcy (BstSegment *self,
                   gdouble     x)
{
  gdouble v = 0;
  switch (self->type)
    {
      gdouble d;
    case BST_SEGMENT_LINE:
      d = self->line.y2 - self->line.y1;
      d /= self->line.x2 - self->line.x1;
      v = self->line.y1 + d * (x - self->line.x1);
      break;
    }
  return v;
}
void
bst_segment_start (BstSegment     *self,
                   gdouble         x,
                   gdouble         y)
{
  bst_segment_expose (self);
  switch (self->type)
    {
    case BST_SEGMENT_LINE:
      self->line.x1 = x;
      self->line.y1 = y;
      self->line.x2 = x;
      self->line.y2 = y;
      break;
    }
  bst_segment_expose (self);
}
void
bst_segment_move_to (BstSegment     *self,
                     gdouble         x,
                     gdouble         y)
{
  bst_segment_expose (self);
  switch (self->type)
    {
    case BST_SEGMENT_LINE:
      self->line.x2 = x;
      self->line.y2 = y;
      break;
    }
  bst_segment_expose (self);
}
void
bst_segment_translate (BstSegment     *self,
                       gdouble         xdiff,
                       gdouble         ydiff)
{
  switch (self->type)
    {
    case BST_SEGMENT_LINE:
      self->line.x1 += xdiff;
      self->line.x2 += xdiff;
      self->line.y1 += ydiff;
      self->line.y2 += ydiff;
      break;
    }
}
void
bst_segment_draw (BstSegment     *self,
                  GtkStyle       *style)
{
  GdkDrawable *drawable = self->any.drawable;
  GdkGC *black_gc = style->fg_gc[GTK_STATE_NORMAL];
  GdkGC *dark_gc = style->dark_gc[GTK_STATE_NORMAL];
  switch (self->type)
    {
    case BST_SEGMENT_LINE:
      gdk_draw_line (drawable, black_gc,
                     self->line.x1 + 0.5, self->line.y1 + 0.5,
                     self->line.x2 + 0.5, self->line.y2 + 0.5);
      gdk_draw_line (drawable, dark_gc,
                     self->line.x1 + 0.5, self->line.y1 + 0.5,
                     self->line.x2 + 0.5, self->line.y1 + 0.5);
      gdk_draw_line (drawable, dark_gc,
                     self->line.x1 + 0.5, self->line.y2 + 0.5,
                     self->line.x2 + 0.5, self->line.y2 + 0.5);
      gdk_draw_line (drawable, dark_gc,
                     self->line.x1 + 0.5, self->line.y1 + 0.5,
                     self->line.x1 + 0.5, self->line.y2 + 0.5);
      gdk_draw_line (drawable, dark_gc,
                     self->line.x2 + 0.5, self->line.y1 + 0.5,
                     self->line.x2 + 0.5, self->line.y2 + 0.5);
      break;
    }
}
