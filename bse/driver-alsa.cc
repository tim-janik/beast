// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "driver.hh"

#define PDEBUG(...)             Bse::debug ("pcm-alsa", __VA_ARGS__)
#define alsa_alloca0(struc)     ({ struc##_t *ptr = (struc##_t*) alloca (struc##_sizeof()); memset (ptr, 0, struc##_sizeof()); ptr; })

#if __has_include(<alsa/asoundlib.h>)
#include <alsa/asoundlib.h>

// for non-little endian, SND_PCM_FORMAT_S16_LE ans other places will need fixups
static_assert (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__, "endianess unimplemented");

namespace Bse {

static DriverP alsa_driver_create (const String &devid);

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

static bool
init_alsa_driver ()
{
  int err = snd_output_stdio_attach (&snd_output, stderr, 0);
  (void) err;
  return true;
}

static void
list_alsa_drivers (Driver::EntryVec &entries)
{
  static const bool BSE_USED initialized = init_alsa_driver();
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
          seen_plughw = seen_plughw || strncmp (name.c_str(), "plughw:", 7) == 0;
          if (name == "pulse")
            {
              PDEBUG ("HINT: %s (%s) - %s", name, ioid, substitute_string ("\n", " ", desc));
              Driver::Entry entry;
              entry.devid = name;
              entry.name = desc;
              entry.blurb = "Warning: PulseAudio routing is not realtime capable";
              entry.readonly = "Input" == ioid;
              entry.writeonly = "Output" == ioid;
              entry.priority = Driver::PULSE;
              entry.create = alsa_driver_create;
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
      PDEBUG ("CARD: %s - %s - %s [%s] - %s", card_id, card_driver, card_name, card_mixername, card_longname);
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
          entry.create = alsa_driver_create;
          entries.push_back (entry);
        }
      snd_ctl_close (chandle);
    }
}

static bool register_alsa = Driver::register_driver (Driver::Type::PCM, list_alsa_drivers);

static DriverP
alsa_driver_create (const String &devid)
{
  return NULL;
}

} // Bse

#endif  // __has_include(<alsa/asoundlib.h>)
