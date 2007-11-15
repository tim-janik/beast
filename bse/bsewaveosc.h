/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2001-2003 Tim Janik
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
 */
#ifndef __BSE_WAVE_OSC_H__
#define __BSE_WAVE_OSC_H__

#include <bse/bsesource.h>
#include <bse/bsewave.h>
#include <bse/gslwaveosc.h>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_WAVE_OSC              (BSE_TYPE_ID (BseWaveOsc))
#define BSE_WAVE_OSC(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_WAVE_OSC, BseWaveOsc))
#define BSE_WAVE_OSC_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_WAVE_OSC, BseWaveOscClass))
#define BSE_IS_WAVE_OSC(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_WAVE_OSC))
#define BSE_IS_WAVE_OSC_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_WAVE_OSC))
#define BSE_WAVE_OSC_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_WAVE_OSC, BseWaveOscClass))


/* --- BseWaveOsc source --- */
typedef struct _BseWaveOsc      BseWaveOsc;
typedef struct _BseWaveOscClass BseWaveOscClass;
struct _BseWaveOsc
{
  BseSource          parent_object;
  
  BseWave           *wave;
  GslWaveChunk      *esample_wchunk;
  GslWaveOscConfig   config;
  gfloat             fm_strength;
  gfloat             n_octaves;
};
struct _BseWaveOscClass
{
  BseSourceClass parent_class;
};


/* --- prototypes --- */
void    bse_wave_osc_request_pcm_position       (BseWaveOsc        *self);
void    bse_wave_osc_mass_seek                  (guint              n_woscs,
                                                 BseWaveOsc       **woscs,
                                                 gfloat             perc);
void    bse_wave_osc_set_from_esample           (BseWaveOsc        *self,
                                                 BseEditableSample *esample);


/* --- channels --- */
enum
{
  BSE_WAVE_OSC_ICHANNEL_FREQ,
  BSE_WAVE_OSC_ICHANNEL_SYNC,
  BSE_WAVE_OSC_ICHANNEL_MOD,
  BSE_WAVE_OSC_N_ICHANNELS
};
enum
{
  BSE_WAVE_OSC_OCHANNEL_WAVE,
  BSE_WAVE_OSC_OCHANNEL_GATE,
  BSE_WAVE_OSC_OCHANNEL_DONE,
  BSE_WAVE_OSC_N_OCHANNELS
};

G_END_DECLS

#endif /* __BSE_WAVE_OSC_H__ */
