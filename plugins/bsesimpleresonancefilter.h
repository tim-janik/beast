/* BseSimpleResonanceFilter - BSE IIR Resonance Filter
 * Copyright (C) 2002 Tim Janik
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
 */
#ifndef __BSE_SIMPLE_RESONANCE_FILTER_H__
#define __BSE_SIMPLE_RESONANCE_FILTER_H__

#define  BSE_PLUGIN_NAME  "BseSimpleResonanceFilter"

#include <bse/bseplugin.h>
#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- type macros --- */
#define BSE_TYPE_SIMPLE_RESONANCE_FILTER              (type_id_simple_resonance_filter)
#define BSE_SIMPLE_RESONANCE_FILTER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SIMPLE_RESONANCE_FILTER, BseSimpleResonanceFilter))
#define BSE_SIMPLE_RESONANCE_FILTER_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SIMPLE_RESONANCE_FILTER, BseSimpleResonanceFilterClass))
#define BSE_IS_SIMPLE_RESONANCE_FILTER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SIMPLE_RESONANCE_FILTER))
#define BSE_IS_SIMPLE_RESONANCE_FILTER_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SIMPLE_RESONANCE_FILTER))
#define BSE_SIMPLE_RESONANCE_FILTER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SIMPLE_RESONANCE_FILTER, BseSimpleResonanceFilterClass))


/* --- BseSimpleResonanceFilter source --- */
typedef struct _BseSimpleResonanceFilter      BseSimpleResonanceFilter;
typedef struct _BseSimpleResonanceFilterClass BseSimpleResonanceFilterClass;
struct _BseSimpleResonanceFilter
{
  BseSource	parent_object;

  gdouble	c1;
  gdouble	c2;
};
struct _BseSimpleResonanceFilterClass
{
  BseSourceClass parent_class;
};


/* --- channels --- */
enum
{
  BSE_SIMPLE_RESONANCE_FILTER_ICHANNEL_MONO,
  BSE_SIMPLE_RESONANCE_FILTER_N_ICHANNELS
};
enum
{
  BSE_SIMPLE_RESONANCE_FILTER_OCHANNEL_MONO,
  BSE_SIMPLE_RESONANCE_FILTER_N_OCHANNELS
};


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_SIMPLE_RESONANCE_FILTER_H__ */
