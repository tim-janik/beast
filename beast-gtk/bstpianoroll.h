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
#ifndef __BST_PIANO_ROLL_H__
#define __BST_PIANO_ROLL_H__

#include        "bstdefs.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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
typedef enum
{
  BST_PIANO_ROLL_DRAG_NONE,
  BST_PIANO_ROLL_DRAG_ADD,
  BST_PIANO_ROLL_DRAG_REMOVE,
  BST_PIANO_ROLL_DRAG_MOVE
} BstPianoRollDragType;


/* --- structures & typedefs --- */
struct _BstPianoRoll
{
  GtkContainer	 parent_instance;

  BswProxy	 proxy;
  guint		 n_octaves;
  guint		 vzoom;

  /* horizontal layout */
  guint		 ppqn;		/* parts per quarter note */
  guint		 qnpt;		/* quarter notes per tact */
  guint		 max_ticks;	/* in ticks */
  gfloat	 hzoom;
  guint		 draw_qn_grid : 1;
  guint		 draw_qqn_grid : 1;
  
  gint		 x_offset, y_offset;

  guint		 hpanel_height;
  GdkWindow	*vpanel, *hpanel, *canvas;
  GdkGC		*color_gc[12];

  /* note dragging/clicks */
  guint16	 drag_type;
  guint16	 drag_button;
  guint		 drag_start;	/* ticks */
  guint		 drag_current;	/* ticks */
  guint		 drag_note;
  guint		 drag_octave;
  guint		 drag_hackid;

  GtkAdjustment	*hadjustment, *vadjustment;
};
struct _BstPianoRollClass
{
  GtkContainerClass parent_class;

  void  (*set_scroll_adjustments) (BstPianoRoll		*proll,
				   GtkAdjustment	*hadjustment,
				   GtkAdjustment	*vadjustment);
};


/* --- prototypes --- */
void proll_test (void);
GType	bst_piano_roll_get_type			(void);
void	bst_piano_roll_set_proxy		(BstPianoRoll	*proll,
						 BswProxy	 proxy);
void	bst_piano_roll_adjust_scroll_area	(BstPianoRoll	*proll);
void	bst_piano_roll_set_hadjustment		(BstPianoRoll	*proll,
						 GtkAdjustment	*adjustment);
void	bst_piano_roll_set_vadjustment		(BstPianoRoll	*proll,
						 GtkAdjustment	*adjustment);
gfloat	bst_piano_roll_set_hzoom		(BstPianoRoll	*proll,
						 gfloat		 hzoom);
gfloat	bst_piano_roll_set_vzoom		(BstPianoRoll	*proll,
						 gfloat		 vzoom);
     
     
     

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_PIANO_ROLL_H__ */
