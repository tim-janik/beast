// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "driver.hh"
#include "bseengine.hh"
#include "internal.hh"
#include "gsldatautils.hh"
#include "bsesequencer.hh"
#include "bsemididecoder.hh"

#define ADEBUG(...)             Bse::debug ("alsa", __VA_ARGS__)
#define alsa_alloca0(struc)     ({ struc##_t *ptr = (struc##_t*) alloca (struc##_sizeof()); memset (ptr, 0, struc##_sizeof()); ptr; })
#define return_error(reason, ERRMEMB) do {      \
  ADEBUG ("PCM: %s: %s: %s",                    \
    alsadev_, reason,                           \
    bse_error_blurb (Bse::Error::ERRMEMB));     \
  return Bse::Error::ERRMEMB;                   \
  } while (0)

/* Notes on the ALSA API:
 * - Increase buffer_size to minimize dropouts.
 * - Decrease buffer_size to minimize latency.
 * - The start threshold specifies that the device starts if that many frames become available for playback.
 * - The stop threshold specifies that the device stops if that many frames become available for write.
 *   Contrary to the ALSA documentation, stop_threshold=boundary fails to keep the device running on
 *   underruns (on 64bit, on 32bit libasound it used to work), but LONG_MAX or 2*buffer_size will do.
 * - For all practical purposes, snd_pcm_sw_params_get_boundary() represents "+Infinity" in the ALSA API.
 *   It's normally the largest multiple of the buffer size that fits LONG_MAX, but may be broken on 64bit.
 * - The avail_min specifies the application wakeup point if that many (free) frames become available,
 *   many cards only support powers of 2.
 * - The "sub unit direction" indicates rounding direction, ALSA refuses to set exact values (dir=0)
 *   if the underlying device has a fractional offset (for e.g. period size). This usually happens when
 *   the plughw device is used for remixing, e.g. 256 * 44100/48000 = 235.2.
 * - Due to fractional period sizes, the effective buffer size can differ from n_periods * period_size,
 *   so the desired buffer size is best set via set_period_size_near + set_periods_near.
 * - With silence_threshold=period_size and silence_size=period_size, an extra 0-value filled period
 *   is inserted while the last period is played. This causes clicks early but can reduce the risk of
 *   future underruns.
 * Driver implementation:
 * - During high load, we need 1 period playing while rendering the next period.
 * - Due to jitter, we might be late writing the next period, so we're better off with one extra period.
 * - If we're very fast generating periods, we ideally have enough buffer space to write without blocking.
 * Thus, we need 1 playing period, 1 extra period, 1 unfilled period, i.e. 3 periods of buffer size and
 * we need avail_min to match the period size.
 */

#if __has_include(<alsa/asoundlib.h>)
#include <alsa/asoundlib.h>

// for non-little endian, SND_PCM_FORMAT_S16_LE and other places will need fixups
static_assert (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__, "endianess unimplemented");

