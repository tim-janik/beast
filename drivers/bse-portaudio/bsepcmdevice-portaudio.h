/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2004 Tim Janik
 * Copyright (C) 2004 Stefan Westerfeld
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
#ifndef __BSE_PCM_DEVICE_PORTAUDIO_H__
#define __BSE_PCM_DEVICE_PORTAUDIO_H__

#include <bse/bsepcmdevice.h>
#include <bse/bseplugin.h>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_PCM_DEVICE_PORT_AUDIO              (BSE_EXPORT_TYPE_ID (BsePcmDevicePortAudio))
#define BSE_PCM_DEVICE_PORT_AUDIO(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_PCM_DEVICE_PORT_AUDIO, BsePcmDevicePortAudio))
#define BSE_PCM_DEVICE_PORT_AUDIO_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_PCM_DEVICE_PORT_AUDIO, BsePcmDevicePortAudioClass))
#define BSE_IS_PCM_DEVICE_PORT_AUDIO(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_PCM_DEVICE_PORT_AUDIO))
#define BSE_IS_PCM_DEVICE_PORT_AUDIO_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PCM_DEVICE_PORT_AUDIO))
#define BSE_PCM_DEVICE_PORT_AUDIO_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_PCM_DEVICE_PORT_AUDIO, BsePcmDevicePortAudioClass))

/* --- BsePcmDevicePortAudio object --- */
typedef struct _BsePcmDevicePortAudio             BsePcmDevicePortAudio;
typedef struct _BsePcmDevicePortAudioClass	  BsePcmDevicePortAudioClass;
struct _BsePcmDevicePortAudio
{
  BsePcmDevice parent_object;
};
struct _BsePcmDevicePortAudioClass
{
  BsePcmDeviceClass parent_class;
};

G_END_DECLS

#endif /* __BSE_PCM_DEVICE_PORTAUDIO_H__ */
