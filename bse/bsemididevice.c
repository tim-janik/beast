/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1996-2002 Tim Janik
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

#include "bsemidireceiver.h"

#include <errno.h>

enum {
  PROP_0,
  PROP_MIDI_RECEIVER
};


/* --- prototypes --- */
static void	bse_midi_device_init		(BseMidiDevice		*self);
static void	bse_midi_device_class_init	(BseMidiDeviceClass	*class);
static void	bse_midi_device_set_property	(GObject                *object,
						 guint                   param_id,
						 const GValue           *value,
						 GParamSpec             *pspec);
static void	bse_midi_device_get_property	(GObject                *object,
						 guint                   param_id,
						 GValue                 *value,
						 GParamSpec             *pspec);
static void	bse_midi_device_dispose		(GObject		*object);


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
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_midi_device_set_property;
  gobject_class->get_property = bse_midi_device_get_property;
  gobject_class->dispose = bse_midi_device_dispose;
  
  class->driver_rating = 0;
  class->open = NULL;
  class->suspend = NULL;
  
  bse_object_class_add_param (object_class, NULL,
			      PROP_MIDI_RECEIVER,
			      g_param_spec_pointer ("midi_receiver", NULL, NULL, G_PARAM_READWRITE));
}

static void
bse_midi_device_init (BseMidiDevice *self)
{
  BSE_OBJECT_UNSET_FLAGS (self, (BSE_MIDI_FLAG_OPEN |
				 BSE_MIDI_FLAG_READABLE |
				 BSE_MIDI_FLAG_WRITABLE));
  self->midi_receiver = NULL;
  self->handle = NULL;
}

static void
bse_midi_device_set_property (GObject                *object,
			      guint                   param_id,
			      const GValue           *value,
			      GParamSpec             *pspec)
{
  BseMidiDevice *self = BSE_MIDI_DEVICE (object);
  
  switch (param_id)
    {
    case PROP_MIDI_RECEIVER:
      if (self->midi_receiver)
	bse_midi_receiver_unref (self->midi_receiver);
      self->midi_receiver = g_value_get_pointer (value);
      if (self->midi_receiver)
	bse_midi_receiver_ref (self->midi_receiver);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_midi_device_get_property (GObject                *object,
			      guint                   param_id,
			      GValue                 *value,
			      GParamSpec             *pspec)
{
  BseMidiDevice *self = BSE_MIDI_DEVICE (object);
  
  switch (param_id)
    {
    case PROP_MIDI_RECEIVER:
      g_value_set_pointer (value, self->midi_receiver);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_midi_device_dispose (GObject *object)
{
  BseMidiDevice *self = BSE_MIDI_DEVICE (object);
  
  if (BSE_MIDI_DEVICE_OPEN (self))
    {
      g_warning ("%s: midi device still opened", G_STRLOC);
      bse_midi_device_suspend (self);
    }
  if (self->handle)
    g_warning (G_STRLOC ": midi device with stale midi handle");
  if (self->midi_receiver)
    bse_midi_receiver_unref (self->midi_receiver);
  self->midi_receiver = NULL;
  
  /* chain parent class' destroy handler */
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

BseErrorType
bse_midi_device_open (BseMidiDevice  *mdev)
{
  BseErrorType error;
  
  g_return_val_if_fail (BSE_IS_MIDI_DEVICE (mdev), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (!BSE_MIDI_DEVICE_OPEN (mdev), BSE_ERROR_INTERNAL);
  
  error = BSE_MIDI_DEVICE_GET_CLASS (mdev)->open (mdev);
  
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
