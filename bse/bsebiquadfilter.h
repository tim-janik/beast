/* BSE - Bedevilled Sound Engine
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
#ifndef __BSE_BIQUAD_FILTER_H__
#define __BSE_BIQUAD_FILTER_H__

#include <bse/bsesource.h>
#include <bse/gslfilter.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- type macros --- */
#define BSE_TYPE_BIQUAD_FILTER              (BSE_TYPE_ID (BseBiquadFilter))
#define BSE_BIQUAD_FILTER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_BIQUAD_FILTER, BseBiquadFilter))
#define BSE_BIQUAD_FILTER_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_BIQUAD_FILTER, BseBiquadFilterClass))
#define BSE_IS_BIQUAD_FILTER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_BIQUAD_FILTER))
#define BSE_IS_BIQUAD_FILTER_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_BIQUAD_FILTER))
#define BSE_BIQUAD_FILTER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_BIQUAD_FILTER, BseBiquadFilterClass))


/* --- enums --- */
typedef enum
{
  BSE_BIQUAD_FILTER_RESONANT_LOWPASS  = GSL_BIQUAD_RESONANT_LOWPASS,
  BSE_BIQUAD_FILTER_RESONANT_HIGHPASS = GSL_BIQUAD_RESONANT_HIGHPASS,
} BseBiquadFilterType;
typedef enum    /* skip */
{
  BSE_BIQUAD_FILTER_NORM_PASSBAND	= GSL_BIQUAD_NORMALIZE_PASSBAND,
  BSE_BIQUAD_FILTER_NORM_RESONANCE_GAIN	= GSL_BIQUAD_NORMALIZE_RESONANCE_GAIN,
  BSE_BIQUAD_FILTER_NORM_PEAK_GAIN	= GSL_BIQUAD_NORMALIZE_PEAK_GAIN
} BseBiquadFilterNorm;


/* --- BseBiquadFilter source --- */
typedef struct _BseBiquadFilter      BseBiquadFilter;
typedef struct _BseBiquadFilterClass BseBiquadFilterClass;
struct _BseBiquadFilter
{
  BseSource	parent_object;

  BseBiquadFilterType filter_type;
  guint		      type_change : 1;
  guint		      exponential_fm : 1;
  gfloat	      freq;
  gfloat	      fm_strength;
  gfloat	      fm_n_octaves;
  BseBiquadFilterNorm norm_type;
  gfloat	      gain;
  gfloat	      gain_strength;
};
struct _BseBiquadFilterClass
{
  BseSourceClass parent_class;
};


/* --- channels --- */
enum
{
  BSE_BIQUAD_FILTER_ICHANNEL_AUDIO,
  BSE_BIQUAD_FILTER_ICHANNEL_FREQ,
  BSE_BIQUAD_FILTER_ICHANNEL_FREQ_MOD,
  BSE_BIQUAD_FILTER_ICHANNEL_GAIN_MOD,
  BSE_BIQUAD_FILTER_N_ICHANNELS
};
enum
{
  BSE_BIQUAD_FILTER_OCHANNEL_AUDIO,
  BSE_BIQUAD_FILTER_N_OCHANNELS
};


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_BIQUAD_FILTER_H__ */
