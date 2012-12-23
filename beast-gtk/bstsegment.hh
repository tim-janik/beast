// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_SEGMENT_H__
#define __BST_SEGMENT_H__
#include "bstutils.hh"
G_BEGIN_DECLS
/* --- structures & typedefs --- */
typedef enum {
  BST_SEGMENT_LINE = 1,
} BstSegmentType;
typedef struct
{
  BstSegmentType type;
  GdkDrawable   *drawable;
} BstSegmentAny;
typedef struct
{
  BstSegmentAny  any;
  gdouble        x1, y1;
  gdouble        x2, y2;
} BstSegmentLine;
typedef union
{
  BstSegmentType   type;
  BstSegmentAny    any;
  BstSegmentLine   line;
} BstSegment;
/* --- API --- */
void    bst_segment_init        (BstSegment     *self,
                                 BstSegmentType  type,
                                 GdkDrawable    *drawable);
gint    bst_segment_initialized (BstSegment     *self);
void    bst_segment_start       (BstSegment     *self,
                                 gdouble         x,
                                 gdouble         y);
void    bst_segment_move_to     (BstSegment     *self,
                                 gdouble         x,
                                 gdouble         y);
void    bst_segment_translate   (BstSegment     *self,
                                 gdouble         xdiff,
                                 gdouble         ydiff);
void    bst_segment_xrange      (BstSegment     *self,
                                 gdouble        *x,
                                 gdouble        *width);
void    bst_segment_yrange      (BstSegment     *self,
                                 gdouble        *x,
                                 gdouble        *height);
gdouble bst_segment_calcy       (BstSegment     *self,
                                 gdouble         x);
void    bst_segment_bbox        (BstSegment     *self,
                                 GdkRectangle   *area);
void    bst_segment_expose      (BstSegment     *self);
void    bst_segment_draw        (BstSegment     *self,
                                 GtkStyle       *style);
void    bst_segment_clear       (BstSegment     *self);
G_END_DECLS
#endif /* __BST_SEGMENT_H__ */
