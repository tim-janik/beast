// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstapp.hh"
#include "bstparam.hh"
#include "bstskinconfig.hh"
#include "bstsupershell.hh"
#include "bstfiledialog.hh"
#include "bstgconfig.hh"
#include "bstpreferences.hh"
#include "bstservermonitor.hh"
#include "bstmenus.hh"
#include "bstprojectctrl.hh"
#include "bstprofiler.hh"
#include "bstusermessage.hh"
#include "bse/internal.hh"
#include <string.h>
#include <algorithm>

/* --- prototypes --- */
static GxkActionList* demo_entries_create         (BstApp      *app);
static GxkActionList* skin_entries_create         (BstApp      *app);
static void           app_action_exec             (gpointer     data,
                                                   size_t       action);
static gboolean       app_action_check            (gpointer     data,
                                                   size_t       action,
                                                   guint64      action_stamp);
static void           bst_app_reload_pages        (BstApp      *self);

/* --- menus --- */
enum {
  ACTION_INTERNALS = BST_ACTION_APP_LAST,
  /* dialogs */
  ACTION_SHOW_PREFERENCES,
  ACTION_SHOW_PROFILER,
  ACTION_EXTRA_VIEW,
#define ACTION_HELP_FIRST   ACTION_HELP_MANUAL
  ACTION_HELP_MANUAL,
  ACTION_HELP_ABOUT,
#define ACTION_HELP_LAST    ACTION_HELP_ABOUT
  ACTION_URL_BEAST_SITE,
  ACTION_URL_REPORT_BUG,
  ACTION_URL_ONLINE_SYNTHESIZERS,
  ACTION_URL_ONLINE_DEMOS,
  ACTION_DEMO_DIALOG_ERROR,
  ACTION_DEMO_DIALOG_WARNING,
  ACTION_DEMO_DIALOG_INFO,
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
  { N_("_Save"),                "<ctrl>S",      N_("Write project to disk"),
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
  { N_("Profiler"),             NULL,           N_("Display statistics and timing information"),
    ACTION_SHOW_PROFILER, },
  { N_("New View"),             NULL,           N_("Create an extra view of the project"),
    ACTION_EXTRA_VIEW, },
};
static const GxkStockAction playback_actions[] = {
  { N_("_Play"),                "<ctrl>P",      N_("Play or restart playback of the project"),
    BST_ACTION_START_PLAYBACK,  BST_STOCK_PLAY },
  { N_("_Stop"),                "<ctrl>D",      N_("Stop playback of the project"),
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
  { N_("_Beast Manual..."),     NULL,           N_("Browse the Beast manual, covering Tutorials, Howotos and in-depth knowledge on Beast"),
    ACTION_HELP_MANUAL,          BST_STOCK_DOC_INDEX },
  { N_("_Beast Website..."),    NULL,           N_("Start a web browser pointing to the Beast website"),
    ACTION_URL_BEAST_SITE,      BST_STOCK_ONLINE_BEAST_SITE },
  { N_("Report a Beast Bug..."),NULL,           N_("Start a web browser with the bug report form for the BEAST bugzilla product"),
    ACTION_URL_REPORT_BUG,      BST_STOCK_ONLINE_BUGS },
#if 0
  { N_("Developing Plugins..."),NULL,           N_("A guide to synthesis plugin development"),
    ACTION_HELP_PLUGIN_DEVEL,   BST_STOCK_DOC_DEVEL },
  { N_("DSP Engine..."),        NULL,           N_("Technical description of the multi-threaded synthesis engine innards"),
    ACTION_HELP_DSP_ENGINE,     BST_STOCK_DOC_DEVEL },
  { N_("Development..."),       NULL,           N_("Provide an overview of development related topics and documents"),
    ACTION_HELP_DEVELOPMENT,    BST_STOCK_DOC_DEVEL },
#endif
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
  { "Demo Error Dialog",        NULL,           "Fire up an error dialog for demonstration purposes",
    ACTION_DEMO_DIALOG_ERROR,   BST_STOCK_ERROR },
  { "Demo Warning Dialog",      NULL,           "Fire up a warning dialog for demonstration purposes",
    ACTION_DEMO_DIALOG_WARNING, BST_STOCK_WARNING },
  { "Demo Info Dialog",         NULL,           "Fire up an information dialog for demonstration purposes",
    ACTION_DEMO_DIALOG_INFO,    BST_STOCK_INFO },
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
  new (&self->project) Bse::ProjectS();
  GtkWidget *widget = GTK_WIDGET (self);
  GxkActionList *al1;

  g_object_set (self,
                "name", "BEAST-Application",
                "allow_shrink", TRUE,
                "allow_grow", TRUE,
                "flags", GXK_DIALOG_STATUS_BAR | GXK_DIALOG_IGNORE_ESCAPE, // | GXK_DIALOG_WINDOW_GROUP,
                NULL);
  bst_app_register (self);
  self->box = gxk_radget_create ("beast", "application-box", NULL);
  gtk_container_add (GTK_CONTAINER (GXK_DIALOG (self)->vbox), (GtkWidget*) self->box);

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
  /* add demo songs */
  al1 = demo_entries_create (self);
  gxk_widget_publish_action_list (widget, "demo-songs", al1);
  /* add skins */
  al1 = skin_entries_create (self);
  gxk_action_list_sort (al1);
  gxk_widget_publish_action_list (widget, "skin-options", al1);

  /* setup playback controls */
  self->pcontrols = (GtkWidget*) g_object_new (BST_TYPE_PROJECT_CTRL, NULL);
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
  self->notebook = (GtkNotebook*) gxk_radget_find (self->box, "main-notebook");
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
        bst_project_ctrl_set_project (BST_PROJECT_CTRL (self->pcontrols), Bse::ProjectH());
      self->project.deactivate();
      bse_server.destroy_project (self->project);
      self->project = Bse::ProjectH(); // NULL
    }

  if (self->ppages)
    gxk_assortment_dispose (self->ppages);

  bst_app_unregister (self);

  GTK_OBJECT_CLASS (bst_app_parent_class)->destroy (object);

  if (!bst_app_class->apps && bst_app_class->seen_apps)
    {
      bst_app_class->seen_apps = FALSE;
      Bst::event_loop_quit ();
    }
}

static void
bst_app_finalize (GObject *object)
{
  BstApp *self = BST_APP (object);

  if (self->project)
    {
      self->project = NULL;
    }
  if (self->ppages)
    {
      gxk_assortment_dispose (self->ppages);
      g_object_unref (self->ppages);
      self->ppages = NULL;
    }

  G_OBJECT_CLASS (bst_app_parent_class)->finalize (object);
  using namespace Bse;
  self->project.~ProjectS();
}

BstApp*
bst_app_new (Bse::ProjectH project)
{
  assert_return (project, NULL);

  BstApp *self = (BstApp*) g_object_new (BST_TYPE_APP, NULL);
  gxk_dialog_set_sizes (GXK_DIALOG (self), 500, 400, 950, 800);

  self->project = project;
  self->project.on ("treechange", [self] () { bst_app_reload_pages (self); });
  self->project.on ("statechanged", [self] () { gxk_widget_update_actions (self); });
  self->project.on ("notify:dirty", [self] () { gxk_widget_update_actions (self); });

  bst_window_sync_title_to_proxy (GXK_DIALOG (self), self->project.proxy_id(), "%s");
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

  assert_return (BSE_IS_PROJECT (project), NULL);

  for (slist = bst_app_class->apps; slist; slist = slist->next)
    {
      BstApp *app = (BstApp*) slist->data;

      if (app->project.proxy_id() == int64_t (project))
        return app;
    }
  return NULL;
}

static SfiProxy
bst_app_get_current_super (BstApp *app)
{
  assert_return (BST_IS_APP (app), 0);
  if (app->notebook && app->notebook->cur_page)
    {
      GtkWidget *shell = gtk_notebook_current_widget (app->notebook);
      if (BST_IS_SUPER_SHELL (shell))
        {
          BstSuperShell *super_shell = BST_SUPER_SHELL (shell);
          return super_shell->super.proxy_id();
        }
    }
  return 0;
}

struct AppPage {
  Bse::SuperS super;
  AppPage (Bse::SuperH sp) :
    super (sp)
  {
    if (super)
      super.use();
  }
  ~AppPage()
  {
    if (super)
      super.unuse();
  }
};

static void
bst_app_add_page_item (BstApp *self, uint position, SfiProxy itemid)
{
  Bse::SuperH super = Bse::SuperH::__cast__ (bse_server.from_proxy (itemid));
  const gchar *stock;
  String name = super.get_name_or_type();
  AppPage *apage = new AppPage (super);
  auto uname_change = [self, apage] () {
    GxkAssortmentEntry *entry = gxk_assortment_find_data (self->ppages, apage);
    return_unless (entry);
    g_free (entry->label);
    entry->label = g_strdup (apage->super.get_name_or_type().c_str());
    gxk_assortment_changed (self->ppages, entry);
  };
  apage->super.on ("notify:uname", uname_change);
  String tip;
  if (BSE_IS_WAVE_REPO (itemid))
    {
      name = _("Waves");
      stock = BST_STOCK_MINI_WAVE_REPO;
      tip = _("Wave Repository");
    }
  else if (BSE_IS_SONG (itemid))
    {
      stock = BST_STOCK_MINI_SONG;
      tip = string_format (_("Song: %s"), name);
    }
  else if (BSE_IS_MIDI_SYNTH (itemid))
    {
      stock = BST_STOCK_MINI_MIDI_SYNTH;
      tip = string_format (_("MIDI Synthesizer: %s"), name);
    }
  else
    {
      stock = BST_STOCK_MINI_CSYNTH;
      tip = string_format (_("Synthesizer: %s"), name);
    }
  GtkWidget *page = NULL;
  if (BSE_IS_SUPER (itemid))
    {
      page = (GtkWidget*) g_object_new (BST_TYPE_SUPER_SHELL, "super", itemid, NULL);
      g_object_ref (page);
      gtk_object_sink (GTK_OBJECT (page));
    }
  gxk_assortment_insert (self->ppages, position, name.c_str(), stock, tip.c_str(), apage, (GObject*) page, self,
                         [] (void *data, GObject *object, gpointer owner) {
                           if (GTK_IS_WIDGET (object))
                             gtk_widget_destroy (GTK_WIDGET (object));
                           delete (AppPage*) data;
                         });
  uname_change();
  if (page)
    g_object_unref (page);
}

static int
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

static bool
super_rating_lesser (Bse::SuperH &a, Bse::SuperH &b)
{
  return proxy_rate_item (a.proxy_id()) < proxy_rate_item (b.proxy_id());
}

static std::function<bool (void*)>
app_page_predicate (Bse::ItemH item)
{
  return [item] (void *ap) { return ((AppPage*) ap)->super == item; };
}

static void
bst_app_reload_pages (BstApp *self)
{
  assert_return (BST_IS_APP (self));

  GtkWidget *old_focus = GTK_WINDOW (self)->focus_widget;
  if (old_focus)
    gtk_widget_ref (old_focus);
  Bse::SuperH old_super = self->ppages->selected ? ((AppPage*) self->ppages->selected->user_data)->super : Bse::SuperH();

  // collect Super objects that need their own view
  Bse::SuperSeq sseq = self->project.get_supers();
  // sort out internal objects
  for (Bse::SuperSeq::iterator it = sseq.begin(); it != sseq.end();)
    if (!BST_DBG_EXT && it->internal())
      it = sseq.erase (it);
    else
      ++it;
  // sort Super objects according to UI display preference
  std::sort (sseq.begin(), sseq.end(), super_rating_lesser);

  // remove views for pruned Super objects
  SfiRing *outdated = NULL;
  for (GSList *slist = self->ppages->entries; slist; slist = slist->next)
    {
      GxkAssortmentEntry *entry = (GxkAssortmentEntry*) slist->data;
      Bse::SuperH found, super = ((AppPage*) entry->user_data)->super;
      for (auto &candidate : sseq)
        if (candidate.proxy_id() == super.proxy_id())
          found = candidate;
      if (!found)
        outdated = sfi_ring_append (outdated, entry);
    }
  while (outdated)
    {
      GxkAssortmentEntry *entry = (GxkAssortmentEntry*) sfi_ring_pop_head (&outdated);
      gxk_assortment_remove (self->ppages, entry);
    }

  // create views for new Super objects
  SfiProxy first_unseen = 0, first_synth = 0;
  size_t pos = 0;
  for (auto &super : sseq)
    {
      SfiProxy view_proxy = super.proxy_id();
      if (!gxk_assortment_find_pred (self->ppages, app_page_predicate (super)))
        {
          bst_app_add_page_item (self, pos, view_proxy);
          if (!first_unseen)
            first_unseen = view_proxy;
        }
      if (!first_synth && BSE_IS_SNET (view_proxy))
        first_synth = view_proxy;
      pos++;
    }

  // re-select current page
  if (first_unseen && self->select_unseen_super)
    gxk_assortment_select_pred (self->ppages, app_page_predicate (Bse::SuperH::__cast__ (bse_server.from_proxy (first_unseen))));
  else if (old_super && gxk_assortment_find_pred (self->ppages, app_page_predicate (old_super)))
    gxk_assortment_select_pred (self->ppages, app_page_predicate (old_super));
  else if (first_synth)
    gxk_assortment_select_pred (self->ppages, app_page_predicate (Bse::SuperH::__cast__ (bse_server.from_proxy (first_synth))));
  self->select_unseen_super = FALSE;
  // restore focus
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
  if (self->project.is_dirty())
    {
      uint result = bst_msg_dialog (BST_MSG_WARNING,
                                    BST_MSG_TITLE (_("Close %s"), self->project.get_name()),
                                    BST_MSG_TEXT1 (_("The project has been modified.")),
                                    BST_MSG_TEXT2 (_("Changes were made to project \"%s\" since the last time it was saved to disk."),
                                                   self->project.get_name()),
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
  assert_return (BST_IS_SUPER_SHELL (super_shell));

  Bse::SuperH super = super_shell->super;
  bst_super_shell_set_super (super_shell, Bse::SuperH());
  bst_super_shell_set_super (super_shell, super);
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
  const DemoEntry *d1 = (const DemoEntry*) v1;
  const DemoEntry *d2 = (const DemoEntry*) v2;
  return strcmp (d1->file, d2->file);
}

static void
demo_entries_setup (void)
{
  if (!demo_entries)
    {
      SfiRing *files = sfi_file_crawler_list_files (bse_server.get_demo_path().c_str(), "*.bse", GFileTest (0));
      while (files)
        {
          char *file = (char*) sfi_ring_pop_head (&files);
          char *name = bst_file_scan_find_key (file, "container-child", "BseSong::");
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
                size_t   callback_action)
{
  const gchar *file_name = demo_entries[callback_action - BST_ACTION_LOAD_DEMO_0000].file;
  Bse::ProjectH project = bse_server.create_project (file_name);
  Bse::Error error = bst_project_restore_from_file (project, file_name, TRUE, TRUE);
  if (error != 0)
    {
      bst_status_eprintf (error, _("Opening project `%s'"), file_name);
      bse_server.destroy_project (project);
    }
  else
    {
      project.get_wave_repo();
      BstApp *app = bst_app_new (project);
      gxk_status_window_push (app);
      bst_status_eprintf (error, _("Opening project `%s'"), file_name);
      gxk_status_window_pop ();
      gxk_idle_show_widget (GTK_WIDGET (app));
    }
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
      SfiRing *files = sfi_file_crawler_list_files (skindirs, "*.skin", GFileTest (0));
      g_free (skindirs);
      while (files)
        {
          char *file = (char*) sfi_ring_pop_head (&files);
          char *name = bst_file_scan_find_key (file, "skin-name", "");
          static guint statici = 1;
          if (!name)
            name = g_strdup_format ("skin-%u", statici++);
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
           size_t   callback_action)
{
  const gchar *file_name = skin_entries[callback_action - BST_ACTION_LOAD_SKIN_0000].file;
  Bse::Error error = bst_skin_parse (file_name);
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
app_action_exec (gpointer data,
                 size_t   action)
{
  static GtkWidget *bst_preferences = NULL;
  BstApp *self = BST_APP (data);
  const gchar *docs_url = NULL;
  GtkWidget *widget = GTK_WIDGET (self);

  gxk_status_window_push (widget);

  switch (action)
    {
      SfiProxy proxy;
      GtkWidget *any;
      BstMsgType demo_type;
    case BST_ACTION_EXIT:
      if (bst_app_class)
        {
          GSList *slist, *free_slist = g_slist_copy (bst_app_class->apps);

          for (slist = free_slist; slist; slist = slist->next)
            gxk_toplevel_delete ((GtkWidget*) slist->data);
          g_slist_free (free_slist);
        }
      break;
    case BST_ACTION_CLOSE_PROJECT:
      gxk_toplevel_delete (widget);
      break;
    case BST_ACTION_NEW_PROJECT:
      if (1)
        {
          Bse::ProjectH project = bse_server.create_project ("Untitled.bse");
          project.get_wave_repo();
          BstApp *new_app = bst_app_new (project);
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
      self->project.group_undo ("Create Song");
      {
        Bse::SongH song = self->project.create_song ("");
        song.ensure_master_bus();
      }
      self->project.ungroup_undo();
      self->select_unseen_super = TRUE;
      break;
    case BST_ACTION_NEW_CSYNTH:
      self->project.create_csynth ("");
      self->select_unseen_super = TRUE;
      break;
    case BST_ACTION_NEW_MIDI_SYNTH:
      self->project.create_midi_synth ("");
      self->select_unseen_super = TRUE;
      break;
    case BST_ACTION_REMOVE_SYNTH:
      proxy = bst_app_get_current_super (self);
      if (BSE_IS_SNET (proxy) && !self->project.is_active())
        {
          Bse::SNetH snet = Bse::SNetH::__cast__ (bse_server.from_proxy (proxy));
          self->project.remove_snet (snet);
        }
      self->select_unseen_super = FALSE;
      break;
    case BST_ACTION_CLEAR_UNDO:
      self->project.clear_undo();
      break;
    case BST_ACTION_UNDO:
      self->project.undo();
      break;
    case BST_ACTION_REDO:
      self->project.redo();
      break;
    case BST_ACTION_START_PLAYBACK:
      bst_project_ctrl_play (BST_PROJECT_CTRL (self->pcontrols));
      break;
    case BST_ACTION_STOP_PLAYBACK:
      bst_project_ctrl_stop (BST_PROJECT_CTRL (self->pcontrols));
      break;
    case ACTION_SHOW_PREFERENCES:
      if (!bst_preferences)
        {
          GtkWidget *widget = (GtkWidget*) g_object_new (BST_TYPE_PREFERENCES,
                                                         "visible", TRUE,
                                                         NULL);
          bst_preferences = (GtkWidget*) gxk_dialog_new (&bst_preferences,
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
    case BST_ACTION_REBUILD:
      gtk_container_foreach (GTK_CONTAINER (self->notebook),
                             (GtkCallback) rebuild_super_shell,
                             NULL);
      gtk_widget_queue_draw (GTK_WIDGET (self->notebook));
      break;
    case ACTION_HELP_MANUAL:
      docs_url = "beast-manual.html";
      goto BROWSE_LOCAL_URL;
    BROWSE_LOCAL_URL:
      if (docs_url)
        {
          gchar *local_url = g_strconcat ("file://", Bse::runpath (Bse::RPath::DOCDIR).c_str(), "/", docs_url, NULL);
          sfi_url_show (local_url);
          g_free (local_url);
        }
      break;
    case ACTION_HELP_ABOUT:
      beast_show_about_box ();
      break;
    case ACTION_URL_BEAST_SITE:
      sfi_url_show ("https://beast.testbit.org/");
      break;
    case ACTION_URL_REPORT_BUG:
      sfi_url_show ("https://github.com/tim-janik/beast/issues/");
      break;
    case ACTION_URL_ONLINE_SYNTHESIZERS:
      sfi_url_show ("http://beast.testbit.eu/Beast_Sound_Gallery");
      break;
    case ACTION_URL_ONLINE_DEMOS:
      sfi_url_show ("http://beast.testbit.eu/Beast_Sound_Gallery");
      break;
    case ACTION_DEMO_DIALOG_ERROR:
    case ACTION_DEMO_DIALOG_WARNING:
    case ACTION_DEMO_DIALOG_INFO:
    case ACTION_DEMO_DIALOG_DEBUG:
      switch (action)
        {
        default: /* silence compiler */
        case ACTION_DEMO_DIALOG_ERROR:   demo_type = BST_MSG_ERROR;   break;
        case ACTION_DEMO_DIALOG_WARNING: demo_type = BST_MSG_WARNING; break;
        case ACTION_DEMO_DIALOG_INFO:    demo_type = BST_MSG_INFO;    break;
        case ACTION_DEMO_DIALOG_DEBUG:   demo_type = BST_MSG_DEBUG;   break;
        }
      bst_msg_dialog (demo_type,
                      BST_MSG_TEXT0 ("Demonstration Dialog"),
                      BST_MSG_TEXT1 ("This is a demonstration dialog"),
                      BST_MSG_TEXT2 ("To help with dialog layout, and to test message display, dialogs may be "
                                     "fired up for pure demonstration purposes. This is such a dialog, so if you "
                                     "are currently looking at a prominent warning or error message, there's no "
                                     "real merit to it."),
                      BST_MSG_TEXT3 ("Demo-Dialog-Type: %s",
                                     Aida::enum_value_to_short_string (Bse::UserMessageType (demo_type))));
      break;
    default:
      assert_return_unreached ();
      break;
    }

  gxk_status_window_pop ();

  gxk_widget_update_actions_downwards (self);
}

static gboolean
app_action_check (gpointer data,
                  size_t   action,
                  guint64  action_stamp)
{
  BstApp *self = BST_APP (data);
  if (!self->project)
    return FALSE;
  switch (action)
    {
      SfiProxy super;
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
      return self->project.is_dirty();
    case BST_ACTION_NEW_SONG:
      {
        Bse::ItemSeq items = self->project.list_children();
        for (size_t i = 0; i < items.size(); i++)
          if (BSE_IS_SONG (items[i].proxy_id()))
            return FALSE;
      }
      return TRUE;
    case BST_ACTION_REMOVE_SYNTH:
      super = bst_app_get_current_super (self);
      return BSE_IS_SNET (super) && !self->project.is_active();
    case BST_ACTION_CLEAR_UNDO:
      return self->project.undo_depth() + self->project.redo_depth() > 0;
    case BST_ACTION_UNDO:
      return self->project.undo_depth() > 0;
    case BST_ACTION_REDO:
      return self->project.redo_depth() > 0;
    case BST_ACTION_REBUILD:
      return TRUE;
    case BST_ACTION_START_PLAYBACK:
      if (self->project && self->project.can_play())
        return TRUE;
      return FALSE;
    case BST_ACTION_STOP_PLAYBACK:
      if (self->project && self->project.is_playing())
        return TRUE;
      return FALSE;
    case ACTION_SHOW_PREFERENCES:
    case ACTION_EXTRA_VIEW:
    case ACTION_SHOW_PROFILER:
      return TRUE;
    case BST_ACTION_MERGE_EFFECT:
    case BST_ACTION_MERGE_INSTRUMENT:
      return !self->project.is_active();
    case BST_ACTION_SAVE_EFFECT:
    case BST_ACTION_SAVE_INSTRUMENT:
      super = bst_app_get_current_super (self);
      return BSE_IS_CSYNTH (super) && !self->project.is_active();
    case ACTION_HELP_MANUAL:
    case ACTION_HELP_ABOUT:
    case ACTION_URL_BEAST_SITE:
    case ACTION_URL_REPORT_BUG:
    case ACTION_URL_ONLINE_SYNTHESIZERS:
    case ACTION_URL_ONLINE_DEMOS:
    case ACTION_DEMO_DIALOG_ERROR:
    case ACTION_DEMO_DIALOG_WARNING:
    case ACTION_DEMO_DIALOG_INFO:
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
      Bse::warning ("BstApp: unknown action: %lu", action);
      return FALSE;
    }
}

static void
bst_app_class_init (BstAppClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  bst_app_class = klass;

  gobject_class->finalize = bst_app_finalize;

  object_class->destroy = bst_app_destroy;

  widget_class->delete_event = bst_app_handle_delete_event;

  klass->apps = NULL;
}
