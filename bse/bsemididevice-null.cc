// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include	"bsemididevice-null.hh"
#include	"bseserver.hh"
static SFI_MSG_TYPE_DEFINE (debug_midi, "midi", SFI_MSG_DEBUG, NULL);
#define MIDI_DEBUG(...) sfi_debug (debug_midi, __VA_ARGS__)
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
                           uint           n_args,
                           const char   **args)
{
  NULLHandle *null = g_new0 (NULLHandle, 1);
  BseMidiHandle *handle = &null->handle;
  /* setup request */
  handle->readable = require_readable;
  handle->writable = require_writable;
  bse_device_set_opened (device, "null", handle->readable, handle->writable);
  BSE_MIDI_DEVICE (device)->handle = handle;
  MIDI_DEBUG ("NULL: opening MIDI readable=%d writable=%d: %s", require_readable, require_writable, bse_error_blurb (BSE_ERROR_NONE));
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
bse_midi_device_null_class_init (BseMidiDeviceNULLClass *klass)
{
  BseDeviceClass *device_class = BSE_DEVICE_CLASS (klass);
  device_class->list_devices = bse_midi_device_null_list_devices;
  bse_device_class_setup (klass,
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
                                   __FILE__, __LINE__,
                                   &type_info);
  return type;
}
