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

#include "topconfig.h"

#include "bstactivatable.h"
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
static void     bst_app_class_init              (BstAppClass    *class);
static void     bst_app_init_activatable        (BstActivatableIface    *iface,
                                                 gpointer                iface_data);
static void     bst_app_init                    (BstApp         *app);
static void     bst_app_destroy                 (GtkObject      *object);
static gboolean bst_app_handle_delete_event     (GtkWidget      *widget,
                                                 GdkEventAny    *event);
static void     bst_app_menu_callback           (GtkWidget      *owner,
                                                 gulong          callback_action,
                                                 gpointer        popup_data);
static void     bst_app_run_script_proc         (GtkWidget      *widget,
                                                 gulong          category_id,
                                                 gpointer        popup_data);
static void     bst_app_activate                (BstActivatable *activatable,
                                                 gulong          action);
static gboolean bst_app_can_activate            (BstActivatable *activatable,
                                                 gulong          action);
static void     bst_app_request_update          (BstActivatable *activatable);
static void     bst_app_update_activatable      (BstActivatable *activatable);


/* --- menus --- */
static gchar       *bst_app_factories_path = "<BstApp>";
static BstMenuConfigEntry menubar_entries[] =
{
#define CB(action) (bst_app_menu_callback), (BST_ACTION_ ## action)
  { N_("/_File"),                       NULL,           NULL, 0,                        "<Branch>" },
  {    "/File/<<<<<<",                  NULL,           NULL, 0,                        "<Tearoff>" },
  { N_("/File/_New"),                   "<ctrl>N",      CB (NEW_PROJECT),               "<StockItem>", BST_STOCK_NEW },
  { N_("/File/_Open..."),               "<ctrl>O",      CB (OPEN_PROJECT),              "<StockItem>", BST_STOCK_OPEN },
  { N_("/File/_Merge..."),              "<ctrl>M",      CB (MERGE_PROJECT),             "<StockItem>", BST_STOCK_MERGE },
  { N_("/File/_Close"),                 "<ctrl>W",      CB (CLOSE_PROJECT),             "<StockItem>", BST_STOCK_CLOSE },
  {    "/File/-----",                   NULL,           NULL, 0,                        "<Separator>" },
  { N_("/File/_Save"),                  NULL,           CB (SAVE_PROJECT),              "<StockItem>", BST_STOCK_SAVE },
  { N_("/File/Save _As..."),            NULL,           CB (SAVE_PROJECT_AS),           "<StockItem>", BST_STOCK_SAVE_AS },
  {    "/File/-----",                   NULL,           NULL, 0,                        "<Separator>" },
  { N_("/File/Preferences..."),         NULL,           CB (SHOW_PREFERENCES),          "<StockItem>", BST_STOCK_PREFERENCES },
  {    "/File/-----",                   NULL,           NULL, 0,                        "<Separator>" },
  { N_("/File/_Quit"),                  "<ctrl>Q",      CB (EXIT),                      "<StockItem>", BST_STOCK_QUIT },
  { N_("/_Edit"),                       NULL,           NULL, 0,                        "<Branch>" },
  {    "/Edit/<<<<<<",                  NULL,           NULL, 0,                        "<Tearoff>" },
  { N_("/Edit/_Undo"),                  "<ctrl>Z",      CB (UNDO),                      "<StockItem>", BST_STOCK_UNDO },
  { N_("/Edit/_Redo"),                  "<ctrl>R",      CB (REDO),                      "<StockItem>", BST_STOCK_REDO },
  { N_("/Edit/_Clear Undo"),            NULL,           CB (CLEAR_UNDO),                "<StockItem>", BST_STOCK_CLEAR_UNDO },
  {    "/Edit/-----",                   NULL,           NULL, 0,                        "<Separator>" },
  { N_("/Edit/Preferences..."),         NULL,           CB (SHOW_PREFERENCES),          "<StockItem>", BST_STOCK_PREFERENCES },
  { N_("/_View"),                       NULL,           NULL, 0,                        "<Branch>" },
  {    "/View/<<<<<<",                  NULL,           NULL, 0,                        "<Tearoff>" },
  { N_("/View/Procedure _Browser"),     NULL,           CB (SHOW_PROC_BROWSER),         "<Item>" },
  { N_("/View/Rack Editor"),            NULL,           CB (RACK_EDITOR),               "<Item>" },
  { N_("/View/Device _Monitor"),        NULL,           CB (SHOW_DEVICE_MONITOR),       "<Item>" },
  {    "/View/-----",                   NULL,           NULL, 0,                        "<Separator>" },
  { N_("/View/Preferences"),            NULL,           CB (SHOW_PREFERENCES),          "<Item>" },
  {    "/View/-----",                   NULL,           NULL, 0,                        "<Separator>" },
  { N_("/View/Rebuild"),                NULL,           CB (REBUILD),                   "<Item>" },
  { N_("/_Project"),                    NULL,           NULL, 0,                        "<Branch>" },
  {    "/Project/<<<<<<",               NULL,           NULL, 0,                        "<Tearoff>" },
  { N_("/Project/_Play"),               "<ctrl>P",      CB (START_PLAYBACK),            "<StockItem>", BST_STOCK_PLAY },
  { N_("/Project/_Stop"),               "<ctrl>S",      CB (STOP_PLAYBACK),             "<StockItem>", BST_STOCK_STOP },
  {    "/Project/-----",                NULL,           NULL, 0,                        "<Separator>" },
  { N_("/Project/New Song"),            NULL,           CB (NEW_SONG),                  "<StockItem>", BST_STOCK_NEW_SONG },
  { N_("/Project/New Custom Synthesizer"), NULL,        CB (NEW_CSYNTH),                "<StockItem>", BST_STOCK_NEW_CSYNTH },
  { N_("/Project/New MIDI Synthesizer"), NULL,          CB (NEW_MIDI_SYNTH),            "<StockItem>", BST_STOCK_NEW_MIDI_SYNTH },
  { N_("/Project/Remove Synth"),        NULL,           CB (REMOVE_SYNTH),              "<StockItem>", BST_STOCK_REMOVE_SYNTH },
#if 0
  { N_("/_Song"),                       NULL,           NULL, 0,                        "<Branch>" },
  {    "/Song/<<<<<<",                  NULL,           NULL, 0,                        "<Tearoff>" },
  { N_("/Song/Add _Part"),              NULL,           CB (ADD_PART),                  "<Item>" },
  { N_("/Song/Delete _Part"),           NULL,           CB (DELETE_PART),               "<Item>" },
  { N_("/Song/Add _Track"),             NULL,           CB (ADD_TRACK),                 "<Item>" },
  { N_("/Song/Delete _Track"),          NULL,           CB (DELETE_TRACK),              "<Item>" },
  { N_("/_Synth"),                      NULL,           NULL, 0,                        "<Branch>" },
  {    "/Synth/<<<<<<",                 NULL,           NULL, 0,                        "<Tearoff>" },
  { N_("/Synth/_Test"),                 "",             CB (NONE),                      "<Item>" },
  { N_("/Wave_s"),                      NULL,           NULL, 0,                        "<Branch>" },
  {    "/Waves/<<<<<<",                 NULL,           NULL, 0,                        "<Tearoff>" },
  { N_("/Waves/_Load Wave..."),         "",             CB (LOAD_WAVE),                 "<Item>" },
  { N_("/Waves/Delete Wave"),           NULL,           CB (DELETE_WAVE),               "<Item>" },
  { N_("/Waves/_Edit Wave..."),         "",             CB (EDIT_WAVE),                 "<Item>" },
#endif
  { N_("/_Tools"),                      NULL,           NULL, 0,                        "<Branch>" },
  { N_("/Tools/_Song"),                 NULL,           NULL, 0,                        "<Branch>" },
  { N_("/Tools/_Synth"),                NULL,           NULL, 0,                        "<Branch>" },
  { N_("/Tools/Wave_s"),                NULL,           NULL, 0,                        "<Branch>" },
};
static BstMenuConfigEntry menubar_help_entries[] = {
  { N_("/_Help"),                       NULL,           NULL, 0,                        "<LastBranch>" },
  {    "/Help/<<<<<<",                  NULL,           NULL, 0,                        "<Tearoff>" },
  { N_("/Help/_Release Notes..."),      NULL,           CB (HELP_RELEASE_NOTES),        "<StockItem>", BST_STOCK_DOC_NEWS },
  { N_("/Help/Quick Start..."),         NULL,           CB (HELP_QUICK_START),          "<StockItem>", BST_STOCK_HELP },
  { N_("/Help/_FAQ..."),                NULL,           CB (HELP_FAQ),                  "<StockItem>", BST_STOCK_DOC_FAQ },
  { N_("/Help/Development/GSL Engine..."), NULL,        CB (HELP_GSL_PLAN),             "<StockItem>", BST_STOCK_DOC_DEVEL },
  {    "/Help/-----",                   NULL,           NULL, 0,                        "<Separator>" },
  { N_("/Help/_About..."),              NULL,           CB (HELP_ABOUT),                "<StockItem>", BST_STOCK_ABOUT },
#undef  CB
};


