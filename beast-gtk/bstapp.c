/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999, 2000 Olaf Hoehmann and Tim Janik
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
#include	"bstapp.h"

#include	"../PKG_config.h"

#include	"bstsongshell.h"
#include	"bstwavereposhell.h"
#include	"bstsnetshell.h"
#include	"bstfiledialog.h"
#include	"bststatusbar.h"
#include	"bstgconfig.h"
#include	"bstpreferences.h"
#include	"bstprocbrowser.h"
#include	"bstservermonitor.h"
#include	"bstrackeditor.h"
#include	"bstmenus.h"
#include	"bstprocedure.h"



/* --- prototypes --- */
static void	bst_app_class_init		(BstAppClass	 *class);
static void	bst_app_init			(BstApp		 *app);
static void	bst_app_destroy			(GtkObject	 *object);
static gboolean bst_app_handle_delete_event	(GtkWidget	 *widget,
						 GdkEventAny	 *event);
static void     bst_app_run_script_proc		(GtkWidget	 *widget,
						 gulong           callback_action,
						 gpointer         popup_data);


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
  { "/File/Dialogs/Procedure _Browser...", NULL,	BST_OP (DIALOG_PROC_BROWSER),	"<Item>" },
  { "/File/Dialogs/_Preferences...", 	NULL,		BST_OP (DIALOG_PREFERENCES),	"<Item>" },
  { "/File/Dialogs/Device _Monitor...",	NULL,		BST_OP (DIALOG_DEVICE_MONITOR),	"<Item>" },
  { "/File/-----",			NULL,		NULL, 0,			"<Separator>" },
  { "/File/_Close",			"<ctrl>W",	BST_OP (PROJECT_CLOSE),		"<Item>" },
  { "/File/_Exit",			"<ctrl>Q",	BST_OP (EXIT),			"<Item>" },
  { "/_Project",			NULL,		NULL, 0,			"<Branch>" },
  { "/Project/<<<<<<",			NULL,		NULL, 0,			"<Tearoff>" },
  { "/Project/_Play",			"<ctrl>P",	BST_OP (PROJECT_PLAY),		"<Item>" },
  { "/Project/_Stop",			"<ctrl>S",	BST_OP (PROJECT_STOP),		"<Item>" },
  { "/Project/-----",			NULL,		NULL, 0,			"<Separator>" },
  { "/Project/New Song",		NULL,		BST_OP (PROJECT_NEW_SONG),	"<Item>" },
  { "/Project/New Synthesizer Network",	NULL,		BST_OP (PROJECT_NEW_SNET),	"<Item>" },
  { "/Project/New MIDI Synthesizer",	NULL,		BST_OP (PROJECT_NEW_MIDI_SYNTH),"<Item>" },
  { "/Project/-----",			NULL,		NULL, 0,			"<Separator>" },
  { "/Project/Rack Editor",		NULL,		BST_OP (PROJECT_RACK_EDITOR),	"<Item>" },
  { "/Project/-----",			NULL,		NULL, 0,			"<Separator>" },
  { "/Project/Rebuild",			NULL,		BST_OP (REBUILD),		"<Item>" },
  { "/Project/Refresh",			NULL,		BST_OP (REFRESH),		"<Item>" },
  { "/_Edit",				NULL,		NULL, 0,			"<Branch>" },
  { "/Edit/<<<<<<",			NULL,		NULL, 0,			"<Tearoff>" },
  { "/Edit/_Undo",			"<ctrl>Z",	BST_OP (UNDO_LAST),		"<Item>" },
  { "/Edit/_Redo",			"<ctrl>R",	BST_OP (REDO_LAST),		"<Item>" },
  { "/S_ong",				NULL,		NULL, 0,			"<Branch>" },
  { "/Song/<<<<<<",			NULL,		NULL, 0,			"<Tearoff>" },
  { "/Song/Add _Part",			NULL,		BST_OP (PART_ADD),		"<Item>" },
  { "/Song/Delete _Part",		NULL,		BST_OP (PART_DELETE),		"<Item>" },
  { "/Song/Add _Track",			NULL,		BST_OP (TRACK_ADD),		"<Item>" },
  { "/Song/Delete _Track",		NULL,		BST_OP (TRACK_DELETE),		"<Item>" },
  { "/_Waves",				NULL,		NULL, 0,			"<Branch>" },
  { "/Waves/<<<<<<",			NULL,		NULL, 0,			"<Tearoff>" },
  { "/Waves/_Load Wave...",		"",		BST_OP (WAVE_LOAD),		"<Item>" },
  { "/Waves/Delete Wave",		NULL,		BST_OP (WAVE_DELETE),		"<Item>" },
  { "/Waves/_Edit Wave...",		"",		BST_OP (WAVE_EDITOR),		"<Item>" },
  { "/_Scripts",			NULL,		NULL, 0,			"<Branch>" },
  // { "/S_Net",			NULL,		NULL, 0,			"<Branch>" },
  // { "/SNet/<<<<<<",			NULL,		NULL, 0,			"<Tearoff>" },
  // { "/SNet/_Test",			"",		BST_OP (NONE),			"<Item>" },
};
static GtkItemFactoryEntry menubar_help_entries[] = {
  { "/_Help",				NULL,		NULL, 0,			"<LastBranch>" },
  { "/Help/<<<<<<",			NULL,		NULL, 0,			"<Tearoff>" },
  { "/Help/_Release Notes...",		NULL,		BST_OP (HELP_RELEASE_NOTES),	"<Item>" },
  { "/Help/_FAQ...",			NULL,		BST_OP (HELP_FAQ),		"<Item>" },
  { "/Help/Development/GSL Engine...",	NULL,		BST_OP (HELP_GSL_PLAN),		"<Item>" },
  { "/Help/-----",			NULL,		NULL, 0,			"<Separator>" },
  { "/Help/_About...",			NULL,		BST_OP (HELP_ABOUT),		"<Item>" },
#undef	BST_OP
};


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

      app_type = gtk_type_unique (BST_TYPE_DIALOG, &app_info);
    }

  return app_type;
}

