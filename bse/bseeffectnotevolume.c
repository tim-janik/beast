/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
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
#include	"bseeffectnotevolume.h"


enum {
  PARAM_0,
  PARAM_VOLUME_PERC,
  PARAM_VOLUME_dB
};

/* --- prototypes --- */
static void bse_effect_note_volume_class_init (BseEffectClass      *class);
static void bse_effect_note_volume_init       (BseEffectNoteVolume *effect);
static void bse_effect_note_volume_set_param  (BseEffectNoteVolume *effect,
					       BseParam            *param);
static void bse_effect_note_volume_get_param  (BseEffectNoteVolume *effect,
					       BseParam            *param);
     

/* --- functions --- */
BSE_BUILTIN_TYPE (BseEffectNoteVolume)
{
  static const BseTypeInfo effect_info = {
    sizeof (BseEffectClass),

    (BseBaseInitFunc) NULL,
    (BseBaseDestroyFunc) NULL,
    (BseClassInitFunc) bse_effect_note_volume_class_init,
    (BseClassDestroyFunc) NULL,
    NULL /* class_data */,

    sizeof (BseEffectNoteVolume),
    8 /* n_preallocs */,
    (BseObjectInitFunc) bse_effect_note_volume_init,
  };

  return bse_type_register_static (BSE_TYPE_EFFECT,
				   "BseEffectNoteVolume",
				   "BSE Effect - set new volume",
				   &effect_info);
}

static void
bse_effect_note_volume_class_init (BseEffectClass *class)
{
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);

  object_class->set_param = (BseObjectSetParamFunc) bse_effect_note_volume_set_param;
  object_class->get_param = (BseObjectGetParamFunc) bse_effect_note_volume_get_param;

  class->effect_type = BSE_EFFECT_TYPE_NOTE_VOLUME;

  bse_object_class_add_param (object_class, NULL,
			      PARAM_VOLUME_PERC,
			      bse_param_spec_uint ("volume_perc", "Volume [%]", NULL,
						   0, bse_dB_to_factor (BSE_MAX_VOLUME_dB) * 100,
						   1,
						   bse_dB_to_factor (0) * 100,
						   BSE_PARAM_GUI |
						   BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, NULL,
			      PARAM_VOLUME_dB,
			      bse_param_spec_float ("volume_dB", "Volume [dB]", NULL,
						    BSE_MIN_VOLUME_dB, BSE_MAX_VOLUME_dB,
						    BSE_STP_VOLUME_dB,
						    0,
						    BSE_PARAM_GUI |
						    BSE_PARAM_HINT_DIAL));
}

static void
bse_effect_note_volume_init (BseEffectNoteVolume *effect)
{
  effect->volume_factor = 1;
}

static void
bse_effect_note_volume_set_param (BseEffectNoteVolume *effect,
				  BseParam            *param)
{
  switch (param->pspec->any.param_id)
    {
    case PARAM_VOLUME_PERC:
      effect->volume_factor = ((gfloat) param->value.v_uint) / 100;
      bse_object_param_changed (BSE_OBJECT (effect), "volume_dB");
      break;
    case PARAM_VOLUME_dB:
      effect->volume_factor = bse_dB_to_factor (param->value.v_float);
      bse_object_param_changed (BSE_OBJECT (effect), "volume_perc");
      break;
    default:
      g_warning ("%s: invalid attempt to set parameter \"%s\" of type `%s'",
		 BSE_OBJECT_TYPE_NAME (effect),
		 param->pspec->any.name,
		 bse_type_name (param->pspec->type));
      break;
    }
}

static void
bse_effect_note_volume_get_param (BseEffectNoteVolume *effect,
				  BseParam            *param)
{
  switch (param->pspec->any.param_id)
    {
    case PARAM_VOLUME_dB:
      param->value.v_float = bse_dB_from_factor (effect->volume_factor, BSE_MIN_VOLUME_dB);
      break;
    case PARAM_VOLUME_PERC:
      param->value.v_uint = effect->volume_factor * ((gfloat) 100) + 0.5;
      break;
    default:
      g_warning ("%s: invalid attempt to get parameter \"%s\" of type `%s'",
		 BSE_OBJECT_TYPE_NAME (effect),
		 param->pspec->any.name,
		 bse_type_name (param->pspec->type));
      break;
    }
}
