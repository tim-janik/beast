// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BIRNET_DEBUG_TOOLS_HH__
#define __BIRNET_DEBUG_TOOLS_HH__
#include <birnet/birnetutils.hh>
#include <stdarg.h>
namespace Birnet {
class DebugChannel : public virtual ReferenceCountable {
  BIRNET_PRIVATE_CLASS_COPY (DebugChannel);
protected:
  explicit              DebugChannel        ();
  virtual               ~DebugChannel       ();
public:
  virtual void          printf_valist       (const char *format,
                                             va_list     args) = 0;
  inline void           printf              (const char *format, ...) BIRNET_PRINTF (2, 3);
  static DebugChannel*  new_from_file_async (const String &filename);
};
inline void
DebugChannel::printf (const char *format,
                      ...)
{
  va_list a;
  va_start (a, format);
  printf_valist (format, a);
  va_end (a);
}
} // Birnet
#endif /* __BIRNET_DEBUG_TOOLS_HH__ */
/* vim:set ts=8 sts=2 sw=2: */
