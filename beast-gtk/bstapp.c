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
#include "bstapp.h"

#include "PKG_config.h"

#include "bstsongshell.h"
#include "bstwavereposhell.h"
#include "bstsnetshell.h"
#include "bstfiledialog.h"
#include "bstgconfig.h"
#include "bstpreferences.h"
#include "bstprocbrowser.h"
#include "bstservermonitor.h"
#include "bstrackeditor.h"
#include "bstmenus.h"
#include "bstprocedure.h"
#include "bstprojectctrl.h"


/* --- prototypes --- */
static void	bst_app_class_init		(BstAppClass	*class);
static void	bst_app_init			(BstApp		*app);
static void	bst_app_destroy			(GtkObject	*object);
static gboolean bst_app_handle_delete_event	(GtkWidget	*widget,
						 GdkEventAny	*event);
static void	bst_app_menu_callback		(GtkWidget	*owner,
						 gulong		 callback_action,
						 gpointer	 popup_data);
static void     bst_app_run_script_proc		(GtkWidget	*widget,
						 gulong          category_id,
						 gpointer        popup_data);


/* --- menus --- */
static gchar	   *bst_app_factories_path = "<BstApp>";
static BstMenuConfigEntry menubar_entries[] =
{
#define BST_OP(bst_op) (bst_app_menu_callback), (BST_OP_ ## bst_op)
  { "/_File",				NULL,		NULL, 0,			"<Branch>" },
  { "/File/<<<<<<",			NULL,		NULL, 0,			"<Tearoff>" },
  { "/File/_New",			"<ctrl>N",	BST_OP (PROJECT_NEW),		"<Item>" },
  { "/File/_Open...",			"<ctrl>O",	BST_OP (PROJECT_OPEN),		"<Item>" },
  { "/File/_Merge...",			"<ctrl>M",	BST_OP (PROJECT_MERGE),		"<Item>" },
  { "/File/_Save",			NULL,		BST_OP (PROJECT_SAVE),		"<Item>" },
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
static BstMenuConfigEntry menubar_help_entries[] = {
  { "/_Help",				NULL,		NULL, 0,			"<LastBranch>" },
  { "/Help/<<<<<<",			NULL,		NULL, 0,			"<Tearoff>" },
  { "/Help/_Release Notes...",		NULL,		BST_OP (HELP_RELEASE_NOTES),	"<Item>" },
  { "/Help/Quick Start...",		NULL,		BST_OP (HELP_QUICK_START),	"<Item>" },
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

      app_type = gtk_type_unique (GXK_TYPE_DIALOG, &app_info);
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
  BseCategorySeq *cseq;
  BstMenuConfig *m1, *m2;
  
  g_object_set (app,
		"allow_shrink", TRUE,
		"allow_grow", TRUE,
		"flags", GXK_DIALOG_STATUS_SHELL,
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
						     "parent", GXK_DIALOG (app)->vbox,
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
  /* standard entries */
  m1 = bst_menu_config_from_entries (G_N_ELEMENTS (menubar_entries), menubar_entries);
  /* script entries */
  cseq = bse_categories_match_typed ("/Scripts/*", "BseProcedure");
  m2 = bst_menu_config_from_cats (cseq, bst_app_run_script_proc, FALSE);
  bst_menu_config_sort (m2);
  /* merge and sort up */
  m1 = bst_menu_config_merge (m1, m2);
  /* add help entries */
  m2 = bst_menu_config_from_entries (G_N_ELEMENTS (menubar_help_entries), menubar_help_entries);
  m1 = bst_menu_config_merge (m1, m2);
  /* and create menu items */
  bst_menu_config_create_items (m1, factory, GTK_WIDGET (app));
  bst_menu_config_free (m1);

  /* setup playback controls */
  app->pcontrols = g_object_new (BST_TYPE_PROJECT_CTRL, NULL);
  gtk_container_add_with_properties (GTK_CONTAINER (app->main_vbox),
				     app->pcontrols,
				     "expand", FALSE,
				     "position", 1,
				     NULL);
  /* setup the main notebook */
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
		    "signal_after::switch-page", gxk_widget_viewable_changed, NULL,
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
      if (app->pcontrols)
	bst_project_ctrl_set_project (BST_PROJECT_CTRL (app->pcontrols), 0);
      bse_project_deactivate (app->project);
      bse_proxy_disconnect (app->project,
			   "any_signal", bst_app_reload_supers, app,
			   NULL);
      bse_item_unuse (app->project);
      app->project = 0;
    }

  bst_app_unregister (app);

  GTK_OBJECT_CLASS (parent_class)->destroy (object);

  if (!bst_app_class->apps && bst_app_class->seen_apps)
    {
      bst_app_class->seen_apps = FALSE;
      BST_MAIN_LOOP_QUIT ();
    }
}

BstApp*
bst_app_new (SfiProxy project)
{
  GdkGeometry geometry;
  GtkWidget *widget;
  BstApp *app;

  g_return_val_if_fail (BSE_IS_PROJECT (project), NULL);

  widget = gtk_widget_new (BST_TYPE_APP,
			   "default_width", 640,
			   "default_height", 512,
			   NULL);
  app = BST_APP (widget);

  geometry.min_width = 320;
  geometry.min_height = 450;
  gtk_window_set_geometry_hints (GTK_WINDOW (widget), NULL, &geometry, GDK_HINT_MIN_SIZE);

  app->project = project;
  bse_item_use (app->project);
  bse_proxy_connect (app->project,
		     "swapped_signal::item-added", bst_app_reload_supers, app,
		     "swapped_signal::item-removed", bst_app_reload_supers, app,
		     NULL);
  bst_window_sync_title_to_proxy (GXK_DIALOG (app), app->project, "%s");
  if (app->pcontrols)
    bst_project_ctrl_set_project (BST_PROJECT_CTRL (app->pcontrols), app->project);
  
  bst_app_reload_supers (app);

  /* update menu entries
   */
  bst_app_update_can_operate (app);

  return app;
}

BstApp*
bst_app_find (SfiProxy project)
{
  GSList *slist;
  
  g_return_val_if_fail (BSE_IS_PROJECT (project), NULL);

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
			    SfiProxy super)
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
  else if (BSE_IS_WAVE_REPO (super))
    shell = gtk_widget_new (BST_TYPE_WAVE_REPO_SHELL,
			    "visible", TRUE,
			    NULL);
  else
    g_warning ("unknown super type `%s'", bse_item_get_type_name (super));

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
  GtkWidget *old_page, *old_focus, *song_page = NULL, *synth_page = NULL;
  GSList *page_list = NULL;
  GSList *slist;
  BseProxySeq *pseq;
  guint i;
  
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

  pseq = bse_project_get_supers (app->project);
  for (i = 0; i < pseq->n_proxies; i++)
    {
      GtkWidget *label, *page = NULL;
      GSList *node;

      for (node = page_list; node; node = node->next)
	if (BST_SUPER_SHELL (node->data)->super == pseq->proxies[i])
	  {
	    page = node->data;
	    page_list = g_slist_remove (page_list, page);
	    break;
	  }
      if (!page)
	page = bst_app_create_super_shell (app, pseq->proxies[i]);
      if (page)
	{
	  if (!song_page && BSE_IS_SONG (pseq->proxies[i]))
	    song_page = page;
	  else if (!synth_page && BSE_IS_SNET (pseq->proxies[i]))
	    synth_page = page;
	  label = g_object_new (GTK_TYPE_LABEL,
				"visible", TRUE,
				"width_request", BST_TAB_WIDTH ? BST_TAB_WIDTH : -1,
				NULL);
	  gtk_notebook_append_page (app->notebook, page, label);
	  gtk_notebook_set_tab_label_packing (app->notebook, page, FALSE, TRUE, GTK_PACK_START);
	  bst_super_shell_update_label (BST_SUPER_SHELL (page));
	  gtk_widget_unref (page);
	}
    }
  if (old_page && old_page->parent == GTK_WIDGET (app->notebook))
    gxk_notebook_set_current_page_widget (app->notebook, old_page);
  else if (song_page)
    gxk_notebook_set_current_page_widget (app->notebook, song_page);
  else if (synth_page)
    gxk_notebook_set_current_page_widget (app->notebook, synth_page);

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
  widget = gxk_item_factory_get_item (bst_app_menu_factory (app), "/Song");
  if (widget)
    gtk_widget_set_sensitive (widget, BST_IS_SONG_SHELL (shell));
  widget = gxk_item_factory_get_item (bst_app_menu_factory (app), "/Waves");
  if (widget)
    gtk_widget_set_sensitive (widget, BST_IS_WAVE_REPO_SHELL (shell));

  widget = gxk_item_factory_get_item (bst_app_menu_factory (app), "/File/Save");	// FIXME: hack
  if (widget)
    gtk_widget_set_sensitive (widget, BST_IS_SONG_SHELL (shell));
  widget = gxk_item_factory_get_item (bst_app_menu_factory (app), "/Edit/Undo");	// FIXME: hack
  if (widget)
    gtk_widget_set_sensitive (widget, BST_IS_SONG_SHELL (shell));
  widget = gxk_item_factory_get_item (bst_app_menu_factory (app), "/Edit/Redo");	// FIXME: hack
  if (widget)
    gtk_widget_set_sensitive (widget, BST_IS_SONG_SHELL (shell));

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
rebuild_super_shell (BstSuperShell *super_shell)
{
  SfiProxy proxy;

  g_return_if_fail (BST_IS_SUPER_SHELL (super_shell));

  proxy = super_shell->super;
  bse_item_use (proxy);
  bst_super_shell_set_super (super_shell, 0);
  bst_super_shell_set_super (super_shell, proxy);
  bse_item_unuse (proxy);
}

static void
bst_app_run_script_proc (GtkWidget *widget,
			 gulong     category_id,
			 gpointer   popup_data)
{
  BstApp *self = BST_APP (widget);
  BseCategory *cat = bse_category_from_id (category_id);

  bst_procedure_exec_auto (cat->type,
			   "project", SFI_TYPE_PROXY, self->project,
			   NULL);
}

static void
bst_app_menu_callback (GtkWidget *owner,
		       gulong     callback_action,
		       gpointer   popup_data)
{
  bst_app_operate (BST_APP (owner), callback_action);
}

void
bst_app_operate (BstApp *app,
		 BstOps	 op)
{
  static GtkWidget *bst_help_dialogs[BST_OP_HELP_LAST - BST_OP_HELP_FIRST + 1] = { NULL, };
  static GtkWidget *bst_preferences = NULL;
  GtkWidget *widget, *shell;
  gchar *help_file = NULL, *help_title = NULL;

  g_return_if_fail (BST_IS_APP (app));
  g_return_if_fail (bst_app_can_operate (app, op));

  widget = GTK_WIDGET (app);
  shell = bst_app_get_current_shell (app);

  gtk_widget_ref (widget);

  gxk_status_window_push (widget);

  switch (op)
    {
      SfiProxy proxy;
      GtkWidget *any;
    case BST_OP_PROJECT_NEW:
      if (1)
	{
	  SfiProxy project = bse_server_use_new_project (BSE_SERVER, "Untitled.bse");
	  BstApp *new_app;

	  bse_project_ensure_wave_repo (project);
	  new_app = bst_app_new (project);
	  bse_item_unuse (project);

	  gxk_idle_show_widget (GTK_WIDGET (new_app));
	}
      break;
    case BST_OP_PROJECT_OPEN:
      bst_file_dialog_popup_open_project (app);
      break;
    case BST_OP_PROJECT_MERGE:
      bst_file_dialog_popup_merge_project (app, app->project);
      break;
    case BST_OP_PROJECT_SAVE:
    case BST_OP_PROJECT_SAVE_AS:
      bst_file_dialog_popup_save_project (app, app->project);
      break;
    case BST_OP_PROJECT_CLOSE:
      gxk_toplevel_delete (widget);
      break;
    case BST_OP_EXIT:
      if (bst_app_class)
	{
	  GSList *slist, *free_slist = g_slist_copy (bst_app_class->apps);

	  for (slist = free_slist; slist; slist = slist->next)
	    gxk_toplevel_delete (slist->data);
	  g_slist_free (free_slist);
	}
      break;
    case BST_OP_PROJECT_NEW_SONG:
      proxy = bse_project_create_song (app->project, NULL);
      break;
    case BST_OP_PROJECT_NEW_SNET:
      proxy = bse_project_create_snet (app->project, NULL);
      break;
    case BST_OP_PROJECT_NEW_MIDI_SYNTH:
      proxy = bse_project_create_midi_synth (app->project, NULL);
      break;
    case BST_OP_PROJECT_PLAY:
      bst_project_ctrl_play (BST_PROJECT_CTRL (app->pcontrols));
      break;
    case BST_OP_PROJECT_STOP:
      bst_project_ctrl_stop (BST_PROJECT_CTRL (app->pcontrols));
      break;
    case BST_OP_PROJECT_RACK_EDITOR:
      if (!app->rack_dialog)
	{
	  BstRackEditor *ed = g_object_new (BST_TYPE_RACK_EDITOR,
					    "visible", TRUE,
					    NULL);

	  app->rack_editor = g_object_connect (ed, "swapped_signal::destroy", g_nullify_pointer, &app->rack_editor, NULL);
	  bst_rack_editor_set_rack_view (ed, bse_project_get_data_pocket (app->project, "BEAST-Rack-View"));
	  app->rack_dialog = gxk_dialog_new (&app->rack_dialog,
					     GTK_OBJECT (app),
					     0, // FIXME: undo Edit when hide && GXK_DIALOG_HIDE_ON_DELETE
					     "Rack editor",
					     app->rack_editor);
	}
      gxk_widget_showraise (app->rack_dialog);
      break;
    case BST_OP_DIALOG_PREFERENCES:
      if (!bst_preferences)
	{
	  GtkWidget *widget = g_object_new (BST_TYPE_PREFERENCES,
					    "visible", TRUE,
					    NULL);
	  bst_preferences = gxk_dialog_new (&bst_preferences,
					    NULL,
					    GXK_DIALOG_HIDE_ON_DELETE,
					    "Preferences",
					    widget);
	  bst_preferences_create_buttons (BST_PREFERENCES (widget), GXK_DIALOG (bst_preferences));
	}
      if (!GTK_WIDGET_VISIBLE (bst_preferences))
	bst_preferences_revert (BST_PREFERENCES (gxk_dialog_get_child (GXK_DIALOG (bst_preferences))));
      gxk_widget_showraise (bst_preferences);
      break;
    case BST_OP_DIALOG_PROC_BROWSER:
#if 0 // FIXME
      if (!bst_proc_browser)
	{
	  GtkWidget *widget;

	  widget = bst_proc_browser_new ();
	  gtk_widget_show (widget);
	  bst_proc_browser = gxk_dialog_new (&bst_proc_browser,
					     NULL,
					     GXK_DIALOG_HIDE_ON_DELETE,
					     "Procedure Browser",
					     widget);
	  bst_proc_browser_create_buttons (BST_PROC_BROWSER (widget), GXK_DIALOG (bst_proc_browser));
	}
      gxk_widget_showraise (bst_proc_browser);
#endif
      break;
    case BST_OP_DIALOG_DEVICE_MONITOR:
      any = g_object_new (BST_TYPE_SERVER_MONITOR, NULL);
      gtk_widget_show (any);
      any = gxk_dialog_new (NULL,
			    GTK_OBJECT (app),
			    GXK_DIALOG_DELETE_BUTTON, // FIXME: GXK_DIALOG_HIDE_ON_DELETE && save dialog pointer
			    "Device Monitor",
			    any);
      gtk_widget_show (any);
      sfi_alloc_report ();
#if 0 // FIXME
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
      help_title = g_strdup (help_file);
      goto HELP_DIALOG;
    case BST_OP_HELP_GSL_PLAN:
      help_file = g_strconcat (BST_PATH_DOCS, "/gsl-mplan.markup", NULL);
      help_title = g_strdup (help_file);
      goto HELP_DIALOG;
    case BST_OP_HELP_QUICK_START:
      help_file = g_strconcat (BST_PATH_DOCS, "/quickstart.markup", NULL);
      help_title = g_strdup (help_file);
      goto HELP_DIALOG;
    case BST_OP_HELP_RELEASE_NOTES:
      help_file = g_strconcat (BST_PATH_DOCS, "/release-notes.markup", NULL);
      help_title = g_strdup_printf ("BEAST-%s Release Notes", BST_VERSION);
      goto HELP_DIALOG;
    HELP_DIALOG:
      if (!bst_help_dialogs[op - BST_OP_HELP_FIRST])
	{
	  GtkWidget *sctext = gxk_scroll_text_create (GXK_SCROLL_TEXT_NAVIGATABLE, NULL);
	  gchar *index = g_strconcat ("file://", BST_PATH_DOCS, "/beast-index.markup", NULL);
	  gxk_scroll_text_set_index (sctext, index);
	  g_free (index);
	  gxk_scroll_text_enter (sctext, help_file);
	  bst_help_dialogs[op - BST_OP_HELP_FIRST] = gxk_dialog_new (&bst_help_dialogs[op - BST_OP_HELP_FIRST],
								     NULL,
								     GXK_DIALOG_HIDE_ON_DELETE | GXK_DIALOG_DELETE_BUTTON,
								     help_title, sctext);
	  g_object_set (bst_help_dialogs[op - BST_OP_HELP_FIRST],
			"default_width", 560,
			"default_height", 640,
			NULL);
	}
      g_free (help_file);
      g_free (help_title);
      gxk_scroll_text_rewind (gxk_dialog_get_child (GXK_DIALOG (bst_help_dialogs[op - BST_OP_HELP_FIRST])));
      gxk_widget_showraise (bst_help_dialogs[op - BST_OP_HELP_FIRST]);
      break;
    case BST_OP_HELP_ABOUT:
      break;
    default:
      if (shell)
	bst_super_shell_operate (BST_SUPER_SHELL (shell), op);
      break;
    }

  gxk_status_window_pop ();

  bst_update_can_operate (widget);

  gtk_widget_unref (widget);
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
    case BST_OP_PROJECT_MERGE:
    case BST_OP_PROJECT_SAVE:
    case BST_OP_PROJECT_SAVE_AS:
    case BST_OP_PROJECT_NEW_SONG:
    case BST_OP_PROJECT_NEW_SNET:
    case BST_OP_PROJECT_NEW_MIDI_SYNTH:
    case BST_OP_PROJECT_CLOSE:
    case BST_OP_REBUILD:
    case BST_OP_EXIT:
      return TRUE;
    case BST_OP_PROJECT_PLAY:
      if (app->project && bse_project_can_play (app->project))
	return TRUE;
      return FALSE;
    case BST_OP_PROJECT_STOP:
      if (app->project && bse_project_is_playing (app->project))
	return TRUE;
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
    case BST_OP_HELP_QUICK_START:
      return TRUE;
    default:
      return shell ? bst_super_shell_can_operate (BST_SUPER_SHELL (shell), bst_op) : FALSE;
    }
}

