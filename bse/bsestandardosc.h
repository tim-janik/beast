/* BseStandardOsc - BSE Standard Oscillator
 * Copyright (C) 1999, 2000-2004 Tim Janik
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
#ifndef __BSE_STANDARD_OSC_H__
#define __BSE_STANDARD_OSC_H__

#include <bse/bsesource.h>
#include <bse/gsloscillator.h>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_STANDARD_OSC              (BSE_TYPE_ID (BseStandardOsc))
#define BSE_STANDARD_OSC(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_STANDARD_OSC, BseStandardOsc))
#define BSE_STANDARD_OSC_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_STANDARD_OSC, BseStandardOscClass))
#define BSE_IS_STANDARD_OSC(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_STANDARD_OSC))
#define BSE_IS_STANDARD_OSC_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_STANDARD_OSC))
#define BSE_STANDARD_OSC_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_STANDARD_OSC, BseStandardOscClass))


/* --- wave forms --- */
typedef enum
{
  BSE_STANDARD_OSC_SINE		= GSL_OSC_WAVE_SINE,		/*< nick=Sine >*/
  BSE_STANDARD_OSC_TRIANGLE	= GSL_OSC_WAVE_TRIANGLE,	/*< nick=Triangle >*/
  BSE_STANDARD_OSC_SAW_RISE	= GSL_OSC_WAVE_SAW_RISE,	/*< nick=Rising Saw >*/
  BSE_STANDARD_OSC_SAW_FALL	= GSL_OSC_WAVE_SAW_FALL,	/*< nick=Falling Saw >*/
  BSE_STANDARD_OSC_PEAK_RISE	= GSL_OSC_WAVE_PEAK_RISE,	/*< nick=Rising Peak >*/
  BSE_STANDARD_OSC_PEAK_FALL	= GSL_OSC_WAVE_PEAK_FALL,	/*< nick=Falling Peak >*/
  BSE_STANDARD_OSC_MOOG_SAW	= GSL_OSC_WAVE_MOOG_SAW,	/*< nick=Moog Saw >*/
  BSE_STANDARD_OSC_SQUARE	= GSL_OSC_WAVE_SQUARE,		/*< nick=Square >*/
  BSE_STANDARD_OSC_PULSE	= GSL_OSC_WAVE_PULSE_SAW	/*< nick=Pulse >*/
} BseStandardOscWaveType;


/* --- BseStandardOsc source --- */
typedef struct _BseStandardOsc      BseStandardOsc;
typedef struct _BseStandardOscClass BseStandardOscClass;
struct _BseStandardOsc
{
  BseSource		 parent_object;

  BseStandardOscWaveType wave;
  GslOscConfig		 config;
  int                    transpose;
  gfloat                 fm_strength;
  gfloat                 n_octaves;
};
struct _BseStandardOscClass
{
  BseSourceClass parent_class;
};


/* --- channels --- */
enum
{
  BSE_STANDARD_OSC_ICHANNEL_FREQ,
  BSE_STANDARD_OSC_ICHANNEL_FREQ_MOD,
  BSE_STANDARD_OSC_ICHANNEL_PWM,
  BSE_STANDARD_OSC_ICHANNEL_SYNC,
  BSE_STANDARD_OSC_N_ICHANNELS
};
enum
{
  BSE_STANDARD_OSC_OCHANNEL_OSC,
  BSE_STANDARD_OSC_OCHANNEL_SYNC,
  BSE_STANDARD_OSC_N_OCHANNELS
};

G_END_DECLS

#endif /* __BSE_STANDARD_OSC_H__ */
