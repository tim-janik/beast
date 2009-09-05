/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2003 Tim Janik
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
#ifndef __BST_DEFS_H__
#define __BST_DEFS_H__

#include <gxk/gxk.h>
#include <libintl.h>
#include "bstzoomedwindow.h"

G_BEGIN_DECLS


/* --- generic constants --- */
typedef enum {
  BST_QUANTIZE_NONE		= 0,
  BST_QUANTIZE_NOTE_1		= 1,
  BST_QUANTIZE_NOTE_2		= 2,
  BST_QUANTIZE_NOTE_4		= 4,
  BST_QUANTIZE_NOTE_8		= 8,
  BST_QUANTIZE_NOTE_16		= 16,
  BST_QUANTIZE_NOTE_32		= 32,
  BST_QUANTIZE_NOTE_64		= 64,
  BST_QUANTIZE_NOTE_128		= 128,
  BST_QUANTIZE_TACT		= 65535
} BstQuantizationType;


typedef struct _BstKeyBinding BstKeyBinding;

/* choose IDs that are unlikely to clash with category IDs */
#define BST_COMMON_ROLL_TOOL_FIRST	(G_MAXINT - 100000)
typedef enum /*< skip >*/
{
  BST_COMMON_ROLL_TOOL_NONE,
  BST_COMMON_ROLL_TOOL_INSERT            = BST_COMMON_ROLL_TOOL_FIRST,
  BST_COMMON_ROLL_TOOL_RESIZE,
  BST_COMMON_ROLL_TOOL_LINK,
  BST_COMMON_ROLL_TOOL_RENAME,
  BST_COMMON_ROLL_TOOL_ALIGN,
  BST_COMMON_ROLL_TOOL_MOVE,
  BST_COMMON_ROLL_TOOL_DELETE,
  BST_COMMON_ROLL_TOOL_SELECT,
  BST_COMMON_ROLL_TOOL_VSELECT,
  BST_COMMON_ROLL_TOOL_EDITOR,
  BST_COMMON_ROLL_TOOL_MOVE_TICK_POINTER,
  BST_COMMON_ROLL_TOOL_MOVE_TICK_LEFT,
  BST_COMMON_ROLL_TOOL_MOVE_TICK_RIGHT,
  BST_COMMON_ROLL_TOOL_LAST
} BstCommonRollTool;


/* --- constants & defines --- */
#define	BST_TAG_DIAMETER	  (20)
#define BST_STRDUP_RC_FILE()	  (g_strconcat (g_get_home_dir (), "/.beast/beastrc", NULL))
#define BST_STRDUP_SKIN_PATH()	  (g_strconcat (BST_PATH_SKINS, ":~/.beast/skins/:~/.beast/skins/*/", NULL))


/* --- configuration candidates --- */
/* mouse button numbers and masks for drag operations */
#define BST_DRAG_BUTTON_COPY	  (1)
#define BST_DRAG_BUTTON_COPY_MASK (GDK_BUTTON1_MASK)
#define BST_DRAG_BUTTON_MOVE	  (2)
#define BST_DRAG_BUTTON_MOVE_MASK (GDK_BUTTON2_MASK)
#define BST_DRAG_BUTTON_CONTEXT   (3) /* delete, clone, linkdup */


/* --- miscellaneous --- */
#define	BST_DVL_HINTS		(bst_developer_hints != FALSE)
#define	BST_DBG_EXT     	(bst_debug_extensions != FALSE)
#define	BST_MAIN_LOOP_QUIT()	do { bst_main_loop_running = FALSE; } while (0)
#define	GNOME_CANVAS_NOTIFY(object)	G_STMT_START { \
    if (GTK_IS_OBJECT (object)) \
      g_signal_emit_by_name (object, "notify::generic-change", NULL); \
} G_STMT_END


/* --- i18n and gettext helpers --- */
#define BEAST_GETTEXT_DOMAIN (NULL)
#define _(str)	dgettext (BEAST_GETTEXT_DOMAIN, str)
#define N_(str)	(str)


/* --- internal stuff --- */
void    beast_show_about_box (void);
extern gboolean bst_developer_hints;
extern gboolean bst_debug_extensions;
extern gboolean bst_main_loop_running;


G_END_DECLS

#endif /* __BST_DEFS_H__ */