/* --- variables --- */
static BstAppClass    *bst_app_class = NULL;
static gpointer        parent_class = NULL;


/* --- functions --- */
GType
bst_app_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static const GTypeInfo type_info = {
        sizeof (BstAppClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) bst_app_class_init,
        NULL,   /* class_finalize */
        NULL,   /* class_data */
        sizeof (BstApp),
        0,      /* n_preallocs */
        (GInstanceInitFunc) bst_app_init,
      };
      static const GInterfaceInfo activatable_info = {
        (GInterfaceInitFunc) bst_app_init_activatable,  /* interface_init */
        NULL,                                           /* interface_finalize */
        NULL                                            /* interface_data */
      };
      type = g_type_register_static (GXK_TYPE_DIALOG, "BstApp", &type_info, 0);
      g_type_add_interface_static (type, BST_TYPE_ACTIVATABLE, &activatable_info);
    }
  return type;
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
bst_app_init_activatable (BstActivatableIface *iface,
                          gpointer             iface_data)
{
  iface->activate = bst_app_activate;
  iface->can_activate = bst_app_can_activate;
  iface->request_update = bst_app_request_update;
  iface->update = bst_app_update_activatable;
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
  factory = bst_item_factory_new (GTK_TYPE_MENU_BAR, bst_app_factories_path);
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
  /* Project utilities */
  cseq = bse_categories_match ("/Project/*");
  m2 = bst_menu_config_from_cats (cseq, bst_app_run_script_proc, 1, "/Tools/", BST_STOCK_EXECUTE);
  bst_menu_config_sort (m2);
  m1 = bst_menu_config_merge (m1, m2);
  /* Song utilities */
  cseq = bse_categories_match ("/Song/*");
  m2 = bst_menu_config_from_cats (cseq, bst_app_run_script_proc, 1, "/Tools/Song/", BST_STOCK_EXECUTE);
  bst_menu_config_sort (m2);
  m1 = bst_menu_config_merge (m1, m2);
  /* CSynth utilities */
  cseq = bse_categories_match ("/CSynth/*");
  m2 = bst_menu_config_from_cats (cseq, bst_app_run_script_proc, 1, "/Tools/Synth/", BST_STOCK_EXECUTE);
  bst_menu_config_sort (m2);
  m1 = bst_menu_config_merge (m1, m2);
  /* SNet utilities */
  cseq = bse_categories_match ("/SNet/*");
  m2 = bst_menu_config_from_cats (cseq, bst_app_run_script_proc, 1, "/Tools/Synth/", BST_STOCK_EXECUTE);
  bst_menu_config_sort (m2);
  m1 = bst_menu_config_merge (m1, m2);
#if 0
  /* Wave utilities */
  cseq = bse_categories_match ("/Wave/*");
  m2 = bst_menu_config_from_cats (cseq, bst_app_run_script_proc, 1, "/Tools/Waves/", BST_STOCK_EXECUTE);
  bst_menu_config_sort (m2);
  m1 = bst_menu_config_merge (m1, m2);
#endif
  /* WaveRepo utilities */
  cseq = bse_categories_match ("/WaveRepo/*");
  m2 = bst_menu_config_from_cats (cseq, bst_app_run_script_proc, 1, "/Tools/Waves/", BST_STOCK_EXECUTE);
  bst_menu_config_sort (m2);
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
                    "swapped_signal_after::switch-page", bst_widget_update_activatable, app,
                    "signal_after::switch-page", gxk_widget_viewable_changed, NULL,
                    NULL);
}

