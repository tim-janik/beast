/* BEAST - Better Audio System
 * Copyright (C) 2002-2004 Tim Janik
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
#ifndef __BST_PIANO_ROLL_H__
#define __BST_PIANO_ROLL_H__

#include        "bstutils.h"

G_BEGIN_DECLS

/* --- type macros --- */
#define BST_TYPE_PIANO_ROLL              (bst_piano_roll_get_type ())
#define BST_PIANO_ROLL(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_PIANO_ROLL, BstPianoRoll))
#define BST_PIANO_ROLL_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_PIANO_ROLL, BstPianoRollClass))
#define BST_IS_PIANO_ROLL(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_PIANO_ROLL))
#define BST_IS_PIANO_ROLL_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_PIANO_ROLL))
#define BST_PIANO_ROLL_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_PIANO_ROLL, BstPianoRollClass))


/* --- typedefs & enums --- */
typedef struct _BstPianoRoll        BstPianoRoll;
typedef struct _BstPianoRollClass   BstPianoRollClass;
typedef enum    /*< skip >*/
{
  BST_PIANO_ROLL_MARKER_NONE,
  BST_PIANO_ROLL_MARKER_POINTER,
  BST_PIANO_ROLL_MARKER_SELECT
} BstPianoRollMarkerType;


/* --- structures & typedefs --- */
typedef struct {
  GXK_SCROLL_CANVAS_DRAG_FIELDS;
  guint	        start_tick;
  gint          start_note;
  guint		start_valid : 1;        /* note out of range or non-existant black key */
  guint         current_tick;
  gint          current_note;
  guint		current_valid : 1;	/* note out of range or non-existant black key */
  /* convenience: */
  BstPianoRoll *proll;
} BstPianoRollDrag;
struct _BstPianoRoll
{
  GxkScrollCanvas parent_instance;

  SfiProxy	 proxy;
  SfiProxy	 song;
  BsePartLinkSeq*plinks;
  gint		 min_note;
  gint		 max_note;
  guint		 vzoom;

  /* horizontal layout */
  guint		 ppqn;		/* parts per quarter note */
  guint		 qnpt;		/* quarter notes per tact */
  guint		 max_ticks;	/* in ticks */
  gfloat	 hzoom;

  /* last drag state */
  guint          start_tick;
  gint           start_note;
  guint          start_valid : 1;

  guint		 draw_qn_grid : 1;
  guint		 draw_qqn_grid : 1;

  /* slight hack */
  guint          release_closes_toplevel : 1;

  /* selection rectangle */
  guint		 selection_tick;
  guint		 selection_duration;
  gint		 selection_min_note;
  gint		 selection_max_note;
};
struct _BstPianoRollClass
{
  GxkScrollCanvasClass parent_class;

  void		(*canvas_drag)			(BstPianoRoll	  *self,
						 BstPianoRollDrag *drag);
  void		(*canvas_clicked)		(BstPianoRoll	  *proll,
						 guint		   button,
						 guint		   tick_position,
						 gint              note,
						 GdkEvent	  *event);
  void		(*piano_drag)			(BstPianoRoll	  *self,
						 BstPianoRollDrag *drag);
  void		(*piano_clicked)		(BstPianoRoll	  *proll,
						 guint		   button,
						 gint              note,
						 GdkEvent	  *event);
};


/* --- prototypes --- */
GType	bst_piano_roll_get_type			(void);
void	bst_piano_roll_set_proxy		(BstPianoRoll	*self,
						 SfiProxy	 proxy);
gfloat	bst_piano_roll_set_hzoom		(BstPianoRoll	*self,
						 gfloat		 hzoom);
gfloat	bst_piano_roll_set_vzoom		(BstPianoRoll	*self,
						 gfloat		 vzoom);
void	bst_piano_roll_set_view_selection	(BstPianoRoll	*self,
						 guint		 tick,
						 guint		 duration,
						 gint		 min_note,
						 gint		 max_note);
gint	bst_piano_roll_get_vpanel_width		(BstPianoRoll	*self);
void    bst_piano_roll_get_paste_pos		(BstPianoRoll	*self,
						 guint          *tick_p,
						 gint		*note_p);
void    bst_piano_roll_set_marker               (BstPianoRoll          *self,
                                                 guint                  mark_index,
                                                 guint                  position,
                                                 BstPianoRollMarkerType mtype);

     
G_END_DECLS

#endif /* __BST_PIANO_ROLL_H__ */
