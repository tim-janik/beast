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
#include "bstdragutils.h"

#if 0
typedef enum	/*< skip >*/
{
  /* Gimp style */
  BST_DRAG_AREA_ENRICH,		/* Start: Shift */
  BST_DRAG_AREA_REDUCE,		/* Start: Ctrl */
  BST_DRAG_RATIO_FIXED,		/* Motion: Shift (Fixed Aspect Ratio) */
  BST_DRAG_OFFSET_CENTERED,	/* Motion: Ctrl  (Center Obj around Offset) */
  /* DND style */
  BST_DRAG_ACTION_LINK,		/* Shift + Ctrl */
  BST_DRAG_ACTION_COPY,		/* Ctrl */
  BST_DRAG_ACTION_MOVE,		/* Shift */
  /* Gnumeric style */
  BST_DRAG_AREA_ADD,		/* Ctrl */
  BST_DRAG_AREA_RESIZE_LAST,	/* Shift */
  /* AbiWord Style */
  BST_DRAG_SELECTION_RESIZE,	/* Shift */
  BST_DRAG_SELECT_WORD,		/* Ctrl */
  /* Nautilus Style */
  BST_DRAG_AREA_XOR,		/* Shift or Ctrl */
} BstDragModifier;
#endif


/* --- functions --- */
BstDragMode
bst_drag_modifier_start (GdkModifierType key_mods)
{
  return 0;
}

BstDragMode
bst_drag_modifier_next (GdkModifierType key_mods,
			BstDragMode     last_drag_mods)
{
  return 0;
}
