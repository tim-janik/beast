/* BseSimpleOsc - BSE Simple Oscillator
 * Copyright (C) 1999,2000-2001 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library Simpleeral Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Simpleeral Public License for more details.
 *
 * You should have received a copy of the GNU Library Simpleeral Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __BSE_SIMPLE_OSC_H__
#define __BSE_SIMPLE_OSC_H__

#define  BSE_PLUGIN_NAME  "BseSimpleOsc"

#include <bse/bseplugin.h>
#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_SIMPLE_OSC              (type_id_simple_osc)
#define BSE_SIMPLE_OSC(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SIMPLE_OSC, BseSimpleOsc))
#define BSE_SIMPLE_OSC_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SIMPLE_OSC, BseSimpleOscClass))
#define BSE_IS_SIMPLE_OSC(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SIMPLE_OSC))
#define BSE_IS_SIMPLE_OSC_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SIMPLE_OSC))
#define BSE_SIMPLE_OSC_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SIMPLE_OSC, BseSimpleOscClass))


/* --- wave forms --- */
typedef enum
{
  BSE_SIMPLE_OSC_NONE,		/*< skip >*/
  BSE_SIMPLE_OSC_SINE,		/*< nick=Sine >*/
  BSE_SIMPLE_OSC_PULSE,		/*< nick=Pulse >*/
  BSE_SIMPLE_OSC_GSAW,		/*< nick=Growing Saw >*/
  BSE_SIMPLE_OSC_SSAW,		/*< nick=Shrinking Saw >*/
  BSE_SIMPLE_OSC_TRIANGLE	/*< nick=Triangle >*/
} BseSimpleOscWaveType;


/* --- BseSimpleOsc source --- */
typedef struct _BseSimpleOsc      BseSimpleOsc;
typedef struct _BseSimpleOscClass BseSimpleOscClass;
typedef struct {
  guint32	  n_table_values;
  BseSampleValue *table;	/* [n_table_values + 1] */

  guint32	  sync_pos;	/* phase */
  gfloat	  fm_strength;
  gboolean	  with_fm_mod;
} BseSimpleOscVars;
struct _BseSimpleOsc
{
  BseSource         parent_object;

  BseSimpleOscWaveType wave;
  gfloat               phase;
  gfloat               base_freq;
  gfloat               fm_perc;
};
struct _BseSimpleOscClass
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
  BSE_SIMPLE_OSC_ICHANNEL_FREQ,
  BSE_SIMPLE_OSC_ICHANNEL_FREQ_MOD,
  BSE_SIMPLE_OSC_ICHANNEL_SYNC
};
enum
{
  BSE_SIMPLE_OSC_OCHANNEL_OSC,
  BSE_SIMPLE_OSC_OCHANNEL_SYNC
};



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_SIMPLE_OSC_H__ */
