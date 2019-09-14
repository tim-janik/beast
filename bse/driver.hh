// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __BSE_DRIVER_HH__
#define __BSE_DRIVER_HH__

#include <bse/bseutils.hh>

namespace Bse {

class Driver {
  enum class Flags { OPENED = 1, READABLE = 2, WRITABLE = 4, };
  Flags flags_ = Flags (0);
protected:
  const String       devid_;
  explicit           Driver     (const String &devid);
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
  bool               opened     () const        { return size_t (flags_) & size_t (Flags::OPENED); }
  bool               readable   () const        { return size_t (flags_) & size_t (Flags::READABLE); }
  bool               writable   () const        { return size_t (flags_) & size_t (Flags::WRITABLE); }
  virtual String     type       () const = 0;
  virtual String     name       () const = 0;
  virtual String     args       () const = 0;
  virtual Bse::Error open       (const String &devid, bool require_readable, bool require_writable);
  virtual void       close      ();
  // registry
  struct Entry {
    String      devid, name, blurb, status;
    uint32      priority = 0xffffffff; // lower is better
    bool        readonly = false;
    bool        writeonly = false;
    bool        duplex = false;
    DriverP   (*create) (const String &devid) = nullptr;
  };
  typedef std::vector<Entry> EntryVec;
  enum class Type { PCM = 1, MIDI = 2, };
  static bool        register_driver (Type, void (*f) (EntryVec&));
  static EntryVec    list_drivers    (Type type);
};
using DriverP = Driver::DriverP;

} // Bse

#endif  // __BSE_DRIVER_HH__