static GSList *op_update_list = NULL;
static guint op_update_id = 0;
static gboolean
op_update_handler (gpointer data)
{
  guint i;
  GDK_THREADS_ENTER ();
  while (op_update_list)
    {
      GSList *tmp = op_update_list->next;
      GtkWidget *widget = op_update_list->data;
      op_update_list = tmp;
      if (BST_IS_APP (widget))
        bst_app_update_can_operate (BST_APP (widget));
      else if (BST_IS_ITEM_VIEW (widget))
        {
          BstItemView *iview = BST_ITEM_VIEW (widget);
          for (i = BST_OP_NONE; i < BST_OP_LAST; i++)
            bst_item_view_can_operate (iview, i);
        }
      g_object_unref (widget);
    }
  op_update_id = 0;
  GDK_THREADS_LEAVE ();
  return FALSE;
}

void            /* read bstdefs.h on this */
bst_update_can_operate (GtkWidget *widget)
{
  GtkWidget *anc;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  /* this function can be called multiple times in a row
   */

  /* figure toplevel app, and update it
   */
  anc = gtk_widget_get_ancestor (widget, BST_TYPE_APP);
  widget = anc ? anc : widget;
  if (!G_OBJECT (widget)->ref_count)
    return;
  if ((BST_IS_APP (widget) || BST_IS_ITEM_VIEW (widget)) &&
      !g_slist_find (op_update_list, widget))
    {
      op_update_list = g_slist_prepend (op_update_list, g_object_ref (widget));
      if (!op_update_id)
	op_update_id = g_idle_add_full (G_PRIORITY_DEFAULT, op_update_handler, NULL, NULL);
    }
}

void            /* read bstdefs.h on this */
bst_update_can_operate_unqueue (GtkWidget *widget)
{
  GtkWidget *anc;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (g_slist_find (op_update_list, widget))
    {
      op_update_list = g_slist_remove (op_update_list, widget);
      g_object_unref (widget);
      return;
    }
  anc = gtk_widget_get_ancestor (widget, BST_TYPE_APP);
  if (anc && anc != widget)
    bst_update_can_operate_unqueue (anc);
}
