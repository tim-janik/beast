/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002 Tim Janik
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
#ifndef __BST_EVENT_ROLL_H__
#define __BST_EVENT_ROLL_H__

#include	"bstdragutils.h"
#include	"bstsegment.h"

G_BEGIN_DECLS

/* --- type macros --- */
#define BST_TYPE_EVENT_ROLL              (bst_event_roll_get_type ())
#define BST_EVENT_ROLL(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_EVENT_ROLL, BstEventRoll))
#define BST_EVENT_ROLL_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_EVENT_ROLL, BstEventRollClass))
#define BST_IS_EVENT_ROLL(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_EVENT_ROLL))
#define BST_IS_EVENT_ROLL_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_EVENT_ROLL))
#define BST_EVENT_ROLL_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_EVENT_ROLL, BstEventRollClass))


/* --- typedefs & enums --- */
typedef struct _BstEventRoll        BstEventRoll;
typedef struct _BstEventRollClass   BstEventRollClass;


/* --- structures & typedefs --- */
typedef struct {
  BstEventRoll *eroll;
  BstDragStatus type;		        /* emission type: start/motion/done/abort */
  BstDragMode   mode;
  guint	        button;
  gint          tick_width;
  guint	        start_tick;
  gfloat        start_value;
  guint		start_valid : 1;
  guint         current_tick;
  gfloat        current_value;          /* between -1 and +1 if valid */
  gfloat        current_value_raw;
  guint		current_valid : 1;	/* value out of range */
  /* user data */
  BstDragStatus state;		        /* request type: unhandled/continue/handled/error */
} BstEventRollDrag;
struct _BstEventRoll
{
  GtkContainer	 parent_instance;

  SfiProxy	 proxy;
  guint          control_type;
  GtkWidget     *child;

  /* horizontal layout */
  guint		 ppqn;		/* parts per quarter note */
  guint		 qnpt;		/* quarter notes per tact */
  guint		 max_ticks;	/* in ticks */
  gfloat	 hzoom;
  guint		 draw_qn_grid : 1;
  guint		 draw_qqn_grid : 1;

  /* scroll offset */
  gint		 x_offset, y_offset;

  gint         (*fetch_vpanel_width) (gpointer data);
  gpointer       fetch_vpanel_width_data;
  guint		 vpanel_width;
  GdkWindow	*vpanel, *canvas;
  GdkCursorType	 canvas_cursor, vpanel_cursor;
#define BST_EVENT_ROLL_N_COLORS (2)
  GdkGC		*color_gc[BST_EVENT_ROLL_N_COLORS];

  BstSegment     segment;

  GtkAdjustment	*hadjustment;
  guint		 scroll_timer;

  /* selection rectangle */
  guint		 selection_tick;
  guint		 selection_duration;
  gint		 selection_min_note;
  gint		 selection_max_note;

  /* drag operations */
  guint		   canvas_drag : 1;
  guint		   vpanel_drag : 1;
  BstEventRollDrag drag;
};
struct _BstEventRollClass
{
  GtkContainerClass parent_class;

  void		(*set_scroll_adjustments)	(BstEventRoll	  *proll,
						 GtkAdjustment	  *hadjustment,
						 GtkAdjustment	  *vadjustment);
  void		(*canvas_drag)			(BstEventRoll	  *self,
						 BstEventRollDrag *drag);
  void		(*canvas_clicked)		(BstEventRoll	  *proll,
						 guint		   button,
						 guint		   tick_position,
						 gfloat            value,
						 GdkEvent	  *event);
  void		(*vpanel_drag)			(BstEventRoll	  *self,
						 BstEventRollDrag *drag);
  void		(*vpanel_clicked)		(BstEventRoll	  *proll,
						 guint		   button,
						 gfloat            value,
						 GdkEvent	  *event);
};


/* --- prototypes --- */
GType       bst_event_roll_get_type              (void);
void        bst_event_roll_set_proxy             (BstEventRoll   *self,
                                                  SfiProxy        proxy);
void        bst_event_roll_set_hadjustment       (BstEventRoll   *self,
                                                  GtkAdjustment  *adjustment);
gfloat      bst_event_roll_set_hzoom             (BstEventRoll   *self,
                                                  gfloat          hzoom);
void        bst_event_roll_set_canvas_cursor     (BstEventRoll   *self,
                                                  GdkCursorType   cursor);
void        bst_event_roll_set_vpanel_cursor     (BstEventRoll   *self,
                                                  GdkCursorType   cursor);
void        bst_event_roll_set_view_selection    (BstEventRoll   *self,
                                                  guint           tick,
                                                  guint           duration);
void        bst_event_roll_set_vpanel_width_hook (BstEventRoll   *self,
                                                  gint          (*fetch_vpanel_width) (gpointer data),
                                                  gpointer        data);
void        bst_event_roll_set_control_type      (BstEventRoll   *self,
                                                  guint           control_type);
void        bst_event_roll_init_segment          (BstEventRoll   *self,
                                                  BstSegmentType  type);
void        bst_event_roll_segment_start         (BstEventRoll   *self,
                                                  guint           tick,
                                                  gfloat          value);
void        bst_event_roll_segment_move_to       (BstEventRoll   *self,
                                                  guint           tick,
                                                  gfloat          value);
void        bst_event_roll_segment_tick_range    (BstEventRoll   *self,
                                                  guint          *tick,
                                                  guint          *duration);
gdouble     bst_event_roll_segment_value         (BstEventRoll   *self,
                                                  guint           tick);
void        bst_event_roll_clear_segment         (BstEventRoll   *self);

G_END_DECLS

#endif /* __BST_EVENT_ROLL_H__ */
