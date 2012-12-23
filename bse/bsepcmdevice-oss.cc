// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include	"bsepcmdevice-oss.hh"
#include	"topconfig.h"
#include	"gsldatautils.hh"
#include	"gslcommon.hh" // FIXME: remove
#ifndef	BSE_PCM_DEVICE_CONF_OSS
BSE_DUMMY_TYPE (BsePcmDeviceOSS);
#else   /* BSE_PCM_DEVICE_CONF_OSS */
#if HAVE_SYS_SOUNDCARD_H
#include	<sys/soundcard.h>
#elif HAVE_SOUNDCARD_H
#include	<soundcard.h>
#endif
#include	<sys/ioctl.h>
#include	<sys/types.h>
#include	<sys/time.h>
#include	<unistd.h>
#include	<string.h>
#include	<errno.h>
#include	<fcntl.h>
#if	G_BYTE_ORDER == G_LITTLE_ENDIAN
#define	AFMT_S16_HE	AFMT_S16_LE
#elif	G_BYTE_ORDER == G_BIG_ENDIAN
#define	AFMT_S16_HE	AFMT_S16_BE
#else
#error	unsupported byte order in G_BYTE_ORDER
#endif
static SFI_MSG_TYPE_DEFINE (debug_pcm, "pcm", SFI_MSG_DEBUG, NULL);
#define DEBUG(...)      sfi_debug (debug_pcm, __VA_ARGS__)
/* --- OSS PCM handle --- */
typedef struct
{
  BsePcmHandle	handle;
  gint		fd;
  guint		n_frags;
  guint		frag_size;
  guint		frame_size;
  guint         queue_length;
  gint16       *frag_buf;
  guint         read_write_count;
  gboolean      needs_trigger;
  gboolean      hard_sync;
} OSSHandle;
#define	FRAG_BUF_SIZE(oss)	((oss)->frag_size * 4)
/* --- prototypes --- */
static BseErrorType oss_device_setup			(OSSHandle		*oss,
                                                         guint                   req_queue_length);
static void	    oss_device_retrigger		(OSSHandle		*oss);
static gsize	    oss_device_read			(BsePcmHandle		*handle,
							 gfloat			*values);
static void	    oss_device_write			(BsePcmHandle		*handle,
							 const gfloat		*values);
static gboolean     oss_device_check_io                 (BsePcmHandle           *handle,
                                                         glong                  *timeoutp);
