/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2003 Tim Janik
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
 */
#include "bsepcmwriter.h"
#include "bseserver.h"
#include "gsldatautils.h"

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


/* --- prototypes --- */
static void	   bse_pcm_writer_init			(BsePcmWriter      *pdev);
static void	   bse_pcm_writer_class_init		(BsePcmWriterClass *class);
static void	   bse_pcm_writer_finalize		(GObject           *object);


/* --- variables --- */
static gpointer parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BsePcmWriter)
{
  static const GTypeInfo pcm_writer_info = {
    sizeof (BsePcmWriterClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_pcm_writer_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BsePcmWriter),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_pcm_writer_init,
  };
  
  return bse_type_register_static (BSE_TYPE_ITEM,
				   "BsePcmWriter",
				   "PCM writer",
                                   __FILE__, __LINE__,
                                   &pcm_writer_info);
}

static void
bse_pcm_writer_class_init (BsePcmWriterClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->finalize = bse_pcm_writer_finalize;
}

static void
bse_pcm_writer_init (BsePcmWriter *self)
{
  sfi_mutex_init (&self->mutex);
}

static void
bse_pcm_writer_finalize (GObject *object)
{
  BsePcmWriter *self = BSE_PCM_WRITER (object);
  
  if (self->open)
    {
      g_warning ("%s: pcm writer still opened", G_STRLOC);
      bse_pcm_writer_close (self);
    }

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
  sfi_mutex_destroy (&self->mutex);
}

BseErrorType
bse_pcm_writer_open (BsePcmWriter *self,
		     const gchar  *file,
		     guint         n_channels,
		     guint         sample_freq,
                     uint64        recorded_maximum)
{
  BseErrorType error;
  gint fd;

  g_return_val_if_fail (BSE_IS_PCM_WRITER (self), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (!self->open, BSE_ERROR_INTERNAL);
  g_return_val_if_fail (file != NULL, BSE_ERROR_INTERNAL);
  g_return_val_if_fail (n_channels > 0, BSE_ERROR_INTERNAL);
  g_return_val_if_fail (sample_freq >= 1000, BSE_ERROR_INTERNAL);

  sfi_mutex_lock (&self->mutex);

  error = 0;

  self->n_bytes = 0;
  self->recorded_maximum = recorded_maximum;
  fd = open (file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd < 0)
    {
      sfi_mutex_unlock (&self->mutex);
      return bse_error_from_errno (errno, BSE_ERROR_FILE_OPEN_FAILED);
    }

  errno = bse_wave_file_dump_header (fd, 0x7fff0000, 16, n_channels, sample_freq);
  if (errno)
    {
      close (fd);
      sfi_mutex_unlock (&self->mutex);
      return bse_error_from_errno (errno, BSE_ERROR_FILE_OPEN_FAILED);
    }
  self->fd = fd;
  self->open = TRUE;
  self->broken = FALSE;

  sfi_mutex_unlock (&self->mutex);
  return BSE_ERROR_NONE;
}

void
bse_pcm_writer_close (BsePcmWriter *self)
{
  g_return_if_fail (BSE_IS_PCM_WRITER (self));
  g_return_if_fail (self->open);

  sfi_mutex_lock (&self->mutex);
  bse_wave_file_patch_length (self->fd, self->n_bytes);
  close (self->fd);
  self->fd = -1;
  self->open = FALSE;
  sfi_mutex_unlock (&self->mutex);
  errno = 0;
}

static gboolean
bsethread_halt_recording (gpointer data)
{
  bse_server_stop_recording (bse_server_get());
  return false;
}

void
bse_pcm_writer_write (BsePcmWriter *self,
		      gsize         n_values,
		      const gfloat *values)
{
  g_return_if_fail (BSE_IS_PCM_WRITER (self));
  g_return_if_fail (self->open);
  if (n_values)
    g_return_if_fail (values != NULL);
  else
    return;

  sfi_mutex_lock (&self->mutex);
  const uint bw = 2; /* 16bit */
  if (!self->broken && (!self->recorded_maximum || self->n_bytes < bw * self->recorded_maximum))
    {
      guint j;
      guint8 *dest = g_new (guint8, n_values * bw);
      uint n_bytes = gsl_conv_from_float_clip (GSL_WAVE_FORMAT_SIGNED_16,
                                               G_BYTE_ORDER,
                                               values,
                                               dest,
                                               n_values);
      if (self->recorded_maximum)
        n_bytes = bw * MIN (n_bytes / bw, self->recorded_maximum - self->n_bytes / bw);
      do
	j = write (self->fd, dest, n_bytes);
      while (j < 0 && errno == EINTR);
      if (j > 0)
        {
          self->n_bytes += j;
          if (self->recorded_maximum && self->n_bytes >= bw * self->recorded_maximum)
            bse_idle_next (bsethread_halt_recording, NULL);
        }
      g_free (dest);
      if (j < 0 && errno)
	{
 	  sfi_diag ("failed to write %u bytes to WAV file: %s", n_bytes, g_strerror (errno));
	  self->broken = TRUE;
	}
    }
  sfi_mutex_unlock (&self->mutex);
}
