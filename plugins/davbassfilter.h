/* DavBassFilter - DAV Bass Filter
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
#ifndef __DAV_BASS_FILTER_H__
#define __DAV_BASS_FILTER_H__

#include <bse/bseplugin.h>
#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* --- object type macros --- */
#define DAV_TYPE_BASS_FILTER              (type_id_bass_filter)
#define DAV_BASS_FILTER(object)           (BSE_CHECK_STRUCT_CAST ((object), DAV_TYPE_BASS_FILTER, DavBassFilter))
#define DAV_BASS_FILTER_CLASS(class)      (BSE_CHECK_CLASS_CAST ((class), DAV_TYPE_BASS_FILTER, DavBassFilter))
#define DAV_IS_BASS_FILTER(object)        (BSE_CHECK_STRUCT_TYPE ((object), DAV_TYPE_BASS_FILTER))
#define DAV_IS_BASS_FILTER_CLASS(class)   (BSE_CHECK_CLASS_TYPE ((class), DAV_TYPE_BASS_FILTER))
#define DAV_BASS_FILTER_GET_CLASS(object) ((DavBassFilter (Class*) (((BseObject*) (object))->bse_struct.bse_class))

/* --- DavBassFilter source --- */
typedef struct _DavBassFilter      DavBassFilter;
typedef struct _DavBassFilterClass DavBassFilterClass;

struct _DavBassFilter
{
  BseSource parent_object;
  
  gfloat cutoff;
  gfloat envmod;
  gfloat envdecay;
  gfloat reso;

  gfloat a, b;
  gfloat decay;
  gfloat resonance;
  gfloat e0, e1;
  gfloat c0;
  gfloat d1, d2;
  gint envpos;
};

struct _DavBassFilterClass
{
  BseSourceClass parent_class;
};


/* --- channels --- */
enum
{
  DAV_BASS_FILTER_ICHANNEL_NONE,
  DAV_BASS_FILTER_ICHANNEL_MONO
};
enum
{
  DAV_BASS_FILTER_OCHANNEL_NONE,
  DAV_BASS_FILTER_OCHANNEL_MONO
};


/* --- prototypes --- */



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DAV_BASS_FILTER_H__ */
