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
#include "topconfig.h"
#include "bstapp.h"
#include "bstparam.h"
#include "bstskinconfig.h"
#include "bstsupershell.h"
#include "bstfiledialog.h"
#include "bstgconfig.h"
#include "bstpreferences.h"
#include "bstprocbrowser.h"
#include "bstservermonitor.h"
#include "bstrackeditor.h"
#include "bstmenus.h"
#include "bstprocedure.h"
#include "bstprojectctrl.h"
#include "bstprofiler.h"
#include "bstusermessage.h"
#include <string.h>


/* --- prototypes --- */
static void           bst_app_run_script_proc     (gpointer     data,
                                                   gulong       category_id);
static GxkActionList* demo_entries_create         (BstApp      *app);
static GxkActionList* skin_entries_create         (BstApp      *app);
static void           app_action_exec             (gpointer     data,
                                                   gulong       action);
static gboolean       app_action_check            (gpointer     data,
                                                   gulong       action,
                                                   guint64      action_stamp);
static void           bst_app_reload_pages        (BstApp      *self);


/* --- menus --- */
enum {
  ACTION_INTERNALS = BST_ACTION_APP_LAST,
  /* dialogs */
  ACTION_SHOW_PREFERENCES,
  ACTION_SHOW_PROC_BROWSER,
  ACTION_SHOW_PROFILER,
  ACTION_EXTRA_VIEW,
  ACTION_RACK_EDITOR,
#define ACTION_HELP_FIRST   ACTION_HELP_INDEX
  ACTION_HELP_INDEX,
  ACTION_HELP_FAQ,
  ACTION_HELP_RELEASE_NOTES,
  ACTION_HELP_QUICK_START,
  ACTION_HELP_PLUGIN_DEVEL,
  ACTION_HELP_DSP_ENGINE,
  ACTION_HELP_DEVELOPMENT,
  ACTION_HELP_ABOUT,
#define ACTION_HELP_LAST    ACTION_HELP_ABOUT
  ACTION_URL_HELP_DESK,
  ACTION_URL_BEAST_SITE,
  ACTION_URL_ONLINE_SYNTHESIZERS,
  ACTION_URL_ONLINE_DEMOS,
  ACTION_DEMO_DIALOG_FATAL,
  ACTION_DEMO_DIALOG_ERROR,
  ACTION_DEMO_DIALOG_WARNING,
  ACTION_DEMO_DIALOG_SCRIPT,
  ACTION_DEMO_DIALOG_INFO,
  ACTION_DEMO_DIALOG_DIAG,
  ACTION_DEMO_DIALOG_DEBUG,
};
static const GxkStockAction file_open_actions[] = {
  { N_("_New"),                 "<ctrl>N",      N_("Create new project"),
    BST_ACTION_NEW_PROJECT,     BST_STOCK_NEW,  },
  { N_("_Open..."),             "<ctrl>O",      N_("Open existing project"),
    BST_ACTION_OPEN_PROJECT,    BST_STOCK_OPEN, },
  { N_("_Merge..."),            "<ctrl>M",      N_("Merge an existing project into the current project"),
    BST_ACTION_MERGE_PROJECT,   BST_STOCK_MERGE, },
  { N_("_Import MIDI..."),      "",             N_("Import a standard MIDI file into the current project"),
    BST_ACTION_IMPORT_MIDI,     BST_STOCK_OPEN, },
  { N_("_Close"),               "<ctrl>W",      N_("Close the project"),
    BST_ACTION_CLOSE_PROJECT,   BST_STOCK_CLOSE, },
};
static const GxkStockAction file_save_actions[] = {
  { N_("_Save"),                NULL,           N_("Write project to disk"),
    BST_ACTION_SAVE_PROJECT,    BST_STOCK_SAVE, },
  { N_("Save _As..."),          NULL,           N_("Write project to a specific file"),
    BST_ACTION_SAVE_PROJECT_AS, BST_STOCK_SAVE_AS, },
};
static const GxkStockAction file_epilog_actions[] = {
  { N_("_Quit"),                "<ctrl>Q",      N_("Close all windows and quit"),
    BST_ACTION_EXIT,            BST_STOCK_QUIT, },
};
static const GxkStockAction preference_actions[] = {
  { N_("_Preferences..."),      NULL,           N_("Adjust overall program behaviour"),
    ACTION_SHOW_PREFERENCES,    BST_STOCK_PREFERENCES, },
};
static const GxkStockAction rebuild_actions[] = {
  { N_("Rebuild"),              NULL,           NULL,   BST_ACTION_REBUILD, },
};
static const GxkStockAction about_actions[] = {
  { N_("_About..."),            NULL,           N_("Display developer and contributor credits"),
    ACTION_HELP_ABOUT,          BST_STOCK_ABOUT },
};
static const GxkStockAction undo_actions[] = {
  { N_("_Undo"),                "<ctrl>Z",      N_("Undo the effect of the last action"),
    BST_ACTION_UNDO,            BST_STOCK_UNDO, },
  { N_("_Redo"),                "<ctrl>Y",      N_("Redo the last undone action"),
    BST_ACTION_REDO,            BST_STOCK_REDO, },
};
static const GxkStockAction undo_dvl_actions[] = {
  { N_("_Clear Undo"),          NULL,           N_("Delete the complete undo history"),
    BST_ACTION_CLEAR_UNDO,      BST_STOCK_CLEAR_UNDO, },
};
static const GxkStockAction dialog_actions[] = {
  { N_("Procedure _Browser"),   NULL,           N_("Display an overview of all procedures"),
    ACTION_SHOW_PROC_BROWSER, },
  { N_("Rack Editor"),          NULL,           NULL,
    ACTION_RACK_EDITOR, },
  { N_("Profiler"),             NULL,           N_("Display statistics and timing information"),
    ACTION_SHOW_PROFILER, },
  { N_("New View"),             NULL,           N_("Create an extra view of the project"),
    ACTION_EXTRA_VIEW, },
};
static const GxkStockAction playback_actions[] = {
  { N_("_Play"),                "<ctrl>P",      N_("Play or restart playback of the project"),
    BST_ACTION_START_PLAYBACK,  BST_STOCK_PLAY },
  { N_("_Stop"),                "<ctrl>S",      N_("Stop playback of the project"),
    BST_ACTION_STOP_PLAYBACK,  BST_STOCK_STOP },
};
static const GxkStockAction project_actions[] = {
  { N_("New Song"),             NULL,           N_("Create a new song, consisting of a mixer, tracks, parts and notes"),
    BST_ACTION_NEW_SONG,        BST_STOCK_MINI_SONG },
  { N_("Add Custom Synthesizer"), NULL,         N_("Add a new synthesizer mesh to be used as effect or instrument in songs"),
    BST_ACTION_NEW_CSYNTH,      BST_STOCK_MINI_CSYNTH },
  { N_("Add MIDI Synthesizer"), NULL,           N_("Add a new MIDI synthesizer to control an instrument from external MIDI events"),
    BST_ACTION_NEW_MIDI_SYNTH,  BST_STOCK_MINI_MIDI_SYNTH },
  { N_("Remove Song or Synthesizer"), NULL,     N_("Remove the currently selected synthesizer (song)"),
    BST_ACTION_REMOVE_SYNTH,    BST_STOCK_REMOVE_SYNTH },
};
static const GxkStockAction library_files_actions[] = {
  { N_("Load _Instrument..."),  NULL,           N_("Load synthesizer mesh from instruments folder"),
    BST_ACTION_MERGE_INSTRUMENT,BST_STOCK_OPEN },
  { N_("Load _Effect..."),      NULL,           N_("Load synthesizer mesh from effects folder"),
    BST_ACTION_MERGE_EFFECT,    BST_STOCK_OPEN },
  { N_("Save As Instrument..."),NULL,           N_("Save synthesizer mesh to instruments folder"),
    BST_ACTION_SAVE_INSTRUMENT, BST_STOCK_SAVE_AS },
  { N_("Save As Effect..."),    NULL,           N_("Save synthesizer mesh to effects folder"),
    BST_ACTION_SAVE_EFFECT,     BST_STOCK_SAVE_AS },
};
static const GxkStockAction simple_help_actions[] = {
  { N_("Document _Index..."),   NULL,           N_("Provide an overview of all BEAST documentation contents"),
    ACTION_HELP_INDEX,          BST_STOCK_DOC_INDEX },
  { N_("_Quick Start..."),      NULL,           N_("Provides an introduction about how to accomplish the most common tasks"),
    ACTION_HELP_QUICK_START,    BST_STOCK_HELP },
  { N_("_FAQ..."),              NULL,           N_("Answers to frequently asked questions"),
    ACTION_HELP_FAQ,            BST_STOCK_DOC_FAQ },
  { N_("Online _Help Desk..."), NULL,           N_("Start a web browser pointing to the online help desk at the BEAST website"),
    ACTION_URL_HELP_DESK,       BST_STOCK_ONLINE_HELP_DESK },
  { N_("_BEAST Website..."),    NULL,           N_("Start a web browser pointing to the BEAST website"),
    ACTION_URL_BEAST_SITE,      BST_STOCK_ONLINE_BEAST_SITE },
#if 0
  { N_("_Release Notes..."),    NULL,           N_("Notes and informations about this release cycle"),
    ACTION_HELP_RELEASE_NOTES,  BST_STOCK_DOC_NEWS },
  { N_("Developing Plugins..."),NULL,           N_("A guide to synthesis plugin development"),
    ACTION_HELP_PLUGIN_DEVEL,   BST_STOCK_DOC_DEVEL },
  { N_("DSP Engine..."),        NULL,           N_("Technical description of the multi-threaded synthesis engine innards"),
    ACTION_HELP_DSP_ENGINE,     BST_STOCK_DOC_DEVEL },
#endif
  { N_("Development..."),       NULL,           N_("Provide an overview of development related topics and documents"),
    ACTION_HELP_DEVELOPMENT,    BST_STOCK_DOC_DEVEL },
};
static const GxkStockAction online_synthesizers[] = {
  { N_("Online Sound Archive..."),  NULL,        N_("Start a web browser pointing to the online sound archive"),
    ACTION_URL_ONLINE_SYNTHESIZERS, BST_STOCK_ONLINE_SOUND_ARCHIVE },
};
static const GxkStockAction online_demos[] = {
  { N_("Online Demos..."),      NULL,           N_("Start a web browser pointing to online demo songs"),
    ACTION_URL_ONLINE_DEMOS,        BST_STOCK_ONLINE_SOUND_ARCHIVE },
};
static const GxkStockAction demo_dialogs[] = {
  { "Demo Fatal Error Dialog",  NULL,           "Fire up a fatal error dialog for demonstration purposes",
    ACTION_DEMO_DIALOG_FATAL,   BST_STOCK_ERROR },
  { "Demo Error Dialog",        NULL,           "Fire up an error dialog for demonstration purposes",
    ACTION_DEMO_DIALOG_ERROR,   BST_STOCK_ERROR },
  { "Demo Warning Dialog",      NULL,           "Fire up a warning dialog for demonstration purposes",
    ACTION_DEMO_DIALOG_WARNING, BST_STOCK_WARNING },
  { "Demo Script Dialog",       NULL,           "Fire up a script dialog for demonstration purposes",
    ACTION_DEMO_DIALOG_SCRIPT,  BST_STOCK_EXECUTE },
  { "Demo Info Dialog",         NULL,           "Fire up an information dialog for demonstration purposes",
    ACTION_DEMO_DIALOG_INFO,    BST_STOCK_INFO },
  { "Demo Diag Dialog",         NULL,           "Fire up a diagnostics dialog for demonstration purposes",
    ACTION_DEMO_DIALOG_DIAG,    BST_STOCK_DIAG },
  { "Demo Debug Dialog",        NULL,           "Fire up a debug dialog for demonstration purposes",
    ACTION_DEMO_DIALOG_DEBUG,   BST_STOCK_DIAG },
};

