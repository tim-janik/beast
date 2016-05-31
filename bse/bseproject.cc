// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseproject.hh"

#include "bsesuper.hh"
#include "bsestorage.hh"
#include "bsesong.hh"
#include "bsesnet.hh"
#include "bsecsynth.hh"
#include "bsewaverepo.hh"
#include "bsesequencer.hh"
#include "bseserver.hh"
#include "bseundostack.hh"
#include "bsemain.hh"
#include "bsestandardsynths.hh"
#include "bsemidireceiver.hh"
#include "bsemidinotifier.hh"
#include "gslcommon.hh"
#include "bseengine.hh"
#include "bsemidifile.hh"
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>


typedef struct {
  GType    base_type;
  gboolean intern_children;
  guint    max_items;
  GSList  *items;
} StorageTrap;

/* --- macros --- */
#define parse_or_return         bse_storage_scanner_parse_or_return
#define peek_or_return          bse_storage_scanner_peek_or_return

enum {
  PARAM_0,
  PARAM_DIRTY
};


/* --- prototypes --- */
static void	bse_project_class_init		(BseProjectClass	*klass);
static void	bse_project_class_finalize	(BseProjectClass	*klass);
static void	bse_project_init		(BseProject		*project,
						 gpointer		 rclass);
static void     bse_project_set_property        (GObject                *object,
                                                 guint                   param_id,
                                                 const GValue           *value,
                                                 GParamSpec             *pspec);
static void     bse_project_get_property        (GObject                *object,
                                                 guint                   param_id,
                                                 GValue                 *value,
                                                 GParamSpec             *pspec);
static void	bse_project_finalize		(GObject		*object);
static void	bse_project_release_children	(BseContainer		*container);
static void	bse_project_dispose		(GObject		*object);
static void	bse_project_add_item		(BseContainer		*container,
						 BseItem		*item);
static void	bse_project_remove_item		(BseContainer		*container,
						 BseItem		*item);
static void	bse_project_forall_items	(BseContainer		*container,
						 BseForallItemsFunc	 func,
						 gpointer		 data);
static BseItem* bse_project_retrieve_child	(BseContainer           *container,
						 GType                   child_type,
						 const gchar            *uname);
static void	bse_project_prepare		(BseSource		*source);
static gboolean project_check_restore		(BseContainer           *container,
						 const gchar            *child_type);
static BseUndoStack* bse_project_get_undo       (BseItem                *item);


/* --- variables --- */
static GTypeClass *parent_class = NULL;
static GQuark      quark_storage_trap = 0;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseProject)
{
  static const GTypeInfo project_info = {
    sizeof (BseProjectClass),

    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_project_class_init,
    (GClassFinalizeFunc) bse_project_class_finalize,
    NULL /* class_data */,

    sizeof (BseProject),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_project_init,
  };

  return bse_type_register_static (BSE_TYPE_CONTAINER,
				   "BseProject",
				   "BSE Super container type",
                                   __FILE__, __LINE__,
                                   &project_info);
}

static void
bse_project_class_init (BseProjectClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (klass);
  BseItemClass *item_class = BSE_ITEM_CLASS (klass);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (klass);
  BseContainerClass *container_class = BSE_CONTAINER_CLASS (klass);

  parent_class = (GTypeClass*) g_type_class_peek_parent (klass);
  quark_storage_trap = g_quark_from_static_string ("bse-project-storage-trap");

  gobject_class->set_property = bse_project_set_property;
  gobject_class->get_property = bse_project_get_property;
  gobject_class->dispose = bse_project_dispose;
  gobject_class->finalize = bse_project_finalize;

  item_class->get_undo = bse_project_get_undo;

  source_class->prepare = bse_project_prepare;

  container_class->add_item = bse_project_add_item;
  container_class->remove_item = bse_project_remove_item;
  container_class->forall_items = bse_project_forall_items;
  container_class->check_restore = project_check_restore;
  container_class->retrieve_child = bse_project_retrieve_child;
  container_class->release_children = bse_project_release_children;

  bse_object_class_add_param (object_class, "State",
                              PARAM_DIRTY,
                              sfi_pspec_bool ("dirty", NULL, "Whether project needs saving",
                                              FALSE, "r"));
}

static void
bse_project_class_finalize (BseProjectClass *klass)
{
}

static void
undo_notify (BseProject     *project,
             BseUndoStack   *ustack,
             gboolean        step_added)
{
  g_object_notify ((GObject*) project, "dirty");
  if (step_added && !project->in_redo)
    {
      bse_undo_stack_force_dirty (project->undo_stack);
      bse_undo_stack_clear (project->redo_stack);
    }
}

