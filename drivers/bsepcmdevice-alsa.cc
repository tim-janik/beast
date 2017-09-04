// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include <bse/bsecxxplugin.hh>
#include "bsepcmdevice-alsa.hh"
#include <bse/gsldatautils.hh>
#include <alsa/asoundlib.h>
#include <string.h>
#include <errno.h>

#if     G_BYTE_ORDER == G_LITTLE_ENDIAN
#define SND_PCM_FORMAT_S16_HE   SND_PCM_FORMAT_S16_LE
#elif   G_BYTE_ORDER == G_BIG_ENDIAN
#define SND_PCM_FORMAT_S16_HE   SND_PCM_FORMAT_S16_BE
#else
#error  unsupported byte order in G_BYTE_ORDER
#endif

#define PDEBUG(...)     Bse::debug ("pcm-alsa", __VA_ARGS__)

static snd_output_t *snd_output = NULL; // only for debugging

/* --- ALSA PCM handle --- */
typedef struct
{
  BsePcmHandle  handle;
  snd_pcm_t    *read_handle;
  snd_pcm_t    *write_handle;
  guint         n_periods;
  guint         period_size;    /* in frames */
  guint         frame_size;
  gint16       *period_buffer;
  guint         read_write_count;
} AlsaPcmHandle;


/* --- prototypes --- */
static void             bse_pcm_device_alsa_class_init  (BsePcmDeviceALSAClass  *klass);
static void             bse_pcm_device_alsa_init        (BsePcmDeviceALSA       *self);
static Bse::Error     alsa_device_setup               (AlsaPcmHandle          *alsa,
                                                         snd_pcm_t              *phandle,
                                                         guint                   latency_ms,
                                                         guint                  *mix_freq,
                                                         guint                  *n_frags,
                                                         guint                  *frag_size);
static gsize            alsa_device_read                (BsePcmHandle           *handle,
                                                         gfloat                 *values);
static void             alsa_device_write               (BsePcmHandle           *handle,
                                                         const gfloat           *values);
static gboolean         alsa_device_check_io            (BsePcmHandle           *handle,
                                                         glong                  *tiumeoutp);
static guint            alsa_device_latency             (BsePcmHandle           *handle);
// == Type Registration ==
BSE_RESIDENT_TYPE_DEF (BsePcmDeviceALSA, bse_pcm_device_alsa, BSE_TYPE_PCM_DEVICE, NULL,
                       "PCM driver implementation for the Advanced Linux Sound Architecture "
                       "(http://alsa-project.org/alsa-doc/alsa-lib/pcm.html)", NULL);
/* --- variables --- */
static gpointer parent_class = NULL;
static guint    const_pcm_status_sizeof = 0;

/* --- functions --- */
static void
bse_pcm_device_alsa_init (BsePcmDeviceALSA *self)
{
  const_pcm_status_sizeof = snd_pcm_status_sizeof();
}

static std::string
substitute_string (const std::string &from, const std::string &to, const std::string &input)
{
  std::string::size_type l = 0;
  std::string target;
  for (std::string::size_type i = input.find (from, 0); i != std::string::npos; l = i + 1, i = input.find (from, l))
    {
      target.append (input, l, i - l);
      target.append (to);
    }
  target.append (input, l, input.size() - l);
  return target;
}

static SfiRing*
list_pcm_devices (BseDevice *device, SfiRing *ring, const char *device_group)
{
  void **nhints = NULL;
  if (snd_device_name_hint (-1, "pcm", &nhints) < 0)
    return ring;
  for (void **hint = nhints; *hint; hint++)
    {
      char *name = snd_device_name_get_hint (*hint, "NAME");
      char *desc = snd_device_name_get_hint (*hint, "DESC");
      char *ioid = snd_device_name_get_hint (*hint, "IOID");
      const char *cioid = ioid ? ioid : "Duplex";
      if (name && desc && strcmp (cioid, device_group) == 0)
        {
          std::string blurb = substitute_string ("\n", " ", desc);
          BseDeviceEntry *entry = bse_device_group_entry_new (device, g_strdup (name), g_strdup (device_group),
                                                              g_strdup_format ("%s - %s", name, blurb.c_str()));
          ring = sfi_ring_append (ring, entry);
        }
      if (name) free (name);
      if (desc) free (desc);
      if (ioid) free (ioid);
    }
  snd_device_name_free_hint (nhints);
  return ring;
}

