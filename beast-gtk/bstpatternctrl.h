/* BEAST - Bedevilled Audio System
 * Copyright (C) 2004 Tim Janik
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
#ifndef __BST_PATTERN_CONTROLLER_H__
#define __BST_PATTERN_CONTROLLER_H__

#include "bstpatternview.h"

G_BEGIN_DECLS

typedef enum /*< skip >*/
{
  /* 0xff reserved for BstPatternAction values */
#define BST_PATTERN_MASK_ACTION    (0x000000ff)
  /* base octave */
  BST_PATTERN_SET_BASE_OCTAVE     = 0x1 << 8,
  BST_PATTERN_CHANGE_BASE_OCTAVE  = 0x2 << 8,
#define BST_PATTERN_MASK_CONTROLS  (0x0000ff00)
  /* focus movement */
  BST_PATTERN_MOVE_LEFT           = 0x1 << 16,
  BST_PATTERN_MOVE_RIGHT          = 0x2 << 16,
  BST_PATTERN_MOVE_UP             = 0x3 << 16,
  BST_PATTERN_MOVE_DOWN           = 0x4 << 16,
  BST_PATTERN_PAGE_LEFT           = 0x5 << 16,
  BST_PATTERN_PAGE_RIGHT          = 0x6 << 16,
  BST_PATTERN_PAGE_UP             = 0x7 << 16,
  BST_PATTERN_PAGE_DOWN           = 0x8 << 16,
  BST_PATTERN_JUMP_LEFT           = 0x9 << 16,
  BST_PATTERN_JUMP_RIGHT          = 0xa << 16,
  BST_PATTERN_JUMP_TOP            = 0xb << 16,
  BST_PATTERN_JUMP_BOTTOM         = 0xc << 16,
  BST_PATTERN_MOVE_NEXT           = 0xd << 16,
#define BST_PATTERN_MASK_MOVEMENT  (0x00ff0000)
} BstPatternFunction;

typedef struct {
  /* misc data */
  guint            ref_count;
  BstPatternView  *pview;
  /* tool selections */
  GxkActionGroup  *quant_rtools;
} BstPatternController;


BstPatternController* bst_pattern_controller_new                  (BstPatternView       *pview,
                                                                   GxkActionGroup       *quant_rtools);
BstPatternController* bst_pattern_controller_ref                  (BstPatternController *self);
void                  bst_pattern_controller_unref                (BstPatternController *self);
BstKeyBinding*        bst_pattern_controller_default_generic_keys (void);
BstKeyBinding*        bst_pattern_controller_generic_keys         (void);
BstKeyBinding*        bst_pattern_controller_default_piano_keys   (void);
BstKeyBinding*        bst_pattern_controller_piano_keys           (void);

G_END_DECLS

#endif /* __BST_PATTERN_CONTROLLER_H__ */
