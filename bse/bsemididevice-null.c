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
 */
#include	"bsemididevice-null.h"

#include	"bseserver.h"


/* --- structs --- */
typedef struct
{
  BseMidiHandle	handle;
} NULLHandle;


/* --- prototypes --- */
static void	    bse_midi_device_null_class_init	(BseMidiDeviceNULLClass	*class);
static void	    bse_midi_device_null_init		(BseMidiDeviceNULL	*midi_device_null);
static BseErrorType bse_midi_device_null_open		(BseMidiDevice		*mdev);
static void	    bse_midi_device_null_close		(BseMidiDevice		*mdev);


/* --- variables --- */
static gpointer parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseMidiDeviceNULL)
{
  GType type;
  
  static const GTypeInfo type_info = {
    sizeof (BseMidiDeviceNULLClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_midi_device_null_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseMidiDeviceNULL),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_midi_device_null_init,
  };
  
  type = bse_type_register_static (BSE_TYPE_MIDI_DEVICE,
				   "BseMidiDeviceNULL",
				   "MIDI device implementation that does nothing",
				   &type_info);
  return type;
}

static void
bse_midi_device_null_class_init (BseMidiDeviceNULLClass *class)
{
  /* BseObjectClass *object_class = BSE_OBJECT_CLASS (class); */
  BseMidiDeviceClass *midi_device_class = BSE_MIDI_DEVICE_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  midi_device_class->driver_rating = BSE_RATING_FALLBACK;
  midi_device_class->open = bse_midi_device_null_open;
  midi_device_class->suspend = bse_midi_device_null_close;
}

static void
bse_midi_device_null_init (BseMidiDeviceNULL *null)
{
}

static BseErrorType
bse_midi_device_null_open (BseMidiDevice *mdev)
{
  NULLHandle *null = g_new0 (NULLHandle, 1);
  BseMidiHandle *handle = &null->handle;
  
  /* setup request */
  handle->writable = FALSE;
  handle->readable = TRUE;
  
  BSE_OBJECT_SET_FLAGS (mdev, BSE_MIDI_FLAG_OPEN);
  if (handle->readable)
    BSE_OBJECT_SET_FLAGS (mdev, BSE_MIDI_FLAG_READABLE);
  if (handle->writable)
    BSE_OBJECT_SET_FLAGS (mdev, BSE_MIDI_FLAG_WRITABLE);
  mdev->handle = handle;
  handle->midi_fd = -1;
  
  return BSE_ERROR_NONE;
}

static void
bse_midi_device_null_close (BseMidiDevice *mdev)
{
  NULLHandle *null = (NULLHandle*) mdev->handle;
  BseMidiHandle *handle = &null->handle;
  
  mdev->handle = NULL;
  
  g_assert (handle->running_thread == FALSE);
  /* midi_handle_abort_wait (handle); */
  
  g_free (null);
}
