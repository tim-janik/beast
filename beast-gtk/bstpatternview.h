/* BEAST - Bedevilled Audio System
 * Copyright (C) 2004 Tim Janik
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
#ifndef __BST_PATTERN_VIEW_H__
#define __BST_PATTERN_VIEW_H__

#include "bstpatterncolumns.h"

G_BEGIN_DECLS

/* --- type macros --- */
#define BST_TYPE_PATTERN_VIEW              (bst_pattern_view_get_type ())
#define BST_PATTERN_VIEW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_PATTERN_VIEW, BstPatternView))
#define BST_PATTERN_VIEW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_PATTERN_VIEW, BstPatternViewClass))
#define BST_IS_PATTERN_VIEW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_PATTERN_VIEW))
#define BST_IS_PATTERN_VIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_PATTERN_VIEW))
#define BST_PATTERN_VIEW_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_PATTERN_VIEW, BstPatternViewClass))


/* --- typedefs & enums --- */
/* bstpatterncolumns.h: typedef struct _BstPatternView      BstPatternView; */
typedef struct _BstPatternViewClass BstPatternViewClass;


/* --- structures & typedefs --- */
typedef enum /*< skip >*/
{
  BST_PATTERN_VIEW_MARKER_NONE,
  BST_PATTERN_VIEW_MARKER_FOCUS,
} BstPatternViewMarkerType;
typedef struct {
  GXK_SCROLL_CANVAS_DRAG_FIELDS;
  guint         start_col;
  guint         start_row;
  guint         start_tick;
  guint         start_duration;
  gboolean      start_valid;
  guint         current_col;
  guint         current_row;
  guint         current_tick;
  guint         current_duration;
  gboolean      current_valid;
  /* convenience: */
  BstPatternView *pview;
} BstPatternViewDrag;
struct _BstPatternView
{
  GxkScrollCanvas    parent_instance;

  SfiProxy           proxy;

  /* vertical layout */
  guint              row_height;
  guint              tpqn;              /* ticks per quarter note */
  guint              tpt;               /* ticks per tact */
  gint               vticks;            /* ticks per row */
  guint              max_ticks;

  /* horizontal layout */
  guint              n_cols;
  BstPatternColumn **cols;
  gchar             *layout_string;

  /* focus cell */
  guint              focus_col;
  guint              focus_row;
  guint              n_focus_cols;
  BstPatternColumn **focus_cols;

  /* last drag state */
  guint              start_col;
  guint              start_row;
  guint              start_tick;
  guint              start_duration;
  gboolean           start_valid;
};
struct _BstPatternViewClass
{
  GxkScrollCanvasClass parent_class;

  void          (*drag)                         (BstPatternView     *self,
                                                 BstPatternViewDrag *drag);
  void          (*clicked)                      (BstPatternView     *tview,
                                                 guint               button,
                                                 guint               row,
                                                 guint               tick_position,
                                                 GdkEvent           *event);
};


/* --- prototypes --- */
GType             bst_pattern_view_get_type        (void);
void              bst_pattern_view_set_proxy       (BstPatternView            *self,
                                                    SfiProxy                   part);
void              bst_pattern_view_vsetup          (BstPatternView            *self,
                                                    guint                      tpqn,
                                                    guint                      qnpt,
                                                    guint                      max_ticks,
                                                    guint                      vticks);
void              bst_pattern_view_set_marker      (BstPatternView            *self,
                                                    guint                      mark_index,
                                                    guint                      position,
                                                    BstPatternViewMarkerType   mtype);
void              bst_pattern_view_set_pixmarker   (BstPatternView            *self,
                                                    guint                      mark_index,
                                                    BstPatternViewMarkerType   mtype,
                                                    gint                       x,
                                                    gint                       y,
                                                    gint                       width,
                                                    gint                       height);
void              bst_pattern_view_add_column      (BstPatternView            *self,
                                                    BstPatternLType            ltype,
                                                    gint                       num,
                                                    BstPatternLFlags           lflags);
void              bst_pattern_view_set_focus       (BstPatternView            *self,
                                                    guint                      focus_col,
                                                    guint                      focus_row);
gint              bst_pattern_view_get_focus_width (BstPatternView            *self);
BstPatternColumn* bst_pattern_view_get_focus_cell  (BstPatternView            *self,
                                                    guint                     *tick,
                                                    guint                     *duration);
gboolean          bst_pattern_view_dispatch_key    (BstPatternView            *self,
                                                    guint                      keyval,
                                                    GdkModifierType            modifier,
                                                    BstPatternFunction         action,
                                                    gdouble                    param,
                                                    BstPatternFunction        *movement);
gint              bst_pattern_view_get_last_row    (BstPatternView            *self);
const gchar*      bst_pattern_view_get_layout      (BstPatternView            *self);
guint             bst_pattern_view_set_layout      (BstPatternView            *self,
                                                    const gchar               *layout);


G_END_DECLS

#endif /* __BST_PATTERN_VIEW_H__ */
