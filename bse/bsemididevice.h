/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1996-1999, 2000-2001 Tim Janik
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
 * bsemididevice.h: midi (musical instruments digital interface) device driver
 */
#ifndef __BSE_MIDI_DEVICE_H__
#define __BSE_MIDI_DEVICE_H__

#include        <bse/bseobject.h>
#include        <bse/bsemidievent.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_MIDI_DEVICE              (BSE_TYPE_ID (BseMidiDevice))
#define BSE_MIDI_DEVICE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_MIDI_DEVICE, BseMidiDevice))
#define BSE_MIDI_DEVICE_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_MIDI_DEVICE, BseMidiDeviceClass))
#define BSE_IS_MIDI_DEVICE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_MIDI_DEVICE))
#define BSE_IS_MIDI_DEVICE_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_MIDI_DEVICE))
#define BSE_MIDI_DEVICE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_MIDI_DEVICE, BseMidiDeviceClass))
/* flag tests */
#define BSE_MIDI_DEVICE_OPEN(pdev)	 ((BSE_OBJECT_FLAGS (pdev) & BSE_MIDI_FLAG_OPEN) != 0)
#define BSE_MIDI_DEVICE_READABLE(pdev)	 ((BSE_OBJECT_FLAGS (pdev) & BSE_MIDI_FLAG_READABLE) != 0)
#define BSE_MIDI_DEVICE_WRITABLE(pdev)	 ((BSE_OBJECT_FLAGS (pdev) & BSE_MIDI_FLAG_WRITABLE) != 0)


/* --- enums --- */
typedef enum
{
  BSE_MIDI_FLAG_OPEN		= 1 << (BSE_OBJECT_FLAGS_USHIFT + 0),
  BSE_MIDI_FLAG_READABLE	= 1 << (BSE_OBJECT_FLAGS_USHIFT + 1),
  BSE_MIDI_FLAG_WRITABLE	= 1 << (BSE_OBJECT_FLAGS_USHIFT + 2)
} BseMidiFlags;
#define	BSE_MIDI_FLAGS_USHIFT	(BSE_OBJECT_FLAGS_USHIFT + 3)


/* --- BseMidiDevice structs --- */
typedef struct _BseMidiHandle		BseMidiHandle;
typedef struct _BseMidiDevice		BseMidiDevice;
typedef struct _BseMidiDeviceClass	BseMidiDeviceClass;
struct _BseMidiHandle
{
  gint			 midi_fd;
  guint			 writable : 1;
  guint			 readable : 1;
  guint			 running_thread : 1;
  BseMidiDecoder	*decoder;
};
struct _BseMidiDevice
{
  BseObject		parent_object;

  /* operational handle */
  BseMidiHandle	       *handle;
};
struct _BseMidiDeviceClass
{
  BseObjectClass	parent_class;

  guint			driver_rating;
  BseErrorType	(*open)		(BseMidiDevice	*mdev,
				 BseMidiDecoder	*decoder);
  void		(*suspend)	(BseMidiDevice	*mdev);
  void		(*trigger)	(BseMidiDevice	*mhandle);
};


/* --- prototypes --- */
BseErrorType	bse_midi_device_open		(BseMidiDevice		*mdev,
						 BseMidiDecoder		*decoder);
void		bse_midi_device_suspend		(BseMidiDevice		*mdev);
void		bse_midi_device_trigger		(BseMidiDevice		*mdev);


/* --- internal utils --- */
void		bse_midi_handle_init		(BseMidiHandle		*handle);
     

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_MIDI_DEVICE_H__ */
