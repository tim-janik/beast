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
#include "bstradiotools.h"

G_BEGIN_DECLS

typedef struct _BstTrackRollUtil BstTrackRollUtil;
typedef struct {
  /* misc data */
  guint		    ref_count;
  BstTrackRoll	   *troll;
  SfiProxy	    song;
  guint		    note_length;
  /* drag data */
  SfiProxy	    obj_track, obj_part;
  guint		    obj_tick, obj_duration;
  guint		    xoffset;
  guint		    tick_bound;
  /* tool data */
  BstTrackRollUtil *current_tool;
  guint		    skip_deletion : 1;
  /* tool selections */
  BstRadioTools    *canvas_rtools;
  BstRadioTools    *hpanel_rtools;
  BstRadioTools    *quant_rtools;
} BstTrackRollController;


/* --- API --- */
BstTrackRollController*	bst_track_roll_controller_new		(BstTrackRoll		*troll);
BstTrackRollController*	bst_track_roll_controller_ref		(BstTrackRollController	*self);
void			bst_track_roll_controller_unref		(BstTrackRollController	*self);
void		bst_track_roll_controller_set_song		(BstTrackRollController	*self,
								 SfiProxy		 song);
void		bst_track_roll_controller_set_quantization	(BstTrackRollController *self,
								 BstQuantizationType     quantization);
guint		bst_track_roll_controller_quantize		(BstTrackRollController *self,
								 guint                   fine_tick);


G_END_DECLS

#endif /* __BST_TRACK_ROLL_CONTROLLER_H__ */
