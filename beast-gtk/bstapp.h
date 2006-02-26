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
#ifndef __BST_APP_H__
#define __BST_APP_H__

#include "bstutils.h"

G_BEGIN_DECLS

/* --- type macros --- */
#define BST_TYPE_APP            (bst_app_get_type ())
#define BST_APP(object)         (GTK_CHECK_CAST ((object), BST_TYPE_APP, BstApp))
#define BST_APP_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_APP, BstAppClass))
#define BST_IS_APP(object)      (GTK_CHECK_TYPE ((object), BST_TYPE_APP))
#define BST_IS_APP_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_APP))
#define BST_APP_GET_CLASS(app)  (G_TYPE_INSTANCE_GET_CLASS ((app), BST_TYPE_APP, BstAppClass))


/* --- typedefs --- */
typedef struct  _BstApp       BstApp;
typedef struct  _BstAppClass  BstAppClass;


/* --- structures --- */
struct _BstApp
{
  GxkDialog       window;

  SfiProxy        project;

  guint           select_unseen_super : 1;

  gchar 	 *cookie;

  GxkParam       *wave_file;

  GxkRadget      *box;
  GtkNotebook    *notebook;

  GtkWidget      *rack_dialog;
  GtkWidget      *rack_editor;
  GtkWidget      *pcontrols;

  GxkAssortment  *ppages;
};
struct _BstAppClass
{
  GxkDialogClass        parent_class;
  gboolean              seen_apps;
  GSList               *apps;
};


/* --- actions --- */
enum {
  BST_ACTION_APP_NONE,
  /* project actions */
  BST_ACTION_NEW_PROJECT,
  BST_ACTION_OPEN_PROJECT,
  BST_ACTION_MERGE_PROJECT,
  BST_ACTION_IMPORT_MIDI,
  BST_ACTION_SAVE_PROJECT,
  BST_ACTION_SAVE_PROJECT_AS,
  BST_ACTION_CLOSE_PROJECT,
  BST_ACTION_EXIT,
  /* synthesizer */
  BST_ACTION_MERGE_EFFECT,
  BST_ACTION_MERGE_INSTRUMENT,
  BST_ACTION_SAVE_EFFECT,
  BST_ACTION_SAVE_INSTRUMENT,
  /* handling supers */
  BST_ACTION_NEW_SONG,
  BST_ACTION_NEW_CSYNTH,
  BST_ACTION_NEW_MIDI_SYNTH,
  BST_ACTION_REMOVE_SYNTH,
  /* playback */
  BST_ACTION_START_PLAYBACK,
  BST_ACTION_STOP_PLAYBACK,
  /* misc */
  BST_ACTION_REBUILD,
  BST_ACTION_CLEAR_UNDO,
  BST_ACTION_UNDO,
  BST_ACTION_REDO,
  /* loading demo songs */
  BST_ACTION_LOAD_DEMO_0000,
  BST_ACTION_LOAD_DEMO_ffff = BST_ACTION_LOAD_DEMO_0000 + 0xffff,
  /* loading skins */
  BST_ACTION_LOAD_SKIN_0000,
  BST_ACTION_LOAD_SKIN_ffff = BST_ACTION_LOAD_SKIN_0000 + 0xffff,
  /* last value */
  BST_ACTION_APP_LAST
};


/* --- prototypes --- */
GType           bst_app_get_type                (void);
BstApp*         bst_app_new                     (SfiProxy        project);
void            bst_app_create_default          (BstApp         *app);
BstApp*         bst_app_find                    (SfiProxy        project);
void            bst_app_show_release_notes      (BstApp         *app);

G_END_DECLS

#endif  /* __BST_APP_H__ */
