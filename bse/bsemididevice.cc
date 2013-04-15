// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsemididevice.hh"

#include "bsemididecoder.hh"

#include <errno.h>


/* --- variables --- */
static void *parent_class = NULL;


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
bse_midi_device_class_init (BseMidiDeviceClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

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
