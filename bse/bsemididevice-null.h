/* BSE - Better Sound Engine
 * Copyright (C) 2000-2004 Tim Janik
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
 * bsemididevice-oss.h: midi device implementation for OSS Lite driver
 */
#ifndef	__BSE_MIDI_DEVICE_NULL_H__
#define	__BSE_MIDI_DEVICE_NULL_H__

#include	<bse/bsemididevice.h>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_MIDI_DEVICE_NULL		(BSE_TYPE_ID (BseMidiDeviceNULL))
#define BSE_MIDI_DEVICE_NULL(object)		(G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_MIDI_DEVICE_NULL, BseMidiDeviceNULL))
#define BSE_MIDI_DEVICE_NULL_CLASS(class)	(G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_MIDI_DEVICE_NULL, BseMidiDeviceNULLClass))
#define BSE_IS_MIDI_DEVICE_NULL(object)		(G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_MIDI_DEVICE_NULL))
#define BSE_IS_MIDI_DEVICE_NULL_CLASS(class)	(G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_MIDI_DEVICE_NULL))
#define BSE_MIDI_DEVICE_NULL_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_MIDI_DEVICE_NULL, BseMidiDeviceNULLClass))

/* --- BseMidiDeviceNULL object --- */
typedef	struct _BseMidiDeviceNULL      BseMidiDeviceNULL;
typedef	struct _BseMidiDeviceNULLClass BseMidiDeviceNULLClass;
struct _BseMidiDeviceNULL
{
  BseMidiDevice parent_object;
};
struct _BseMidiDeviceNULLClass
{
  BseMidiDeviceClass parent_class;
};

G_END_DECLS

#endif /* __BSE_MIDI_DEVICE_NULL_H__ */
