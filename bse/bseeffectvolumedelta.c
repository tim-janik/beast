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
#include	"bseeffectvolumedelta.h"


enum {
  PARAM_0,
  PARAM_DELTA_PERC
};

/* --- prototypes --- */
static void bse_effect_volume_delta_class_init (BseEffectClass      *class);
static void bse_effect_volume_delta_init       (BseEffectVolumeDelta *effect);
static void bse_effect_volume_delta_set_param  (BseEffectVolumeDelta *effect,
						BseParam             *param);
static void bse_effect_volume_delta_get_param  (BseEffectVolumeDelta *effect,
						BseParam             *param);
     

/* --- functions --- */
BSE_BUILTIN_TYPE (BseEffectVolumeDelta)
{
  static const BseTypeInfo effect_info = {
    sizeof (BseEffectClass),

    (BseBaseInitFunc) NULL,
    (BseBaseDestroyFunc) NULL,
    (BseClassInitFunc) bse_effect_volume_delta_class_init,
    (BseClassDestroyFunc) NULL,
    NULL /* class_data */,

    sizeof (BseEffectVolumeDelta),
    8 /* n_preallocs */,
    (BseObjectInitFunc) bse_effect_volume_delta_init,
  };

  return bse_type_register_static (BSE_TYPE_EFFECT,
				   "BseEffectVolumeDelta",
				   "BSE Effect - modify volume by delta",
				   &effect_info);
}

static void
bse_effect_volume_delta_class_init (BseEffectClass *class)
{
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);

  object_class->set_param = (BseObjectSetParamFunc) bse_effect_volume_delta_set_param;
  object_class->get_param = (BseObjectGetParamFunc) bse_effect_volume_delta_get_param;

  class->effect_type = BSE_EFFECT_TYPE_VOLUME_DELTA;

  bse_object_class_add_param (object_class, NULL,
			      PARAM_DELTA_PERC,
			      bse_param_spec_uint ("delta_perc", "Delta [%]", NULL,
						   bse_dB_to_factor (BSE_MAX_VOLUME_dB) * (-100),
						   bse_dB_to_factor (BSE_MAX_VOLUME_dB) * 100,
						   1,
						   0,
						   BSE_PARAM_DEFAULT |
						   BSE_PARAM_HINT_DIAL));
}

static void
bse_effect_volume_delta_init (BseEffectVolumeDelta *effect)
{
  effect->volume_delta = 0;
}

static void
bse_effect_volume_delta_set_param (BseEffectVolumeDelta *effect,
				   BseParam             *param)
{
  switch (param->pspec->any.param_id)
    {
    case PARAM_DELTA_PERC:
      effect->volume_delta = ((gfloat) param->value.v_uint) / 100;
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
bse_effect_volume_delta_get_param (BseEffectVolumeDelta *effect,
				   BseParam             *param)
{
  switch (param->pspec->any.param_id)
    {
    case PARAM_DELTA_PERC:
      param->value.v_uint = effect->volume_delta * ((gfloat) 100) + 0.5;
      break;
    default:
      g_warning ("%s: invalid attempt to get parameter \"%s\" of type `%s'",
		 BSE_OBJECT_TYPE_NAME (effect),
		 param->pspec->any.name,
		 bse_type_name (param->pspec->type));
      break;
    }
}
