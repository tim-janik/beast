/* DavSynDrum - DAV Drum Synthesizer
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

#ifndef __DAV_SYNDRUM_H__
#define __DAV_SYNDRUM_H__

#include <bse/bseplugin.h>
#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* --- object type macros --- */
#define DAV_TYPE_SYN_DRUM              (type_id_syn_drum)
#define DAV_SYN_DRUM(object)           (BSE_CHECK_STRUCT_CAST ((object), DAV_TYPE_SYN_DRUM, DavSynDrum))
#define DAV_SYN_DRUM_CLASS(class)      (BSE_CHECK_CLASS_CAST ((class), DAV_TYPE_SYN_DRUM, DavSynDrumClass))
#define DAV_IS_SYN_DRUM(object)        (BSE_CHECK_STRUCT_TYPE ((object), DAV_TYPE_SYN_DRUM))
#define DAV_IS_SYN_DRUM_CLASS(class)   (BSE_CHECK_CLASS_TYPE ((class), DAV_TYPE_SYN_DRUM))
#define DAV_SYN_DRUM_GET_CLASS(object) ((DavSynDrumClass*) (((BseObject*) (object))->bse_struct.bse_class))

/* --- DavSynDrum source --- */
typedef struct _DavSynDrum      DavSynDrum;
typedef struct _DavSynDrumClass DavSynDrumClass;

struct _DavSynDrum
{
  BseSource parent_object;
  
  gfloat      ratio;
  gfloat      freq;
  gfloat      trigger_vel;
  gfloat      spring_pos;
  gfloat      spring_vel;
  gfloat      env;
  gfloat      half;
  gfloat      res;

  guint       input_trigger_state : 1;
};

struct _DavSynDrumClass
{
  BseSourceClass parent_class;
};


/* --- channels --- */
enum
{
  BSE_SYN_DRUM_ICHANNEL_NONE,
  BSE_SYN_DRUM_ICHANNEL_TRIGGER
};
enum
{
  DAV_SYN_DRUM_OCHANNEL_NONE,
  DAV_SYN_DRUM_OCHANNEL_MONO
};


/* --- prototypes --- */
void	dav_syn_drum_trigger	(DavSynDrum	*drum);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DAV_SYNDRUM_H__ */
