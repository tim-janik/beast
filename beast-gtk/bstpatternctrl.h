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

typedef struct {
  GxkParam        *vraster;
  GxkParam        *steps;
  GxkParam        *step_dir;
  GxkParam        *hwrap;
  GxkParam        *base_octave;
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
