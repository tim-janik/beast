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
#include	"bseeffectpatternjump.h"


#include        "bsecategories.h"


enum {
  PARAM_0,
  PARAM_PATTERN_ID
};

/* --- prototypes --- */
static void bse_effect_pattern_jump_class_init (BseEffectClass      *class);
static void bse_effect_pattern_jump_init       (BseEffectPatternJump *effect);
static void bse_effect_pattern_jump_set_property  (BseEffectPatternJump *effect,
						   guint                 param_id,
						   GValue               *value,
						   GParamSpec           *pspec,
						   const gchar          *trailer);
static void bse_effect_pattern_jump_get_property  (BseEffectPatternJump *effect,
						   guint                 param_id,
						   GValue               *value,
						   GParamSpec           *pspec,
						   const gchar          *trailer);


/* --- functions --- */
BSE_BUILTIN_TYPE (BseEffectPatternJump)
{
  static const GTypeInfo effect_info = {
    sizeof (BseEffectPatternJumpClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_effect_pattern_jump_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseEffectPatternJump),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_effect_pattern_jump_init,
  };
  GType effect_type;
  
  effect_type = bse_type_register_static (BSE_TYPE_EFFECT,
					  "BseEffectPatternJump",
					  "BSE Effect - jump to new pattern",
					  &effect_info);
  // bse_categories_register ("/Effect/Pattern Jump", effect_type);
  
  return effect_type;
}

static void
bse_effect_pattern_jump_class_init (BseEffectClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  
  gobject_class->set_property = bse_effect_pattern_jump_set_property;
  gobject_class->get_property = bse_effect_pattern_jump_get_property;
  
  bse_object_class_add_param (object_class, NULL,
			      PARAM_PATTERN_ID,
			      sfi_pspec_int ("pattern_id", "Pattern Id", NULL,
					     1, 1, BSE_MAX_SEQ_ID, 1,
					     SFI_PARAM_STANDARD));
}

static void
bse_effect_pattern_jump_init (BseEffectPatternJump *effect)
{
  effect->pattern_id = 1;
}

static void
bse_effect_pattern_jump_set_property (BseEffectPatternJump *effect,
				      guint                 param_id,
				      GValue               *value,
				      GParamSpec           *pspec,
				      const gchar          *trailer)
{
  switch (param_id)
    {
    case PARAM_PATTERN_ID:
      effect->pattern_id = sfi_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (effect, param_id, pspec);
      break;
    }
}

static void
bse_effect_pattern_jump_get_property (BseEffectPatternJump *effect,
				      guint                 param_id,
				      GValue               *value,
				      GParamSpec           *pspec,
				      const gchar          *trailer)
{
  switch (param_id)
    {
    case PARAM_PATTERN_ID:
      sfi_value_set_int (value, effect->pattern_id);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (effect, param_id, pspec);
      break;
    }
}
