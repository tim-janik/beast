/* BseSimpleADSR - BSE Simpl ADSR Envelope Generator
 * Copyright (C) 2001 Tim Janik
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
#ifndef __BSE_SIMPLE_ADSR_H__
#define __BSE_SIMPLE_ADSR_H__

#define  BSE_PLUGIN_NAME  "BseSimpleADSR"

#include <bse/bseplugin.h>
#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_SIMPLE_ADSR              (type_id_simple_adsr)
#define BSE_SIMPLE_ADSR(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SIMPLE_ADSR, BseSimpleADSR))
#define BSE_SIMPLE_ADSR_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SIMPLE_ADSR, BseSimpleADSRClass))
#define BSE_IS_SIMPLE_ADSR(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SIMPLE_ADSR))
#define BSE_IS_SIMPLE_ADSR_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SIMPLE_ADSR))
#define BSE_SIMPLE_ADSR_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SIMPLE_ADSR, BseSimpleADSRClass))


/* --- BseSimpleADSR source --- */
typedef struct _BseSimpleADSR      BseSimpleADSR;
typedef struct _BseSimpleADSRClass BseSimpleADSRClass;
typedef struct {
  gfloat	  attack_inc;
  gfloat	  decay_dec;
  gfloat	  sustain_level;
  gfloat	  release_dec;
} BseSimpleADSRVars;
struct _BseSimpleADSR
{
  BseSource         parent_object;

  gfloat	    attack_time;
  gfloat	    decay_time;
  gfloat	    sustain_level;
  gfloat	    release_time;
  BseTimeRangeType  time_range;
};
struct _BseSimpleADSRClass
{
  BseSourceClass parent_class;
};


/* --- channels --- */
enum
{
  BSE_SIMPLE_ADSR_ICHANNEL_GATE,
  BSE_SIMPLE_ADSR_ICHANNEL_RETRIGGER
};
enum
{
  BSE_SIMPLE_ADSR_OCHANNEL_OUT
};



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_SIMPLE_ADSR_H__ */