namespace Bse {

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

static void
silent_error_handler (const char *file, int line, const char *function, int err, const char *fmt, ...)
{}

static snd_output_t *snd_output = nullptr; // used for debugging

static void
init_lib_alsa()
{
  static const bool BSE_USED initialized = [] {
    return snd_output_stdio_attach (&snd_output, stderr, 0);
  } ();
}

static void
list_alsa_drivers (Driver::EntryVec &entries, bool need_pcm, bool need_rawmidi)
{
  init_lib_alsa();
  // discover virtual (non-hw) devices
  bool seen_plughw = false; // maybe needed to resample at device boundaries
  void **nhints = nullptr;
  if (need_pcm && snd_device_name_hint (-1, "pcm", &nhints) >= 0)
    {
      String name, desc, ioid;
      for (void **hint = nhints; *hint; hint++)
        {
          name = cxfree (snd_device_name_get_hint (*hint, "NAME"));           // full ALSA device name
          desc = cxfree (snd_device_name_get_hint (*hint, "DESC"));           // card_name + pcm_name + alsa.conf-description
          ioid = cxfree (snd_device_name_get_hint (*hint, "IOID"), "Duplex"); // one of: "Duplex", "Input", "Output"
          seen_plughw = seen_plughw || strncmp (name.c_str(), "plughw:", 7) == 0;
          if (name == "pulse")
            {
              ADEBUG ("DISCOVER: %s - %s - %s", name, ioid, substitute_string ("\n", " ", desc));
              Driver::Entry entry;
              entry.devid = name;
              entry.device_name = desc;
              entry.device_info = "Routing via the PulseAudio sound system";
              entry.notice = "Note: PulseAudio routing is not realtime capable";
              entry.readonly = "Input" == ioid;
              entry.writeonly = "Output" == ioid;
              entry.priority = Driver::PULSE;
              entries.push_back (entry);
            }
        }
      snd_device_name_free_hint (nhints);
    }
  // discover hardware cards
  snd_ctl_card_info_t *cinfo = alsa_alloca0 (snd_ctl_card_info);
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
      // ADEBUG ("DISCOVER: CARD: %s - %s - %s [%s] - %s", card_id, card_driver, card_name, card_mixername, card_longname);
      // discover PCM hardware
      snd_pcm_info_t *wpi = !need_pcm ? nullptr : alsa_alloca0 (snd_pcm_info);
      snd_pcm_info_t *rpi = !need_pcm ? nullptr : alsa_alloca0 (snd_pcm_info);
      int dindex = -1;
      while (need_pcm && snd_ctl_pcm_next_device (chandle, &dindex) == 0 && dindex >= 0)
        {
          snd_pcm_info_set_device (wpi, dindex);
          snd_pcm_info_set_subdevice (wpi, 0);
          snd_pcm_info_set_stream (wpi, SND_PCM_STREAM_PLAYBACK);
          const bool writable = snd_ctl_pcm_info (chandle, wpi) == 0;
          snd_pcm_info_set_device (rpi, dindex);
          snd_pcm_info_set_subdevice (rpi, 0);
          snd_pcm_info_set_stream (rpi, SND_PCM_STREAM_CAPTURE);
          const bool readable = snd_ctl_pcm_info (chandle, rpi) == 0;
          const auto pcmclass = snd_pcm_info_get_class (writable ? wpi : rpi);
          if (!writable && !readable)
            continue;
          const int total_playback_subdevices = writable ? snd_pcm_info_get_subdevices_count (wpi) : 0;
          const int avail_playback_subdevices = writable ? snd_pcm_info_get_subdevices_avail (wpi) : 0;
          String wdevs, rdevs;
          if (total_playback_subdevices && total_playback_subdevices != avail_playback_subdevices)
            wdevs = string_format ("%u*playback (%u busy)", total_playback_subdevices, total_playback_subdevices - avail_playback_subdevices);
          else if (total_playback_subdevices)
            wdevs = string_format ("%u*playback", total_playback_subdevices);
          const int total_capture_subdevices = readable ? snd_pcm_info_get_subdevices_count (rpi) : 0;
          const int avail_capture_subdevices = readable ? snd_pcm_info_get_subdevices_avail (rpi) : 0;
          if (total_capture_subdevices && total_capture_subdevices != avail_capture_subdevices)
            rdevs = string_format ("%u*capture (%u busy)", total_capture_subdevices, total_capture_subdevices - avail_capture_subdevices);
          else if (total_capture_subdevices)
            rdevs = string_format ("%u*capture", total_capture_subdevices);
          const String joiner = !wdevs.empty() && !rdevs.empty() ? " + " : "";
          Driver::Entry entry;
          entry.devid = string_format ("hw:CARD=%s,DEV=%u", card_id, dindex);
          const bool is_usb = string_startswith (chars2string (snd_pcm_info_get_id (writable ? wpi : rpi)), "USB Audio");
          entry.device_name = chars2string (snd_pcm_info_get_name (writable ? wpi : rpi));
          entry.device_name += " - " + card_name;
          if (card_name != card_mixername && !card_mixername.empty())
            entry.device_name += " [" + card_mixername + "]";
          if (pcmclass == SND_PCM_CLASS_GENERIC)
            entry.capabilities = readable && writable ? "Full-Duplex Audio" : readable ? "Audio Input" : "Audio Output";
          else // pcmclass == SND_PCM_CLASS_MODEM // other SND_PCM_CLASS_ types are unused
            entry.capabilities = readable && writable ? "Full-Duplex Modem" : readable ? "Modem Input" : "Modem Output";
          entry.capabilities += ", streams: " + wdevs + joiner + rdevs;
          if (!string_startswith (card_longname, card_name + " at "))
            entry.device_info = card_longname;
          entry.readonly = !writable;
          entry.writeonly = !readable;
          entry.modem = pcmclass == SND_PCM_CLASS_MODEM;
          entry.priority = (is_usb ? Driver::ALSA_USB : Driver::ALSA_KERN) + Driver::WCARD * cindex + Driver::WDEV * dindex;
          entries.push_back (entry);
          ADEBUG ("DISCOVER: PCM: %s - %s", entry.devid, entry.device_name);
        }
      // discover MIDI hardware
      snd_rawmidi_info_t *minfo = !need_rawmidi ? nullptr : alsa_alloca0 (snd_rawmidi_info);
      dindex = -1;
      while (need_rawmidi && snd_ctl_rawmidi_next_device (chandle, &dindex) == 0 && dindex >= 0)
        {
          snd_rawmidi_info_set_device (minfo, dindex);
          uint total_subdevs = 0;
          snd_rawmidi_info_set_stream (minfo, SND_RAWMIDI_STREAM_INPUT);
          if (snd_ctl_rawmidi_info (chandle, minfo) >= 0)
            total_subdevs = std::max (total_subdevs, snd_rawmidi_info_get_subdevices_count (minfo));
          snd_rawmidi_info_set_stream (minfo, SND_RAWMIDI_STREAM_OUTPUT);
          if (snd_ctl_rawmidi_info (chandle, minfo) >= 0)
            total_subdevs = std::max (total_subdevs, snd_rawmidi_info_get_subdevices_count (minfo));
          for (uint subdev = 0; subdev < total_subdevs; subdev += 1)
            {
              snd_rawmidi_info_set_subdevice (minfo, subdev);
              snd_rawmidi_info_set_stream (minfo, SND_RAWMIDI_STREAM_INPUT);
              const bool readable = snd_ctl_rawmidi_info (chandle, minfo) == 0;
              bool writable = false;
              if (!readable)
                {
                  snd_rawmidi_info_set_stream (minfo, SND_RAWMIDI_STREAM_OUTPUT);
                  if (snd_ctl_rawmidi_info (chandle, minfo) < 0)
                    continue;
                  writable = true;
                }
              Driver::Entry entry;
              if (total_subdevs > 1)
                entry.devid = string_format ("hw:CARD=%s,DEV=%u,SUBDEV=%u", card_id, dindex, subdev);
              else
                entry.devid = string_format ("hw:CARD=%s,DEV=%u", card_id, dindex);
              entry.device_name = chars2string (snd_rawmidi_info_get_subdevice_name (minfo));
              entry.device_name += entry.device_name.empty() ? string_format ("Midi#%u - ", subdev) : " - ";
              entry.device_name += chars2string (snd_rawmidi_info_get_name (minfo));
              entry.device_name += " [" + chars2string (snd_rawmidi_info_get_id (minfo)) + "]";
              entry.device_info = card_longname;
              if (!writable)
                {
                  snd_rawmidi_info_set_stream (minfo, SND_RAWMIDI_STREAM_OUTPUT);
                  writable = snd_ctl_rawmidi_info (chandle, minfo) == 0;
                }
              const String joiner = readable && writable ? " + " : "";
              entry.capabilities = (readable ? "MIDI Input" : "") + joiner + (writable ? "MIDI Output" : "");
              entry.readonly = !writable;
              entry.writeonly = !readable;
              entry.priority = Driver::ALSA_KERN + Driver::WCARD * cindex + Driver::WDEV * dindex + Driver::WSUB * subdev;
              entries.push_back (entry);
              ADEBUG ("DISCOVER: RawMIDI: %s - %s", entry.devid, entry.device_name);
            }
        }
      snd_ctl_close (chandle);
    }
}

