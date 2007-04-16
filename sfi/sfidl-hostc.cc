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
#include "sfidl-hostc.hh"
#include "sfidl-factory.hh"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include "sfidl-namespace.hh"
#include "sfidl-options.hh"
#include "sfidl-parser.hh"

using namespace Sfidl;
using std::make_pair;

void CodeGeneratorHostC::printRecordFieldDeclarations()
{
  /*
   * data for init functions (since it is possible to use different C files
   * for defining records/sequences which depend on each other, this data
   * must be declared extern)
   */
  for(vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
    {
      if (parser.fromInclude (ri->name)) continue;

      printf("extern SfiRecFields %s_fields;\n",makeLowerName (ri->name).c_str());
    }
  printf("\n");
}

void CodeGeneratorHostC::printInitFunction (const String& initFunction)
{
  /*
   * data for init function
   */
  for(vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
    {
      if (parser.fromInclude (ri->name)) continue;

      String name = makeLowerName (ri->name);

      printf("static GParamSpec *%s_field[%zd];\n", name.c_str(), ri->contents.size());
      printf("SfiRecFields %s_fields = { %zd, %s_field };\n", name.c_str(), ri->contents.size(), name.c_str());
      printf("\n");
    }
  for(vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
    {
      if (parser.fromInclude (si->name)) continue;

      String name = makeLowerName (si->name);

      printf("static GParamSpec *%s_content;\n", name.c_str());
      printf("\n");
    }

  /*
   * the init function itself
   */
  bool first = true;
  printf("static void\n%s (void)\n", initFunction.c_str());
  printf("{\n");

  /*
   * It is important to follow the declaration order of the idl file here, as for
   * instance a Param inside a record might come from a sequence, and a Param
   * inside a Sequence might come from a record - to avoid using yet-unitialized
   * Params, we follow the getTypes() 
   */
  vector<String>::const_iterator ti;

  for(ti = parser.getTypes().begin(); ti != parser.getTypes().end(); ti++)
    {
      if (parser.fromInclude (*ti)) continue;

      if (parser.isRecord (*ti) || parser.isSequence (*ti))
	{
	  if(!first) printf("\n");
	  first = false;
	}
      if (parser.isRecord (*ti))
	{
	  const Record& rdef = parser.findRecord (*ti);

	  String name = makeLowerName (rdef.name);
	  int f = 0;

	  for (vector<Param>::const_iterator pi = rdef.contents.begin(); pi != rdef.contents.end(); pi++, f++)
	    {
	      printf("#line %u \"%s\"\n", pi->line, parser.fileName().c_str());
	      printf("  %s_field[%d] = %s;\n", name.c_str(), f, makeParamSpec (*pi).c_str());
	    }
	}
      if (parser.isSequence (*ti))
	{
	  const Sequence& sdef = parser.findSequence (*ti);

	  String name = makeLowerName (sdef.name);

	  printf("#line %u \"%s\"\n", sdef.content.line, parser.fileName().c_str());
	  printf("  %s_content = %s;\n", name.c_str(), makeParamSpec (sdef.content).c_str());
	}
    }
  printf("}\n");
}

void CodeGeneratorHostC::printChoiceMethodPrototypes (PrefixSymbolMode mode)
{
  for(vector<Choice>::const_iterator ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
    {
      if (parser.fromInclude (ei->name)) continue;

      if (mode == generatePrefixSymbols)
	prefix_symbols.push_back (makeLowerName (ei->name) + "_get_values");
      else
	printf("SfiChoiceValues %s_get_values (void);\n", makeLowerName (ei->name).c_str());
    }
}

void CodeGeneratorHostC::printChoiceMethodImpl()
{
  int enumCount = 0;
  for(vector<Choice>::const_iterator ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
    {
      if (parser.fromInclude (ei->name))
	continue;

      String name = makeLowerName (ei->name);

      printf ("SfiChoiceValues\n");
      printf ("%s_get_values (void)\n", makeLowerName (ei->name).c_str());
      printf ("{\n");
      printf ("  static SfiChoiceValue values[%zu];\n", ei->contents.size());
      printf ("  static const SfiChoiceValues choice_values = {\n");
      printf ("    G_N_ELEMENTS (values), values,\n");
      printf ("  };\n");
      printf ("  if (!values[0].choice_ident)\n");
      printf ("    {\n");
      int i = 0;
      for (vector<ChoiceValue>::const_iterator vi = ei->contents.begin(); vi != ei->contents.end(); i++, vi++)
	{
	  printf ("      values[%u].choice_ident = \"%s\";\n", i, makeUpperName (vi->name).c_str());
	  printf ("      values[%u].choice_label = %s;\n", i, vi->label.escaped().c_str());
	  printf ("      values[%u].choice_blurb = %s;\n", i, vi->blurb.escaped().c_str());
	}
      printf ("  }\n");
      printf ("  return choice_values;\n");
      printf ("}\n");
      printf("\n");

      enumCount++;
    }
}

bool CodeGeneratorHostC::run ()
{
  printf("\n/*-------- begin %s generated code --------*/\n\n\n", options.sfidlName.c_str());

  if (generateHeader)
    {
      /* namespace prefixing */

      prefix_symbols.clear();

      printClientChoiceConverterPrototypes (generatePrefixSymbols);
      printClientRecordMethodPrototypes (generatePrefixSymbols);
      printClientSequenceMethodPrototypes (generatePrefixSymbols);
      printChoiceMethodPrototypes (generatePrefixSymbols);

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
      printChoiceMethodPrototypes (generateOutput);

      printRecordFieldDeclarations();
    }

  if (generateSource)
    {
      printf("#include <string.h>\n");

      printClientRecordMethodImpl();
      printClientSequenceMethodImpl();
      printChoiceMethodImpl();
      printChoiceConverters();

      if (generateInitFunction != "")
	printInitFunction (generateInitFunction);
    }

  printf("\n/*-------- end %s generated code --------*/\n\n\n", options.sfidlName.c_str());
  return true;
}

OptionVector
CodeGeneratorHostC::getOptions()
{
  OptionVector opts = CodeGeneratorCBase::getOptions();

  opts.push_back (make_pair ("--prefix", true));
  opts.push_back (make_pair ("--init", true));

  return opts;
}

void
CodeGeneratorHostC::setOption (const String& option, const String& value)
{
  if (option == "--prefix")
    {
      prefix = value;
    }
  else if (option == "--init")
    {
      generateInitFunction = value;
    }
  else
    {
      CodeGeneratorCBase::setOption (option, value);
    }
}

void
CodeGeneratorHostC::help()
{
  CodeGeneratorCBase::help();
  fprintf (stderr, " --prefix <prefix>           set the prefix for C functions\n");
  fprintf (stderr, " --init <name>               set the name of the init function\n");
}

namespace {

class HostCFactory : public Factory {
public:
  String option() const	      { return "--host-c"; }
  String description() const  { return "generate host C language binding"; }
  
  CodeGenerator *create (const Parser& parser) const
  {
    return new CodeGeneratorHostC (parser);
  }
} host_c_factory;

}

/* vim:set ts=8 sts=2 sw=2: */
