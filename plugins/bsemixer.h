/* BseMixer - BSE Mixer
 * Copyright (C) 1999, 2000-2001 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 *
 * bsemixer.h: BSE Mixer - mix incoming signals
 */
#ifndef __BSE_MIXER_H__
#define __BSE_MIXER_H__

#include <bse/bseplugin.hh>
#include <bse/bsesource.hh>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_MIXER              (bse_mixer_get_type())
#define BSE_MIXER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_MIXER, BseMixer))
#define BSE_MIXER_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_MIXER, BseMixerClass))
#define BSE_IS_MIXER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_MIXER))
#define BSE_IS_MIXER_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_MIXER))
#define BSE_MIXER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_MIXER, BseMixerClass))

#define	BSE_MIXER_N_INPUTS	(4)


/* --- BseMixer source --- */
typedef struct _BseMixer      BseMixer;
typedef struct _BseMixerClass BseMixerClass;
struct _BseMixer
{
  BseSource       parent_object;

  gfloat	  master_volume_factor;
  gfloat          volume_factors[BSE_MIXER_N_INPUTS];
};
struct _BseMixerClass
{
  BseSourceClass parent_class;
};


/* --- channels --- */
enum
{
  BSE_MIXER_OCHANNEL_MONO
};





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_MIXER_H__ */
