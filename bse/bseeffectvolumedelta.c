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


#include	"bsecategories.h"


enum {
  PARAM_0,
  PARAM_DELTA_PERC
};

/* --- prototypes --- */
static void bse_effect_volume_delta_class_init (BseEffectClass      *class);
static void bse_effect_volume_delta_init       (BseEffectVolumeDelta *effect);
static void bse_effect_volume_delta_set_property  (BseEffectVolumeDelta *effect,
						guint                 param_id,
						GValue               *value,
						GParamSpec           *pspec,
						const gchar          *trailer);
static void bse_effect_volume_delta_get_property  (BseEffectVolumeDelta *effect,
						guint                 param_id,
						GValue               *value,
						GParamSpec           *pspec,
						const gchar          *trailer);
     

/* --- functions --- */
BSE_BUILTIN_TYPE (BseEffectVolumeDelta)
{
  static const GTypeInfo effect_info = {
    sizeof (BseEffectVolumeDeltaClass),

    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_effect_volume_delta_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,

    sizeof (BseEffectVolumeDelta),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_effect_volume_delta_init,
  };
  GType effect_type;

  effect_type = bse_type_register_static (BSE_TYPE_EFFECT,
					  "BseEffectVolumeDelta",
					  "BSE Effect - modify volume by delta",
					  &effect_info);
  // bse_categories_register ("/Effect/Volume Delta", effect_type);
  
  return effect_type;
}

static void
bse_effect_volume_delta_class_init (BseEffectClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);

  gobject_class->set_property = (GObjectSetPropertyFunc) bse_effect_volume_delta_set_property;
  gobject_class->get_property = (GObjectGetPropertyFunc) bse_effect_volume_delta_get_property;

  bse_object_class_add_param (object_class, NULL,
			      PARAM_DELTA_PERC,
			      bse_param_spec_int ("delta_perc", "Delta [%]", NULL,
						bse_dB_to_factor (BSE_MAX_VOLUME_dB) * -100,
						bse_dB_to_factor (BSE_MAX_VOLUME_dB) * 100,
						0, 1,
						BSE_PARAM_DEFAULT |
						BSE_PARAM_HINT_DIAL));
}

static void
bse_effect_volume_delta_init (BseEffectVolumeDelta *effect)
{
  effect->volume_delta = 0;
}

static void
bse_effect_volume_delta_set_property (BseEffectVolumeDelta *effect,
				   guint                 param_id,
				   GValue               *value,
				   GParamSpec           *pspec,
				   const gchar          *trailer)
{
  switch (param_id)
    {
    case PARAM_DELTA_PERC:
      effect->volume_delta = g_value_get_int (value) / 100.0;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (effect, param_id, pspec);
      break;
    }
}

static void
bse_effect_volume_delta_get_property (BseEffectVolumeDelta *effect,
				   guint                 param_id,
				   GValue               *value,
				   GParamSpec           *pspec,
				   const gchar          *trailer)
{
  switch (param_id)
    {
    case PARAM_DELTA_PERC:
      g_value_set_int (value, effect->volume_delta * 100.0 + 0.5);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (effect, param_id, pspec);
      break;
    }
}
