/* CSL Support for BSE
 * Copyright (C) 2003 Stefan Westerfeld
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
 * bsepcmdevice-csl.h: pcm device support using the Common Sound Layer
 */
#ifndef __BSE_PCM_DEVICE_CSL_H__
#define __BSE_PCM_DEVICE_CSL_H__

#include <bse/bsepcmdevice.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* --- object type macros --- */
#define BSE_TYPE_PCM_DEVICE_CSL              (BSE_TYPE_ID (BsePcmDeviceCSL))
#define BSE_PCM_DEVICE_CSL(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_PCM_DEVICE_CSL, BsePcmDeviceCSL))
#define BSE_PCM_DEVICE_CSL_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_PCM_DEVICE_CSL, BsePcmDeviceCSLClass))
#define BSE_IS_PCM_DEVICE_CSL(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_PCM_DEVICE_CSL))
#define BSE_IS_PCM_DEVICE_CSL_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PCM_DEVICE_CSL))
#define BSE_PCM_DEVICE_CSL_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_PCM_DEVICE_CSL, BsePcmDeviceCSLClass))

/* --- BsePcmDeviceCSL object --- */
typedef struct _BsePcmDeviceCSL      BsePcmDeviceCSL;
typedef struct _BsePcmDeviceCSLClass BsePcmDeviceCSLClass;
struct _BsePcmDeviceCSL
{
  BsePcmDevice parent_object;

  gchar       *device_name;
};
struct _BsePcmDeviceCSLClass
{
  BsePcmDeviceClass parent_class;
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_PCM_DEVICE_CSL_H__ */
