/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2005 Tim Janik
 * Copyright (C) 2009 Stefan Westerfeld
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "bsesoundfont.h"
#include "bsesoundfontrepo.h"
#include "bsesoundfontpreset.h"
#include "bsemain.h"
#include "bsestorage.h"
#include "bseprocedure.h"
#include "gsldatahandle.h"
#include "bseserver.h"
#include "bseloader.h"

#include <string.h>

#define parse_or_return         bse_storage_scanner_parse_or_return

enum {
  PARAM_0,
  PARAM_FILE_NAME,
};

/* --- prototypes --- */


/* --- variables --- */
static GTypeClass *parent_class = NULL;
static GQuark      quark_load_sound_font = 0;


/* --- functions --- */
static void
bse_sound_font_init (BseSoundFont *sound_font)
{
  sound_font->blob = NULL;
  sound_font->sfont_id = -1;
  sound_font->sfrepo = NULL;
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
  switch (param_id)
    {
    case PARAM_FILE_NAME:
      if (sound_font->blob)
        sfi_value_set_string (value, bse_storage_blob_file_name (sound_font->blob));
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
  BseSoundFont *sound_font = BSE_SOUND_FONT (object);
  if (sound_font->sfont_id != -1)
    bse_sound_font_unload (sound_font);
  if (sound_font->sfrepo)
    {
      g_object_unref (sound_font->sfrepo);
      sound_font->sfrepo = NULL;
    }
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
bse_sound_font_finalize (GObject *object)
{
  BseSoundFont *sound_font = BSE_SOUND_FONT (object);

  /* free blob */
  if (sound_font->blob)
    {
      bse_storage_blob_unref (sound_font->blob);
      sound_font->blob = NULL;
    }

  if (sound_font->sfrepo != NULL || sound_font->blob != NULL || sound_font->sfont_id != -1)
    g_warning (G_STRLOC ": some resources could not be freed.");

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

BseErrorType
bse_sound_font_load_blob (BseSoundFont    *self,
                          BseStorageBlob  *blob,
			  gboolean         init_presets)
{
  if (self->sfrepo == NULL)
    {
      self->sfrepo = BSE_SOUND_FONT_REPO (BSE_ITEM (self)->parent);
      g_object_ref (self->sfrepo);
    }

  g_return_val_if_fail (blob != NULL, BSE_ERROR_INTERNAL);
  g_return_val_if_fail (self->sfrepo != NULL, BSE_ERROR_INTERNAL);
  g_return_val_if_fail (self->sfont_id == -1, BSE_ERROR_INTERNAL);

  bse_storage_blob_ref (blob);
  if (self->blob)
    {
      bse_storage_blob_unref (self->blob);
      self->blob = NULL;
    }

  fluid_synth_t *fluid_synth = bse_sound_font_repo_lock_fluid_synth (self->sfrepo);
  int sfont_id = fluid_synth_sfload (fluid_synth, bse_storage_blob_file_name (blob), 0);
  BseErrorType error;
  if (sfont_id != -1)
    {
      if (init_presets)
	{
	  fluid_sfont_t *fluid_sfont = fluid_synth_get_sfont_by_id (fluid_synth, sfont_id);
	  fluid_preset_t fluid_preset;

	  fluid_sfont->iteration_start (fluid_sfont);
	  while (fluid_sfont->iteration_next (fluid_sfont, &fluid_preset))
	    {
	      BseSoundFontPreset *sound_font_preset = g_object_new (BSE_TYPE_SOUND_FONT_PRESET,
								    "uname", fluid_preset.get_name (&fluid_preset),
								    NULL);
	      bse_container_add_item (BSE_CONTAINER (self), BSE_ITEM (sound_font_preset));
	      bse_sound_font_preset_init_preset (sound_font_preset, &fluid_preset);
	    }
	}
      self->sfont_id = sfont_id;
      self->blob = blob;
      error = BSE_ERROR_NONE;
    }
  else
    {
      bse_storage_blob_unref (blob);
      error = BSE_ERROR_WAVE_NOT_FOUND;
    }
  bse_sound_font_repo_unlock_fluid_synth (self->sfrepo);
  return error;
}

void
bse_sound_font_unload (BseSoundFont *sound_font)
{
  g_return_if_fail (sound_font->sfrepo != NULL);

  if (sound_font->sfont_id != -1)
    {
      fluid_synth_t *fluid_synth = bse_sound_font_repo_lock_fluid_synth (sound_font->sfrepo);
      fluid_synth_sfunload (fluid_synth, sound_font->sfont_id, 1 /* reset presets */);
      bse_sound_font_repo_unlock_fluid_synth (sound_font->sfrepo);
    }
  sound_font->sfont_id = -1;
}

BseErrorType
bse_sound_font_reload (BseSoundFont *sound_font)
{
  g_return_val_if_fail (sound_font->sfont_id == -1, BSE_ERROR_INTERNAL);

  return bse_sound_font_load_blob (sound_font, sound_font->blob, FALSE);
}

static void
bse_sound_font_store_private (BseObject  *object,
			      BseStorage *storage)
{
  BseSoundFont *sound_font = BSE_SOUND_FONT (object);
  /* chain parent class' handler */
  BSE_OBJECT_CLASS (parent_class)->store_private (object, storage);

  if (!BSE_STORAGE_SELF_CONTAINED (storage) && !bse_storage_blob_is_temp_file (sound_font->blob))
    {
      bse_storage_break (storage);
      bse_storage_printf (storage, "(load-sound-font \"%s\")", bse_storage_blob_file_name (sound_font->blob));
    }
  else
    {
      bse_storage_break (storage);
      bse_storage_printf (storage, "(load-sound-font ");
      bse_storage_put_blob (storage, sound_font->blob);
      bse_storage_printf (storage, ")");
    }
}

static SfiTokenType
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
      BseStorageBlob *blob;
      BseErrorType error;

      g_scanner_get_next_token (scanner); /* eat quark identifier */
      if (g_scanner_peek_next_token (scanner) == G_TOKEN_STRING)
	{
	  parse_or_return (scanner, G_TOKEN_STRING);
	  blob = bse_storage_blob_new_from_file (scanner->value.v_string, FALSE);
	}
      else
	{
          GTokenType token = bse_storage_parse_blob (storage, &blob);
	  if (token != G_TOKEN_NONE)
	    {
	      if (blob)
	        bse_storage_blob_unref (blob);
              return token;
	    }
	}
      if (g_scanner_peek_next_token (scanner) != ')')
	{
	  bse_storage_blob_unref (blob);
	  return ')';
	}
      parse_or_return (scanner, ')');
      error = bse_sound_font_load_blob (sound_font, blob, FALSE);
      if (error)
	bse_storage_warn (storage, "failed to load sound font \"%s\": %s",
				    bse_storage_blob_file_name (blob), bse_error_blurb (error));
      bse_storage_blob_unref (blob);
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
  BseSoundFont *sound_font = BSE_SOUND_FONT (container);

  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_SOUND_FONT_PRESET))
    sound_font->presets = g_list_append (sound_font->presets, item);
  else
    g_warning ("BseSoundFont: cannot hold non-sound-font-preset item type `%s'",
	       BSE_OBJECT_TYPE_NAME (item));

  /* chain parent class' add_item handler */
  BSE_CONTAINER_CLASS (parent_class)->add_item (container, item);
}

