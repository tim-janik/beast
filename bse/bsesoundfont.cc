// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsesoundfont.hh"
#include "bsesoundfontrepo.hh"
#include "bsesoundfontpreset.hh"
#include "bsemain.hh"
#include "bsestorage.hh"
#include "gsldatahandle.hh"
#include "bseserver.hh"
#include "bseloader.hh"
#include "bse/internal.hh"
#include <string.h>

#define parse_or_return         bse_storage_scanner_parse_or_return

enum {
  PARAM_0,
  PARAM_FILE_NAME,
};

/* --- prototypes --- */


/* --- variables --- */
static void       *parent_class = NULL;
static GQuark      quark_load_sound_font = 0;


/* --- functions --- */
static void
bse_sound_font_init (BseSoundFont *sound_font)
{
}

static void
bse_sound_font_set_property (GObject      *object,
		             guint         param_id,
		             const GValue *value,
		             GParamSpec   *pspec)
{
  switch (param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
bse_sound_font_get_property (GObject    *object,
		             guint	 param_id,
		             GValue     *value,
		             GParamSpec *pspec)
{
  BseSoundFont *sound_font = BSE_SOUND_FONT (object);
  Bse::SoundFontImpl *sound_font_impl = sound_font->as<Bse::SoundFontImpl *>();
  switch (param_id)
    {
    case PARAM_FILE_NAME:
      if (sound_font_impl->blob)
        sfi_value_set_string (value, sound_font_impl->blob->file_name().c_str());
      else
        sfi_value_set_string (value, NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
bse_sound_font_dispose (GObject *object)
{
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
bse_sound_font_finalize (GObject *object)
{
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static std::string
use_searchpath (std::string file_name)
{
  /* abolute path: do nothing */
  if (g_path_is_absolute (file_name.c_str()))
    return file_name;

  /* resolve relative path using search dir */
  std::string found_file;
  SfiRing *files, *walk;
  std::string sample_path = Bse::config_string ("override-sample-path");
  if (sample_path.empty())
    sample_path = Bse::Path::searchpath_join (Bse::runpath (Bse::RPath::SAMPLEDIR), Bse::global_prefs->sample_path);
  files = sfi_file_crawler_list_files (sample_path.c_str(), file_name.c_str(), G_FILE_TEST_IS_REGULAR);

  for (walk = files; walk; walk = sfi_ring_walk (files, walk))
    {
      char *fname = (char*) walk->data;
      if (found_file.empty()) // we don't really try loading it here
        found_file = fname;
      g_free (fname);
    }
  sfi_ring_free (files);
  return found_file.empty() ? file_name : found_file;
}

Bse::Error
bse_sound_font_load_blob (BseSoundFont       *self,
                          BseStorage::BlobP   blob,
			  gboolean            init_presets)
{
  Bse::SoundFontImpl *sound_font_impl = self->as<Bse::SoundFontImpl *>();

  if (sound_font_impl->sfrepo == NULL)
    {
      sound_font_impl->sfrepo = BSE_SOUND_FONT_REPO (BSE_ITEM (self)->parent);
      g_object_ref (sound_font_impl->sfrepo);
    }

  assert_return (blob != NULL, Bse::Error::INTERNAL);
  assert_return (sound_font_impl->sfrepo != NULL, Bse::Error::INTERNAL);
  assert_return (sound_font_impl->sfont_id == -1, Bse::Error::INTERNAL);

  fluid_synth_t *fluid_synth = bse_sound_font_repo_fluid_synth (sound_font_impl->sfrepo);
  int sfont_id = fluid_synth_sfload (fluid_synth, use_searchpath (blob->file_name()).c_str(), 0);
  Bse::Error error;
  if (sfont_id != -1)
    {
      if (init_presets)
	{
	  fluid_sfont_t *fluid_sfont = fluid_synth_get_sfont_by_id (fluid_synth, sfont_id);

	  fluid_sfont_iteration_start (fluid_sfont);
	  while (fluid_preset_t *fluid_preset = fluid_sfont_iteration_next (fluid_sfont))
	    {
	      BseSoundFontPreset *sound_font_preset;
              sound_font_preset = (BseSoundFontPreset *) bse_object_new (BSE_TYPE_SOUND_FONT_PRESET,
								         "uname", fluid_preset_get_name (fluid_preset),
								         NULL);
	      bse_container_add_item (BSE_CONTAINER (self), BSE_ITEM (sound_font_preset));
	      bse_sound_font_preset_init_preset (sound_font_preset, fluid_preset);
	    }
	}
      sound_font_impl->sfont_id = sfont_id;
      sound_font_impl->blob = blob;
      error = Bse::Error::NONE;
    }
  else
    {
      sound_font_impl->blob = nullptr;
      error = Bse::Error::WAVE_NOT_FOUND;
    }
  return error;
}

void
bse_sound_font_unload (BseSoundFont *sound_font)
{
  Bse::SoundFontImpl *sound_font_impl = sound_font->as<Bse::SoundFontImpl *>();

  assert_return (sound_font_impl->sfrepo != NULL);

  if (sound_font_impl->sfont_id != -1)
    {
      fluid_synth_t *fluid_synth = bse_sound_font_repo_fluid_synth (sound_font_impl->sfrepo);

      fluid_synth_sfunload (fluid_synth, sound_font_impl->sfont_id, 1 /* reset presets */);
    }
  sound_font_impl->sfont_id = -1;
}

Bse::Error
bse_sound_font_reload (BseSoundFont *sound_font)
{
  Bse::SoundFontImpl *sound_font_impl = sound_font->as<Bse::SoundFontImpl *>();

  assert_return (sound_font_impl->sfont_id == -1, Bse::Error::INTERNAL);

  return bse_sound_font_load_blob (sound_font, sound_font_impl->blob, FALSE);
}

std::string
bse_sound_font_get_filename (BseSoundFont *sound_font)
{
  Bse::SoundFontImpl *sound_font_impl = sound_font->as<Bse::SoundFontImpl *>();

  return use_searchpath (sound_font_impl->blob->file_name());
}

static void
bse_sound_font_store_private (BseObject  *object,
			      BseStorage *storage)
{
  BseSoundFont *sound_font = BSE_SOUND_FONT (object);
  Bse::SoundFontImpl *sound_font_impl = sound_font->as<Bse::SoundFontImpl *>();
  /* chain parent class' handler */
  BSE_OBJECT_CLASS (parent_class)->store_private (object, storage);

  if (!BSE_STORAGE_SELF_CONTAINED (storage) && !sound_font_impl->blob->is_temp_file())
    {
      bse_storage_break (storage);
      bse_storage_printf (storage, "(load-sound-font \"%s\")", sound_font_impl->blob->file_name().c_str());
    }
  else
    {
      bse_storage_break (storage);
      bse_storage_printf (storage, "(load-sound-font ");
      bse_storage_put_blob (storage, sound_font_impl->blob);
      bse_storage_printf (storage, ")");
    }
}

static GTokenType
bse_sound_font_restore_private (BseObject  *object,
			        BseStorage *storage,
                                GScanner   *scanner)
{
  BseSoundFont *sound_font = BSE_SOUND_FONT (object);
  GTokenType expected_token;
  GQuark quark;

  /* chain parent class' handler */
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return BSE_OBJECT_CLASS (parent_class)->restore_private (object, storage, scanner);

  /* parse storage commands */
  quark = g_quark_try_string (scanner->next_value.v_identifier);
  if (quark == quark_load_sound_font)
    {
      BseStorage::BlobP blob;
      Bse::Error error;

      g_scanner_get_next_token (scanner); /* eat quark identifier */
      if (g_scanner_peek_next_token (scanner) == G_TOKEN_STRING)
	{
	  parse_or_return (scanner, G_TOKEN_STRING);
	  blob = std::make_shared<BseStorage::Blob> (scanner->value.v_string, false);
	}
      else
	{
          GTokenType token = bse_storage_parse_blob (storage, blob);
	  if (token != G_TOKEN_NONE)
            return token;
	}
      if (g_scanner_peek_next_token (scanner) != ')')
	{
	  return GTokenType (')');
	}
      parse_or_return (scanner, ')');
      error = bse_sound_font_load_blob (sound_font, blob, FALSE);
      if (error != 0)
	bse_storage_warn (storage, "failed to load sound font \"%s\": %s",
				    blob->file_name().c_str(), bse_error_blurb (error));
      expected_token = G_TOKEN_NONE; /* got ')' */
    }
  else /* chain parent class' handler */
    expected_token = BSE_OBJECT_CLASS (parent_class)->restore_private (object, storage, scanner);

  return expected_token;
}


static void
bse_sound_font_add_item (BseContainer *container,
			 BseItem      *item)
{
  BseSoundFont       *sound_font      = BSE_SOUND_FONT (container);
  Bse::SoundFontImpl *sound_font_impl = sound_font->as<Bse::SoundFontImpl *>();

  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_SOUND_FONT_PRESET))
    {
      sound_font_impl->presets.push_back (BSE_SOUND_FONT_PRESET (item));
    }
  else
    Bse::warning ("BseSoundFont: cannot hold non-sound-font-preset item type `%s'", BSE_OBJECT_TYPE_NAME (item));

  /* chain parent class' add_item handler */
  BSE_CONTAINER_CLASS (parent_class)->add_item (container, item);
}

static void
bse_sound_font_forall_items (BseContainer      *container,
			     BseForallItemsFunc func,
			     gpointer           data)
{
  BseSoundFont       *sound_font      = BSE_SOUND_FONT (container);
  Bse::SoundFontImpl *sound_font_impl = sound_font->as<Bse::SoundFontImpl *>();

  for (auto preset : sound_font_impl->presets)
    {
      if (!func (preset, data))
	return;
    }
}

static void
bse_sound_font_remove_item (BseContainer *container,
			    BseItem      *item)
{
  BseSoundFont       *sound_font      = BSE_SOUND_FONT (container);
  Bse::SoundFontImpl *sound_font_impl = sound_font->as<Bse::SoundFontImpl *>();

  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_SOUND_FONT_PRESET))
    {
      for (auto it = sound_font_impl->presets.begin(); it != sound_font_impl->presets.end(); it++)
        {
          if (*it == item)
            {
              sound_font_impl->presets.erase (it);
              break;
            }
        }
    }
  else
    Bse::warning ("BseSoundFontRepo: cannot hold non-sound-font-preset item type `%s'", BSE_OBJECT_TYPE_NAME (item));

  /* chain parent class' remove_item handler */
  BSE_CONTAINER_CLASS (parent_class)->remove_item (container, item);
}

static void
bse_sound_font_release_children (BseContainer *container)
{
  /* real release children: done in ~SoundFontImpl() */

  /* chain parent class' handler */
  BSE_CONTAINER_CLASS (parent_class)->release_children (container);
}

static void
bse_sound_font_class_init (BseSoundFontClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (klass);
  BseContainerClass *container_class = BSE_CONTAINER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->set_property = bse_sound_font_set_property;
  gobject_class->get_property = bse_sound_font_get_property;
  gobject_class->dispose = bse_sound_font_dispose;
  gobject_class->finalize = bse_sound_font_finalize;

  container_class->add_item = bse_sound_font_add_item;
  container_class->remove_item = bse_sound_font_remove_item;
  container_class->forall_items = bse_sound_font_forall_items;
  container_class->release_children = bse_sound_font_release_children;

  object_class->store_private = bse_sound_font_store_private;
  object_class->restore_private = bse_sound_font_restore_private;

  quark_load_sound_font = g_quark_from_static_string ("load-sound-font");

  bse_object_class_add_param (object_class, "Locator",
			      PARAM_FILE_NAME,
			      sfi_pspec_string ("file_name", "File Name", NULL,
						NULL, "G:r"));
}

BSE_BUILTIN_TYPE (BseSoundFont)
{
  static const GTypeInfo sound_font_info = {
    sizeof (BseSoundFontClass),

    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_sound_font_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,

    sizeof (BseSoundFont),
    0  /* n_preallocs */,
    (GInstanceInitFunc) bse_sound_font_init,
  };

  return bse_type_register_static (BSE_TYPE_CONTAINER,
				   "BseSoundFont",
				   "BSE sound_font type",
                                   __FILE__, __LINE__,
                                   &sound_font_info);
}

namespace Bse {

SoundFontImpl::SoundFontImpl (BseObject *bobj) :
  ContainerImpl (bobj),
  sfrepo (nullptr),
  sfont_id (-1)
{}

SoundFontImpl::~SoundFontImpl ()
{
  BseSoundFont *sound_font = as<BseSoundFont *>();

  /* release children */
  while (!presets.empty())
    bse_container_remove_item (BSE_CONTAINER (sound_font), presets.front());

  /* cleanup state */
  if (sfont_id != -1)
    bse_sound_font_unload (sound_font);

  if (sfrepo)
    {
      g_object_unref (sfrepo);
      sfrepo = nullptr;
    }

  if (sfrepo != NULL || sfont_id != -1)
    Bse::warning (G_STRLOC ": some resources could not be freed.");
}

} // Bse
