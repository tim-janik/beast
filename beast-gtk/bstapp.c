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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include	"bstapp.h"

#include	"../PKG_config.h"

#include	"bstsongshell.h"
#include	"bstsnetshell.h"
#include	"bstfiledialog.h"
#include	"bststatusbar.h"
#include	"bstheartmonitor.h"
#include	"bstgconfig.h"
#include	"bstpreferences.h"



/* --- prototypes --- */
static void	bst_app_class_init		(BstAppClass	 *class);
static void	bst_app_init			(BstApp		 *app);
static void	bst_app_destroy			(GtkObject	 *object);
static gboolean bst_app_handle_delete_event	(GtkWidget	 *widget,
						 GdkEventAny	 *event);


/* --- menus --- */
static gchar	   *bst_app_factories_path = "<BstApp>";
static GtkItemFactoryEntry menubar_entries[] =
{
#define BST_OP(bst_op) (bst_app_operate), (BST_OP_ ## bst_op)
  { "/_File",				NULL,		NULL, 0,			"<Branch>" },
  { "/File/<<<<<<",			NULL,		NULL, 0,			"<Tearoff>" },
  { "/File/_New",			"<ctrl>N",	BST_OP (PROJECT_NEW),		"<Item>" },
  { "/File/_Open...",			"<ctrl>O",	BST_OP (PROJECT_OPEN),		"<Item>" },
  { "/File/_Save",			"<ctrl>S",	BST_OP (PROJECT_SAVE),		"<Item>" },
  { "/File/Save _As...",		NULL,		BST_OP (PROJECT_SAVE_AS),	"<Item>" },
  { "/File/-----",			NULL,		NULL, 0,			"<Separator>" },
  { "/File/_Dialogs",			NULL,		NULL, 0,			"<Branch>" },
  { "/File/Dialogs/<<<<<<",		NULL,		NULL, 0,			"<Tearoff>" },
  { "/File/Dialogs/_Preferences...", 	NULL,		BST_OP (DIALOG_PREFERENCES),	"<Item>" },
  { "/File/Dialogs/Device _Monitor...",	NULL,		BST_OP (DIALOG_DEVICE_MONITOR),	"<Item>" },
  { "/File/-----",			NULL,		NULL, 0,			"<Separator>" },
  { "/File/_Close",			"<ctrl>W",	BST_OP (PROJECT_CLOSE),		"<Item>" },
  { "/File/_Exit",			"<ctrl>Q",	BST_OP (EXIT),			"<Item>" },
  { "/_Project",			NULL,		NULL, 0,			"<Branch>" },
  { "/Project/<<<<<<",			NULL,		NULL, 0,			"<Tearoff>" },
  { "/Project/_Play",			"",		BST_OP (PROJECT_PLAY),		"<Item>" },
  { "/Project/_Stop",			"",		BST_OP (PROJECT_STOP),		"<Item>" },
  { "/Project/-----",			NULL,		NULL, 0,			"<Separator>" },
  { "/Project/New Song",		NULL,		BST_OP (PROJECT_NEW_SONG),	"<Item>" },
  { "/Project/New Source Net",		NULL,		BST_OP (PROJECT_NEW_SNET),	"<Item>" },
  { "/Project/-----",			NULL,		NULL, 0,			"<Separator>" },
  { "/Project/Rebuild",			NULL,		BST_OP (REBUILD),		"<Item>" },
  { "/Project/Refresh",			NULL,		BST_OP (REFRESH),		"<Item>" },
  { "/_Edit",				NULL,		NULL, 0,			"<Branch>" },
  { "/Edit/<<<<<<",			NULL,		NULL, 0,			"<Tearoff>" },
  { "/Edit/_Undo",			"<ctrl>U",	BST_OP (UNDO_LAST),		"<Item>" },
  { "/Edit/_Redo",			"<ctrl>R",	BST_OP (REDO_LAST),		"<Item>" },
  { "/_Song",				NULL,		NULL, 0,			"<Branch>" },
  { "/Song/<<<<<<",			NULL,		NULL, 0,			"<Tearoff>" },
  { "/Song/_Add Pattern",		"<ctrl>A",	BST_OP (PATTERN_ADD),		"<Item>" },
  { "/Song/Delete Pattern",		NULL,		BST_OP (PATTERN_DELETE),	"<Item>" },
  { "/Song/_Edit Pattern...",		"<ctrl>E",	BST_OP (PATTERN_EDITOR),	"<Item>" },
  { "/Song/Add _Instrument",		"<ctrl>A",	BST_OP (INSTRUMENT_ADD),	"<Item>" },
  // { "/S_Net",				NULL,		NULL, 0,			"<Branch>" },
  // { "/SNet/<<<<<<",			NULL,		NULL, 0,			"<Tearoff>" },
  // { "/SNet/_Test",			"",		BST_OP (NONE),			"<Item>" },
  { "/_Help",				NULL,		NULL, 0,			"<LastBranch>" },
  { "/Help/<<<<<<",			NULL,		NULL, 0,			"<Tearoff>" },
  { "/Help/_FAQ...",			NULL,		BST_OP (HELP_FAQ),		"<Item>" },
  { "/Help/_Heart...",			NULL,		BST_OP (HELP_HEART),		"<Item>" },
  { "/Help/Synthesis _Networks...",	NULL,		BST_OP (HELP_NETWORKS),		"<Item>" },
  { "/Help/-----",			NULL,		NULL, 0,			"<Separator>" },
  { "/Help/_About...",			NULL,		BST_OP (HELP_ABOUT),		"<Item>" },
#undef	BST_OP
};
static guint n_menubar_entries = sizeof (menubar_entries) / sizeof (menubar_entries[0]);


