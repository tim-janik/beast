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
#include	"bseeffect.h"


/* --- prototypes --- */
static void bse_effect_class_init (BseEffectClass *class);
static void bse_effect_init       (BseEffect      *effect);
static void bse_effect_destroy    (BseObject      *object);


/* --- variables --- */
static BseTypeClass *parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseEffect)
{
  static const BseTypeInfo effect_info = {
    sizeof (BseEffectClass),

    (BseBaseInitFunc) NULL,
    (BseBaseDestroyFunc) NULL,
    (BseClassInitFunc) bse_effect_class_init,
    (BseClassDestroyFunc) NULL,
    NULL /* class_data */,

    sizeof (BseEffect),
    0 /* n_preallocs */,
    (BseObjectInitFunc) bse_effect_init,
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

  parent_class = bse_type_class_peek (BSE_TYPE_OBJECT);

  object_class->destroy = bse_effect_destroy;

  class->effect_type = BSE_EFFECT_TYPE_NONE;
}

static void
bse_effect_init	(BseEffect *effect)
{
  /* effect->next = NULL; */
}

static void
bse_effect_destroy (BseObject *object)
{
  /*
    BseEffect *effect = BSE_EFFECT (object);

    if (effect->next)
    g_warning ("BseEffect was not properly unlinked before destruction (->next: %p)",
    effect->next);
  */

  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}
