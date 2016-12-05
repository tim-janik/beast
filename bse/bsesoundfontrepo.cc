// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include        "bsesoundfontrepo.hh"
#include        "bsesoundfont.hh"
#include        "bsesoundfontpreset.hh"
#include        "bsedefs.hh"
#include        "bseblockutils.hh"


/* --- parameters --- */
enum
{
  PARAM_0,
};


/* --- prototypes --- */
static void	bse_sound_font_repo_class_init		(BseSoundFontRepoClass	*klass);
static void	bse_sound_font_repo_init		(BseSoundFontRepo	*wrepo);
static void	bse_sound_font_repo_dispose		(GObject		*object);
static void     bse_sound_font_repo_release_children    (BseContainer		*container);
static void	bse_sound_font_repo_set_property	(GObject                *object,
							 guint			 param_id,
							 const GValue		*value,
							 GParamSpec		*pspec);
static void	bse_sound_font_repo_get_property (GObject               *object,
						  guint			 param_id,
						  GValue		*value,
						  GParamSpec		*pspec);
static void     bse_sound_font_repo_add_item     (BseContainer		*container,
						  BseItem		*item);
static void     bse_sound_font_repo_forall_items (BseContainer		*container,
						  BseForallItemsFunc	 func,
						  gpointer		 data);
static void     bse_sound_font_repo_remove_item	 (BseContainer		*container,
						  BseItem		*item);
static void     bse_sound_font_repo_prepare      (BseSource             *source);


/* --- variables --- */
static gpointer parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseSoundFontRepo)
{
  GType sound_font_repo_type;

  static const GTypeInfo sfrepo_info = {
    sizeof (BseSoundFontRepoClass),

    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_sound_font_repo_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,

    sizeof (BseSoundFontRepo),
    0,
    (GInstanceInitFunc) bse_sound_font_repo_init,
  };

  sound_font_repo_type = bse_type_register_static (BSE_TYPE_SUPER,
					           "BseSoundFontRepo",
					           "BSE Sound Font Repository",
                                                   __FILE__, __LINE__,
                                                   &sfrepo_info);
  return sound_font_repo_type;
}

static void
bse_sound_font_repo_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bse_sound_font_repo_class_init (BseSoundFontRepoClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseContainerClass *container_class = BSE_CONTAINER_CLASS (klass);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->set_property = bse_sound_font_repo_set_property;
  gobject_class->get_property = bse_sound_font_repo_get_property;
  gobject_class->dispose = bse_sound_font_repo_dispose;
  gobject_class->finalize = bse_sound_font_repo_finalize;

  container_class->add_item = bse_sound_font_repo_add_item;
  container_class->remove_item = bse_sound_font_repo_remove_item;
  container_class->forall_items = bse_sound_font_repo_forall_items;
  container_class->release_children = bse_sound_font_repo_release_children;

  source_class->prepare = bse_sound_font_repo_prepare;
}

static void
bse_sound_font_repo_init (BseSoundFontRepo *sfrepo)
{
  sfrepo->fluid_settings = new_fluid_settings();
  sfrepo->fluid_synth = new_fluid_synth (sfrepo->fluid_settings);
  sfrepo->fluid_events = NULL;
  sfrepo->sound_fonts = NULL;
  sfrepo->fluid_mix_freq = 0;

  sfrepo->n_fluid_channels = 0;

  sfrepo->n_channel_oscs_active = 0;
  sfrepo->channel_values_tick_stamp = 0;
}

static gboolean
reload_sound_font (BseItem *item,
                   gpointer data)
{
  BseSoundFont *sound_font = BSE_SOUND_FONT (item);
  bse_sound_font_reload (sound_font);
  return TRUE;
}

static gboolean
unload_sound_font (BseItem *item,
                   gpointer data)
{
  BseSoundFont *sound_font = BSE_SOUND_FONT (item);
  bse_sound_font_unload (sound_font);
  return TRUE;
}

