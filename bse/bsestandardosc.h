/* BseStandardOsc - BSE Standard Oscillator
 * Copyright (C) 1999, 2000-2002 Tim Janik
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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


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
  BSE_STANDARD_OSC_NONE,	/*< skip >*/
  BSE_STANDARD_OSC_SINE,	/*< nick=Sine >*/
  BSE_STANDARD_OSC_PULSE,	/*< nick=Pulse >*/
  BSE_STANDARD_OSC_GSAW,	/*< nick=Growing Saw >*/
  BSE_STANDARD_OSC_SSAW,	/*< nick=Shrinking Saw >*/
  BSE_STANDARD_OSC_TRIANGLE	/*< nick=Triangle >*/
} BseStandardOscWaveType;


/* --- BseStandardOsc source --- */
typedef struct _BseStandardOsc      BseStandardOsc;
typedef struct _BseStandardOscClass BseStandardOscClass;
typedef struct {
  guint32	  n_table_values;
  BseSampleValue *table;	/* [n_table_values + 1] */

  guint32	  sync_pos;	/* phase */
  gfloat	  fm_strength;
  gboolean	  with_fm_mod;
} BseStandardOscVars;
struct _BseStandardOsc
{
  BseSource         parent_object;

  BseStandardOscWaveType wave;
  gfloat               phase;
  gfloat               base_freq;
  gfloat               fm_perc;
};
struct _BseStandardOscClass
{
  BseSourceClass parent_class;

  guint           ref_count;

  guint           sine_table_size;
  BseSampleValue *sine_table;
  guint           gsaw_table_size;
  BseSampleValue *gsaw_table;
  guint           ssaw_table_size;
  BseSampleValue *ssaw_table;
  guint           pulse_table_size;
  BseSampleValue *pulse_table;
  guint           triangle_table_size;
  BseSampleValue *triangle_table;
};


/* --- channels --- */
enum
{
  BSE_STANDARD_OSC_ICHANNEL_FREQ,
  BSE_STANDARD_OSC_ICHANNEL_FREQ_MOD,
  BSE_STANDARD_OSC_ICHANNEL_SYNC
};
enum
{
  BSE_STANDARD_OSC_OCHANNEL_OSC,
  BSE_STANDARD_OSC_OCHANNEL_SYNC
};



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_STANDARD_OSC_H__ */