static void
bst_app_class_init (BstAppClass *class)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  bst_app_class = class;
  parent_class = g_type_class_peek_parent (class);

  object_class->destroy = bst_app_destroy;

  widget_class->delete_event = bst_app_handle_delete_event;

  class->apps = NULL;
}

static void
bst_app_register (BstApp *app)
{
  if (!g_slist_find (bst_app_class->apps, app))
    bst_app_class->apps = g_slist_prepend (bst_app_class->apps, app);
  BST_APP_GET_CLASS (app)->seen_apps = TRUE;
}
static void
bst_app_unregister (BstApp *app)
{
  bst_app_class->apps = g_slist_remove (bst_app_class->apps, app);
}
static void
bst_app_init (BstApp *app)
{
  GtkWidget *widget = GTK_WIDGET (app);
  GtkWindow *window = GTK_WINDOW (app);
  GtkItemFactory *factory;
  GtkItemFactoryEntry *centries;
  BseCategory *cats;
  GSList *slist;
  guint n_cats;
  
  g_object_set (app,
		"allow_shrink", TRUE,
		"allow_grow", TRUE,
		"flags", BST_DIALOG_STATUS,
		NULL);
  bst_app_register (app);
  if (0)
    g_object_connect (widget,
		      "signal::map", bst_app_register, NULL,
		      "signal::unrealize", bst_app_unregister, NULL,
		      NULL);
  bst_app_register (app);
  app->main_vbox = g_object_connect (gtk_widget_new (GTK_TYPE_VBOX,
						     "visible", TRUE,
						     "parent", BST_DIALOG (app)->vbox,
						     NULL),
				     "swapped_signal::destroy", g_nullify_pointer, &app->main_vbox,
				     NULL);


  /* setup the menu bar
   */
  factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, bst_app_factories_path, NULL);
  gtk_window_add_accel_group (window, factory->accel_group);
  gtk_container_add_with_properties (GTK_CONTAINER (app->main_vbox),
				     factory->widget,
				     "expand", FALSE,
				     "position", 0,
				     NULL);
  gtk_widget_show (factory->widget);
  gtk_object_set_data_full (GTK_OBJECT (app),
			    bst_app_factories_path,
			    factory,
			    (GtkDestroyNotify) gtk_object_unref);

  /* setup standard entries
   */
  gtk_item_factory_create_items (factory, G_N_ELEMENTS (menubar_entries), menubar_entries, app);

  /* add scripts to the menu bar
   */
  cats = bse_categories_match_typed ("/Scripts/*", BSE_TYPE_PROCEDURE, &n_cats);
  centries = bst_menu_entries_from_cats (n_cats, cats, bst_app_run_script_proc, FALSE);
  slist = bst_menu_entries_slist (n_cats, centries);
  slist = bst_menu_entries_sort (slist);
  bst_menu_entries_create (factory, slist, GTK_WIDGET (app));
  g_slist_free (slist);
  g_free (centries);
  g_free (cats);

  /* setup help entries
   */
  gtk_item_factory_create_items (factory, G_N_ELEMENTS (menubar_help_entries), menubar_help_entries, app);

  /* setup the main notebook
   */
  app->notebook = g_object_new (GTK_TYPE_NOTEBOOK,
				"visible", TRUE,
				"parent", app->main_vbox,
				"tab_pos", GTK_POS_LEFT,
				"scrollable", TRUE,
				"can_focus", TRUE,
				NULL);
  g_object_connect (app->notebook,
		    "swapped_signal::destroy", g_nullify_pointer, &app->notebook,
		    "swapped_signal_after::switch-page", bst_update_can_operate, app,
		    "signal_after::switch-page", gtk_widget_viewable_changed, NULL,
		    NULL);
}

