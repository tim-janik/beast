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
#include	"bstfiledialog.h"

#include	"bststatusbar.h"
#include	"bstmenus.h"
#include	<unistd.h>
#include	<errno.h>


/* --- prototypes --- */
static void	bst_file_dialog_class_init	(BstFileDialogClass	*class);
static void	bst_file_dialog_init		(BstFileDialog		*fd);


/* --- variables --- */
static GtkFileSelection *parent_class = NULL;


/* --- functions --- */
GtkType
bst_file_dialog_get_type (void)
{
  static GtkType file_dialog_type = 0;

  if (!file_dialog_type)
    {
      GtkTypeInfo file_dialog_info =
      {
	"BstFileDialog",
	sizeof (BstFileDialog),
	sizeof (BstFileDialogClass),
	(GtkClassInitFunc) bst_file_dialog_class_init,
	(GtkObjectInitFunc) bst_file_dialog_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };

      file_dialog_type = gtk_type_unique (GTK_TYPE_FILE_SELECTION, &file_dialog_info);
    }

  return file_dialog_type;
}

static void
bst_file_dialog_class_init (BstFileDialogClass	*class)
{
  parent_class = gtk_type_class (gtk_file_selection_get_type ());
}

static void
bst_file_dialog_init (BstFileDialog *fd)
{
  gtk_widget_set (GTK_WIDGET (fd),
		  "title", "BEAST FileDialog",
		  "window_position", GTK_WIN_POS_MOUSE,
		  NULL);
  
  gtk_file_selection_heal (GTK_FILE_SELECTION (fd));
}

static void
bst_file_dialog_open (BstFileDialog *fd)
{
  gchar *file_name;
  BswProxy project;
  BseErrorType error;

  file_name = g_strdup (gtk_file_selection_get_filename (GTK_FILE_SELECTION (fd)));

  project = bsw_server_use_new_project (BSW_SERVER, file_name);
  error = bsw_project_restore_from_file (project, file_name);

  if (error)
    g_message ("failed to load project `%s': %s", /* FIXME */
	       file_name,
	       bse_error_blurb (error));
  else
    {
      BstApp *app;

      bsw_project_ensure_wave_repo (project);
      app = bst_app_new (project);
      bst_status_window_push (app);
      bst_status_printf (error ? 0 : 100,
			 error ? "Failed" : "Done",
			 "Loading project `%s'",
			 file_name);
      bst_status_window_pop ();
      gtk_idle_show_widget (GTK_WIDGET (app));
    }
  bsw_item_unuse (project);

  g_free (file_name);
  gtk_widget_destroy (GTK_WIDGET (fd));
  gdk_flush ();
}

static void
bst_file_dialog_save (BstFileDialog *fd)
{
  BstApp *app;
  gchar *file_name;
  GtkWidget *radio;
  BseErrorType error;
  gboolean self_contained = FALSE;

  file_name = g_strdup (gtk_file_selection_get_filename (GTK_FILE_SELECTION (fd)));
  app = gtk_object_get_data (GTK_OBJECT (fd), "app");

  radio = gtk_object_get_data (GTK_OBJECT (fd), "radio-1");
  if (radio)
    self_contained = !GTK_TOGGLE_BUTTON (radio)->active;

  bst_status_window_push (app);

 retry_saving:

  error = bsw_project_store_bse (app->project, file_name, self_contained);

  /* offer retry if file exists
   */
  if (error == BSE_ERROR_FILE_EXISTS)
    {
      GtkWidget *choice;
      gchar *title = g_strdup_printf ("Saving project `%s'", bsw_item_get_name (app->project));
      gchar *text = g_strdup_printf ("Failed to save\n`%s'\nto\n`%s':\n%s",
				     bsw_item_get_name (app->project),
				     file_name,
				     bse_error_blurb (error));
      
      choice = bst_choice_dialog_createv (BST_CHOICE_TITLE (title),
					  BST_CHOICE_TEXT (text),
					  BST_CHOICE_D (1, BST_STOCK_ACTION_OVERWRITE, NONE),
					  BST_CHOICE (0, BST_STOCK_ACTION_CANCEL, NONE),
					  BST_CHOICE_END);
      g_free (title);
      g_free (text);
      if (bst_choice_modal (choice, 0, 0) == 1)
	{
	  bst_choice_destroy (choice);
	  if (unlink (file_name) < 0)
	    bst_status_printf (0, g_strerror (errno), "Deleting `%s'", file_name);
	  else
	    goto retry_saving;
	}
      else
	bst_choice_destroy (choice);
    }
  else
    {
      bst_status_printf (error ? 0 : 100,
			 error ? bse_error_blurb (error) : "Done",
			 "Saving project `%s'",
			 file_name);
      if (!error)
	gtk_widget_destroy (GTK_WIDGET (fd));
    }

  bst_status_window_pop ();
  g_free (file_name);
}

