
/*
 * Generated data (by mkcproc.pl)
 */
#line 1 "bseproject.proc"
// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include <bse/bseplugin.hh>
#include <bse/bseprocedure.hh>
#include <bse/bseproject.hh>
#include <bse/bsestorage.hh>
#include <bse/bsesong.hh>
#include <bse/bseundostack.hh>
#include <bse/bsewaverepo.hh>
#include <bse/bsesoundfontrepo.hh>
#include <bse/bsecsynth.hh>
#include <bse/bsemidisynth.hh>
#include <bse/bsemidifile.hh>
#include <bse/bsemidireceiver.hh>
#include <bse/bsemidinotifier.hh>
#include <bse/bseengine.hh>
#include "bsecxxplugin.hh"
#include "bsebuiltin_externs.cc"

#line 21 "bseproject.proc"



/* --- get-midi-notifier --- */
static void
get_midi_notifier_setup (BseProcedureClass *proc, GParamSpec **in_pspecs, GParamSpec **out_pspecs) {
#line 24 "bseproject.proc"
 {
#line 26 "bseproject.proc"
  *(in_pspecs++)    = bse_param_spec_object ("project", "Project", NULL, BSE_TYPE_PROJECT, SFI_PARAM_STANDARD);
  *(out_pspecs++)   = bse_param_spec_object ("midi_notifier", NULL, NULL, BSE_TYPE_MIDI_NOTIFIER, SFI_PARAM_STANDARD);
#line 28 "bseproject.proc"
}  }
static Bse::Error
#line 28 "bseproject.proc"
get_midi_notifier_exec (BseProcedureClass *proc,
        const GValue      *in_values,
        GValue            *out_values)
#line 31 "bseproject.proc"
{
#line 32 "bseproject.proc"
  
  BseProject *self = (BseProject*) bse_value_get_object (in_values++);

#line 35 "bseproject.proc"
  
  if (!BSE_IS_PROJECT (self))
    return Bse::Error::PROC_PARAM_INVAL;

  BseMidiNotifier *notifier = bse_project_get_midi_notifier (self);

#line 41 "bseproject.proc"
  
  bse_value_set_object (out_values++, G_OBJECT (notifier));

  return Bse::Error::NONE;
}



/* --- store-bse --- */
static void
store_bse_setup (BseProcedureClass *proc, GParamSpec **in_pspecs, GParamSpec **out_pspecs) {
#line 49 "bseproject.proc"
 {
#line 52 "bseproject.proc"
  *(in_pspecs++)    = bse_param_spec_object ("project", "Project", NULL,
                                 BSE_TYPE_PROJECT, SFI_PARAM_STANDARD);
  *(in_pspecs++)    = bse_param_spec_object ("super", "Super", NULL,
                                 BSE_TYPE_SUPER, SFI_PARAM_STANDARD);
  *(in_pspecs++)    = sfi_pspec_string ("file-name", "File", "Destination file name",
                            NULL, SFI_PARAM_STANDARD);
  *(in_pspecs++)    = sfi_pspec_bool ("self-contained", "Self Contained",
                          "Whether references to other objects (e.g. samples) should "
                          "be stored or whether to include everything in a self-contained .bse file",
                          FALSE, SFI_PARAM_STANDARD);
  *(out_pspecs++)   = bse_param_spec_genum ("error", "Error", "Error indicating possible failures",
                                BSE_TYPE_ERROR_TYPE, Bse::Error::NONE,
                                SFI_PARAM_STANDARD);
#line 65 "bseproject.proc"
}  }
static Bse::Error
#line 65 "bseproject.proc"
store_bse_exec (BseProcedureClass *proc,
        const GValue      *in_values,
        GValue            *out_values)
#line 68 "bseproject.proc"
{
#line 69 "bseproject.proc"
  
  BseProject *project = (BseProject*) bse_value_get_object (in_values++);
  BseSuper *super = (BseSuper*) bse_value_get_object (in_values++);
  const char *file_name = sfi_value_get_string (in_values++);
  gboolean self_contained = sfi_value_get_bool (in_values++);
  Bse::Error error;

#line 76 "bseproject.proc"
  
  if (!BSE_IS_PROJECT (project) || !file_name)
    return Bse::Error::PROC_PARAM_INVAL;
  if (super && BSE_ITEM (super)->parent != BSE_ITEM (project))
    return Bse::Error::PROC_PARAM_INVAL;

  error = bse_project_store_bse (project, super, file_name, self_contained);

#line 84 "bseproject.proc"
  
  g_value_set_enum (out_values++, int (error));

  return Bse::Error::NONE;
}

