/* DavChorus - DAV Chorus Effect
 * Copyright (c) 2000 David A. Bartold
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
#ifndef __DAV_CHORUS_H__
#define __DAV_CHORUS_H__

#include <bse/bseplugin.h>
#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define DAV_TYPE_CHORUS              (type_id_chorus)
#define DAV_CHORUS(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), DAV_TYPE_CHORUS, DavChorus))
#define DAV_CHORUS_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), DAV_TYPE_CHORUS, DavChorus))
#define DAV_IS_CHORUS(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), DAV_TYPE_CHORUS))
#define DAV_IS_CHORUS_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), DAV_TYPE_CHORUS))
#define DAV_CHORUS_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), DavChorus))


/* --- DavChorus source --- */
typedef struct _DavChorus      DavChorus;
typedef struct _DavChorusClass DavChorusClass;

struct _DavChorus
{
  BseSource parent_object;
  
  BseSampleValue *delay;
  gint            delay_length;
  gint            delay_pos;
  gfloat          sine_pos;
  gfloat          sine_delta;
  gfloat          wet_out;
};

struct _DavChorusClass
{
  BseSourceClass parent_class;
};


/* --- channels --- */
enum
{
  DAV_CHORUS_ICHANNEL_NONE,
  DAV_CHORUS_ICHANNEL_MONO
};
enum
{
  DAV_CHORUS_OCHANNEL_NONE,
  DAV_CHORUS_OCHANNEL_MONO
};


/* --- prototypes --- */



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DAV_CHORUS_H__ */