/* --- variables --- */
static BstAppClass    *bst_app_class = NULL;
static gpointer        parent_class = NULL;


/* --- functions --- */
GtkType
bst_app_get_type (void)
{
  static GtkType app_type = 0;

  if (!app_type)
    {
      GtkTypeInfo app_info =
      {
	"BstApp",
	sizeof (BstApp),
	sizeof (BstAppClass),
	(GtkClassInitFunc) bst_app_class_init,
	(GtkObjectInitFunc) bst_app_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };

      app_type = gtk_type_unique (GTK_TYPE_WINDOW, &app_info);
    }

  return app_type;
}

static void
bst_app_class_init (BstAppClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  bst_app_class = class;
  parent_class = gtk_type_class (GTK_TYPE_WINDOW);
  object_class = GTK_OBJECT_CLASS (class);
  widget_class = GTK_WIDGET_CLASS (class);

  object_class->destroy = bst_app_destroy;

  widget_class->delete_event = bst_app_handle_delete_event;

  class->apps = NULL;
}

static void
bst_app_init (BstApp *app)
{
  GtkWidget *widget = GTK_WIDGET (app);
  GtkWindow *window = GTK_WINDOW (app);
  GtkItemFactory *factory;

  bst_app_class->apps = g_slist_prepend (bst_app_class->apps, app);

  gtk_widget_set (widget,
		  "auto_shrink", FALSE,
		  "allow_shrink", TRUE,
		  "allow_grow", TRUE,
		  NULL);
  app->main_vbox = gtk_widget_new (GTK_TYPE_VBOX,
				   "visible", TRUE,
				   "parent", app,
				   "object_signal::destroy", bse_nullify_pointer, &app->main_vbox,
				   NULL);


  /* setup the menu bar
   */
  factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, bst_app_factories_path, NULL);
  gtk_window_add_accel_group (window, factory->accel_group);
  gtk_item_factory_create_items (factory, n_menubar_entries, menubar_entries, app);
  gtk_container_add_with_args (GTK_CONTAINER (app->main_vbox),
			       factory->widget,
			       "expand", FALSE,
			       "position", 0,
			       NULL);
  gtk_widget_show (factory->widget);
  gtk_object_set_data_full (GTK_OBJECT (app),
			    bst_app_factories_path,
			    factory,
			    (GtkDestroyNotify) gtk_object_unref);


  /* setup the main notebook
   */
  app->notebook =
    (GtkNotebook*) gtk_widget_new (GTK_TYPE_NOTEBOOK,
				   "visible", TRUE,
				   "parent", app->main_vbox,
				   "tab_pos", GTK_POS_LEFT,
				   "scrollable", TRUE,
				   "can_focus", TRUE,
				   "object_signal::destroy", bse_nullify_pointer, &app->notebook,
				   "object_signal_after::switch-page", bst_update_can_operate, app,
				   NULL);
  
}

