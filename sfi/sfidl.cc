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
#include "sfidl-namespace.h"
#include "sfidl-options.h"
#include "sfidl-parser.h"
#include "sfidl-module.h"
#include "sfidl-cxx.h"
#include "sfiparams.h" /* scatId (SFI_SCAT_*) */
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

using namespace Sfidl;
using namespace std;



int main (int argc, char **argv)
{
  Options options;
  if (!options.parse (&argc, &argv))
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

  if((argc-optind) != 1)
    {
      options.printUsage ();
      return 1;
    }

  Parser parser;
  if (!parser.parse(argv[1]))
    {
      /* parse error */
      return 1;
    }

  CodeGenerator *codeGenerator = 0;
  switch (options.target)
    {
      case Options::TARGET_C:
	codeGenerator = new CodeGeneratorC (parser);
	break;
      case Options::TARGET_QT:
	codeGenerator = new CodeGeneratorQt (parser);
	break;
      case Options::TARGET_CXX:
	codeGenerator = new CodeGeneratorCxx (parser);
	break;
      case Options::TARGET_MODULE:
	codeGenerator = new CodeGeneratorModule (parser);
	break;
    }
  if (!codeGenerator)
    {
      fprintf(stderr, "no target given\n");
      return 1;
    }

  printf("\n/*-------- begin %s generated code --------*/\n\n\n",argv[0]);
  codeGenerator->run ();
  printf("\n\n/*-------- end %s generated code --------*/\n\n\n",argv[0]);

  delete codeGenerator;
  return 0;
}

/* vim:set ts=8 sts=2 sw=2: */
