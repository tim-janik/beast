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


#include	"bstsongshell.h"
#include	"bstsnetshell.h"
#include	"bstfiledialog.h"
#include	"bststatusbar.h"



/* --- prototypes --- */
static void	bst_app_class_init		(BstAppClass	 *class);
static void	bst_app_build			(GleGWidget	 *gwidget,
						 gpointer	  data);
static void	bst_app_destroy			(GtkObject	 *object);
static gint	bst_app_handle_delete_event	(GtkWidget	 *widget);


/* --- menus --- */
static gchar	   *bst_app_factories_path = "<BstApp>";
static GtkItemFactoryEntry menubar_entries[] =
{
#define BST_OP(bst_op) (bst_app_operate), (BST_OP_ ## bst_op)
  { "/_File",			NULL,		NULL, 0,		"<Branch>" },
  { "/File/<<<<<<",		NULL,		NULL, 0,		"<Tearoff>" },
  { "/File/_New",		"<ctrl>N",	BST_OP (PROJECT_NEW),	"<Item>" },
  { "/File/_Open...",		"<ctrl>O",	BST_OP (PROJECT_OPEN),	"<Item>" },
  { "/File/_Save",		"<ctrl>S",	BST_OP (PROJECT_SAVE),	"<Item>" },
  { "/File/Save _As...",	NULL,		BST_OP (PROJECT_SAVE_AS),"<Item>" },
  { "/File/-----",		NULL,		NULL, 0,		"<Separator>" },
  { "/File/_Close",		"<ctrl>W",	BST_OP (PROJECT_CLOSE),	"<Item>" },
  { "/File/_Exit",		"<ctrl>Q",	BST_OP (EXIT),		"<Item>" },
  { "/_Project",		NULL,		NULL, 0,		"<Branch>" },
  { "/Project/<<<<<<",		NULL,		NULL, 0,		"<Tearoff>" },
  { "/Project/_Play",		"",		BST_OP (PROJECT_PLAY),	"<Item>" },
  { "/Project/_Stop",		"",		BST_OP (PROJECT_STOP),	"<Item>" },
  { "/Project/-----",		NULL,		NULL, 0,		"<Separator>" },
  { "/Project/New Song",	NULL,		BST_OP (PROJECT_NEW_SONG), "<Item>" },
  { "/Project/New Source Net",	NULL,		BST_OP (PROJECT_NEW_SNET), "<Item>" },
  { "/Project/-----",		NULL,		NULL, 0,		"<Separator>" },
  { "/Project/Rebuild",		NULL,		BST_OP (REBUILD),	"<Item>" },
  { "/Project/Refresh",		NULL,		BST_OP (REFRESH),	"<Item>" },
  { "/_Edit",			NULL,		NULL, 0,		"<Branch>" },
  { "/Edit/<<<<<<",		NULL,		NULL, 0,		"<Tearoff>" },
  { "/Edit/_Undo",		"<ctrl>U",	BST_OP (UNDO_LAST),	"<Item>" },
  { "/Edit/_Redo",		"<ctrl>R",	BST_OP (REDO_LAST),	"<Item>" },
  { "/_Song",			NULL,		NULL, 0,		"<Branch>" },
  { "/Song/<<<<<<",		NULL,		NULL, 0,		"<Tearoff>" },
  { "/Song/_Add Pattern",	"<ctrl>A",	BST_OP (PATTERN_ADD),	"<Item>" },
  { "/Song/Delete Pattern",	NULL,		BST_OP (PATTERN_DELETE),"<Item>" },
  { "/Song/_Edit Pattern...",	"<ctrl>E",	BST_OP (PATTERN_EDITOR),"<Item>" },
  { "/Song/Add _Instrument",	"<ctrl>A",	BST_OP (INSTRUMENT_ADD),"<Item>" },
  { "/S_Net",			NULL,		NULL, 0,		"<Branch>" },
  { "/SNet/<<<<<<",		NULL,		NULL, 0,		"<Tearoff>" },
  { "/SNet/_Test",		"",		BST_OP (NONE),		"<Item>" },
  { "/_Help",			NULL,		NULL, 0,		"<LastBranch>" },
  { "/Help/<<<<<<",		NULL,		NULL, 0,		"<Tearoff>" },
  { "/Help/_About...",		NULL,		BST_OP (HELP_ABOUT),	"<Item>" },
#undef	BST_OP
};
static guint n_menubar_entries = sizeof (menubar_entries) / sizeof (menubar_entries[0]);


/* --- variables --- */
static BstAppClass    *bst_app_class = NULL;
static GleGToplevel   *bst_app_proxy = NULL;
static gpointer        parent_class = NULL;


/* --- functions --- */
void
bst_app_register (void)
{
  bst_app_get_type ();
  gle_handler_register_default ("bst_app_handle_delete_event", bst_app_handle_delete_event, NULL);
}

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
	(GtkObjectInitFunc) NULL,
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

  bst_app_class = class;
  parent_class = gtk_type_class (GTK_TYPE_WINDOW);
  object_class = GTK_OBJECT_CLASS (class);

  object_class->destroy = bst_app_destroy;

  class->apps = NULL;
}

static void
app_set_title (BstApp *app)
{
  gchar *title;

  title = g_strconcat ("BEAST: ", BSE_OBJECT_NAME (app->project), NULL);
  gtk_window_set_title (GTK_WINDOW (app), title);
  g_free (title);
}

