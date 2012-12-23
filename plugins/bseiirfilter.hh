/* BseIIRFilter - BSE Infinite Impulse Response Filter
 * Copyright (C) 1999-2002 Tim Janik
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
#ifndef __BSE_IIR_FILTER_H__
#define __BSE_IIR_FILTER_H__

#include <bse/bsesource.hh>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- type macros --- */
#define BSE_TYPE_IIR_FILTER              (bse_iir_filter_get_type())
#define BSE_IIR_FILTER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_IIR_FILTER, BseIIRFilter))
#define BSE_IIR_FILTER_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_IIR_FILTER, BseIIRFilterClass))
#define BSE_IS_IIR_FILTER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_IIR_FILTER))
#define BSE_IS_IIR_FILTER_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_IIR_FILTER))
#define BSE_IIR_FILTER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_IIR_FILTER, BseIIRFilterClass))
#define	BSE_IIR_FILTER_MAX_ORDER	 (18)


/* --- BseIIRFilter source --- */
typedef struct _BseIIRFilter      BseIIRFilter;
typedef struct _BseIIRFilterClass BseIIRFilterClass;
typedef struct
{
  gdouble a[BSE_IIR_FILTER_MAX_ORDER];
  gdouble b[BSE_IIR_FILTER_MAX_ORDER];
} BseIIRFilterVars;
struct _BseIIRFilter
{
  BseSource	parent_object;

  BseIIRFilterAlgorithm filter_algo;
  BseIIRFilterType      filter_type;
  guint		        algo_type_change : 1;

  guint		order;
  gdouble	epsilon;
  gfloat	cut_off_freq1;
  gfloat	cut_off_freq2;	/* band pass/stop */

  gdouble	a[BSE_IIR_FILTER_MAX_ORDER + 1];
  gdouble	b[BSE_IIR_FILTER_MAX_ORDER + 1];
};
struct _BseIIRFilterClass
{
  BseSourceClass parent_class;
};


/* --- channels --- */
enum
{
  BSE_IIR_FILTER_ICHANNEL_MONO,
  BSE_IIR_FILTER_N_ICHANNELS
};
enum
{
  BSE_IIR_FILTER_OCHANNEL_MONO,
  BSE_IIR_FILTER_N_OCHANNELS
};


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_IIR_FILTER_H__ */