static void
bse_sound_font_repo_prepare (BseSource *source)
{
  BseSoundFontRepo *sfrepo = BSE_SOUND_FONT_REPO (source);
  Bse::SoundFontRepoImpl *sfrepo_impl = sfrepo->as<Bse::SoundFontRepoImpl *>();

  guint channels_required = 0;
  for (auto& o : sfrepo_impl->oscs)
    {
      if (o.osc)
	o.channel = channels_required++;
    }
  guint mix_freq = bse_engine_sample_freq();
  if (sfrepo->n_fluid_channels != channels_required || sfrepo->fluid_mix_freq != mix_freq)
    {
      sfrepo_impl->channel_state.resize (channels_required);

      for (auto& cstate : sfrepo_impl->channel_state)
        {
          cstate.n_silence_samples = 0;
          cstate.values_left.resize (BSE_STREAM_MAX_VALUES);
          cstate.values_right.resize (BSE_STREAM_MAX_VALUES);
        }

      sfrepo->n_fluid_channels = channels_required;
      sfrepo->fluid_mix_freq = mix_freq;

      fluid_settings_setnum (sfrepo->fluid_settings, "synth.sample-rate", mix_freq);
      /* soundfont instruments should be as loud as beast synthesis network instruments */
      fluid_settings_setnum (sfrepo->fluid_settings, "synth.gain", 1.0);
      fluid_settings_setint (sfrepo->fluid_settings, "synth.midi-channels", channels_required);
      fluid_settings_setint (sfrepo->fluid_settings, "synth.audio-channels", channels_required);
      fluid_settings_setint (sfrepo->fluid_settings, "synth.audio-groups", channels_required);
      fluid_settings_setstr (sfrepo->fluid_settings, "synth.reverb.active", "no");
      fluid_settings_setstr (sfrepo->fluid_settings, "synth.chorus.active", "no");

      bse_sound_font_repo_forall_items (BSE_CONTAINER (sfrepo), unload_sound_font, sfrepo);
      if (sfrepo->fluid_synth)
	delete_fluid_synth (sfrepo->fluid_synth);

      sfrepo->fluid_synth = new_fluid_synth (sfrepo->fluid_settings);
      bse_sound_font_repo_forall_items (BSE_CONTAINER (sfrepo), reload_sound_font, sfrepo);
    }

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

static void
bse_sound_font_repo_release_children (BseContainer *container)
{
  BseSoundFontRepo *sfrepo = BSE_SOUND_FONT_REPO (container);

  while (sfrepo->sound_fonts)
    bse_container_remove_item (container, BSE_ITEM (sfrepo->sound_fonts->data));

  /* chain parent class' handler */
  BSE_CONTAINER_CLASS (parent_class)->release_children (container);
}

static void
bse_sound_font_repo_dispose (GObject *object)
{
  BseSoundFontRepo *sfrepo = BSE_SOUND_FONT_REPO (object);

  bse_sound_font_repo_forall_items (BSE_CONTAINER (sfrepo), unload_sound_font, sfrepo);

  if (sfrepo->fluid_synth)
    {
      delete_fluid_synth (sfrepo->fluid_synth);
      sfrepo->fluid_synth = NULL;
    }
  if (sfrepo->fluid_settings)
    {
      delete_fluid_settings (sfrepo->fluid_settings);
      sfrepo->fluid_settings = NULL;
    }
  sfrepo->n_fluid_channels = 0;

  if (sfrepo->fluid_events != NULL)
    g_warning (G_STRLOC ": fluid event queue should be empty in dispose");

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
bse_sound_font_repo_set_property (GObject      *object,
			          guint         param_id,
			          const GValue *value,
			          GParamSpec   *pspec)
{
  BseSoundFontRepo *sfrepo = BSE_SOUND_FONT_REPO (object);
  switch (param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (sfrepo, param_id, pspec);
      break;
    }
}

static void
bse_sound_font_repo_get_property (GObject      *object,
			          guint         param_id,
			          GValue       *value,
			          GParamSpec   *pspec)
{
  BseSoundFontRepo *sfrepo = BSE_SOUND_FONT_REPO (object);
  switch (param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (sfrepo, param_id, pspec);
      break;
    }
}

static void
bse_sound_font_repo_add_item (BseContainer *container,
			      BseItem      *item)
{
  BseSoundFontRepo *sfrepo = BSE_SOUND_FONT_REPO (container);

  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_SOUND_FONT))
    sfrepo->sound_fonts = g_list_append (sfrepo->sound_fonts, item);
  else
    g_warning ("BseSoundFontRepo: cannot hold non-sound-font item type `%s'",
	       BSE_OBJECT_TYPE_NAME (item));

  /* chain parent class' add_item handler */
  BSE_CONTAINER_CLASS (parent_class)->add_item (container, item);
}

