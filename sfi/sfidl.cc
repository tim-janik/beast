/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Stefan Westerfeld
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
#include "sfidl-generator.h"
#include "sfidl-options.h"
#include "sfidl-parser.h"

using namespace Sfidl;
using namespace std;

int main (int argc, char **argv)
{
  Options options;
  Parser parser;

  if (!options.parse (&argc, &argv, parser))
    {
      /* invalid options */
      return 1;
    }

  if (options.doExit)
    {
      return 0;
    }

  if (options.doHelp)
    {
      options.printUsage ();
      return 0;
    }

  if (argc != 2)
    {
      options.printUsage ();
      return 1;
    }

  if (!options.codeGenerator)
    {
      options.printUsage();
      return 1;
    }

  if (!parser.parse(argv[1]))
    {
      /* parse error */
      return 1;
    }

  if (!options.codeGenerator->run ())
    {
      delete options.codeGenerator;
      return 1;
    }

  delete options.codeGenerator;
  return 0;
}

/* vim:set ts=8 sts=2 sw=2: */
