/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2004 Tim Janik
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
#ifndef __BSE_MIDI_DEVICE_ALSA_H__
#define __BSE_MIDI_DEVICE_ALSA_H__

#include <bse/bsemididevice.h>
#include <bse/bseplugin.h>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_MIDI_DEVICE_ALSA              (BSE_EXPORT_TYPE_ID (BseMidiDeviceALSA))
#define BSE_MIDI_DEVICE_ALSA(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_MIDI_DEVICE_ALSA, BseMidiDeviceALSA))
#define BSE_MIDI_DEVICE_ALSA_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_MIDI_DEVICE_ALSA, BseMidiDeviceALSAClass))
#define BSE_IS_MIDI_DEVICE_ALSA(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_MIDI_DEVICE_ALSA))
#define BSE_IS_MIDI_DEVICE_ALSA_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_MIDI_DEVICE_ALSA))
#define BSE_MIDI_DEVICE_ALSA_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_MIDI_DEVICE_ALSA, BseMidiDeviceALSAClass))

/* --- BseMidiDeviceALSA object --- */
typedef struct _BseMidiDeviceALSA             BseMidiDeviceALSA;
typedef struct _BseMidiDeviceALSAClass BseMidiDeviceALSAClass;
struct _BseMidiDeviceALSA
{
  BseMidiDevice parent_object;
};
struct _BseMidiDeviceALSAClass
{
  BseMidiDeviceClass parent_class;
};

G_END_DECLS

#endif /* __BSE_MIDI_DEVICE_ALSA_H__ */