static guint        oss_device_latency                  (BsePcmHandle           *handle);
/* --- variables --- */
static gpointer parent_class = NULL;
/* --- functions --- */
static void
bse_pcm_device_oss_init (BsePcmDeviceOSS *oss)
{
  oss->device_name = g_strdup (BSE_PCM_DEVICE_CONF_OSS);
}
static BseErrorType
check_device_usage (const gchar *name,
                    const gchar *check_mode)
{
  BseErrorType error = gsl_file_check (name, check_mode);
  if (!error && strchr (check_mode, 'w'))
    {
      errno = 0;
      /* beware, some drivers panic on O_RDONLY */
      gint fd = open (name, O_WRONLY | O_NONBLOCK, 0);  /* open non blocking to avoid waiting for other clients */
      /* we only check for ENODEV here, since the mode
       * might be wrong and the device may be busy.
       */
      if (errno == ENODEV)
        error = BSE_ERROR_DEVICE_NOT_AVAILABLE;
      if (fd >= 0)
        close (fd);
    }
  return error;
}
static SfiRing*
bse_pcm_device_oss_list_devices (BseDevice    *device)
{
  const gchar *postfixes[] = { "", "0", "1", "2", "3" };
  SfiRing *ring = NULL;
  guint i;
  gchar *last = NULL;
  for (i = 0; i < G_N_ELEMENTS (postfixes); i++)
    {
      gchar *dname = g_strconcat (BSE_PCM_DEVICE_OSS (device)->device_name, postfixes[i], NULL);
      if (!birnet_file_equals (last, dname))
        {
          if (check_device_usage (dname, "crw") == BSE_ERROR_NONE)
            ring = sfi_ring_append (ring,
                                    bse_device_entry_new (device,
                                                          g_strdup_printf ("%s,rw", dname),
                                                          g_strdup_printf ("%s (read-write)", dname)));
          else if (check_device_usage (dname, "cw") == BSE_ERROR_NONE)
            ring = sfi_ring_append (ring,
                                    bse_device_entry_new (device,
                                                          g_strdup_printf ("%s,wo", dname),
                                                          g_strdup_printf ("%s (write only)", dname)));
        }
      g_free (last);
      last = dname;
    }
  g_free (last);
  if (!ring)
    ring = sfi_ring_append (ring, bse_device_error_new (device, g_strdup_printf ("No devices found")));
  return ring;
}
static BseErrorType
bse_pcm_device_oss_open (BseDevice     *device,
                         gboolean       require_readable,
                         gboolean       require_writable,
                         guint          n_args,
                         const gchar  **args)
{
  const gchar *dname;
  if (n_args >= 1)      /* DEVICE */
    dname = args[0];
  else
    dname = BSE_PCM_DEVICE_OSS (device)->device_name;
  gint omode;
  gboolean hard_sync = FALSE;
  if (n_args >= 2)      /* MODE */
    {
      if (strstr (args[1], "rw"))
        omode = require_readable ? O_RDWR : O_WRONLY;
      else if (strstr (args[1], "wo"))
        omode = O_WRONLY;
      else if (strstr (args[1], "ro"))
        omode = O_RDONLY;
      else
        omode = require_readable ? O_RDWR : O_WRONLY;
      hard_sync = strstr (args[1], "hs") != NULL;
    }
  else
    omode = require_readable && require_writable ? O_RDWR : require_readable ? O_RDONLY : O_WRONLY;
  OSSHandle *oss = g_new0 (OSSHandle, 1);
  BsePcmHandle *handle = &oss->handle;
  /* setup request */
  handle->n_channels = BSE_PCM_DEVICE (device)->req_n_channels;
  handle->mix_freq = BSE_PCM_DEVICE (device)->req_mix_freq;
  oss->n_frags = 1024;
  oss->frag_buf = NULL;
  oss->fd = -1;
  oss->needs_trigger = TRUE;
  oss->hard_sync = hard_sync;
  /* try open */
  BseErrorType error;
  gint fd = -1;
  handle->readable = omode == O_RDWR || omode == O_RDONLY;      /* O_RDONLY maybe defined to 0 */
  handle->writable = omode == O_RDWR || omode == O_WRONLY;
  if ((handle->readable || !require_readable) && (handle->writable || !require_writable))
    fd = open (dname, omode | O_NONBLOCK, 0);                   /* open non blocking to avoid waiting for other clients */
  if (fd >= 0)
    {
      oss->fd = fd;
      /* try setup */
      guint frag_length = BSE_PCM_DEVICE (device)->req_block_length;
      oss->frag_size = frag_length * handle->n_channels * 2;
      guint latency = CLAMP (BSE_PCM_DEVICE (device)->req_latency_ms, 1, 5000); /* in milliseconds */
      latency = BSE_PCM_DEVICE (device)->req_mix_freq / 1000.0 * latency;       /* in frames */
      error = oss_device_setup (oss, latency);
    }
  else
    error = bse_error_from_errno (errno, BSE_ERROR_FILE_OPEN_FAILED);
  /* setup PCM handle or shutdown */
  if (!error)
    {
      oss->frag_buf = (gint16*) g_malloc (FRAG_BUF_SIZE (oss));
      handle->block_length = 0; /* setup after open */
      bse_device_set_opened (device, dname, handle->readable, handle->writable);
      if (handle->readable)
        handle->read = oss_device_read;
      if (handle->writable)
        handle->write = oss_device_write;
      handle->check_io = oss_device_check_io;
      handle->latency = oss_device_latency;
      BSE_PCM_DEVICE (device)->handle = handle;
    }
  else
    {
      if (oss->fd >= 0)
	close (oss->fd);
      g_free (oss->frag_buf);
      g_free (oss);
    }
  DEBUG ("OSS: opening \"%s\" readable=%d writable=%d: %s", dname, require_readable, require_writable, bse_error_blurb (error));
  return error;
}
static void
bse_pcm_device_oss_close (BseDevice *device)
{
  OSSHandle *oss = (OSSHandle*) BSE_PCM_DEVICE (device)->handle;
  BSE_PCM_DEVICE (device)->handle = NULL;
  (void) ioctl (oss->fd, SNDCTL_DSP_RESET, NULL);
  (void) close (oss->fd);
  g_free (oss->frag_buf);
  g_free (oss);
}
static void
bse_pcm_device_oss_finalize (GObject *object)
{
  BsePcmDeviceOSS *pdev_oss = BSE_PCM_DEVICE_OSS (object);
  g_free (pdev_oss->device_name);
  pdev_oss->device_name = NULL;
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}
static BseErrorType
oss_device_setup (OSSHandle *oss,
                  guint      req_queue_length)
{
  BsePcmHandle *handle = &oss->handle;
  gint fd = oss->fd;
  glong d_long;
  gint d_int;
  /* to get usable low-latency behaviour with OSS, we
   * make the device blocking, choose small fragments
   * and read() the first fragment once available.
   */
  d_long = fcntl (fd, F_GETFL);
  d_long &= ~O_NONBLOCK;
  if (fcntl (fd, F_SETFL, d_long))
    return BSE_ERROR_DEVICE_ASYNC;
  d_int = 0;
  if (ioctl (fd, SNDCTL_DSP_GETFMTS, &d_int) < 0)
    return BSE_ERROR_DEVICE_FORMAT;
  if ((d_int & AFMT_S16_HE) != AFMT_S16_HE)
    return BSE_ERROR_DEVICE_FORMAT;
  d_int = AFMT_S16_HE;
  if (ioctl (fd, SNDCTL_DSP_SETFMT, &d_int) < 0 ||
      d_int != AFMT_S16_HE)
    return BSE_ERROR_DEVICE_FORMAT;
  guint bytes_per_value = 2;
  d_int = handle->n_channels - 1;
  if (ioctl (fd, SNDCTL_DSP_STEREO, &d_int) < 0)
    return BSE_ERROR_DEVICE_CHANNELS;
  if (int (handle->n_channels) != d_int + 1)
    return BSE_ERROR_DEVICE_CHANNELS;
  oss->frame_size = handle->n_channels * bytes_per_value;
  d_int = handle->mix_freq;
  if (ioctl (fd, SNDCTL_DSP_SPEED, &d_int) < 0)
    return BSE_ERROR_DEVICE_FREQUENCY;
  handle->mix_freq = d_int;
  if (MAX (d_int, handle->mix_freq) - MIN (d_int, handle->mix_freq) > handle->mix_freq / 100)
    return BSE_ERROR_DEVICE_FREQUENCY;
  /* Note: fragment = n_fragments << 16;
   *       fragment |= g_bit_storage (fragment_size - 1);
   */
  oss->frag_size = CLAMP (oss->frag_size, 128, 65536);
  oss->n_frags = CLAMP (oss->n_frags, 128, 65536);
  if (handle->readable)
    {
      /* don't allow fragments to be too large, to get
       * low latency behaviour (hack)
       */
      oss->frag_size = MIN (oss->frag_size, 512);
    }
  d_int = (oss->n_frags << 16) | g_bit_storage (oss->frag_size - 1);
  if (ioctl (fd, SNDCTL_DSP_SETFRAGMENT, &d_int) < 0)
    return BSE_ERROR_DEVICE_LATENCY;
  d_int = 0;
  if (ioctl (fd, SNDCTL_DSP_GETBLKSIZE, &d_int) < 0 ||
      d_int < 128 || d_int > 131072 || (d_int & 1))
    return BSE_ERROR_DEVICE_BUFFER;
  audio_buf_info info = { 0, };
  if (handle->writable && ioctl (fd, SNDCTL_DSP_GETOSPACE, &info) < 0)
    return BSE_ERROR_DEVICE_BUFFER;
  else if (!handle->writable && ioctl (fd, SNDCTL_DSP_GETISPACE, &info) < 0)
    return BSE_ERROR_DEVICE_BUFFER;
  oss->n_frags = info.fragstotal;
  oss->frag_size = info.fragsize;
  oss->queue_length = info.bytes / oss->frame_size;
  if (oss->queue_length != oss->frag_size * oss->n_frags / oss->frame_size)
    {
      /* return BSE_ERROR_DEVICE_BUFFER; */
      sfi_diag ("OSS: buffer size (%d) differs from fragment space (%d)", info.bytes, info.fragstotal * info.fragsize);
      oss->queue_length = oss->n_frags * oss->frag_size / oss->frame_size;
    }
  if (handle->readable)
    {
      req_queue_length = MAX (req_queue_length, 3 * info.fragsize / oss->frame_size);   /* can't get better than 3 fragments */
      oss->queue_length = MIN (oss->queue_length, req_queue_length);
    }
  else  /* only writable */
    {
      /* give up on low latency for write-only handles, there's no reliable
       * way to achieve this with OSS. so we set a lower bound of enough milli
       * seconds for the latency, to not force the suer to adjust latency if
       * he catches a write-only OSS device temporarily.
       */
      req_queue_length = MIN (req_queue_length, oss->queue_length);
      oss->queue_length = CLAMP (25 * handle->mix_freq / 1000, req_queue_length, oss->queue_length);
    }
  DEBUG ("OSS: setup: w=%d r=%d n_channels=%d mix_freq=%u queue=%u nfrags=%u fsize=%u bufsz=%u",
         handle->writable,
         handle->readable,
         handle->n_channels,
         handle->mix_freq,
         oss->queue_length,
         oss->n_frags,
         oss->frag_size / oss->frame_size,
         info.bytes / oss->frame_size);
  return BSE_ERROR_NONE;
}
static void
oss_device_retrigger (OSSHandle *oss)
{
  BsePcmHandle *handle = &oss->handle;
  /* first, clear io buffers */
  (void) ioctl (oss->fd, SNDCTL_DSP_RESET, NULL);
  /* it should be enough to select() on the fd to trigger
   * capture/playback, but with some new OSS drivers
   * (clones) this is not the case anymore, so we also
   * use the SNDCTL_DSP_SETTRIGGER ioctl to achive this.
   */
  gint d_int = 0;
  if (oss->handle.readable)
    d_int |= PCM_ENABLE_INPUT;
  if (oss->handle.writable)
    d_int |= PCM_ENABLE_OUTPUT;
  (void) ioctl (oss->fd, SNDCTL_DSP_SETTRIGGER, &d_int);
  /* Jaroslav Kysela <perex@jcu.cz>:
   *  Important sequence:
   *     1) turn on capture
   *     2) write at least one fragment to create appropriate delay between
   *        capture and playback; we need some time for process wakeup and
   *        data copy in the user space
   * e.g.:
   * we write two fragments to playback
   * software latency is:
   *   2 * frag_size + delay between capture start & playback start
   */
  if (oss->handle.readable)
    {
      struct timeval tv = { 0, 0, };
      fd_set in_fds, out_fds;
      FD_ZERO (&in_fds);
      FD_ZERO (&out_fds);
      FD_SET (oss->fd, &in_fds);
      FD_SET (oss->fd, &out_fds);
      select (oss->fd + 1, &in_fds, &out_fds, NULL, &tv);
    }
  /* provide latency buffering */
  gint size = oss->queue_length * oss->frame_size, n;
  guint8 *silence = (guint8*) g_malloc0 (size);
  do
    n = write (oss->fd, silence, size);
  while (n < 0 && errno == EAGAIN); /* retry on signals */
  g_free (silence);
  glong d_long = fcntl (oss->fd, F_GETFL);
  DEBUG ("OSS: retriggering device (blocking=%u, r=%d, w=%d)...", (int) !(d_long & O_NONBLOCK), handle->readable, handle->writable);
  oss->needs_trigger = FALSE;
}
static gboolean
oss_device_check_io (BsePcmHandle *handle,
                     glong        *timeoutp)
{
  OSSHandle *oss = (OSSHandle*) handle;
  /* query device status and handle underruns */
  gboolean checked_underrun = FALSE;
 handle_underrun:
  if (handle->readable && oss->needs_trigger)
    oss_device_retrigger (oss);
  guint n_capture_avail; /* in n_frames */
  if (handle->readable)
    {
      audio_buf_info info = { 0, };
      (void) ioctl (oss->fd, SNDCTL_DSP_GETISPACE, &info);
      guint n_total_frames = info.fragstotal * info.fragsize / oss->frame_size;
      n_capture_avail = info.fragments * info.fragsize / oss->frame_size;
      /* probably more accurate: */
      n_capture_avail = info.bytes / oss->frame_size;
      /* OSS-bug fix, at least for es1371 in 2.3.34 */
      n_capture_avail = MIN (n_capture_avail, n_total_frames);
    }
  else
    n_capture_avail = 0;
  guint n_total_playback, n_playback_avail; /* in n_frames */
  if (handle->writable)
    {
      audio_buf_info info = { 0, };
      (void) ioctl (oss->fd, SNDCTL_DSP_GETOSPACE, &info);
      n_total_playback = info.fragstotal * info.fragsize / oss->frame_size;
      n_playback_avail = info.fragments * info.fragsize / oss->frame_size;
      /* probably more accurate: */
      n_playback_avail = info.bytes / oss->frame_size;
      /* OSS-bug fix, at least for es1371 in 2.3.34 */
      n_playback_avail = MIN (n_playback_avail, n_total_playback);
    }
  else
    n_playback_avail = n_total_playback = 0;
  if (!checked_underrun && handle->readable && handle->writable)
    {
      checked_underrun = TRUE;
      if (n_capture_avail > oss->queue_length + oss->frag_size / oss->frame_size)
        {
          if (oss->hard_sync)
            {
              g_printerr ("OSS: underrun detected (diff=%d), forcing hard sync (retrigger)\n", n_capture_avail - oss->queue_length);
              oss->needs_trigger = TRUE;
            }
          else /* soft-sync */
            {
              g_printerr ("OSS: underrun detected (diff=%d), skipping input\n", n_capture_avail - oss->queue_length);
              /* soft sync, throw away extra data */
              guint n_bytes = oss->frame_size * (n_capture_avail - oss->queue_length);
              do
                {
                  gssize l, n = MIN (FRAG_BUF_SIZE (oss), n_bytes);
                  do
                    l = read (oss->fd, oss->frag_buf, n);
                  while (l < 0 && errno == EINTR); /* don't mind signals */
                  if (l < 0)
                    n_bytes = 0; /* error, bail out */
                  else
                    n_bytes -= l;
                }
              while (n_bytes);
            }
          goto handle_underrun;
        }
    }
  /* check whether processing is possible */
  if (n_capture_avail >= handle->block_length)
    return TRUE;        /* can process */
  /* check immediate processing need */
  guint fill_frames = n_total_playback - n_playback_avail;
  if (fill_frames <= oss->queue_length)
    return TRUE;        /* need process */
  /* calculate timeout until processing is possible/needed */
  guint diff_frames;
  if (handle->readable)
    diff_frames = handle->block_length - n_capture_avail;
  else /* only writable */
    diff_frames = fill_frames - oss->queue_length;
  *timeoutp = diff_frames * 1000 / handle->mix_freq;
  /* OSS workaround for low latency */
  if (handle->readable)
    *timeoutp = 0;      /* prevent waiting in poll(), to force blocking read() */
  return *timeoutp == 0;
}
static guint
oss_device_latency (BsePcmHandle *handle)
{
  OSSHandle *oss = (OSSHandle*) handle;
  /* query device status */
  guint n_capture_avail; /* in n_frames */
  if (handle->readable)
    {
      audio_buf_info info = { 0, };
      (void) ioctl (oss->fd, SNDCTL_DSP_GETISPACE, &info);
      guint n_total_frames = info.fragstotal * info.fragsize / oss->frame_size;
      n_capture_avail = info.fragments * info.fragsize / oss->frame_size;
      /* probably more accurate: */
      n_capture_avail = info.bytes / oss->frame_size;
      /* OSS-bug fix, at least for es1371 in 2.3.34 */
      n_capture_avail = MIN (n_capture_avail, n_total_frames);
    }
  else
    n_capture_avail = 0;
  guint n_playback_filled; /* in n_frames */
  if (handle->writable)
    {
      audio_buf_info info = { 0, };
      (void) ioctl (oss->fd, SNDCTL_DSP_GETOSPACE, &info);
      guint n_total_playback = info.fragstotal * info.fragsize / oss->frame_size;
      guint n_playback_avail = info.fragments * info.fragsize / oss->frame_size;
      /* probably more accurate: */
      n_playback_avail = info.bytes / oss->frame_size;
      /* OSS-bug fix, at least for es1371 in 2.3.34 */
      n_playback_avail = MIN (n_playback_avail, n_total_playback);
      n_playback_filled = n_total_playback - n_playback_avail;
    }
  else
    n_playback_filled = 0;
  /* return total latency in frames */
  return n_capture_avail + n_playback_filled;
}
static gsize
oss_device_read (BsePcmHandle *handle,
		 gfloat       *values)
{
  const gsize n_values = handle->n_channels * handle->block_length;
  OSSHandle *oss = (OSSHandle*) handle;
  gint fd = oss->fd;
  gsize buf_size = FRAG_BUF_SIZE (oss);
  gpointer buf = oss->frag_buf;
  gfloat *dest = values;
  gsize n_left = n_values;
  g_return_val_if_fail (oss->frame_size == 4, 0);
  do
    {
      gsize n = MIN (buf_size, n_left << 1);
      gint16 *b, *s = (gint16*) buf;
      gssize l;
      do
	l = read (fd, buf, n);
      while (l < 0 && errno == EINTR); /* don't mind signals */
      if (l < 0) /* sigh, errors during read? */
	{
	  memset (buf, 0, n);
	  l = n;
	}
      l >>= 1;
      if (values)       /* don't convert on dummy reads */
        for (b = s + l; s < b; s++)
          *dest++ = *s * (1.0 / 32768.0);
      n_left -= l;
    }
  while (n_left);
  oss->read_write_count += 1;
  return n_values;
}
static void
oss_device_write (BsePcmHandle *handle,
		  const gfloat *values)
{
  gsize n_values = handle->n_channels * handle->block_length;
  OSSHandle *oss = (OSSHandle*) handle;
  gint fd = oss->fd;
  gsize buf_size = FRAG_BUF_SIZE (oss);
  gpointer buf = oss->frag_buf;
  const gfloat *s = values;
  if (handle->readable)
    while (oss->read_write_count < 1)
      oss_device_read (handle, NULL);   /* dummy read to sync device */
  g_return_if_fail (oss->frame_size == 4);
  do
    {
      gsize n = MIN (buf_size, n_values << 1);
      gssize l;
      gsl_conv_from_float_clip (GSL_WAVE_FORMAT_SIGNED_16,
				G_BYTE_ORDER,
				s,
				buf,
				n >> 1);
      s += n >> 1;
      do
	l = write (fd, buf, n);
      while (l < 0 && errno == EINTR); /* don't mind signals */
      if (l < 0) /* sigh, errors during write? */
	l = n;
      n_values -= l >> 1;
    }
  while (n_values);
  oss->read_write_count -= 1;
}
static void
bse_pcm_device_oss_class_init (BsePcmDeviceOSSClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseDeviceClass *device_class = BSE_DEVICE_CLASS (klass);
  parent_class = g_type_class_peek_parent (klass);
  gobject_class->finalize = bse_pcm_device_oss_finalize;
  device_class->list_devices = bse_pcm_device_oss_list_devices;
  bse_device_class_setup (klass,
                          BSE_RATING_DEFAULT,
                          "oss",
                          _("DEVICE,MODE"),
                          /* TRANSLATORS: keep this text to 70 chars in width */
                          _("Open Sound System PCM driver:\n"
                            "  DEVICE - PCM device file name\n"
                            "  MODE   - may contain \"rw\", \"ro\" or \"wo\" for\n"
                            "           read-only, read-write or write-only access;\n"
                            "           adding \"hs\" forces hard sync on underruns.\n"));
  device_class->open = bse_pcm_device_oss_open;
  device_class->close = bse_pcm_device_oss_close;
}
BSE_BUILTIN_TYPE (BsePcmDeviceOSS)
{
  GType pcm_device_oss_type;
  static const GTypeInfo pcm_device_oss_info = {
    sizeof (BsePcmDeviceOSSClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_pcm_device_oss_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    sizeof (BsePcmDeviceOSS),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_pcm_device_oss_init,
  };
  pcm_device_oss_type = bse_type_register_static (BSE_TYPE_PCM_DEVICE,
						  "BsePcmDeviceOSS",
						  "PCM device implementation for OSS Lite /dev/dsp",
                                                  __FILE__, __LINE__,
                                                  &pcm_device_oss_info);
  return pcm_device_oss_type;
}
#endif	/* BSE_PCM_DEVICE_CONF_OSS */
