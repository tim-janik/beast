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
#include	"bseeffect.h"



/* --- prototypes --- */
static void bse_effect_class_init 	(BseEffectClass *class);
static void bse_effect_init		(BseEffect      *effect);
static void bse_effect_destroy		(BseObject      *object);


/* --- variables --- */
static GTypeClass *parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseEffect)
{
  static const GTypeInfo effect_info = {
    sizeof (BseEffectClass),

    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_effect_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,

    sizeof (BseEffect),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_effect_init,
  };

  return bse_type_register_static (BSE_TYPE_OBJECT,
				   "BseEffect",
				   "Base type for all BSE Effects",
				   &effect_info);
}

static void
bse_effect_class_init (BseEffectClass *class)
{
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  object_class->destroy = bse_effect_destroy;

  class->jump_sequencer = NULL;
  class->setup_voice = NULL;
}

static void
bse_effect_init	(BseEffect *effect)
{
}

static void
bse_effect_destroy (BseObject *object)
{

  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

void
bse_effect_jump_sequencer (BseEffect *effect,
			   guint     *current_pattern,
			   guint     *current_row)
{
  BseEffectClass *class;

  g_return_if_fail (BSE_IS_EFFECT (effect));
  g_return_if_fail (current_pattern != NULL);
  g_return_if_fail (current_row != NULL);

  class = BSE_EFFECT_GET_CLASS (effect);
  if (class->jump_sequencer)
    class->jump_sequencer (effect, current_pattern, current_row);
}

void
bse_effect_setup_voice (BseEffect *effect,
			BseVoice  *voice)
{
  BseEffectClass *class;

  g_return_if_fail (BSE_IS_EFFECT (effect));
  g_return_if_fail (voice != NULL);

  class = BSE_EFFECT_GET_CLASS (effect);
  if (class->setup_voice)
    class->setup_voice (effect, voice);
}
