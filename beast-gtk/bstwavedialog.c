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
#include	"bstwavedialog.h"

#include	"bststatusbar.h"
#include	"bstmenus.h"
#include	<unistd.h>
#include	<errno.h>


/* --- prototypes --- */
static void	bst_wave_dialog_class_init	(BstWaveDialogClass	*class);
static void	bst_wave_dialog_init		(BstWaveDialog		*fd);
static void	bst_wave_dialog_destroy		(GtkObject		*object);


/* --- variables --- */
static gpointer parent_class = NULL;


/* --- functions --- */
GType
bst_wave_dialog_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info = {
	sizeof (BstWaveDialogClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) bst_wave_dialog_class_init,
	NULL,   /* class_finalize */
	NULL,   /* class_data */
	sizeof (BstWaveDialog),
	0,      /* n_preallocs */
	(GInstanceInitFunc) bst_wave_dialog_init,
      };

      type = g_type_register_static (GTK_TYPE_FILE_SELECTION,
				     "BstWaveDialog",
				     &type_info, 0);
    }

  return type;
}

static void
bst_wave_dialog_class_init (BstWaveDialogClass *class)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  object_class->destroy = bst_wave_dialog_destroy;
}

static void
bst_wave_dialog_init (BstWaveDialog *wd)
{
  gtk_widget_set (GTK_WIDGET (wd),
		  "title", "BEAST WaveDialog",
		  "window_position", GTK_WIN_POS_MOUSE,
		  "modal", TRUE,
		  NULL);
  g_object_connect (wd,
		    "signal::delete_event", gtk_widget_hide_on_delete, NULL,
		    NULL);
  
  gtk_file_selection_heal (GTK_FILE_SELECTION (wd));
}

static void
bst_wave_dialog_destroy (GtkObject *object)
{
  BstWaveDialog *wd = BST_WAVE_DIALOG (object);

  bst_wave_dialog_set_wave_repo (wd, 0);

  /* chain parent handler */
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bst_wave_dialog_open (BstWaveDialog *wd)
{
  BswProxy wrepo = wd->wave_repo;
  gchar *wave_name;
  BseErrorType error;

  g_return_if_fail (wrepo != 0);

  if (!GTK_WIDGET_DRAWABLE (wd))	/* we can get spurious clicks after long loads */
    return;

  wave_name = g_strdup (gtk_file_selection_get_filename (GTK_FILE_SELECTION (wd)));

  bst_status_printf (0, NULL, "Loading wave `%s'", wave_name);

  error = bsw_wave_repo_load_file (wrepo, wave_name);

  bst_status_printf (error ? 0 : 100, bse_error_blurb (error), "Loading wave `%s'", wave_name);

  g_free (wave_name);
}

GtkWidget*
bst_wave_dialog_new_load (BswProxy   wrepo,
			  GtkWidget *parent_dialog)
{
  BstWaveDialog *wd;
  GtkFileSelection *fd;

  if (parent_dialog)
    g_return_val_if_fail (GTK_IS_WIDGET (parent_dialog), NULL);

  wd = g_object_new (BST_TYPE_WAVE_DIALOG,
		     "title", "BEAST Open Wave",
		     NULL);
  fd = GTK_FILE_SELECTION (wd);
  g_object_connect (fd->ok_button,
		    "swapped_object_signal::clicked", bst_wave_dialog_open, wd,
		    NULL);
  g_object_connect (fd->cancel_button,
		    "swapped_object_signal::clicked", gtk_widget_hide, wd,
		    NULL);
  if (parent_dialog)
    {
      wd->parent_dialog = g_object_ref (parent_dialog);
      g_object_connect (wd->parent_dialog,
			"swapped_object_signal::destroy", gtk_widget_destroy, wd,
			NULL);
      if (GTK_IS_WINDOW (wd->parent_dialog))
	gtk_window_set_transient_for (GTK_WINDOW (wd), GTK_WINDOW (wd->parent_dialog));
    }
  bst_wave_dialog_set_wave_repo (wd, wrepo);

  return GTK_WIDGET (wd);
}

void
bst_wave_dialog_set_wave_repo (BstWaveDialog *wd,
			       BswProxy       wrepo)
{
  g_return_if_fail (BST_IS_WAVE_DIALOG (wd));
  if (wrepo)
    g_return_if_fail (BSW_IS_WAVE_REPO (wrepo));

  if (wd->wave_repo)
    bsw_item_unuse (wd->wave_repo);
  wd->wave_repo = wrepo;
  if (wd->wave_repo)
    bsw_item_use (wd->wave_repo);
}
