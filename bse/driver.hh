// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __BSE_DRIVER_HH__
#define __BSE_DRIVER_HH__

#include <bse/bseutils.hh>
#include <functional>

namespace Bse {

struct DriverConfig {
  bool require_readable = false;
  bool require_writable = false;
  uint n_channels = 0;
  uint mix_freq = 0;
  uint latency_ms = 0;
  uint block_length = 0;
};

class Driver {
protected:
  struct Flags { enum { OPENED = 1, READABLE = 2, WRITABLE = 4, }; };
  const String       devid_;
  size_t             flags_ = 0;
  explicit           Driver     (const String &devid);
  virtual           ~Driver     ();
public:
  enum {
    JACK  = 0x01 << 24,
    ALSA  = 0x02 << 24,
    OSS   = 0x03 << 24,
    PULSE = 0x04 << 24,
    DUMMY = 0x7f << 24,
    WCARD = 0x01 << 16,
    WDEV  = 0x01 <<  8,
    WSUB  = 0x01 <<  0,
  };
  typedef std::shared_ptr<Driver> DriverP;
  bool           opened        () const        { return flags_ & Flags::OPENED; }
  bool           readable      () const        { return flags_ & Flags::READABLE; }
  bool           writable      () const        { return flags_ & Flags::WRITABLE; }
  virtual String devid         () const        { return devid_; }
  virtual void   close         () = 0;
  // registry
  struct Entry {
    String      devid, name, blurb, status;
    uint32      priority = 0xffffffff; // lower is better
    uint32      driverid = 0;
    bool        readonly = false;
    bool        writeonly = false;
    bool        duplex = false;
  };
  typedef std::vector<Entry> EntryVec;
};
using DriverP = Driver::DriverP;

class PcmDriver : public Driver {
protected:
  explicit           PcmDriver       (const String &devid);
  virtual Bse::Error open            (const DriverConfig &config) = 0;
public:
  typedef std::shared_ptr<PcmDriver> PcmDriverP;
  static PcmDriverP  open            (const Entry &entry, const DriverConfig &config, Bse::Error *ep);
  virtual bool       pcm_check_io    (long *timeoutp) = 0;
  virtual uint       pcm_latency     () const = 0;
  virtual float      pcm_frequency   () const = 0;
  virtual size_t     pcm_read        (size_t n, float *values) = 0;
  virtual void       pcm_write       (size_t n, const float *values) = 0;
  static EntryVec    list_drivers    ();
  static uint32      register_driver (const std::function<PcmDriverP (const String&)> &create,
                                      const std::function<void (EntryVec&, uint32)> &list);
};
using PcmDriverP = PcmDriver::PcmDriverP;

class MidiDriver : public Driver {
protected:
  explicit           MidiDriver      (const String &devid);
  virtual Bse::Error open            (const DriverConfig &config) = 0;
public:
  typedef std::shared_ptr<MidiDriver> MidiDriverP;
  static MidiDriverP open            (const Entry &entry, const DriverConfig &config, Bse::Error *ep);
  static EntryVec    list_drivers    ();
  static uint32      register_driver (const std::function<MidiDriverP (const String&)> &create,
                                      const std::function<void (EntryVec&, uint32)> &list);
};
using MidiDriverP = MidiDriver::MidiDriverP;

} // Bse

#endif  // __BSE_DRIVER_HH__
