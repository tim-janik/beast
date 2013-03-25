// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "birnetdebugtools.hh"
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
struct DebugChannelFileAsync : public virtual DebugChannel {
  FILE                  *fout_;
  Rapicorn::Atomic<uint> skip_count_;
  std::thread            thread_;
  Rapicorn::AsyncRingBuffer<char> aring_;
  ~DebugChannelFileAsync()
  {
    if (fout_)
      fclose (fout_);
  }
  DebugChannelFileAsync (const String &filename) :
    fout_ (NULL), skip_count_ (0), aring_ (65536)
  {
    fout_ = fopen (filename.c_str(), "w");
    if (fout_)
      thread_ = std::thread (&DebugChannelFileAsync::run, this);
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
        uint n = aring_.write (l, buffer, false);
        if (!n)
          skip_count_ += 1;
      }
  }
  virtual void
  run ()
  {
    while (1)
      {
        char buffer[65536];
        uint n;
        do
          {
            n = aring_.read (sizeof (buffer), buffer);
            if (n)
              {
                fwrite (buffer, n, 1, fout_);
                fflush (fout_);
              }
            else
              {
                uint j;
                do
                  j = skip_count_;
                while (!skip_count_.cas (j, 0));
                if (j)
                  fprintf (fout_, "...[skipped %u messages]\n", j);
              }
          }
        while (n);
        std::this_thread::sleep_for (std::chrono::milliseconds (15));
      }
  }
};
DebugChannel*
DebugChannel::new_from_file_async (const String &filename)
{
  DebugChannelFileAsync *dcfa = new DebugChannelFileAsync (filename);
  return dcfa;
}
} // Birnet
