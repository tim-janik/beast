/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1999 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * bsecapture.h: BSE Recording output source
 */
#ifndef __BSE_CAPTURE_H__
#define __BSE_CAPTURE_H__

#include <bse/bsesource.h>
#include <bse/bsepcmdevice.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_CAPTURE              (BSE_TYPE_ID (BseCapture))
#define BSE_CAPTURE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_CAPTURE, BseCapture))
#define BSE_CAPTURE_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_CAPTURE, BseCaptureClass))
#define BSE_IS_CAPTURE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_CAPTURE))
#define BSE_IS_CAPTURE_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_CAPTURE))
#define BSE_CAPTURE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_CAPTURE, BseCaptureClass))


/* --- BseCapture source --- */
struct _BseCapture
{
  BseSource       parent_object;

  gchar		 *idevice;
  BsePcmDevice   *pdev;
};
struct _BseCaptureClass
{
  BseSourceClass parent_class;
};


/* --- channels --- */
enum
{
  BSE_CAPTURE_OCHANNEL_MONO
};





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_CAPTURE_H__ */
