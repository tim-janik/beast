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
 *
 * bseeffectpatternjump.h: BSE Effect - jump to new pattern
 */
#ifndef __BSE_EFFECT_PATTERN_JUMP_H__
#define __BSE_EFFECT_PATTERN_JUMP_H__

#include	<bse/bseeffect.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* BSE effects are fairly lightweight (and somewhat uncommon) objects.
 * look at bseeffect.h for the actuall class implementation, common
 * other object stuff and the BseEffectType enum which lists this effect
 * type amongst others.
 */

/* --- BseEffectPatternJump type macros --- */
#define BSE_TYPE_EFFECT_PATTERN_JUMP    (BSE_TYPE_ID (BseEffectPatternJump))
#define BSE_EFFECT_PATTERN_JUMP(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BSE_TYPE_EFFECT_PATTERN_JUMP, BseEffectPatternJump))
#define BSE_IS_EFFECT_PATTERN_JUMP(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_EFFECT_PATTERN_JUMP))


/* --- BseEffectPatternJump --- */
typedef struct _BseEffectPatternJump BseEffectPatternJump;
struct _BseEffectPatternJump
{
  BseEffect parent_object;

  guint pattern_id;
};


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_EFFECT_PATTERN_JUMP_H__ */