static void
bst_app_destroy (GtkObject *object)
{
  BstApp *self = BST_APP (object);

  if (self->rack_dialog)
    gtk_widget_destroy (self->rack_dialog);

  if (self->project)
    {
      if (self->pcontrols)
        bst_project_ctrl_set_project (BST_PROJECT_CTRL (self->pcontrols), 0);
      bse_project_deactivate (self->project);
      bse_proxy_disconnect (self->project,
                           "any_signal", bst_app_reload_supers, self,
                           "any_signal", bst_widget_update_activatable, self,
                           NULL);
      bse_item_unuse (self->project);
      self->project = 0;
    }

  bst_app_unregister (self);

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
  BstApp *self;

  g_return_val_if_fail (BSE_IS_PROJECT (project), NULL);

  widget = gtk_widget_new (BST_TYPE_APP,
                           "default_width", 640,
                           "default_height", 512,
                           NULL);
  self = BST_APP (widget);

  geometry.min_width = 320;
  geometry.min_height = 450;
  gtk_window_set_geometry_hints (GTK_WINDOW (widget), NULL, &geometry, GDK_HINT_MIN_SIZE);

  self->project = project;
  bse_item_use (self->project);
  bse_proxy_connect (self->project,
                     "swapped_signal::item-added", bst_app_reload_supers, self,
                     "swapped_signal::item-remove", bst_app_reload_supers, self,
                     "swapped_signal::state-changed", bst_widget_update_activatable, self,
                     "swapped_signal::property-notify::dirty", bst_widget_update_activatable, self,
                     NULL);
  bst_window_sync_title_to_proxy (GXK_DIALOG (self), self->project, "%s");
  if (self->pcontrols)
    bst_project_ctrl_set_project (BST_PROJECT_CTRL (self->pcontrols), self->project);
  
  bst_app_reload_supers (self);

  /* update menu entries
   */
  bst_widget_update_activatable (self);

  return self;
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

SfiProxy
bst_app_get_current_super (BstApp *app)
{
  GtkWidget *shell = bst_app_get_current_shell (app);
  if (BST_IS_SUPER_SHELL (shell))
    {
      BstSuperShell *super_shell = BST_SUPER_SHELL (shell);
      return super_shell->super;
    }
  return 0;
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

      if (!BST_DBG_EXT && bse_item_internal (pseq->proxies[i]))
        continue;

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
  GtkWidget *shell = bst_app_get_current_shell (self);
  SfiProxy super = shell ? BST_SUPER_SHELL (shell)->super : 0;
  const gchar *song = "", *wave_repo = "", *snet = "", *csynth = "";

  if (BST_IS_SONG_SHELL (shell) && super)
    song = "song";
  else if (BST_IS_WAVE_REPO_SHELL (shell) && super)
    wave_repo = "wrepo";
  else if (BST_IS_SNET_SHELL (shell) && super)
    {
      snet = "snet";
      if (BSE_IS_CSYNTH (super))
        csynth = "csynth";
    }

  bst_procedure_exec_auto (cat->type,
                           "project", SFI_TYPE_PROXY, self->project,
                           song, SFI_TYPE_PROXY, super,
                           wave_repo, SFI_TYPE_PROXY, super,
                           snet, SFI_TYPE_PROXY, super,
                           csynth, SFI_TYPE_PROXY, super,
                           NULL);
}

static void
bst_app_menu_callback (GtkWidget *owner,
                       gulong     callback_action,
                       gpointer   popup_data)
{
  bst_activatable_activate (BST_ACTIVATABLE (owner), callback_action);
}

static void
bst_app_activate (BstActivatable *activatable,
                  gulong          action)
{
  static GtkWidget *bst_help_dialogs[BST_ACTION_HELP_LAST - BST_ACTION_HELP_FIRST + 1] = { NULL, };
  static GtkWidget *bst_preferences = NULL;
  BstApp *self = BST_APP (activatable);
  gchar *help_file = NULL, *help_title = NULL;
  GtkWidget *widget = GTK_WIDGET (self);
  GtkWidget *shell = bst_app_get_current_shell (self);

  gxk_status_window_push (widget);

  switch (action)
    {
      SfiProxy proxy;
      GtkWidget *any;
    case BST_ACTION_NEW_PROJECT:
      if (1)
        {
          SfiProxy project = bse_server_use_new_project (BSE_SERVER, "Untitled.bse");
          BstApp *new_app;

          bse_project_get_wave_repo (project);
          new_app = bst_app_new (project);
          bse_item_unuse (project);

          gxk_idle_show_widget (GTK_WIDGET (new_app));
        }
      break;
    case BST_ACTION_OPEN_PROJECT:
      bst_file_dialog_popup_open_project (self);
      break;
    case BST_ACTION_MERGE_PROJECT:
      bst_file_dialog_popup_merge_project (self, self->project);
      break;
    case BST_ACTION_SAVE_PROJECT:
    case BST_ACTION_SAVE_PROJECT_AS:
      bst_file_dialog_popup_save_project (self, self->project);
      break;
    case BST_ACTION_CLOSE_PROJECT:
      gxk_toplevel_delete (widget);
      break;
    case BST_ACTION_EXIT:
      if (bst_app_class)
        {
          GSList *slist, *free_slist = g_slist_copy (bst_app_class->apps);

          for (slist = free_slist; slist; slist = slist->next)
            gxk_toplevel_delete (slist->data);
          g_slist_free (free_slist);
        }
      break;
    case BST_ACTION_NEW_SONG:
      proxy = bse_project_create_song (self->project, NULL);
      break;
    case BST_ACTION_NEW_CSYNTH:
      proxy = bse_project_create_csynth (self->project, NULL);
      break;
    case BST_ACTION_NEW_MIDI_SYNTH:
      proxy = bse_project_create_midi_synth (self->project, NULL);
      break;
    case BST_ACTION_REMOVE_SYNTH:
      proxy = bst_app_get_current_super (self);
      if (BSE_IS_SNET (proxy) && !bse_project_is_active (self->project))
        bse_project_remove_snet (self->project, proxy);
      break;
    case BST_ACTION_CLEAR_UNDO:
      bse_project_clear_undo (self->project);
      break;
    case BST_ACTION_UNDO:
      bse_project_undo (self->project);
      break;
    case BST_ACTION_REDO:
      bse_project_redo (self->project);
      break;
    case BST_ACTION_START_PLAYBACK:
      bst_project_ctrl_play (BST_PROJECT_CTRL (self->pcontrols));
      break;
    case BST_ACTION_STOP_PLAYBACK:
      bst_project_ctrl_stop (BST_PROJECT_CTRL (self->pcontrols));
      break;
    case BST_ACTION_RACK_EDITOR:
      if (!self->rack_dialog)
        {
          BstRackEditor *ed = g_object_new (BST_TYPE_RACK_EDITOR,
                                            "visible", TRUE,
                                            NULL);
          
          self->rack_editor = g_object_connect (ed, "swapped_signal::destroy", g_nullify_pointer, &self->rack_editor, NULL);
          bst_rack_editor_set_rack_view (ed, bse_project_get_data_pocket (self->project, "BEAST-Rack-View"));
          self->rack_dialog = gxk_dialog_new (&self->rack_dialog,
                                              GTK_OBJECT (self),
                                              0, // FIXME: undo Edit when hide && GXK_DIALOG_HIDE_ON_DELETE
                                              "Rack editor",
                                              self->rack_editor);
        }
      gxk_widget_showraise (self->rack_dialog);
      break;
    case BST_ACTION_SHOW_PREFERENCES:
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
    case BST_ACTION_SHOW_DEVICE_MONITOR:
      any = g_object_new (BST_TYPE_SERVER_MONITOR, NULL);
      gtk_widget_show (any);
      any = gxk_dialog_new (NULL,
                            GTK_OBJECT (self),
                            GXK_DIALOG_DELETE_BUTTON, // FIXME: GXK_DIALOG_HIDE_ON_DELETE && save dialog pointer
                            "Device Monitor",
                            any);
      gtk_widget_show (any);
      break;
    case BST_ACTION_SHOW_PROC_BROWSER:
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
    case BST_ACTION_REBUILD:
      gtk_container_foreach (GTK_CONTAINER (self->notebook),
                             (GtkCallback) rebuild_super_shell,
                             NULL);
      gtk_widget_queue_draw (GTK_WIDGET (self->notebook));
      break;
    case BST_ACTION_HELP_FAQ:
      help_file = g_strconcat (BST_PATH_DOCS, "/faq.markup", NULL);
      help_title = g_strdup (help_file);
      goto HELP_DIALOG;
    case BST_ACTION_HELP_GSL_PLAN:
      help_file = g_strconcat (BST_PATH_DOCS, "/gsl-mplan.markup", NULL);
      help_title = g_strdup (help_file);
      goto HELP_DIALOG;
    case BST_ACTION_HELP_QUICK_START:
      help_file = g_strconcat (BST_PATH_DOCS, "/quickstart.markup", NULL);
      help_title = g_strdup (help_file);
      goto HELP_DIALOG;
    case BST_ACTION_HELP_RELEASE_NOTES:
      help_file = g_strconcat (BST_PATH_DOCS, "/release-notes.markup", NULL);
      help_title = g_strdup_printf ("BEAST-%s Release Notes", BST_VERSION);
      goto HELP_DIALOG;
    HELP_DIALOG:
      if (!bst_help_dialogs[action - BST_ACTION_HELP_FIRST])
        {
          GtkWidget *sctext = gxk_scroll_text_create (GXK_SCROLL_TEXT_NAVIGATABLE, NULL);
          gchar *index = g_strconcat ("file://", BST_PATH_DOCS, "/beast-index.markup", NULL);
          gxk_scroll_text_set_index (sctext, index);
          g_free (index);
          gxk_scroll_text_enter (sctext, help_file);
          bst_help_dialogs[action - BST_ACTION_HELP_FIRST] = gxk_dialog_new (&bst_help_dialogs[action - BST_ACTION_HELP_FIRST],
                                                                             NULL,
                                                                             GXK_DIALOG_HIDE_ON_DELETE | GXK_DIALOG_DELETE_BUTTON,
                                                                             help_title, sctext);
          g_object_set (bst_help_dialogs[action - BST_ACTION_HELP_FIRST],
                        "default_width", 560,
                        "default_height", 640,
                        NULL);
        }
      g_free (help_file);
      g_free (help_title);
      gxk_scroll_text_rewind (gxk_dialog_get_child (GXK_DIALOG (bst_help_dialogs[action - BST_ACTION_HELP_FIRST])));
      gxk_widget_showraise (bst_help_dialogs[action - BST_ACTION_HELP_FIRST]);
      break;
    case BST_ACTION_HELP_ABOUT:
      break;
    default:
      if (shell)
        bst_activatable_activate (BST_ACTIVATABLE (shell), action);
      break;
    }

  gxk_status_window_pop ();

  bst_widget_update_activatable (activatable);
}

static gboolean
bst_app_can_activate (BstActivatable *activatable,
                      gulong          action)
{
  BstApp *self = BST_APP (activatable);

  switch (action)
    {
      GtkWidget *shell;
      SfiProxy super;
    case BST_ACTION_NEW_PROJECT:
    case BST_ACTION_OPEN_PROJECT:
    case BST_ACTION_MERGE_PROJECT:
    case BST_ACTION_SAVE_PROJECT:
    case BST_ACTION_SAVE_PROJECT_AS:
    case BST_ACTION_NEW_SONG:
    case BST_ACTION_NEW_CSYNTH:
    case BST_ACTION_NEW_MIDI_SYNTH:
    case BST_ACTION_CLOSE_PROJECT:
      return TRUE;
    case BST_ACTION_REMOVE_SYNTH:
      super = bst_app_get_current_super (self);
      return BSE_IS_SNET (super) && !bse_project_is_active (self->project);
    case BST_ACTION_CLEAR_UNDO:
      return bse_project_undo_depth (self->project) + bse_project_redo_depth (self->project) > 0;
    case BST_ACTION_UNDO:
      return bse_project_undo_depth (self->project) > 0;
    case BST_ACTION_REDO:
      return bse_project_redo_depth (self->project) > 0;
    case BST_ACTION_REBUILD:
    case BST_ACTION_EXIT:
      return TRUE;
    case BST_ACTION_START_PLAYBACK:
      if (self->project && bse_project_can_play (self->project))
        return TRUE;
      return FALSE;
    case BST_ACTION_STOP_PLAYBACK:
      if (self->project && bse_project_is_playing (self->project))
        return TRUE;
      return FALSE;
    case BST_ACTION_RACK_EDITOR:
    case BST_ACTION_SHOW_PREFERENCES:
    case BST_ACTION_SHOW_PROC_BROWSER:
    case BST_ACTION_SHOW_DEVICE_MONITOR:
      return TRUE;
      // case BST_OP_HELP_ABOUT:
    case BST_ACTION_HELP_FAQ:
    case BST_ACTION_HELP_GSL_PLAN:
    case BST_ACTION_HELP_RELEASE_NOTES:
    case BST_ACTION_HELP_QUICK_START:
      return TRUE;
    default:
      shell = bst_app_get_current_shell (self);
      return shell ? bst_activatable_can_activate (BST_ACTIVATABLE (shell), action) : FALSE;
    }
}

static void
bst_app_request_update (BstActivatable *activatable)
{
  BstApp *self = BST_APP (activatable);
  GList *list;

  /* check if the app (its widget tree) was already destroyed */
  if (!GTK_BIN (self)->child)
    return;

  /* chain to normal handler */
  bst_activatable_default_request_update (activatable);

  /* enqueue activatable children */
  list = gtk_container_get_children (GTK_CONTAINER (self->notebook));
  while (list)
    {
      BstActivatable *activatable = g_list_pop_head (&list);
      if (BST_IS_ACTIVATABLE (activatable))
        bst_widget_update_activatable (activatable);
    }
}

static void
bst_app_update_activatable (BstActivatable *activatable)
{
  BstApp *self = BST_APP (activatable);
  GtkWidget *widget, *shell = bst_app_get_current_shell (self);
  GtkItemFactory *factory = bst_app_menu_factory (self);
  gulong i;

  /* check if the app (its widget tree) was already destroyed */
  if (!GTK_BIN (self)->child)
    return;

  /* update menu bar menu items */
  gxk_item_factory_sensitize (factory, "/Song", BST_IS_SONG_SHELL (shell));
  gxk_item_factory_sensitize (factory, "/Tools/Song", BST_IS_SONG_SHELL (shell));
  gxk_item_factory_sensitize (factory, "/Synth", BST_IS_SNET_SHELL (shell));
  gxk_item_factory_sensitize (factory, "/Tools/Synth", BST_IS_SNET_SHELL (shell));
  gxk_item_factory_sensitize (factory, "/Waves", BST_IS_WAVE_REPO_SHELL (shell));
  gxk_item_factory_sensitize (factory, "/Tools/Waves", BST_IS_WAVE_REPO_SHELL (shell));

  /* special adjustments */
  widget = gxk_item_factory_get_item (factory, "/Edit/Clear Undo");
  if (widget && BST_DVL_HINTS)
    gtk_widget_show (widget);
  else if (widget && !BST_DVL_HINTS)
    gtk_widget_hide (widget);

  /* update app actions */
  for (i = 0; i < G_N_ELEMENTS (menubar_entries); i++)
    {
      gulong action = menubar_entries[i].callback_action;
      widget = gtk_item_factory_get_widget_by_action (factory, action);
      if (widget && action)
        gtk_widget_set_sensitive (widget, bst_activatable_can_activate (activatable, action));
    }
  for (i = 0; i < G_N_ELEMENTS (menubar_help_entries); i++)
    {
      gulong action = menubar_help_entries[i].callback_action;
      widget = gtk_item_factory_get_widget_by_action (factory, action);
      if (widget && action)
        gtk_widget_set_sensitive (widget, bst_activatable_can_activate (activatable, action));
    }
}
