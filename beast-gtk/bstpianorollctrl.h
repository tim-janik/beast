/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002-2003 Tim Janik
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

G_BEGIN_DECLS

typedef struct {
  /* misc data */
  guint		   ref_count;
  BstPianoRoll	  *proll;
  /* drag data */
  guint		   obj_id, obj_tick, obj_duration;
  gint		   obj_note, obj_fine_tune;
  gfloat           obj_velocity;
  guint		   xoffset;
  guint		   tick_bound;
  BsePartNoteSeq  *sel_pseq;
  /* tool data */
  guint		   tool_index;
  /* tool selections */
  GxkActionGroup  *note_rtools;
  GxkActionGroup  *quant_rtools;
  GxkActionGroup  *canvas_rtools;
} BstPianoRollController;


/* --- API --- */
BstPianoRollController*	bst_piano_roll_controller_new		 (BstPianoRoll		 *proll);
BstPianoRollController*	bst_piano_roll_controller_ref		 (BstPianoRollController *self);
void			bst_piano_roll_controller_unref		 (BstPianoRollController *self);
guint                   bst_piano_roll_controller_quantize       (BstPianoRollController *self,
                                                                 guint                    fine_tick);
void			bst_piano_roll_controller_set_clipboard  (BsePartNoteSeq	 *pseq);
BsePartNoteSeq*		bst_piano_roll_controller_get_clipboard	 (void);
GxkActionList*          bst_piano_roll_controller_canvas_actions (BstPianoRollController *self);
GxkActionList*          bst_piano_roll_controller_note_actions   (BstPianoRollController *self);
GxkActionList*          bst_piano_roll_controller_quant_actions  (BstPianoRollController *self);
void			bst_piano_roll_controller_clear		 (BstPianoRollController *self);
void			bst_piano_roll_controller_cut		 (BstPianoRollController *self);
gboolean		bst_piano_roll_controller_copy		 (BstPianoRollController *self);
void			bst_piano_roll_controller_paste		 (BstPianoRollController *self);
gboolean                bst_piano_roll_controler_clipboard_full  (BstPianoRollController *self);


G_END_DECLS

#endif /* __BST_PIANO_ROLL_CONTROLLER_H__ */
