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

  GtkWidget      *main_vbox;
  GtkNotebook    *notebook;

  GtkWidget      *rack_dialog;
  GtkWidget      *rack_editor;
  GtkWidget      *pcontrols;
};
struct _BstAppClass
{
  GxkDialogClass        parent_class;
  gboolean              seen_apps;
  GSList               *apps;
};


/* --- actions --- */
enum {
  /* dialogs */
  BST_ACTION_SHOW_PREFERENCES   = BST_ACTION_APP_FIRST,
  BST_ACTION_SHOW_PROC_BROWSER,
  BST_ACTION_SHOW_DEVICE_MONITOR,
  /* help dialogs */
#define BST_ACTION_HELP_FIRST   BST_ACTION_HELP_FAQ
  BST_ACTION_HELP_FAQ,
  BST_ACTION_HELP_KEYTABLE,
  BST_ACTION_HELP_RELEASE_NOTES,
  BST_ACTION_HELP_GSL_PLAN,
  BST_ACTION_HELP_QUICK_START,
  BST_ACTION_HELP_ABOUT,
#define BST_ACTION_HELP_LAST    BST_ACTION_HELP_ABOUT
  /* project dialogs */
  BST_ACTION_RACK_EDITOR,
  /* project actions */
  BST_ACTION_NEW_PROJECT,
  BST_ACTION_OPEN_PROJECT,
  BST_ACTION_MERGE_PROJECT,
  BST_ACTION_SAVE_PROJECT,
  BST_ACTION_SAVE_PROJECT_AS,
  BST_ACTION_CLOSE_PROJECT,
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
  /* and shutdown */
  BST_ACTION_EXIT,
  BST_ACTION_APP_LAST
};


/* --- prototypes --- */
GType           bst_app_get_type                (void);
BstApp*         bst_app_new                     (SfiProxy        project);
void            bst_app_reload_supers           (BstApp         *app);
void            bst_app_create_default          (BstApp         *app);
GtkWidget*      bst_app_get_current_shell       (BstApp         *app);
SfiProxy        bst_app_get_current_super       (BstApp         *app);
GtkItemFactory* bst_app_menu_factory            (BstApp         *app);
BstApp*         bst_app_find                    (SfiProxy        project);

G_END_DECLS

#endif  /* __BST_APP_H__ */
