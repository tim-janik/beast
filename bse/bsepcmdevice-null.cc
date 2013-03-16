// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsepcmdevice-null.hh"
#include "bsesequencer.hh"
#include "bseengine.hh"
#include <string.h>
static SFI_MSG_TYPE_DEFINE (debug_pcm, "pcm", SFI_MSG_DEBUG, NULL);
#define DEBUG(...)      sfi_debug (debug_pcm, __VA_ARGS__)
typedef struct
{
  BsePcmHandle handle;
  guint        busy_us;
  guint        sleep_us;
} NullHandle;
/* --- prototypes --- */
static gsize        null_device_read      (BsePcmHandle *handle,
                                           gfloat       *values);
static void         null_device_write     (BsePcmHandle *handle,
                                           const gfloat *values);
static gboolean     null_device_check_io  (BsePcmHandle *handle,
                                           glong        *timeoutp);
static guint        null_device_latency   (BsePcmHandle *handle);
/* --- functions --- */
static void
bse_pcm_device_null_init (BsePcmDeviceNull *null)
{
}
static SfiRing*
bse_pcm_device_null_list_devices (BseDevice *device)
{
  SfiRing *ring = NULL;
  ring = sfi_ring_append (ring, bse_device_entry_new (device, g_strdup_printf ("default"), NULL));
  return ring;
}
static BseErrorType
bse_pcm_device_null_open (BseDevice     *device,
                          gboolean       require_readable,
                          gboolean       require_writable,
                          guint          n_args,
                          const gchar  **args)
{
  NullHandle *null = g_new0 (NullHandle, 1);
  BsePcmHandle *handle = &null->handle;
  /* setup request */
  handle->readable = require_readable;
  handle->writable = require_writable;
  handle->n_channels = 2;
  handle->mix_freq = BSE_PCM_DEVICE (device)->req_mix_freq;
  bse_device_set_opened (device, "null", handle->readable, handle->writable);
  handle->read = null_device_read;
  handle->write = null_device_write;
  handle->check_io = null_device_check_io;
  handle->latency = null_device_latency;
  null->busy_us = 0;
  if (n_args == 1 && strcmp (args[0], "nosleep") == 0)
    null->sleep_us = 0;
  else
    null->sleep_us = 10 * 1000;
  BSE_PCM_DEVICE (device)->handle = handle;
  DEBUG ("NULL: opening PCM readable=%d writable=%d: %s", require_readable, require_writable, bse_error_blurb (BSE_ERROR_NONE));
  return BSE_ERROR_NONE;
}
static void
bse_pcm_device_null_close (BseDevice *device)
{
  NullHandle *null = (NullHandle*) BSE_PCM_DEVICE (device)->handle;
  BSE_PCM_DEVICE (device)->handle = NULL;
  g_free (null);
}
static gboolean
null_device_check_io (BsePcmHandle *handle,
                      glong        *timeoutp)
{
  /* keep the sequencer busy or we will constantly timeout */
  bse_sequencer_wakeup();
  *timeoutp = 1;
  /* ensure sequencer fairness */
  return !bse_sequencer_thread_lagging (2);
}
static guint
null_device_latency (BsePcmHandle *handle)
{
  /* total latency in frames */
  return handle->mix_freq / 10;
}
static gsize
null_device_read (BsePcmHandle *handle,
                  gfloat       *values)
{
  const gsize n_values = handle->n_channels * handle->block_length;
  memset (values, 0, sizeof (values[0]) * n_values);
  return n_values;
}
static void
null_device_write (BsePcmHandle *handle,
                   const gfloat *values)
{
  NullHandle *null = (NullHandle*) handle;
  null->busy_us += handle->block_length * 1000000 / handle->mix_freq;
  if (null->busy_us >= 100 * 1000)
    {
      null->busy_us = 0;
      /* give cpu to other applications (we might run at nice level -20) */
      if (null->sleep_us)
        g_usleep (null->sleep_us);
    }
}
static void
bse_pcm_device_null_class_init (BsePcmDeviceNullClass *klass)
{
  BseDeviceClass *device_class = BSE_DEVICE_CLASS (klass);
  device_class->list_devices = bse_pcm_device_null_list_devices;
  bse_device_class_setup (device_class,
                          -1,
                          "null",
                          _("[nosleep]"), /* syntax */
                          /* TRANSLATORS: keep this text to 70 chars in width */
                          _("Discard all PCM output and provide zero blocks as input. This\n"
                            "driver is not part of the automatic PCM device selection list.\n"
                            "Normally the null driver sleeps regularily to give CPU cycles to\n"
                            "other apps, which is helpful when running at high priority.\n"
                            "Sleeping can be disabled by passing \"nosleep\" as option.\n"));
  device_class->open = bse_pcm_device_null_open;
  device_class->close = bse_pcm_device_null_close;
}
BSE_BUILTIN_TYPE (BsePcmDeviceNull)
{
  GType pcm_device_null_type;
  static const GTypeInfo pcm_device_null_info = {
    sizeof (BsePcmDeviceNullClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_pcm_device_null_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    sizeof (BsePcmDeviceNull),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_pcm_device_null_init,
  };
  pcm_device_null_type = bse_type_register_static (BSE_TYPE_PCM_DEVICE,
                                                   "BsePcmDeviceNull",
                                                   "Null PCM device implementation",
                                                   __FILE__, __LINE__,
                                                   &pcm_device_null_info);
  return pcm_device_null_type;
}
