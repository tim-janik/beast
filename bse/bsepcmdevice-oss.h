/* BSE - Better Sound Engine
 * Copyright (C) 1996-1999, 2000-2002 Tim Janik
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
 *
 * bsepcmdevice-oss.h: pcm device implementation for OSS Lite driver
 */
#ifndef	__BSE_PCM_DEVICE_OSS_H__
#define	__BSE_PCM_DEVICE_OSS_H__

#include	<bse/bsepcmdevice.h>


G_BEGIN_DECLS


/* --- object type macros --- */
#define BSE_TYPE_PCM_DEVICE_OSS		     (BSE_TYPE_ID (BsePcmDeviceOSS))
#define BSE_PCM_DEVICE_OSS(object)	     (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_PCM_DEVICE_OSS, BsePcmDeviceOSS))
#define BSE_PCM_DEVICE_OSS_CLASS(class)	     (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_PCM_DEVICE_OSS, BsePcmDeviceOSSClass))
#define BSE_IS_PCM_DEVICE_OSS(object)	     (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_PCM_DEVICE_OSS))
#define BSE_IS_PCM_DEVICE_OSS_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PCM_DEVICE_OSS))
#define BSE_PCM_DEVICE_OSS_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_PCM_DEVICE_OSS, BsePcmDeviceOSSClass))


/* --- BsePcmDeviceOSS object --- */
typedef	struct _BsePcmDeviceOSS	     BsePcmDeviceOSS;
typedef	struct _BsePcmDeviceOSSClass BsePcmDeviceOSSClass;
struct _BsePcmDeviceOSS
{
  BsePcmDevice parent_object;

  gchar       *device_name;
};
struct _BsePcmDeviceOSSClass
{
  BsePcmDeviceClass parent_class;
};



G_END_DECLS

#endif /* __BSE_PCM_DEVICE_OSS_H__ */