/* --- create-song --- */
static void
create_song_setup (BseProcedureClass *proc, GParamSpec **in_pspecs, GParamSpec **out_pspecs) {
#line 90 "bseproject.proc"
 {
#line 92 "bseproject.proc"
  *(in_pspecs++)    = bse_param_spec_object ("project", "Project", "The project",
                                 BSE_TYPE_PROJECT, SFI_PARAM_STANDARD);
  *(in_pspecs++)    = sfi_pspec_string ("name", "Name", "Song name",
                            NULL, SFI_PARAM_STANDARD);
  *(out_pspecs++)   = bse_param_spec_object ("song", "Song", "The new song",
                                 BSE_TYPE_SONG, SFI_PARAM_STANDARD);
#line 98 "bseproject.proc"
}  }
static Bse::Error
#line 98 "bseproject.proc"
create_song_exec (BseProcedureClass *proc,
        const GValue      *in_values,
        GValue            *out_values)
#line 101 "bseproject.proc"
{
#line 102 "bseproject.proc"
  
  BseContainer *container = (BseContainer*) bse_value_get_object (in_values++);
  const char *name = sfi_value_get_string (in_values++);
  BseUndoStack *ustack;
  BseItem *child;

#line 108 "bseproject.proc"
  
  if (!BSE_IS_PROJECT (container))
    return Bse::Error::PROC_PARAM_INVAL;

#line 112 "bseproject.proc"
  
  ustack = bse_item_undo_open (container, "create-song");
  child = (BseItem*) bse_container_new_child (container, BSE_TYPE_SONG, NULL);
  if (name)
    bse_item_set (child, "uname", name, NULL);
  bse_item_push_undo_proc (container, "remove-snet", child);
  bse_item_undo_close (ustack);

#line 120 "bseproject.proc"
  
  bse_value_set_object (out_values++, child);

  return Bse::Error::NONE;
}

/* --- get-wave-repo --- */
static void
get_wave_repo_setup (BseProcedureClass *proc, GParamSpec **in_pspecs, GParamSpec **out_pspecs) {
#line 126 "bseproject.proc"
 {
#line 128 "bseproject.proc"
  *(in_pspecs++)    = bse_param_spec_object ("project", "Project", "The project",
                                 BSE_TYPE_PROJECT, SFI_PARAM_STANDARD);
  *(out_pspecs++)   = bse_param_spec_object ("wrepo", "Wave Repo", "The project's unique wave repo",
                                 BSE_TYPE_WAVE_REPO, SFI_PARAM_STANDARD);
#line 132 "bseproject.proc"
}  }
static Bse::Error
#line 132 "bseproject.proc"
get_wave_repo_exec (BseProcedureClass *proc,
        const GValue      *in_values,
        GValue            *out_values)
#line 135 "bseproject.proc"
{
#line 136 "bseproject.proc"
  
  BseProject *project = (BseProject*) bse_value_get_object (in_values++);
  BseWaveRepo *wrepo = NULL;

#line 140 "bseproject.proc"
  
  if (!BSE_IS_PROJECT (project))
    return Bse::Error::PROC_PARAM_INVAL;

#line 144 "bseproject.proc"
  
  wrepo = bse_project_get_wave_repo (project);

#line 147 "bseproject.proc"
  
  bse_value_set_object (out_values++, wrepo);

  return Bse::Error::NONE;
}

/* --- get-sound-font-repo --- */
static void
get_sound_font_repo_setup (BseProcedureClass *proc, GParamSpec **in_pspecs, GParamSpec **out_pspecs) {
#line 153 "bseproject.proc"
 {
#line 155 "bseproject.proc"
  *(in_pspecs++)    = bse_param_spec_object ("project", "Project", "The project",
                                 BSE_TYPE_PROJECT, SFI_PARAM_STANDARD);
  *(out_pspecs++)   = bse_param_spec_object ("sfrepo", "Sound Font Repo", "The project's unique sound font repo",
                                 BSE_TYPE_SOUND_FONT_REPO, SFI_PARAM_STANDARD);

#line 160 "bseproject.proc"
}  }
static Bse::Error
#line 160 "bseproject.proc"
get_sound_font_repo_exec (BseProcedureClass *proc,
        const GValue      *in_values,
        GValue            *out_values)
