/* BseWaveTool - BSE Wave creation tool                 -*-mode: c++;-*-
 * Copyright (C) 2001-2004 Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include <bse/gsldatahandle.h>
#include <bse/gslwavechunk.h>
#include "bseloopfuncs.h"
#include "bwtwave.hh"
#include <typeinfo>
#include <string>

namespace BseWaveTool {
using namespace std;

/* --- command + registry --- */
class Command {
public:
  const string name;
  Command (const char *command_name) :
    name (command_name)
  {
    registry.push_back (this);
  }
  virtual guint
  parse_args (guint  argc,
              char **argv)
  { return 0; }
  virtual Wave*
  create ()
  {
    return NULL;
  }
  virtual void
  exec (Wave *wave) = 0;
  virtual void
  blurb (bool bshort)
  {
    g_print ("\n");
    if (bshort)
      return;
  }
  virtual
  ~Command()
  {}
  static list<Command*> registry;
};

} // BseWaveTool