static void
bst_app_destroy (GtkObject *object)
{
  BstApp *app = BST_APP (object);

  bst_master_unref (app->master);
  app->master = NULL;
  if (app->project)
    {
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

  if (!bst_app_proxy)
    {
      bst_app_proxy = (GleGToplevel*) gle_gname_lookup ("BstApp");

      g_return_val_if_fail (bst_app_proxy != NULL, NULL);

      GLE_NOTIFIER_INSTALL (bst_app_proxy, "post_instantiate", bst_app_build, NULL);
    }

  gle_gtoplevel_instantiate (bst_app_proxy);
  widget = GLE_GWIDGET_WIDGET (bst_app_proxy);
  app = BST_APP (widget);
  gle_gtoplevel_disassociate (bst_app_proxy);

  bst_status_bar_ensure (GTK_WINDOW (app));

  bse_object_ref (BSE_OBJECT (project));
  app->project = project;
  bse_object_add_data_notifier (BSE_OBJECT (project),
				"name-set",
				app_set_title,
				app);
  app_set_title (app);

  bst_app_reload_supers (app);

  return app;
}

static void
bst_app_build (GleGWidget *gwidget,
	       gpointer	   data)
{
  BstApp *app;
  GtkWidget *widget;
  GtkItemFactory *factory;

  widget = GLE_GWIDGET_WIDGET (gwidget);
  app = BST_APP (widget);

  bst_app_class->apps = g_slist_prepend (bst_app_class->apps, app);

  app->master = bst_master_ref ();

  app->main_vbox = gle_widget_from_gname ("MainVBox");
  app->notebook = (GtkNotebook*) gle_widget_from_gname ("Notebook");

  gtk_window_set_wmclass (GTK_WINDOW (widget), GTK_WINDOW (widget)->title, g_get_prgname ());

  /* setup the menu bar
   */
  factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, bst_app_factories_path, NULL);
  gtk_window_add_accel_group (GTK_WINDOW (widget), factory->accel_group);
  gtk_item_factory_create_items (factory, n_menubar_entries, menubar_entries, widget);
  gtk_container_add_with_args (GTK_CONTAINER (app->main_vbox),
			       factory->widget,
			       "expand", FALSE,
			       "position", 0,
			       NULL);
  gtk_widget_show (factory->widget);
  gtk_object_set_data_full (GTK_OBJECT (widget),
			    bst_app_factories_path,
			    factory,
			    (GtkDestroyNotify) gtk_object_unref);

  /* setup notebook handlers
   */
  gtk_signal_connect_object_after (GTK_OBJECT (app->notebook),
				   "switch-page",
				   bst_update_can_operate,
				   GTK_OBJECT (app));

  /* update menu entries
   */
  bst_app_update_can_operate (app);
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

BseMaster*
bst_app_get_master (BstApp *app)
{
  g_return_val_if_fail (BST_IS_APP (app), NULL);

  return BSE_MASTER (app->master);
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
			    "super", super,
			    "visible", TRUE,
			    NULL);
  else if (BSE_IS_SNET (super))
    shell = gtk_widget_new (BST_TYPE_SNET_SHELL,
			    "super", super,
			    "visible", TRUE,
			    NULL);
  else
    {
      g_message ("FIXME: skipping dialog for %s", BSE_OBJECT_TYPE_NAME (super));
      return;
    }

  gtk_notebook_popup_enable (app->notebook);
  gtk_notebook_append_page (app->notebook,
			    shell,
			    gtk_widget_new (GTK_TYPE_LABEL,
					    "label", BSE_OBJECT_NAME (super),
					    "visible", TRUE,
					    NULL));
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

static gint
bst_app_handle_delete_event (GtkWidget *widget)
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
  static GtkWidget *bst_dialog_open = NULL;
  static GtkWidget *bst_dialog_save = NULL;
  GtkWidget *widget, *shell;

  g_return_if_fail (BST_IS_APP (app));
  g_return_if_fail (bst_app_can_operate (app, op));

  widget = GTK_WIDGET (app);
  shell = bst_app_get_current_shell (app);

  gtk_widget_ref (widget);

  switch (op)
    {
      BstSuperShell *super_shell;
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
	  gtk_widget_show (bst_dialog_open);
	}
      gdk_window_raise (bst_dialog_open->window);
      break;
    case BST_OP_PROJECT_SAVE_AS:
      if (bst_dialog_save)
	gtk_widget_destroy (bst_dialog_save);
      bst_dialog_save = bst_file_dialog_new_save (app);
      gtk_signal_connect (GTK_OBJECT (bst_dialog_save),
			  "destroy",
			  gtk_widget_destroyed,
			  &bst_dialog_save);
      gtk_widget_show (bst_dialog_save);
      gdk_window_raise (bst_dialog_save->window);
      break;
    case BST_OP_PROJECT_CLOSE:
      bst_app_handle_delete_event (widget);
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
      gtk_container_foreach (GTK_CONTAINER (app->notebook),
			     (GtkCallback) foreach_super_shell_operate,
			     GUINT_TO_POINTER (BST_OP_PLAY));
      break;
    case BST_OP_PROJECT_STOP:
      gtk_container_foreach (GTK_CONTAINER (app->notebook),
			     (GtkCallback) foreach_super_shell_operate,
			     GUINT_TO_POINTER (BST_OP_STOP));
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
      gpointer data[2];
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
      data[0] = GUINT_TO_POINTER (BST_OP_STOP);
      data[1] = NONE;
      gtk_container_foreach (GTK_CONTAINER (app->notebook),
			     (GtkCallback) forwhich_super_shell_can_operate,
			     data);
      return data[1] == NULL;
      break;
    case BST_OP_PROJECT_STOP:
      data[0] = GUINT_TO_POINTER (BST_OP_STOP);
      data[1] = NULL;
      gtk_container_foreach (GTK_CONTAINER (app->notebook),
			     (GtkCallback) forany_super_shell_can_operate,
			     data);
      return data[1] != NULL;
      break;
    default:
      return shell ? bst_super_shell_can_operate (BST_SUPER_SHELL (shell), bst_op) : FALSE;
    }
}
