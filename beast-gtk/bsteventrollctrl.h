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
#ifndef __BST_EVENT_ROLL_CONTROLLER_H__
#define __BST_EVENT_ROLL_CONTROLLER_H__

#include "bsteventroll.h"

G_BEGIN_DECLS

typedef struct {
  /* misc data */
  guint		     ref_count;
  BstEventRoll	    *eroll;
  /* drag data */
  guint		     obj_id, obj_tick;
  gfloat             obj_value;
  BsePartControlSeq *sel_cseq;
  BstSegment        *segment;
  /* tool data */
  guint		     tool_index;
  /* tool selections */
  GxkActionGroup  *quant_rtools;
  GxkActionGroup  *canvas_rtools;
} BstEventRollController;


/* --- API --- */
BstEventRollController*	bst_event_roll_controller_new		(BstEventRoll		*eroll,
                                                                 GxkActionGroup         *quant_rtools,
                                                                 GxkActionGroup         *canvas_rtools);
BstEventRollController*	bst_event_roll_controller_ref		(BstEventRollController	*self);
void			bst_event_roll_controller_unref		(BstEventRollController	*self);
guint                   bst_event_roll_controller_quantize      (BstEventRollController *self,
                                                                 guint                   fine_tick);
void			bst_event_roll_controller_set_clipboard (BsePartControlSeq	*cseq);
BsePartControlSeq*	bst_event_roll_controller_get_clipboard	(void);
void			bst_event_roll_controller_clear		(BstEventRollController	*self);
void			bst_event_roll_controller_cut		(BstEventRollController	*self);
gboolean		bst_event_roll_controller_copy		(BstEventRollController	*self);
void			bst_event_roll_controller_paste		(BstEventRollController	*self);
gboolean                bst_event_roll_controler_clipboard_full (BstEventRollController *self);


G_END_DECLS

#endif /* __BST_EVENT_ROLL_CONTROLLER_H__ */