// == AlsaPcmDriver ==
class AlsaPcmDriver : public PcmDriver {
  snd_pcm_t    *read_handle_ = nullptr;
  snd_pcm_t    *write_handle_ = nullptr;
  uint          mix_freq_ = 0;
  uint          block_size_ = 0;
  uint          n_channels_ = 0;
  uint          n_periods_ = 0;
  uint          period_size_ = 0;       // count in frames
  int16        *period_buffer_ = nullptr;
  uint          read_write_count_ = 0;
  String        alsadev_;
public:
  explicit      AlsaPcmDriver (const String &devid) : PcmDriver (devid) {}
  static PcmDriverP
  create (const String &devid)
  {
    auto pdriverp = std::make_shared<AlsaPcmDriver> (devid);
    return pdriverp;
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
  virtual uint
  block_length () const override
  {
    return block_size_;
  }
  virtual void
  close () override
  {
    assert_return (opened());
    ADEBUG ("PCM: %s: CLOSE: r=%d w=%d", alsadev_, !!read_handle_, !!write_handle_);
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
    alsadev_ = "";
  }
  virtual Error
  open (IODir iodir, const PcmDriverConfig &config) override
  {
    Error error = open (devid_, iodir, config);
    if (error != Error::NONE && strncmp ("hw:", devid_.c_str(), 3) == 0)
      error = open ("plug" + devid_, iodir, config);
    return error;
  }
  Error
  open (const String &alsadev, IODir iodir, const PcmDriverConfig &config)
  {
    assert_return (!opened(), Error::INTERNAL);
    int aerror = 0;
    alsadev_ = alsadev;
    // setup request
    const bool require_readable = iodir == READONLY || iodir == READWRITE;
    const bool require_writable = iodir == WRITEONLY || iodir == READWRITE;
    flags_ |= Flags::READABLE * require_readable;
    flags_ |= Flags::WRITABLE * require_writable;
    n_channels_ = config.n_channels;
    // try open
    snd_lib_error_set_handler (silent_error_handler);
    if (!aerror && require_readable)
      aerror = snd_pcm_open (&read_handle_, alsadev_.c_str(), SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
    if (!aerror && require_writable)
      aerror = snd_pcm_open (&write_handle_, alsadev_.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
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
        ADEBUG ("PCM: %s: %s: %f==%f && %d*%d==%d*%d && linked==%d", alsadev_,
                error != 0 ? "MISMATCH" : "LINKED", rh_freq, wh_freq, rh_n_periods, rh_period_size, wh_n_periods, wh_period_size, linked);
      }
    mix_freq_ = read_handle_ ? rh_freq : wh_freq;
    block_size_ = read_handle_ ? rh_period_size : wh_period_size;
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
    ADEBUG ("PCM: %s: opening readable=%d writable=%d: %s", alsadev_, readable(), writable(), bse_error_blurb (error));
    if (error != 0)
      alsadev_ = "";
    return error;
  }
  Error
  alsa_device_setup (snd_pcm_t *phandle, uint latency_ms, uint *mix_freq, uint *n_periodsp, uint *period_sizep)
  {
    // turn on blocking behaviour since we may end up in read() with an unfilled buffer
    if (snd_pcm_nonblock (phandle, 0) < 0)
      return_error ("snd_pcm_nonblock", FILE_OPEN_FAILED);
    // setup hardware configuration
    snd_pcm_hw_params_t *hparams = alsa_alloca0 (snd_pcm_hw_params);
    if (snd_pcm_hw_params_any (phandle, hparams) < 0)
      return_error ("snd_pcm_hw_params_any", FILE_OPEN_FAILED);
    if (snd_pcm_hw_params_set_channels (phandle, hparams, n_channels_) < 0)
      return_error ("snd_pcm_hw_params_set_channels", DEVICE_CHANNELS);
    if (snd_pcm_hw_params_set_access (phandle, hparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
      return_error ("snd_pcm_hw_params_set_access", DEVICE_FORMAT);
    if (snd_pcm_hw_params_set_format (phandle, hparams, SND_PCM_FORMAT_S16_LE) < 0)
      return_error ("snd_pcm_hw_params_set_format", DEVICE_FORMAT);
    // sample_rate
    uint rate = *mix_freq;
    if (snd_pcm_hw_params_set_rate (phandle, hparams, rate, 0) < 0 || rate != *mix_freq)
      return_error ("snd_pcm_hw_params_set_rate", DEVICE_FREQUENCY);
    ADEBUG ("PCM: %s: rate: %d", alsadev_, rate);
    // fragment size
    snd_pcm_uframes_t period_min = 2, period_max = 1048576;
    snd_pcm_hw_params_get_period_size_min (hparams, &period_min, nullptr);
    snd_pcm_hw_params_get_period_size_max (hparams, &period_max, nullptr);
    const snd_pcm_uframes_t latency_frames = rate * latency_ms / 1000; // full IO latency in frames
    snd_pcm_uframes_t period_size = 32; // smaller sizes are infeasible with most hw
    while (period_size + 16 <= latency_frames / 3)
      period_size += 16; // maximize period_size as long as 3 fit the latency
    period_size = CLAMP (period_size, period_min, period_max);
    period_size = MIN (period_size, *period_sizep); // MAX_BLOCK_SIZE constraint
    int dir = 0;
    if (snd_pcm_hw_params_set_period_size_near (phandle, hparams, &period_size, &dir) < 0)
      return_error ("snd_pcm_hw_params_set_period_size_near", DEVICE_LATENCY);
    ADEBUG ("PCM: %s: period_size: %d (dir=%+d, min=%d max=%d)", alsadev_,
            period_size, dir, period_min, period_max);
    // fragment count
    const uint want_nperiods = latency_ms == 0 ? 2 : CLAMP (latency_frames / period_size, 2, 1023) + 1;
    uint nperiods = want_nperiods;
    if (snd_pcm_hw_params_set_periods_near (phandle, hparams, &nperiods, nullptr) < 0)
      return_error ("snd_pcm_hw_params_set_periods", DEVICE_LATENCY);
    ADEBUG ("PCM: %s: n_periods: %d (requested: %d)", alsadev_, nperiods, want_nperiods);
    if (snd_pcm_hw_params (phandle, hparams) < 0)
      return_error ("snd_pcm_hw_params", FILE_OPEN_FAILED);
    // verify hardware settings
    snd_pcm_uframes_t buffer_size_min = 0, buffer_size_max = 0, buffer_size = 0;
    if (snd_pcm_hw_params_get_buffer_size_min (hparams, &buffer_size_min) < 0 ||
        snd_pcm_hw_params_get_buffer_size_max (hparams, &buffer_size_max) < 0 ||
        snd_pcm_hw_params_get_buffer_size (hparams, &buffer_size) < 0)
      return_error ("snd_pcm_hw_params_get_buffer_size", DEVICE_BUFFER);
    ADEBUG ("PCM: %s: buffer_size: %d (min=%d, max=%d)", alsadev_, buffer_size, buffer_size_min, buffer_size_max);
    // setup software configuration
    snd_pcm_sw_params_t *sparams = alsa_alloca0 (snd_pcm_sw_params);
    if (snd_pcm_sw_params_current (phandle, sparams) < 0)
      return_error ("snd_pcm_sw_params_current", FILE_OPEN_FAILED);
    if (snd_pcm_sw_params_set_start_threshold (phandle, sparams, (buffer_size / period_size) * period_size) < 0)
      return_error ("snd_pcm_sw_params_set_start_threshold", DEVICE_BUFFER);
    snd_pcm_uframes_t availmin = 0;
    if (snd_pcm_sw_params_set_avail_min (phandle, sparams, period_size) < 0 ||
        snd_pcm_sw_params_get_avail_min (sparams, &availmin) < 0)
      return_error ("snd_pcm_sw_params_set_avail_min", DEVICE_LATENCY);
    ADEBUG ("PCM: %s: avail_min: %d", alsadev_, availmin);
    if (snd_pcm_sw_params_set_stop_threshold (phandle, sparams, LONG_MAX) < 0) // keep going on underruns
      return_error ("snd_pcm_sw_params_set_stop_threshold", DEVICE_BUFFER);
    snd_pcm_uframes_t stopthreshold = 0;
    if (snd_pcm_sw_params_get_stop_threshold (sparams, &stopthreshold) < 0)
      return_error ("snd_pcm_sw_params_get_stop_threshold", DEVICE_BUFFER);
    ADEBUG ("PCM: %s: stop_threshold: %d", alsadev_, stopthreshold);
    if (snd_pcm_sw_params_set_silence_threshold (phandle, sparams, 0) < 0)   // avoid early dropouts
      return_error ("snd_pcm_sw_params_set_silence_threshold", DEVICE_BUFFER);
    if (snd_pcm_sw_params_set_silence_size (phandle, sparams, LONG_MAX) < 0) // silence past frames
      return_error ("snd_pcm_sw_params_set_silence_size", DEVICE_BUFFER);
    if (snd_pcm_sw_params (phandle, sparams) < 0)
      return_error ("snd_pcm_sw_params", FILE_OPEN_FAILED);
    // return values
    *mix_freq = rate;
    *n_periodsp = nperiods;
    *period_sizep = period_size;
    ADEBUG ("PCM: %s: OPEN: r=%d w=%d n_channels=%d sample_freq=%d nperiods=%u period=%u (%u) bufsz=%u",
            alsadev_, phandle == read_handle_, phandle == write_handle_,
            n_channels_, *mix_freq, *n_periodsp, *period_sizep,
            nperiods * period_size, buffer_size);
    // snd_pcm_dump (phandle, snd_output);
    return Error::NONE;
  }
  void
  pcm_retrigger ()
  {
    snd_lib_error_set_handler (silent_error_handler);
    ADEBUG ("PCM: %s: retriggering device (r=%s w=%s)...",
            alsadev_, !read_handle_ ? "<CLOSED>" : snd_pcm_state_name (snd_pcm_state (read_handle_)),
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
      info ("ALSA: failed to prepare for io: %s\n", snd_strerror (aerror));
    // fill playback buffer with silence
    if (write_handle_)
      {
        const float *zeros = bse_engine_const_zeros (period_size_ / 2); // sizeof (int16) / sizeof (float)
        for (size_t i = 0; i < n_periods_; i++)
          {
            int n;
            do
              n = snd_pcm_writei (write_handle_, zeros, period_size_);
            while (n == -EAGAIN); // retry on signals
            // printerr ("%s: written=%d, left: %d / %d\n", __func__, n, snd_pcm_avail (write_handle_), n_periods_ * period_size_);
          }
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
  virtual void
  pcm_latency (uint *rlatency, uint *wlatency) const override
  {
    snd_pcm_sframes_t rdelay, wdelay;
    if (!read_handle_ || snd_pcm_delay (read_handle_, &rdelay) < 0)
      rdelay = 0;
    if (!write_handle_ || snd_pcm_delay (write_handle_, &wdelay) < 0)
      wdelay = 0;
    const int buffer_length = n_periods_ * period_size_; // buffer size chosen by ALSA based on latency request
    // return total latency in frames
    *rlatency = CLAMP (rdelay, 0, buffer_length);
    *wlatency = CLAMP (wdelay, 0, buffer_length);
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
            ADEBUG ("PCM: %s: read() error: %s", alsadev_, snd_strerror (n_frames));
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
            ADEBUG ("PCM: %s: write() error: %s", alsadev_, snd_strerror (n));
            snd_lib_error_set_handler (silent_error_handler);
            snd_pcm_prepare (write_handle_);    // force retrigger
            snd_lib_error_set_handler (NULL);
            return;
          }
        n_left -= n;
      }
  }
};

static const String alsa_pcm_driverid = PcmDriver::register_driver ("alsa",
                                                                    AlsaPcmDriver::create,
                                                                    [] (Driver::EntryVec &entries) {
                                                                      list_alsa_drivers (entries, true, false);
                                                                    });

// == AlsaRawMidiDriver ==
class AlsaRawMidiDriver : public MidiDriver {
  snd_rawmidi_t  *read_handle_ = nullptr;
  snd_rawmidi_t  *write_handle_ = nullptr;
  BseMidiDecoder *midi_decoder_ = nullptr;
  uint            total_pfds_ = 0;
public:
  static MidiDriverP
  create (const String &devid)
  {
    auto mdriverp = std::make_shared<AlsaRawMidiDriver> (devid);
    return mdriverp;
  }
  explicit
  AlsaRawMidiDriver (const String &devid) :
    MidiDriver (devid)
  {
    midi_decoder_ = bse_midi_decoder_new (TRUE, FALSE, MusicalTuning::OD_12_TET);
  }
  ~AlsaRawMidiDriver()
  {
    bse_midi_decoder_destroy (midi_decoder_);
    if (read_handle_)
      snd_rawmidi_close (read_handle_);
    if (write_handle_)
      snd_rawmidi_close (write_handle_);
  }
  virtual void
  close () override
  {
    assert_return (opened());
    if (total_pfds_)
      Sequencer::instance().remove_io_watch (midi_io_handler_func, static_cast<void*> (this));
    if (read_handle_)
      {
        snd_rawmidi_drop (read_handle_);
        snd_rawmidi_close (read_handle_);
        read_handle_ = nullptr;
      }
    if (write_handle_)
      {
        snd_rawmidi_nonblock (write_handle_, 0);
        snd_rawmidi_drain (write_handle_);
        snd_rawmidi_close (write_handle_);
        write_handle_ = nullptr;
      }
    flags_ &= ~size_t (Flags::OPENED | Flags::READABLE | Flags::WRITABLE);
  }
  bool // called from Sequencer thread
  midi_io_handler (uint n_pfds, GPollFD *pfds)
  {
    const size_t buf_size = 8192;
    uint8 buffer[buf_size];
    const uint64 systime = sfi_time_system ();
    ssize_t l;
    do
      l = snd_rawmidi_read (read_handle_, buffer, buf_size);
    while (l < 0 && errno == EINTR);    // restart on signal
    if (l > 0)
      bse_midi_decoder_push_data (midi_decoder_, l, buffer, systime);
    return true;
  }
  static gboolean // called from Sequencer thread
  midi_io_handler_func (void *thisdata, uint n_pfds, GPollFD *pfds)
  {
    AlsaRawMidiDriver *thisp = static_cast<AlsaRawMidiDriver*> (thisdata);
    return thisp->midi_io_handler (n_pfds, pfds);
  }
  virtual Error
  open (IODir iodir) override
  {
    assert_return (!opened(), Error::INTERNAL);
    int aerror = 0;
    // setup request
    const bool require_readable = iodir == READONLY || iodir == READWRITE;
    const bool require_writable = iodir == WRITEONLY || iodir == READWRITE;
    flags_ |= Flags::READABLE * require_readable;
    flags_ |= Flags::WRITABLE * require_writable;
    // try open
    snd_lib_error_set_handler (silent_error_handler);
    aerror = snd_rawmidi_open (require_readable ? &read_handle_ : NULL,
                               require_writable ? &write_handle_ : NULL,
                               devid_.c_str(), SND_RAWMIDI_NONBLOCK);
    snd_lib_error_set_handler (NULL);
    // try setup
    Error error = !aerror ? Error::NONE : bse_error_from_errno (-aerror, Error::FILE_OPEN_FAILED);
    snd_rawmidi_params_t *mparams = alsa_alloca0 (snd_rawmidi_params);
    if (error == 0 && read_handle_)
      {
        if (snd_rawmidi_params_current (read_handle_, mparams) < 0)
          error = Error::FILE_OPEN_FAILED;
        else
          ADEBUG ("RawMIDI: %s: readable: buffer=%d active_sensing=%d min_avail=%d\n",
                  devid_, snd_rawmidi_params_get_buffer_size (mparams),
                  !snd_rawmidi_params_get_no_active_sensing (mparams),
                  snd_rawmidi_params_get_avail_min (mparams));
      }
    if (error == 0 && write_handle_)
      {
        if (snd_rawmidi_params_current (write_handle_, mparams) < 0)
          error = Error::FILE_OPEN_FAILED;
        else
          ADEBUG ("RawMIDI: %s: writable: buffer=%d active_sensing=%d min_avail=%d\n",
                  devid_, snd_rawmidi_params_get_buffer_size (mparams),
                  !snd_rawmidi_params_get_no_active_sensing (mparams),
                  snd_rawmidi_params_get_avail_min (mparams));
      }
    if (error == 0 && read_handle_ && snd_rawmidi_poll_descriptors_count (read_handle_) <= 0)
      error = Error::FILE_OPEN_FAILED;
    if (!read_handle_ && !write_handle_)
      error = Error::FILE_OPEN_FAILED;
    // finish opening or shutdown
    if (error == 0)
      {
        flags_ |= Flags::OPENED;
        if (read_handle_)
          snd_rawmidi_nonblock (read_handle_, 1);
        if (write_handle_)
          snd_rawmidi_nonblock (write_handle_, 1);
        int rn = read_handle_ ? snd_rawmidi_poll_descriptors_count (read_handle_) : 0;
        int wn = write_handle_ ? snd_rawmidi_poll_descriptors_count (write_handle_) : 0;
        rn = MAX (rn, 0);
        wn = MAX (wn, 0);
        if (rn || wn)
          {
            struct pollfd *pfds = g_newa (struct pollfd, rn + wn);
            static_assert (sizeof (struct pollfd) == sizeof (GPollFD), "");
            total_pfds_ = 0;
            if (rn && snd_rawmidi_poll_descriptors (read_handle_, pfds, rn) >= 0)
              total_pfds_ += rn;
            if (wn && snd_rawmidi_poll_descriptors (write_handle_, pfds + total_pfds_, wn) >= 0)
              total_pfds_ += wn;
            if (total_pfds_)
              Sequencer::instance().add_io_watch (total_pfds_, (GPollFD*) pfds, midi_io_handler_func, static_cast<void*> (this));
          }
      }
    else
      {
        if (read_handle_)
          snd_rawmidi_close (read_handle_);
        read_handle_ = nullptr;
        if (write_handle_)
          snd_rawmidi_close (write_handle_);
        write_handle_ = nullptr;
      }
    ADEBUG ("RawMIDI: %s: opening readable=%d writable=%d: %s", devid_, readable(), writable(), bse_error_blurb (error));
    return error;
  }
};

static const String alsa_rawmidi_driverid = MidiDriver::register_driver ("alsarawmidi",
                                                                         AlsaRawMidiDriver::create,
                                                                         [] (Driver::EntryVec &entries) {
                                                                           list_alsa_drivers (entries, false, true);
                                                                         });

// == AlsaSeqMidiDriver ==
class AlsaSeqMidiDriver : public MidiDriver {
  using PortSubscribe = std::unique_ptr<snd_seq_port_subscribe_t, decltype (&snd_seq_port_subscribe_free)>;
  snd_seq_t        *seq_ = nullptr;
  int               queue_ = -1, iport_ = -1, total_fds_ = 0;
  snd_midi_event_t *evparser_ = nullptr;
  PortSubscribe     subs_;
  PortSubscribe
  make_port_subscribe (snd_seq_port_subscribe_t *other = nullptr)
  {
    snd_seq_port_subscribe_t *subs = nullptr;
    snd_seq_port_subscribe_malloc (&subs);
    if (!subs)
      return { nullptr, snd_seq_port_subscribe_free };
    if (other)
      snd_seq_port_subscribe_copy (subs, other);
    return { subs, snd_seq_port_subscribe_free };
  }
public:
  static MidiDriverP
  create (const String &devid)
  {
    auto mdriverp = std::make_shared<AlsaSeqMidiDriver> (devid);
    return mdriverp;
  }
  explicit
  AlsaSeqMidiDriver (const String &devid) :
    MidiDriver (devid), subs_ { nullptr, nullptr }
  {}
  ~AlsaSeqMidiDriver()
  {
    cleanup();
  }
  Error // setup seq_, queue_
  initialize (const std::string &myname)
  {
    // https://www.alsa-project.org/alsa-doc/alsa-lib/seq.html
    assert_return (seq_ == nullptr, Error::INTERNAL);
    assert_return (queue_ == -1, Error::INTERNAL);
    int aerror = 0;
    if (!aerror)
      aerror = snd_seq_open (&seq_, "default", SND_SEQ_OPEN_DUPLEX, SND_SEQ_NONBLOCK);
    if (!aerror)
      aerror = snd_seq_set_client_name (seq_, myname.c_str());
    if (!aerror)
      queue_ = snd_seq_alloc_named_queue (seq_, (myname + " SeqQueue").c_str());
    snd_seq_queue_tempo_t *qtempo = alsa_alloca0 (snd_seq_queue_tempo);
    snd_seq_queue_tempo_set_tempo (qtempo, 60 * 1000000 / 480); // 480 BPM in µs
    snd_seq_queue_tempo_set_ppq (qtempo, 1920); // pulse per quarter note
    if (!aerror)
      aerror = snd_seq_set_queue_tempo (seq_, queue_, qtempo);
    if (!aerror)
      snd_seq_start_queue (seq_, queue_, nullptr);
    if (!aerror)
      aerror = snd_seq_drain_output (seq_);
    ADEBUG ("SeqMIDI: %s: queue started: %.5f", myname, queue_now());
    return Error::NONE;
  }
  static std::string
  normalize (const std::string &string)
  {
    std::string normalized;
    auto is_identifier_char = [] (int ch) {
      return ( (ch >= 'A' && ch <= 'Z') ||
               (ch >= 'a' && ch <= 'z') ||
               (ch >= '0' && ch <= '9') ||
               ch == '_' || ch == '$' );
    };
    for (size_t i = 0; i < string.size() && string[i]; ++i)
      if (is_identifier_char (string[i]))
        normalized += string[i];
      else if (normalized.size() && normalized[normalized.size() - 1] != '-')
        normalized += '-';
    return normalized;
  }
  std::string
  make_devid (int card, uint type, const std::string &clientname, int client, uint caps)
  {
    if (0 == (type & SND_SEQ_PORT_TYPE_MIDI_GENERIC))
      return ""; // not suitable
    std::string devid;
    if ((type & SND_SEQ_PORT_TYPE_SYNTHESIZER) && (type & SND_SEQ_PORT_TYPE_HARDWARE))
      devid = "hwsynth:";
    else if ((type & SND_SEQ_PORT_TYPE_SYNTHESIZER) && (type & SND_SEQ_PORT_TYPE_SOFTWARE))
      devid = "swsynth:";
    else if (type & SND_SEQ_PORT_TYPE_SYNTHESIZER)
      devid = "synth:";
    else if (type & SND_SEQ_PORT_TYPE_APPLICATION)
      devid = "midiapp:";
    else if (type & SND_SEQ_PORT_TYPE_HARDWARE)
      devid = "hwmidi:";
    else if (type & SND_SEQ_PORT_TYPE_SOFTWARE)
      devid = "swmidi:";
    else // MIDI_GENERIC
      devid = "gmidi:";
    std::string cardid;
    if (card >= 0)
      {
        snd_ctl_t *chandle = nullptr;
        const auto sbuf = string_format ("hw:CARD=%u", card);
        if (snd_ctl_open (&chandle, sbuf.c_str(), SND_CTL_NONBLOCK) >= 0 && chandle)
          {
            snd_ctl_card_info_t *cinfo = alsa_alloca0 (snd_ctl_card_info);
            if (snd_ctl_card_info (chandle, cinfo) >= 0)
              {
                const char *cid = snd_ctl_card_info_get_id (cinfo);
                if (cid)
                  cardid = cid;
              }
            snd_ctl_close (chandle);
          }
      }
    if (!cardid.empty())
      devid += normalize (cardid);
    else if (!clientname.empty())
      devid += normalize (clientname);
    else // unlikely, ALSA makes up *some* clientname
      return ""; // not suitable
    // devid += "."; devid += itos (port);
    return devid;
  }
  bool
  enumerate (Driver::EntryVec *entries, snd_seq_port_info_t *sinfo = nullptr, const std::string &selector = "", uint need_caps = 0)
  {
    assert_return (seq_ != nullptr, false);
    snd_seq_client_info_t *cinfo = alsa_alloca0 (snd_seq_client_info);
    snd_seq_client_info_set_client (cinfo, -1);
    while (snd_seq_query_next_client (seq_, cinfo) == 0)
      {
        const int client = snd_seq_client_info_get_client (cinfo);
        if (client == 0)
          continue; // System Sequencer
        snd_seq_port_info_t *pinfo = alsa_alloca0 (snd_seq_port_info);
        snd_seq_port_info_set_client (pinfo, client);
        snd_seq_port_info_set_port (pinfo, -1);
        while (snd_seq_query_next_port (seq_, pinfo) == 0)
          {
            const uint tmask = SND_SEQ_PORT_TYPE_MIDI_GENERIC |
                               SND_SEQ_PORT_TYPE_SYNTHESIZER |
                               SND_SEQ_PORT_TYPE_APPLICATION;
            const int type = snd_seq_port_info_get_type (pinfo);
            if (0 == (type & tmask))
              continue;
            const uint cmask = SND_SEQ_PORT_CAP_READ |
                               SND_SEQ_PORT_CAP_WRITE |
                               SND_SEQ_PORT_CAP_DUPLEX;
            const int caps = snd_seq_port_info_get_capability (pinfo);
            if (0 == (caps & cmask) || need_caps != (need_caps & caps))
              continue;
            const int card = snd_seq_client_info_get_card (cinfo);
            const std::string clientname = snd_seq_client_info_get_name (cinfo);
            std::string devportid = make_devid (card, type, clientname, client, caps);
            if (devportid.empty())
              continue; // device needs to be uniquely identifiable
            const int cport = snd_seq_port_info_get_port (pinfo);
            devportid += "." + string_from_int (cport);
            if (entries)
              {
                std::string cardname, longname;
                if (card >= 0)
                  {
                    char *str = nullptr;
                    if (snd_card_get_longname (card, &str) == 0 && str)
                      longname = str;
                    if (str)
                      free (str);
                    if (snd_card_get_name (card, &str) == 0 && str)
                      cardname = str;
                    if (str)
                      free (str);
                  }
                const bool is_usb = longname.find (" at usb-") != std::string::npos;
                const bool is_kern = snd_seq_client_info_get_type (cinfo) == SND_SEQ_KERNEL_CLIENT;
                const bool is_thru = is_kern && clientname == "Midi Through";
                const std::string devname = string_capitalize (clientname, 1, false);
                Driver::Entry entry;
                entry.devid = devportid;
                entry.device_name = string_capitalize (snd_seq_port_info_get_name (pinfo), 1, false);
                if (!string_startswith (entry.device_name, devname))
                  entry.device_name = devname + " " + entry.device_name;
                if (!cardname.empty())
                  entry.device_name += " - " + cardname;
                if (caps & SND_SEQ_PORT_CAP_DUPLEX)
                  entry.capabilities = "Full-Duplex MIDI";
                else if ((caps & SND_SEQ_PORT_CAP_READ) && (caps & SND_SEQ_PORT_CAP_WRITE))
                  entry.capabilities = "MIDI In-Out";
                else if (caps & SND_SEQ_PORT_CAP_READ)
                  entry.capabilities = "MIDI Output";
                else if (caps & SND_SEQ_PORT_CAP_WRITE)
                  entry.capabilities = "MIDI Input";
                if (!string_startswith (longname, cardname + " at "))
                  entry.device_info = longname;
                if (type & SND_SEQ_PORT_TYPE_APPLICATION || !is_kern)
                  entry.notice = "Note: MIDI device is provided by an application";
                entry.readonly = (caps & SND_SEQ_PORT_CAP_READ) && !(caps & SND_SEQ_PORT_CAP_WRITE);
                entry.writeonly = (caps & SND_SEQ_PORT_CAP_WRITE) && !(caps & SND_SEQ_PORT_CAP_READ);
                entry.priority = is_thru ? Driver::ALSA_THRU :
                                 is_usb ? Driver::ALSA_USB :
                                 is_kern ? Driver::ALSA_KERN :
                                 Driver::ALSA_USER;
                entry.priority += Driver::WCARD * MAX (0, card);
                entry.priority += Driver::WDEV * client; // priorize HW over software apps
                entry.priority += Driver::WSUB * cport;
                entries->push_back (entry);
                ADEBUG ("DISCOVER: MIDI: %s - %s", entry.devid, entry.device_name);
              }
            const bool match = selector == devportid;
            if (match && sinfo)
              {
                snd_seq_port_info_copy (sinfo, pinfo);
                return true;
              }
          }
      }
    return false;
  }
  static void
  list_drivers (Driver::EntryVec &entries)
  {
    AlsaSeqMidiDriver smd ("?");
    smd.initialize (program_alias() + " Probing");
    smd.enumerate (&entries);
  }
  virtual Error // setup iport_ subs_ evparser_ total_fds_
  open (IODir iodir) override
  {
    // initial sequencer setup
    assert_return (iport_ == -1, Error::INTERNAL);
    assert_return (!subs_, Error::INTERNAL);
    assert_return (!evparser_, Error::INTERNAL);
    assert_return (total_fds_ == 0, Error::INTERNAL);
    PortSubscribe psub = make_port_subscribe();
    assert_return (!!psub, Error::NO_MEMORY);
    const std::string myname = program_alias();
    Error error = seq_ ? Error::NONE : initialize (myname);
    if (error != Error::NONE)
      return error;
    assert_return (queue_ >= -1, Error::INTERNAL);
    // find devid_
    const bool require_readable = iodir == READONLY || iodir == READWRITE;
    const bool require_writable = iodir == WRITEONLY || iodir == READWRITE;
    const uint caps = require_writable * (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ) +
                      require_readable * (SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE);
    snd_seq_port_info_t *pinfo = alsa_alloca0 (snd_seq_port_info);
    const bool match_devid = enumerate (nullptr, pinfo, devid_, caps);
    if (!match_devid)
      return Error::DEVICE_NOT_AVAILABLE;
    flags_ |= Flags::READABLE * require_readable;
    flags_ |= Flags::WRITABLE * require_writable;
    // create port for subscription
    snd_seq_port_info_t *minfo = alsa_alloca0 (snd_seq_port_info);
    snd_seq_port_info_set_port (minfo, 0); // desired port number
    snd_seq_port_info_set_port_specified (minfo, true);
    snd_seq_port_info_set_name (minfo, (myname + " LSP-0").c_str()); // Local Subscription Port
    snd_seq_port_info_set_type (minfo, SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);
    const int intracaps = SND_SEQ_PORT_CAP_NO_EXPORT | SND_SEQ_PORT_CAP_WRITE;
    snd_seq_port_info_set_capability (minfo, intracaps);
    snd_seq_port_info_set_midi_channels (minfo, 16);
    snd_seq_port_info_set_timestamping (minfo, true);
    snd_seq_port_info_set_timestamp_real (minfo, true);
    snd_seq_port_info_set_timestamp_queue (minfo, queue_);
    int aerror = snd_seq_create_port (seq_, minfo);
    if (!aerror)
      {
        iport_ = snd_seq_port_info_get_port (minfo);
        if (iport_ < 0)
          aerror = -ENOMEM;
      }
    // subscribe to port
    snd_seq_addr_t qaddr = {};
    qaddr.client = snd_seq_port_info_get_client (pinfo);
    qaddr.port = snd_seq_port_info_get_port (pinfo);
    snd_seq_port_subscribe_set_sender (&*psub, &qaddr);
    qaddr.client = snd_seq_client_id (seq_);
    qaddr.port = iport_; // receiver
    snd_seq_port_subscribe_set_dest (&*psub, &qaddr);
    snd_seq_port_subscribe_set_queue (&*psub, queue_);
    snd_seq_port_subscribe_set_time_update (&*psub, true);
    snd_seq_port_subscribe_set_time_real (&*psub, true);
    if (!aerror)
      aerror = snd_seq_subscribe_port (seq_, &*psub);
    if (!aerror)
      {
        subs_ = make_port_subscribe (&*psub);
        if (!subs_)
          aerror = -ENOMEM;
      }
    // setup event parser
    if (!aerror)
      snd_seq_drain_output (seq_);
    if (!aerror)
      aerror = snd_midi_event_new (1024, &evparser_);
    if (!aerror)
      {
        snd_midi_event_init (evparser_);
        snd_midi_event_no_status (evparser_, true);
      }
    // done, cleanup
    if (!aerror)
      {
        // Note, this *only* sets up polling for MIDI reading...
        total_fds_ = snd_seq_poll_descriptors_count (seq_, POLLIN);
        if (total_fds_ > 0)
          {
            struct pollfd *pfds = (struct pollfd*) alloca (total_fds_ * sizeof (struct pollfd));
            if (snd_seq_poll_descriptors (seq_, pfds, total_fds_, POLLIN) > 0)
              {
                static_assert (sizeof (struct pollfd) == sizeof (GPollFD));
                BseTrans *trans = bse_trans_open ();
                BseJob *job = bse_job_add_poll (pollin_func, (void*) this, pollfree_func, total_fds_, (const GPollFD*) pfds);
                bse_trans_add (trans, job);
                bse_trans_commit (trans);
              }
            else
              total_fds_ = 0;
          }
        flags_ |= Flags::OPENED;
      }
    error = !aerror ? Error::NONE : bse_error_from_errno (-aerror, Error::FILE_OPEN_FAILED);
    ADEBUG ("SeqMIDI: %s: opening readable=%d writable=%d: %s", devid_, readable(), writable(), bse_error_blurb (error));
    if (error != Error::NONE)
      cleanup();
    return error;
  }
  void
  cleanup()
  {
    if (total_fds_ > 0)
      {
        total_fds_ = 0;
        BseTrans *trans = bse_trans_open ();
        BseJob *job = bse_job_remove_poll (pollin_func, (void*) this);
        bse_trans_add (trans, job);
        bse_trans_commit (trans);
      }
    if (evparser_)
      {
        snd_midi_event_free (evparser_);
        evparser_ = nullptr;
      }
    if (subs_)
      {
        snd_seq_unsubscribe_port (seq_, &*subs_);
        subs_.reset();
      }
    if (iport_ >= 0)
      {
        snd_seq_delete_port (seq_, iport_);
        iport_ = -1;
      }
    if (queue_ >= 0)
      {
        snd_seq_free_queue (seq_, queue_);
        queue_ = -1;
      }
    if (seq_)
      {
        snd_seq_close (seq_);
        seq_ = nullptr;
      }
  }
  virtual void
  close () override
  {
    assert_return (opened());
    cleanup();
    ADEBUG ("SeqMIDI: %s: CLOSE: r=%d w=%d", devid_, readable(), writable());
    flags_ &= ~size_t (Flags::OPENED | Flags::READABLE | Flags::WRITABLE);
  }
  static void
  pollfree_func (void *data)
  {
    // AlsaSeqMidiDriver *thisp = (AlsaSeqMidiDriver*) data;
  }
  static gboolean
  pollin_func (void *data, uint n_values, long *timeout_p, uint n_fds, const GPollFD *fds, gboolean revents_filled)
  {
    AlsaSeqMidiDriver *thisp = (AlsaSeqMidiDriver*) data;
    // FIXME: instead of all this, do: get_pollfds(); get_events (&evstack);
    if (revents_filled && fds->revents)
      thisp->get_events();
    return false;
  }
  void
  get_events()
  {
    assert_return (!!evparser_);
    // receive
    const bool pull_fifo = true;
    double now = -1;
    uint8_t mebuf[1024];
    while (snd_seq_event_input_pending (seq_, pull_fifo) > 0)
      {
        snd_seq_event_t *ev = nullptr;
        int r = snd_seq_event_input (seq_, &ev);
        while (r > 0)
          {
            if (now < 0)
              now = queue_now();
            switch (ev->type)
              {
                // decide if decoding is worth it
              }
            r = snd_midi_event_decode (evparser_, mebuf, sizeof (mebuf), ev);
            const double t = ev->time.time.tv_sec + 1e-9 * ev->time.time.tv_nsec;
            const char *et = nullptr;
            if (r > 0)
              switch (ev->type)
                {
                case SND_SEQ_EVENT_NOTE:            if (!et) et = "NOTE";
                case SND_SEQ_EVENT_NOTEON:          if (!et) et = "NOTEON";
                case SND_SEQ_EVENT_NOTEOFF:         if (!et) et = "NOTEOFF";
                case SND_SEQ_EVENT_KEYPRESS:        if (!et) et = "KEYPRESS";
                  printf ("%5.6f: %s: ch=%d note=%d vel=%d off=%d dur=%d\n",
                          now - t, et,
                          ev->data.note.channel,
                          ev->data.note.note,
                          ev->data.note.velocity,
                          ev->data.note.off_velocity,
                          ev->data.note.duration);
                  break;
                case SND_SEQ_EVENT_CONTROLLER:	if (!et) et = "CONTROLLER";
                case SND_SEQ_EVENT_PGMCHANGE:	if (!et) et = "PGMCHANGE";
                case SND_SEQ_EVENT_CHANPRESS:	if (!et) et = "CHANPRESS";
                case SND_SEQ_EVENT_PITCHBEND:	if (!et) et = "PITCHBEND";
                case SND_SEQ_EVENT_CONTROL14:	if (!et) et = "CONTROL14";
                case SND_SEQ_EVENT_NONREGPARAM:	if (!et) et = "NONREGPARAM";
                case SND_SEQ_EVENT_REGPARAM:	if (!et) et = "REGPARAM";
                case SND_SEQ_EVENT_SONGPOS:		if (!et) et = "SONGPOS";
                case SND_SEQ_EVENT_SONGSEL:		if (!et) et = "SONGSEL";
                case SND_SEQ_EVENT_QFRAME:		if (!et) et = "QFRAME";
                case SND_SEQ_EVENT_TIMESIGN:	if (!et) et = "TIMESIGN";
                case SND_SEQ_EVENT_KEYSIGN:		if (!et) et = "KEYSIGN";
                  printf ("%5.6f: %s: ch=%d param=%d val=%d\n",
                          now - t, et,
                          ev->data.control.channel,
                          ev->data.control.param,
                          ev->data.control.value);
                  break;
                case SND_SEQ_EVENT_START:           if (!et) et = "START";
                case SND_SEQ_EVENT_CONTINUE:        if (!et) et = "CONTINUE";
                case SND_SEQ_EVENT_STOP:            if (!et) et = "STOP";
                case SND_SEQ_EVENT_SETPOS_TICK:     if (!et) et = "SETPOS_TICK";
                case SND_SEQ_EVENT_SETPOS_TIME:     if (!et) et = "SETPOS_TIME";
                case SND_SEQ_EVENT_TEMPO:           if (!et) et = "";
                case SND_SEQ_EVENT_CLOCK:           if (!et) et = "CLOCK";
                case SND_SEQ_EVENT_TICK:            if (!et) et = "TICK";
                case SND_SEQ_EVENT_QUEUE_SKEW:      if (!et) et = "QUEUE_SKEW";
                case SND_SEQ_EVENT_SYNC_POS:        if (!et) et = "SYNC_POS";
                  printf ("%5.6f: %s: q=%d param=%d t=%.5f pos=%d skew=%d,%d\n",
                          now - t, et,
                          ev->data.queue.queue,
                          ev->data.queue.param.value,
                          ev->data.queue.param.time.time.tv_sec +
                          1e-9 * ev->data.queue.param.time.time.tv_nsec,
                          ev->data.queue.param.position,
                          ev->data.queue.param.skew.value,
                          ev->data.queue.param.skew.base);
                  break;
                case SND_SEQ_EVENT_SYSEX:		if (!et) et = "SYSEX";
                  printf ("%5.6f: %s: LEN=%d",
                          now - t, et, ev->data.ext.len);
                  for (size_t i = 0; i < ev->data.ext.len; i++)
                    {
                      printf (" %02x", ((const uint8_t*) ev->data.ext.ptr)[i]);
                      if (i >= 8)
                        {
                          if (ev->data.ext.len > 8)
                            printf (" …");
                          break;
                        }
                    }
                  printf ("\n");
                  break;
                default:
                  printf ("%5.6f: SEQEVENT: type=%d\n",
                          now - t, ev->type);
                  break;
                }
            else if (r < 0)
              {
                errno = -r;
                perror ("snd_midi_event_decode");
              }
            // DEPRECATED: snd_seq_free_event (ev);
            r = snd_seq_event_input (seq_, &ev);
          }
        if (r < 0 && r != -EAGAIN) // -ENOSPC - sequencer FIFO overran
          {
            errno = -r;
            perror ("snd_seq_event_input");
            info ("ALSA: SeqMIDI: %s: sequencer FIFO error: %s\n", devid_, snd_strerror (r));
          }
      }
  }
  double
  queue_now ()
  {
    union { uint64_t u64[16]; char c[1]; } stbuf = {};
    assert_return (snd_seq_queue_status_sizeof() <= sizeof (stbuf), NAN);
    snd_seq_queue_status_t *stat = (snd_seq_queue_status_t*) &stbuf;
    int aerror = snd_seq_get_queue_status (seq_, queue_, stat);
    if (!aerror)
      {
        const snd_seq_real_time_t *rt = snd_seq_queue_status_get_real_time (stat);
        return rt->tv_sec + 1e-9 * rt->tv_nsec;
      }
    return NAN;
  }
};

static const String alsa_seqmidi_driverid = MidiDriver::register_driver ("alsa",
                                                                         AlsaSeqMidiDriver::create,
                                                                         AlsaSeqMidiDriver::list_drivers);

} // Bse

#endif  // __has_include(<alsa/asoundlib.h>)
