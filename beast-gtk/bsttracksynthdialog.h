/* BEAST - Bedevilled Audio System
 * Copyright (C) 2003 Tim Janik
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
#ifndef __BST_TRACK_SYNTH_DIALOG_H__
#define __BST_TRACK_SYNTH_DIALOG_H__

#include "bstutils.h"
#include "bstwaveview.h"

G_BEGIN_DECLS


/* --- Gtk+ type macros --- */
#define BST_TYPE_TRACK_SYNTH_DIALOG            (bst_track_synth_dialog_get_type ())
#define BST_TRACK_SYNTH_DIALOG(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_TRACK_SYNTH_DIALOG, BstTrackSynthDialog))
#define BST_TRACK_SYNTH_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_TRACK_SYNTH_DIALOG, BstTrackSynthDialogClass))
#define BST_IS_TRACK_SYNTH_DIALOG(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_TRACK_SYNTH_DIALOG))
#define BST_IS_TRACK_SYNTH_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_TRACK_SYNTH_DIALOG))
#define BST_TRACK_SYNTH_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BST_TYPE_TRACK_SYNTH_DIALOG, BstTrackSynthDialogClass))


/* --- structures & typedefs --- */
typedef struct _BstTrackSynthDialog      BstTrackSynthDialog;
typedef struct _BstTrackSynthDialogClass BstTrackSynthDialogClass;
typedef void (*BstTrackSynthDialogSelected)     (gpointer                data,
                                                 SfiProxy                proxy,
                                                 BstTrackSynthDialog    *tsdialog);
struct _BstTrackSynthDialog
{
  GxkDialog      parent_instance;
  GtkWidget     *notebook;
  GtkWidget     *wpage;         /* wave repo item view */
  GtkWidget     *spage;         /* synth list */
  GtkWidget     *ok;            /* ok button */
  GtkWindow     *parent_window;
  guint          ignore_activate : 1;
  GtkTreeModel  *pstore;        /* proxy store */
  GtkTreeView   *tview;         /* synth selection tree view */
  gpointer       selected_callback;
  gpointer       selected_data;
};
struct _BstTrackSynthDialogClass
{
  GxkDialogClass parent_class;
};


/* --- prototypes --- */
GType           bst_track_synth_dialog_get_type (void);
GtkWidget*      bst_track_synth_dialog_popup    (gpointer             parent_widget,
                                                 SfiProxy             track,
                                                 BseProxySeq         *pseq,
                                                 SfiProxy             wrepo,
                                                 gpointer             selected_callback,
                                                 gpointer             data);
void            bst_track_synth_dialog_set      (BstTrackSynthDialog *self,
                                                 BseProxySeq         *pseq,
                                                 SfiProxy             wrepo);


G_END_DECLS

#endif /* __BST_TRACK_SYNTH_DIALOG_H__ */