static void
app_set_title (BstApp *app)
{
  GtkWindow *window = GTK_WINDOW (app);
  gchar *title;

  title = g_strconcat ("BEAST: ", BSE_OBJECT_NAME (app->project), NULL);
  gtk_window_set_title (window, title);
  g_free (title);
  gtk_window_set_wmclass (window, window->title, g_get_prgname ());
}

static void
bst_app_destroy (GtkObject *object)
{
  BstApp *app = BST_APP (object);

  if (app->project)
    {
      bse_project_stop_playback (app->project);
      bse_object_remove_notifiers_by_func (BSE_OBJECT (app->project),
					   app_set_title,
					   app);
      bse_object_unref (BSE_OBJECT (app->project));
      app->project = NULL;
    }

  bst_app_class->apps = g_slist_remove (bst_app_class->apps, app);

  GTK_OBJECT_CLASS (parent_class)->destroy (object);

  if (!bst_app_class->apps)
    gtk_main_quit ();
}

BstApp*
bst_app_new (BseProject *project)
{
  GtkWidget *widget;
  BstApp *app;

  g_return_val_if_fail (BSE_IS_PROJECT (project), NULL);

  widget = gtk_widget_new (BST_TYPE_APP, NULL);
  app = BST_APP (widget);

  bst_status_bar_ensure (GTK_WINDOW (app));

  bse_object_ref (BSE_OBJECT (project));
  app->project = project;
  bse_object_add_data_notifier (BSE_OBJECT (project),
				"name-set",
				app_set_title,
				app);
  app_set_title (app);

  bst_app_reload_supers (app);

  /* update menu entries
   */
  bst_app_update_can_operate (app);

  return app;
}

GtkWidget*
bst_app_get_current_shell (BstApp *app)
{
  g_return_val_if_fail (BST_IS_APP (app), NULL);

  if (app->notebook && app->notebook->cur_page)
    {
      g_return_val_if_fail (BST_IS_SUPER_SHELL (app->notebook->cur_page->child), NULL);

      return app->notebook->cur_page->child;
    }

  return NULL;
}

GtkItemFactory*
bst_app_menu_factory (BstApp *app)
{
  g_return_val_if_fail (BST_IS_APP (app), NULL);

  return gtk_object_get_data (GTK_OBJECT (app), bst_app_factories_path);
}

static void
bst_app_add_super (BstApp   *app,
		   BseSuper *super)
{
  GtkWidget *shell = NULL;

  if (BSE_IS_SONG (super))
    shell = gtk_widget_new (BST_TYPE_SONG_SHELL,
			    "visible", TRUE,
			    NULL);
  else if (BSE_IS_SNET (super))
    shell = gtk_widget_new (BST_TYPE_SNET_SHELL,
			    "visible", TRUE,
			    NULL);
  else
    {
      g_message ("FIXME: skipping dialog for %s", BSE_OBJECT_TYPE_NAME (super));
      return;
    }

  /* gtk_notebook_popup_enable (app->notebook); */
  gtk_notebook_append_page (app->notebook,
			    shell,
			    gtk_widget_new (GTK_TYPE_LABEL,
					    "visible", TRUE,
					    NULL));
  bst_super_shell_set_super (BST_SUPER_SHELL (shell), super);
}

