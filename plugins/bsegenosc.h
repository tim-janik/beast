/* BseGenOsc - BSE Generic Oscillator
 * Copyright (C) 1999 Tim Janik
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
 * bsegenosc.h: BSE Generic Oscillator
 */
#ifndef __BSE_GEN_OSC_H__
#define __BSE_GEN_OSC_H__

#define  BSE_PLUGIN_NAME  "BseGenOsc"

#include <bse/bseplugin.h>
#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */





/* --- object type macros --- */
#define BSE_TYPE_GEN_OSC              (type_id_gen_osc)
#define BSE_GEN_OSC(object)           (BSE_CHECK_STRUCT_CAST ((object), BSE_TYPE_GEN_OSC, BseGenOsc))
#define BSE_GEN_OSC_CLASS(class)      (BSE_CHECK_CLASS_CAST ((class), BSE_TYPE_GEN_OSC, BseGenOscClass))
#define BSE_IS_GEN_OSC(object)        (BSE_CHECK_STRUCT_TYPE ((object), BSE_TYPE_GEN_OSC))
#define BSE_IS_GEN_OSC_CLASS(class)   (BSE_CHECK_CLASS_TYPE ((class), BSE_TYPE_GEN_OSC))
#define BSE_GEN_OSC_GET_CLASS(object) ((BseGenOscClass*) (((BseObject*) (object))->bse_struct.bse_class))


/* --- wave forms --- */
enum {
  BSE_GEN_OSC_NOWAVE,
  BSE_GEN_OSC_SINE,
  BSE_GEN_OSC_GSAW,
  BSE_GEN_OSC_SSAW,
  BSE_GEN_OSC_PULSE,
  BSE_GEN_OSC_TRIANGLE
};


/* --- BseGenOsc source --- */
typedef struct _BseGenOsc      BseGenOsc;
typedef struct _BseGenOscClass BseGenOscClass;
struct _BseGenOsc
{
  BseSource       parent_object;

  guint   wave;
  gfloat  phase;
  gfloat  base_freq;
  gfloat  fm_perc;

  guint32 rate_pos;
  guint32 rate;
  gfloat  fm_strength;
  guint   table_size;
  BseSampleValue *table;
  BseSampleValue  last_value;
};
struct _BseGenOscClass
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
  BSE_GEN_OSC_OCHANNEL_NONE,
  BSE_GEN_OSC_OCHANNEL_MONO
};
enum
{
  BSE_GEN_OSC_ICHANNEL_NONE,
  BSE_GEN_OSC_ICHANNEL_FREQ_MOD
};


/* --- prototypes --- */
void	bse_gen_osc_sync	(BseGenOsc	*gosc);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_GEN_OSC_H__ */
