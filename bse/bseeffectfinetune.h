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
 *
 * bseeffectfinetune.h: BSE Effect - set fine tune
 */
#ifndef __BSE_EFFECT_FINE_TUNE_H__
#define __BSE_EFFECT_FINE_TUNE_H__

#include	<bse/bseeffect.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* BSE effects are fairly lightweight (and somewhat uncommon) objects.
 * look at bseeffect.h for the actuall class implementation, common
 * other object stuff and the BseEffectType enum which lists this effect
 * type amongst others.
 */

/* --- BseEffectFineTune type macros --- */
#define BSE_TYPE_EFFECT_FINE_TUNE    (BSE_TYPE_ID (BseEffectFineTune))
#define BSE_EFFECT_FINE_TUNE(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BSE_TYPE_EFFECT_FINE_TUNE, BseEffectFineTune))
#define BSE_IS_EFFECT_FINE_TUNE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_EFFECT_FINE_TUNE))


/* --- BseEffectFineTune --- */
typedef struct _BseEffectFineTune BseEffectFineTune;
typedef struct _BseEffectClass    BseEffectFineTuneClass;
struct _BseEffectFineTune
{
  BseEffect parent_object;

  gint fine_tune;
};


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_EFFECT_FINE_TUNE_H__ */