static void
redo_notify (BseProject     *project,
             BseUndoStack   *ustack,
             gboolean        step_added)
{
  g_object_notify ((GObject*) project, "dirty");
}

static void
bse_project_init (BseProject *self,
		  gpointer    rclass)
{
  self->state = Bse::ProjectState::INACTIVE;
  self->supers = NULL;
  self->items = NULL;
  self->in_undo = FALSE;
  self->in_redo = FALSE;
  self->undo_stack = bse_undo_stack_new (self, undo_notify);
  self->redo_stack = bse_undo_stack_new (self, redo_notify);
  self->deactivate_usecs = 3 * 1000000;
  self->midi_receiver = bse_midi_receiver_new ("BseProjectReceiver");
  bse_midi_receiver_enter_farm (self->midi_receiver);
  /* we always have a wave-repo */
  BseWaveRepo *wrepo = (BseWaveRepo*) bse_container_new_child (BSE_CONTAINER (self), BSE_TYPE_WAVE_REPO, "uname", "Wave-Repository", NULL);
  /* with fixed uname */
  BSE_OBJECT_SET_FLAGS (wrepo, BSE_OBJECT_FLAG_FIXED_UNAME);
}

static void
bse_project_set_property (GObject                *object,
                          guint                   param_id,
                          const GValue           *value,
                          GParamSpec             *pspec)
{
  BseProject *self = BSE_PROJECT (object);

  switch (param_id)
    {
    case PARAM_DIRTY:
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_project_get_property (GObject                *object,
                          guint                   param_id,
                          GValue                 *value,
                          GParamSpec             *pspec)
{
  BseProject *self = BSE_PROJECT (object);

  switch (param_id)
    {
    case PARAM_DIRTY:
      sfi_value_set_bool (value, bse_undo_stack_dirty (self->undo_stack));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_project_release_children (BseContainer *container)
{
  BseProject *project = BSE_PROJECT (container);

  while (project->items)
    bse_container_remove_item (BSE_CONTAINER (project), (BseItem*) project->items->data);
  while (project->supers)
    bse_container_remove_item (BSE_CONTAINER (project), (BseItem*) project->supers->data);

  /* chain parent class' handler */
  BSE_CONTAINER_CLASS (parent_class)->release_children (container);
}

static void
bse_project_dispose (GObject *object)
{
  BseProject *self = BSE_PROJECT (object);

  bse_project_deactivate (self);

  bse_undo_stack_limit (self->undo_stack, 0);
  bse_undo_stack_limit (self->redo_stack, 0);

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
bse_project_finalize (GObject *object)
{
  BseProject *self = BSE_PROJECT (object);

  bse_midi_receiver_unref (self->midi_receiver);
  self->midi_receiver = NULL;

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);

  bse_undo_stack_destroy (self->undo_stack);
  bse_undo_stack_destroy (self->redo_stack);
}

static BseUndoStack*
bse_project_get_undo (BseItem *item)
{
  BseProject *self = BSE_PROJECT (item);
  return self->in_undo ? self->redo_stack : self->undo_stack;
}

void
bse_project_clear_undo (BseProject *self)
{
  assert_return (BSE_IS_PROJECT (self));
  if (!self->in_undo && !self->in_redo)
    {
      bse_undo_stack_clear (self->undo_stack);
      bse_undo_stack_clear (self->redo_stack);
      g_object_notify ((GObject*) self, "dirty");
    }
}

void
bse_project_clean_dirty (BseProject *self)
{
  assert_return (BSE_IS_PROJECT (self));
  bse_undo_stack_clean_dirty (self->undo_stack);
  bse_undo_stack_clean_dirty (self->redo_stack);
  g_object_notify ((GObject*) self, "dirty");
}

static void
project_undo_do_deactivate (BseUndoStep  *ustep,
                            BseUndoStack *ustack)
{
  BseProject *self = (BseProject*) bse_undo_pointer_unpack ((const char*) ustep->data[0].v_pointer, ustack);
  bse_project_deactivate (self);
}

static void
project_undo_do_deactivate_free (BseUndoStep *ustep)
{
  g_free (ustep->data[0].v_pointer);
}

void
bse_project_push_undo_silent_deactivate (BseProject *self)
{
  assert_return (BSE_IS_PROJECT (self));

  /* certain things work only (can only be undone/redone) in deactivated projects,
   * so we need to push an undo step here. this step isn't required however
   * if there're no undo steps pushed so far, and it shouldn't be visible
   * as a seperate undo step to the user either. so what we do is to push
   * an "add-on" step, which just prepares state for execution of further
   * undo steps, if there are any pending.
   */
  if (self->state != Bse::ProjectState::INACTIVE)
    {
      BseUndoStack *ustack = bse_item_undo_open (self, "deactivate-project");
      BseUndoStep *ustep = bse_undo_step_new (project_undo_do_deactivate, project_undo_do_deactivate_free, 1);
      ustep->data[0].v_pointer = bse_undo_pointer_pack (self, ustack);
      bse_undo_stack_push_add_on (ustack, ustep);
      bse_item_undo_close (ustack);

      gboolean in_undo = self->in_undo;
      self->in_undo = !in_undo;             /* swap undo<=>redo */
      ustack = bse_item_undo_open (self, "deactivate-project");
      ustep = bse_undo_step_new (project_undo_do_deactivate, project_undo_do_deactivate_free, 1);
      ustep->data[0].v_pointer = bse_undo_pointer_pack (self, ustack);
      bse_undo_stack_push_add_on (ustack, ustep);
      bse_item_undo_close (ustack);
      self->in_undo = in_undo;              /* swap undo<=>redo */
    }
}

static void
bse_project_add_item (BseContainer *container,
		      BseItem      *item)
{
  BseProject *self = BSE_PROJECT (container);

  if (BSE_IS_SUPER (item))
    self->supers = g_slist_append (self->supers, item);
  else
    self->items = g_slist_append (self->items, item);

  /* chain parent class' add_item handler */
  BSE_CONTAINER_CLASS (parent_class)->add_item (container, item);
}

static void
bse_project_remove_item (BseContainer *container,
			 BseItem      *item)
{
  BseProject *self = BSE_PROJECT (container);

  /* chain parent class' remove_item handler */
  BSE_CONTAINER_CLASS (parent_class)->remove_item (container, item);

  if (BSE_IS_SUPER (item))
    self->supers = g_slist_remove (self->supers, item);
  else
    self->items = g_slist_remove (self->items, item);
}

static void
bse_project_forall_items (BseContainer      *container,
			  BseForallItemsFunc func,
			  gpointer           data)
{
  BseProject *self = BSE_PROJECT (container);
  GSList *slist;

  slist = self->supers;
  while (slist)
    {
      BseItem *item = (BseItem*) slist->data;
      slist = slist->next;
      if (!func (item, data))
	return;
    }

  slist = self->items;
  while (slist)
    {
      BseItem *item = (BseItem*) slist->data;
      slist = slist->next;
      if (!func (item, data))
	return;
    }
}

static BseItem*
bse_project_retrieve_child (BseContainer *container,
			    GType         child_type,
			    const gchar  *uname)
{
  BseProject *self = BSE_PROJECT (container);

  /* always hand out the same wave repo */
  if (g_type_is_a (child_type, BSE_TYPE_WAVE_REPO))
    {
      GSList *slist;

      for (slist = self->supers; slist; slist = slist->next)
	if (g_type_is_a (G_OBJECT_TYPE (slist->data), BSE_TYPE_WAVE_REPO))
	  return (BseItem*) slist->data;
      g_warning ("%s: no wave-repo found in project\n", G_STRLOC);
      return NULL;	/* shouldn't happen */
    }
  else
    {
      BseItem *item = BSE_CONTAINER_CLASS (parent_class)->retrieve_child (container, child_type, uname);
      StorageTrap *strap = (StorageTrap*) g_object_get_qdata ((GObject*) self, quark_storage_trap);
      if (item && strap)
	{
          if (strap->intern_children)
            bse_item_set_internal (item, TRUE);
	  strap->items = g_slist_prepend (strap->items, item);
	  strap->max_items--;
	}
      return item;
    }
}

static GSList*
compute_missing_supers (BseProject *self,
                        BseStorage *storage)
{
  BseItem *project_item = BSE_ITEM (self);
  GSList *targets = NULL, *missing = sfi_ppool_slist (storage->referenced_items);
  while (missing)
    {
      BseItem *item = (BseItem*) g_slist_pop_head (&missing);
      BseSuper *super = bse_item_get_super (item);
      if (BSE_ITEM (super)->parent == project_item &&
          !sfi_ppool_lookup (storage->stored_items, super))
        targets = g_slist_prepend (targets, super);
    }
  return targets;
}

Bse::Error
bse_project_store_bse (BseProject  *self,
                       BseSuper    *super,
		       const gchar *bse_file,
		       gboolean     self_contained)
{
  BseStorage *storage;
  GSList *slist = NULL;
  gchar *string;
  guint l, flags;
  gint fd;

  assert_return (BSE_IS_PROJECT (self), Bse::Error::INTERNAL);
  if (super)
    {
      assert_return (BSE_IS_SUPER (super), Bse::Error::INTERNAL);
      assert_return (BSE_ITEM (super)->parent == BSE_ITEM (self), Bse::Error::INTERNAL);
    }
  assert_return (bse_file != NULL, Bse::Error::INTERNAL);

  fd = open (bse_file, O_WRONLY | O_CREAT | O_EXCL, 0666);
  if (fd < 0)
    return bse_error_from_errno (errno, Bse::Error::FILE_OPEN_FAILED);

  storage = (BseStorage*) bse_object_new (BSE_TYPE_STORAGE, NULL);
  flags = 0;
  if (self_contained)
    flags |= BSE_STORAGE_SELF_CONTAINED;
  bse_storage_prepare_write (storage, BseStorageMode (flags));

  slist = g_slist_prepend (slist, super ? (void*) super : (void*) self);
  while (slist)
    {
      BseItem *item = (BseItem*) g_slist_pop_head (&slist);
      if (item == (BseItem*) self)
        bse_storage_store_item (storage, item);
      else
        bse_storage_store_child (storage, item);
      slist = g_slist_concat (compute_missing_supers (self, storage), slist);
    }

  string = g_strdup_format ("; BseProject\n\n"); /* %010o mflags */
  do
    l = write (fd, string, strlen (string));
  while (l < 0 && errno == EINTR);
  g_free (string);

  Bse::Error error = bse_storage_flush_fd (storage, fd);
  if (close (fd) < 0 && error == Bse::Error::NONE)
    error = bse_error_from_errno (errno, Bse::Error::FILE_WRITE_FAILED);
  bse_storage_reset (storage);
  g_object_unref (storage);

  return error;
}

Bse::Error
bse_project_restore (BseProject *self,
		     BseStorage *storage)
{
  GScanner *scanner;
  GTokenType expected_token = G_TOKEN_NONE;

  assert_return (BSE_IS_PROJECT (self), Bse::Error::INTERNAL);
  assert_return (BSE_IS_STORAGE (storage), Bse::Error::INTERNAL);

  scanner = bse_storage_get_scanner (storage);
  assert_return (scanner != NULL, Bse::Error::INTERNAL);

  g_object_ref (self);

  expected_token = bse_storage_restore_item (storage, BSE_ITEM (self));
  if (expected_token != G_TOKEN_NONE)
    bse_storage_unexp_token (storage, expected_token);

  bse_storage_finish_parsing (storage);

  GSList *slist = self->supers;
  while (slist)
    {
      BseSuper *super = (BseSuper*) slist->data;
      slist = slist->next;
      BseSuperClass *super_class = BSE_SUPER_GET_CLASS (super);
      super_class->compat_finish (super, storage->major_version, storage->minor_version, storage->micro_version);
    }

  bse_undo_stack_force_dirty (self->undo_stack);

  g_object_unref (self);

  return (scanner->parse_errors >= scanner->max_parse_errors ?
	  Bse::Error::PARSE_ERROR :
	  Bse::Error::NONE);
}

BseObject*
bse_project_upath_resolver (gpointer     func_data,
			    GType        required_type,
			    const gchar *upath,
			    gchar      **error_p)
{
  BseProject *self = (BseProject*) func_data;
  gpointer item = NULL;

  if (error_p)
    *error_p = NULL;
  assert_return (BSE_IS_PROJECT (self), NULL);
  assert_return (upath != NULL, NULL);

  /* FIXME: need error handling, warnings.... */

  if (g_type_is_a (required_type, BSE_TYPE_ITEM))
    item = bse_container_resolve_upath (BSE_CONTAINER (self), upath);
  else if (error_p)
    *error_p = g_strdup_format ("unable to resolve object of type `%s' from upath: %s", g_type_name (required_type), upath);

  return (BseObject*) item;
}

BseItem*
bse_project_lookup_typed_item (BseProject  *self,
			       GType	    item_type,
			       const gchar *uname)
{
  BseItem *item;

  assert_return (BSE_IS_PROJECT (self), NULL);
  assert_return (uname != NULL, NULL);

  item = bse_container_lookup_item (BSE_CONTAINER (self), uname);
  if (item && G_OBJECT_TYPE (item) == item_type)
    return item;

  return NULL;
}

BseWaveRepo*
bse_project_get_wave_repo (BseProject *self)
{
  assert_return (BSE_IS_PROJECT (self), NULL);
  GSList *slist;
  for (slist = self->supers; slist; slist = slist->next)
    if (BSE_IS_WAVE_REPO (slist->data))
      return (BseWaveRepo*) slist->data;
  return NULL;
}

BseSong*
bse_project_get_song (BseProject *self)
{
  assert_return (BSE_IS_PROJECT (self), NULL);
  GSList *slist;
  for (slist = self->supers; slist; slist = slist->next)
    if (BSE_IS_SONG (slist->data))
      return (BseSong*) slist->data;
  return NULL;
}

static gboolean
project_check_restore (BseContainer *container,
		       const gchar  *child_type)
{
  if (BSE_CONTAINER_CLASS (parent_class)->check_restore (container, child_type))
    {
      StorageTrap *strap = (StorageTrap*) g_object_get_qdata ((GObject*) container, quark_storage_trap);
      if (!strap)
	return TRUE;
      if (!g_type_is_a (g_type_from_name (child_type), strap->base_type))
	return FALSE;
      if (strap->max_items < 1)
	return FALSE;
      return TRUE;
    }
  else
    return FALSE;
}

BseSNet*
bse_project_create_intern_synth (BseProject  *self,
				 const gchar *synth_name,
				 GType        check_type)
{
  BseItem *synth = NULL;
  gchar *bse_synth;

  assert_return (BSE_IS_PROJECT (self), NULL);
  assert_return (synth_name != NULL, NULL);

  bse_synth = bse_standard_synth_inflate (synth_name, NULL);
  if (bse_synth)
    {
      BseStorage *storage = (BseStorage*) bse_object_new (BSE_TYPE_STORAGE, NULL);
      Bse::Error error = Bse::Error::NONE;
      StorageTrap strap = { 0, TRUE, }, *old_strap = (StorageTrap*) g_object_get_qdata ((GObject*) self, quark_storage_trap);
      bse_storage_input_text (storage, bse_synth, "<builtin-lib>");
      g_object_set_qdata ((GObject*) self, quark_storage_trap, &strap);
      strap.max_items = 1;
      strap.base_type = check_type;
      strap.items = NULL;
      if (error == 0)
	error = bse_project_restore (self, storage);
      bse_storage_reset (storage);
      g_object_unref (storage);
      g_free (bse_synth);
      if (error != 0 || !strap.items)
	g_warning ("failed to create internal synth \"%s\": %s",
		   synth_name, bse_error_blurb (error != 0 ? error : Bse::Error::NO_ENTRY));
      else
	synth = (BseItem*) strap.items->data;
      g_slist_free (strap.items);
      g_object_set_qdata ((GObject*) self, quark_storage_trap, old_strap);
    }
  return BSE_SNET (synth);
}

BseCSynth*
bse_project_create_intern_csynth (BseProject *self,
                                  const char *base_name)
{
  BseCSynth *csynth = (BseCSynth*) bse_container_new_child_bname (BSE_CONTAINER (self), BSE_TYPE_CSYNTH, base_name, NULL);
  bse_item_set_internal (BSE_ITEM (csynth), TRUE);
  return csynth;
}

BseMidiNotifier*
bse_project_get_midi_notifier (BseProject *self)
{
  GSList *slist;
  for (slist = self->items; slist; slist = slist->next)
    if (BSE_IS_MIDI_NOTIFIER (slist->data))
      return (BseMidiNotifier*) slist->data;

  BseMidiNotifier *mnot = (BseMidiNotifier*) bse_container_new_child_bname (BSE_CONTAINER (self), BSE_TYPE_MIDI_NOTIFIER,
                                                                            "%bse-intern-midi-notifier", NULL);
  bse_midi_notifier_set_receiver (mnot, self->midi_receiver);
  bse_item_set_internal (BSE_ITEM (mnot), TRUE);
  return mnot;
}

static void
bse_project_prepare (BseSource *source)
{
  BseProject *self = BSE_PROJECT (source);
  GSList *slist;

  /* make sure Wave repositories are prepared first */
  for (slist = self->supers; slist; slist = slist->next)
    if (BSE_IS_WAVE_REPO (slist->data))
      bse_source_prepare ((BseSource*) slist->data);

  /* chain parent class' handler to prepare the rest */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

static gboolean
auto_deactivate (gpointer data)
{
  BseProject *self = BSE_PROJECT (data);
  self->deactivate_timer = 0;
  if (self->state == Bse::ProjectState::ACTIVE)
    bse_project_deactivate (self);
  return FALSE;
}

void
bse_project_state_changed (BseProject *self, Bse::ProjectState state)
{
  assert_return (BSE_IS_PROJECT (self));

  if (self->deactivate_timer)
    {
      bse_idle_remove (self->deactivate_timer);
      self->deactivate_timer = 0;
    }
  self->state = state;
  if (self->state == Bse::ProjectState::ACTIVE && self->deactivate_usecs >= 0)
    {
      SfiTime stamp = Bse::TickStamp::current();
      SfiTime delay_usecs = 0;
      if (SfiTime (self->deactivate_min_tick) > stamp)
	delay_usecs = (self->deactivate_min_tick - stamp) * 1000000 / bse_engine_sample_freq ();
      self->deactivate_timer = bse_idle_timed (self->deactivate_usecs + delay_usecs, auto_deactivate, self);
    }
  self->as<Bse::ProjectIface*>()->sig_state_changed.emit (state);
}

void
bse_project_keep_activated (BseProject *self,
			    guint64     min_tick)
{
  assert_return (BSE_IS_PROJECT (self));

  if (min_tick > self->deactivate_min_tick)
    {
      self->deactivate_min_tick = min_tick;
      if (self->deactivate_timer)
	bse_project_state_changed (self, self->state);
    }
}

Bse::Error
bse_project_activate (BseProject *self)
{
  Bse::Error error;
  BseTrans *trans;
  GSList *slist;

  assert_return (BSE_IS_PROJECT (self), Bse::Error::INTERNAL);

  if (self->state != Bse::ProjectState::INACTIVE)
    return Bse::Error::NONE;

  assert_return (BSE_SOURCE_PREPARED (self) == FALSE, Bse::Error::INTERNAL);

  error = bse_server_open_devices (bse_server_get ());
  if (error != 0)
    return error;

  bse_source_prepare (BSE_SOURCE (self));
  self->deactivate_min_tick = 0;

  trans = bse_trans_open ();
  for (slist = self->supers; slist; slist = slist->next)
    {
      BseSuper *super = BSE_SUPER (slist->data);
      if (BSE_SUPER_NEEDS_CONTEXT (super))
	{
          BseMidiContext mcontext = { 0, 0, 0 };
	  BseSNet *snet = BSE_SNET (super);
          mcontext.midi_receiver = self->midi_receiver;
          mcontext.midi_channel = 1; /* midi channel default */
	  super->context_handle = bse_snet_create_context (snet, mcontext, trans);
	  bse_source_connect_context (BSE_SOURCE (snet), super->context_handle, trans);
	}
      else
	super->context_handle = ~0;
    }
  bse_trans_commit (trans);
  bse_project_state_changed (self, Bse::ProjectState::ACTIVE);
  return Bse::Error::NONE;
}

void
bse_project_start_playback (BseProject *self)
{
  BseTrans *trans;
  GSList *slist;
  guint seen_synth = 0;

  assert_return (BSE_IS_PROJECT (self));

  if (self->state != Bse::ProjectState::ACTIVE)
    return;
  assert_return (BSE_SOURCE_PREPARED (self) == TRUE);

  SfiRing *songs = NULL;
  trans = bse_trans_open ();
  for (slist = self->supers; slist; slist = slist->next)
    {
      BseSuper *super = BSE_SUPER (slist->data);
      if (BSE_SUPER_NEEDS_CONTEXT (super) &&
	  super->context_handle == ~uint (0))
	{
          BseMidiContext mcontext = { 0, 0, 0 };
          BseSNet *snet = BSE_SNET (super);
          mcontext.midi_receiver = self->midi_receiver;
          mcontext.midi_channel = 1; /* midi channel default */
          super->context_handle = bse_snet_create_context (snet, mcontext, trans);
	  bse_source_connect_context (BSE_SOURCE (snet), super->context_handle, trans);
	}
      if (BSE_SUPER_NEEDS_CONTEXT (super))
        seen_synth++;
      if (BSE_IS_SONG (super))
	songs = sfi_ring_append (songs, super);
    }
  /* enfore MasterThread roundtrip */
  bse_trans_add (trans, bse_job_nop());
  bse_trans_commit (trans);
  /* first, enforce integrated (and possibly scheduled) modules; */
  bse_engine_wait_on_trans();
  /* update state */
  if (seen_synth || songs)
    bse_project_state_changed (self, Bse::ProjectState::PLAYING);
  /* then, start the sequencer */
  while (songs)
    Bse::Sequencer::instance().start_song ((BseSong*) sfi_ring_pop_head (&songs), 0);
}

void
bse_project_stop_playback (BseProject *self)
{
  BseTrans *trans;
  GSList *slist;

  assert_return (BSE_IS_PROJECT (self));

  if (self->state != Bse::ProjectState::PLAYING)
    return;
  assert_return (BSE_SOURCE_PREPARED (self) == TRUE);

  trans = bse_trans_open ();
  for (slist = self->supers; slist; slist = slist->next)
    {
      BseSuper *super = BSE_SUPER (slist->data);
      if (BSE_IS_SONG (super))
        Bse::Sequencer::instance().remove_song (BSE_SONG (super));
      if (super->context_handle != ~uint (0) && BSE_SUPER_NEEDS_CONTEXT (super))
	{
	  BseSource *source = BSE_SOURCE (super);
	  bse_source_dismiss_context (source, super->context_handle, trans);
	  super->context_handle = ~0;
	}
    }
  /* enfore MasterThread roundtrip */
  bse_trans_add (trans, bse_job_nop());
  bse_trans_commit (trans);
  /* wait until after all modules have actually been dismissed */
  bse_engine_wait_on_trans ();
  /* update state */
  bse_project_state_changed (self, Bse::ProjectState::ACTIVE);
}

void
bse_project_check_auto_stop (BseProject *self)
{
  assert_return (BSE_IS_PROJECT (self));

  if (self->state == Bse::ProjectState::PLAYING)
    {
      GSList *slist;
      for (slist = self->supers; slist; slist = slist->next)
	{
	  BseSuper *super = BSE_SUPER (slist->data);
	  if (super->context_handle != ~uint (0))
	    {
	      if (!BSE_IS_SONG (super) || !BSE_SONG (super)->sequencer_done_SL)
		return;
	    }
	}
      bse_project_stop_playback (self);
    }
}

void
bse_project_deactivate (BseProject *self)
{
  BseTrans *trans;
  GSList *slist;

  assert_return (BSE_IS_PROJECT (self));

  if (self->state == Bse::ProjectState::INACTIVE)
    return;
  assert_return (BSE_SOURCE_PREPARED (self) == TRUE);

  bse_project_stop_playback (self);

  trans = bse_trans_open ();
  for (slist = self->supers; slist; slist = slist->next)
    {
      BseSuper *super = BSE_SUPER (slist->data);
      if (super->context_handle != ~uint (0))
	{
	  BseSource *source = BSE_SOURCE (super);
	  bse_source_dismiss_context (source, super->context_handle, trans);
	  super->context_handle = ~0;
	}
    }
  bse_trans_commit (trans);
  /* wait until after all modules have actually been dismissed */
  bse_engine_wait_on_trans ();
  bse_source_reset (BSE_SOURCE (self));
  bse_project_state_changed (self, Bse::ProjectState::INACTIVE);

  bse_server_close_devices (bse_server_get ());
}

namespace Bse {

ProjectImpl::ProjectImpl (BseObject *bobj) :
  ContainerImpl (bobj)
{}

ProjectImpl::~ProjectImpl ()
{}

void
ProjectImpl::change_name (const String &name)
{
  BseProject *self = as<BseProject*>();
  g_object_set (self, "uname", name.c_str(), NULL); /* no undo */
}

Error
ProjectImpl::play ()
{
  BseProject *self = as<BseProject*>();
  Bse::ProjectState state_before = self->state;
  Bse::Error error = bse_project_activate (self);
  if (error == 0)
    {
      if (self->state == Bse::ProjectState::PLAYING)
        bse_project_stop_playback (self);
      bse_project_start_playback (self);
    }
  if (state_before == Bse::ProjectState::INACTIVE && self->state != Bse::ProjectState::INACTIVE)
    {
      // some things work only (can only be undone) in deactivated projects
      bse_project_push_undo_silent_deactivate (self);
    }
  return Bse::Error (error);
}

Error
ProjectImpl::activate ()
{
  BseProject *self = as<BseProject*>();
  Bse::ProjectState state_before = self->state;
  Bse::Error error = bse_project_activate (self);
  if (state_before == Bse::ProjectState::INACTIVE && self->state != Bse::ProjectState::INACTIVE)
    {
      // some things work only (can only be undone) in deactivated projects
      bse_project_push_undo_silent_deactivate (self);
    }
  return Bse::Error (error);
}

bool
ProjectImpl::can_play ()
{
  BseProject *self = as<BseProject*>();
  /* playback works if we have supers other than wave repo */
  gpointer wrepo = bse_project_get_wave_repo (self);
  return self->supers->data != wrepo || self->supers->next;
}

bool
ProjectImpl::is_playing ()
{
  BseProject *self = as<BseProject*>();
  return self->state == Bse::ProjectState::PLAYING;
}

bool
ProjectImpl::is_active ()
{
  BseProject *self = as<BseProject*>();
  return self->state != Bse::ProjectState::INACTIVE;
}

void
ProjectImpl::start_playback ()
{
  BseProject *self = as<BseProject*>();
  Bse::ProjectState state_before = self->state;
  bse_project_start_playback (self);
  if (state_before == Bse::ProjectState::INACTIVE && self->state != Bse::ProjectState::INACTIVE)
    {
      // some things work only (can only be undone) in deactivated projects
      bse_project_push_undo_silent_deactivate (self);
    }
}

void
ProjectImpl::stop_playback ()
{
  BseProject *self = as<BseProject*>();
  bse_project_stop_playback (self);
}

void
ProjectImpl::deactivate ()
{
  BseProject *self = as<BseProject*>();
  bse_project_deactivate (self);
}

void
ProjectImpl::stop ()
{
  BseProject *self = as<BseProject*>();
  bse_project_deactivate (self);
}

void
ProjectImpl::auto_deactivate (int msec_delay)
{
  BseProject *self = as<BseProject*>();
  self->deactivate_usecs = msec_delay < 0 ? -1 : msec_delay * 1000;
}

int
ProjectImpl::undo_depth ()
{
  BseProject *self = as<BseProject*>();
  return bse_undo_stack_depth (self->undo_stack);
}

void
ProjectImpl::undo ()
{
  BseProject *self = as<BseProject*>();
  if (!self->in_undo && !self->in_redo)
    {
      const gchar *name = bse_undo_stack_peek (self->undo_stack);
      if (name)
        {
          self->in_undo = true;         // swap undo<=>redo
          bse_undo_group_open (self->redo_stack, name);
          bse_undo_stack_undo (self->undo_stack);
          bse_undo_group_close (self->redo_stack);
          self->in_undo = false;        // swap undo<=>redo
        }
    }
}

int
ProjectImpl::redo_depth ()
{
  BseProject *self = as<BseProject*>();
  return bse_undo_stack_depth (self->redo_stack);
}

void
ProjectImpl::redo ()
{
  BseProject *self = as<BseProject*>();
  if (!self->in_undo && !self->in_redo)
    {
      const gchar *name = bse_undo_stack_peek (self->redo_stack);
      if (name)
        {
          self->in_redo = true;         // disable redo-stack clearing
          bse_undo_group_open (self->undo_stack, name);
          bse_undo_stack_undo (self->redo_stack);
          bse_undo_group_close (self->undo_stack);
          self->in_redo = false;        // enable redo-stack clearing
        }
    }
}

void
ProjectImpl::clear_undo ()
{
  BseProject *self = as<BseProject*>();
  bse_project_clear_undo (self);
}

void
ProjectImpl::clean_dirty ()
{
  BseProject *self = as<BseProject*>();
  bse_project_clean_dirty (self);
}

bool
ProjectImpl::is_dirty ()
{
  BseProject *self = as<BseProject*>();
  gboolean dirty = false;
  g_object_get (self, "dirty", &dirty, NULL);
  return dirty;
}

void
ProjectImpl::inject_midi_control (int midi_channel, int midi_control, double control_value)
{
  BseProject *self = as<BseProject*>();
  if (BSE_SOURCE_PREPARED (self))
    {
      // construct event
      BseMidiEvent *event = bse_midi_alloc_event ();
      event->status = BSE_MIDI_CONTROL_CHANGE;
      event->channel = midi_channel;
      event->delta_time = bse_engine_tick_stamp_from_systime (sfi_time_system ());
      // midi control data portion
      event->data.control.control = midi_control;
      event->data.control.value = control_value;
      // send event
      bse_midi_receiver_push_event (self->midi_receiver, event);
      bse_midi_receiver_process_events (self->midi_receiver, event->delta_time);
    }
}

Error
ProjectImpl::import_midi_file (const String &file_name)
{
  BseProject *self = as<BseProject*>();
  Bse::Error error = Bse::Error::NONE;
  BseMidiFile *smf = bse_midi_file_load (file_name.c_str(), &error);
  if (error == 0)
    {
      BseUndoStack *ustack = bse_item_undo_open (self, "import-midi-file");
      BseSong *song = bse_project_get_song (self);
      if (!song)
	{
	  gchar *basename = g_path_get_basename (file_name.c_str());
	  bse_item_exec (self, "create-song", basename, &song);
	  g_free (basename);
	}
      bse_midi_file_setup_song (smf, song);
      bse_item_undo_close (ustack);
      bse_project_clear_undo (self); // FIXME: why can't we undo MIDI imports?
    }
  if (smf)
    bse_midi_file_free (smf);
  return Bse::Error (error);
}

Error
ProjectImpl::restore_from_file (const String &file_name)
{
  BseProject *self = as<BseProject*>();
  Bse::Error error;
  if (!self->in_undo && !self->in_redo)
    {
      BseStorage *storage = (BseStorage*) bse_object_new (BSE_TYPE_STORAGE, NULL);
      error = bse_storage_input_file (storage, file_name.c_str());
      if (error == 0)
        error = bse_project_restore (self, storage);
      bse_storage_reset (storage);
      g_object_unref (storage);
      bse_project_clear_undo (self);
    }
  else
    error = Bse::Error::PROC_BUSY;
  return Bse::Error (error);
}

ProjectState
ProjectImpl::get_state ()
{
  BseProject *self = as<BseProject*>();
  return self->state;
}

} // Bse
