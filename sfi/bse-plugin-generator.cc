/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2003 Tim Janik
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
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include "sfidl-namespace.h"
#include "sfidl-options.h"
#include "sfidl-parser.h"
#include "sfidl-module.h"
#include "sfiparams.h" /* scatId (SFI_SCAT_*) */

#define PRG_NAME "bse-module-generator"

using namespace Sfidl;
using namespace std;

static void
usage ()
{
  g_print ("usage: %s [options] <input.mdef>\n", PRG_NAME);
  g_print (" -h, --help                  print usage information\n");
  g_print (" -I <directory>              add this directory to the include path\n");
}


int
main (int   argc,
      char *argv[])
{
  Options options;      // need options singleton
  gint i, e;

  /* roll own option parsing as Options is sfidl specific */
  for (i = 1; i < argc; i++)
    {
      if (strcmp ("-h", argv[i]) == 0 ||
          strcmp ("--help", argv[i]) == 0)
        {
          usage ();
          argv[i] = NULL;
          exit (0);
        }
      else if (strcmp ("-I", argv[i]) == 0 ||
               strncmp ("-I", argv[i], 2) == 0)
        {
          gchar *dir = argv[i] + 2;
          if (*dir)
            options.includePath.push_back (dir);
          else if (i + 1 < argc)
            {
              argv[i++] = NULL;
              options.includePath.push_back (argv[i]);
            }
          argv[i] = NULL;
        }
    }

  /* collapse parsed options */
  e = 0;
  for (i = 1; i < argc; i++)
    {
      if (e)
        {
          if (argv[i])
            {
              argv[e++] = argv[i];
              argv[i] = NULL;
            }
        }
      else if (!argv[i])
        e = i;
    }
  if (e)
    argc = e;

  /* verify file presence */
  if (argc != 2)
    {
      g_printerr ("%s: %s\n", argv[0],
                  argc < 2
                  ? "missing module definition file"
                  : "too many file names specified");
      usage();
      return 1;
    }

  Parser parser;
  if (!parser.parse (argv[1]))
    return 1;

  CodeGeneratorModule cg (parser);

  g_print ("\n/*-------- begin %s generated code --------*/\n\n\n", argv[0]);
  cg.run();
  g_print ("\n\n/*-------- end %s generated code --------*/\n\n\n", argv[0]);

  return 0;
}

/* vim:set ts=8 sts=2 sw=2: */
