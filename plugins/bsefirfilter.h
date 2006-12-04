/* BseFIRFilter - BSE Finite Impulse Response Filter
 * Copyright (C) 1999 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library FIReral Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU FIReral Public License for more details.
 *
 * You should have received a copy of the GNU Library FIReral Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * bsefirfilter.h: BSE Finite Impulse Response Filter
 */
#ifndef __BSE_FIR_FILTER_H__
#define __BSE_FIR_FILTER_H__

#include <bse/bseplugin.h>
#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */





/* --- object type macros --- */
#define BSE_TYPE_FIR_FILTER              (type_id_fir_filter)
#define BSE_FIR_FILTER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_FIR_FILTER, BseFIRFilter))
#define BSE_FIR_FILTER_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_FIR_FILTER, BseFIRFilterClass))
#define BSE_IS_FIR_FILTER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_FIR_FILTER))
#define BSE_IS_FIR_FILTER_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_FIR_FILTER))
#define BSE_FIR_FILTER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_FIR_FILTER, BseFIRFilterClass))


/* --- BseFIRFilter source --- */
typedef struct _BseFIRFilter      BseFIRFilter;
typedef struct _BseFIRFilterClass BseFIRFilterClass;
struct _BseFIRFilter
{
  BseSource       parent_object;

  guint16 degree;
  guint   filter_type : 8;
  guint   lanczos_smoothing : 1;
  guint   hann_smoothing : 1;
  gfloat  cut_off_freq;

  guint           n_coeffs;
  gfloat         *coeffs;
  guint           history_pos;
  BseSampleValue *history;
};
struct _BseFIRFilterClass
{
  BseSourceClass parent_class;
};


/* --- enums --- */
typedef enum
{
  BSE_FIR_FILTER_ALLPASS,
  BSE_FIR_FILTER_LOWPASS,
  BSE_FIR_FILTER_HIGHPASS
} BseFIRFilterType;


/* --- channels --- */
enum
{
  BSE_FIR_FILTER_OCHANNEL_NONE,
  BSE_FIR_FILTER_OCHANNEL_MONO
};
enum
{
  BSE_FIR_FILTER_ICHANNEL_NONE,
  BSE_FIR_FILTER_ICHANNEL_MONO
};




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_FIR_FILTER_H__ */
