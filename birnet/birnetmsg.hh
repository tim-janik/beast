/* Birnet
 * Copyright (C) 2006 Tim Janik
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
#ifndef __BIRNET_MSG_HH__
#define __BIRNET_MSG_HH__

#include <birnet/birnetcore.h>
#include <birnet/birnetutilsxx.hh> // FIXME: need String in core.hh

namespace Birnet {

/* --- messaging --- */
struct Msg {
  typedef enum {
    ERROR       = 'E',
    WARNING     = 'W',
    INFO        = 'I',
    DEBUG       = 'D',
  } Type;
  struct Part {
    String string;
    uint8  ptype;
  public:
    explicit            Part();
  protected:
    void                setup (uint8       ptype,
                               String      smsg);
    void                setup (uint8       ptype,
                               const char *format,
                               va_list     varargs);
  };
  struct Text0 : public Part {  /* message title */
    Text0 (const char *format, ...) G_GNUC_PRINTF (2, 3)    { va_list a; va_start (a, format); setup ('0', format, a); va_end (a); }
    Text0 (const String &s)                                 { setup ('0', s); }
  };
  struct Text1 : public Part {  /* primary message */
    Text1 (const char *format, ...) G_GNUC_PRINTF (2, 3)    { va_list a; va_start (a, format); setup ('1', format, a); va_end (a); }
    Text1 (const String &s)                                 { setup ('1', s); }
  };
  struct Text2 : public Part {  /* secondary message (lengthy) */
    Text2 (const char *format, ...) G_GNUC_PRINTF (2, 3)    { va_list a; va_start (a, format); setup ('2', format, a); va_end (a); }
    Text2 (const String &s)                                 { setup ('2', s); }
  };
  struct Text3 : public Part {  /* message details */
    Text3 (const char *format, ...) G_GNUC_PRINTF (2, 3)    { va_list a; va_start (a, format); setup ('3', format, a); va_end (a); }
    Text3 (const String &s)                                 { setup ('3', s); }
  };
  struct Check : public Part {  /* user switch */
    Check (const char *format, ...) G_GNUC_PRINTF (2, 3)    { va_list a; va_start (a, format); setup ('c', format, a); va_end (a); }
    Check (const String &s)                                 { setup ('c', s); }
  };
  typedef Text0 Title;          /* message title */
  typedef Text1 Primary;        /* primary message */
  typedef Text2 Secondary;      /* secondary message (lengthy) */
  typedef Text3 Detail;         /* message details */
  static inline void display (Type        message_type,
                              const Part &p0 = empty_part,
                              const Part &p1 = empty_part,
                              const Part &p2 = empty_part,
                              const Part &p3 = empty_part,
                              const Part &p4 = empty_part);
protected:
  static const Part   &empty_part;
  static void display (const char         *domain,
                       const vector<Part> &parts);
  /* FIXME: allow installing of handler for vector<Part&> */
  BIRNET_PRIVATE_CLASS_COPY (Msg);
};

/* --- inline implementation --- */
inline void
Msg::display (Type        message_type,
              const Part &p0,
              const Part &p1,
              const Part &p2,
              const Part &p3,
              const Part &p4)
{
  vector<Part> parts;
  parts.push_back (p0);
  parts.push_back (p1);
  parts.push_back (p2);
  parts.push_back (p3);
  parts.push_back (p4);
  display (BIRNET_LOG_DOMAIN, parts);
}

} // Birnet

#endif /* __BIRNET_MSG_HH__ */
/* vim:set ts=8 sts=2 sw=2: */
