/* DavXtalStrings - DAV Physical Modelling String Synthesizer
 * Copyright (c) 2000 David A. Bartold
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __DAV_XTAL_STRINGS_H__
#define __DAV_XTAL_STRINGS_H__

#include <bse/bseplugin.h>
#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* --- object type macros --- */
#define DAV_TYPE_XTAL_STRINGS		   (type_id_xtal_strings)
#define DAV_XTAL_STRINGS(object)	   (G_TYPE_CHECK_INSTANCE_CAST ((object), DAV_TYPE_XTAL_STRINGS, DavXtalStrings))
#define DAV_XTAL_STRINGS_CLASS(class)	   (G_TYPE_CHECK_CLASS_CAST ((class), DAV_TYPE_XTAL_STRINGS, DavXtalStringsClass))
#define DAV_IS_XTAL_STRINGS(object)	   (G_TYPE_CHECK_INSTANCE_TYPE ((object), DAV_TYPE_XTAL_STRINGS))
#define DAV_IS_XTAL_STRINGS_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), DAV_TYPE_XTAL_STRINGS))
#define DAV_XTAL_STRINGS_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), DavXtalStringsClass))


/* --- DavXtalStrings source --- */
typedef struct _DavXtalStrings	    DavXtalStrings;
typedef struct _DavXtalStringsClass DavXtalStringsClass;

struct _DavXtalStrings
{
  BseSource parent_object;
  
  gfloat      freq;
  gfloat      trigger_vel;
  gfloat      note_decay;
  gfloat      tension_decay;
  gfloat      metallic_factor;
  gfloat      snap_factor;
  
  gfloat      a;
  gfloat      damping_factor;
  
  gfloat      d;
  gfloat      *string;
  gint	      size;
  gint	      pos;
  guint	      count;
  
  guint	      input_trigger_state : 1;
};

struct _DavXtalStringsClass
{
  BseSourceClass parent_class;
};


/* --- channels --- */
enum
{
  DAV_XTAL_STRINGS_OCHANNEL_NONE,
  DAV_XTAL_STRINGS_OCHANNEL_MONO
};


/* --- prototypes --- */
void	dav_xtal_strings_trigger	(DavXtalStrings *strings);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DAV_XTAL_STRINGS_H__ */