/* --- variables --- */
static BstAppClass    *bst_app_class = NULL;


/* --- functions --- */
G_DEFINE_TYPE (BstApp, bst_app, GXK_TYPE_DIALOG);

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
bst_app_init (BstApp *self)
{
  GtkWidget *widget = GTK_WIDGET (self);
  BseCategorySeq *cseq;
  GxkActionList *al1, *al2;
  
  g_object_set (self,
                "name", "BEAST-Application",
                "allow_shrink", TRUE,
                "allow_grow", TRUE,
                "flags", GXK_DIALOG_STATUS_BAR | GXK_DIALOG_IGNORE_ESCAPE, // | GXK_DIALOG_WINDOW_GROUP,
                NULL);
  bst_app_register (self);
  self->box = gxk_radget_create ("beast", "application-box", NULL);
  gtk_container_add (GTK_CONTAINER (GXK_DIALOG (self)->vbox), self->box);
  
  /* publish widget specific actions */
  gxk_widget_publish_actions (self, "file-open", G_N_ELEMENTS (file_open_actions), file_open_actions,
                              NULL, app_action_check, app_action_exec);
  gxk_widget_publish_actions (self, "file-save", G_N_ELEMENTS (file_save_actions), file_save_actions,
                              NULL, app_action_check, app_action_exec);
  gxk_widget_publish_actions (self, "file-epilog", G_N_ELEMENTS (file_epilog_actions), file_epilog_actions,
                              NULL, app_action_check, app_action_exec);
  gxk_widget_publish_actions (self, "preference", G_N_ELEMENTS (preference_actions), preference_actions,
                              NULL, app_action_check, app_action_exec);
  gxk_widget_publish_actions (self, "rebuild", G_N_ELEMENTS (rebuild_actions), rebuild_actions,
                              NULL, app_action_check, app_action_exec);
  gxk_widget_publish_actions (self, "about", G_N_ELEMENTS (about_actions), about_actions,
                              NULL, app_action_check, app_action_exec);
  gxk_widget_publish_actions (self, "undo", G_N_ELEMENTS (undo_actions), undo_actions,
                              NULL, app_action_check, app_action_exec);
  if (BST_DVL_HINTS)
    gxk_widget_publish_actions (self, "undo-dvl", G_N_ELEMENTS (undo_dvl_actions), undo_dvl_actions,
                                NULL, app_action_check, app_action_exec);
  gxk_widget_publish_actions (self, "dialog", G_N_ELEMENTS (dialog_actions), dialog_actions,
                              NULL, app_action_check, app_action_exec);
  gxk_widget_publish_actions (self, "playback", G_N_ELEMENTS (playback_actions), playback_actions,
                              NULL, app_action_check, app_action_exec);
  gxk_widget_publish_actions (self, "project", G_N_ELEMENTS (project_actions), project_actions,
                              NULL, app_action_check, app_action_exec);
  gxk_widget_publish_actions (self, "library-files", G_N_ELEMENTS (library_files_actions), library_files_actions,
                              NULL, app_action_check, app_action_exec);
  gxk_widget_publish_actions (self, "simple-help", G_N_ELEMENTS (simple_help_actions), simple_help_actions,
                              NULL, app_action_check, app_action_exec);
  gxk_widget_publish_actions (self, "online-synthesizers", G_N_ELEMENTS (online_synthesizers), online_synthesizers,
                              NULL, app_action_check, app_action_exec);
  gxk_widget_publish_actions (self, "online-demos", G_N_ELEMENTS (online_demos), online_demos,
                              NULL, app_action_check, app_action_exec);
  if (BST_DVL_HINTS)
    gxk_widget_publish_actions (self, "demo-dialogs", G_N_ELEMENTS (demo_dialogs), demo_dialogs,
                                NULL, app_action_check, app_action_exec);
  /* Project utilities */
  cseq = bse_categories_match ("/Project/*");
  al1 = bst_action_list_from_cats (cseq, 1, BST_STOCK_EXECUTE, NULL, bst_app_run_script_proc, self);
  gxk_action_list_sort (al1);
  gxk_widget_publish_action_list (widget, "tools-project", al1);
  /* Song scripts */
  cseq = bse_categories_match ("/Song/*");
  al1 = bst_action_list_from_cats (cseq, 1, BST_STOCK_EXECUTE, NULL, bst_app_run_script_proc, self);
  gxk_action_list_sort (al1);
  gxk_widget_publish_action_list (widget, "tools-song", al1);
  /* CSynth & SNet utilities */
  cseq = bse_categories_match ("/CSynth/*");
  al1 = bst_action_list_from_cats (cseq, 1, BST_STOCK_EXECUTE, NULL, bst_app_run_script_proc, self);
  gxk_action_list_sort (al1);
  cseq = bse_categories_match ("/SNet/*");
  al2 = bst_action_list_from_cats (cseq, 1, BST_STOCK_EXECUTE, NULL, bst_app_run_script_proc, self);
  gxk_action_list_sort (al2);
  al1 = gxk_action_list_merge (al1, al2);
  gxk_widget_publish_action_list (widget, "tools-synth", al1);
  /* WaveRepo utilities */
  cseq = bse_categories_match ("/WaveRepo/*");
  al1 = bst_action_list_from_cats (cseq, 1, BST_STOCK_EXECUTE, NULL, bst_app_run_script_proc, self);
  gxk_action_list_sort (al1);
  gxk_widget_publish_action_list (widget, "tools-wave-repo", al1);
  /* add demo songs */
  al1 = demo_entries_create (self);
  gxk_widget_publish_action_list (widget, "demo-songs", al1);
  /* add skins */
  al1 = skin_entries_create (self);
  gxk_action_list_sort (al1);
  gxk_widget_publish_action_list (widget, "skin-options", al1);
  
  /* setup playback controls */
  self->pcontrols = g_object_new (BST_TYPE_PROJECT_CTRL, NULL);
  gxk_radget_add (self->box, "control-area", self->pcontrols);

  /* setup project pages */
  self->ppages = gxk_assortment_new ();
  gxk_widget_publish_assortment (widget, "project-pages", self->ppages);

  /* setup WAVE file entry */
  // gxk_radget_add (self->box, "control-area", gxk_vseparator_space_new (TRUE));
  self->wave_file = bst_param_new_proxy (bse_proxy_get_pspec (BSE_SERVER, "wave_file"), BSE_SERVER);
  if (0) // FIXME
    gxk_radget_add (self->box, "export-area-file-label", gxk_param_create_editor (self->wave_file, "name"));
  else
    gxk_radget_add (self->box, "export-area-file-label", g_object_new (GTK_TYPE_LABEL,
                                                                        "visible", TRUE,
                                                                        "label", _("Export Audio"),
                                                                        NULL));
  gxk_radget_add (self->box, "export-area-file-entry", gxk_param_create_editor (self->wave_file, NULL));
  gxk_param_update (self->wave_file);
  
  /* setup the main notebook */
  self->notebook = gxk_radget_find (self->box, "main-notebook");
  gxk_nullify_in_object (self, &self->notebook);
  g_object_connect (self->notebook,
                    "swapped_signal_after::switch-page", gxk_widget_update_actions, self,
                    NULL);
}

