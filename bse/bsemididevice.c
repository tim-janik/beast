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
 */
#include "bsemididevice.h"

#include <errno.h>


/* --- prototypes --- */
static void	   bse_midi_device_init			(BseMidiDevice      *mdev);
static void	   bse_midi_device_class_init		(BseMidiDeviceClass *class);
static void	   bse_midi_device_destroy		(BseObject         *object);


/* --- variables --- */
static gpointer parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseMidiDevice)
{
  static const GTypeInfo midi_device_info = {
    sizeof (BseMidiDeviceClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_midi_device_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseMidiDevice),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_midi_device_init,
  };
  
  g_assert (BSE_MIDI_FLAGS_USHIFT < BSE_OBJECT_FLAGS_MAX_SHIFT);
  
  return bse_type_register_static (BSE_TYPE_OBJECT,
				   "BseMidiDevice",
				   "MIDI device base type",
				   &midi_device_info);
}

static void
bse_midi_device_class_init (BseMidiDeviceClass *class)
{
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  object_class->destroy = bse_midi_device_destroy;

  class->driver_rating = 0;
  class->open = NULL;
  class->suspend = NULL;
  class->trigger = NULL;
}

static void
bse_midi_device_init (BseMidiDevice *mdev)
{
  BSE_OBJECT_UNSET_FLAGS (mdev, (BSE_MIDI_FLAG_OPEN |
				 BSE_MIDI_FLAG_READABLE |
				 BSE_MIDI_FLAG_WRITABLE));
  mdev->handle = NULL;
}

static void
bse_midi_device_destroy (BseObject *object)
{
  BseMidiDevice *mdev = BSE_MIDI_DEVICE (object);

  if (BSE_MIDI_DEVICE_OPEN (mdev))
    {
      g_warning (G_STRLOC ": midi device still opened");
      bse_midi_device_suspend (mdev);
    }
  if (mdev->handle)
      g_warning (G_STRLOC ": midi device with stale midi handle");

  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

BseErrorType
bse_midi_device_open (BseMidiDevice  *mdev,
		      BseMidiDecoder *decoder)
{
  BseErrorType error;

  g_return_val_if_fail (BSE_IS_MIDI_DEVICE (mdev), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (!BSE_MIDI_DEVICE_OPEN (mdev), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (decoder != NULL, BSE_ERROR_INTERNAL);

  error = BSE_MIDI_DEVICE_GET_CLASS (mdev)->open (mdev, decoder);

  if (!error)
    g_return_val_if_fail (BSE_MIDI_DEVICE_OPEN (mdev) && mdev->handle, BSE_ERROR_INTERNAL);
  else
    g_return_val_if_fail (!BSE_MIDI_DEVICE_OPEN (mdev), BSE_ERROR_INTERNAL);

  errno = 0;
    
  return error;
}

void
bse_midi_device_suspend (BseMidiDevice *mdev)
{
  g_return_if_fail (BSE_IS_MIDI_DEVICE (mdev));
  g_return_if_fail (BSE_MIDI_DEVICE_OPEN (mdev));
  
  BSE_MIDI_DEVICE_GET_CLASS (mdev)->suspend (mdev);

  BSE_OBJECT_UNSET_FLAGS (mdev, (BSE_MIDI_FLAG_OPEN |
				 BSE_MIDI_FLAG_READABLE |
				 BSE_MIDI_FLAG_WRITABLE));
  errno = 0;
}

void
bse_midi_device_trigger (BseMidiDevice *mdev)
{
  g_return_if_fail (BSE_IS_MIDI_DEVICE (mdev));

  if (BSE_MIDI_DEVICE_OPEN (mdev))
    BSE_MIDI_DEVICE_GET_CLASS (mdev)->trigger (mdev);
}


#if 0  // FIXME remove code:
/* --- decoder / utils --- */
struct _BseMidiDecoder
{
  BseMidiEvent    *events;
  BseMidiEventType status;
  BseMidiEventType last_status;
  guint		   channel_id;
  guint            n_bytes;
  guint8	  *bytes;
  guint		   left_bytes;
};

void
bse_midi_handle_init (BseMidiHandle *handle)
{
  g_return_if_fail (handle != NULL);
  g_return_if_fail (handle->decoder == NULL);
  g_return_if_fail (handle->channels == NULL);

  handle->decoder = g_new0 (BseMidiDecoder, 1);
  // memset (handle->channels, 0, sizeof (handle->channels));
}

static void
read_data (BseMidiDecoder *decoder,
	   guint          *n_bytes_p,
	   guint8        **bytes_p)
{
  guint n_bytes = *n_bytes_p;
  guint8 *bytes = *bytes_p;

  g_return_if_fail (decoder->status & 0x80);

  if (decoder->status != BSE_MIDI_SYS_EX)
    {
      guint i = MIN (decoder->left_bytes, n_bytes);
      guint n = decoder->n_bytes;
      
      decoder->n_bytes += i;
      decoder->bytes = g_renew (guint8, decoder->bytes, decoder->n_bytes);
      memcpy (decoder->bytes + n, bytes, i);
      decoder->left_bytes -= i;
      *n_bytes_p -= i;
      *bytes_p += i;
    }
  else /* search BSE_MIDI_END_EX */
    {
      guint i;

      /* search for end mark */
      for (i = 0; i < n_bytes; i++)
	if (bytes[i] == BSE_MIDI_END_EX)
	  break;
      /* append data bytes */
      if (i)
	{
	  guint n = decoder->n_bytes;

	  decoder->n_bytes += i - 1;
	  decoder->bytes = g_renew (guint8, decoder->bytes, decoder->n_bytes + 1);
	  memcpy (decoder->bytes + n, bytes, i - 1);
	}
      *n_bytes_p -= i;
      *bytes_p += i;
      /* did we find end mark? */
      if (i < n_bytes)
	{
	  decoder->status = BSE_MIDI_END_EX;
	  decoder->left_bytes = 0;
	  read_data (decoder, n_bytes_p, bytes_p);
	}
    }
}
#endif