void
bst_app_reload_supers (BstApp *app)
{
  GSList *slist;

  g_return_if_fail (BST_IS_APP (app));

  gtk_widget_hide (GTK_WIDGET (app->notebook));

  gtk_container_foreach (GTK_CONTAINER (app->notebook), (GtkCallback) gtk_widget_destroy, NULL);

  for (slist = app->project->supers; slist; slist = slist->next)
    bst_app_add_super (app, slist->data);

  gtk_widget_show (GTK_WIDGET (app->notebook));
}

static gboolean
bst_app_handle_delete_event (GtkWidget   *widget,
			     GdkEventAny *event)
{
  BstApp *app;

  g_return_val_if_fail (BST_IS_APP (widget), FALSE);

  app = BST_APP (widget);

  gtk_widget_destroy (widget);

  return TRUE;
}

void
bst_app_update_can_operate (BstApp *app)
{
  GtkWidget *widget;
  guint i;

  g_return_if_fail (BST_IS_APP (app));
  if (GTK_OBJECT_DESTROYED (app))
    return;

  widget = gtk_item_factory_get_item (bst_app_menu_factory (app), "/Song");
  if (widget)
    gtk_widget_set_sensitive (widget, BST_IS_SONG_SHELL (bst_app_get_current_shell (app)));
  else
    g_warning ("can't find menu item \"/Song\" for <%p>", app);

  widget = gtk_item_factory_get_item (bst_app_menu_factory (app), "/EPNet");
  if (widget)
    gtk_widget_set_sensitive (widget, FALSE);

  for (i = BST_OP_NONE; i < BST_OP_LAST; i++)
    {
      gboolean can_operate;

      widget = gtk_item_factory_get_widget_by_action (bst_app_menu_factory (app), i);
      can_operate = bst_app_can_operate (app, i);
      if (widget)
	gtk_widget_set_sensitive (widget, can_operate);
    }
}

static void
foreach_super_shell_operate (BstSuperShell *shell,
			     gpointer       data)
{
  if (bst_super_shell_can_operate (shell, GPOINTER_TO_UINT (data)))
    bst_super_shell_operate (shell, GPOINTER_TO_UINT (data));
}

