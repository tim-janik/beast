/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2000-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * bsemididevice-oss.h: midi device implementation for OSS Lite driver
 */
#ifndef	__BSE_MIDI_DEVICE_NULL_H__
#define	__BSE_MIDI_DEVICE_NULL_H__

#include	<bse/bsemididevice.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


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


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_MIDI_DEVICE_NULL_H__ */
