/* DavGuitar - DAV Physical Modelling Acoustic Guitar Synthesizer
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
#ifndef __DAV_GUITAR_H__
#define __DAV_GUITAR_H__

#include <bse/bseplugin.h>
#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* --- object type macros --- */
#define DAV_TYPE_GUITAR              (type_id_guitar)
#define DAV_GUITAR(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), DAV_TYPE_GUITAR, DavGuitar))
#define DAV_GUITAR_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), DAV_TYPE_GUITAR, DavGuitar))
#define DAV_IS_GUITAR(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), DAV_TYPE_GUITAR))
#define DAV_IS_GUITAR_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), DAV_TYPE_GUITAR))
#define DAV_GUITAR_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), DavGuitarClass))

/* --- DavGuitar source --- */
typedef struct _DavGuitar      DavGuitar;
typedef struct _DavGuitarClass DavGuitarClass;

typedef struct _WaveGuide WaveGuide;

struct _WaveGuide
{
  gfloat  freq;
  gfloat  lowpass_data, lowpass_coeff;
  gint    wavelen;
  gint    pos;
  gfloat *data;
};

struct _DavGuitar
{
  BseSource parent_object;

  gfloat hipass_data, hipass_coeff;

  gint      body_taps[6];
  WaveGuide strings[6];
  WaveGuide body;
};

struct _DavGuitarClass
{
  BseSourceClass parent_class;
};


/* --- channels --- */
enum
{
  DAV_GUITAR_OCHANNEL_NONE,
  DAV_GUITAR_OCHANNEL_MONO
};



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DAV_GUITAR_H__ */