#line 163 "bseproject.proc"
{
#line 164 "bseproject.proc"
  
  BseProject *project = BSE_PROJECT (bse_value_get_object (in_values++));
  BseSoundFontRepo *sfrepo = NULL;

#line 168 "bseproject.proc"
  
  if (!BSE_IS_PROJECT (project))
    return Bse::Error::PROC_PARAM_INVAL;

#line 172 "bseproject.proc"
  
  sfrepo = bse_project_get_sound_font_repo (project);

#line 175 "bseproject.proc"
  
  bse_value_set_object (out_values++, sfrepo);

  return Bse::Error::NONE;
}

/* --- create-csynth --- */
static void
create_csynth_setup (BseProcedureClass *proc, GParamSpec **in_pspecs, GParamSpec **out_pspecs) {
#line 181 "bseproject.proc"
 {
#line 183 "bseproject.proc"
  *(in_pspecs++)    = bse_param_spec_object ("project", "Project", "The project",
                                 BSE_TYPE_PROJECT, SFI_PARAM_STANDARD);
  *(in_pspecs++)    = sfi_pspec_string ("name", "Name", "Synth network name",
                            NULL, SFI_PARAM_STANDARD);
  *(out_pspecs++)   = bse_param_spec_object ("csynth", "Synthesizer Network", "New synth network",
                                 BSE_TYPE_CSYNTH, SFI_PARAM_STANDARD);
#line 189 "bseproject.proc"
}  }
static Bse::Error
#line 189 "bseproject.proc"
create_csynth_exec (BseProcedureClass *proc,
        const GValue      *in_values,
        GValue            *out_values)
#line 192 "bseproject.proc"
{
#line 193 "bseproject.proc"
  
  BseContainer *container = (BseContainer*) bse_value_get_object (in_values++);
  const char *name = sfi_value_get_string (in_values++);
  BseUndoStack *ustack;
  BseItem *child;

#line 199 "bseproject.proc"
  
  if (!BSE_IS_PROJECT (container))
    return Bse::Error::PROC_PARAM_INVAL;

#line 203 "bseproject.proc"
  
  ustack = bse_item_undo_open (container, "create-csynth");
  child = (BseItem*) bse_container_new_child (container, BSE_TYPE_CSYNTH, NULL);
  if (name)
    bse_item_set (child, "uname", name, NULL);
  bse_item_push_undo_proc (container, "remove-snet", child);
  bse_item_undo_close (ustack);

#line 211 "bseproject.proc"
  
  bse_value_set_object (out_values++, child);

  return Bse::Error::NONE;
}

/* --- create-midi-synth --- */
static void
create_midi_synth_setup (BseProcedureClass *proc, GParamSpec **in_pspecs, GParamSpec **out_pspecs) {
#line 217 "bseproject.proc"
 {
#line 219 "bseproject.proc"
  *(in_pspecs++)    = bse_param_spec_object ("project", "Project", "The project",
                                 BSE_TYPE_PROJECT, SFI_PARAM_STANDARD);
  *(in_pspecs++)    = sfi_pspec_string ("name", "Name", "MIDI synth name",
                            NULL, SFI_PARAM_STANDARD);
  *(out_pspecs++)   = bse_param_spec_object ("midi_synth", "MIDI Synthesizer", "New MIDI synth",
                                 BSE_TYPE_MIDI_SYNTH, SFI_PARAM_STANDARD);
#line 225 "bseproject.proc"
}  }
static Bse::Error
#line 225 "bseproject.proc"
create_midi_synth_exec (BseProcedureClass *proc,
        const GValue      *in_values,
        GValue            *out_values)
#line 228 "bseproject.proc"
{
#line 229 "bseproject.proc"
  
  BseContainer *container = (BseContainer*) bse_value_get_object (in_values++);
  const char *name = sfi_value_get_string (in_values++);
  BseUndoStack *ustack;
  BseItem *child;

#line 235 "bseproject.proc"
  
  if (!BSE_IS_PROJECT (container))
    return Bse::Error::PROC_PARAM_INVAL;

#line 239 "bseproject.proc"
  
  ustack = bse_item_undo_open (container, "create-midi-synth");
  child = (BseItem*) bse_container_new_child (container, BSE_TYPE_MIDI_SYNTH, NULL);
  if (name)
    bse_item_set (child, "uname", name, NULL);
  bse_item_push_undo_proc (container, "remove-snet", child);
  bse_item_undo_close (ustack);

#line 247 "bseproject.proc"
  
  bse_value_set_object (out_values++, child);

  return Bse::Error::NONE;
}