#define alsa_alloca0(struc)     ({ struc##_t *ptr = (struc##_t*) alloca (struc##_sizeof()); memset (ptr, 0, struc##_sizeof()); ptr; })

static SfiRing*
bse_pcm_device_alsa_list_devices (BseDevice *device)
{
  SfiRing *ring = NULL;
  // find and enlist Duplex and Output devices, 'default' should be amongst these
  ring = list_pcm_devices (device, ring, "Duplex");
  ring = list_pcm_devices (device, ring, "Output");
  // walk and enlist devices from all sound cards
  snd_ctl_card_info_t *cinfo = alsa_alloca0 (snd_ctl_card_info);
  snd_pcm_info_t *pinfo = alsa_alloca0 (snd_pcm_info);
  snd_pcm_info_t *rinfo = alsa_alloca0 (snd_pcm_info);

  int cindex = -1;
  snd_card_next (&cindex);
  while (cindex >= 0)
    {
      snd_ctl_card_info_clear (cinfo);
      std::string hwid = Bse::string_format ("hw:CARD=%u", cindex);
      snd_ctl_t *chandle = NULL;
      if (snd_ctl_open (&chandle, hwid.c_str(), SND_CTL_NONBLOCK) < 0 || !chandle)
        continue;
      if (snd_ctl_card_info (chandle, cinfo) < 0)
        {
          snd_ctl_close (chandle);
          continue;
        }

      gchar *device_group = g_strdup_format ("%s - %s", snd_ctl_card_info_get_id (cinfo), snd_ctl_card_info_get_longname (cinfo));

      int pindex = -1;
      snd_ctl_pcm_next_device (chandle, &pindex);
      while (pindex >= 0)
        {
          snd_pcm_info_set_device (pinfo, pindex);
          snd_pcm_info_set_device (rinfo, pindex);
          snd_pcm_info_set_subdevice (pinfo, 0);
          snd_pcm_info_set_subdevice (rinfo, 0);
          snd_pcm_info_set_stream (pinfo, SND_PCM_STREAM_PLAYBACK);
          snd_pcm_info_set_stream (rinfo, SND_PCM_STREAM_CAPTURE);
          gboolean writable = !(snd_ctl_pcm_info (chandle, pinfo) < 0);
          gboolean readable = !(snd_ctl_pcm_info (chandle, rinfo) < 0);
          if (!writable && !readable)
            continue;
          guint total_playback_subdevices = writable ? snd_pcm_info_get_subdevices_count (pinfo) : 0;
          guint avail_playback_subdevices = writable ? snd_pcm_info_get_subdevices_avail (pinfo) : 0;
          guint total_capture_subdevices = readable ? snd_pcm_info_get_subdevices_count (rinfo) : 0;
          guint avail_capture_subdevices = readable ? snd_pcm_info_get_subdevices_avail (rinfo) : 0;
          gchar *pdevs = NULL, *rdevs = NULL;
          if (total_playback_subdevices && total_playback_subdevices != avail_playback_subdevices)
            pdevs = g_strdup_format ("%u*playback (%u busy)", total_playback_subdevices, total_playback_subdevices - avail_playback_subdevices);
          else if (total_playback_subdevices)
            pdevs = g_strdup_format ("%u*playback", total_playback_subdevices);
          if (total_capture_subdevices && total_capture_subdevices != avail_capture_subdevices)
            rdevs = g_strdup_format ("%u*capture (%u busy)", total_capture_subdevices, total_capture_subdevices - avail_capture_subdevices);
          else if (total_capture_subdevices)
            rdevs = g_strdup_format ("%u*capture", total_capture_subdevices);
          const gchar *joiner = pdevs && rdevs ? " + " : "";
          BseDeviceEntry *entry;
          entry = bse_device_group_entry_new (device,
                                              g_strdup_format ("hw:%u,%u", cindex, pindex),
                                              g_strdup (device_group),
                                              g_strdup_format ("hw:%u,%u - subdevices: %s%s%s",
                                                               cindex, pindex,
                                                               pdevs ? pdevs : "",
                                                               joiner,
                                                               rdevs ? rdevs : ""));
          ring = sfi_ring_append (ring, entry);
          g_free (pdevs);
          g_free (rdevs);
          if (snd_ctl_pcm_next_device (chandle, &pindex) < 0)
            break;
        }
      g_free (device_group);
      snd_ctl_close (chandle);
      if (snd_card_next (&cindex) < 0)
        break;
    }
  if (!ring)
    ring = sfi_ring_append (ring, bse_device_error_new (device, g_strdup_format ("No devices found")));
  return ring;
}

