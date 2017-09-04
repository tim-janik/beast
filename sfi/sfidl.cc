// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "sfidl-generator.hh"
#include "sfidl-options.hh"
#include "sfidl-parser.hh"

#define app_error(...)  do { g_printerr ("%s: ", g_get_prgname()); g_printerr (__VA_ARGS__); exit (-1); } while (0)

using namespace Sfidl;

int main (int argc, char **argv)
{
  Bse::assertion_failed_hook ([&] () { Bse::breakpoint(); });

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

#include "sfidl-generator.cc"
#include "sfidl-namespace.cc"
#include "sfidl-options.cc"
#include "sfidl-parser.cc"
#include "sfidl-factory.cc"
#include "sfidl-typelist.cc"
#include "sfidl-cbase.cc"
#include "sfidl-clientc.cc"
#include "sfidl-clientcxx.cc"
#include "sfidl-corec.cc"
#include "sfidl-corecxx.cc"
#include "sfidl-cxxbase.cc"
#include "sfidl-hostc.cc"
#include "sfidl-utils.cc"
#include "formatter.cc"

/* vim:set ts=8 sts=2 sw=2: */
