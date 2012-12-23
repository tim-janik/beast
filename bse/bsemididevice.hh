/* BSE - Better Sound Engine
 * Copyright (C) 1996-2003 Tim Janik
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
 * bsemididevice.hh: midi (musical instruments digital interface) device driver
 */
#ifndef __BSE_MIDI_DEVICE_H__
#define __BSE_MIDI_DEVICE_H__

#include        <bse/bsedevice.hh>
#include        <bse/bsemidievent.hh>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_MIDI_DEVICE              (BSE_TYPE_ID (BseMidiDevice))
#define BSE_MIDI_DEVICE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_MIDI_DEVICE, BseMidiDevice))
#define BSE_MIDI_DEVICE_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_MIDI_DEVICE, BseMidiDeviceClass))
#define BSE_IS_MIDI_DEVICE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_MIDI_DEVICE))
#define BSE_IS_MIDI_DEVICE_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_MIDI_DEVICE))
#define BSE_MIDI_DEVICE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_MIDI_DEVICE, BseMidiDeviceClass))


/* --- BseMidiDevice structs --- */
typedef struct _BseMidiHandle		BseMidiHandle;
typedef struct _BseMidiDevice		BseMidiDevice;
typedef struct _BseMidiDeviceClass	BseMidiDeviceClass;
struct _BseMidiHandle	/* this should be nuked, it's useless */
{
  guint			 readable : 1;
  guint			 writable : 1;
  guint			 running_thread : 1;
  BseMidiDecoder	*midi_decoder;
};
struct _BseMidiDevice
{
  BseDevice              parent_object;

  BseMidiDecoder	*midi_decoder;

  /* operational handle */
  BseMidiHandle	        *handle;
};
struct _BseMidiDeviceClass
{
  BseDeviceClass	parent_class;
};


/* --- internal utils --- */
void		bse_midi_handle_init		(BseMidiHandle		*handle);
     

G_END_DECLS

#endif /* __BSE_MIDI_DEVICE_H__ */
