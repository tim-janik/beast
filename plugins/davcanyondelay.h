/* DavCanyonDelay - DAV Canyon Delay
 * Copyright (c) 1999, 2000 David A. Bartold
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
#ifndef __DAV_CANYONDELAY_H__
#define __DAV_CANYONDELAY_H__

#include <bse/bseplugin.h>
#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* --- object type macros --- */
#define DAV_TYPE_CANYON_DELAY              (type_id_canyon_delay)
#define DAV_CANYON_DELAY(object)           (BSE_CHECK_STRUCT_CAST ((object), DAV_TYPE_CANYON_DELAY, DavCanyonDelay))
#define DAV_CANYON_DELAY_CLASS(class)      (BSE_CHECK_CLASS_CAST ((class), DAV_TYPE_CANYON_DELAY, DavCanyonDelayClass))
#define DAV_IS_CANYON_DELAY(object)        (BSE_CHECK_STRUCT_TYPE ((object), DAV_TYPE_CANYON_DELAY))
#define DAV_IS_CANYON_DELAY_CLASS(class)   (BSE_CHECK_CLASS_TYPE ((class), DAV_TYPE_CANYON_DELAY))
#define DAV_CANYON_DELAY_GET_CLASS(object) ((DavCanyonDelayClass*) (((BseObject*) (object))->bse_struct.bse_class))

/* --- DavCanyonDelay source --- */
typedef struct _DavCanyonDelay      DavCanyonDelay;
typedef struct _DavCanyonDelayClass DavCanyonDelayClass;

struct _DavCanyonDelay
{
  BseSource parent_object;
  
  BseSampleValue *data_l;
  BseSampleValue *data_r;
  BseSampleValue accum_l;
  BseSampleValue accum_r;

  gfloat l_to_r_seconds;
  gfloat r_to_l_seconds;
  gint32 l_to_r_pos;
  gint32 r_to_l_pos;

  gfloat l_to_r_feedback;
  gfloat r_to_l_feedback;
  gint32 l_to_r_mag;
  gint32 r_to_l_mag;
  gint32 l_to_r_invmag;
  gint32 r_to_l_invmag;

  gfloat filter_freq;
  gint32 filter_mag;
  gint32 filter_invmag;

  gint32 datasize;
  gint32 pos;
};

struct _DavCanyonDelayClass
{
  BseSourceClass parent_class;
};


/* --- channels --- */
enum
{
  DAV_CANYON_DELAY_ICHANNEL_NONE,
  DAV_CANYON_DELAY_ICHANNEL_MULTI
};
enum
{
  DAV_CANYON_DELAY_OCHANNEL_NONE,
  DAV_CANYON_DELAY_OCHANNEL_STEREO
};


/* --- prototypes --- */



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DAV_CANYONDELAY_H__ */
