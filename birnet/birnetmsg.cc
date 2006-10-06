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
#include <glib.h>
#include "birnetmsg.hh"

namespace Birnet {

Msg::Part::Part() :
  ptype (0)
{}

void
Msg::Part::setup (uint8       _ptype,
                  String      smsg)
{
  ptype = _ptype;
  string = smsg;
}

void
Msg::Part::setup (uint8       _ptype,
                  const char *format,
                  va_list     varargs)
{
  char *s = g_strdup_vprintf (format, varargs);
  setup (_ptype, String (s));
  g_free (s);
}

const Msg::Part &Msg::empty_part = Part();

void
Msg::display (const char         *domain,
              const vector<Part> &parts)
{
  // FIXME: implement Msg::display()
}

} // Birnet
