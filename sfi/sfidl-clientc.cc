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
#include "sfidl-clientc.h"
#include "sfidl-factory.h"
#include "sfidl-namespace.h"
#include "sfidl-options.h"
#include "sfidl-parser.h"
#include <stdio.h>

using namespace Sfidl;
using namespace std;

void CodeGeneratorClientC::printRecordPrototypes()
{
  for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
    {
      if (parser.fromInclude (ri->name)) continue;

      string mname = makeMixedName (ri->name);
      printf("typedef struct _%s %s;\n", mname.c_str(), mname.c_str());
    }
}

void CodeGeneratorClientC::printSequencePrototypes()
{
  for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
    {
      if (parser.fromInclude (si->name)) continue;

      string mname = makeMixedName (si->name);
      printf("typedef struct _%s %s;\n", mname.c_str(), mname.c_str());
    }
}

void CodeGeneratorClientC::printRecordDefinitions()
{
  for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
    {
      if (parser.fromInclude (ri->name)) continue;

      string mname = makeMixedName (ri->name.c_str());

      printf("struct _%s {\n", mname.c_str());
      for (vector<Param>::const_iterator pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
	{
	  printf("  %s %s;\n", cTypeField (pi->type), pi->name.c_str());
	}
      printf("};\n");
    }
  printf("\n");
}

void CodeGeneratorClientC::printSequenceDefinitions()
{
  for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
    {
      if (parser.fromInclude (si->name)) continue;

      string mname = makeMixedName (si->name.c_str());
      string array = typeArray (si->content.type);
      string elements = si->content.name;

      printf("struct _%s {\n", mname.c_str());
      printf("  guint n_%s;\n", elements.c_str ());
      printf("  %s %s;\n", array.c_str(), elements.c_str());
      printf("};\n");
    }
}

void CodeGeneratorClientC::printRecordMethodPrototypes (PrefixSymbolMode mode)
{
  for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
    {
      if (parser.fromInclude (ri->name)) continue;

      string ret = typeRet (ri->name);
      string arg = typeArg (ri->name);
      string lname = makeLowerName (ri->name.c_str());

      if (mode == generatePrefixSymbols)
	{
	  prefix_symbols.push_back (lname + "_new");
	  prefix_symbols.push_back (lname + "_copy_shallow");
	  prefix_symbols.push_back (lname + "_from_rec");
	  prefix_symbols.push_back (lname + "_to_rec");
	  prefix_symbols.push_back (lname + "_free");
	}
      else
	{
	  printf("%s %s_new (void);\n", ret.c_str(), lname.c_str());
	  printf("%s %s_copy_shallow (%s rec);\n", ret.c_str(), lname.c_str(), arg.c_str());
	  printf("%s %s_from_rec (SfiRec *sfi_rec);\n", ret.c_str(), lname.c_str());
	  printf("SfiRec *%s_to_rec (%s rec);\n", lname.c_str(), arg.c_str());
	  printf("void %s_free (%s rec);\n", lname.c_str(), arg.c_str());
	  printf("\n");
	}
    }
  printf("\n");
}

void CodeGeneratorClientC::printSequenceMethodPrototypes (PrefixSymbolMode mode)
{
  for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
    {
      if (parser.fromInclude (si->name)) continue;

      string ret = typeRet (si->name);
      string arg = typeArg (si->name);
      string element = typeArg (si->content.type);
      string lname = makeLowerName (si->name.c_str());

      if (mode == generatePrefixSymbols)
	{
	  prefix_symbols.push_back (lname + "_new");
	  prefix_symbols.push_back (lname + "_append");
	  prefix_symbols.push_back (lname + "_copy_shallow");
	  prefix_symbols.push_back (lname + "_from_seq");
	  prefix_symbols.push_back (lname + "_to_seq");
	  prefix_symbols.push_back (lname + "_resize");
	  prefix_symbols.push_back (lname + "_free");
	}
      else
	{
	  printf("%s %s_new (void);\n", ret.c_str(), lname.c_str());
	  printf("void %s_append (%s seq, %s element);\n", lname.c_str(), arg.c_str(), element.c_str());
	  printf("%s %s_copy_shallow (%s seq);\n", ret.c_str(), lname.c_str(), arg.c_str());
	  printf("%s %s_from_seq (SfiSeq *sfi_seq);\n", ret.c_str(), lname.c_str());
	  printf("SfiSeq *%s_to_seq (%s seq);\n", lname.c_str(), arg.c_str());
	  printf("void %s_resize (%s seq, guint new_size);\n", lname.c_str(), arg.c_str());
	  printf("void %s_free (%s seq);\n", lname.c_str(), arg.c_str());
	  printf("\n");
	}
    }
}

void CodeGeneratorClientC::printRecordMethodImpl()
{
  vector<Param>::const_iterator pi;

  for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
    {
      if (parser.fromInclude (ri->name)) continue;

      string ret = typeRet (ri->name);
      string arg = typeArg (ri->name);
      string lname = makeLowerName (ri->name.c_str());
      string mname = makeMixedName (ri->name.c_str());

      printf("%s\n", ret.c_str());
      printf("%s_new (void)\n", lname.c_str());
      printf("{\n");
      printf("  %s rec = g_new0 (%s, 1);\n", arg.c_str(), mname.c_str());
      for (pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
	{
	  /* FIXME(tim): this needs to be much more versatile, so we can e.g. change
	   * whether record fields are NULL initialized (need for category->icon)
	   *
	   * FIXME(stw): probably all record fields will be NULL initialized (thats the
	   * way we do it in the C++ language binding)
	   */
	  string init = funcNew (pi->type);
	  if (init != "") printf("  rec->%s = %s();\n", pi->name.c_str(), init.c_str());
	}
      printf("  return rec;\n");
      printf("}\n\n");

      printf("%s\n", ret.c_str());
      printf("%s_copy_shallow (%s rec)\n", lname.c_str(), arg.c_str());
      printf("{\n");
      printf("  %s rec_copy;\n", arg.c_str());
      printf("  if (!rec)\n");
      printf("    return NULL;");
      printf("\n");
      printf("  rec_copy = g_new0 (%s, 1);\n", mname.c_str());
      for (pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
	{
	  /* FIXME(tim): this needs to be more versatile, so NULL fields can be special
	   * cased before copying */
	  string copy =  funcCopy (pi->type);
	  printf("  rec_copy->%s = %s (rec->%s);\n", pi->name.c_str(), copy.c_str(),
	      pi->name.c_str());
	}
      printf("  return rec_copy;\n");
      printf("}\n\n");

      printf("%s\n", ret.c_str());
      printf("%s_from_rec (SfiRec *sfi_rec)\n", lname.c_str());
      printf("{\n");
      printf("  GValue *element;\n");
      printf("  %s rec;\n", arg.c_str());
      printf("  if (!sfi_rec)\n");
      printf("    return NULL;\n");
      printf("\n");
      printf("  rec = g_new0 (%s, 1);\n", mname.c_str());
      for (pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
	{
	  string elementFromValue = createTypeCode (pi->type, "element", MODEL_FROM_VALUE);
	  string init = funcNew (pi->type);

	  printf("  element = sfi_rec_get (sfi_rec, \"%s\");\n", pi->name.c_str());
	  printf("  if (element)\n");
	  printf("    rec->%s = %s;\n", pi->name.c_str(), elementFromValue.c_str());

	  if (init != "")
	    {
	      printf("  else\n");
	      printf("    rec->%s = %s();\n", pi->name.c_str(), init.c_str());
	    }
	}
      printf("  return rec;\n");
      printf("}\n\n");

      printf("SfiRec *\n");
      printf("%s_to_rec (%s rec)\n", lname.c_str(), arg.c_str());
      printf("{\n");
      printf("  SfiRec *sfi_rec;\n");
      printf("  GValue *element;\n");
      printf("  if (!rec)\n");
      printf("    return NULL;\n");
      printf("\n");
      printf("  sfi_rec = sfi_rec_new ();\n");
      for (pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
	{
	  string elementToValue = createTypeCode (pi->type, "rec->" + pi->name, MODEL_TO_VALUE);
	  printf("  element = %s;\n", elementToValue.c_str());
	  printf("  sfi_rec_set (sfi_rec, \"%s\", element);\n", pi->name.c_str());
	  printf("  sfi_value_free (element);\n");        // FIXME: couldn't we have take_set
	}
      printf("  return sfi_rec;\n");
      printf("}\n\n");

      printf("void\n");
      printf("%s_free (%s rec)\n", lname.c_str(), arg.c_str());
      printf("{\n");
      printf("  g_return_if_fail (rec != NULL);\n");
      /* FIXME (tim): should free functions generally demand non-NULL structures? */
      printf("  \n");
      for (pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
	{
	  /* FIXME (tim): needs to be more verstaile, so NULL fields can be properly special cased */
	  // FIXME (stw): there _should_ be no NULL fields in some cases (sequences)!
	  string free = funcFree (pi->type);
	  if (free != "") printf("  if (rec->%s) %s (rec->%s);\n",
	      pi->name.c_str(), free.c_str(), pi->name.c_str());
	}
      printf("  g_free (rec);\n");
      printf("}\n\n");
      printf("\n");
    }
}

void CodeGeneratorClientC::printSequenceMethodImpl()
{
  for(vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
    {
      if (parser.fromInclude (si->name)) continue;

      string ret = typeRet (si->name);
      string arg = typeArg (si->name);
      string element = typeArg (si->content.type);
      string elements = si->content.name;
      string lname = makeLowerName (si->name.c_str());
      string mname = makeMixedName (si->name.c_str());

      printf("%s\n", ret.c_str());
      printf("%s_new (void)\n", lname.c_str());
      printf("{\n");
      printf("  return g_new0 (%s, 1);\n",mname.c_str());
      printf("}\n\n");

      string elementCopy = funcCopy (si->content.type);
      printf("void\n");
      printf("%s_append (%s seq, %s element)\n", lname.c_str(), arg.c_str(), element.c_str());
      printf("{\n");
      printf("  g_return_if_fail (seq != NULL);\n");
      printf("\n");
      printf("  seq->%s = g_realloc (seq->%s, "
	  "(seq->n_%s + 1) * sizeof (seq->%s[0]));\n",
	  elements.c_str(), elements.c_str(), elements.c_str(), elements.c_str());
      printf("  seq->%s[seq->n_%s++] = %s (element);\n", elements.c_str(), elements.c_str(),
	  elementCopy.c_str());
      printf("}\n\n");

      printf("%s\n", ret.c_str());
      printf("%s_copy_shallow (%s seq)\n", lname.c_str(), arg.c_str());
      printf("{\n");
      printf("  %s seq_copy;\n", arg.c_str ());
      printf("  guint i;\n");
      printf("  if (!seq)\n");
      printf("    return NULL;\n");
      printf("\n");
      printf("  seq_copy = %s_new ();\n", lname.c_str());
      printf("  for (i = 0; i < seq->n_%s; i++)\n", elements.c_str());
      printf("    %s_append (seq_copy, seq->%s[i]);\n", lname.c_str(), elements.c_str());
      printf("  return seq_copy;\n");
      printf("}\n\n");

      string elementFromValue = createTypeCode (si->content.type, "element", MODEL_FROM_VALUE);
      printf("%s\n", ret.c_str());
      printf("%s_from_seq (SfiSeq *sfi_seq)\n", lname.c_str());
      printf("{\n");
      printf("  %s seq;\n", arg.c_str());
      printf("  guint i, length;\n");
      printf("\n");
      printf("  g_return_val_if_fail (sfi_seq != NULL, NULL);\n");
      printf("\n");
      printf("  length = sfi_seq_length (sfi_seq);\n");
      printf("  seq = g_new0 (%s, 1);\n",mname.c_str());
      printf("  seq->n_%s = length;\n", elements.c_str());
      printf("  seq->%s = g_malloc (seq->n_%s * sizeof (seq->%s[0]));\n\n",
	  elements.c_str(), elements.c_str(), elements.c_str());
      printf("  for (i = 0; i < length; i++)\n");
      printf("    {\n");
      printf("      GValue *element = sfi_seq_get (sfi_seq, i);\n");
      printf("      seq->%s[i] = %s;\n", elements.c_str(), elementFromValue.c_str());
      printf("    }\n");
      printf("  return seq;\n");
      printf("}\n\n");

      string elementToValue = createTypeCode (si->content.type, "seq->" + elements + "[i]", MODEL_TO_VALUE);
      printf("SfiSeq *\n");
      printf("%s_to_seq (%s seq)\n", lname.c_str(), arg.c_str());
      printf("{\n");
      printf("  SfiSeq *sfi_seq;\n");
      printf("  guint i;\n");
      printf("\n");
      printf("  g_return_val_if_fail (seq != NULL, NULL);\n");
      printf("\n");
      printf("  sfi_seq = sfi_seq_new ();\n");
      printf("  for (i = 0; i < seq->n_%s; i++)\n", elements.c_str());
      printf("    {\n");
      printf("      GValue *element = %s;\n", elementToValue.c_str());
      printf("      sfi_seq_append (sfi_seq, element);\n");
      printf("      sfi_value_free (element);\n");        // FIXME: couldn't we have take_append
      printf("    }\n");
      printf("  return sfi_seq;\n");
      printf("}\n\n");

      // FIXME: we should check whether we _really_ need to deal with a seperate free_check
      //        function here, as it needs to be specialcased everywhere
      //
      //        especially in some cases (sequences of sequences) we will free invalid
      //        data structures without complaining!
      string element_i_free_check = "if (seq->" + elements + "[i]) ";
      string element_i_free = funcFree (si->content.type);
      string element_i_new = funcNew (si->content.type);
      printf("void\n");
      printf("%s_resize (%s seq, guint new_size)\n", lname.c_str(), arg.c_str());
      printf("{\n");
      printf("  g_return_if_fail (seq != NULL);\n");
      printf("\n");
      if (element_i_free != "")
	{
	  printf("  if (seq->n_%s > new_size)\n", elements.c_str());
	  printf("    {\n");
	  printf("      guint i;\n");
	  printf("      for (i = new_size; i < seq->n_%s; i++)\n", elements.c_str());
	  printf("        %s %s (seq->%s[i]);\n", element_i_free_check.c_str(),
	      element_i_free.c_str(), elements.c_str());
	  printf("    }\n");
	}
      printf("\n");
      printf("  seq->%s = g_realloc (seq->%s, new_size * sizeof (seq->%s[0]));\n",
	  elements.c_str(), elements.c_str(), elements.c_str());
      printf("  if (new_size > seq->n_%s)\n", elements.c_str());
      if (element_i_new != "")
	{
	  printf("    {\n");
	  printf("      guint i;\n");
	  printf("      for (i = seq->n_%s; i < new_size; i++)\n", elements.c_str());
	  printf("        seq->%s[i] = %s();\n", elements.c_str(), element_i_new.c_str());
	  printf("    }\n");
	}
      else
	{
	  printf("    memset (&seq->%s[seq->n_%s], 0, sizeof(seq->%s[0]) * (new_size - seq->n_%s));\n",
	      elements.c_str(), elements.c_str(), elements.c_str(), elements.c_str());
	}
      printf("  seq->n_%s = new_size;\n", elements.c_str());
      printf("}\n\n");

      printf("void\n");
      printf("%s_free (%s seq)\n", lname.c_str(), arg.c_str());
      printf("{\n");
      if (element_i_free != "")
	printf("  guint i;\n\n");
      printf("  g_return_if_fail (seq != NULL);\n");
      printf("  \n");
      if (element_i_free != "")
	{
	  printf("  for (i = 0; i < seq->n_%s; i++)\n", elements.c_str());
	  printf("        %s %s (seq->%s[i]);\n", element_i_free_check.c_str(),
	      element_i_free.c_str(), elements.c_str());
	}
      printf("  g_free (seq->%s);\n", elements.c_str());
      printf("  g_free (seq);\n");
      printf("}\n\n");
      printf("\n");
    }
}

void CodeGeneratorClientC::printChoiceDefinitions()
{
  for(vector<Choice>::const_iterator ci = parser.getChoices().begin(); ci != parser.getChoices().end(); ci++)
    {
      if (parser.fromInclude (ci->name)) continue;

      string mname = makeMixedName (ci->name);
      string lname = makeLowerName (ci->name);
      printf("\ntypedef enum {\n");
      for (vector<ChoiceValue>::const_iterator vi = ci->contents.begin(); vi != ci->contents.end(); vi++)
	{
	  /* don't export server side assigned choice values to the client */
	  string ename = makeUpperName (vi->name);
	  printf("  %s = %d,\n", ename.c_str(), vi->sequentialValue);
	}
      printf("} %s;\n", mname.c_str());
    }
  printf("\n");
}

void CodeGeneratorClientC::printChoiceConverterPrototypes (PrefixSymbolMode mode)
{
  for (vector<Choice>::const_iterator ci = parser.getChoices().begin(); ci != parser.getChoices().end(); ci++)
    {
      if (parser.fromInclude (ci->name)) continue;

      string mname = makeMixedName (ci->name);
      string lname = makeLowerName (ci->name);

      if (mode == generatePrefixSymbols)
	{
	  prefix_symbols.push_back (lname + "_to_choice");
	  prefix_symbols.push_back (lname + "_from_choice");
	}
      else
	{
	  printf ("const gchar* %s_to_choice (%s value);\n", lname.c_str(), mname.c_str());
	  printf ("%s %s_from_choice (const gchar *choice);\n", mname.c_str(), lname.c_str());
	}
    }
  printf("\n");
}

void CodeGeneratorClientC::printClassMacros()
{
  for (vector<Class>::const_iterator ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
    {
      if (parser.fromInclude (ci->name)) continue;

      string macro = makeUpperName (NamespaceHelper::namespaceOf (ci->name)) + "_IS_" +
	makeUpperName (NamespaceHelper::nameOf (ci->name));
      string mname = makeMixedName (ci->name);

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

bool CodeGeneratorClientC::run()
{
  printf("\n/*-------- begin %s generated code --------*/\n\n\n", options.sfidlName.c_str());

  prefix_symbols.clear();

  if (options.doHeader)
    {
      /* namespace prefixing for symbols defined by the client */

      printChoiceConverterPrototypes (generatePrefixSymbols);
      printRecordMethodPrototypes (generatePrefixSymbols);
      printSequenceMethodPrototypes (generatePrefixSymbols);
      printProcedurePrototypes (generatePrefixSymbols);

      if (options.prefixC != "")
	{
	  for (vector<string>::const_iterator pi = prefix_symbols.begin(); pi != prefix_symbols.end(); pi++)
	    printf("#define %s %s_%s\n", pi->c_str(), options.prefixC.c_str(), pi->c_str());
	  printf("\n");
	}

      /* generate the header */

      printRecordPrototypes();
      printSequencePrototypes();

      printChoiceDefinitions();
      printRecordDefinitions();
      printSequenceDefinitions();

      printRecordMethodPrototypes (generateOutput);
      printSequenceMethodPrototypes (generateOutput);
      printChoiceConverterPrototypes (generateOutput);
      printProcedurePrototypes (generateOutput);

      printClassMacros();
    }

  if (options.doSource)
    {
      printf("#include <string.h>\n");

      printRecordMethodImpl();
      printSequenceMethodImpl();
      printChoiceConverters();
      printProcedureImpl();
    }

  printf("\n/*-------- end %s generated code --------*/\n\n\n", options.sfidlName.c_str());
  return true;
}

namespace {

class ClientCFactory : public Factory {
public:
  string option() const	      { return "--client-c"; }
  string description() const  { return "generate client C language binding"; }
  
  void init (Options& options) const
  {
    options.doImplementation = false;
    options.doInterface = true;
  }
  
  CodeGenerator *create (const Parser& parser) const
  {
    return new CodeGeneratorClientC (parser);
  }
} client_c_factory;

}

/* vim:set ts=8 sts=2 sw=2: */
