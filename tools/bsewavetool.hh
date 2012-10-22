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
#include "bseloopfuncs.hh"
#include "bwtwave.hh"
#include <unistd.h>
#include <typeinfo>
#include <string>

namespace BseWaveTool {
using namespace std;

/* --- command + registry --- */
class Command {
public:
  const string name;
  explicit      Command    (const char *command_name);
  virtual uint  parse_args (uint   argc,
                            char **argv)        { return 0; }
  virtual Wave* create     ()                   { return NULL; }
  virtual bool  exec       (Wave *wave) = 0;
  virtual void  blurb      (bool bshort);
  virtual      ~Command    ()                   {}
  static list<Command*> registry;
};

} // BseWaveTool
