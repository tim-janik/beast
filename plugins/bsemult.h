/* BseMult - BSE Mult
 * Copyright (C) 1999, 2000-2001 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * bsemult.h: BSE Multiplier - fold incoming signals
 */
#ifndef __BSE_MULT_H__
#define __BSE_MULT_H__

#define  BSE_PLUGIN_NAME  "BseMult"

#include <bse/bseplugin.h>
#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */





/* --- object type macros --- */
#define BSE_TYPE_MULT              (type_id_mult)
#define BSE_MULT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_MULT, BseMult))
#define BSE_MULT_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_MULT, BseMultClass))
#define BSE_IS_MULT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_MULT))
#define BSE_IS_MULT_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_MULT))
#define BSE_MULT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_MULT, BseMultClass))


/* --- BseMult source --- */
typedef struct _BseMult      BseMult;
typedef struct _BseMultClass BseMultClass;
struct _BseMult
{
  BseSource       parent_object;
};
struct _BseMultClass
{
  BseSourceClass parent_class;
};


/* --- channels --- */
enum
{
  BSE_MULT_ICHANNEL_MONO1,
  BSE_MULT_ICHANNEL_MONO2,
  BSE_MULT_ICHANNEL_MONO3,
  BSE_MULT_ICHANNEL_MONO4
};
enum
{
  BSE_MULT_OCHANNEL_MONO
};





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_MULT_H__ */
