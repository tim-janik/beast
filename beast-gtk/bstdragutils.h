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
#ifndef __BST_DRAG_UTILS_H__
#define __BST_DRAG_UTILS_H__

#include        "bstdefs.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef enum	/*< skip >*/
{
  BST_DRAG_AREA_RESIZE		= 1 << 0,
  BST_DRAG_AREA_ENRICH		= 1 << 1,
  BST_DRAG_AREA_REDUCE		= 1 << 2,
  BST_DRAG_AREA_XOR		= 1 << 3,
  BST_DRAG_RATIO_FIXED		= 1 << 8,
  BST_DRAG_OFFSET_CENTERED	= 1 << 9,
} BstDragMode;

BstDragMode	bst_drag_modifier_start	(GdkModifierType	key_mods);
BstDragMode	bst_drag_modifier_next	(GdkModifierType	key_mods,
					 BstDragMode		last_drag_mods);
typedef enum	/*< skip >*/
{
  /* drag emission state */
  BST_DRAG_START,	  /* initial drag event */
  BST_DRAG_MOTION,	  /* drag motion, pointer moved */
  BST_DRAG_DONE,	  /* final drag motion */
  BST_DRAG_ABORT,	  /* drag abortion requested */
  /* drag-action requests */
  BST_DRAG_UNHANDLED	= BST_DRAG_START,	/* continue with button-press or similar */
  BST_DRAG_CONTINUE	= BST_DRAG_MOTION,	/* request drag-motion emissions */
  BST_DRAG_HANDLED	= BST_DRAG_DONE,	/* no further emissions */
  BST_DRAG_ERROR	= BST_DRAG_ABORT	/* request abortion */
} BstDragStatus;


typedef void (*BstControllerDestroy) (gpointer	 controller,
				      GtkWidget	*view);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_DRAG_UTILS_H__ */
