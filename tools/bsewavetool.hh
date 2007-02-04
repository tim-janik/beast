/* BseWaveTool - BSE Wave creation tool
 * Copyright (C) 2001-2004 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
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
