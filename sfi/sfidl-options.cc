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
#include "sfidl-options.h"
#include "topconfig.h"
#include <sfi/glib-extra.h>
#include <stdio.h>

/* FIXME: should be filled out and written into topconfig.h by configure */
#define SFIDL_VERSION        BST_VERSION
#define SFIDL_PRG_NAME	     "sfidl"

using namespace Sfidl;

static Options *Options_the = 0;

Options *Options::the() {
  g_return_val_if_fail (Options_the != 0, 0);

  return Options_the;
};

Options::Options ()
{
  generateExtern = generateData = generateConstant = false;
  generateTypeH = generateTypeC = false;
  generateBoxedTypes = generateProcedures = generateSignalStuff = false;
  generateIdlLineNumbers = false;
  target = TARGET_C;
  style = STYLE_DEFAULT;
  doHeader = doSource = doImplementation = doInterface = doHelp = doExit = false;
  sfidlName = "sfidl";

  Options_the = this;
}

bool Options::parse (int *argc_p, char **argv_p[])
{
  bool printIncludePath = false;
  bool printVersion = false;
  bool noStdInc = false;

  unsigned int argc;
  char **argv;
  unsigned int i, e;

  g_return_val_if_fail (argc_p != NULL, false);
  g_return_val_if_fail (argv_p != NULL, false);
  g_return_val_if_fail (*argc_p >= 0, false);

  argc = *argc_p;
  argv = *argv_p;

  if (argc && argv[0])
    sfidlName = argv[0];

  for (i = 1; i < argc; i++)
    {
      unsigned int len = 0;

      if (strcmp ("--header", argv[i]) == 0)
	{
	  doHeader = true;
	  argv[i] = NULL;
	}
      else if (strcmp ("--source", argv[i]) == 0)
	{
	  doSource = true;
	  argv[i] = NULL;
	}
      else if (strcmp ("--interface", argv[i]) == 0)
	{
	  doInterface = true;
	  argv[i] = NULL;
	}
      else if (strcmp ("--implementation", argv[i]) == 0)
	{
	  doImplementation = true;
	  argv[i] = NULL;
	}
      else if (strcmp ("--boxed", argv[i]) == 0)
	{
	  generateBoxedTypes = true;
	  argv[i] = NULL;
	}
      else if ((len = strlen("--prefix=")) &&
	  (strcmp ("--prefix", argv[i]) == 0 ||
	   strncmp ("--prefix=", argv[i], len) == 0))
	{
	  char *equal = argv[i] + len;

	  if (*equal == '=')
	    prefixC = equal + 1;
	  else if (i + 1 < argc)
	    {
	      prefixC = argv[i + 1];
	      argv[i] = NULL;
	      i += 1;
	    }
	  argv[i] = NULL;
	}
      else if (strcmp ("--qt", argv[i]) == 0)
	{
	  target = TARGET_QT;
	  argv[i] = NULL;
	}
      else if (strcmp ("--module", argv[i]) == 0)
	{
	  target = TARGET_MODULE;
          /* configure for module generation */
          doImplementation = true;
          doInterface = false;
          doHeader = true;
          doSource = false;
	  argv[i] = NULL;
	}
      else if (strcmp ("--list-types", argv[i]) == 0)
	{
	  target = TARGET_TYPELIST;
          /* configure for module generation */
          doImplementation = true;
          doInterface = false;
          doHeader = true;
          doSource = false;
	  argv[i] = NULL;
	}
      else if (strcmp ("--cxx", argv[i]) == 0)
	{
	  target = TARGET_CXX;
	  /*
	   * we will probably extend this to support both:
	   * interface and implementation
	   */
          doImplementation = false;
          doInterface = true;
	  argv[i] = 0;
	}
      else if (strcmp ("--lower", argv[i]) == 0)
	{
	  style = STYLE_LOWER;
	  argv[i] = 0;
	}
      else if (strcmp ("--mixed", argv[i]) == 0)
	{
	  style = STYLE_MIXED;
	  argv[i] = 0;
	}
      else if (strcmp ("--help", argv[i]) == 0)
	{
	  doHelp = true;
	  argv[i] = NULL;
	}
      else if (strcmp ("--idl-line-numbers", argv[i]) == 0)
	{
	  generateIdlLineNumbers = true;
	  argv[i] = NULL;
	}
      else if ((len = strlen("--namespace=")) &&
	  (strcmp ("--namespace", argv[i]) == 0 ||
	   strncmp ("--namespace=", argv[i], len) == 0))
	{
	  char *equal = argv[i] + len;

	  if (*equal == '=')
	    namespaceQt = equal + 1;
	  else if (i + 1 < argc)
	    {
	      namespaceQt = argv[i + 1];
	      argv[i] = NULL;
	      i += 1;
	    }
	  argv[i] = NULL;
	}
      else if ((len = strlen("--init=")) &&
	  (strcmp ("--init", argv[i]) == 0 ||
	   strncmp ("--init=", argv[i], len) == 0))
	{
	  char *equal = argv[i] + len;

	  if (*equal == '=')
	    initFunction = equal + 1;
	  else if (i + 1 < argc)
	    {
	      initFunction = argv[i + 1];
	      argv[i] = NULL;
	      i += 1;
	    }
	  argv[i] = NULL;
	}
      else if ((len = strlen("-I")) &&
	  (strcmp ("-I", argv[i]) == 0 ||
	   strncmp ("-I", argv[i], len) == 0))
	{
	  char *path = argv[i] + len;

	  if (*path != 0)
	    includePath.push_back (path);
	  else if (i + 1 < argc)
	    {
	      includePath.push_back (argv[i + 1]);
	      argv[i] = NULL;
	      i += 1;
	    }
	  argv[i] = NULL;
	}
      else if (strcmp ("--print-include-path", argv[i]) == 0)
	{
	  printIncludePath = true;
	  doExit = true;
	  argv[i] = NULL;
	}
      else if (strcmp ("--version", argv[i]) == 0)
	{
	  printVersion = true;
	  doExit = true;
	  argv[i] = NULL;
	}
      else if (strcmp ("--nostdinc", argv[i]) == 0)
	{
	  noStdInc = true;
	  argv[i] = NULL;
	}
    }

  /* resort argc/argv */
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
    *argc_p = e;

  /* add std include path */
  if (!noStdInc)
    {
      char *x = g_strdup (SFIDL_PATH_STDINC);
      char *dir = strtok (x, G_SEARCHPATH_SEPARATOR_S);
      while (dir && dir[0])
	{
	  includePath.push_back (dir);
	  dir = strtok (NULL, G_SEARCHPATH_SEPARATOR_S);
	}
      g_free (x);
    }

  /* option validation */

  if (doHelp)
    return true;

  if (printIncludePath)
    {
      bool first = true;
      for (std::vector<std::string>::const_iterator ii = includePath.begin(); ii != includePath.end(); ii++)
	{
	  if (!first)
	    printf (":");
	  else
	    first = false;
	  printf ("%s", ii->c_str());
	}
      printf ("\n");
      return true;
    }

  if (printVersion)
    {
      printf ("%s %s\n", SFIDL_PRG_NAME, SFIDL_VERSION);
      return true;
    }


  // exactly one of --source | --header, --interface | --implementation
  if (((doSource?1:0) + (doHeader?1:0)) != 1)
    {
      fprintf (stderr, "%s: need --source or --header option\n", sfidlName.c_str());
      return false;
    }
  if (((doInterface?1:0) + (doImplementation?1:0)) != 1)
    {
      fprintf (stderr, "%s: need --interface or --implementation option\n", sfidlName.c_str());
      return false;
    }

  // init function
  if (doHeader && initFunction != "")
    {
      fprintf (stderr, "%s: --init is not required for headers\n", sfidlName.c_str());
      return false;
    }
  if (doInterface && initFunction != "")
    {
      fprintf (stderr, "%s: --init is not required for interfacing code\n", sfidlName.c_str());
      return false;
    }

  // style

  if (style == STYLE_DEFAULT && target == TARGET_QT)
    style = STYLE_MIXED;

  if (style == STYLE_DEFAULT)
    style = STYLE_LOWER;

  // --qt
  if (target == TARGET_QT && doImplementation)
    {
      fprintf (stderr, "%s: --implementation is not supported for Qt\n", sfidlName.c_str());
      return false;
    }

  // --module
  if (target == TARGET_MODULE && doInterface)
    {
      fprintf (stderr, "%s: --interface is not supported for Module\n", sfidlName.c_str());
      return false;
    }

  /* implications of header/source options */
  if (doHeader)
    {
      generateExtern = true;
      generateTypeH = true;
      generateConstant = false; // useless
    }

  if (doSource)
    {
      if (doImplementation)
	{
	  generateData = true;
	  generateTypeC = true;
	}

      if (doInterface)
	{
	  generateProcedures = true;
	  generateTypeC = true;
	}
    }

  return true;
}

