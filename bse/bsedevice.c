/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
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
#include "bsedevice.h"

#include <unistd.h>
#include <errno.h>


/* --- prototypes --- */
static void	   bse_device_init			(BseDevice         *dev);
static void	   bse_device_class_init		(BseDeviceClass    *class);
static guint       bse_device_default_read	  	(BseDevice         *dev,
							 guint              n_bytes,
							 guint8            *bytes);
static guint       bse_device_default_write	 	(BseDevice         *dev,
							 guint              n_bytes,
							 guint8            *bytes);
static inline void bse_device_reset_device	  	(BseDevice         *dev);
static void	   bse_device_shutdown			(BseObject         *object);


/* --- variables --- */
static gpointer parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseDevice)
{
  static const GTypeInfo device_info = {
    sizeof (BseDeviceClass),
    
    (GBaseInitFunc) NULL,
    (GBaseDestroyFunc) NULL,
    (GClassInitFunc) bse_device_class_init,
    (GClassDestroyFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseDevice),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_device_init,
  };
  
  g_assert (BSE_DEVICE_FLAGS_USHIFT < BSE_OBJECT_FLAGS_MAX_SHIFT);
  
  return bse_type_register_static (BSE_TYPE_OBJECT,
				   "BseDevice",
				   "BSE device base type",
				   &device_info);
}

static void
bse_device_class_init (BseDeviceClass *class)
{
  BseObjectClass *object_class;
  
  parent_class = g_type_class_peek (BSE_TYPE_OBJECT);
  object_class = BSE_OBJECT_CLASS (class);
  
  object_class->shutdown = bse_device_shutdown;
  
  class->device_name = NULL;
  class->read = bse_device_default_read;
  class->write = bse_device_default_write;
  class->close = NULL;
}

static inline void
bse_device_reset_device (BseDevice *dev)
{
  dev->pfd.fd = -1;
  dev->pfd.events = 0;
  dev->pfd.revents = 0;
}

static void
bse_device_init (BseDevice *dev)
{
  dev->last_error = BSE_ERROR_NONE;
  bse_device_reset_device (dev);
}

static void
bse_device_shutdown (BseObject *object)
{
  BseDevice *dev = BSE_DEVICE (object);
  
  if (BSE_DEVICE_OPEN (dev))
    bse_device_close (dev);
  
  /* chain parent class' shutdown handler */
  BSE_OBJECT_CLASS (parent_class)->shutdown (object);
}

gchar*
bse_device_get_device_name (BseDevice *dev)
{
  gchar *name;

  g_return_val_if_fail (BSE_IS_DEVICE (dev), NULL);

  if (BSE_DEVICE_GET_CLASS (dev)->device_name)
    name = BSE_DEVICE_GET_CLASS (dev)->device_name (dev, FALSE);
  else
    name = BSE_OBJECT_TYPE_NAME (dev);

  return name ? name : "";
}

gchar*
bse_device_get_device_blurb (BseDevice *dev)
{
  gchar *name;

  g_return_val_if_fail (BSE_IS_DEVICE (dev), NULL);

  if (BSE_DEVICE_GET_CLASS (dev)->device_name)
    name = BSE_DEVICE_GET_CLASS (dev)->device_name (dev, TRUE);
  else
    name = BSE_OBJECT_TYPE_NAME (dev);

  return name ? name : "";
}

void
bse_device_close (BseDevice *dev)
{
  g_return_if_fail (BSE_IS_DEVICE (dev));
  g_return_if_fail (BSE_DEVICE_OPEN (dev));
  
  dev->last_error = BSE_ERROR_NONE;
  
  BSE_DEVICE_GET_CLASS (dev)->close (dev);
  
  bse_device_reset_device (dev);
  BSE_OBJECT_UNSET_FLAGS (dev,
			  BSE_DEVICE_FLAG_OPEN |
			  BSE_DEVICE_FLAG_READABLE |
			  BSE_DEVICE_FLAG_WRITABLE);

  errno = 0;
}

static guint
bse_device_default_read (BseDevice *dev,
			 guint      n_bytes,
			 guint8    *bytes)
{
  gint n;
  
  do
    n = read (dev->pfd.fd, bytes, n_bytes);
  while (n < 0 && errno == EINTR); /* don't mind signals */
  
  return MAX (n, 0);
}

static guint
bse_device_default_write (BseDevice *dev,
			  guint      n_bytes,
			  guint8    *bytes)
{
  gint n;
  
  do
    n = write (dev->pfd.fd, bytes, n_bytes);
  while (n < 0 && errno == EINTR); /* don't mind signals */
  
  return MAX (n, 0);
}