static void
bse_sound_font_repo_forall_items (BseContainer      *container,
			          BseForallItemsFunc func,
			          gpointer           data)
{
  BseSoundFontRepo *sfrepo = BSE_SOUND_FONT_REPO (container);
  GList *list;

  list = sfrepo->sound_fonts;
  while (list)
    {
      BseItem *item;

      item = BSE_ITEM (list->data);
      list = list->next;
      if (!func (item, data))
	return;
    }
}

static void
bse_sound_font_repo_remove_item (BseContainer *container,
			         BseItem      *item)
{
  BseSoundFontRepo *sfrepo = BSE_SOUND_FONT_REPO (container);

  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_SOUND_FONT))
    sfrepo->sound_fonts = g_list_remove (sfrepo->sound_fonts, item);
  else
    g_warning ("BseSoundFontRepo: cannot hold non-sound-font item type `%s'",
	       BSE_OBJECT_TYPE_NAME (item));

  /* chain parent class' remove_item handler */
  BSE_CONTAINER_CLASS (parent_class)->remove_item (container, item);
}

static gboolean
gather_presets (BseItem    *item,
                void       *pitems)
{
  BseIt3mSeq *items = (BseIt3mSeq *) pitems;
  if (BSE_IS_SOUND_FONT (item) || BSE_IS_SOUND_FONT_REPO (item))
    bse_container_forall_items (BSE_CONTAINER (item), gather_presets, items);
  else if (BSE_IS_SOUND_FONT_PRESET (item))
    bse_it3m_seq_append (items, item);
  else
    g_warning ("Searching for sound font presets, an unexpected `%s' item was found", BSE_OBJECT_TYPE_NAME (item));
  return TRUE;
}

void
bse_sound_font_repo_list_all_presets (BseSoundFontRepo *sfrepo,
                                      BseIt3mSeq       *items)
{
  gather_presets (BSE_ITEM (sfrepo), items);
}

Bse::Mutex&
bse_sound_font_repo_mutex (BseSoundFontRepo *sfrepo)
{
  Bse::SoundFontRepoImpl *sfrepo_impl = sfrepo->as<Bse::SoundFontRepoImpl *>();
  return sfrepo_impl->fluid_synth_mutex;
}

fluid_synth_t*
bse_sound_font_repo_fluid_synth (BseSoundFontRepo *sfrepo)
{
  return sfrepo->fluid_synth;
}

int
bse_sound_font_repo_add_osc (BseSoundFontRepo *sfrepo,
                             BseSoundFontOsc  *osc)
{
  Bse::SoundFontRepoImpl *sfrepo_impl = sfrepo->as<Bse::SoundFontRepoImpl *>();
  for (guint i = 0; i < sfrepo_impl->oscs.size(); i++)
    {
      if (!sfrepo_impl->oscs[i].osc)
	{
	  sfrepo_impl->oscs[i].osc = osc;
	  return i;
	}
    }
  sfrepo_impl->oscs.push_back ({ osc, 0 });
  return sfrepo_impl->oscs.size() - 1;
}

void
bse_sound_font_repo_remove_osc (BseSoundFontRepo *sfrepo,
                                guint             osc_id)
{
  Bse::SoundFontRepoImpl *sfrepo_impl = sfrepo->as<Bse::SoundFontRepoImpl *>();

  g_return_if_fail (osc_id < sfrepo_impl->oscs.size());

  sfrepo_impl->oscs[osc_id].osc = nullptr;
}

