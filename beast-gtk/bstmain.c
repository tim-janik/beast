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
#include        "bstdefs.h"

#include        "bstconfigpaths.h"
#include        "bstapp.h"
#include        "bstsamplerepo.h"
#include        "bstprocedure.h"
#include	"bstxkb.h"
#include	"bstkeytables.h"
#include	<BEASTconfig.h>




#define	PROGRAM	"BEAST"
#define	TITLE	"Beast"
#define	VERSION	"Pre-Alpha"


/* --- variables --- */
static guint args_changed_signal_id = 0;


/* --- functions --- */
int
main (int   argc,
      char *argv[])
{
  GleParserData *pdata;
  gchar *error, *resource_file = BST_PDATA_DIR "/beast.glr";
  GSList *slist;
  GSList *unref_list = NULL;
  BstApp *app = NULL;
  guint i;

  g_message ("BEAST: pid = %u", getpid ());
  
  /* initialize libraries
   */
  bse_init (&argc, &argv);
  gtk_init (&argc, &argv);
  gle_init (&argc, &argv);


  /* hackery rulez!
   */
  args_changed_signal_id =
    gtk_object_class_user_signal_new (gtk_type_class (GTK_TYPE_OBJECT),
				      "args_changed",
				      GTK_RUN_ACTION,
				      gtk_signal_default_marshaller,
				      GTK_TYPE_NONE, 0);


  /* open master output stream
   */
  error = bst_master_init ();
  if (error)
    {
      g_printerr (error);
      g_free (error);

      return -1;
    }

  /* register neccessary GLE components
   */
  bst_app_register ();

  /* register sample repositories
   */
  bst_sample_repo_init ();

  /* parse GLE (GUI) resources
   */
  pdata = gle_parser_data_from_file (resource_file);
  if (!pdata)
    {
      g_message ("Can't retrive neccessary resources from \"%s\"", resource_file);
      return 1;
    }
  for (slist = pdata->gtoplevels; slist; slist = slist->next)
    {
      gle_gobject_ref (slist->data);
      gle_gwidget_set_visible (slist->data, FALSE);
    }
  gle_parser_data_destroy (pdata);


  /* setup default keytable for pattern editor class
   */
  {
    gchar *encoding, *layout, *model, *variant, *name = NULL;
    GSList *slist, *name_list = NULL;
    BstKeyTablePatch *patch = NULL;

    if (bst_xkb_open (gdk_get_display (), TRUE))
      {
	name = g_strdup (bst_xkb_get_symbol (TRUE));
	if (!name)
	  name = g_strdup (bst_xkb_get_symbol (FALSE));
	bst_xkb_close ();
      }
    
    bst_xkb_parse_symbol (name, &encoding, &layout, &model, &variant);
    BST_DEBUG (KEYTABLE, {
      g_message ("keytable %s: encoding(%s) layout(%s) model(%s) variant(%s)",
		 name, encoding, layout, model, variant);
    });
    g_free (name);

    /* strip number of keys (if present) */
    if (layout)
      {
	gchar *n, *l = layout;

	while (*l && (*l < '0' || *l > '9'))
	  l++;
	n = l;
	while (*n >= '0' && *n <= '9')
	  n++;
	*n = 0;
	n = layout;
	layout = *l ? g_strdup (l) : NULL;
	g_free (n);
      }

    /* list guesses */
    if (encoding)
      {
	name_list = g_slist_prepend (name_list, g_strdup (encoding));
	if (layout)
	  name_list = g_slist_prepend (name_list,
				       g_strdup_printf ("%s-%s",
							encoding,
							layout));
      }
    if (model)
      {
	name_list = g_slist_prepend (name_list, g_strdup (model));
	if (layout)
	  name_list = g_slist_prepend (name_list,
				       g_strdup_printf ("%s-%s",
							model,
							layout));
      }
    g_free (encoding);
    g_free (layout);
    g_free (model);
    g_free (variant);

    for (slist = name_list; slist; slist = slist->next)
      {
	name = slist->data;

	if (!patch)
	  {
	    patch = bst_key_table_patch_find (name);
	    BST_DEBUG (KEYTABLE, {
	      g_message ("Guessing keytable, %s \"%s\"",
			 patch ? "found" : "failed to get",
			 name);
	    });
	  }
	else
	  BST_DEBUG (KEYTABLE, {
	    g_message ("Guessing keytable, discarding \"%s\"", name);
	  });
	g_free (name);
      }
    g_slist_free (name_list);

    if (!patch)
      {
	name = BST_DFL_KEYTABLE;	/* default keyboard */
	BST_DEBUG (KEYTABLE, {
	  g_message ("Guessing keytable failed, reverting to \"%s\"", name);
	});
	patch = bst_key_table_patch_find (name);
      }

    bst_key_table_install_patch (patch);
  }
    
  
  /* open files given on command line
   */
  for (i = 1; i < argc; i++)
    {
      BseStorage *storage = bse_storage_new ();
      BseErrorType error;


#if 0
      if (!strcmp (argv[i], "--+debug"))
	{
	  gle_shell_popup ();
	  gtk_main ();
	  return -1;
	}
#endif
      
      error = bse_storage_input_file (storage, argv[i]);

      if (!error)
	{
	  BseProject *project = bse_project_new (argv[i]);

	  bse_storage_set_path_resolver (storage, bse_project_path_resolver, project);
	  error = bse_project_restore (project, storage);
	  if (!error)
	    {
	      app = bst_app_new (project);
	      gtk_idle_show_widget (GTK_WIDGET (app));
	    }
	  bse_object_unref (BSE_OBJECT (project));
	}
      bse_storage_destroy (storage);
      if (error)
	g_message ("failed to load project `%s': %s", /* FIXME */
		   argv[i],
		   bse_error_blurb (error));
    }

  /* open default app window
   */
  if (!app)
    {
      BseProject *project = bse_project_new ("Untitled.bse");

      app = bst_app_new (project);
      bse_object_unref (BSE_OBJECT (project));
      bst_app_operate (app, BST_OP_SONG_NEW);
      gtk_idle_show_widget (GTK_WIDGET (app));
    }
  
  
  /* pre load plugin types
   */
  {
    GList *free_list, *list;

    free_list = bse_plugin_dir_list_files (BSE_PATH_PLUGINS);
    for (list = free_list; list; list = list->next)
      {
	gchar *string = list->data;

	g_message ("load \"%s\": %s",
		   string,
		   bse_plugin_check_load (string));
	g_free (string);
      }
    g_list_free (free_list);
  }
  
  
/* and away into the main loop
   */
  gtk_main ();


  for (slist = unref_list; slist; slist = slist->next)
    bse_object_unref (slist->data);
  g_slist_free (unref_list);


  /* shutdown master
   */
  bst_master_shutdown ();


  bse_chunk_debug ();
  
  return 0;
}