static void
bst_app_destroy (GtkObject *object)
{
  BstApp *self = BST_APP (object);
  
  if (self->wave_file)
    {
      gxk_param_destroy (self->wave_file);
      self->wave_file = NULL;
    }
  
  if (self->rack_dialog)
    gtk_widget_destroy (self->rack_dialog);
  
  if (self->project)
    {
      if (self->pcontrols)
        bst_project_ctrl_set_project (BST_PROJECT_CTRL (self->pcontrols), 0);
      bse_project_deactivate (self->project);
      bse_proxy_disconnect (self->project,
                            "any_signal", bst_app_reload_pages, self,
                            "any_signal", gxk_widget_update_actions, self,
                            NULL);
      bse_item_unuse (self->project);
      self->project = 0;
    }

  if (self->ppages)
    gxk_assortment_dispose (self->ppages);

  bst_app_unregister (self);
  
  GTK_OBJECT_CLASS (bst_app_parent_class)->destroy (object);
  
  if (!bst_app_class->apps && bst_app_class->seen_apps)
    {
      bst_app_class->seen_apps = FALSE;
      BST_MAIN_LOOP_QUIT ();
    }
}

static void
bst_app_finalize (GObject *object)
{
  BstApp *self = BST_APP (object);
  
  if (self->project)
    {
      bse_proxy_disconnect (self->project,
                            "any_signal", bst_app_reload_pages, self,
                            "any_signal", gxk_widget_update_actions, self,
                            NULL);
      bse_item_unuse (self->project);
      self->project = 0;
    }
  if (self->ppages)
    {
      gxk_assortment_dispose (self->ppages);
      g_object_unref (self->ppages);
      self->ppages = NULL;
    }
  
  G_OBJECT_CLASS (bst_app_parent_class)->finalize (object);
}

