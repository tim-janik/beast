/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1999, 2000-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __BSE_PCM_INPUT_H__
#define __BSE_PCM_INPUT_H__

#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_PCM_INPUT		(BSE_TYPE_ID (BsePcmInput))
#define BSE_PCM_INPUT(object)		(G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_PCM_INPUT, BsePcmInput))
#define BSE_PCM_INPUT_CLASS(class)	(G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_PCM_INPUT, BsePcmInputClass))
#define BSE_IS_PCM_INPUT(object)	(G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_PCM_INPUT))
#define BSE_IS_PCM_INPUT_CLASS(class)	(G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PCM_INPUT))
#define BSE_PCM_INPUT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_PCM_INPUT, BsePcmInputClass))


/* --- BsePcmInput source --- */
typedef struct _BsePcmInput      BsePcmInput;
typedef struct _BsePcmInputClass BsePcmInputClass;
struct _BsePcmInput
{
  BseSource       parent_object;

  gfloat	  volume_factor;

  /* PREPARED */
  GslModule	 *uplink;
};
struct _BsePcmInputClass
{
  BseSourceClass     parent_class;
};


/* --- channels --- */
enum
{
  BSE_PCM_INPUT_OCHANNEL_LEFT,
  BSE_PCM_INPUT_OCHANNEL_RIGHT,
  BSE_PCM_INPUT_N_OCHANNELS
};





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_PCM_INPUT_H__ */