/* --- remove-snet --- */
static void
remove_snet_setup (BseProcedureClass *proc, GParamSpec **in_pspecs, GParamSpec **out_pspecs) {
#line 253 "bseproject.proc"
 {
#line 255 "bseproject.proc"
  *(in_pspecs++)    = bse_param_spec_object ("project", "Project", "The project",
                                 BSE_TYPE_PROJECT, SFI_PARAM_STANDARD);
  *(in_pspecs++)    = bse_param_spec_object ("snet", "SNet", "Synthesizer Network",
                                 BSE_TYPE_SNET, SFI_PARAM_STANDARD);
#line 259 "bseproject.proc"
}  }
static Bse::Error
#line 259 "bseproject.proc"
remove_snet_exec (BseProcedureClass *proc,
        const GValue      *in_values,
        GValue            *out_values)
#line 262 "bseproject.proc"
{
#line 263 "bseproject.proc"
  
  BseContainer *self = (BseContainer*) bse_value_get_object (in_values++);
  BseItem *child = (BseItem*) bse_value_get_object (in_values++);
  BseUndoStack *ustack;

#line 268 "bseproject.proc"
  
  if (!BSE_IS_PROJECT (self) || !BSE_IS_SNET (child) || child->parent != (BseItem*) self)
    return Bse::Error::PROC_PARAM_INVAL;

#line 272 "bseproject.proc"
  
  if (!BSE_SOURCE_PREPARED (self))
    {
      ustack = bse_item_undo_open (self, "remove-child %s", bse_object_debug_name (child));
#line 276 "bseproject.proc"
      
      bse_container_uncross_undoable (BSE_CONTAINER (self), child);
#line 278 "bseproject.proc"
      
      bse_item_push_redo_proc (self, "remove-snet", child);
#line 280 "bseproject.proc"
      
      bse_container_remove_backedup (BSE_CONTAINER (self), child, ustack);
#line 282 "bseproject.proc"
      
      bse_item_undo_close (ustack);
    }

  return Bse::Error::NONE;
}

/* --- get-supers --- */
static void
get_supers_setup (BseProcedureClass *proc, GParamSpec **in_pspecs, GParamSpec **out_pspecs) {
#line 289 "bseproject.proc"
 {
#line 291 "bseproject.proc"
  *(in_pspecs++)    = bse_param_spec_object ("project", NULL, NULL,
                                 BSE_TYPE_PROJECT, SFI_PARAM_STANDARD);
  *(out_pspecs++)   = bse_param_spec_boxed ("super_list", NULL, NULL, BSE_TYPE_IT3M_SEQ, SFI_PARAM_STANDARD);
#line 294 "bseproject.proc"
}  }
static Bse::Error
#line 294 "bseproject.proc"
get_supers_exec (BseProcedureClass *proc,
        const GValue      *in_values,
        GValue            *out_values)
#line 297 "bseproject.proc"
{
#line 298 "bseproject.proc"
  
  BseProject *project = (BseProject*) bse_value_get_object (in_values++);
  BseIt3mSeq *iseq;
  GSList *slist;

#line 303 "bseproject.proc"
  
  if (!BSE_IS_PROJECT (project))
    return Bse::Error::PROC_PARAM_INVAL;

#line 307 "bseproject.proc"
  
  iseq = bse_it3m_seq_new ();
  for (slist = project->supers; slist; slist = slist->next)
    bse_it3m_seq_append (iseq, (BseItem*) slist->data);

#line 312 "bseproject.proc"
  
  bse_value_take_boxed (out_values++, iseq);

  return Bse::Error::NONE;
}


/* --- get-state --- */
static void
get_state_setup (BseProcedureClass *proc, GParamSpec **in_pspecs, GParamSpec **out_pspecs) {
#line 319 "bseproject.proc"
 {
#line 321 "bseproject.proc"
  *(in_pspecs++)    = bse_param_spec_object ("project", "Project", "The project",
                                 BSE_TYPE_PROJECT, SFI_PARAM_STANDARD);
  *(out_pspecs++)   = bse_param_spec_genum ("state", "State", "Project playback/activation state",
                                BSE_TYPE_PROJECT_STATE, BSE_PROJECT_INACTIVE, SFI_PARAM_STANDARD);
#line 325 "bseproject.proc"
}  }
static Bse::Error
#line 325 "bseproject.proc"
get_state_exec (BseProcedureClass *proc,
        const GValue      *in_values,
        GValue            *out_values)