void
bst_app_operate (BstApp *app,
		 BstOps	 op)
{
  static GtkWidget *bst_help_dialogs[BST_OP_HELP_LAST - BST_OP_HELP_FIRST + 1] = { NULL, };
  static GtkWidget *bst_dialog_open = NULL;
  static GtkWidget *bst_dialog_save = NULL;
  static GtkWidget *bst_preferences = NULL;
  GtkWidget *widget, *shell;
  gchar *help_file;

  g_return_if_fail (BST_IS_APP (app));
  g_return_if_fail (bst_app_can_operate (app, op));

  widget = GTK_WIDGET (app);
  shell = bst_app_get_current_shell (app);

  gtk_widget_ref (widget);

  switch (op)
    {
      BstSuperShell *super_shell;
      BseHeart *heart;
      BseSong *song;
      BseSNet *snet;

    case BST_OP_PROJECT_NEW:
      if (1)
	{
	  BseProject *project = bse_project_new ("Untitled.bse");
	  BstApp *napp = bst_app_new (project);

	  bse_object_unref (BSE_OBJECT (project));
	  if (0)
	    bst_app_operate (napp, BST_OP_PROJECT_NEW_SONG);
	  gtk_idle_show_widget (GTK_WIDGET (napp));
	}
      break;
    case BST_OP_PROJECT_OPEN:
      if (!bst_dialog_open)
	{
	  bst_dialog_open = bst_file_dialog_new_open (app);
	  gtk_signal_connect (GTK_OBJECT (bst_dialog_open),
			      "destroy",
			      gtk_widget_destroyed,
			      &bst_dialog_open);
	}
      gtk_widget_showraise (bst_dialog_open);
      break;
    case BST_OP_PROJECT_SAVE_AS:
      if (bst_dialog_save)
	gtk_widget_destroy (bst_dialog_save);
      bst_dialog_save = bst_file_dialog_new_save (app);
      gtk_signal_connect (GTK_OBJECT (bst_dialog_save),
			  "destroy",
			  gtk_widget_destroyed,
			  &bst_dialog_save);
      gtk_widget_showraise (bst_dialog_save);
      break;
    case BST_OP_PROJECT_CLOSE:
      bst_app_handle_delete_event (widget, NULL);
      break;
    case BST_OP_EXIT:
      if (bst_app_class)
	{
	  GSList *slist, *free_slist = g_slist_copy (bst_app_class->apps);

	  for (slist = free_slist; slist; slist = slist->next)
	    bst_app_operate (slist->data, BST_OP_PROJECT_CLOSE);
	  g_slist_free (free_slist);
	}
      break;
    case BST_OP_PROJECT_NEW_SONG:
      song = bse_song_new (BST_APP_PROJECT (app), BSE_DFL_SONG_N_CHANNELS);
      bst_app_reload_supers (app);
      super_shell = bst_super_shell_from_super (BSE_SUPER (song));
      bst_super_shell_operate (super_shell, BST_OP_PATTERN_ADD);
      bse_object_unref (BSE_OBJECT (song));
      break;
    case BST_OP_PROJECT_NEW_SNET:
      snet = bse_snet_new (BST_APP_PROJECT (app), NULL);
      bst_app_reload_supers (app);
      super_shell = bst_super_shell_from_super (BSE_SUPER (snet));
      bse_object_unref (BSE_OBJECT (snet));
      break;
    case BST_OP_PROJECT_PLAY:
      bse_project_start_playback (app->project);
      break;
    case BST_OP_PROJECT_STOP:
      bse_project_stop_playback (app->project);
      break;
    case BST_OP_DIALOG_PREFERENCES:
      if (!bst_preferences)
	{
	  GtkWidget *deflt;
	  BseGConfig *gconf = bse_object_new (BST_TYPE_GCONFIG, NULL);

	  bse_gconfig_revert (gconf);
	  bst_preferences = bst_preferences_new (gconf);
	  bse_object_unref (BSE_OBJECT (gconf));
	  gtk_widget_show (bst_preferences);
	  deflt = BST_PREFERENCES (bst_preferences)->close; // apply;
	  bst_preferences = bst_subwindow_new (NULL, &bst_preferences, bst_preferences, 0);
	  gtk_window_set_title (GTK_WINDOW (bst_preferences), "BEAST: Preferences");
	  gtk_widget_grab_default (deflt);
	}
      gtk_widget_showraise (bst_preferences);
      break;
    case BST_OP_DIALOG_DEVICE_MONITOR:
      heart = bse_heart_get_global (FALSE);
      if (heart)
	{
	  GtkWidget *hmon;

	  hmon = (GtkWidget*) bst_heart_monitor_from_heart (heart);
	  if (hmon)
	    hmon = gtk_widget_get_toplevel (hmon);
	  else
	    {
	      hmon = bst_heart_monitor_new (heart);
	      gtk_widget_show (hmon);
	      hmon = bst_subwindow_new (NULL, NULL, hmon, 0);
	      gtk_window_set_title (GTK_WINDOW (hmon), "BEAST: Device Monitor");
	    }
	  gtk_widget_showraise (hmon);
	}
      break;
    case BST_OP_REFRESH:
      gtk_container_foreach (GTK_CONTAINER (app->notebook),
			     (GtkCallback) bst_super_shell_update,
			     NULL);
      gtk_widget_queue_draw (GTK_WIDGET (app->notebook));
      break;
    case BST_OP_REBUILD:
      gtk_container_foreach (GTK_CONTAINER (app->notebook),
			     (GtkCallback) bst_super_shell_rebuild,
			     NULL);
      gtk_widget_queue_draw (GTK_WIDGET (app->notebook));
      break;
    case BST_OP_HELP_FAQ:
      help_file = "faq.txt";
      goto case_help_dialog;
    case BST_OP_HELP_HEART:
      help_file = "bse-heart.txt";
      goto case_help_dialog;
    case BST_OP_HELP_NETWORKS:
      help_file = "bse-networks.txt";
      goto case_help_dialog;
    case BST_OP_HELP_ABOUT:
      break;
    case_help_dialog:
      if (!bst_help_dialogs[op - BST_OP_HELP_FIRST])
	{
	  gchar *string;

	  string = g_strconcat (BST_PATH_DOCS, "/", help_file, NULL);
	  bst_help_dialogs[op - BST_OP_HELP_FIRST] = bst_subwindow_new (NULL,
									&bst_help_dialogs[op - BST_OP_HELP_FIRST],
									bst_text_view_from_file (string),
									0);
	  g_free (string);
	  string = g_strconcat ("BEAST: ", help_file, NULL);
	  gtk_window_set_title (GTK_WINDOW (bst_help_dialogs[op - BST_OP_HELP_FIRST]), string);
	  g_free (string);
	}
      gtk_widget_showraise (bst_help_dialogs[op - BST_OP_HELP_FIRST]);
      break;
    default:
      if (shell)
	bst_super_shell_operate (BST_SUPER_SHELL (shell), op);
      break;
    }

  bst_update_can_operate (widget);

  gtk_widget_unref (widget);
}

