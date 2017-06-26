/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2004 Tim Janik
 * Copyright (C) 2006 Stefan Westerfeld
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __BSE_PCM_DEVICE_JACK_H__
#define __BSE_PCM_DEVICE_JACK_H__

#include <bse/bsepcmdevice.h>
#include <bse/bseplugin.h>
#include <jack/jack.h>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_PCM_DEVICE_JACK              (BSE_EXPORT_TYPE_ID (BsePcmDeviceJACK))
#define BSE_PCM_DEVICE_JACK(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_PCM_DEVICE_JACK, BsePcmDeviceJACK))
#define BSE_PCM_DEVICE_JACK_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_PCM_DEVICE_JACK, BsePcmDeviceJACKClass))
#define BSE_IS_PCM_DEVICE_JACK(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_PCM_DEVICE_JACK))
#define BSE_IS_PCM_DEVICE_JACK_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PCM_DEVICE_JACK))
#define BSE_PCM_DEVICE_JACK_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_PCM_DEVICE_JACK, BsePcmDeviceJACKClass))

/* --- BsePcmDeviceJACK object --- */
typedef struct _BsePcmDeviceJACK             BsePcmDeviceJACK;
typedef struct _BsePcmDeviceJACKClass	     BsePcmDeviceJACKClass;
struct _BsePcmDeviceJACK
{
  BsePcmDevice    parent_object;
  jack_client_t  *jack_client;
};
struct _BsePcmDeviceJACKClass
{
  BsePcmDeviceClass parent_class;
};

G_END_DECLS

#endif /* __BSE_PCM_DEVICE_JACK_H__ */
