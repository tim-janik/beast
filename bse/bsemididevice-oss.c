/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1996-2003 Tim Janik
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
#include	"bsemididevice-oss.h"

#include	"bseserver.h"
#include	"bsemididecoder.h"
#include	"gslcommon.h"

#include	"PKG_config.h"

#ifndef	BSE_MIDI_DEVICE_CONF_OSS
BSE_DUMMY_TYPE (BseMidiDeviceOSS);
#else   /* BSE_MIDI_DEVICE_CONF_OSS */

#include	<sys/soundcard.h>
#include	<sys/ioctl.h>
#include	<sys/types.h>
#include	<sys/time.h>
#include	<unistd.h>
#include	<errno.h>
#include	<fcntl.h>


/* --- structs --- */
typedef struct
{
  BseMidiHandle	handle;
  gint		fd;
} OSSHandle;


/* --- prototypes --- */
static void	    bse_midi_device_oss_class_init	(BseMidiDeviceOSSClass	*class);
static void	    bse_midi_device_oss_init		(BseMidiDeviceOSS	*midi_device_oss);
static void	    bse_midi_device_oss_finalize	(GObject		*object);
static BseErrorType bse_midi_device_oss_open		(BseMidiDevice		*mdev);
static void	    bse_midi_device_oss_close		(BseMidiDevice		*mdev);
static void	    io_handler				(BseMidiDevice		*mdev,
							 GPollFD		*pfd);


/* --- variables --- */
static gpointer parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseMidiDeviceOSS)
{
  GType midi_device_oss_type;
  
  static const GTypeInfo midi_device_oss_info = {
    sizeof (BseMidiDeviceOSSClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_midi_device_oss_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseMidiDeviceOSS),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_midi_device_oss_init,
  };
  
  midi_device_oss_type = bse_type_register_static (BSE_TYPE_MIDI_DEVICE,
						   "BseMidiDeviceOSS",
						   "MIDI device implementation for OSS Lite /dev/dsp",
						   &midi_device_oss_info);
  return midi_device_oss_type;
}

static void
bse_midi_device_oss_class_init (BseMidiDeviceOSSClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseMidiDeviceClass *midi_device_class = BSE_MIDI_DEVICE_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->finalize = bse_midi_device_oss_finalize;
  
  midi_device_class->driver_rating = BSE_RATING_DEFAULT;
  midi_device_class->open = bse_midi_device_oss_open;
  midi_device_class->suspend = bse_midi_device_oss_close;
}

static void
bse_midi_device_oss_init (BseMidiDeviceOSS *oss)
{
  oss->device_name = g_strdup (BSE_MIDI_DEVICE_CONF_OSS);
}

static BseErrorType
bse_midi_device_oss_open (BseMidiDevice *mdev)
{
  OSSHandle *oss = g_new0 (OSSHandle, 1);
  BseMidiHandle *handle = &oss->handle;
  BseErrorType error = BSE_ERROR_NONE;
  
  /* setup request */
  handle->writable = FALSE;
  handle->readable = TRUE;
  oss->fd = -1;
  
  /* try open */
  if (!error)
    {
      gint fd, extra_id = -1;
      BseErrorType first_error = 0;
      do
	{
	  gint omode = (handle->readable && handle->writable ? O_RDWR
			: handle->readable ? O_RDONLY
			: handle->writable ? O_WRONLY : 0);
	  gchar *dname;
	  if (extra_id >= 0)
	    dname = g_strdup_printf ("%s%u", BSE_MIDI_DEVICE_OSS (mdev)->device_name, extra_id);
	  else
	    dname = g_strdup (BSE_MIDI_DEVICE_OSS (mdev)->device_name);

	  /* need to open explicitely non-blocking or we'll have to wait untill someone else closes the device */
	  fd = open (dname, omode | O_NONBLOCK, 0);
	  if (fd >= 0)
	    {
	      oss->fd = fd;
	      error = 0;
	    }
	  else
	    {
	      // g_printerr ("open(\"%s\") failed: %s\n", dname, g_strerror (errno));
	      error = bse_error_from_errno (errno, BSE_ERROR_FILE_OPEN_FAILED);
	      if (!first_error)
		first_error = error;
	    }
	  g_free (dname);
	}
      while (error && extra_id++ < 3);
      error = error && first_error ? first_error : error;
    }

  /* setup mdev or shutdown */
  if (!error)
    {
      BSE_OBJECT_SET_FLAGS (mdev, BSE_MIDI_FLAG_OPEN);
      if (handle->readable)
	BSE_OBJECT_SET_FLAGS (mdev, BSE_MIDI_FLAG_READABLE);
      if (handle->writable)
	BSE_OBJECT_SET_FLAGS (mdev, BSE_MIDI_FLAG_WRITABLE);
      mdev->handle = handle;
      handle->midi_fd = oss->fd;
      bse_server_add_io_watch (bse_server_get (), handle->midi_fd, G_IO_IN, (BseIOWatch) io_handler, mdev);
    }
  else
    {
      if (oss->fd < 0)
	close (oss->fd);
      g_free (oss);
    }
  
  return error;
}

static void
bse_midi_device_oss_finalize (GObject *object)
{
  BseMidiDeviceOSS *mdev_oss = BSE_MIDI_DEVICE_OSS (object);
  
  g_free (mdev_oss->device_name);
  mdev_oss->device_name = NULL;
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bse_midi_device_oss_close (BseMidiDevice *mdev)
{
  OSSHandle *oss = (OSSHandle*) mdev->handle;
  BseMidiHandle *handle = &oss->handle;
  
  mdev->handle = NULL;
  
  g_assert (handle->running_thread == FALSE);
  /* midi_handle_abort_wait (handle); */
  
  bse_server_remove_io_watch (bse_server_get (), (BseIOWatch) io_handler, mdev);
  (void) close (oss->fd);
  g_free (oss);
}

static void
io_handler (BseMidiDevice *mdev,
	    GPollFD       *pfd)
{
  OSSHandle *oss = (OSSHandle*) mdev->handle;
  BseMidiHandle *handle = &oss->handle;
  const gsize buf_size = 8192;
  guint8 buffer[buf_size];
  guint64 systime;
  gssize l;
  
  /* this should spawn its own thread someday */
  g_assert (handle->running_thread == FALSE);
  
  systime = sfi_time_system ();
  do
    l = read (oss->fd, buffer, buf_size);
  while (l < 0 && errno == EINTR);	/* don't mind signals */
  
  if (l > 0)
    bse_midi_decoder_push_data (mdev->midi_decoder, l, buffer, systime);
}

#endif	/* BSE_MIDI_DEVICE_CONF_OSS */
