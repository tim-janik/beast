/* BST - The GTK+ Layout Engine
 * Copyright (C) 1998, 1999 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include	"bstfiledialog.h"

#include	"bststatusbar.h"
#include	<stdio.h>
#include	<fcntl.h>
#include	<errno.h>


/* --- prototypes --- */
static void	bst_file_dialog_class_init	(BstFileDialogClass	*class);
static void	bst_file_dialog_init		(BstFileDialog		*fd);
static void	bst_file_dialog_destroy		(GtkObject		*object);


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
		  "GtkWindow::title", "BEAST FileDialog",
		  NULL);
  
  gtk_file_selection_heal (GTK_FILE_SELECTION (fd));
}

static void
bst_file_dialog_destroy (GtkObject *object)
{
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bst_file_dialog_open (BstFileDialog *fd)
{
  gchar *file_name;
  BseStorage *storage;
  BseErrorType error;
  BstApp *app = NULL;

  file_name = g_strdup (gtk_file_selection_get_filename (GTK_FILE_SELECTION (fd)));

  storage = bse_storage_new ();
  error = bse_storage_input_file (storage, file_name);

  if (error)
    {
      g_message ("failed to open `%s': %s", /* FIXME */
		 file_name,
		 bse_error_blurb (error));
      bst_status_printf (0, bse_error_blurb (error), "Failed to open `%s'", file_name);
    }
  else
    {
      BseProject *project = bse_project_new (file_name);

      bse_storage_set_path_resolver (storage, bse_project_path_resolver, project);
      
      error = bse_project_restore (project, storage);
      if (error)
	g_message ("failed to load project `%s': %s", /* FIXME */
		   file_name,
		   bse_error_blurb (error));
      app = bst_app_new (project);
      bst_status_window_push (app);
      bse_object_unref (BSE_OBJECT (project));
      bst_status_printf (error ? 0 : 100,
			 error ? "Failed" : "Done",
			 "Loading project `%s'",
			 file_name);
      bst_status_window_pop ();
      gtk_idle_show_widget (GTK_WIDGET (app));
    }
  
  bse_storage_destroy (storage);

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
  
  file_name = g_strdup (gtk_file_selection_get_filename (GTK_FILE_SELECTION (fd)));
  app = gtk_object_get_data (GTK_OBJECT (fd), "app");

  radio = gtk_object_get_data (GTK_OBJECT (fd), "radio-1");
  if (radio && GTK_TOGGLE_BUTTON (radio)->active)
    ;

  bst_status_window_push (app);
  error = bse_project_store_bse (app->project, file_name);
  
  if (error)
    {
      g_message ("failed to save `%s' to `%s': %s", /* FIXME */
		 BSE_OBJECT_NAME (app->project),
		 file_name,
		 bse_error_blurb (error));
      bst_status_printf (error ? 0 : 100,
			 error ? "Failed" : "Done",
			 "Saving project `%s'",
			 file_name);
      bst_status_window_pop ();
      g_free (file_name);

      return;
    }

  bst_status_window_pop ();
  g_free (file_name);
  gtk_widget_destroy (GTK_WIDGET (fd));
}

GtkWidget*
bst_file_dialog_new_open (BstApp *app)
{
  GtkWidget *dialog;

  dialog = gtk_widget_new (BST_TYPE_FILE_DIALOG,
			   "title", "BEAST Open Project",
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
					     gtk_widget_destroy,
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

  string = g_strconcat ("BEAST Save Project: ", BSE_OBJECT_NAME (app->project), NULL);
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
			   "label", "Checkbox1",
			   "visible", TRUE,
			   "parent", vbox,
			   "can_focus", FALSE,
			   NULL);
  gtk_misc_set_alignment (GTK_MISC (GTK_BIN (radio1)->child), 0, .5);
  gtk_object_set_data (GTK_OBJECT (dialog), "radio-1", radio1);
  radio2 = gtk_widget_new (GTK_TYPE_RADIO_BUTTON,
			   "label", "Checkbox2",
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
					 gtk_widget_destroy,
					 GTK_OBJECT (dialog));

  return dialog;
}

GtkWidget*
bst_text_view_from (GString     *gstring,
		    const gchar *file_name,
		    const gchar *font_name,
		    const gchar *font_fallback) /* FIXME: should go into misc.c or utils.c */
{
  GtkWidget *hbox, *text, *sb;
  GdkFont *font;
  
  hbox = gtk_widget_new (GTK_TYPE_HBOX,
			 "visible", TRUE,
			 "homogeneous", FALSE,
			 "spacing", 0,
			 "border_width", 5,
			 NULL);
  sb = gtk_vscrollbar_new (NULL);
  gtk_widget_show (sb);
  gtk_box_pack_end (GTK_BOX (hbox), sb, FALSE, TRUE, 0);
  text = gtk_widget_new (GTK_TYPE_TEXT,
			 "visible", TRUE,
			 "vadjustment", GTK_RANGE (sb)->adjustment,
			 "editable", FALSE,
			 "word_wrap", TRUE,
			 "line_wrap", FALSE,
			 "width", 500,
			 "height", 500,
			 "parent", hbox,
			 NULL);

  font = font_name ? gdk_font_load (font_name) : NULL;
  if (!font && font_fallback)
    font = gdk_font_load (font_fallback);
  if (font)
    {
      GtkRcStyle *rc_style = gtk_rc_style_new();

      gdk_font_unref (font);
      g_free (rc_style->font_name);
      rc_style->font_name = g_strdup (font_name);

      gtk_widget_modify_style (text, rc_style);
    }

  if (gstring)
    gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL, gstring->str, gstring->len);
  
  if (file_name)
    {
      gint fd;
      
      fd = open (file_name, O_RDONLY, 0);
      if (fd >= 0)
	{
	  gchar buffer[512];
	  guint n;
	  
	  do
	    {
	      do
		n = read (fd, buffer, 512);
	      while (n < 0 && errno == EINTR); /* don't mind signals */
	      
	      gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL, buffer, n);
	    }
	  while (n > 0);
	  close (fd);
	  
	  if (n < 0)
	    fd = -1;
	}
      if (fd < 0)
	{
	  gchar *error;
	  
	  error = g_strconcat ("Failed to load \"", file_name, "\":\n", g_strerror (errno), NULL);
	  gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL, error, strlen (error));
	  g_free (error);
	}
    }
      
  return hbox;
}