void Options::printUsage ()
{
  fprintf (stderr, "usage: %s [ <options> ] <idlfile>\n", sfidlName.c_str());
  fprintf (stderr, "\n");
  fprintf (stderr, "general options:\n");
  fprintf (stderr, " --header                    generate header file\n");
  fprintf (stderr, " --source                    generate source file\n");
  fprintf (stderr, "\n");
  fprintf (stderr, " --interface                 generate interface\n");
  fprintf (stderr, " --implementation            generate implementation\n");
  fprintf (stderr, "\n");
  fprintf (stderr, " --init <name>               set the name of the init function\n");
  fprintf (stderr, " --idl-line-numbers          generate #line directives relative to .sfidl file\n");
  fprintf (stderr, " -I <directory>              add this directory to the include path\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "options for the C language binding:\n");
  fprintf (stderr, " --boxed                     generate glib boxed types (bse specific!)\n");
  fprintf (stderr, " --prefix <prefix>           set the prefix for c functions\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "options for the C++ language binding:\n");
  fprintf (stderr, " --qt                        use Qt language binding\n");
  fprintf (stderr, " --cxx                       use C++ language binding\n");
  fprintf (stderr, " --module                    generate skeleton Module implementation\n");
  fprintf (stderr, " --namespace <namespace>     set the namespace to use for the code\n");
  fprintf (stderr, " --mixed                     mixed case identifiers (createMidiSynth)\n");
  fprintf (stderr, " --lower                     lower case identifiers (create_midi_synth)\n");
  fprintf (stderr, "\n");
  fprintf (stderr, " --help                      this help\n");
  fprintf (stderr, " --version                   print version\n");
  fprintf (stderr, " --print-include-path        print include path\n");
  fprintf (stderr, " --list-types                print all types defined in the idlfile\n");
  fprintf (stderr, " --nostdinc                  don't use standard include path\n");
}


/* vim:set ts=8 sts=2 sw=2: */
