/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2003 Tim Janik
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
#ifndef __BST_DEFS_H__
#define __BST_DEFS_H__

#include <gxk/gxk.h>
#include <libintl.h>
#include "glewidgets.h"
#include "bstzoomedwindow.h"
#include "bstfreeradiobutton.h"

G_BEGIN_DECLS


/* --- BEAST mainmenu operations --- */
typedef enum
{
  BST_ACTION_NONE,
  /* app actions */
  BST_ACTION_APP_FIRST          = 0x0001 << 16,
  /* wave view actions */
  BST_ACTION_WAVE_FIRST         = 0x0002 << 16,
  /* (song) part view actions */
  BST_ACTION_PART_FIRST         = 0x0003 << 16,
  /* (song) track view actions */
  BST_ACTION_TRACK_FIRST        = 0x0004 << 16,
} BstActionRegions;

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


/* --- constants & defines --- */
#define	BST_TAG_DIAMETER	  (20)
#define BST_STRDUP_RC_FILE()	  (g_strconcat (g_get_home_dir (), "/.beastrc", NULL))

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
#define CKEY(x) x
/* allow gettext-ization */
#define _(String) gettext (String)
#ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#else
#    define N_(String) (String)
#endif


/* --- internal stuff --- */
extern gboolean bst_developer_hints;
extern gboolean bst_debug_extensions;
extern gboolean bst_main_loop_running;


G_END_DECLS

#endif /* __BST_DEFS_H__ */
