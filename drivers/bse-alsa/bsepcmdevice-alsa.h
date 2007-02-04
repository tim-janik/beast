/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2004 Tim Janik
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
#ifndef __BSE_PCM_DEVICE_ALSA_H__
#define __BSE_PCM_DEVICE_ALSA_H__

#include <bse/bsepcmdevice.h>
#include <bse/bseplugin.h>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_PCM_DEVICE_ALSA              (BSE_EXPORT_TYPE_ID (BsePcmDeviceALSA))
#define BSE_PCM_DEVICE_ALSA(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_PCM_DEVICE_ALSA, BsePcmDeviceALSA))
#define BSE_PCM_DEVICE_ALSA_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_PCM_DEVICE_ALSA, BsePcmDeviceALSAClass))
#define BSE_IS_PCM_DEVICE_ALSA(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_PCM_DEVICE_ALSA))
#define BSE_IS_PCM_DEVICE_ALSA_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PCM_DEVICE_ALSA))
#define BSE_PCM_DEVICE_ALSA_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_PCM_DEVICE_ALSA, BsePcmDeviceALSAClass))

/* --- BsePcmDeviceALSA object --- */
typedef struct _BsePcmDeviceALSA             BsePcmDeviceALSA;
typedef struct _BsePcmDeviceALSAClass BsePcmDeviceALSAClass;
struct _BsePcmDeviceALSA
{
  BsePcmDevice parent_object;
};
struct _BsePcmDeviceALSAClass
{
  BsePcmDeviceClass parent_class;
};

G_END_DECLS

#endif /* __BSE_PCM_DEVICE_ALSA_H__ */