static void
silent_error_handler (const char *file,
                      int         line,
                      const char *function,
                      int         err,
                      const char *fmt,
                      ...)
{
}

static Bse::Error
bse_pcm_device_alsa_open (BseDevice     *device,
                          gboolean       require_readable,
                          gboolean       require_writable,
                          guint          n_args,
                          const gchar  **args)
{
  int aerror = 0;
  AlsaPcmHandle *alsa = g_new0 (AlsaPcmHandle, 1);
  BsePcmHandle *handle = &alsa->handle;
  /* setup request */
  handle->readable = require_readable;
  handle->writable = require_writable;
  handle->n_channels = BSE_PCM_DEVICE (device)->req_n_channels;
  alsa->frame_size = handle->n_channels * 2; /* for 16bit samples */
  /* try open */
  gchar *dname = n_args ? g_strjoinv (",", (gchar**) args) : g_strdup ("default");
  snd_lib_error_set_handler (silent_error_handler);
  if (!aerror && require_readable)
    aerror = snd_pcm_open (&alsa->read_handle, dname, SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
  if (!aerror && require_writable)
    aerror = snd_pcm_open (&alsa->write_handle, dname, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
  snd_lib_error_set_handler (NULL);
  /* try setup */
  const guint period_size = BSE_PCM_DEVICE (device)->req_block_length;
  Bse::Error error = !aerror ? Bse::Error::NONE : bse_error_from_errno (-aerror, Bse::Error::FILE_OPEN_FAILED);
  guint rh_freq = BSE_PCM_DEVICE (device)->req_mix_freq, rh_n_periods = 0, rh_period_size = period_size;
  if (!aerror && alsa->read_handle)
    error = alsa_device_setup (alsa, alsa->read_handle, BSE_PCM_DEVICE (device)->req_latency_ms, &rh_freq, &rh_n_periods, &rh_period_size);
  guint wh_freq = BSE_PCM_DEVICE (device)->req_mix_freq, wh_n_periods = 0, wh_period_size = period_size;
  if (!aerror && alsa->write_handle)
    error = alsa_device_setup (alsa, alsa->write_handle, BSE_PCM_DEVICE (device)->req_latency_ms, &wh_freq, &wh_n_periods, &wh_period_size);
  /* check duplex */
  if (error == 0 && alsa->read_handle && alsa->write_handle && rh_freq != wh_freq)
    error = Bse::Error::DEVICES_MISMATCH;
  handle->mix_freq = alsa->read_handle ? rh_freq : wh_freq;
  if (error == 0 && alsa->read_handle && alsa->write_handle && rh_n_periods != wh_n_periods)
    error = Bse::Error::DEVICES_MISMATCH;
  alsa->n_periods = alsa->read_handle ? rh_n_periods : wh_n_periods;
  if (error == 0 && alsa->read_handle && alsa->write_handle && rh_period_size != wh_period_size)
    error = Bse::Error::DEVICES_MISMATCH;
  alsa->period_size = alsa->read_handle ? rh_period_size : wh_period_size;
  if (error == 0 && alsa->read_handle && alsa->write_handle &&
      snd_pcm_link (alsa->read_handle, alsa->write_handle) < 0)
    error = Bse::Error::DEVICES_MISMATCH;
  if (error == 0 && snd_pcm_prepare (alsa->read_handle ? alsa->read_handle : alsa->write_handle) < 0)
    error = Bse::Error::FILE_OPEN_FAILED;

  /* setup PCM handle or shutdown */
  if (error == 0)
    {
      alsa->period_buffer = (gint16*) g_malloc (alsa->period_size * alsa->frame_size);
      bse_device_set_opened (device, dname, handle->readable, handle->writable);
      if (handle->readable)
        handle->read = alsa_device_read;
      if (handle->writable)
        handle->write = alsa_device_write;
      handle->check_io = alsa_device_check_io;
      handle->latency = alsa_device_latency;
      BSE_PCM_DEVICE (device)->handle = handle;
    }
  else
    {
      if (alsa->read_handle)
        snd_pcm_close (alsa->read_handle);
      if (alsa->write_handle)
        snd_pcm_close (alsa->write_handle);
      g_free (alsa->period_buffer);
      g_free (alsa);
    }
  PDEBUG ("ALSA: opening PCM \"%s\" readable=%d writable=%d: %s", dname, require_readable, require_writable, bse_error_blurb (error));
  g_free (dname);

  return error;
}

static void
bse_pcm_device_alsa_close (BseDevice *device)
{
  AlsaPcmHandle *alsa = (AlsaPcmHandle*) BSE_PCM_DEVICE (device)->handle;
  BSE_PCM_DEVICE (device)->handle = NULL;

  if (alsa->read_handle)
    {
      snd_pcm_drop (alsa->read_handle);
      snd_pcm_close (alsa->read_handle);
    }
  if (alsa->write_handle)
    {
      snd_pcm_nonblock (alsa->write_handle, 0);
      snd_pcm_drain (alsa->write_handle);
      snd_pcm_close (alsa->write_handle);
    }
  g_free (alsa->period_buffer);
  g_free (alsa);
}

static void
bse_pcm_device_alsa_finalize (GObject *object)
{
  /* BsePcmDeviceALSA *self = BSE_PCM_DEVICE_ALSA (object); */
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static Bse::Error
alsa_device_setup (AlsaPcmHandle       *alsa,
                   snd_pcm_t           *phandle,
                   guint                latency_ms,
                   guint               *mix_freq,
                   guint               *n_periodsp,
                   guint               *period_sizep)
{
  BsePcmHandle *handle = &alsa->handle;
  /* turn on blocking behaviour since we may end up in read() with an unfilled buffer */
  if (snd_pcm_nonblock (phandle, 0) < 0)
    return Bse::Error::FILE_OPEN_FAILED;
  /* setup hardware configuration */
  snd_pcm_hw_params_t *hparams = alsa_alloca0 (snd_pcm_hw_params);
  if (snd_pcm_hw_params_any (phandle, hparams) < 0)
    return Bse::Error::FILE_OPEN_FAILED;
  if (snd_pcm_hw_params_set_channels (phandle, hparams, handle->n_channels) < 0)
    return Bse::Error::DEVICE_CHANNELS;
  if (snd_pcm_hw_params_set_access (phandle, hparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
    return Bse::Error::DEVICE_FORMAT;
  if (snd_pcm_hw_params_set_format (phandle, hparams, SND_PCM_FORMAT_S16_HE) < 0)
    return Bse::Error::DEVICE_FORMAT;
  if (alsa->frame_size != handle->n_channels * 2) /* 16bit samples */
    return Bse::Error::DEVICE_FORMAT;
  unsigned int rate = *mix_freq;
  if (snd_pcm_hw_params_set_rate_near (phandle, hparams, &rate, NULL) < 0)
    return Bse::Error::DEVICE_FREQUENCY;
  if (MAX (rate, *mix_freq) - MIN (rate, *mix_freq) > *mix_freq / 100)
    return Bse::Error::DEVICE_FREQUENCY;
  snd_pcm_uframes_t period_size = *period_sizep;
  if (snd_pcm_hw_params_set_period_size_near (phandle, hparams, &period_size, 0) < 0)
    return Bse::Error::DEVICE_LATENCY;
  guint buffer_time_us = latency_ms * 1000;
  if (snd_pcm_hw_params_set_buffer_time_near (phandle, hparams, &buffer_time_us, NULL) < 0)
    return Bse::Error::DEVICE_LATENCY;
  if (snd_pcm_hw_params (phandle, hparams) < 0)
    return Bse::Error::FILE_OPEN_FAILED;
  /* verify hardware settings */
  unsigned int nperiods = 0;
  if (snd_pcm_hw_params_get_periods (hparams, &nperiods, NULL) < 0 || nperiods < 2)
    return Bse::Error::DEVICE_BUFFER;
  snd_pcm_uframes_t buffer_size = 0;
  if (snd_pcm_hw_params_get_buffer_size (hparams, &buffer_size) < 0 || buffer_size != nperiods * period_size)
    return Bse::Error::DEVICE_BUFFER;
  /* setup software configuration */
  snd_pcm_sw_params_t *sparams = alsa_alloca0 (snd_pcm_sw_params);
  if (snd_pcm_sw_params_current (phandle, sparams) < 0)
    return Bse::Error::FILE_OPEN_FAILED;
  if (snd_pcm_sw_params_set_start_threshold (phandle, sparams, (buffer_size / period_size) * period_size) < 0)
    return Bse::Error::DEVICE_BUFFER;
  if (snd_pcm_sw_params_set_avail_min (phandle, sparams, period_size) < 0)
    return Bse::Error::DEVICE_LATENCY;
  snd_pcm_uframes_t boundary;
  if (snd_pcm_sw_params_get_boundary (sparams, &boundary) < 0)
    return Bse::Error::FILE_OPEN_FAILED;
  gboolean stop_on_xrun = FALSE;                                                /* ignore XRUN */
  guint threshold = stop_on_xrun ? buffer_size : MIN (buffer_size * 2, boundary);
  // constrain boundary for stop_threshold, to work around 64bit alsa lib setting boundary to 0x5000000000000000
  if (snd_pcm_sw_params_set_stop_threshold (phandle, sparams, threshold) < 0)
    return Bse::Error::DEVICE_BUFFER;
  if (snd_pcm_sw_params_set_silence_threshold (phandle, sparams, 0) < 0 ||
      snd_pcm_sw_params_set_silence_size (phandle, sparams, boundary) < 0)      /* play silence on XRUN */
    return Bse::Error::DEVICE_BUFFER;
  // if (snd_pcm_sw_params_set_xfer_align (phandle, sparams, 1) < 0) return Bse::Error::DEVICE_BUFFER;
  if (snd_pcm_sw_params (phandle, sparams) < 0)
    return Bse::Error::FILE_OPEN_FAILED;
  /* assign out values */
  *mix_freq = rate;
  *n_periodsp = nperiods;
  *period_sizep = period_size;
  /* dump debugging info */
  PDEBUG ("ALSA: setup: w=%d r=%d n_channels=%d sample_freq=%d nperiods=%u period=%u (%u) bufsz=%u",
          phandle == alsa->write_handle,
          phandle == alsa->read_handle,
          handle->n_channels,
          *mix_freq, *n_periodsp, *period_sizep,
          (guint) (nperiods * period_size),
          (guint) buffer_size);
  // snd_pcm_dump (phandle, snd_output);
  return Bse::Error::NONE;
}
static void
alsa_device_retrigger (AlsaPcmHandle *alsa)
{
  snd_lib_error_set_handler (silent_error_handler);
  PDEBUG ("ALSA: retriggering device (r=%s w=%s)...",
          !alsa->read_handle ? "<CLOSED>" : snd_pcm_state_name (snd_pcm_state (alsa->read_handle)),
          !alsa->write_handle ? "<CLOSED>" : snd_pcm_state_name (snd_pcm_state (alsa->write_handle)));
  snd_pcm_prepare (alsa->read_handle ? alsa->read_handle : alsa->write_handle);
  /* first, clear io buffers */
  if (alsa->read_handle)
    snd_pcm_drop (alsa->read_handle);
  if (alsa->write_handle)
    snd_pcm_drain (alsa->write_handle); /* write_handle must be blocking */
  /* prepare for playback/capture */
  gint aerror = snd_pcm_prepare (alsa->read_handle ? alsa->read_handle : alsa->write_handle);
  if (aerror)   /* this really shouldn't fail */
    Bse::info ("ALSA: failed to prepare for io: %s\n", snd_strerror (aerror));
  /* fill playback buffer with silence */
  if (alsa->write_handle)
    {
      gint n, buffer_length = alsa->n_periods * alsa->period_size; /* buffer size chosen by ALSA based on latency request */
      guint8 *silence = (guint8*) g_malloc0 (buffer_length * alsa->frame_size);
      do
        n = snd_pcm_writei (alsa->write_handle, silence, buffer_length);
      while (n == -EAGAIN); /* retry on signals */
      g_free (silence);
    }

  snd_lib_error_set_handler (NULL);
}

static gboolean
alsa_device_check_io (BsePcmHandle *handle,
                      glong        *timeoutp)
{
  AlsaPcmHandle *alsa = (AlsaPcmHandle*) handle;

  if (0)
    {
      snd_pcm_state_t ws = SND_PCM_STATE_DISCONNECTED, rs = SND_PCM_STATE_DISCONNECTED;
      snd_pcm_status_t *stat = alsa_alloca0 (snd_pcm_status);
      if (alsa->read_handle)
        {
          snd_pcm_status (alsa->read_handle, stat);
          rs = snd_pcm_state (alsa->read_handle);
        }
      guint rn = snd_pcm_status_get_avail (stat);
      if (alsa->write_handle)
        {
          snd_pcm_status (alsa->write_handle, stat);
          ws = snd_pcm_state (alsa->write_handle);
        }
      guint wn = snd_pcm_status_get_avail (stat);
      Bse::printerr ("ALSA: check_io: read=%4u/%4u (%s) write=%4u/%4u (%s) block=%u: %s\n",
                     rn, alsa->period_size * alsa->n_periods, snd_pcm_state_name (rs),
                     wn, alsa->period_size * alsa->n_periods, snd_pcm_state_name (ws),
                     handle->block_length,
                     rn >= handle->block_length ? "TRUE" : "FALSE");
    }

  /* quick check for data availability */
  gint n_frames_avail = snd_pcm_avail_update (alsa->read_handle ? alsa->read_handle : alsa->write_handle);
  if (n_frames_avail < 0 ||     /* error condition, probably an underrun (-EPIPE) */
      (n_frames_avail == 0 &&   /* check RUNNING state */
       snd_pcm_state (alsa->read_handle ? alsa->read_handle : alsa->write_handle) != SND_PCM_STATE_RUNNING))
    alsa_device_retrigger (alsa);
  if (n_frames_avail < (gint) handle->block_length)
    {
      /* not enough data? sync with hardware pointer */
      snd_pcm_hwsync (alsa->read_handle ? alsa->read_handle : alsa->write_handle);
      n_frames_avail = snd_pcm_avail_update (alsa->read_handle ? alsa->read_handle : alsa->write_handle);
      n_frames_avail = MAX (n_frames_avail, 0);
    }

  /* check whether data can be processed */
  if (n_frames_avail >= ssize_t (handle->block_length))
    return TRUE;        /* need processing */

  /* calculate timeout until processing is possible or needed */
  guint diff_frames = handle->block_length - n_frames_avail;
  *timeoutp = diff_frames * 1000 / handle->mix_freq;

  return FALSE;
}

static guint
alsa_device_latency (BsePcmHandle *handle)
{
  AlsaPcmHandle *alsa = (AlsaPcmHandle*) handle;
  snd_pcm_sframes_t rdelay, wdelay;
  if (!alsa->read_handle || snd_pcm_delay (alsa->read_handle, &rdelay) < 0)
    rdelay = 0;
  if (!alsa->write_handle || snd_pcm_delay (alsa->write_handle, &wdelay) < 0)
    wdelay = 0;
  gint buffer_length = alsa->n_periods * alsa->period_size; /* buffer size chosen by ALSA based on latency request */
  /* return total latency in frames */
  return CLAMP (rdelay, 0, buffer_length) + CLAMP (wdelay, 0, buffer_length);
}

static gsize
alsa_device_read (BsePcmHandle *handle,
                  gfloat       *values)
{
  AlsaPcmHandle *alsa = (AlsaPcmHandle*) handle;
  gpointer buf = alsa->period_buffer;
  gfloat *dest = values;
  gsize n_left = handle->block_length;
  const gsize n_values = n_left * handle->n_channels;

  alsa->read_write_count += 1;
  do
    {
      gsize n = MIN (alsa->period_size, n_left);
      gssize n_frames = snd_pcm_readi (alsa->read_handle, buf, n);
      if (n_frames < 0) /* errors during read, could be underrun (-EPIPE) */
        {
          PDEBUG ("ALSA: read() error: %s", snd_strerror (n_frames));
          snd_lib_error_set_handler (silent_error_handler);
          snd_pcm_prepare (alsa->read_handle);  /* force retrigger */
          snd_lib_error_set_handler (NULL);
          memset (buf, 0, n * alsa->frame_size);
          n_frames = n;
        }
      if (dest) /* ignore dummy reads() */
        {
          guint nv = n_frames * handle->n_channels;
          gsl_conv_to_float (GSL_WAVE_FORMAT_SIGNED_16, G_BYTE_ORDER, buf, dest, nv);
          dest += nv;
        }
      n_left -= n_frames;
    }
  while (n_left);

  return n_values;
}

static void
alsa_device_write (BsePcmHandle *handle,
                   const gfloat *values)
{
  AlsaPcmHandle *alsa = (AlsaPcmHandle*) handle;
  gpointer buf = alsa->period_buffer;                            /* size in bytes = n_values * 2 */
  const gfloat *floats = values;
  gsize n_left = handle->block_length;

  if (alsa->read_handle && alsa->read_write_count < 1)
    {
      snd_lib_error_set_handler (silent_error_handler); /* silence libALSA about -EPIPE */
      snd_pcm_forward (alsa->read_handle, handle->block_length);
      alsa->read_write_count += 1;
      snd_lib_error_set_handler (NULL);
    }

  alsa->read_write_count -= 1;
  do
    {
      gsize j = MIN (alsa->period_size, handle->block_length);  /* in frames */
      gsl_conv_from_float_clip (GSL_WAVE_FORMAT_SIGNED_16, G_BYTE_ORDER, floats, buf, j * handle->n_channels);
      floats += j * handle->n_channels;
      gssize n = 0;                                             /* in frames */
      do
        {
          if (n < 0)    /* errors during write, could be overrun (-EPIPE) */
            {
              PDEBUG ("ALSA: write() error: %s", snd_strerror (n));
              snd_lib_error_set_handler (silent_error_handler);
              snd_pcm_prepare (alsa->write_handle);  /* force retrigger */
              snd_lib_error_set_handler (NULL);
              return;
            }
          n = snd_pcm_writei (alsa->write_handle, buf, j);
        }
      while (n < 0);
      if (n < 0) /* sigh, errors during write? */
        n = j;
      n_left -= n;
    }
  while (n_left);
}

static void
bse_pcm_device_alsa_class_init (BsePcmDeviceALSAClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseDeviceClass *device_class = BSE_DEVICE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->finalize = bse_pcm_device_alsa_finalize;

  device_class->list_devices = bse_pcm_device_alsa_list_devices;
  const gchar *name = "alsa";
  const gchar *syntax = _("PLUGIN:CARD,DEV,SUBDEV");
  const gchar *info = g_intern_format (/* TRANSLATORS: keep this text to 70 chars in width */
                                       _("Advanced Linux Sound Architecture PCM driver, using\n"
                                         "ALSA %s.\n"
                                         "The device specification follows the ALSA device naming\n"
                                         "conventions, a detailed description of which is available\n"
                                         "at the project's website:\n"
                                         "    http://alsa-project.org/alsa-doc/alsa-lib/pcm.html\n"
                                         "Various ALSA plugins can be used here, 'hw' allows direct\n"
                                         "adressing of hardware channels by card, device and subdevice.\n"
                                         "  PLUGIN - the ALSA plugin, 'default' is usually good enough\n"
                                         "  CARD   - the card number for plugins like 'hw'\n"
                                         "  DEV    - the device number for plugins like 'hw'\n"
                                         "  SUBDEV - the subdevice number for plugins like 'hw'\n"),
                                       snd_asoundlib_version());
  bse_device_class_setup (klass, BSE_RATING_PREFERRED, name, syntax, info);
  device_class->open = bse_pcm_device_alsa_open;
  device_class->close = bse_pcm_device_alsa_close;
  int err = snd_output_stdio_attach (&snd_output, stderr, 0);
  (void) err;
}
