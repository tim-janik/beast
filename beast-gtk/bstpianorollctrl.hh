/* BEAST - Better Audio System
 * Copyright (C) 2002-2003 Tim Janik
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
#ifndef __BST_PIANO_ROLL_CONTROLLER_H__
#define __BST_PIANO_ROLL_CONTROLLER_H__

#include "bstpianoroll.hh"

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
  /* action cache */
  guint64          cached_stamp;
  guint            cached_n_notes;
} BstPianoRollController;


/* --- API --- */
BstPianoRollController*	bst_piano_roll_controller_new		 (BstPianoRoll		 *proll);
BstPianoRollController*	bst_piano_roll_controller_ref		 (BstPianoRollController *self);
void			bst_piano_roll_controller_unref		 (BstPianoRollController *self);
guint                   bst_piano_roll_controller_quantize       (BstPianoRollController *self,
                                                                 guint                    fine_tick);
void			bst_piano_roll_controller_set_clipboard  (BsePartNoteSeq	 *pseq);
BsePartNoteSeq*		bst_piano_roll_controller_get_clipboard	 (void);
GxkActionList*          bst_piano_roll_controller_select_actions (BstPianoRollController *self);
GxkActionList*          bst_piano_roll_controller_canvas_actions (BstPianoRollController *self);
GxkActionList*          bst_piano_roll_controller_note_actions   (BstPianoRollController *self);
GxkActionList*          bst_piano_roll_controller_quant_actions  (BstPianoRollController *self);
void			bst_piano_roll_controller_clear		 (BstPianoRollController *self);
void			bst_piano_roll_controller_cut		 (BstPianoRollController *self);
gboolean		bst_piano_roll_controller_copy		 (BstPianoRollController *self);
void			bst_piano_roll_controller_paste		 (BstPianoRollController *self);
gboolean                bst_piano_roll_controller_clipboard_full (BstPianoRollController *self);
gboolean                bst_piano_roll_controller_has_selection  (BstPianoRollController *self,
                                                                  guint64                 action_stamp);


G_END_DECLS

#endif /* __BST_PIANO_ROLL_CONTROLLER_H__ */
