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
 * bseeffect.h: base type for BSE effects.
 */
#ifndef __BSE_EFFECT_H__
#define __BSE_EFFECT_H__

#include	<bse/bseobject.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- BseEffect type macros --- */
#define BSE_TYPE_IS_EFFECT(type)     (g_type_is_a ((type), BSE_TYPE_EFFECT))
#define BSE_TYPE_EFFECT		     (BSE_TYPE_ID (BseEffect))
#define BSE_EFFECT(object)	     (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_EFFECT, BseEffect))
#define BSE_EFFECT_CLASS(class)	     (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_EFFECT, BseEffectClass))
#define BSE_IS_EFFECT(object)	     (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_EFFECT))
#define BSE_IS_EFFECT_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_EFFECT))
#define BSE_EFFECT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_EFFECT, BseEffectClass))


/* BSE effects are fairly lightweight objects.
 * they are managed by BsePatterns and in principle serve as simple
 * operators and id containers only. the reason to implement them as
 * real objects, rather than plain auxillary structures is to provide
 * the convenient BseObject facilities, i.e. a generalized parameter
 * interface and an automated store/restore mechanism for these
 * structures as well.
 */


/* --- BseEffect --- */
struct _BseEffect
{
  BseObject  parent_object;
};
struct _BseEffectClass
{
  BseObjectClass parent_class;

  void		(*jump_sequencer)	(BseEffect	*effect,
					 guint		*current_pattern,
					 guint		*current_row);
  void		(*setup_voice)		(BseEffect	*effect,
					 BseVoice	*voice);
};


/* --- prototypes --- */
void		bse_effect_jump_sequencer	(BseEffect	*effect,
						 guint		*current_pattern,
						 guint		*current_row);
void		bse_effect_setup_voice		(BseEffect	*effect,
						 BseVoice	*voice);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_EFFECT_H__ */
