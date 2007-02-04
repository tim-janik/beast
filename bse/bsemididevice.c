/* BSE - Bedevilled Sound Engine
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
 */
#include "bsemididevice.h"

#include "bsemididecoder.h"

#include <errno.h>


/* --- variables --- */
static gpointer parent_class = NULL;


/* --- functions --- */
static void
bse_midi_device_init (BseMidiDevice *self)
{
  self->midi_decoder = bse_midi_decoder_new (TRUE, FALSE, BSE_MUSICAL_TUNING_12_TET);
  self->handle = NULL;
}

static void
bse_midi_device_dispose (GObject *object)
{
  BseMidiDevice *self = BSE_MIDI_DEVICE (object);
  
  if (BSE_DEVICE_OPEN (self))
    {
      g_warning ("%s: midi device still opened", G_STRLOC);
      bse_device_close (BSE_DEVICE (self));
    }
  if (self->handle)
    g_warning (G_STRLOC ": midi device with stale midi handle");
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
bse_midi_device_finalize (GObject *object)
{
  BseMidiDevice *self = BSE_MIDI_DEVICE (object);
  
  bse_midi_decoder_destroy (self->midi_decoder);
  self->midi_decoder = NULL;
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bse_midi_device_class_init (BseMidiDeviceClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->dispose = bse_midi_device_dispose;
  gobject_class->finalize = bse_midi_device_finalize;
}

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
  
  return bse_type_register_abstract (BSE_TYPE_DEVICE,
                                     "BseMidiDevice",
                                     "MIDI device base type",
                                     __FILE__, __LINE__,
                                     &midi_device_info);
}
