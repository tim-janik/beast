/* BseWaveOsc - BSE Wave Oscillator
 * Copyright (C) 2001 Tim Janik
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
 * bsewaveosc.h: BSE Wave Oscillator
 */
#ifndef __BSE_WAVE_OSC_H__
#define __BSE_WAVE_OSC_H__

#define  BSE_PLUGIN_NAME  "BseWaveOsc"

#include <bse/bseplugin.h>
#include <bse/bsesource.h>
#include <bse/bsewave.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- object type macros --- */
#define BSE_TYPE_WAVE_OSC              (type_id_wave_osc)
#define BSE_WAVE_OSC(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_WAVE_OSC, BseWaveOsc))
#define BSE_WAVE_OSC_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_WAVE_OSC, BseWaveOscClass))
#define BSE_IS_WAVE_OSC(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_WAVE_OSC))
#define BSE_IS_WAVE_OSC_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_WAVE_OSC))
#define BSE_WAVE_OSC_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_WAVE_OSC, BseWaveOscClass))


/* --- BseWaveOsc source --- */
typedef struct _BseWaveOsc      BseWaveOsc;
typedef struct _BseWaveOscClass BseWaveOscClass;
typedef struct {
  GslLong	 start_offset;
  gint		 play_dir;
  BseWaveIndex  *index;
} BseWaveOscVars;
struct _BseWaveOsc
{
  BseSource      parent_object;

  BseWave	*wave;
  BseWaveOscVars vars;
};
struct _BseWaveOscClass
{
  BseSourceClass parent_class;
};


/* --- channels --- */
enum
{
  BSE_WAVE_OSC_ICHANNEL_FREQ,
  BSE_WAVE_OSC_ICHANNEL_SYNC
};
enum
{
  BSE_WAVE_OSC_OCHANNEL_WAVE,
  BSE_WAVE_OSC_OCHANNEL_SYNC,
  BSE_WAVE_OSC_OCHANNEL_GATE
};


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_WAVE_OSC_H__ */
