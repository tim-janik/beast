/* SFI - Synthesis Fusion Kit Interface                 -*-mode: c++;-*-
 * Copyright (C) 2003-2004 Tim Janik and Stefan Westerfeld
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
#ifndef __SFI_CXX_H__
#define __SFI_CXX_H__

#include <sfi/sfi.h>

namespace Sfi {

class String {
  char *cstring;
public:
  String()
  {
    cstring = g_strdup ("");
  }
  String (const String &s)
  {
    cstring = g_strdup (s.cstring);
  }
  String (const std::string &s)
  {
    cstring = g_strdup (s.c_str());
  }
  String (const char *cstr)
  {
    cstring = g_strdup (cstr ? cstr : "");
  }
  String& operator= (const std::string &s)
  {
    g_free (cstring);
    cstring = g_strdup (s.c_str());
    return *this;
  }
  String& operator= (const gchar *cstr)
  {
    g_free (cstring);
    cstring = g_strdup (cstr ? cstr : "");
    return *this;
  }
  String& operator= (const String &s)
  {
    g_free (cstring);
    cstring = g_strdup (s.cstring);
    return *this;
  }
  const char* c_str() const
  {
    return cstring;
  }
  String& operator+= (const std::string &src)
  {
    char *old = cstring;
    cstring = g_strconcat (old, src.c_str(), NULL);
    g_free (old);
    return *this;
  }
  String& operator+= (const gchar *cstr)
  {
    char *old = cstring;
    cstring = g_strconcat (old, cstr, NULL);
    g_free (old);
    return *this;
  }
  String& operator+= (const String &src)
  {
    char *old = cstring;
    cstring = g_strconcat (old, src.cstring, NULL);
    g_free (old);
    return *this;
  }
  unsigned int length()
  {
    return strlen (cstring);
  }
  ~String()
  {
    g_free (cstring);
  }
};

} // Sfi

#endif /* __SFI_CXX_H__ */

/* vim:set ts=8 sts=2 sw=2: */
