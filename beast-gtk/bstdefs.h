/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2002 Tim Janik
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
#include "glewidgets.h"
#include "bstzoomedwindow.h"
#include "bstfreeradiobutton.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- BEAST mainmenu operations --- */
typedef enum
{
  BST_OP_NONE,

  /* project operations
   */
  BST_OP_PROJECT_NEW,
  BST_OP_PROJECT_OPEN,
  BST_OP_PROJECT_MERGE,
  BST_OP_PROJECT_SAVE,
  BST_OP_PROJECT_SAVE_AS,
  BST_OP_PROJECT_NEW_SONG,
  BST_OP_PROJECT_NEW_SNET,
  BST_OP_PROJECT_NEW_MIDI_SYNTH,
  BST_OP_PROJECT_CLOSE,
  BST_OP_PROJECT_PLAY,
  BST_OP_PROJECT_STOP,
  BST_OP_PROJECT_RACK_EDITOR,

  /* spawn new dialogs
   */
  BST_OP_DIALOG_PREFERENCES,
  BST_OP_DIALOG_PROC_BROWSER,
  BST_OP_DIALOG_DEVICE_MONITOR,

  /* debugging */
  BST_OP_REBUILD,

  /* song operations
   */
  BST_OP_PART_ADD,
  BST_OP_PART_DELETE,
  BST_OP_PART_EDITOR,
  BST_OP_WAVE_LOAD,
  BST_OP_WAVE_DELETE,
  BST_OP_WAVE_EDITOR,
  BST_OP_TRACK_ADD,
  BST_OP_TRACK_DELETE,

  /* super operations
   */
  BST_OP_UNDO_LAST,
  BST_OP_REDO_LAST,

  /* application wide
   */
  BST_OP_EXIT,

  /* help dialogs
   */
#define BST_OP_HELP_FIRST	BST_OP_HELP_FAQ
  BST_OP_HELP_FAQ,
  BST_OP_HELP_KEYTABLE,
  BST_OP_HELP_RELEASE_NOTES,
  BST_OP_HELP_GSL_PLAN,
  BST_OP_HELP_QUICK_START,
  BST_OP_HELP_ABOUT,
#define	BST_OP_HELP_LAST	BST_OP_HELP_ABOUT

  BST_OP_LAST
} BstOps;

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


/* --- debug stuff --- */
typedef enum                    /*< skip >*/
{ /* keep in sync with bstmain.c */
  BST_DEBUG_KEYTABLE		= (1 << 0),
  BST_DEBUG_SAMPLES		= (1 << 1)
} BstDebugFlags;
extern BstDebugFlags bst_debug_flags;
#ifdef G_ENABLE_DEBUG
#  define BST_IF_DEBUG(type)	if (!(bst_debug_flags & BST_DEBUG_ ## type)) { } else
#else  /* !G_ENABLE_DEBUG */
#  define BST_IF_DEBUG(type)	while (0) /* don't exec */
#endif /* !G_ENABLE_DEBUG */
#define	BST_DVL_EXT		(0) // FIXME: BSE_DVL_EXT
#define	BST_DVL_HINTS		(bst_dvl_hints)
extern gboolean bst_dvl_hints;

extern gboolean beast_main_loop;

extern void bst_update_can_operate (GtkWidget   *some_widget);

#define	GNOME_CANVAS_NOTIFY(object)	G_STMT_START { \
    if (GTK_IS_OBJECT (object)) \
      g_signal_emit_by_name (object, "notify::generic-change", NULL); \
} G_STMT_END

#define	BST_TOOLTIPS			(GXK_TOOLTIPS)




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_DEFS_H__ */