static void
bse_sound_font_forall_items (BseContainer      *container,
			     BseForallItemsFunc func,
			     gpointer           data)
{
  BseSoundFont *sound_font = BSE_SOUND_FONT (container);
  GList *list;

  list = sound_font->presets;
  while (list)
    {
      BseItem *item;

      item = list->data;
      list = list->next;
      if (!func (item, data))
	return;
    }
}

static void
bse_sound_font_remove_item (BseContainer *container,
			    BseItem      *item)
{
  BseSoundFont *sound_font = BSE_SOUND_FONT (container);

  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_SOUND_FONT_PRESET))
    sound_font->presets = g_list_remove (sound_font->presets, item);
  else
    g_warning ("BseSoundFontRepo: cannot hold non-sound-font-preset item type `%s'",
	       BSE_OBJECT_TYPE_NAME (item));

  /* chain parent class' remove_item handler */
  BSE_CONTAINER_CLASS (parent_class)->remove_item (container, item);
}

static void
bse_sound_font_release_children (BseContainer *container)
{
  BseSoundFont *self = BSE_SOUND_FONT (container);

  while (self->presets)
    bse_container_remove_item (container, self->presets->data);

  /* chain parent class' handler */
  BSE_CONTAINER_CLASS (parent_class)->release_children (container);
}


static void
bse_sound_font_class_init (BseSoundFontClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseContainerClass *container_class = BSE_CONTAINER_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

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
