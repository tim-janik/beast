// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "birnetdebugtools.hh"
#include "birnetthread.hh"
#include <syslog.h>
#include <errno.h>
#include <stdio.h>
#ifndef _ // FIXME
#define _(x)    (x)
#endif
namespace Birnet {
DebugChannel::DebugChannel()
{}
DebugChannel::~DebugChannel ()
{}
struct DebugChannelFileAsync : public virtual DebugChannel, public virtual Thread {
  FILE                    *fout;
  uint                     skip_count;
  Atomic::RingBuffer<char> aring;
  ~DebugChannelFileAsync()
  {
    if (fout)
      fclose (fout);
  }
  DebugChannelFileAsync (const String &filename) :
    Thread ("DebugChannelFileAsync::logger"),
    fout (NULL), skip_count (0), aring (65536)
  {
    fout = fopen (filename.c_str(), "w");
    if (fout)
      start();
  }
  virtual void
  printf_valist (const char *format,
                 va_list     args)
  {
    const int bsz = 4000;
    char buffer[bsz + 2];
    int l = vsnprintf (buffer, bsz, format, args);
    if (l > 0)
      {
        l = MIN (l, bsz);
        if (buffer[l - 1] != '\n')
          {
            buffer[l++] = '\n';
            buffer[l] = 0;
          }
        if (false) // skip trailing 0 in ring buffer
          buffer[l++] = 0;
        uint n = aring.write (l, buffer, false);
        if (!n)
          Atomic::uint_swap_add (&skip_count, 1);
      }
  }
  virtual void
  run ()
  {
    do
      {
        char buffer[65536];
        uint n;
        do
          {
            n = aring.read (sizeof (buffer), buffer);
            if (n)
              {
                fwrite (buffer, n, 1, fout);
                fflush (fout);
              }
            else
              {
                uint j;
                do
                  j = Atomic::uint_get (&skip_count);
                while (!Atomic::uint_cas (&skip_count, j, 0));
                if (j)
                  fprintf (fout, "...[skipped %u messages]\n", j);
              }
          }
        while (n);
      }
    while (Thread::Self::sleep (15 * 1000));
  }
};
DebugChannel*
DebugChannel::new_from_file_async (const String &filename)
{
  DebugChannelFileAsync *dcfa = new DebugChannelFileAsync (filename);
  return dcfa;
}
} // Birnet
