/* BseAmplifier - BSE Amplifier
 * Copyright (C) 2002 Tim Janik
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
#ifndef __BSE_AMPLIFIER_H__
#define __BSE_AMPLIFIER_H__

#include <bse/bseplugin.h>
#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_AMPLIFIER              (BSE_TYPE_ID (BseAmplifier))
#define BSE_AMPLIFIER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_AMPLIFIER, BseAmplifier))
#define BSE_AMPLIFIER_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_AMPLIFIER, BseAmplifierClass))
#define BSE_IS_AMPLIFIER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_AMPLIFIER))
#define BSE_IS_AMPLIFIER_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_AMPLIFIER))
#define BSE_AMPLIFIER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_AMPLIFIER, BseAmplifierClass))


/* --- BseAmplifier source --- */
typedef struct _BseAmplifier      BseAmplifier;
typedef struct _BseAmplifierClass BseAmplifierClass;
typedef struct
{
  gfloat ctrl_balance;
  gfloat ctrl_strength;
  gfloat audio_balance;
  gfloat audio_gain;
  gfloat master_gain;
  guint  ctrl_mul : 1;
  guint	 exp_ctrl : 1;
} BseAmplifierConfig;
struct _BseAmplifier
{
  BseSource       parent_object;

  BseAmplifierConfig config;
};
struct _BseAmplifierClass
{
  BseSourceClass parent_class;
};


/* --- channels --- */
enum
{
  BSE_AMPLIFIER_ICHANNEL_AUDIO1,
  BSE_AMPLIFIER_ICHANNEL_AUDIO2,
  BSE_AMPLIFIER_ICHANNEL_CONTROL1,
  BSE_AMPLIFIER_ICHANNEL_CONTROL2,
  BSE_AMPLIFIER_N_ICHANNELS
};
enum
{
  BSE_AMPLIFIER_OCHANNEL_AUDIO_OUT,
  BSE_AMPLIFIER_N_OCHANNELS
};


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_AMPLIFIER_H__ */
