/* BseAtanDistort - BSE Arcus Tangens alike Distortion
 * Copyright (C) 1999, 2000-2002 Tim Janik
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
#ifndef __BSE_ATAN_DISTORT_H__
#define __BSE_ATAN_DISTORT_H__

#include <bse/bseplugin.h>
#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_ATAN_DISTORT              (BSE_EXPORT_TYPE_ID (BseAtanDistort))
#define BSE_ATAN_DISTORT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_ATAN_DISTORT, BseAtanDistort))
#define BSE_ATAN_DISTORT_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_ATAN_DISTORT, BseAtanDistortClass))
#define BSE_IS_ATAN_DISTORT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_ATAN_DISTORT))
#define BSE_IS_ATAN_DISTORT_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_ATAN_DISTORT))
#define BSE_ATAN_DISTORT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_ATAN_DISTORT, BseAtanDistortClass))


/* --- BseAtanDistort source --- */
typedef struct _BseAtanDistort      BseAtanDistort;
typedef struct _BseAtanDistortClass BseAtanDistortClass;
struct _BseAtanDistort
{
  BseSource         parent_object;

  gfloat	    boost_amount;
  gdouble	    prescale;
};
struct _BseAtanDistortClass
{
  BseSourceClass parent_class;
};


/* --- channels --- */
enum
{
  BSE_ATAN_DISTORT_ICHANNEL_MONO1,
  BSE_ATAN_DISTORT_N_ICHANNELS
};
enum
{
  BSE_ATAN_DISTORT_OCHANNEL_MONO1,
  BSE_ATAN_DISTORT_N_OCHANNELS
};



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_ATAN_DISTORT_H__ */
