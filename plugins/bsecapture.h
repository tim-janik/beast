/* BseCapture - BSE Recording output source
 * Copyright (C) 1999 Tim Janik
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
 *
 * bsecapture.h: BSE Recording output source, reading from /dev/dsp
 */
#ifndef __BSE_CAPTURE_H__
#define __BSE_CAPTURE_H__

#define  BSE_PLUGIN_NAME  "BseCapture"

#include <bse/bseplugin.h>
#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */





/* --- object type macros --- */
#define BSE_TYPE_CAPTURE              (type_id_capture)
#define BSE_CAPTURE(object)           (BSE_CHECK_STRUCT_CAST ((object), BSE_TYPE_CAPTURE, BseCapture))
#define BSE_CAPTURE_CLASS(class)      (BSE_CHECK_CLASS_CAST ((class), BSE_TYPE_CAPTURE, BseCaptureClass))
#define BSE_IS_CAPTURE(object)        (BSE_CHECK_STRUCT_TYPE ((object), BSE_TYPE_CAPTURE))
#define BSE_IS_CAPTURE_CLASS(class)   (BSE_CHECK_CLASS_TYPE ((class), BSE_TYPE_CAPTURE))
#define BSE_CAPTURE_GET_CLASS(object) ((BseCaptureClass*) (((BseObject*) (object))->bse_struct.bse_class))


/* --- BseCapture object --- */
typedef struct _BseCapture      BseCapture;
typedef struct _BseCaptureClass BseCaptureClass;
struct _BseCapture
{
  BseSource       parent_object;
};
struct _BseCaptureClass
{
  BseSourceClass parent_class;

  guint           ref_count;
  BseIndex        n_buffers;
  BseSampleValue *buffer;
};


/* --- channels --- */
enum
{
  BSE_CAPTURE_OCHANNEL_NONE,
  BSE_CAPTURE_OCHANNEL_MONO
};





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_CAPTURE_H__ */
