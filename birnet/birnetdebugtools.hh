/* Birnet
 * Copyright (C) 2007 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __BIRNET_DEBUG_TOOLS_HH__
#define __BIRNET_DEBUG_TOOLS_HH__

#include <birnet/birnetutils.hh>
#include <stdarg.h>

namespace Birnet {

class DebugChannel : public virtual ReferenceCountImpl {
  BIRNET_PRIVATE_CLASS_COPY (DebugChannel);
protected:
  explicit              DebugChannel        ();
  virtual               ~DebugChannel       ();
  virtual void          printf_valist       (const char *format,
                                             va_list     args) = 0;
public:
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
