/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2000-2004 Tim Janik
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


/* --- functions --- */
static void
bse_midi_device_null_init (BseMidiDeviceNULL *null)
{
}

static SfiRing*
bse_midi_device_null_list_devices (BseDevice *device)
{
  SfiRing *ring = NULL;
  ring = sfi_ring_append (ring, bse_device_entry_new (device, g_strdup_printf ("default"), NULL));
  return ring;
}

static BseErrorType
bse_midi_device_null_open (BseDevice     *device,
                           gboolean       require_readable,
                           gboolean       require_writable,
                           guint          n_args,
                           const gchar  **args)
{
  NULLHandle *null = g_new0 (NULLHandle, 1);
  BseMidiHandle *handle = &null->handle;
  
  /* setup request */
  handle->readable = require_readable;
  handle->writable = require_writable;
  
  BSE_OBJECT_SET_FLAGS (device, BSE_DEVICE_FLAG_OPEN);
  if (handle->readable)
    BSE_OBJECT_SET_FLAGS (device, BSE_DEVICE_FLAG_READABLE);
  if (handle->writable)
    BSE_OBJECT_SET_FLAGS (device, BSE_DEVICE_FLAG_WRITABLE);
  BSE_MIDI_DEVICE (device)->handle = handle;
  
  return BSE_ERROR_NONE;
}

static void
bse_midi_device_null_close (BseDevice *device)
{
  NULLHandle *null = (NULLHandle*) BSE_MIDI_DEVICE (device)->handle;
  BseMidiHandle *handle = &null->handle;
  BSE_MIDI_DEVICE (device)->handle = NULL;
  
  g_assert (handle->running_thread == FALSE);
  /* midi_handle_abort_wait (handle); */
  
  g_free (null);
}

static void
bse_midi_device_null_class_init (BseMidiDeviceNULLClass *class)
{
  BseDeviceClass *device_class = BSE_DEVICE_CLASS (class);
  
  device_class->list_devices = bse_midi_device_null_list_devices;
  bse_device_class_setup (class,
                          -1,
                          "null", NULL,
                          /* TRANSLATORS: keep this text to 70 chars in width */
                          _("Discards all output events and generates no input events. This driver\n"
                            "is not part of the automatic device selection list for MIDI devices."));
  device_class->open = bse_midi_device_null_open;
  device_class->close = bse_midi_device_null_close;
}

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
