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
#include	"bseeffectpatternjump.h"


enum {
  PARAM_0,
  PARAM_PATTERN_ID
};

/* --- prototypes --- */
static void bse_effect_pattern_jump_class_init (BseEffectClass      *class);
static void bse_effect_pattern_jump_init       (BseEffectPatternJump *effect);
static void bse_effect_pattern_jump_set_param  (BseEffectPatternJump *effect,
						BseParam             *param,
						guint                 param_id);
static void bse_effect_pattern_jump_get_param  (BseEffectPatternJump *effect,
						BseParam             *param,
						guint                 param_id);


/* --- functions --- */
BSE_BUILTIN_TYPE (BseEffectPatternJump)
{
  static const GTypeInfo effect_info = {
    sizeof (BseEffectClass),

    (GBaseInitFunc) NULL,
    (GBaseDestroyFunc) NULL,
    (GClassInitFunc) bse_effect_pattern_jump_class_init,
    (GClassDestroyFunc) NULL,
    NULL /* class_data */,

    sizeof (BseEffectPatternJump),
    BSE_PREALLOC_N_EFFECTS /* n_preallocs */,
    (GInstanceInitFunc) bse_effect_pattern_jump_init,
  };

  return bse_type_register_static (BSE_TYPE_EFFECT,
				   "BseEffectPatternJump",
				   "BSE Effect - jump to new pattern",
				   &effect_info);
}

static void
bse_effect_pattern_jump_class_init (BseEffectClass *class)
{
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);

  object_class->set_param = (BseObjectSetParamFunc) bse_effect_pattern_jump_set_param;
  object_class->get_param = (BseObjectGetParamFunc) bse_effect_pattern_jump_get_param;

  class->effect_type = BSE_EFFECT_TYPE_PATTERN_JUMP;

  bse_object_class_add_param (object_class, NULL,
			      PARAM_PATTERN_ID,
			      bse_param_spec_uint ("pattern_id", "Pattern Id", NULL,
						   1, BSE_MAX_SEQ_ID,
						   1,
						   1,
						   BSE_PARAM_DEFAULT));
}

static void
bse_effect_pattern_jump_init (BseEffectPatternJump *effect)
{
  effect->pattern_id = 1;
}

static void
bse_effect_pattern_jump_set_param (BseEffectPatternJump *effect,
				   BseParam             *param,
				   guint                 param_id)
{
  switch (param_id)
    {
    case PARAM_PATTERN_ID:
      effect->pattern_id = param->value.v_uint;
      break;
    default:
      BSE_UNHANDLED_PARAM_ID (effect, param, param_id);
      break;
    }
}

static void
bse_effect_pattern_jump_get_param (BseEffectPatternJump *effect,
                                   BseParam             *param,
				   guint                 param_id)
{
  switch (param_id)
    {
    case PARAM_PATTERN_ID:
      param->value.v_uint = effect->pattern_id;
      break;
    default:
      BSE_UNHANDLED_PARAM_ID (effect, param, param_id);
      break;
    }
}