static void
bst_app_destroy (GtkObject *object)
{
  BstApp *app = BST_APP (object);

  if (app->rack_dialog)
    gtk_widget_destroy (app->rack_dialog);

  if (app->project)
    {
      bsw_server_halt_project (BSW_SERVER, app->project);
      g_object_disconnect (bse_object_from_id (app->project),
			   "any_signal", bst_app_reload_supers, app,
			   NULL);
      bsw_item_unuse (app->project);
      app->project = 0;
    }

  bst_app_unregister (app);

  GTK_OBJECT_CLASS (parent_class)->destroy (object);

  if (!bst_app_class->apps && bst_app_class->seen_apps)
    {
      bst_app_class->seen_apps = FALSE;
      gtk_main_quit ();
    }
}

BstApp*
bst_app_new (BswProxy project)
{
  GdkGeometry geometry;
  GtkWidget *widget;
  BstApp *app;

  g_return_val_if_fail (BSW_IS_PROJECT (project), NULL);

  widget = gtk_widget_new (BST_TYPE_APP,
			   "default_width", 640,
			   "default_height", 512,
			   NULL);
  app = BST_APP (widget);

  geometry.min_width = 320;
  geometry.min_height = 450;
  gtk_window_set_geometry_hints (GTK_WINDOW (widget), NULL, &geometry, GDK_HINT_MIN_SIZE);

  app->project = project;
  bsw_item_use (app->project);
  g_object_connect (bse_object_from_id (app->project),
		    "swapped_signal::item-added", bst_app_reload_supers, app,
		    "swapped_signal::item-removed", bst_app_reload_supers, app,
		    NULL);
  bst_dialog_sync_title_to_proxy (BST_DIALOG (app), app->project, "%s");

  bst_app_reload_supers (app);

  /* update menu entries
   */
  bst_app_update_can_operate (app);

  return app;
}

BstApp*
bst_app_find (BswProxy project)
{
  GSList *slist;
  
  g_return_val_if_fail (BSW_IS_PROJECT (project), NULL);

  for (slist = bst_app_class->apps; slist; slist = slist->next)
    {
      BstApp *app = slist->data;

      if (app->project == project)
	return app;
    }
  return NULL;
}

GtkWidget*
bst_app_get_current_shell (BstApp *app)
{
  g_return_val_if_fail (BST_IS_APP (app), NULL);

  if (app->notebook && app->notebook->cur_page)
    {
      g_return_val_if_fail (BST_IS_SUPER_SHELL (gtk_notebook_current_widget (app->notebook)), NULL);

      return gtk_notebook_current_widget (app->notebook);
    }

  return NULL;
}

