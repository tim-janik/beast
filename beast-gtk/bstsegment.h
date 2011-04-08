/* BEAST - Better Audio System
 * Copyright (C) 2003 Tim Janik
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
#ifndef __BST_SEGMENT_H__
#define __BST_SEGMENT_H__

#include "bstutils.h"

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