#define NONE ((void*) -1)

static void
forwhich_super_shell_can_operate (BstSuperShell *shell,
				  gpointer       data_p)
{
  gpointer *data = data_p;

  if (data[1] == NONE || data[1] == NULL)
    {
      if (bst_super_shell_can_operate (shell, GPOINTER_TO_UINT (data[0])))
	data[1] = shell;
      else
	data[1] = NULL;
    }
}

static void
forany_super_shell_can_operate (BstSuperShell *shell,
				gpointer       data_p)
{
  gpointer *data = data_p;

  if (bst_super_shell_can_operate (shell, GPOINTER_TO_UINT (data[0])))
    data[1] = shell;
}

gboolean
bst_app_can_operate (BstApp *app,
		     BstOps  bst_op)
{
  GtkWidget *shell;

  g_return_val_if_fail (BST_IS_APP (app), FALSE);
  g_return_val_if_fail (bst_op < BST_OP_LAST, FALSE);

  shell = bst_app_get_current_shell (app);

  switch (bst_op)
    {
    case BST_OP_PROJECT_NEW:
    case BST_OP_PROJECT_OPEN:
    case BST_OP_PROJECT_SAVE_AS:
    case BST_OP_PROJECT_NEW_SONG:
    case BST_OP_PROJECT_NEW_SNET:
    case BST_OP_PROJECT_CLOSE:
    case BST_OP_REFRESH:
    case BST_OP_REBUILD:
    case BST_OP_EXIT:
      return TRUE;
    case BST_OP_PROJECT_PLAY:
      if (app->project && app->project->supers)
	return TRUE;
      return FALSE;
    case BST_OP_PROJECT_STOP:
      if (app->project)
	{
	  GSList *slist;

	  for (slist = app->project->supers; slist; slist = slist->next)
	    if (BSE_SOURCE_PREPARED (slist->data))
	      return TRUE;
	}
      return FALSE;
    case BST_OP_DIALOG_PREFERENCES:
    case BST_OP_DIALOG_DEVICE_MONITOR:
      return TRUE;
      // case BST_OP_HELP_ABOUT:
    case BST_OP_HELP_FAQ:
    case BST_OP_HELP_NETWORKS:
    case BST_OP_HELP_HEART:
      return TRUE;
    default:
      return shell ? bst_super_shell_can_operate (BST_SUPER_SHELL (shell), bst_op) : FALSE;
    }
}
