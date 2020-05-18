// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "bse/bcore.hh"
#include <unistd.h>

struct IpcSharedMem {
  static const int64   MAGIC = 0x4253457465737473;        // BSEtests
  int64                magic;
  std::atomic<int64>   counter;
  int64                length;
  char                 stringdata[];
  std::string          get_string     () const  { return std::string (stringdata, length); }
  static IpcSharedMem* create_shared  (const std::string &data, int *fd, int64 count);
  static IpcSharedMem* acquire_shared (int fd);
  static void          release_shared (IpcSharedMem *sm, int fd);
  static void          destroy_shared (IpcSharedMem *sm, int fd);
};


template<class ...Args> BSE_NORETURN void
die (const char *format, const Args &...args)
{
  Bse::printerr ("%s\n", Bse::string_format (format, args...));
  _exit (99);
}
