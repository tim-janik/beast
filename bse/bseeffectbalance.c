/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999, 2000 Olaf Hoehmann and Tim Janik
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
#include        "bseeffectbalance.h"


#include        "bsecategories.h"


enum {
  PARAM_0,
  PARAM_BALANCE
};

/* --- prototypes --- */
static void bse_effect_balance_class_init   (BseEffectClass   *class);
static void bse_effect_balance_init         (BseEffectBalance *effect);
static void bse_effect_balance_set_property    (BseEffectBalance *effect,
					     guint             param_id,
					     GValue           *value,
					     GParamSpec       *pspec,
					     const gchar      *trailer);
static void bse_effect_balance_get_property    (BseEffectBalance *effect,
					     guint             param_id,
					     GValue           *value,
					     GParamSpec       *pspec,
					     const gchar      *trailer);
static void bse_effect_balance_setup_voice  (BseEffect        *effect,
					     BseVoice         *voice);


/* --- functions --- */
BSE_BUILTIN_TYPE (BseEffectBalance)
{
  static const GTypeInfo effect_info = {
    sizeof (BseEffectBalanceClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_effect_balance_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseEffectBalance),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_effect_balance_init,
  };
  GType effect_type;
  
  effect_type = bse_type_register_static (BSE_TYPE_EFFECT,
                                          "BseEffectBalance",
                                          "BSE Effect - set new balance",
                                          &effect_info);
  bse_categories_register ("/Effect/Balance", effect_type);
  
  return effect_type;
}

static void
bse_effect_balance_class_init (BseEffectClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  
  gobject_class->set_property = (GObjectSetPropertyFunc) bse_effect_balance_set_property;
  gobject_class->get_property = (GObjectGetPropertyFunc) bse_effect_balance_get_property;
  
  class->setup_voice = bse_effect_balance_setup_voice;
  
  bse_object_class_add_param (object_class, NULL,
			      PARAM_BALANCE,
			      bse_param_spec_int ("balance", "Balance", NULL,
						BSE_MIN_BALANCE_f, BSE_MAX_BALANCE_f,
						0, BSE_STP_BALANCE_f,
						BSE_PARAM_DEFAULT |
						BSE_PARAM_HINT_SCALE));
}

static void
bse_effect_balance_init (BseEffectBalance *effect)
{
  effect->balance = 0;
}

static void
bse_effect_balance_set_property (BseEffectBalance *effect,
			      guint             param_id,
			      GValue           *value,
			      GParamSpec       *pspec,
			      const gchar      *trailer)
{
  switch (param_id)
    {
    case PARAM_BALANCE:
      effect->balance = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (effect, param_id, pspec);
      break;
    }
}

static void
bse_effect_balance_get_property (BseEffectBalance *effect,
			      guint             param_id,
			      GValue           *value,
			      GParamSpec       *pspec,
			      const gchar      *trailer)
{
  switch (param_id)
    {
    case PARAM_BALANCE:
      g_value_set_int (value, effect->balance);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (effect, param_id, pspec);
      break;
    }
}

static void
bse_effect_balance_setup_voice (BseEffect *effect,
				BseVoice  *voice)
{
  BseEffectBalance *balance_effect = BSE_EFFECT_BALANCE (effect);

  // _bse_voice_set_balance (voice, balance_effect->balance);
}