namespace Bse {

SoundFontRepoImpl::SoundFontRepoImpl (BseObject *bobj) :
  SuperImpl (bobj)
{}

SoundFontRepoImpl::~SoundFontRepoImpl ()
{}

static Error
repo_load_file (BseSoundFontRepo *sfrepo, const String &file_name, BseSoundFont **sound_font_p)
{
  String fname = Path::basename (file_name);
  BseSoundFont *sound_font = (BseSoundFont*) bse_object_new (BSE_TYPE_SOUND_FONT, "uname", fname.c_str(), NULL);
  bse_container_add_item (BSE_CONTAINER (sfrepo), BSE_ITEM (sound_font));

  BseStorageBlob *blob = bse_storage_blob_new_from_file (file_name.c_str(), FALSE);
  Error error = bse_sound_font_load_blob (sound_font, blob, TRUE);
  bse_storage_blob_unref (blob);

  if (error == Bse::Error::NONE)
    {
      *sound_font_p = sound_font;
      error = Error::NONE;
    }
  else
    {
      bse_container_remove_item (BSE_CONTAINER (sfrepo), BSE_ITEM (sound_font));
      *sound_font_p = NULL;
    }
  g_object_unref (sound_font);
  return error;
}


Error
SoundFontRepoImpl::load_file (const String &file_name)
{
  BseSoundFontRepo *self = as<BseSoundFontRepo*>();

  if (BSE_SOURCE_PREPARED (self))
    {
      /* In theory, its possible to allow loading sound fonts while
       * the project is playing; in practice, the sound font repo
       * lock would be locked for a very long time, which would stall
       * the audio production ...
       */
      return Bse::Error::SOURCE_BUSY;
    }

  BseSoundFont *sound_font = NULL;
  Bse::Error error = repo_load_file (self, file_name, &sound_font);
  if (sound_font)
    {
      UndoDescriptor<SoundFontImpl> sound_font_descriptor = undo_descriptor (*sound_font->as<SoundFontImpl*>());
      auto remove_sound_font_lambda = [sound_font_descriptor] (SoundFontRepoImpl &self, BseUndoStack *ustack) -> Error {
        SoundFontImpl &sound_font = self.undo_resolve (sound_font_descriptor);
        self.remove_sound_font (sound_font);
        return Error::NONE;
      };
      push_undo (__func__, *this, remove_sound_font_lambda);
    }
  return error;
}

Error
SoundFontRepoImpl::remove_sound_font (SoundFontIface &sound_font_iface)
{
  BseSoundFontRepo *self = as<BseSoundFontRepo*>();
  BseSoundFont *sound_font = sound_font_iface.as<BseSoundFont*>();

  assert_return (sound_font->parent == self, Bse::Error::INTERNAL);

  if (BSE_SOURCE_PREPARED (self))
    {
      /* Don't allow unloading sound fonts which could be required by engine
       * modules in the DSP thread currently producing audio output.
       */
      return Bse::Error::SOURCE_BUSY;
    }
  BseUndoStack *ustack = bse_item_undo_open (self, __func__);
  bse_container_uncross_undoable (self, sound_font);    // removes object references
  if (sound_font)                                       // push undo for 'remove_backedup'
    {
      UndoDescriptor<SoundFontImpl> sound_font_descriptor = undo_descriptor (*sound_font->as<SoundFontImpl*>());
      auto remove_sound_font_lambda = [sound_font_descriptor] (SoundFontRepoImpl &self, BseUndoStack *ustack) -> Error {
        SoundFontImpl &sound_font = self.undo_resolve (sound_font_descriptor);
        self.remove_sound_font (sound_font);
        return Error::NONE;
      };
      push_undo_to_redo (__func__, *this, remove_sound_font_lambda);
    }
  bse_container_remove_backedup (self, sound_font, ustack);   // removes without undo
  bse_item_undo_close (ustack);

  return Bse::Error::NONE;

}

} // Bse
