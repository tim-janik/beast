/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include        "bseeffectnotevolume.h"


#include        "bsecategories.h"


enum {
  PARAM_0,
  PARAM_VOLUME_PERC,
  PARAM_VOLUME_dB
};

/* --- prototypes --- */
static void bse_effect_note_volume_class_init   (BseEffectClass      *class);
static void bse_effect_note_volume_init         (BseEffectNoteVolume *effect);
static void bse_effect_note_volume_set_property    (BseEffectNoteVolume *effect,
						 guint                param_id,
						 GValue              *value,
						 GParamSpec          *pspec,
						 const gchar         *trailer);
static void bse_effect_note_volume_get_property    (BseEffectNoteVolume *effect,
						 guint                param_id,
						 GValue              *value,
						 GParamSpec          *pspec,
						 const gchar         *trailer);
static void bse_effect_note_volume_setup_voice  (BseEffect           *effect,
                                                 BseVoice            *voice);


/* --- functions --- */
BSE_BUILTIN_TYPE (BseEffectNoteVolume)
{
  static const GTypeInfo effect_info = {
    sizeof (BseEffectNoteVolumeClass),

    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_effect_note_volume_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,

    sizeof (BseEffectNoteVolume),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_effect_note_volume_init,
  };
  GType effect_type;
  
  effect_type = bse_type_register_static (BSE_TYPE_EFFECT,
                                          "BseEffectNoteVolume",
                                          "BSE Effect - set new volume",
                                          &effect_info);
  bse_categories_register ("/Effect/Note Volume", effect_type);

  return effect_type;
}

static void
bse_effect_note_volume_class_init (BseEffectClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);

  gobject_class->set_property = (GObjectSetPropertyFunc) bse_effect_note_volume_set_property;
  gobject_class->get_property = (GObjectGetPropertyFunc) bse_effect_note_volume_get_property;

  class->setup_voice = bse_effect_note_volume_setup_voice;

  bse_object_class_add_param (object_class, NULL,
                              PARAM_VOLUME_PERC,
                              bse_param_spec_uint ("volume_perc", "Volume [%]", NULL,
                                                 0, bse_dB_to_factor (BSE_MAX_VOLUME_dB) * 100,
                                                 bse_dB_to_factor (0) * 100,
                                                 1,
                                                 BSE_PARAM_GUI |
                                                 BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, NULL,
                              PARAM_VOLUME_dB,
                              bse_param_spec_float ("volume_dB", "Volume [dB]", NULL,
                                                  BSE_MIN_VOLUME_dB, BSE_MAX_VOLUME_dB,
                                                  0,
                                                  BSE_STP_VOLUME_dB,
                                                  BSE_PARAM_GUI |
                                                  BSE_PARAM_HINT_DIAL));
}

static void
bse_effect_note_volume_init (BseEffectNoteVolume *effect)
{
  effect->volume_factor = 1;
}

static void
bse_effect_note_volume_set_property (BseEffectNoteVolume *effect,
                                  guint                param_id,
                                  GValue              *value,
                                  GParamSpec          *pspec,
                                  const gchar         *trailer)
{
  switch (param_id)
    {
    case PARAM_VOLUME_PERC:
      effect->volume_factor = g_value_get_uint (value) / 100.0;
      bse_object_param_changed (BSE_OBJECT (effect), "volume_dB");
      break;
    case PARAM_VOLUME_dB:
      effect->volume_factor = bse_dB_to_factor (g_value_get_float (value));
      bse_object_param_changed (BSE_OBJECT (effect), "volume_perc");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (effect, param_id, pspec);
      break;
    }
}

static void
bse_effect_note_volume_get_property (BseEffectNoteVolume *effect,
                                  guint                param_id,
                                  GValue              *value,
                                  GParamSpec          *pspec,
                                  const gchar         *trailer)
{
  switch (param_id)
    {
    case PARAM_VOLUME_PERC:
      g_value_set_uint (value, effect->volume_factor * 100.0 + 0.5);
      break;
    case PARAM_VOLUME_dB:
      g_value_set_float (value, bse_dB_from_factor (effect->volume_factor, BSE_MIN_VOLUME_dB));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (effect, param_id, pspec);
      break;
    }
}

static void
bse_effect_note_volume_setup_voice (BseEffect *effect,
				    BseVoice  *voice)
{
  BseEffectNoteVolume *nv_effect = BSE_EFFECT_NOTE_VOLUME (effect);

  // _bse_voice_set_volume (voice, nv_effect->volume_factor);
}