GtkItemFactory*
bst_app_menu_factory (BstApp *app)
{
  g_return_val_if_fail (BST_IS_APP (app), NULL);

  return gtk_object_get_data (GTK_OBJECT (app), bst_app_factories_path);
}

static GtkWidget*
bst_app_create_super_shell (BstApp  *app,
			    BswProxy super)
{
  GtkWidget *shell = NULL;

  if (BSW_IS_SONG (super))
    shell = gtk_widget_new (BST_TYPE_SONG_SHELL,
			    "visible", TRUE,
			    NULL);
  else if (BSW_IS_SNET (super))
    shell = gtk_widget_new (BST_TYPE_SNET_SHELL,
			    "visible", TRUE,
			    NULL);
  else if (BSW_IS_WAVE_REPO (super))
    shell = gtk_widget_new (BST_TYPE_WAVE_REPO_SHELL,
			    "visible", TRUE,
			    NULL);
  else
    g_warning ("unknown super type `%s'", bsw_item_get_type_name (super));

  if (shell)
    {
      bst_super_shell_set_super (BST_SUPER_SHELL (shell), super);
      
      gtk_widget_ref (shell);
      gtk_object_sink (GTK_OBJECT (shell));
    }
  
  return shell;
}

void
bst_app_reload_supers (BstApp *app)
{
  GtkWidget *old_page, *old_focus;
  GSList *page_list = NULL;
  GSList *slist;

  g_return_if_fail (BST_IS_APP (app));

  old_focus = GTK_WINDOW (app)->focus_widget;
  if (old_focus)
    gtk_widget_ref (old_focus);
  old_page = app->notebook->cur_page ? gtk_notebook_current_widget (app->notebook) : NULL;
  while (gtk_notebook_current_widget (app->notebook))
    {
      g_object_ref (gtk_notebook_current_widget (app->notebook));
      page_list = g_slist_prepend (page_list, gtk_notebook_current_widget (app->notebook));
      gtk_container_remove (GTK_CONTAINER (app->notebook), page_list->data);
    }

  for (slist = BSE_PROJECT (bse_object_from_id (app->project))->supers; slist; slist = slist->next)
    {
      GtkWidget *label, *page = NULL;
      GSList *node;

      for (node = page_list; node; node = node->next)
	if (BST_SUPER_SHELL (node->data)->super == BSE_OBJECT_ID (slist->data))
	  {
	    page = node->data;
	    page_list = g_slist_remove (page_list, page);
	    break;
	  }
      if (!page)
	page = bst_app_create_super_shell (app, BSE_OBJECT_ID (slist->data));
      if (page)
	{
	  label = gtk_widget_new (GTK_TYPE_LABEL,
				  "visible", TRUE,
				  "width_request", BST_TAB_WIDTH ? BST_TAB_WIDTH : -1,
				  NULL);
	  gtk_notebook_append_page (app->notebook,
				    page,
				    label);
	  gtk_notebook_set_tab_label_packing (app->notebook, page, FALSE, TRUE, GTK_PACK_START);
	  bst_super_shell_update_parent (BST_SUPER_SHELL (page));
	  gtk_widget_unref (page);
	}
    }
  if (old_page && old_page->parent == GTK_WIDGET (app->notebook))
    gtk_notebook_set_current_page (app->notebook,
				   gtk_notebook_page_num (app->notebook,
							  old_page));
  if (old_focus)
    {
      if (old_page && gtk_widget_is_ancestor (old_focus, old_page) &&
	  gtk_widget_get_toplevel (old_focus) == GTK_WIDGET (app))
	gtk_widget_grab_focus (old_focus);
      gtk_widget_unref (old_focus);
    }
  for (slist = page_list; slist; slist = slist->next)
    {
      gtk_widget_destroy (slist->data);
      gtk_widget_unref (slist->data);
    }
  g_slist_free (page_list);
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
  GtkWidget *widget, *shell;
  guint i;

  g_return_if_fail (BST_IS_APP (app));

  /* check if the app (its widget tree) was already destroyed */
  if (!GTK_BIN (app)->child)
    return;

  shell = bst_app_get_current_shell (app);
  widget = gtk_item_factory_get_item (bst_app_menu_factory (app), "/Song");
  gtk_widget_set_sensitive (widget, BST_IS_SONG_SHELL (shell));
  widget = gtk_item_factory_get_item (bst_app_menu_factory (app), "/Waves");
  gtk_widget_set_sensitive (widget, BST_IS_WAVE_REPO_SHELL (shell));

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

static void
rebuild_super_shell (BstSuperShell *super_shell)
{
  BswProxy proxy;

  g_return_if_fail (BST_IS_SUPER_SHELL (super_shell));

  proxy = super_shell->super;
  bsw_item_use (proxy);
  bst_super_shell_set_super (super_shell, 0);
  bst_super_shell_set_super (super_shell, proxy);
  bsw_item_unuse (proxy);
}

static void
bst_app_run_script_proc (GtkWidget *widget,
			 gulong     callback_action,
			 gpointer   popup_data)
{
  BstApp *self = BST_APP (widget);
  GType proc_type = callback_action;

  bst_procedure_exec_auto (proc_type,
			   "project", BSE_TYPE_PROJECT, bse_object_from_id (self->project),
			   NULL);
}

void
bst_app_operate (BstApp *app,
		 BstOps	 op)
{
  static GtkWidget *bst_help_dialogs[BST_OP_HELP_LAST - BST_OP_HELP_FIRST + 1] = { NULL, };
  static GtkWidget *bst_dialog_open = NULL;
  static GtkWidget *bst_dialog_save = NULL;
  static GtkWidget *bst_preferences = NULL;
  static GtkWidget *bst_proc_browser = NULL;
  GtkWidget *widget, *shell;
  gchar *help_file = NULL, *help_title = NULL;
  GString *help_string = NULL;

  g_return_if_fail (BST_IS_APP (app));
  g_return_if_fail (bst_app_can_operate (app, op));

  widget = GTK_WIDGET (app);
  shell = bst_app_get_current_shell (app);

  gtk_widget_ref (widget);

  bst_status_window_push (widget);

  switch (op)
    {
      BstSuperShell *super_shell;
      BswProxy proxy;
      GtkWidget *any;

    case BST_OP_PROJECT_NEW:
      if (1)
	{
	  BswProxy project = bsw_server_use_new_project (BSW_SERVER, "Untitled.bse");
	  BstApp *new_app;

	  bsw_project_ensure_wave_repo (project);
	  new_app = bst_app_new (project);
	  bsw_item_unuse (project);

	  gtk_idle_show_widget (GTK_WIDGET (new_app));
	}
      break;
    case BST_OP_PROJECT_OPEN:
      if (!bst_dialog_open)
	{
	  bst_dialog_open = bst_file_dialog_new_open (app);
	  g_object_connect (bst_dialog_open,
			    "signal::destroy", gtk_widget_destroyed, &bst_dialog_open,
			    NULL);
	}
      gtk_widget_showraise (bst_dialog_open);
      break;
    case BST_OP_PROJECT_SAVE_AS:
      if (bst_dialog_save)
	gtk_widget_destroy (bst_dialog_save);
      bst_dialog_save = bst_file_dialog_new_save (app);
      g_object_connect (bst_dialog_save,
			"signal::destroy", gtk_widget_destroyed, &bst_dialog_save,
			NULL);
      gtk_widget_showraise (bst_dialog_save);
      break;
    case BST_OP_PROJECT_CLOSE:
      gtk_toplevel_delete (widget);
      break;
    case BST_OP_EXIT:
      if (bst_app_class)
	{
	  GSList *slist, *free_slist = g_slist_copy (bst_app_class->apps);

	  for (slist = free_slist; slist; slist = slist->next)
	    gtk_toplevel_delete (slist->data);
	  g_slist_free (free_slist);
	}
      break;
    case BST_OP_PROJECT_NEW_SONG:
      proxy = bsw_project_create_song (app->project, NULL);
      super_shell = bst_super_shell_from_super (proxy);
      break;
    case BST_OP_PROJECT_NEW_SNET:
      proxy = bsw_project_create_snet (app->project, NULL);
      super_shell = bst_super_shell_from_super (proxy);
      break;
    case BST_OP_PROJECT_NEW_MIDI_SYNTH:
      proxy = bsw_project_create_midi_synth (app->project, NULL);
      super_shell = bst_super_shell_from_super (proxy);
      break;
    case BST_OP_PROJECT_PLAY:
      {
	gchar *starting;
	BswErrorType error;

	if (bsw_project_is_playing (app->project))
	  starting = "Restarting Playback";
	else
	  starting = "Starting Playback";

	error = bsw_server_run_project (BSW_SERVER, app->project);
	bst_status_eprintf (error, starting);
      }
      break;
    case BST_OP_PROJECT_STOP:
      bsw_server_halt_project (BSW_SERVER, app->project);
      bst_status_set (BST_STATUS_DONE, "Stopping Playback", NULL);
      break;
    case BST_OP_PROJECT_RACK_EDITOR:
      if (!app->rack_dialog)
	{
	  BstRackEditor *ed = g_object_new (BST_TYPE_RACK_EDITOR,
					    "visible", TRUE,
					    NULL);

	  app->rack_editor = g_object_connect (ed, "swapped_signal::destroy", g_nullify_pointer, &app->rack_editor, NULL);
	  bst_rack_editor_set_rack_view (ed, bsw_project_get_data_pocket (app->project, "BEAST-Rack-View"));
	  app->rack_dialog = bst_dialog_new (&app->rack_dialog,
					     GTK_OBJECT (app),
					     0, // FIXME: undo Edit when hide && BST_DIALOG_HIDE_ON_DELETE
					     "Rack editor",
					     app->rack_editor);
	}
      gtk_widget_showraise (app->rack_dialog);
      break;
    case BST_OP_DIALOG_PREFERENCES:
      if (!bst_preferences)
	{
	  BseGConfig *gconf = bse_object_new (BST_TYPE_GCONFIG, NULL);
	  GtkWidget *widget;

	  bse_gconfig_revert (gconf);
	  widget = bst_preferences_new (gconf);
	  bse_object_unref (BSE_OBJECT (gconf));
	  gtk_widget_show (widget);

	  bst_preferences = bst_dialog_new (&bst_preferences,
					    NULL,
					    BST_DIALOG_HIDE_ON_DELETE,
					    "Preferences",
					    widget);
	  bst_preferences_create_buttons (BST_PREFERENCES (widget), BST_DIALOG (bst_preferences));
	}
      if (!GTK_WIDGET_VISIBLE (bst_preferences))
	bst_preferences_revert (BST_PREFERENCES (bst_dialog_get_child (BST_DIALOG (bst_preferences))));
      gtk_widget_showraise (bst_preferences);
      break;
    case BST_OP_DIALOG_PROC_BROWSER:
      if (!bst_proc_browser)
	{
	  GtkWidget *widget;

	  widget = bst_proc_browser_new ();
	  gtk_widget_show (widget);
	  bst_proc_browser = bst_dialog_new (&bst_proc_browser,
					     NULL,
					     BST_DIALOG_HIDE_ON_DELETE,
					     "Procedure Browser",
					     widget);
	  bst_proc_browser_create_buttons (BST_PROC_BROWSER (widget), BST_DIALOG (bst_proc_browser));
	}
      gtk_widget_showraise (bst_proc_browser);
      break;
    case BST_OP_DIALOG_DEVICE_MONITOR:
      any = g_object_new (BST_TYPE_SERVER_MONITOR, NULL);
      gtk_widget_show (any);
      any = bst_dialog_new (NULL,
			    GTK_OBJECT (app),
			    BST_DIALOG_DELETE_BUTTON, // FIXME: BST_DIALOG_HIDE_ON_DELETE && save dialog pointer
			    "Device Monitor",
			    any);
      gtk_widget_show (any);
      break;
    case BST_OP_REFRESH:
      gtk_container_foreach (GTK_CONTAINER (app->notebook),
			     (GtkCallback) bst_super_shell_update,
			     NULL);
      gtk_widget_queue_draw (GTK_WIDGET (app->notebook));
#if 0
      //gsl_alloc_report ();
      {
	GSList *slist, *olist = g_object_debug_list();
	guint i, n_buckets = 257;
	guint buckets[n_buckets];
	guint max=0,min=0xffffffff,empty=0,avg=0;
	memset(buckets,0,sizeof(buckets[0])*n_buckets);
	for (slist = olist; slist; slist = slist->next)
	  {
	    guint hash, h = (guint) slist->data;
	    hash = (h & 0xffff) ^ (h >> 16);
	    hash = (hash & 0xff) ^ (hash >> 8);
	    hash = h % n_buckets;
	    buckets[hash]++;
	  }
	for (i = 0; i < n_buckets; i++)
	  {
	    g_printerr ("bucket[%u] = %u\n", i, buckets[i]);
	    max = MAX (max, buckets[i]);
	    min = MIN (min, buckets[i]);
	    avg += buckets[i];
	    if (!buckets[i])
	      empty++;
	  }
	g_printerr ("n_objects: %u, minbucket=%u, maxbucket=%u, empty=%u, avg=%u\n",
		    avg, min, max, empty, avg / n_buckets);
	g_slist_free (olist);
      }
#endif
      break;
    case BST_OP_REBUILD:
      gtk_container_foreach (GTK_CONTAINER (app->notebook),
			     (GtkCallback) rebuild_super_shell,
			     NULL);
      gtk_widget_queue_draw (GTK_WIDGET (app->notebook));
      break;
    case BST_OP_HELP_FAQ:
      help_file = g_strconcat (BST_PATH_DOCS, "/faq.markup", NULL);
      help_title = help_file;
      goto case_help_dialog;
    case BST_OP_HELP_RELEASE_NOTES:
      help_file = g_strconcat (BST_PATH_DOCS, "/release-notes.markup", NULL);
      help_title = help_file;
      goto case_help_dialog;
    case BST_OP_HELP_GSL_PLAN:
      help_file = g_strconcat (BST_PATH_DOCS, "/gsl-mplan.markup", NULL);
      help_title = help_file;
      goto case_help_dialog;
    case BST_OP_HELP_ABOUT:
      break;
    case_help_dialog:
      if (!bst_help_dialogs[op - BST_OP_HELP_FIRST])
	bst_help_dialogs[op - BST_OP_HELP_FIRST] = bst_dialog_new (&bst_help_dialogs[op - BST_OP_HELP_FIRST],
								   NULL,
								   BST_DIALOG_HIDE_ON_DELETE | BST_DIALOG_DELETE_BUTTON,
								   help_title,
								   bst_text_view_from (help_string,
										       help_file,
										       "mono"));
      g_free (help_file);
      if (help_string)
	g_string_free (help_string, TRUE);
      gtk_widget_showraise (bst_help_dialogs[op - BST_OP_HELP_FIRST]);
      break;
    default:
      if (shell)
	bst_super_shell_operate (BST_SUPER_SHELL (shell), op);
      break;
    }

  bst_status_window_pop ();

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
    case BST_OP_PROJECT_NEW_MIDI_SYNTH:
    case BST_OP_PROJECT_CLOSE:
    case BST_OP_REFRESH:
    case BST_OP_REBUILD:
    case BST_OP_EXIT:
      return TRUE;
    case BST_OP_PROJECT_PLAY:
      if (app->project && BSE_PROJECT (bse_object_from_id (app->project))->supers)
	return TRUE;
      return FALSE;
    case BST_OP_PROJECT_STOP:
      if (app->project)
	{
	  GSList *slist;

	  for (slist = BSE_PROJECT (bse_object_from_id (app->project))->supers; slist; slist = slist->next)
	    if (BSE_SOURCE_PREPARED (slist->data))
	      return TRUE;
	}
      return FALSE;
    case BST_OP_PROJECT_RACK_EDITOR:
    case BST_OP_DIALOG_PREFERENCES:
    case BST_OP_DIALOG_PROC_BROWSER:
    case BST_OP_DIALOG_DEVICE_MONITOR:
      return TRUE;
      // case BST_OP_HELP_ABOUT:
    case BST_OP_HELP_FAQ:
    case BST_OP_HELP_GSL_PLAN:
    case BST_OP_HELP_RELEASE_NOTES:
      return TRUE;
    default:
      return shell ? bst_super_shell_can_operate (BST_SUPER_SHELL (shell), bst_op) : FALSE;
    }
}