/* read bstdefs.h on this */
void
bst_update_can_operate (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  /* FIXME, we need to queue a high prioritized idle here as
   * this function can be called multiple times in a row
   */

  /* figure toplevel app, and update it
   */
  widget = gtk_widget_get_ancestor (widget, BST_TYPE_APP);
  g_return_if_fail (BST_IS_APP (widget));

  bst_app_update_can_operate (BST_APP (widget));
}

/* read bstdefs.h on this */
extern void
bst_object_set (gpointer     object,
		const gchar *first_arg_name,
		...)
{
  va_list args;

  g_return_if_fail (GTK_IS_OBJECT (object));
  
  gtk_object_ref (object);
  
  va_start (args, first_arg_name);

  if (GNOME_IS_CANVAS_ITEM (object))
    gnome_canvas_item_set_valist (object, first_arg_name, args);
  else
    {
      GSList *arg_list = NULL;
      GSList *info_list = NULL;
      gchar *error;
      
      error = gtk_object_args_collect (GTK_OBJECT_TYPE (object),
				       &arg_list,
				       &info_list,
				       first_arg_name,
				       args);
      
      if (error)
	{
	  g_warning ("bst_object_set(): %s", error);
	  g_free (error);
	}
      else if (arg_list)
	{
	  GSList *arg;
	  GSList *info;
	  
	  for (arg = arg_list, info = info_list; arg; arg = arg->next, info = info->next)
	    gtk_object_arg_set (object, arg->data, info->data);
	  
	  gtk_args_collect_cleanup (arg_list, info_list);
	}
    }
  va_end (args);

  gtk_signal_emit (object, args_changed_signal_id);
  
  gtk_object_unref (object);
}

GnomeCanvasPoints*
gnome_canvas_points_new0 (guint num_points)
{
  GnomeCanvasPoints *points;
  guint i;

  g_return_val_if_fail (num_points > 1, NULL);

  points = gnome_canvas_points_new (num_points);
  for (i = 0; i < num_points; i++)
    {
      points->coords[i] = 0;
      points->coords[i + num_points] = 0;
    }

  return points;
}

/*
  static void
  about_cb (GtkWidget *widget,
  gpointer   data)
  {
  static const gchar *authors[] = { "Olaf Hoehmann", "Tim Janik", NULL };
  static GtkWidget *about;
  
  if (!about)
  {
  about = gnome_about_new (TITLE, VERSION,
  "Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik",
  authors,
  "Beast commentary",
  NULL);
  gtk_quit_add_destroy (1, GTK_OBJECT (about));
  gtk_window_set_position (GTK_WINDOW (about), GTK_WIN_POS_CENTER);
  gnome_dialog_close_hides (GNOME_DIALOG (about), TRUE);
  gnome_dialog_append_button ( GNOME_DIALOG(about),
  GNOME_STOCK_BUTTON_OK);
  gnome_dialog_append_button ( GNOME_DIALOG(about),
  GNOME_STOCK_BUTTON_OK);
  gtk_signal_connect (GTK_OBJECT (about), "destroy", printf, "destruction");
  }
  gtk_widget_show (about);
  }
*/
