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
#include	"bseeffectfinetune.h"


enum {
  PARAM_0,
  PARAM_FINE_TUNE
};

/* --- prototypes --- */
static void bse_effect_fine_tune_class_init (BseEffectClass      *class);
static void bse_effect_fine_tune_init       (BseEffectFineTune *effect);
static void bse_effect_fine_tune_set_param  (BseEffectFineTune *effect,
					     BseParam          *param,
					     guint              param_id);
static void bse_effect_fine_tune_get_param  (BseEffectFineTune *effect,
					     BseParam          *param,
					     guint              param_id);


/* --- functions --- */
BSE_BUILTIN_TYPE (BseEffectFineTune)
{
  static const GTypeInfo effect_info = {
    sizeof (BseEffectClass),
    
    (GBaseInitFunc) NULL,
    (GBaseDestroyFunc) NULL,
    (GClassInitFunc) bse_effect_fine_tune_class_init,
    (GClassDestroyFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseEffectFineTune),
    BSE_PREALLOC_N_EFFECTS /* n_preallocs */,
    (GInstanceInitFunc) bse_effect_fine_tune_init,
  };
  
  return bse_type_register_static (BSE_TYPE_EFFECT,
				   "BseEffectFineTune",
				   "BSE Effect - set fine tune",
				   &effect_info);
}

static void
bse_effect_fine_tune_class_init (BseEffectClass *class)
{
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  
  object_class->set_param = (BseObjectSetParamFunc) bse_effect_fine_tune_set_param;
  object_class->get_param = (BseObjectGetParamFunc) bse_effect_fine_tune_get_param;
  
  class->effect_type = BSE_EFFECT_TYPE_FINE_TUNE;
  
  bse_object_class_add_param (object_class, NULL,
			      PARAM_FINE_TUNE,
			      bse_param_spec_int ("fine_tune", "Fine tune", NULL,
						  BSE_MIN_FINE_TUNE, BSE_MAX_FINE_TUNE,
						  BSE_STP_FINE_TUNE,
						  BSE_DFL_INSTRUMENT_FINE_TUNE,
						  BSE_PARAM_DEFAULT |
						  BSE_PARAM_HINT_SCALE));
}

static void
bse_effect_fine_tune_init (BseEffectFineTune *effect)
{
  effect->fine_tune = 0;
}

static void
bse_effect_fine_tune_set_param (BseEffectFineTune *effect,
				BseParam          *param,
				guint              param_id)
{
  switch (param_id)
    {
    case PARAM_FINE_TUNE:
      effect->fine_tune = param->value.v_int;
      break;
    default:
      BSE_UNHANDLED_PARAM_ID (effect, param, param_id);
      break;
    }
}

static void
bse_effect_fine_tune_get_param (BseEffectFineTune *effect,
                                BseParam          *param,
				guint              param_id)
{
  switch (param_id)
    {
    case PARAM_FINE_TUNE:
      param->value.v_int = effect->fine_tune;
      break;
    default:
      BSE_UNHANDLED_PARAM_ID (effect, param, param_id);
      break;
    }
}