BstApp*
bst_app_new (SfiProxy project)
{
  g_return_val_if_fail (BSE_IS_PROJECT (project), NULL);
  
  BstApp *self = g_object_new (BST_TYPE_APP, NULL);
  gxk_dialog_set_sizes (GXK_DIALOG (self), 500, 400, 950, 800);

  self->project = project;
  bse_item_use (self->project);
  bse_proxy_connect (self->project,
                     "swapped_signal::item-added", bst_app_reload_pages, self,
                     "swapped_signal::item-remove", bst_app_reload_pages, self,
                     "swapped_signal::state-changed", gxk_widget_update_actions, self,
                     "swapped_signal::property-notify::dirty", gxk_widget_update_actions, self,
                     NULL);
  bst_window_sync_title_to_proxy (GXK_DIALOG (self), self->project, "%s");
  if (self->pcontrols)
    bst_project_ctrl_set_project (BST_PROJECT_CTRL (self->pcontrols), self->project);
  
  bst_app_reload_pages (self);
  
  /* update menu entries
   */
  gxk_widget_update_actions (self);
  
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

static SfiProxy
bst_app_get_current_super (BstApp *app)
{
  g_return_val_if_fail (BST_IS_APP (app), 0);
  if (app->notebook && app->notebook->cur_page)
    {
      GtkWidget *shell = gtk_notebook_current_widget (app->notebook);
      if (BST_IS_SUPER_SHELL (shell))
        {
          BstSuperShell *super_shell = BST_SUPER_SHELL (shell);
          return super_shell->super;
        }
    }
  return 0;
}

static gint
proxy_rate_item (SfiProxy p)
{
  if (BSE_IS_WAVE_REPO (p))
    return 1;
  else if (BSE_IS_SONG (p))
    return 2;
  else if (BSE_IS_MIDI_SYNTH (p))
    return 4;
  else if (BSE_IS_CSYNTH (p))
    return 3;
  return 5;
}

static void
app_update_page_item (SfiProxy     item,
                      const gchar *property_name,
                      BstApp      *self)
{
  GxkAssortmentEntry *entry = gxk_assortment_find_data (self->ppages, (void*) item);
  if (entry)
    {
      g_free (entry->label);
      entry->label = g_strdup (bse_item_get_name_or_type (item));
      gxk_assortment_changed (self->ppages, entry);
    }
}

static void
ppage_item_free (gpointer user_data,
                 GObject *object,
                 gpointer owner)
{
  BstApp *self = BST_APP (owner);
  SfiProxy item = (SfiProxy) user_data;
  bse_proxy_disconnect (item, "any-signal::property-notify::uname", app_update_page_item, self, NULL);
  if (GTK_IS_WIDGET (object))
    gtk_widget_destroy (GTK_WIDGET (object));
  bse_item_unuse (item);
}

static void
bst_app_add_page_item (BstApp  *self,
                       guint    position,
                       SfiProxy item)
{
  const gchar *stock, *name = bse_item_get_name_or_type (item);
  bse_item_use (item);
  bse_proxy_connect (item, "signal::property-notify::uname", app_update_page_item, self, NULL);
  gchar *tip;
  if (BSE_IS_WAVE_REPO (item))
    {
      name = _("Waves");
      stock = BST_STOCK_MINI_WAVE_REPO;
      tip = g_strdup (_("Wave Repository"));
    }
  else if (BSE_IS_SONG (item))
    {
      stock = BST_STOCK_MINI_SONG;
      tip = g_strdup_printf (_("Song: %s"), name);
    }
  else if (BSE_IS_MIDI_SYNTH (item))
    {
      stock = BST_STOCK_MINI_MIDI_SYNTH;
      tip = g_strdup_printf (_("MIDI Synthesizer: %s"), name);
    }
  else
    {
      stock = BST_STOCK_MINI_CSYNTH;
      tip = g_strdup_printf (_("Synthesizer: %s"), name);
    }
  GtkWidget *page = NULL;
  if (BSE_IS_SUPER (item))
    {
      page = g_object_new (BST_TYPE_SUPER_SHELL, "super", item, NULL);
      g_object_ref (page);
      gtk_object_sink (GTK_OBJECT (page));
    }
  gxk_assortment_insert (self->ppages, position, name, stock, tip, (void*) item, (GObject*) page, self, ppage_item_free);
  if (page)
    g_object_unref (page);
  g_free (tip);
}

static gint
proxyp_cmp_items (gconstpointer v1,
                  gconstpointer v2,
                  gpointer      data)
{
  const SfiProxy *p1 = v1;
  const SfiProxy *p2 = v2;
  if (*p1 == *p2)
    return 0;
  return proxy_rate_item (*p1) - proxy_rate_item (*p2);
}

static void
bst_app_reload_pages (BstApp *self)
{
  g_return_if_fail (BST_IS_APP (self));
  
  GtkWidget *old_focus = GTK_WINDOW (self)->focus_widget;
  if (old_focus)
    gtk_widget_ref (old_focus);
  SfiProxy old_item = self->ppages->selected ? (SfiProxy) self->ppages->selected->user_data : 0;

  /* collect page objects */
  BseItemSeq *iseq = bse_project_get_supers (self->project);
  SfiRing *ring, *proxies = NULL;
  guint i;
  for (i = 0; i < iseq->n_items; i++)
    if (!bse_item_internal (iseq->items[i]) || BST_DBG_EXT)
      proxies = sfi_ring_append (proxies, iseq->items + i);
  /* sort proxies */
  proxies = sfi_ring_sort (proxies, proxyp_cmp_items, NULL);

  /* remove outdated project pages */
  SfiRing *outdated = NULL;
  GSList *slist;
  for (slist = self->ppages->entries; slist; slist = slist->next)
    {
      GxkAssortmentEntry *entry = slist->data;
      for (ring = proxies; ring; ring = sfi_ring_next (ring, proxies))
        {
          SfiProxy *pp = ring->data, item = *pp;
          if (item == (SfiProxy) entry->user_data)
            break;
        }
      if (!ring)
        outdated = sfi_ring_append (outdated, entry);
    }
  while (outdated)
    {
      GxkAssortmentEntry *entry = sfi_ring_pop_head (&outdated);
      gxk_assortment_remove (self->ppages, entry);
    }

  /* add missing project pages */
  SfiProxy first_unseen = 0, first_synth = 0;
  i = 0;
  for (ring = proxies; ring; ring = sfi_ring_next (ring, proxies), i++)
    {
      SfiProxy *pp = ring->data, item = *pp;
      if (!gxk_assortment_find_data (self->ppages, (void*) item))
        {
          bst_app_add_page_item (self, i, item);
          if (!first_unseen)
            first_unseen = item;
        }
      if (!first_synth && BSE_IS_SNET (item))
        first_synth = item;
    }

  /* select/restore current page */
  if (first_unseen && self->select_unseen_super)
    gxk_assortment_select_data (self->ppages, (void*) first_unseen);
  else if (old_item && gxk_assortment_find_data (self->ppages, (void*) old_item))
    gxk_assortment_select_data (self->ppages, (void*) old_item);
  else if (first_synth)
    gxk_assortment_select_data (self->ppages, (void*) first_synth);
  self->select_unseen_super = FALSE;
  /* restore focus */
  if (old_focus)
    {
      if (gxk_widget_ancestry_viewable (old_focus) &&
          gtk_widget_get_toplevel (old_focus) == GTK_WIDGET (self))
        gtk_widget_grab_focus (old_focus);
      gtk_widget_unref (old_focus);
    }
}

static gboolean
bst_app_handle_delete_event (GtkWidget   *widget,
                             GdkEventAny *event)
{
  BstApp *self = BST_APP (widget);
  if (bse_project_is_dirty (self->project))
    {
      guint result = bst_msg_dialog (BST_MSG_WARNING,
                                     BST_MSG_TITLE (_("Close %s"), bse_item_get_name (self->project)),
                                     BST_MSG_TEXT1 (_("The project has been modified.")),
                                     BST_MSG_TEXT2 (_("Changes were made to project \"%s\" since the last time it was saved to disk."),
                                                    bse_item_get_name (self->project)),
                                     BST_MSG_TEXT2 (_("Save the project before closing its window?")),
                                     BST_MSG_CHOICE   (2, _("Save Changes"), BST_STOCK_SAVE),
                                     BST_MSG_CHOICE   (1, _("Discard Changes"), BST_STOCK_DELETE),
                                     BST_MSG_CHOICE_D (0, _("Cancel"), BST_STOCK_CANCEL));
      if (result == 1)
        gtk_widget_destroy (widget);
      else if (result == 2)
        {
          GtkWidget *fdialog = bst_file_dialog_popup_save_project (self, self->project, FALSE, TRUE);
          if (!fdialog)
            gtk_widget_destroy (widget);
        }
    }
  else
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

typedef struct {
  gchar *file;
  gchar *name;
} DemoEntry;

static DemoEntry *demo_entries = NULL;
static guint      n_demo_entries = 0;

static int
demo_entries_compare (const void *v1,
                      const void *v2)
{
  const DemoEntry *d1 = v1;
  const DemoEntry *d2 = v2;
  return strcmp (d1->file, d2->file);
}

static void
demo_entries_setup (void)
{
  if (!demo_entries)
    {
      SfiRing *files = sfi_file_crawler_list_files (bse_server_get_demo_path (BSE_SERVER), "*.bse", 0);
      while (files)
        {
          gchar *file = sfi_ring_pop_head (&files);
          gchar *name = bst_file_scan_find_key (file, "container-child", "BseSong::");
          if (!name)
            name = bst_file_scan_find_key (file, "container-child", "BseMidiSynth::"); // FIXME
          if (!name)
            name = bst_file_scan_find_key (file, "container-child", "BseCSynth::"); // FIXME
          if (name && n_demo_entries < 0xffff)
            {
              guint i = n_demo_entries++;
              demo_entries = g_renew (DemoEntry, demo_entries, n_demo_entries);
              demo_entries[i].file = file;
              demo_entries[i].name = name;
            }
          else
            {
              g_free (name);
              g_free (file);
            }
        }
      qsort (demo_entries, n_demo_entries, sizeof (demo_entries[0]), demo_entries_compare);
    }
}

static void
demo_play_song (gpointer data,
                gulong   callback_action)
{
  const gchar *file_name = demo_entries[callback_action - BST_ACTION_LOAD_DEMO_0000].file;
  SfiProxy project = bse_server_use_new_project (BSE_SERVER, file_name);
  BseErrorType error = bst_project_restore_from_file (project, file_name, TRUE);
  if (error)
    bst_status_eprintf (error, _("Opening project `%s'"), file_name);
  else
    {
      BstApp *app;
      bse_project_get_wave_repo (project);
      app = bst_app_new (project);
      gxk_status_window_push (app);
      bst_status_eprintf (error, _("Opening project `%s'"), file_name);
      gxk_status_window_pop ();
      gxk_idle_show_widget (GTK_WIDGET (app));
    }
  bse_item_unuse (project);
}

static GxkActionList*
demo_entries_create (BstApp *app)
{
  GxkActionList *alist = gxk_action_list_create ();
  guint i;
  demo_entries_setup ();
  for (i = 0; i < n_demo_entries; i++)
    gxk_action_list_add_translated (alist, demo_entries[i].name, demo_entries[i].name,
                                    NULL, NULL, BST_ACTION_LOAD_DEMO_0000 + i, BST_STOCK_NOTE_ICON,
                                    NULL, demo_play_song, app);
  return alist;
}

static DemoEntry *skin_entries = NULL;
static guint     n_skin_entries = 0;

static void
skin_entries_setup (void)
{
  if (!skin_entries)
    {
      gchar *skindirs = BST_STRDUP_SKIN_PATH ();
      SfiRing *files = sfi_file_crawler_list_files (skindirs, "*.skin", 0);
      g_free (skindirs);
      while (files)
        {
          gchar *file = sfi_ring_pop_head (&files);
          gchar *name = bst_file_scan_find_key (file, "skin-name", "");
          static guint statici = 1;
          if (!name)
            name = g_strdup_printf ("skin-%u", statici++);
          if (name && n_skin_entries < 0xffff)
            {
              guint i = n_skin_entries++;
              skin_entries = g_renew (DemoEntry, skin_entries, n_skin_entries);
              skin_entries[i].file = file;
              skin_entries[i].name = name;
            }
          else
            {
              g_free (name);
              g_free (file);
            }
        }
    }
}

static void
load_skin (gpointer data,
           gulong   callback_action)
{
  const gchar *file_name = skin_entries[callback_action - BST_ACTION_LOAD_SKIN_0000].file;
  BseErrorType error = bst_skin_parse (file_name);
  bst_status_eprintf (error, _("Loading skin `%s'"), file_name);
}

static GxkActionList*
skin_entries_create (BstApp *app)
{
  GxkActionList *alist = gxk_action_list_create ();
  guint i;
  skin_entries_setup ();
  for (i = 0; i < n_skin_entries; i++)
    gxk_action_list_add_translated (alist, skin_entries[i].name, skin_entries[i].name,
                                    NULL, NULL, BST_ACTION_LOAD_SKIN_0000 + i, BST_STOCK_BROWSE_IMAGE,
                                    NULL, load_skin, app);
  return alist;
}

static void
bst_app_run_script_proc (gpointer data,
                         gulong   category_id)
{
  BstApp *self = BST_APP (data);
  BseCategory *cat = bse_category_from_id (category_id);
  SfiProxy super = bst_app_get_current_super (self);
  const gchar *song = "", *wave_repo = "", *snet = "", *csynth = "";
  
  if (BSE_IS_SONG (super))
    song = "song";
  else if (BSE_IS_WAVE_REPO (super))
    wave_repo = "wave-repo";
  else if (BSE_IS_SNET (super))
    {
      snet = "synth-net";
      if (BSE_IS_CSYNTH (super))
        csynth = "custom-synth";
    }
  
  bst_procedure_exec_auto (cat->type,
                           "project", SFI_TYPE_PROXY, self->project,
                           song, SFI_TYPE_PROXY, super,
                           wave_repo, SFI_TYPE_PROXY, super,
                           snet, SFI_TYPE_PROXY, super,
                           csynth, SFI_TYPE_PROXY, super,
                           NULL);
}

void
bst_app_show_release_notes (BstApp *app)
{
  if (app_action_check (app, ACTION_HELP_RELEASE_NOTES, gxk_action_inc_cache_stamp()))
    app_action_exec (app, ACTION_HELP_RELEASE_NOTES);
}

static void
app_action_exec (gpointer data,
                 gulong   action)
{
  static GtkWidget *bst_help_dialogs[ACTION_HELP_LAST - ACTION_HELP_FIRST + 1] = { NULL, };
  static GtkWidget *bst_preferences = NULL;
  BstApp *self = BST_APP (data);
  gchar *help_file = NULL, *help_title = NULL;
  GtkWidget *widget = GTK_WIDGET (self);
  
  gxk_status_window_push (widget);
  
  switch (action)
    {
      SfiProxy proxy;
      GtkWidget *any;
      guint demo_type;
    case BST_ACTION_EXIT:
      if (bst_app_class)
        {
          GSList *slist, *free_slist = g_slist_copy (bst_app_class->apps);
          
          for (slist = free_slist; slist; slist = slist->next)
            gxk_toplevel_delete (slist->data);
          g_slist_free (free_slist);
        }
      break;
    case BST_ACTION_CLOSE_PROJECT:
      gxk_toplevel_delete (widget);
      break;
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
    case BST_ACTION_IMPORT_MIDI:
      bst_file_dialog_popup_import_midi (self, self->project);
      break;
    case BST_ACTION_SAVE_PROJECT:
      bst_file_dialog_popup_save_project (self, self->project, FALSE, TRUE);
      break;
    case BST_ACTION_SAVE_PROJECT_AS:
      bst_file_dialog_popup_save_project (self, self->project, TRUE, TRUE);
      break;
    case BST_ACTION_MERGE_EFFECT:
      bst_file_dialog_popup_merge_effect (self, self->project);
      self->select_unseen_super = TRUE;
      break;
    case BST_ACTION_MERGE_INSTRUMENT:
      bst_file_dialog_popup_merge_instrument (self, self->project);
      self->select_unseen_super = TRUE;
      break;
    case BST_ACTION_SAVE_EFFECT:
      bst_file_dialog_popup_save_effect (self, self->project, bst_app_get_current_super (self));
      break;
    case BST_ACTION_SAVE_INSTRUMENT:
      bst_file_dialog_popup_save_instrument (self, self->project, bst_app_get_current_super (self));
      break;
    case BST_ACTION_NEW_SONG:
      bse_item_group_undo (self->project, "Create Song");
      proxy = bse_project_create_song (self->project, NULL);
      bse_song_ensure_master_bus (proxy);
      bse_item_ungroup_undo (self->project);
      self->select_unseen_super = TRUE;
      break;
    case BST_ACTION_NEW_CSYNTH:
      proxy = bse_project_create_csynth (self->project, NULL);
      self->select_unseen_super = TRUE;
      break;
    case BST_ACTION_NEW_MIDI_SYNTH:
      proxy = bse_project_create_midi_synth (self->project, NULL);
      self->select_unseen_super = TRUE;
      break;
    case BST_ACTION_REMOVE_SYNTH:
      proxy = bst_app_get_current_super (self);
      if (BSE_IS_SNET (proxy) && !bse_project_is_active (self->project))
        bse_project_remove_snet (self->project, proxy);
      self->select_unseen_super = FALSE;
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
    case ACTION_RACK_EDITOR:
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
                                              _("Rack editor"),
                                              self->rack_editor);
        }
      gxk_widget_showraise (self->rack_dialog);
      break;
    case ACTION_SHOW_PREFERENCES:
      if (!bst_preferences)
        {
          GtkWidget *widget = g_object_new (BST_TYPE_PREFERENCES,
                                            "visible", TRUE,
                                            NULL);
          bst_preferences = gxk_dialog_new (&bst_preferences,
                                            NULL,
                                            GXK_DIALOG_HIDE_ON_DELETE,
                                            _("Preferences"),
                                            widget);
          bst_preferences_create_buttons (BST_PREFERENCES (widget), GXK_DIALOG (bst_preferences));
        }
      if (!GTK_WIDGET_VISIBLE (bst_preferences))
        bst_preferences_revert (BST_PREFERENCES (gxk_dialog_get_child (GXK_DIALOG (bst_preferences))));
      gxk_widget_showraise (bst_preferences);
      break;
    case ACTION_EXTRA_VIEW:
      any = (GtkWidget*) bst_app_new (self->project);
      gxk_idle_show_widget (any);
      break;
    case ACTION_SHOW_PROFILER:
      any = bst_profiler_window_get ();
      gxk_idle_show_widget (any);
      break;
    case ACTION_SHOW_PROC_BROWSER:
#if 0 // FIXME
      if (!bst_proc_browser)
        {
          GtkWidget *widget;
          
          widget = bst_proc_browser_new ();
          gtk_widget_show (widget);
          bst_proc_browser = gxk_dialog_new (&bst_proc_browser,
                                             NULL,
                                             GXK_DIALOG_HIDE_ON_DELETE,
                                             _("Procedure Browser"),
                                             widget);
          bst_proc_browser_create_buttons (BST_PROC_BROWSER (widget), GXK_DIALOG (bst_proc_browser));
        }
      gxk_idle_show_widget (bst_proc_browser);
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
    case ACTION_HELP_INDEX:
      help_file = g_strconcat (BST_PATH_DOCS, "/beast-index.markup", NULL);
      help_title = g_strdup (help_file);
      goto HELP_DIALOG;
    case ACTION_HELP_FAQ:
      help_file = g_strconcat (BST_PATH_DOCS, "/faq.markup", NULL);
      help_title = g_strdup (help_file);
      goto HELP_DIALOG;
    case ACTION_HELP_QUICK_START:
      help_file = g_strconcat (BST_PATH_DOCS, "/quickstart.markup", NULL);
      help_title = g_strdup (help_file);
      goto HELP_DIALOG;
    case ACTION_HELP_RELEASE_NOTES:
      help_file = g_strconcat (BST_PATH_DOCS, "/release-notes.markup", NULL);
      help_title = g_strdup_printf (_("BEAST-%s Release Notes"), BST_VERSION);
      goto HELP_DIALOG;
    case ACTION_HELP_DSP_ENGINE:
      help_file = g_strconcat (BST_PATH_DOCS, "/engine-mplan.markup", NULL);
      help_title = g_strdup (help_file);
      goto HELP_DIALOG;
    case ACTION_HELP_PLUGIN_DEVEL:
      help_file = g_strconcat (BST_PATH_DOCS, "/plugin-devel.markup", NULL);
      help_title = g_strdup (help_file);
      goto HELP_DIALOG;
    case ACTION_HELP_DEVELOPMENT:
      help_file = g_strconcat (BST_PATH_DOCS, "/beast-index.markup#development", NULL);
      help_title = g_strdup (help_file);
      goto HELP_DIALOG;
    HELP_DIALOG:
      if (!bst_help_dialogs[action - ACTION_HELP_FIRST])
        {
          GtkWidget *sctext = gxk_scroll_text_create (GXK_SCROLL_TEXT_NAVIGATABLE, NULL);
          gchar *index = g_strconcat ("file://", BST_PATH_DOCS, "/beast-index.markup", NULL);
          gxk_scroll_text_set_index (sctext, index);
          g_free (index);
          gxk_scroll_text_enter (sctext, help_file);
          bst_help_dialogs[action - ACTION_HELP_FIRST] = gxk_dialog_new (&bst_help_dialogs[action - ACTION_HELP_FIRST],
                                                                         NULL,
                                                                         GXK_DIALOG_HIDE_ON_DELETE | GXK_DIALOG_DELETE_BUTTON,
                                                                         help_title, sctext);
          gxk_dialog_set_sizes (GXK_DIALOG (bst_help_dialogs[action - ACTION_HELP_FIRST]), 500, 400, 560, 640);
        }
      g_free (help_file);
      g_free (help_title);
      gxk_scroll_text_rewind (gxk_dialog_get_child (GXK_DIALOG (bst_help_dialogs[action - ACTION_HELP_FIRST])));
      gxk_widget_showraise (bst_help_dialogs[action - ACTION_HELP_FIRST]);
      break;
    case ACTION_HELP_ABOUT:
      beast_show_about_box ();
      break;
    case ACTION_URL_HELP_DESK:
      gxk_show_url ("http://beast.gtk.org/wiki:HelpDesk");
      break;
    case ACTION_URL_BEAST_SITE:
      gxk_show_url ("http://beast.gtk.org/");
      break;
    case ACTION_URL_ONLINE_SYNTHESIZERS:
      gxk_show_url ("http://beast.gtk.org/browse-bse-files.html");
      break;
    case ACTION_URL_ONLINE_DEMOS:
      gxk_show_url ("http://beast.gtk.org/browse-bse-files.html");
      break;
    case ACTION_DEMO_DIALOG_FATAL:
    case ACTION_DEMO_DIALOG_ERROR:
    case ACTION_DEMO_DIALOG_WARNING:
    case ACTION_DEMO_DIALOG_SCRIPT:
    case ACTION_DEMO_DIALOG_INFO:
    case ACTION_DEMO_DIALOG_DIAG:
    case ACTION_DEMO_DIALOG_DEBUG:
      switch (action)
        {
        default: /* silence compiler */
        case ACTION_DEMO_DIALOG_FATAL:   demo_type = BST_MSG_FATAL;   break;
        case ACTION_DEMO_DIALOG_ERROR:   demo_type = BST_MSG_ERROR;   break;
        case ACTION_DEMO_DIALOG_WARNING: demo_type = BST_MSG_WARNING; break;
        case ACTION_DEMO_DIALOG_SCRIPT:  demo_type = BST_MSG_SCRIPT;  break;
        case ACTION_DEMO_DIALOG_INFO:    demo_type = BST_MSG_INFO;    break;
        case ACTION_DEMO_DIALOG_DIAG:    demo_type = BST_MSG_DIAG;    break;
        case ACTION_DEMO_DIALOG_DEBUG:   demo_type = BST_MSG_DEBUG;   break;
        }
      bst_msg_dialog (demo_type,
                      SFI_MSG_TEXT0 ("Demonstration Dialog"),
                      SFI_MSG_TEXT1 ("This is a demonstrative dialog"),
                      SFI_MSG_TEXT2 ("To help with dialog layout, and to test message display, dialogs may be "
                                     "fired up for pure demonstration purposes. This is such a dialog, so if you "
                                     "are currently looking at a prominent warning or error message, there's no "
                                     "real merit to it."),
                      SFI_MSG_TEXT3 ("Demo-Dialog-Type: %s", sfi_msg_type_label (demo_type)));
      break;
    default:
      g_assert_not_reached ();
      break;
    }
  
  gxk_status_window_pop ();
  
  gxk_widget_update_actions_downwards (self);
}

