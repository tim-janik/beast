// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "driver.hh"
#include "bseengine.hh"
#include "internal.hh"
#include "gsldatautils.hh"

#define ADEBUG(...)             Bse::debug ("alsa", __VA_ARGS__)
#define alsa_alloca0(struc)     ({ struc##_t *ptr = (struc##_t*) alloca (struc##_sizeof()); memset (ptr, 0, struc##_sizeof()); ptr; })

#if __has_include(<alsa/asoundlib.h>)
#include <alsa/asoundlib.h>

// for non-little endian, SND_PCM_FORMAT_S16_LE ans other places will need fixups
static_assert (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__, "endianess unimplemented");

namespace Bse {

static void silent_error_handler (const char *file, int line, const char *function, int err, const char *fmt, ...) {}

class AlsaPcmDriver : public PcmDriver {
  snd_pcm_t    *read_handle_ = nullptr;
  snd_pcm_t    *write_handle_ = nullptr;
  uint          mix_freq_ = 0;
  uint          n_channels_ = 0;
  uint          n_periods_ = 0;
  uint          period_size_ = 0;       // count in frames
  int16        *period_buffer_ = nullptr;
  uint          read_write_count_ = 0;
public:
  explicit      AlsaPcmDriver (const String &devid) : PcmDriver (devid) {}
  static PcmDriverP
  create (const String &devid)
  {
    auto adriverp = std::make_shared<AlsaPcmDriver> (devid);
    return adriverp;
  }
  ~AlsaPcmDriver()
  {
    if (read_handle_)
      snd_pcm_close (read_handle_);
    if (write_handle_)
      snd_pcm_close (write_handle_);
    delete[] period_buffer_;
  }
  virtual float
  pcm_frequency () const override
  {
    return mix_freq_;
  }
  virtual void
  close () override
  {
    assert_return (opened());
    if (read_handle_)
      {
        snd_pcm_drop (read_handle_);
        snd_pcm_close (read_handle_);
        read_handle_ = nullptr;
      }
    if (write_handle_)
      {
        snd_pcm_nonblock (write_handle_, 0);
        snd_pcm_drain (write_handle_);
        snd_pcm_close (write_handle_);
        write_handle_ = nullptr;
      }
    delete[] period_buffer_;
    period_buffer_ = nullptr;
    flags_ &= ~size_t (Flags::OPENED | Flags::READABLE | Flags::WRITABLE);
  }
  virtual Error
  open (const DriverConfig &config) override
  {
    assert_return (!opened(), Error::INTERNAL);
    int aerror = 0;
    // setup request
    flags_ |= Flags::READABLE * config.require_readable;
    flags_ |= Flags::WRITABLE * config.require_writable;
    n_channels_ = config.n_channels;
    // try open
    snd_lib_error_set_handler (silent_error_handler);
    if (!aerror && config.require_readable)
      aerror = snd_pcm_open (&read_handle_, devid_.c_str(), SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
    if (!aerror && config.require_writable)
      aerror = snd_pcm_open (&write_handle_, devid_.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
    snd_lib_error_set_handler (NULL);
    // try setup
    const uint period_size = config.block_length;
    Error error = !aerror ? Error::NONE : bse_error_from_errno (-aerror, Error::FILE_OPEN_FAILED);
    uint rh_freq = config.mix_freq, rh_n_periods = 2, rh_period_size = period_size;
    if (!aerror && read_handle_)
      error = alsa_device_setup (read_handle_, config.latency_ms, &rh_freq, &rh_n_periods, &rh_period_size);
    uint wh_freq = config.mix_freq, wh_n_periods = 2, wh_period_size = period_size;
    if (!aerror && write_handle_)
      error = alsa_device_setup (write_handle_, config.latency_ms, &wh_freq, &wh_n_periods, &wh_period_size);
    // check duplex
    if (error == 0 && read_handle_ && write_handle_)
      {
        const bool linked = snd_pcm_link (read_handle_, write_handle_) == 0;
        if (rh_freq != wh_freq || rh_n_periods != wh_n_periods || rh_period_size != wh_period_size || !linked)
          error = Error::DEVICES_MISMATCH;
        ADEBUG ("ALSA: %s: %s: %f==%f && %d==%d && %d==%d && linked==%d", devid_,
                error != 0 ? "MISMATCH" : "LINKED", rh_freq, wh_freq, rh_n_periods, wh_n_periods, rh_period_size, wh_period_size, linked);
      }
    mix_freq_ = read_handle_ ? rh_freq : wh_freq;
    n_periods_ = read_handle_ ? rh_n_periods : wh_n_periods;
    period_size_ = read_handle_ ? rh_period_size : wh_period_size;
    if (error == 0 && snd_pcm_prepare (read_handle_ ? read_handle_ : write_handle_) < 0)
      error = Error::FILE_OPEN_FAILED;
    // finish opening or shutdown
    if (error == 0)
      {
        period_buffer_ = new int16[period_size_ * n_channels_];
        flags_ |= Flags::OPENED;
      }
    else
      {
        if (read_handle_)
          snd_pcm_close (read_handle_);
        read_handle_ = nullptr;
        if (write_handle_)
          snd_pcm_close (write_handle_);
        write_handle_ = nullptr;
      }
    ADEBUG ("ALSA: opening PCM \"%s\" readable=%d writable=%d: %s", devid_, config.require_readable, config.require_writable, bse_error_blurb (error));
    return error;
  }
  Error
  alsa_device_setup (snd_pcm_t *phandle, uint latency_ms, uint *mix_freq, uint *n_periodsp, uint *period_sizep)
  {
    // turn on blocking behaviour since we may end up in read() with an unfilled buffer
    if (snd_pcm_nonblock (phandle, 0) < 0)
      return Error::FILE_OPEN_FAILED;
    // setup hardware configuration
    snd_pcm_hw_params_t *hparams = alsa_alloca0 (snd_pcm_hw_params);
    if (snd_pcm_hw_params_any (phandle, hparams) < 0)
      return Error::FILE_OPEN_FAILED;
    if (snd_pcm_hw_params_set_channels (phandle, hparams, n_channels_) < 0)
      return Error::DEVICE_CHANNELS;
    if (snd_pcm_hw_params_set_access (phandle, hparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
      return Error::DEVICE_FORMAT;
    if (snd_pcm_hw_params_set_format (phandle, hparams, SND_PCM_FORMAT_S16_LE) < 0)
      return Error::DEVICE_FORMAT;
    uint rate = *mix_freq;
    if (snd_pcm_hw_params_set_rate (phandle, hparams, rate, 0) < 0 || rate != *mix_freq)
      return Error::DEVICE_FREQUENCY;
    snd_pcm_uframes_t period_size = *period_sizep;
    if (snd_pcm_hw_params_set_period_size (phandle, hparams, period_size, 0) < 0)
      return Error::DEVICE_LATENCY;
    uint buffer_time_us = latency_ms * 1000;
    if (snd_pcm_hw_params_set_buffer_time_near (phandle, hparams, &buffer_time_us, NULL) < 0)
      return Error::DEVICE_LATENCY;
    if (snd_pcm_hw_params (phandle, hparams) < 0)
      return Error::FILE_OPEN_FAILED;
    // verify hardware settings
    uint nperiods = 0;
    if (snd_pcm_hw_params_get_periods (hparams, &nperiods, NULL) < 0 || nperiods < 2)
      return Error::DEVICE_BUFFER;
    snd_pcm_uframes_t buffer_size = 0;
    if (snd_pcm_hw_params_get_buffer_size (hparams, &buffer_size) < 0 || buffer_size != nperiods * period_size)
      return Error::DEVICE_BUFFER;
    // setup software configuration
    snd_pcm_sw_params_t *sparams = alsa_alloca0 (snd_pcm_sw_params);
    if (snd_pcm_sw_params_current (phandle, sparams) < 0)
      return Error::FILE_OPEN_FAILED;
    if (snd_pcm_sw_params_set_start_threshold (phandle, sparams, (buffer_size / period_size) * period_size) < 0)
      return Error::DEVICE_BUFFER;
    if (snd_pcm_sw_params_set_avail_min (phandle, sparams, period_size) < 0)
      return Error::DEVICE_LATENCY;
    snd_pcm_uframes_t boundary = 0;
    if (snd_pcm_sw_params_get_boundary (sparams, &boundary) < 0)
      return Error::FILE_OPEN_FAILED;
    bool stop_on_xrun = false;          // ignore XRUN
    uint threshold = stop_on_xrun ? buffer_size : MIN (buffer_size * 2, boundary);
    // constrain boundary for stop_threshold, to work around 64bit alsa lib setting boundary to 0x5000000000000000
    if (snd_pcm_sw_params_set_stop_threshold (phandle, sparams, threshold) < 0)
      return Error::DEVICE_BUFFER;
    if (snd_pcm_sw_params_set_silence_threshold (phandle, sparams, 0) < 0 ||
        snd_pcm_sw_params_set_silence_size (phandle, sparams, boundary) < 0)    // play silence on XRUN
      return Error::DEVICE_BUFFER;
    // if (snd_pcm_sw_params_set_xfer_align (phandle, sparams, 1) < 0) return Error::DEVICE_BUFFER;
    if (snd_pcm_sw_params (phandle, sparams) < 0)
      return Error::FILE_OPEN_FAILED;
    // return values
    *mix_freq = rate;
    *n_periodsp = nperiods;
    *period_sizep = period_size;
    ADEBUG ("ALSA: setup: r=%d w=%d n_channels=%d sample_freq=%d nperiods=%u period=%u (%u) bufsz=%u",
            phandle == read_handle_, phandle == write_handle_,
            n_channels_, *mix_freq, *n_periodsp, *period_sizep,
            nperiods * period_size, buffer_size);
    // snd_pcm_dump (phandle, snd_output);
    return Error::NONE;
  }
  void
  pcm_retrigger ()
  {
    snd_lib_error_set_handler (silent_error_handler);
    ADEBUG ("ALSA: retriggering device (r=%s w=%s)...",
            !read_handle_ ? "<CLOSED>" : snd_pcm_state_name (snd_pcm_state (read_handle_)),
            !write_handle_ ? "<CLOSED>" : snd_pcm_state_name (snd_pcm_state (write_handle_)));
    snd_pcm_prepare (read_handle_ ? read_handle_ : write_handle_);
    // first, clear io buffers
    if (read_handle_)
      snd_pcm_drop (read_handle_);
    if (write_handle_)
      snd_pcm_drain (write_handle_); // write_handle_ must be blocking
    // prepare for playback/capture
    int aerror = snd_pcm_prepare (read_handle_ ? read_handle_ : write_handle_);
    if (aerror)   // this really should not fail
      Bse::info ("ALSA: failed to prepare for io: %s\n", snd_strerror (aerror));
    // fill playback buffer with silence
    if (write_handle_)
      {
        int n, buffer_length = n_periods_ * period_size_; // buffer size chosen by ALSA based on latency request
        const float *zeros = bse_engine_const_zeros (buffer_length / 2); // sizeof (int16) / sizeof (float)
        do
          n = snd_pcm_writei (write_handle_, zeros, buffer_length);
        while (n == -EAGAIN); // retry on signals
      }
    snd_lib_error_set_handler (NULL);
  }
  virtual bool
  pcm_check_io (long *timeoutp) override
  {
    if (0)
      {
        snd_pcm_state_t ws = SND_PCM_STATE_DISCONNECTED, rs = SND_PCM_STATE_DISCONNECTED;
        snd_pcm_status_t *stat = alsa_alloca0 (snd_pcm_status);
        if (read_handle_)
          {
            snd_pcm_status (read_handle_, stat);
            rs = snd_pcm_state (read_handle_);
          }
        uint rn = snd_pcm_status_get_avail (stat);
        if (write_handle_)
          {
            snd_pcm_status (write_handle_, stat);
            ws = snd_pcm_state (write_handle_);
          }
        uint wn = snd_pcm_status_get_avail (stat);
        printerr ("ALSA: check_io: read=%4u/%4u (%s) write=%4u/%4u (%s) block=%u: %s\n",
                  rn, period_size_ * n_periods_, snd_pcm_state_name (rs),
                  wn, period_size_ * n_periods_, snd_pcm_state_name (ws),
                  period_size_, rn >= period_size_ ? "true" : "false");
      }
    // quick check for data availability
    int n_frames_avail = snd_pcm_avail_update (read_handle_ ? read_handle_ : write_handle_);
    if (n_frames_avail < 0 ||   // error condition, probably an underrun (-EPIPE)
        (n_frames_avail == 0 && // check RUNNING state
         snd_pcm_state (read_handle_ ? read_handle_ : write_handle_) != SND_PCM_STATE_RUNNING))
      pcm_retrigger();
    if (n_frames_avail < period_size_)
      {
        // not enough data? sync with hardware pointer
        snd_pcm_hwsync (read_handle_ ? read_handle_ : write_handle_);
        n_frames_avail = snd_pcm_avail_update (read_handle_ ? read_handle_ : write_handle_);
        n_frames_avail = MAX (n_frames_avail, 0);
      }
    // check whether data can be processed
    if (n_frames_avail >= period_size_)
      return true;      // need processing
    // calculate timeout until processing is possible or needed
    const uint diff_frames = period_size_ - n_frames_avail;
    *timeoutp = diff_frames * 1000 / mix_freq_;
    return false;
  }
  virtual uint
  pcm_latency () const override
  {
    snd_pcm_sframes_t rdelay, wdelay;
    if (!read_handle_ || snd_pcm_delay (read_handle_, &rdelay) < 0)
      rdelay = 0;
    if (!write_handle_ || snd_pcm_delay (write_handle_, &wdelay) < 0)
      wdelay = 0;
    const int buffer_length = n_periods_ * period_size_; // buffer size chosen by ALSA based on latency request
    // return total latency in frames
    return CLAMP (rdelay, 0, buffer_length) + CLAMP (wdelay, 0, buffer_length);
  }
  virtual size_t
  pcm_read (size_t n, float *values) override
  {
    assert_return (n == period_size_ * n_channels_, 0);
    float *dest = values;
    size_t n_left = period_size_;
    const size_t n_values = n_left * n_channels_;

    read_write_count_ += 1;
    do
      {
        ssize_t n_frames = snd_pcm_readi (read_handle_, period_buffer_, n_left);
        if (n_frames < 0) // errors during read, could be underrun (-EPIPE)
          {
            ADEBUG ("ALSA: read() error: %s", snd_strerror (n_frames));
            snd_lib_error_set_handler (silent_error_handler);
            snd_pcm_prepare (read_handle_);     // force retrigger
            snd_lib_error_set_handler (NULL);
            n_frames = n_left;
            const size_t frame_size = n_channels_ * sizeof (period_buffer_[0]);
            memset (period_buffer_, 0, n_frames * frame_size);
          }
        if (dest) // ignore dummy reads()
          {
            gsl_conv_to_float (GSL_WAVE_FORMAT_SIGNED_16, G_BYTE_ORDER, period_buffer_, dest, n_frames * n_channels_);
            dest += n_frames * n_channels_;
          }
        n_left -= n_frames;
      }
    while (n_left);

    return n_values;
  }
  virtual void
  pcm_write (size_t n, const float *values) override
  {
    assert_return (n == period_size_ * n_channels_);
    if (read_handle_ && read_write_count_ < 1)
      {
        snd_lib_error_set_handler (silent_error_handler); // silence ALSA about -EPIPE
        snd_pcm_forward (read_handle_, period_size_);
        snd_lib_error_set_handler (NULL);
        read_write_count_ += 1;
      }
    read_write_count_ -= 1;
    const float *floats = values;
    size_t n_left = period_size_;       // in frames
    while (n_left)
      {
        gsl_conv_from_float_clip (GSL_WAVE_FORMAT_SIGNED_16, G_BYTE_ORDER, floats, period_buffer_, n_left * n_channels_);
        floats += n_left * n_channels_;
        ssize_t n = 0;                  // in frames
        n = snd_pcm_writei (write_handle_, period_buffer_, n_left);
        if (n < 0)                      // errors during write, could be overrun (-EPIPE)
          {
            ADEBUG ("ALSA: write() error: %s", snd_strerror (n));
            snd_lib_error_set_handler (silent_error_handler);
            snd_pcm_prepare (write_handle_);    // force retrigger
            snd_lib_error_set_handler (NULL);
            return;
          }
        n_left -= n;
      }
  }
};

static snd_output_t *snd_output = nullptr; // used for debugging

static String
chars2string (const char *s)
{
  return s ? s : "";
}

static String
cxfree (char *mallocedstring, const char *fallback = "")
{
  if (mallocedstring)
    {
      const String string = mallocedstring;
      free (mallocedstring);
      return string;
    }
  return fallback;
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

static const char*
pcm_class_name (snd_pcm_class_t pcmclass)
{
  switch (pcmclass)
    {
    case SND_PCM_CLASS_MULTI:     return "Multichannel";
    case SND_PCM_CLASS_MODEM:     return "Modem";
    case SND_PCM_CLASS_DIGITIZER: return "Digitizer";
    default:
    case SND_PCM_CLASS_GENERIC:   return "Standard";
    }
}

static void
list_alsa_drivers (Driver::EntryVec &entries, uint32 driverid)
{
  static const bool BSE_USED initialized = [] {
    return snd_output_stdio_attach (&snd_output, stderr, 0);
  } ();
  // discover virtual (non-hw) devices
  bool seen_plughw = false; // maybe needed to resample at device boundaries
  void **nhints = nullptr;
  if (snd_device_name_hint (-1, "pcm", &nhints) >= 0)
    {
      String name, desc, ioid;
      for (void **hint = nhints; *hint; hint++)
        {
          name = cxfree (snd_device_name_get_hint (*hint, "NAME"));           // full ALSA device name
          desc = cxfree (snd_device_name_get_hint (*hint, "DESC"));           // card_name + pcm_name + alsa.conf-description
          ioid = cxfree (snd_device_name_get_hint (*hint, "IOID"), "Duplex"); // one of: "Duplex", "Input", "Output"
          seen_plughw = seen_plughw || 0; // FIXME: strncmp (name.c_str(), "plughw:", 7) == 0;
          if (name == "pulse")
            {
              ADEBUG ("HINT: %s (%s) - %s", name, ioid, substitute_string ("\n", " ", desc));
              Driver::Entry entry;
              entry.devid = name;
              entry.name = desc;
              entry.blurb = "Warning: PulseAudio routing is not realtime capable";
              entry.readonly = "Input" == ioid;
              entry.writeonly = "Output" == ioid;
              entry.priority = Driver::PULSE;
              entry.driverid = driverid;
              entries.push_back (entry);
            }
        }
      snd_device_name_free_hint (nhints);
    }
  // discover PCM hardware
  snd_ctl_card_info_t *cinfo = alsa_alloca0 (snd_ctl_card_info);
  snd_pcm_info_t *pinfo = alsa_alloca0 (snd_pcm_info);
  snd_pcm_info_t *rinfo = alsa_alloca0 (snd_pcm_info);
  int cindex = -1;
  while (snd_card_next (&cindex) == 0 && cindex >= 0)
    {
      snd_ctl_card_info_clear (cinfo);
      snd_ctl_t *chandle = nullptr;
      if (snd_ctl_open (&chandle, string_format ("hw:CARD=%u", cindex).c_str(), SND_CTL_NONBLOCK) < 0 || !chandle)
        continue;
      if (snd_ctl_card_info (chandle, cinfo) < 0)
        {
          snd_ctl_close (chandle);
          continue;
        }
      const String card_id = chars2string (snd_ctl_card_info_get_id (cinfo));
      const String card_driver = chars2string (snd_ctl_card_info_get_driver (cinfo));
      const String card_name = chars2string (snd_ctl_card_info_get_name (cinfo));
      const String card_longname = chars2string (snd_ctl_card_info_get_longname (cinfo));
      const String card_mixername = chars2string (snd_ctl_card_info_get_mixername (cinfo));
      ADEBUG ("CARD: %s - %s - %s [%s] - %s", card_id, card_driver, card_name, card_mixername, card_longname);
      int pindex = -1;
      while (snd_ctl_pcm_next_device (chandle, &pindex) == 0 && pindex >= 0)
        {
          snd_pcm_info_set_device (pinfo, pindex);
          snd_pcm_info_set_subdevice (pinfo, 0);
          snd_pcm_info_set_stream (pinfo, SND_PCM_STREAM_PLAYBACK);
          const bool writable = snd_ctl_pcm_info (chandle, pinfo) == 0;
          snd_pcm_info_set_device (rinfo, pindex);
          snd_pcm_info_set_subdevice (rinfo, 0);
          snd_pcm_info_set_stream (rinfo, SND_PCM_STREAM_CAPTURE);
          const bool readable = snd_ctl_pcm_info (chandle, rinfo) == 0;
          const auto pcmclass = snd_pcm_info_get_class (writable ? pinfo : rinfo);
          if (!writable && !readable)
            continue;
          const int total_playback_subdevices = writable ? snd_pcm_info_get_subdevices_count (pinfo) : 0;
          const int avail_playback_subdevices = writable ? snd_pcm_info_get_subdevices_avail (pinfo) : 0;
          String pdevs, rdevs;
          if (total_playback_subdevices && total_playback_subdevices != avail_playback_subdevices)
            pdevs = string_format ("%u*playback (%u busy)", total_playback_subdevices, total_playback_subdevices - avail_playback_subdevices);
          else if (total_playback_subdevices)
            pdevs = string_format ("%u*playback", total_playback_subdevices);
          const int total_capture_subdevices = readable ? snd_pcm_info_get_subdevices_count (rinfo) : 0;
          const int avail_capture_subdevices = readable ? snd_pcm_info_get_subdevices_avail (rinfo) : 0;
          if (total_capture_subdevices && total_capture_subdevices != avail_capture_subdevices)
            rdevs = string_format ("%u*capture (%u busy)", total_capture_subdevices, total_capture_subdevices - avail_capture_subdevices);
          else if (total_capture_subdevices)
            rdevs = string_format ("%u*capture", total_capture_subdevices);
          const String joiner = !pdevs.empty() && !rdevs.empty() ? " + " : "";
          Driver::Entry entry; // alsa=hw:CARD=PCH,DEV=0
          entry.devid = string_format ("%s:CARD=%s,DEV=%u", seen_plughw ? "plughw" : "hw", card_id, pindex);
          entry.name = chars2string (snd_pcm_info_get_name (writable ? pinfo : rinfo));
          entry.name += " - " + card_name;
          if (card_name != card_mixername && !card_mixername.empty())
            entry.name += " [" + card_mixername + "]";
          entry.status = pcm_class_name (pcmclass) + String (" ") + (readable && writable ? "Duplex" : readable ? "Input" : "Output");
          entry.status += " " + pdevs + joiner + rdevs;
          entry.blurb = card_longname;
          entry.readonly = !writable;
          entry.writeonly = !readable;
          entry.priority = Driver::ALSA + Driver::WCARD * cindex + Driver::WDEV * pindex;
          entry.driverid = driverid;
          entries.push_back (entry);
        }
      snd_ctl_close (chandle);
    }
}

static const uint32 alsa_driverid = PcmDriver::register_driver (AlsaPcmDriver::create, list_alsa_drivers);

} // Bse

#endif  // __has_include(<alsa/asoundlib.h>)
