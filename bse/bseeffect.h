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
 * bseeffect.h: base type for BSE effects.
 */
#ifndef __BSE_EFFECT_H__
#define __BSE_EFFECT_H__

#include	<bse/bseobject.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- BseEffect type macros --- */
#define BSE_TYPE_EFFECT		     (BSE_TYPE_ID (BseEffect))
#define BSE_EFFECT(object)	     (BSE_CHECK_STRUCT_CAST ((object), BSE_TYPE_EFFECT, BseEffect))
#define BSE_EFFECT_CLASS(class)	     (BSE_CHECK_CLASS_CAST ((class), BSE_TYPE_EFFECT, BseEffectClass))
#define BSE_IS_EFFECT(object)	     (BSE_CHECK_STRUCT_TYPE ((object), BSE_TYPE_EFFECT))
#define BSE_IS_EFFECT_CLASS(class)   (BSE_CHECK_CLASS_TYPE ((class), BSE_TYPE_EFFECT))
#define BSE_EFFECT_GET_CLASS(object) ((BseEffectClass*) (((BseObject*) (object))->bse_struct.bse_class))


/* BSE effects are fairly lightweight (and somewhat uncommon) objects.
 * they are managed by BsePatterns and in principle serve as simple
 * operators and id containers only. the reason to implement them as
 * real objects, rather than plain auxillary structures is to provide
 * the convenient BseObject facilities, i.e. a generalized parameter
 * interface and an automated store/restore mechanism for these
 * structures as well.
 */

/* --- BSE Effect types --- */
typedef enum
{
  BSE_EFFECT_TYPE_NONE,

  /* sequencer effects */
  BSE_EFFECT_TYPE_PATTERN_BREAK,
  BSE_EFFECT_TYPE_PATTERN_JUMP,

  /* voice effects */
  BSE_EFFECT_TYPE_NOTE_VOLUME,
  BSE_EFFECT_TYPE_VOLUME_DELTA,
  BSE_EFFECT_TYPE_FINE_TUNE,

  /* unimplemented */
  BSE_EFFECT_TYPE_APPREGIO,
  BSE_EFFECT_TYPE_VIBRATO,
  BSE_EFFECT_TYPE_TREMOLO,
  BSE_EFFECT_TYPE_SLIDE,
  BSE_EFFECT_TYPE_PITCH_BEND,
  BSE_EFFECT_TYPE_PORTAMENTO,
  BSE_EFFECT_TYPE_PORTAMENTO_UP,
  BSE_EFFECT_TYPE_PORTAMENTO_DOWN,
  BSE_EFFECT_TYPE_LAST                       /* <skip> */
} BseEffectType;


/* --- BseEffect --- */
struct _BseEffect
{
  BseObject  parent_object;

  /* private, for BsePattern use */
  /* BseEffect *next; */
};
struct _BseEffectClass
{
  BseObjectClass parent_class;

  /* though effects can be uniquely identified through their type
   * ids, we use this enum to be able to construct fast switch
   * statements to special case the various effect types.
   */

  BseEffectType effect_type;
};



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_EFFECT_H__ */
