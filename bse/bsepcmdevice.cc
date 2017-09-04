// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsepcmdevice.hh"

#include "gslcommon.hh"
#include <errno.h>


/* --- variables --- */
static gpointer parent_class = NULL;


/* --- functions --- */
static void
bse_pcm_device_init (BsePcmDevice *pdev)
{
  pdev->req_n_channels = 2;
  pdev->req_mix_freq = 44100;
  pdev->req_latency_ms = 150;
  pdev->req_block_length = 1024;
  pdev->handle = NULL;
}

void
bse_pcm_device_request (BsePcmDevice  *self,
                        guint	       n_channels,
                        guint          mix_freq,
                        guint          latency_ms,
                        guint          block_length) /* in frames */
{
  assert_return (BSE_IS_PCM_DEVICE (self));
  assert_return (!BSE_DEVICE_OPEN (self));
  assert_return (n_channels >= 1 && n_channels <= 128);
  assert_return (mix_freq >= 1000 && mix_freq <= 192000);

  self->req_n_channels = n_channels;
  self->req_mix_freq = mix_freq;
  self->req_block_length = MAX (block_length, 2);
  self->req_latency_ms = latency_ms;
}

static void
bse_pcm_device_dispose (GObject *object)
{
  BsePcmDevice *pdev = BSE_PCM_DEVICE (object);

  if (BSE_DEVICE_OPEN (pdev))
    {
      Bse::warning (G_STRLOC ": pcm device still opened");
      bse_device_close (BSE_DEVICE (pdev));
    }
  if (pdev->handle)
    Bse::warning (G_STRLOC ": pcm device with stale pcm handle");
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
pcm_device_post_open (BseDevice *device)
{
  BsePcmDevice *self = BSE_PCM_DEVICE (device);
  assert_return (BSE_DEVICE_OPEN (self) && self->handle);
  assert_return (BSE_DEVICE_OPEN (self) && self->handle->block_length == 0);
  new (&self->handle->spinlock) Bse::Spinlock();
}

static void
pcm_device_pre_close (BseDevice *device)
{
  BsePcmDevice *self = BSE_PCM_DEVICE (device);
  self->handle->spinlock.~Spinlock();
}

guint
bse_pcm_device_get_mix_freq (BsePcmDevice *pdev)
{
  assert_return (BSE_IS_PCM_DEVICE (pdev), 0);
  if (BSE_DEVICE_OPEN (pdev))
    return pdev->handle->mix_freq;
  else
    return 0;
}

BsePcmHandle*
bse_pcm_device_get_handle (BsePcmDevice *pdev,
                           guint         block_length)
{
  assert_return (BSE_IS_PCM_DEVICE (pdev), NULL);
  assert_return (BSE_DEVICE_OPEN (pdev), NULL);
  assert_return (block_length > 0, NULL);
  pdev->handle->spinlock.lock();
  if (!pdev->handle->block_length)
    pdev->handle->block_length = block_length;
  pdev->handle->spinlock.unlock();
  if (pdev->handle->block_length == block_length)
    return pdev->handle;
  else
    return NULL;
}
gsize
bse_pcm_handle_read (BsePcmHandle *handle,
		     gsize         n_values,
		     gfloat       *values)
{
  gsize n;
  assert_return (handle != NULL, 0);
  assert_return (handle->readable, 0);
  assert_return (n_values == handle->block_length * handle->n_channels, 0);
  handle->spinlock.lock();
  n = handle->read (handle, values);
  handle->spinlock.unlock();
  assert_return (n == handle->block_length * handle->n_channels, n);
  return n;
}
void
bse_pcm_handle_write (BsePcmHandle *handle,
		      gsize         n_values,
		      const gfloat *values)
{
  assert_return (handle != NULL);
  assert_return (handle->writable);
  assert_return (values != NULL);
  assert_return (n_values == handle->block_length * handle->n_channels);
  handle->spinlock.lock();
  handle->write (handle, values);
  handle->spinlock.unlock();
}
gboolean
bse_pcm_handle_check_io (BsePcmHandle           *handle,
                         glong                  *timeoutp)
{
  assert_return (handle != NULL, 0);
  glong dummy;
  if (!timeoutp)
    timeoutp = &dummy;
  handle->spinlock.lock();
  gboolean can_read_write = handle->check_io (handle, timeoutp);
  handle->spinlock.unlock();
  return can_read_write;
}

guint
bse_pcm_handle_latency (BsePcmHandle *handle)
{
  assert_return (handle != NULL, 0);
  handle->spinlock.lock();
  guint n_frames = handle->latency (handle);
  handle->spinlock.unlock();
  return n_frames;
}


/* --- frequency utilities --- */
guint
bse_pcm_device_frequency_align (gint mix_freq)
{
  static const gint frequency_list[] = {
    5512, 8000, 11025, 16000, 22050, 32000,
    44100, 48000, 64000, 88200, 96000, 176400, 192000
  };
  guint i, best = frequency_list[0], diff = ABS (mix_freq - frequency_list[0]);
  for (i = 1; i < G_N_ELEMENTS (frequency_list); i++)
    {
      guint diff2 = ABS (mix_freq - frequency_list[i]);
      if (diff2 <= diff)
        {
          best = frequency_list[i];
          diff = diff2;
        }
    }
  return best;
}

static void
bse_pcm_device_class_init (BsePcmDeviceClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseDeviceClass *device_class = BSE_DEVICE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->dispose = bse_pcm_device_dispose;

  device_class->post_open = pcm_device_post_open;
  device_class->pre_close = pcm_device_pre_close;
}

BSE_BUILTIN_TYPE (BsePcmDevice)
{
  static const GTypeInfo pcm_device_info = {
    sizeof (BsePcmDeviceClass),

    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_pcm_device_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,

    sizeof (BsePcmDevice),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_pcm_device_init,
  };

  return bse_type_register_abstract (BSE_TYPE_DEVICE,
                                     "BsePcmDevice",
                                     "PCM device base type",
                                     __FILE__, __LINE__,
                                     &pcm_device_info);
}
