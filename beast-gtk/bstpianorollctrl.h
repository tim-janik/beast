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
#ifndef __BST_PIANO_ROLL_CONTROLLER_H__
#define __BST_PIANO_ROLL_CONTROLLER_H__


#include "bstpianoroll.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
  BST_PIANO_ROLL_TOOL_NONE,
  BST_PIANO_ROLL_TOOL_INSERT,
  BST_PIANO_ROLL_TOOL_RESIZE,
  BST_PIANO_ROLL_TOOL_MOVE,
  BST_PIANO_ROLL_TOOL_DELETE,
  BST_PIANO_ROLL_TOOL_SELECT,
  BST_PIANO_ROLL_TOOL_VSELECT
} BstPianoRollTool;

typedef struct {
  BstPianoRollTool obj_tool1;
  BstPianoRollTool obj_tool2;
  BstPianoRollTool obj_tool3;
  BstPianoRollTool bg_tool1;
  BstPianoRollTool bg_tool2;
  BstPianoRollTool bg_tool3;
  guint		   ref_count;
  BstPianoRoll	  *proll;
  guint		   note_length;
  /* drag data */
  BswIterPartNote *sel_iter;
  guint		   tool_index;
  guint		   obj_id, obj_tick, obj_duration;
  gint		   obj_note, obj_fine_tune;
  gfloat           obj_velocity;
  guint		   xoffset;
  guint		   tick_bound;
} BstPianoRollController;


/* --- API --- */
BstPianoRollController*	bst_piano_roll_controller_new		(BstPianoRoll		*proll);
BstPianoRollController*	bst_piano_roll_controller_ref		(BstPianoRollController	*self);
void			bst_piano_roll_controller_unref		(BstPianoRollController	*self);
void			bst_piano_roll_controller_set_bg_tools	(BstPianoRollController	*self,
								 BstPianoRollTool	 tool1,
								 BstPianoRollTool	 tool2,
								 BstPianoRollTool	 tool3);
void			bst_piano_roll_controller_set_obj_tools	(BstPianoRollController	*self,
								 BstPianoRollTool	 tool1,
								 BstPianoRollTool	 tool2,
								 BstPianoRollTool	 tool3);
void			bst_piano_roll_controller_set_clipboard (BswIterPartNote	*iter);
BswIterPartNote*	bst_piano_roll_controller_get_clipboard	(void);
void			bst_piano_roll_controller_clear		(BstPianoRollController	*self);
void			bst_piano_roll_controller_cut		(BstPianoRollController	*self);
void			bst_piano_roll_controller_copy		(BstPianoRollController	*self);
void			bst_piano_roll_controller_paste		(BstPianoRollController	*self);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_PIANO_ROLL_CONTROLLER_H__ */
