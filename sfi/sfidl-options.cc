/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Stefan Westerfeld
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
#include "sfidl-options.hh"
#include "sfidl-factory.hh"
#include "sfidl-generator.hh"
#include "topconfig.h"
#include <sfi/glib-extra.h>
#include <stdio.h>

/* FIXME: should be filled out and written into topconfig.h by configure */
#define SFIDL_VERSION        BST_VERSION
#define SFIDL_PRG_NAME	     "sfidl"

using namespace Sfidl;
using namespace std;

static Options *Options_the = 0;

Options *Options::the() {
  g_return_val_if_fail (Options_the != 0, 0);

  return Options_the;
};

Options::Options ()
{
  doHelp = doExit = false;
  sfidlName = "sfidl";
  codeGenerator = 0;

  Options_the = this;
}

bool Options::parse (int *argc_p, char **argv_p[], const Parser& parser)
{
  bool printIncludePath = false;
  bool printVersion = false;
  bool noStdInc = false;

  OptionVector codeGeneratorOptions;

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

      if (strcmp ("--help", argv[i]) == 0)
	{
	  doHelp = true;
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
      else if (!codeGenerator) /* only one code generator allowed */
	{
	  list<Factory *> factories = Factory::listFactories();

	  for (list<Factory *>::const_iterator fi = factories.begin(); fi != factories.end(); fi++)
	    {
	      Factory *factory = *fi;

	      if (argv[i] && factory->option() == argv[i])
		{
		  codeGenerator = factory->create (parser);
		  codeGeneratorName = factory->option() + " (" + factory->description() + ")";
		  codeGeneratorOptions = codeGenerator->getOptions();
		  argv[i] = NULL;
		}
	    }
	}
      else /* look for code generator specific options */
	{
	  OptionVector::iterator oi;
	  const char *value = 0;
	  for (oi = codeGeneratorOptions.begin(); oi != codeGeneratorOptions.end() && !value; oi++)
	    {
	      const String& option = oi->first;
	      const bool& needArg = oi->second;
	      String optioneq = option + "=";

	      if (option == argv[i]) // --option
		{
		  if (!needArg) // --option
		    {
		      value = "1";
		    }
		  else if (i + 1 < argc) // --option foo
		    {
		      argv[i] = NULL;
		      i += 1;
		      value = argv[i];
		    }
		}
	      else if (strncmp(optioneq.c_str(), argv[i], optioneq.size()) == 0) // --option=foo
		{
		  if (needArg)
		    value = argv[i] + optioneq.size();
		}

	      if (value)
		{
		  codeGenerator->setOption (option, value);
		  argv[i] = NULL;
		}
	    }
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
      for (std::vector<String>::const_iterator ii = includePath.begin(); ii != includePath.end(); ii++)
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

  return true;
}

void Options::printUsage ()
{
  list<Factory *> factories = Factory::listFactories();

  fprintf (stderr, "usage: %s <binding> [ <options> ] <idlfile>\n", sfidlName.c_str());
  fprintf (stderr, "\n");
  fprintf (stderr, "general options:\n");
  fprintf (stderr, " -I <directory>              add this directory to the include path\n");
  fprintf (stderr, " --print-include-path        print include path\n");
  fprintf (stderr, " --nostdinc                  don't use standard include path\n");
  fprintf (stderr, "\n");
  fprintf (stderr, " --help                      help for %s\n", sfidlName.c_str());
  fprintf (stderr, " --help <binding>            help for a specific binding\n");
  fprintf (stderr, " --version                   print version\n");
  fprintf (stderr, "\n");

  if (!codeGenerator)
    {
      fprintf (stderr, "language bindings:\n");
      for (list<Factory *>::const_iterator fi = factories.begin(); fi != factories.end(); fi++)
	fprintf (stderr, " %-28s%s\n", (*fi)->option().c_str(), (*fi)->description().c_str());
    }
  else
    {
      fprintf (stderr, "options for %s:\n", codeGeneratorName.c_str());
      codeGenerator->help();
    }
}


/* vim:set ts=8 sts=2 sw=2: */