#line 328 "bseproject.proc"
{
#line 329 "bseproject.proc"
  
  BseProject *self = (BseProject*) bse_value_get_object (in_values++);

#line 332 "bseproject.proc"
  
  if (!BSE_IS_PROJECT (self))
    return Bse::Error::PROC_PARAM_INVAL;

#line 336 "bseproject.proc"
  
  g_value_set_enum (out_values++, self->state);

  return Bse::Error::NONE;
}

/* --- Export to BSE --- */
static void
__enode_get_midi_notifier__fill_strings (BseExportStrings *es)
{
  es->blurb = "Retrieve the project's midi notifier object.";
  es->file = "/home/stefan/src/stwbeast/bse/bseproject.proc";
  es->line = 25;
  es->authors = "Tim Janik <timj@gtk.org>";
  es->license = "GNU Lesser General Public License";
}
static BseExportNodeProc __enode_get_midi_notifier = {
  { NULL, BSE_EXPORT_NODE_PROC,
    "BseProject+get-midi-notifier", 
    NULL,
    "/Methods/BseProject/General/Get Midi Notifier",
    NULL,
    __enode_get_midi_notifier__fill_strings,
  },
  0, get_midi_notifier_setup, get_midi_notifier_exec, 
};
static void
__enode_store_bse__fill_strings (BseExportStrings *es)
{
  es->blurb = "Save supers of a project into a BSE file. "
          "If no super is specified, the project itself is stored.";
  es->file = "/home/stefan/src/stwbeast/bse/bseproject.proc";
  es->line = 50;
  es->authors = "Tim Janik <timj@gtk.org>";
  es->license = "GNU Lesser General Public License";
}
static BseExportNodeProc __enode_store_bse = {
  { (BseExportNode*) &__enode_get_midi_notifier, BSE_EXPORT_NODE_PROC,
    "BseProject+store-bse", 
    NULL,
    "/Methods/BseProject/File/Store",
    NULL,
    __enode_store_bse__fill_strings,
  },
  0, store_bse_setup, store_bse_exec, 
};
static void
__enode_create_song__fill_strings (BseExportStrings *es)
{
  es->blurb = "Create a song for this project.";
  es->file = "/home/stefan/src/stwbeast/bse/bseproject.proc";
  es->line = 91;
  es->authors = "Tim Janik <timj@gtk.org>";
  es->license = "GNU Lesser General Public License";
}
static BseExportNodeProc __enode_create_song = {
  { (BseExportNode*) &__enode_store_bse, BSE_EXPORT_NODE_PROC,
    "BseProject+create-song", 
    NULL,
    "/Methods/BseProject/General/Create Song",
    NULL,
    __enode_create_song__fill_strings,
  },
  0, create_song_setup, create_song_exec, 
};
static void
__enode_get_wave_repo__fill_strings (BseExportStrings *es)
{
  es->blurb = "Ensure the project has a wave repository";
  es->file = "/home/stefan/src/stwbeast/bse/bseproject.proc";
  es->line = 127;
  es->authors = "Tim Janik <timj@gtk.org>";
  es->license = "GNU Lesser General Public License";
}
static BseExportNodeProc __enode_get_wave_repo = {
  { (BseExportNode*) &__enode_create_song, BSE_EXPORT_NODE_PROC,
    "BseProject+get-wave-repo", 
    NULL,
    "/Methods/BseProject/General/Get Wave Repo",
    NULL,
    __enode_get_wave_repo__fill_strings,
  },
  0, get_wave_repo_setup, get_wave_repo_exec, 
};
static void
__enode_get_sound_font_repo__fill_strings (BseExportStrings *es)
{
  es->blurb = "Get sound font repository for project";
  es->file = "/home/stefan/src/stwbeast/bse/bseproject.proc";
  es->line = 154;
  es->authors = "Tim Janik <timj@gtk.org>";
  es->license = "GNU Lesser General Public License";
}
static BseExportNodeProc __enode_get_sound_font_repo = {
  { (BseExportNode*) &__enode_get_wave_repo, BSE_EXPORT_NODE_PROC,
    "BseProject+get-sound-font-repo", 
    NULL,
    "/Methods/BseProject/General/Get Sound Font Repo",
    NULL,
    __enode_get_sound_font_repo__fill_strings,
  },
  0, get_sound_font_repo_setup, get_sound_font_repo_exec, 
};
static void
__enode_create_csynth__fill_strings (BseExportStrings *es)
{
  es->blurb = "Create a synthsizer network for this project.";
  es->file = "/home/stefan/src/stwbeast/bse/bseproject.proc";
  es->line = 182;
  es->authors = "Tim Janik <timj@gtk.org>";
  es->license = "GNU Lesser General Public License";
}
static BseExportNodeProc __enode_create_csynth = {
  { (BseExportNode*) &__enode_get_sound_font_repo, BSE_EXPORT_NODE_PROC,
    "BseProject+create-csynth", 
    NULL,
    "/Methods/BseProject/General/Create Csynth",
    NULL,
    __enode_create_csynth__fill_strings,
  },
  0, create_csynth_setup, create_csynth_exec, 
};
static void
__enode_create_midi_synth__fill_strings (BseExportStrings *es)
{
  es->blurb = "Create a MIDI synthesizer network for this project.";
  es->file = "/home/stefan/src/stwbeast/bse/bseproject.proc";
  es->line = 218;
  es->authors = "Tim Janik <timj@gtk.org>";
  es->license = "GNU Lesser General Public License";
}
static BseExportNodeProc __enode_create_midi_synth = {
  { (BseExportNode*) &__enode_create_csynth, BSE_EXPORT_NODE_PROC,
    "BseProject+create-midi-synth", 
    NULL,
    "/Methods/BseProject/General/Create Midi Synth",
    NULL,
    __enode_create_midi_synth__fill_strings,
  },
  0, create_midi_synth_setup, create_midi_synth_exec, 
};
static void
__enode_remove_snet__fill_strings (BseExportStrings *es)
{
  es->blurb = "Remove an existing synthesizer network from this project.";
  es->file = "/home/stefan/src/stwbeast/bse/bseproject.proc";
  es->line = 254;
  es->authors = "Tim Janik <timj@gtk.org>";
  es->license = "GNU Lesser General Public License";
}
static BseExportNodeProc __enode_remove_snet = {
  { (BseExportNode*) &__enode_create_midi_synth, BSE_EXPORT_NODE_PROC,
    "BseProject+remove-snet", 
    NULL,
    "/Methods/BseProject/General/Remove Snet",
    NULL,
    __enode_remove_snet__fill_strings,
  },
  0, remove_snet_setup, remove_snet_exec, 
};
static void
__enode_get_supers__fill_strings (BseExportStrings *es)
{
  es->blurb = "Retrieve all supers of this project.";
  es->file = "/home/stefan/src/stwbeast/bse/bseproject.proc";
  es->line = 290;
  es->authors = "Tim Janik <timj@gtk.org>";
  es->license = "GNU Lesser General Public License";
}
static BseExportNodeProc __enode_get_supers = {
  { (BseExportNode*) &__enode_remove_snet, BSE_EXPORT_NODE_PROC,
    "BseProject+get-supers", 
    NULL,
    "/Methods/BseProject/General/Get Supers",
    NULL,
    __enode_get_supers__fill_strings,
  },
  0, get_supers_setup, get_supers_exec, 
};
static void
__enode_get_state__fill_strings (BseExportStrings *es)
{
  es->blurb = "Retrieve the current project state.";
  es->file = "/home/stefan/src/stwbeast/bse/bseproject.proc";
  es->line = 320;
  es->authors = "Tim Janik <timj@gtk.org>";
  es->license = "GNU Lesser General Public License";
}
static BseExportNodeProc __enode_get_state = {
  { (BseExportNode*) &__enode_get_supers, BSE_EXPORT_NODE_PROC,
    "BseProject+get-state", 
    NULL,
    "/Methods/BseProject/General/Get State",
    NULL,
    __enode_get_state__fill_strings,
  },
  0, get_state_setup, get_state_exec, 
};
extern "C" BseExportNode* bse__builtin_init_bseproject_genprc_cc (void)
{
  return (BseExportNode*) &__enode_get_state;
}

/*
 * Generated data ends here
 */
