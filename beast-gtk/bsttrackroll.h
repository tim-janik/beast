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
#ifndef __BST_TRACK_ROLL_H__
#define __BST_TRACK_ROLL_H__

#include "bstutils.h"

G_BEGIN_DECLS

/* --- type macros --- */
#define BST_TYPE_TRACK_ROLL              (bst_track_roll_get_type ())
#define BST_TRACK_ROLL(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_TRACK_ROLL, BstTrackRoll))
#define BST_TRACK_ROLL_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_TRACK_ROLL, BstTrackRollClass))
#define BST_IS_TRACK_ROLL(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_TRACK_ROLL))
#define BST_IS_TRACK_ROLL_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_TRACK_ROLL))
#define BST_TRACK_ROLL_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_TRACK_ROLL, BstTrackRollClass))


/* --- typedefs & enums --- */
typedef struct _BstTrackRoll              BstTrackRoll;
typedef struct _BstTrackRollClass         BstTrackRollClass;
typedef SfiProxy (*BstTrackRollTrackFunc)   (gpointer proxy_data,
                                             gint     row);


/* --- structures & typedefs --- */
typedef enum    /*< skip >*/
{
  BST_TRACK_ROLL_MARKER_NONE,
  BST_TRACK_ROLL_MARKER_POS,
  BST_TRACK_ROLL_MARKER_LOOP,
  BST_TRACK_ROLL_MARKER_SELECT
} BstTrackRollMarkerType;
typedef struct {
  GXK_SCROLL_CANVAS_DRAG_FIELDS;
  guint         start_row;
  SfiProxy      start_track;
  guint         start_tick;
  gboolean      start_valid;
  guint         current_row;
  SfiProxy      current_track;
  guint         current_tick;
  gboolean      current_valid;
  /* convenience: */
  BstTrackRoll *troll;
} BstTrackRollDrag;
struct _BstTrackRoll
{
  GxkScrollCanvas   parent_instance;

  SfiProxy          proxy;
  GtkTreeView      *tree;
  guint             n_scopes;   /* does not always reflect number of rows */
  GtkWidget       **scopes;
  guint             scope_update;
  
  /* horizontal layout */
  guint          tpt;           /* ticks (parts) per tact */
  guint          max_ticks;
  gdouble        hzoom;
  guint          draw_tact_grid : 1;

  guint          prelight_row;
  guint          hpanel_height;

  /* editable popup */
  GtkCellEditable *ecell;
  guint            ecell_row;
  guint            ecell_tick;
  guint            ecell_duration;

  /* size queries */
  gint                  area_offset;

  /* BseTrack retrieval */
  gpointer              proxy_data;
  BstTrackRollTrackFunc get_track;

  /* last drag state */
  guint         start_row;
  SfiProxy      start_track;
  guint         start_tick;
  gboolean      start_valid;
};
struct _BstTrackRollClass
{
  GxkScrollCanvasClass parent_class;

  void          (*select_row)                   (BstTrackRoll     *troll,
                                                 gint              row);
  void          (*drag)                         (BstTrackRoll     *self,
                                                 BstTrackRollDrag *drag);
  void          (*clicked)                      (BstTrackRoll     *troll,
                                                 guint             button,
                                                 guint             row,
                                                 guint             tick_position,
                                                 GdkEvent         *event);
  void          (*stop_edit)                    (BstTrackRoll     *self,
                                                 gboolean          canceled,
                                                 GtkCellEditable  *ecell);
};


/* --- prototypes --- */
GType   bst_track_roll_get_type            (void);
void    bst_track_roll_setup               (BstTrackRoll           *troll,
                                            GtkTreeView            *tree,
                                            SfiProxy                song);
gdouble bst_track_roll_set_hzoom           (BstTrackRoll           *troll,
                                            gdouble                 hzoom);
void    bst_track_roll_set_track_callback  (BstTrackRoll           *self,
                                            gpointer                data,
                                            BstTrackRollTrackFunc   get_track);
void    bst_track_roll_check_update_scopes (BstTrackRoll           *self);
void    bst_track_roll_reselect            (BstTrackRoll           *self);
void    bst_track_roll_queue_row_change    (BstTrackRoll           *self,
                                            guint                   row);
void    bst_track_roll_set_prelight_row    (BstTrackRoll           *self,
                                            guint                   row);
void    bst_track_roll_start_edit          (BstTrackRoll           *self,
                                            guint                   row,
                                            guint                   tick,
                                            guint                   duration,
                                            GtkCellEditable        *ecell);
void    bst_track_roll_stop_edit           (BstTrackRoll           *self);
void    bst_track_roll_abort_edit          (BstTrackRoll           *self);
void    bst_track_roll_set_marker          (BstTrackRoll           *self,
                                            guint                   mark_index,
                                            guint                   position,
                                            BstTrackRollMarkerType  mtype);


G_END_DECLS

#endif /* __BST_TRACK_ROLL_H__ */
