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
#ifndef __BST_TRACK_ROLL_CONTROLLER_H__
#define __BST_TRACK_ROLL_CONTROLLER_H__


#include "bsttrackroll.h"

G_BEGIN_DECLS

typedef enum /*< skip >*/
{
  BST_TRACK_ROLL_TOOL_NONE,
  /* choose IDs that are unlikely to clash with category IDs */
  BST_TRACK_ROLL_TOOL_INSERT		= G_MAXINT - 1000,
  BST_TRACK_ROLL_TOOL_EDIT_NAME,
  BST_TRACK_ROLL_TOOL_MOVE,
  BST_TRACK_ROLL_TOOL_DELETE
} BstTrackRollTool;

typedef struct {
  BstTrackRollTool obj_tool1;
  BstTrackRollTool obj_tool2;
  BstTrackRollTool obj_tool3;
  BstTrackRollTool bg_tool1;
  BstTrackRollTool bg_tool2;
  BstTrackRollTool bg_tool3;
  guint		   ref_count;
  BstTrackRoll	  *troll;
  guint		   note_length;
  /* drag data */
  guint		   tool_index;
  SfiProxy	   obj_track, obj_part;
  guint		   obj_tick, obj_duration;
  guint		   xoffset;
  guint		   tick_bound;
} BstTrackRollController;


/* --- API --- */
BstTrackRollController*	bst_track_roll_controller_new		(BstTrackRoll		*troll);
BstTrackRollController*	bst_track_roll_controller_ref		(BstTrackRollController	*self);
void			bst_track_roll_controller_unref		(BstTrackRollController	*self);
void                    bst_track_roll_controller_set_bg_tools  (BstTrackRollController *self,
								 BstTrackRollTool        tool1,
								 BstTrackRollTool        tool2,
								 BstTrackRollTool        tool3);
void                    bst_track_roll_controller_set_obj_tools (BstTrackRollController *self,
								 BstTrackRollTool        tool1,
								 BstTrackRollTool        tool2,
								 BstTrackRollTool        tool3);


G_END_DECLS

#endif /* __BST_TRACK_ROLL_CONTROLLER_H__ */
