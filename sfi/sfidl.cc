// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "sfidl-generator.hh"
#include "sfidl-options.hh"
#include "sfidl-parser.hh"

using namespace Sfidl;

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
