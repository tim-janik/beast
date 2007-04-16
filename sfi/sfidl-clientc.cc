/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002-2007 Stefan Westerfeld
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
#include "sfidl-clientc.hh"
#include "sfidl-factory.hh"
#include "sfidl-namespace.hh"
#include "sfidl-options.hh"
#include "sfidl-parser.hh"
#include <stdio.h>

using namespace Sfidl;
using namespace std;

void CodeGeneratorClientC::printClassMacros()
{
  for (vector<Class>::const_iterator ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
    {
      if (parser.fromInclude (ci->name)) continue;

      String macro = makeUpperName (NamespaceHelper::namespaceOf (ci->name)) + "_IS_" +
	makeUpperName (NamespaceHelper::nameOf (ci->name));
      String mname = makeMixedName (ci->name);

      printf ("#define %s(proxy) bse_proxy_is_a ((proxy), \"%s\")\n",
	  macro.c_str(), mname.c_str());
    }
  printf("\n");
}

Method CodeGeneratorClientC::methodWithObject (const Class& c, const Method& method)
{
  Method md;
  md.name = method.name;
  md.result = method.result;

  Param class_as_param;
  class_as_param.name = makeLowerName(c.name) + "_object";
  class_as_param.type = c.name;
  md.params.push_back (class_as_param);

  for (vector<Param>::const_iterator pi = method.params.begin(); pi != method.params.end(); pi++)
    md.params.push_back (*pi);

  return md;
}

void CodeGeneratorClientC::printProcedurePrototypes (PrefixSymbolMode mode)
{
  vector<Class>::const_iterator ci;
  vector<Method>::const_iterator mi;

  for (ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
    {
      if (parser.fromInclude (ci->name)) continue;

      for (mi = ci->methods.begin(); mi != ci->methods.end(); mi++)
	{
	  if (mode == generatePrefixSymbols)
	    prefix_symbols.push_back (makeLowerName (ci->name + "_" + mi->name));
	  else
	    printProcedure (methodWithObject (*ci, *mi), true, ci->name);
	}
    }
  for (mi = parser.getProcedures().begin(); mi != parser.getProcedures().end(); mi++)
    {
      if (parser.fromInclude (mi->name)) continue;

      if (mode == generatePrefixSymbols)
	prefix_symbols.push_back (makeLowerName (mi->name));
      else
	printProcedure (*mi, true);
    }
}

void CodeGeneratorClientC::printProcedureImpl ()
{
  vector<Class>::const_iterator ci;
  vector<Method>::const_iterator mi;

  for (ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
    {
      if (parser.fromInclude (ci->name)) continue;

      for (mi = ci->methods.begin(); mi != ci->methods.end(); mi++)
	printProcedure (methodWithObject (*ci, *mi), false, ci->name);
    }
  for (mi = parser.getProcedures().begin(); mi != parser.getProcedures().end(); mi++)
    {
      if (parser.fromInclude (mi->name)) continue;

      printProcedure (*mi, false);
    }
}

void
CodeGeneratorClientC::addBindingSpecificFiles (const String& binding_specific_file)
{
  vector<Pragma> pragmas = parser.getPragmas ("ClientC");

  for (vector<Pragma>::iterator pi = pragmas.begin(); pi != pragmas.end(); pi++)
  {
    if (pi->fromInclude) continue;

    String filename;
    if (pi->getString (binding_specific_file, filename))
      {
	gchar *directory = g_path_get_dirname (pi->filename.c_str());
	filename = directory + String (G_DIR_SEPARATOR_S) + filename;
	g_free (directory);

	printf ("/* %s: including binding specific file \"%s\", as requested in %s:%d */\n",
	        options.sfidlName.c_str(), filename.c_str(), pi->filename.c_str(), pi->line);
	FILE *f = fopen (filename.c_str(), "r");
	if (f)
	  {
	    char buffer[1024];
	    int bytes;
	    while ((bytes = fread (buffer, 1, sizeof (buffer), f)) > 0)
	      fwrite (buffer, 1, bytes, stdout);
	    fclose (f);
	  }
	else
	  {
	    fprintf (stderr, "binding specific file '%s' not found.\n", filename.c_str());
	    exit (1);
	  }
      }
  }
}

bool CodeGeneratorClientC::run()
{
  printf("\n/*-------- begin %s generated code --------*/\n\n\n", options.sfidlName.c_str());

  if (generateHeader)
    {
      /* namespace prefixing for symbols defined by the client */

      prefix_symbols.clear();

      printClientChoiceConverterPrototypes (generatePrefixSymbols);
      printClientRecordMethodPrototypes (generatePrefixSymbols);
      printClientSequenceMethodPrototypes (generatePrefixSymbols);
      printProcedurePrototypes (generatePrefixSymbols);

      if (prefix != "")
	{
	  for (vector<String>::const_iterator pi = prefix_symbols.begin(); pi != prefix_symbols.end(); pi++)
	    printf("#define %s %s_%s\n", pi->c_str(), prefix.c_str(), pi->c_str());
	  printf("\n");
	}

      /* generate the header */

      printClientRecordPrototypes();
      printClientSequencePrototypes();

      printClientChoiceDefinitions();
      printClientRecordDefinitions();
      printClientSequenceDefinitions();

      printClientRecordMethodPrototypes (generateOutput);
      printClientSequenceMethodPrototypes (generateOutput);
      printClientChoiceConverterPrototypes (generateOutput);
      printProcedurePrototypes (generateOutput);

      printClassMacros();
      addBindingSpecificFiles ("binding_specific_c_header");
    }

  if (generateSource)
    {
      printf("#include <string.h>\n");

      printClientRecordMethodImpl();
      printClientSequenceMethodImpl();
      printChoiceConverters();
      printProcedureImpl();
      addBindingSpecificFiles ("binding_specific_c_source");
    }

  printf("\n/*-------- end %s generated code --------*/\n\n\n", options.sfidlName.c_str());
  return true;
}

OptionVector
CodeGeneratorClientC::getOptions()
{
  OptionVector opts = CodeGeneratorCBase::getOptions();

  opts.push_back (make_pair ("--prefix", true));

  return opts;
}

void
CodeGeneratorClientC::setOption (const String& option, const String& value)
{
  if (option == "--prefix")
    {
      prefix = value;
    }
  else
    {
      CodeGeneratorCBase::setOption (option, value);
    }
}

void
CodeGeneratorClientC::help ()
{
  CodeGeneratorCBase::help();
  fprintf (stderr, " --prefix <prefix>           set the prefix for C functions\n");
}

namespace {

class ClientCFactory : public Factory {
public:
  String option() const	      { return "--client-c"; }
  String description() const  { return "generate client C language binding"; }
  
  CodeGenerator *create (const Parser& parser) const
  {
    return new CodeGeneratorClientC (parser);
  }
} client_c_factory;

}

/* vim:set ts=8 sts=2 sw=2: */
