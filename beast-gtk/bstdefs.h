/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BST_DEFS_H__
#define __BST_DEFS_H__

#include        <bse/bse.h>
#include	<gtk/gtk.h>
#include        <gle/gle.h>
#include	<gnome.h>

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */


typedef enum
{
  BST_OP_NONE,

  /* project operations
   */
  BST_OP_PROJECT_NEW,
  BST_OP_PROJECT_OPEN,
  BST_OP_PROJECT_SAVE,
  BST_OP_PROJECT_SAVE_AS,
  BST_OP_REBUILD,
  BST_OP_REFRESH,

  /* song operations
   */
  BST_OP_SONG_NEW,
  BST_OP_PATTERN_ADD,
  BST_OP_PATTERN_DELETE,
  BST_OP_PATTERN_EDITOR,
  BST_OP_INSTRUMENT_ADD,
  BST_OP_INSTRUMENT_DELETE,

  /* super operations
   */
  BST_OP_UNDO_LAST,
  BST_OP_REDO_LAST,
  BST_OP_PLAY,
  BST_OP_STOP,

  /* application wide
   */
  BST_OP_CLOSE,
  BST_OP_EXIT,
  BST_OP_HELP_ABOUT,

  BST_OP_LAST
} BstOps;

#define	BST_TAG_DIAMETER	(20)

/* it's hackish to have these prototypes in here, but we need
 * 'em somewhere, implementations are in bstmain.c
 */
extern void bst_update_can_operate (GtkWidget   *some_widget);
extern void bst_object_set         (gpointer     object,
				    const gchar *first_arg_name,
				    ...); /* hackery rulez! */

extern GnomeCanvasPoints* gnome_canvas_points_new0 (guint num_points);


     
#ifdef __cplusplus
#pragma {
}
#endif /* __cplusplus */

#endif /* __BST_DEFS_H__ */
