/* BseNoise - BSE Noise generator
 * Copyright (C) 1999,2000-2001 Tim Janik
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
 * bsenoise.h: BSE Noise generator
 */
#ifndef __BSE_NOISE_H__
#define __BSE_NOISE_H__

#define  BSE_PLUGIN_NAME  "BseNoise"

#include <bse/bseplugin.h>
#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */





/* --- object type macros --- */
#define BSE_TYPE_NOISE              (type_id_noise)
#define BSE_NOISE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_NOISE, BseNoise))
#define BSE_NOISE_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_NOISE, BseNoiseClass))
#define BSE_IS_NOISE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_NOISE))
#define BSE_IS_NOISE_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_NOISE))
#define BSE_NOISE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_NOISE, BseNoiseClass))


/* --- BseNoise source --- */
typedef struct _BseNoise      BseNoise;
typedef struct _BseNoiseClass BseNoiseClass;
struct _BseNoise
{
  BseSource       parent_object;

  BseSampleValue *static_noise;
};
struct _BseNoiseClass
{
  BseSourceClass parent_class;
};


/* --- channels --- */
enum
{
  BSE_NOISE_OCHANNEL_NOISE
};





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_NOISE_H__ */