static gboolean
app_action_check (gpointer data,
                  gulong   action,
                  guint64  action_stamp)
{
  BstApp *self = BST_APP (data);
  if (!self->project)
    return FALSE;
  switch (action)
    {
      SfiProxy super;
      BseItemSeq *iseq;
      guint i;
    case BST_ACTION_NEW_PROJECT:
    case BST_ACTION_OPEN_PROJECT:
    case BST_ACTION_MERGE_PROJECT:
    case BST_ACTION_IMPORT_MIDI:
    case BST_ACTION_SAVE_PROJECT_AS:
    case BST_ACTION_NEW_CSYNTH:
    case BST_ACTION_NEW_MIDI_SYNTH:
    case BST_ACTION_CLOSE_PROJECT:
      return TRUE;
    case BST_ACTION_SAVE_PROJECT:
      return bse_project_is_dirty (self->project);
    case BST_ACTION_NEW_SONG:
      iseq = bse_container_list_children (self->project);
      for (i = 0; i < iseq->n_items; i++)
        if (BSE_IS_SONG (iseq->items[i]))
          return FALSE;
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
      return TRUE;
    case BST_ACTION_START_PLAYBACK:
      if (self->project && bse_project_can_play (self->project))
        return TRUE;
      return FALSE;
    case BST_ACTION_STOP_PLAYBACK:
      if (self->project && bse_project_is_playing (self->project))
        return TRUE;
      return FALSE;
    case ACTION_RACK_EDITOR:
    case ACTION_SHOW_PROC_BROWSER:
      return FALSE;
    case ACTION_SHOW_PREFERENCES:
    case ACTION_EXTRA_VIEW:
    case ACTION_SHOW_PROFILER:
      return TRUE;
    case BST_ACTION_MERGE_EFFECT:
    case BST_ACTION_MERGE_INSTRUMENT:
      return !bse_project_is_active (self->project);
    case BST_ACTION_SAVE_EFFECT:
    case BST_ACTION_SAVE_INSTRUMENT:
      super = bst_app_get_current_super (self);
      return BSE_IS_CSYNTH (super) && !bse_project_is_active (self->project);
    case ACTION_HELP_INDEX:
    case ACTION_HELP_RELEASE_NOTES:
    case ACTION_HELP_QUICK_START:
    case ACTION_HELP_FAQ:
    case ACTION_HELP_DSP_ENGINE:
    case ACTION_HELP_PLUGIN_DEVEL:
    case ACTION_HELP_DEVELOPMENT:
    case ACTION_HELP_ABOUT:
    case ACTION_URL_HELP_DESK:
    case ACTION_URL_BEAST_SITE:
    case ACTION_URL_ONLINE_SYNTHESIZERS:
    case ACTION_URL_ONLINE_DEMOS:
    case ACTION_DEMO_DIALOG_FATAL:
    case ACTION_DEMO_DIALOG_ERROR:
    case ACTION_DEMO_DIALOG_WARNING:
    case ACTION_DEMO_DIALOG_SCRIPT:
    case ACTION_DEMO_DIALOG_INFO:
    case ACTION_DEMO_DIALOG_DIAG:
    case ACTION_DEMO_DIALOG_DEBUG:
      return TRUE;
    case BST_ACTION_EXIT:
      /* abuse generic "Exit" update to sync Tools menu items */
      super = bst_app_get_current_super (self);
      gxk_radget_sensitize (self, "song-submenu", BSE_IS_SONG (super));
      gxk_radget_sensitize (self, "synth-submenu", BSE_IS_SNET (super) && !BSE_IS_SONG (super));
      gxk_radget_sensitize (self, "waves-submenu", BSE_IS_WAVE_REPO (super));
      return TRUE;
    default:
      g_warning ("BstApp: unknown action: %lu", action);
      return FALSE;
    }
}

static void
bst_app_class_init (BstAppClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  
  bst_app_class = class;
  
  gobject_class->finalize = bst_app_finalize;

  object_class->destroy = bst_app_destroy;
  
  widget_class->delete_event = bst_app_handle_delete_event;
  
  class->apps = NULL;
}
