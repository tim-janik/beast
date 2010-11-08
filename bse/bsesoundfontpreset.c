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
#include "bsesoundfontpreset.h"
#include "bsemain.h"
#include "bsestorage.h"
#include "bseprocedure.h"
#include "gsldatahandle.h"
#include "bseserver.h"
#include "bseloader.h"

#include <string.h>
#include <fluidsynth.h>

#define parse_or_return         bse_storage_scanner_parse_or_return

/* --- variables --- */
static GTypeClass *parent_class = NULL;
static GQuark      quark_program = 0;
static GQuark      quark_bank = 0;


/* --- functions --- */
static void
bse_sound_font_preset_init (BseSoundFontPreset *sound_font_preset)
{
  // init members here
}

void
bse_sound_font_preset_init_preset (BseSoundFontPreset *self,
                                   fluid_preset_t     *fluid_preset)
{
  self->bank = fluid_preset->get_banknum (fluid_preset);
  self->program = fluid_preset->get_num (fluid_preset);
}

static void
bse_sound_font_preset_store_private (BseObject  *object,
			             BseStorage *storage)
{
  BseSoundFontPreset *self = BSE_SOUND_FONT_PRESET (object);
  /* chain parent class' handler */
  BSE_OBJECT_CLASS (parent_class)->store_private (object, storage);

  bse_storage_break (storage);
  bse_storage_printf (storage, "(bank %d)", self->bank);
  bse_storage_break (storage);
  bse_storage_printf (storage, "(program %d)", self->program);
}

static SfiTokenType
bse_sound_font_preset_restore_private (BseObject  *object,
			               BseStorage *storage,
                                       GScanner   *scanner)
{
  BseSoundFontPreset *sound_font_preset = BSE_SOUND_FONT_PRESET (object);
  GTokenType expected_token;
  GQuark quark;

  /* chain parent class' handler */
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return BSE_OBJECT_CLASS (parent_class)->restore_private (object, storage, scanner);

  /* parse storage commands */
  quark = g_quark_try_string (scanner->next_value.v_identifier);
  if (quark == quark_program)
    {
      g_scanner_get_next_token (scanner); /* eat quark identifier */
      parse_or_return (scanner, G_TOKEN_INT);
      sound_font_preset->program = scanner->value.v_int;
      parse_or_return (scanner, ')');
      expected_token = G_TOKEN_NONE; /* got ')' */
    }
  else if (quark == quark_bank)
    {
      g_scanner_get_next_token (scanner); /* eat quark identifier */
      parse_or_return (scanner, G_TOKEN_INT);
      sound_font_preset->bank = scanner->value.v_int;
      parse_or_return (scanner, ')');
      expected_token = G_TOKEN_NONE; /* got ')' */
    }
  else /* chain parent class' handler */
    expected_token = BSE_OBJECT_CLASS (parent_class)->restore_private (object, storage, scanner);

  return expected_token;
}



static void
bse_sound_font_preset_set_property (GObject      *object,
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
bse_sound_font_preset_get_property (GObject    *object,
		                    guint       param_id,
		                    GValue     *value,
		                    GParamSpec *pspec)
{
  // BseSoundFontPreset *sound_font_preset = BSE_SOUND_FONT_PRESET (object);
  switch (param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
bse_sound_font_preset_dispose (GObject *object)
{
  // BseSoundFontPreset *sound_font_preset = BSE_SOUND_FONT_PRESET (object);
  // bse_sound_font_preset_clear (sound_font_preset);

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
bse_sound_font_preset_finalize (GObject *object)
{
  // BseSoundFontPreset *sound_font_preset = BSE_SOUND_FONT_PRESET (object);
  // bse_sound_font_preset_clear (sound_font_preset);

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bse_sound_font_preset_class_init (BseSoundFontPresetClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  gobject_class->set_property = bse_sound_font_preset_set_property;
  gobject_class->get_property = bse_sound_font_preset_get_property;
  gobject_class->dispose = bse_sound_font_preset_dispose;
  gobject_class->finalize = bse_sound_font_preset_finalize;

  object_class->store_private = bse_sound_font_preset_store_private;
  object_class->restore_private = bse_sound_font_preset_restore_private;

  quark_program = g_quark_from_static_string ("program");
  quark_bank = g_quark_from_static_string ("bank");
}

BSE_BUILTIN_TYPE (BseSoundFontPreset)
{
  static const GTypeInfo sound_font_preset_info = {
    sizeof (BseSoundFontPresetClass),

    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_sound_font_preset_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,

    sizeof (BseSoundFontPreset),
    0  /* n_preallocs */,
    (GInstanceInitFunc) bse_sound_font_preset_init,
  };

  return bse_type_register_static (BSE_TYPE_ITEM,
				   "BseSoundFontPreset",
				   "BSE sound_font_preset type",
                                   __FILE__, __LINE__,
                                   &sound_font_preset_info);
}
