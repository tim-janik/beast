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
#include	"bseeffectpatternbreak.h"


/* --- prototypes --- */
static void bse_effect_pattern_break_class_init (BseEffectClass      *class);
static void bse_effect_pattern_break_init       (BseEffectPatternBreak *effect);


/* --- functions --- */
BSE_BUILTIN_TYPE (BseEffectPatternBreak)
{
  static const GTypeInfo effect_info = {
    sizeof (BseEffectClass),

    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_effect_pattern_break_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,

    sizeof (BseEffectPatternBreak),
    BSE_PREALLOC_N_EFFECTS /* n_preallocs */,
    (GInstanceInitFunc) bse_effect_pattern_break_init,
  };

  return bse_type_register_static (BSE_TYPE_EFFECT,
				   "BseEffectPatternBreak",
				   "BSE Effect - break and advance to new pattern",
				   &effect_info);
}

static void
bse_effect_pattern_break_class_init (BseEffectClass *class)
{
  /* BseObjectClass *object_class = BSE_OBJECT_CLASS (class); */

  class->effect_type = BSE_EFFECT_TYPE_PATTERN_BREAK;
}

static void
bse_effect_pattern_break_init (BseEffectPatternBreak *effect)
{
}