GtkWidget*
bst_file_dialog_new_open (BstApp *app)
{
  GtkWidget *dialog;

  dialog = gtk_widget_new (BST_TYPE_FILE_DIALOG,
			   "title", "Open Project: BEAST",
			   NULL);
  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (dialog)->ok_button),
			     "clicked",
			     GTK_SIGNAL_FUNC (bst_file_dialog_open),
			     GTK_OBJECT (dialog));
  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (dialog)->cancel_button),
			     "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     GTK_OBJECT (dialog));
  if (app)
    {
      gtk_object_set_data (GTK_OBJECT (dialog), "app", app);
      gtk_signal_connect_object_while_alive (GTK_OBJECT (app),
					     "destroy",
					     G_CALLBACK (gtk_widget_destroy),
					     GTK_OBJECT (dialog));
    }

  return dialog;
}

GtkWidget*
bst_file_dialog_new_save (BstApp *app)
{
  GtkWidget *dialog;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *radio1;
  GtkWidget *radio2;
  gchar *string;

  g_return_val_if_fail (BST_IS_APP (app), NULL);

  string = g_strconcat ("Save Project: ", bsw_item_get_name (app->project), ": BEAST", NULL);
  dialog = gtk_widget_new (BST_TYPE_FILE_DIALOG,
			   "title", string,
			   NULL);
  g_free (string);
  
  frame = gtk_widget_new (GTK_TYPE_FRAME,
			  "label", "Contents",
			  "visible", TRUE,
			  "parent", GTK_FILE_SELECTION (dialog)->action_area,
			  NULL);
  vbox = gtk_widget_new (GTK_TYPE_VBOX,
			 "visible", TRUE,
			 "parent", frame,
			 NULL);
  radio1 = gtk_widget_new (GTK_TYPE_RADIO_BUTTON,
			   "label", "Store references to wave files",
			   "visible", TRUE,
			   "parent", vbox,
			   "can_focus", FALSE,
			   NULL);
  gtk_misc_set_alignment (GTK_MISC (GTK_BIN (radio1)->child), 0, .5);
  gtk_object_set_data (GTK_OBJECT (dialog), "radio-1", radio1);
  radio2 = gtk_widget_new (GTK_TYPE_RADIO_BUTTON,
			   "label", "Include wave files",
			   "visible", TRUE,
			   "parent", vbox,
			   "group", radio1,
			   "can_focus", FALSE,
			   NULL);
  gtk_misc_set_alignment (GTK_MISC (GTK_BIN (radio2)->child), 0, .5);
  gtk_object_set_data (GTK_OBJECT (dialog), "radio-2", radio2);
  
  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (dialog)->ok_button),
			     "clicked",
			     GTK_SIGNAL_FUNC (bst_file_dialog_save),
			     GTK_OBJECT (dialog));
  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (dialog)->cancel_button),
			     "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     GTK_OBJECT (dialog));
  gtk_object_set_data (GTK_OBJECT (dialog), "app", app);
  gtk_signal_connect_object_while_alive (GTK_OBJECT (app),
					 "destroy",
					 G_CALLBACK (gtk_widget_destroy),
					 GTK_OBJECT (dialog));

  return dialog;
}
