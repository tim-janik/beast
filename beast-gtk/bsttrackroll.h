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
#ifndef __BST_TRACK_ROLL_H__
#define __BST_TRACK_ROLL_H__

#include "bstdragutils.h"
#include "bstsnifferscope.h"
#include "bstmarker.h"

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
typedef struct {
  BstTrackRoll *troll;
  BstDragStatus type : 16;		/* emission type: start/motion/done/abort */
  guint		canvas_drag : 1;
  guint		vpanel_drag : 1;
  guint		hpanel_drag : 1;
  BstDragMode   mode : 16;
  guint16       button;
  guint         start_row;
  SfiProxy      start_track;
  guint	        start_tick;
  gboolean      start_valid;
  guint         current_row;
  SfiProxy      current_track;
  guint         current_tick;
  gboolean      current_valid;
  /* user data */
  BstDragStatus state;		/* request type: unhandled/continue/handled/error */
} BstTrackRollDrag;
struct _BstTrackRoll
{
  GtkContainer	 parent_instance;

  SfiProxy          proxy;
  GtkTreeView      *tree;
  guint             n_scopes;   /* does not always reflect number of rows */
  BstSnifferScope **scopes;
  guint             scope_update;
  
  /* horizontal layout */
  guint		 tpt;		/* ticks (parts) per tact */
  guint		 max_ticks;
  gdouble	 hzoom;
  guint		 draw_tact_grid : 1;

  /* scroll offset */
  gint		 x_offset, y_offset;

  guint		 prelight_row;
  guint		 hpanel_height;
  GdkWindow	*canvas, *vpanel, *hpanel;
  GdkCursorType	 canvas_cursor, vpanel_cursor, hpanel_cursor;

  GtkAdjustment	*hadjustment, *vadjustment;
  guint		 scroll_timer;

  /* editable popup */
  GtkCellEditable *ecell;
  guint		   ecell_row;
  guint		   ecell_tick;
  guint		   ecell_duration;

  /* size queries */
  gint			  area_offset;

  /* BseTrack retrieval */
  gpointer              proxy_data;
  BstTrackRollTrackFunc get_track;

  /* marks */
  BstMarkerSetup	vmarker;

  /* drag operations */
  guint		   in_drag : 1;
  BstTrackRollDrag drag;
};
struct _BstTrackRollClass
{
  GtkContainerClass parent_class;

  void		(*set_scroll_adjustments)	(BstTrackRoll	  *troll,
						 GtkAdjustment	  *hadjustment,
						 GtkAdjustment	  *vadjustment);
  void		(*select_row)			(BstTrackRoll	  *troll,
						 gint		   row);
  void		(*drag)				(BstTrackRoll	  *self,
						 BstTrackRollDrag *drag);
  void		(*clicked)			(BstTrackRoll	  *troll,
						 guint		   button,
						 guint             row,
						 guint		   tick_position,
						 GdkEvent	  *event);
  void		(*stop_edit)			(BstTrackRoll	  *self,
						 gboolean	   canceled,
						 GtkCellEditable  *ecell);
};


/* --- prototypes --- */
GType	bst_track_roll_get_type			(void);
void    bst_track_roll_setup                    (BstTrackRoll   *troll,
                                                 GtkTreeView    *tree,
                                                 SfiProxy        song);
void	bst_track_roll_set_hadjustment		(BstTrackRoll	*troll,
						 GtkAdjustment	*adjustment);
void	bst_track_roll_set_vadjustment		(BstTrackRoll	*troll,
						 GtkAdjustment	*adjustment);
gdouble	bst_track_roll_set_hzoom		(BstTrackRoll	*troll,
						 gdouble	 hzoom);
void	bst_track_roll_set_canvas_cursor	(BstTrackRoll	*troll,
						 GdkCursorType	 cursor);
void	bst_track_roll_set_hpanel_cursor	(BstTrackRoll	*troll,
						 GdkCursorType	 cursor);
void	bst_track_roll_set_vpanel_cursor	(BstTrackRoll	*troll,
						 GdkCursorType	 cursor);
void	bst_track_roll_set_track_callback	(BstTrackRoll   *self,
						 gpointer        data,
						 BstTrackRollTrackFunc get_track);
void	bst_track_roll_reallocate		(BstTrackRoll	*self);
void	bst_track_roll_check_update_scopes	(BstTrackRoll	*self);
void	bst_track_roll_reselect 		(BstTrackRoll	*self);
void	bst_track_roll_queue_draw_row		(BstTrackRoll	*self,
						 guint		 row);
void	bst_track_roll_set_prelight_row		(BstTrackRoll	*self,
						 guint		 row);
void	bst_track_roll_start_edit		(BstTrackRoll	*self,
						 guint           row,
						 guint           tick,
						 guint           duration,
						 GtkCellEditable*ecell);
void	bst_track_roll_stop_edit		(BstTrackRoll	*self);
void	bst_track_roll_abort_edit		(BstTrackRoll	*self);
void	bst_track_roll_set_mark			(BstTrackRoll	*self,
						 guint		 mark_index,
						 guint		 position,
						 BstMarkerType	 type);


G_END_DECLS

#endif /* __BST_TRACK_ROLL_H__ */
