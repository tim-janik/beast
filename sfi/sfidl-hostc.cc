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
#include "sfidl-hostc.h"
#include "sfidl-factory.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include "sfidl-namespace.h"
#include "sfidl-options.h"
#include "sfidl-parser.h"

using namespace Sfidl;
using namespace std;

void CodeGeneratorHostC::printInfoStrings (const string& name, const Map<string,IString>& infos)
{
  printf("static const gchar *%s[] = {\n", name.c_str());

  Map<string,IString>::const_iterator ii;
  for (ii = infos.begin(); ii != infos.end(); ii++)
    printf("  \"%s=%s\",\n", ii->first.c_str(), ii->second.c_str());

  printf("  NULL,\n");
  printf("};\n");
}

bool CodeGeneratorHostC::run ()
{
  vector<Sequence>::const_iterator si;
  vector<Record>::const_iterator ri;
  vector<Choice>::const_iterator ei;
  vector<Param>::const_iterator pi;
  vector<Class>::const_iterator ci;
  vector<Method>::const_iterator mi;
 
  printf("\n/*-------- begin %s generated code --------*/\n\n\n", options.sfidlName.c_str());

  if (options.generateTypeC)
    printf("#include <string.h>\n");
  if (options.generateTypeH)
    {
      if (options.prefixC != "")
	{
	  vector<string> todo;
	  /* namespace prefixing */
	  for (si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
	    {
	      if (parser.fromInclude (si->name)) continue;

	      string lname = makeLowerName (si->name.c_str());
	      todo.push_back (lname + "_new");
	      todo.push_back (lname + "_append");
	      todo.push_back (lname + "_copy_shallow");
	      todo.push_back (lname + "_from_seq");
	      todo.push_back (lname + "_to_seq");
	      todo.push_back (lname + "_resize");
	      todo.push_back (lname + "_free");
	    }
	  for (ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
	    {
	      if (parser.fromInclude (ri->name)) continue;

	      string lname = makeLowerName (ri->name.c_str());
	      todo.push_back (lname + "_new");
	      todo.push_back (lname + "_copy_shallow");
	      todo.push_back (lname + "_from_rec");
	      todo.push_back (lname + "_to_rec");
	      todo.push_back (lname + "_free");
	    }
	  for(ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
	    {
	      if (parser.fromInclude (ei->name)) continue;

	      string lname = makeLowerName (ei->name);
	      todo.push_back (lname + "_to_choice");
	      todo.push_back (lname + "_from_choice");
	    }

	  for (ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
	    {
	      if (parser.fromInclude (ci->name)) continue;

	      for (mi = ci->methods.begin(); mi != ci->methods.end(); mi++)
		todo.push_back (makeLowerName (ci->name + "_" + mi->name));
	    }
	  for (mi = parser.getProcedures().begin(); mi != parser.getProcedures().end(); mi++)
	    {
	      if (parser.fromInclude (mi->name)) continue;
	      todo.push_back (makeLowerName (mi->name));
	    }

	  for (vector<string>::const_iterator ti = todo.begin(); ti != todo.end(); ti++)
	    printf("#define %s %s_%s\n", ti->c_str(), options.prefixC.c_str(), ti->c_str());
	  printf("\n");
	}

      for (si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
	{
	  if (parser.fromInclude (si->name)) continue;

	  string mname = makeMixedName (si->name);
	  printf("typedef struct _%s %s;\n", mname.c_str(), mname.c_str());
	}
      for (ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
	{
	  if (parser.fromInclude (ri->name)) continue;

	  string mname = makeMixedName (ri->name);
	  printf("typedef struct _%s %s;\n", mname.c_str(), mname.c_str());
	}
      for(ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
	{
	  if (parser.fromInclude (ei->name)) continue;

	  string mname = makeMixedName (ei->name);
	  string lname = makeLowerName (ei->name);
	  printf("\ntypedef enum {\n");
	  for (vector<ChoiceValue>::const_iterator ci = ei->contents.begin(); ci != ei->contents.end(); ci++)
	    {
	      /* don't export server side assigned choice values to the client */
	      gint value = options.doInterface || !options.generateBoxedTypes ? ci->sequentialValue : ci->value;
              // FIXME: the above condition is really hairy, basically we just want value for BSE and GType
	      string ename = makeUpperName (ci->name);
	      printf("  %s = %d,\n", ename.c_str(), value);
	    }
	  printf("} %s;\n", mname.c_str());

	  printf("const gchar* %s_to_choice (%s value);\n", lname.c_str(), mname.c_str());
	  printf("%s %s_from_choice (const gchar *choice);\n", mname.c_str(), lname.c_str());
	}
     
      printf("\n");

      for (si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
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
      for (ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
	{
	  if (parser.fromInclude (ri->name)) continue;

	  string mname = makeMixedName (ri->name.c_str());
	  
	  printf("struct _%s {\n", mname.c_str());
	  for (pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
	    {
	      printf("  %s %s;\n", cTypeField (pi->type), pi->name.c_str());
	    }
	  printf("};\n");
	}

      printf("\n");

      for (si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
	{
	  if (parser.fromInclude (si->name)) continue;

	  string ret = typeRet (si->name);
	  string arg = typeArg (si->name);
	  string element = typeArg (si->content.type);
	  string lname = makeLowerName (si->name.c_str());
	  
	  printf("%s %s_new (void);\n", ret.c_str(), lname.c_str());
	  printf("void %s_append (%s seq, %s element);\n", lname.c_str(), arg.c_str(), element.c_str());
	  printf("%s %s_copy_shallow (%s seq);\n", ret.c_str(), lname.c_str(), arg.c_str());
	  printf("%s %s_from_seq (SfiSeq *sfi_seq);\n", ret.c_str(), lname.c_str());
	  printf("SfiSeq *%s_to_seq (%s seq);\n", lname.c_str(), arg.c_str());
	  printf("void %s_resize (%s seq, guint new_size);\n", lname.c_str(), arg.c_str());
	  printf("void %s_free (%s seq);\n", lname.c_str(), arg.c_str());
	  printf("\n");
	}
      for (ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
	{
	  if (parser.fromInclude (ri->name)) continue;

	  string ret = typeRet (ri->name);
	  string arg = typeArg (ri->name);
	  string lname = makeLowerName (ri->name.c_str());
	  
	  printf("%s %s_new (void);\n", ret.c_str(), lname.c_str());
	  printf("%s %s_copy_shallow (%s rec);\n", ret.c_str(), lname.c_str(), arg.c_str());
	  printf("%s %s_from_rec (SfiRec *sfi_rec);\n", ret.c_str(), lname.c_str());
	  printf("SfiRec *%s_to_rec (%s rec);\n", lname.c_str(), arg.c_str());
	  printf("void %s_free (%s rec);\n", lname.c_str(), arg.c_str());
	  printf("\n");
	}
      printf("\n");
    }
  
  if (options.generateExtern)
    {
      for(ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
	{
	  if (parser.fromInclude (ei->name)) continue;

          printf("const SfiChoiceValues %s_get_values (void);\n", makeLowerName (ei->name).c_str());
	  if (options.generateBoxedTypes)
	    printf("extern GType %s;\n", makeGTypeName (ei->name).c_str());
	}
      
      for(ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
	if (parser.fromInclude (ri->name)) continue;

	printf("extern SfiRecFields %s_fields;\n",makeLowerName (ri->name).c_str());
        if (options.generateBoxedTypes)
	  printf("extern GType %s;\n", makeGTypeName (ri->name).c_str());
      }

      if (options.generateBoxedTypes)
      {
	for(si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
	  {
	    if (parser.fromInclude (si->name)) continue;
	    printf("extern GType %s;\n", makeGTypeName (si->name).c_str());
	  }
      }
      printf("\n");
    }
  
  if (options.generateTypeC)
    {
      for (si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
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
      for (ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
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
  
  if (options.generateData)
    {
      int enumCount = 0;

      for(ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
	{
	  if (parser.fromInclude (ei->name))
            continue;

	  string name = makeLowerName (ei->name);

          if (options.generateBoxedTypes)
            {
              printf("static const GEnumValue %s_value[%d] = {\n", name.c_str(), ei->contents.size() + 1); // FIXME: i18n
              for (vector<ChoiceValue>::const_iterator ci = ei->contents.begin(); ci != ei->contents.end(); ci++)
                {
                  string ename = makeUpperName (ci->name);
                  printf("  { %d, \"%s\", \"%s\" },\n", ci->value, ename.c_str(), ci->label.c_str());
                }
              printf("  { 0, NULL, NULL }\n");
              printf("};\n");
            }
          printf ("const SfiChoiceValues\n");
          printf ("%s_get_values (void)\n", makeLowerName (ei->name).c_str());
          printf ("{\n");
          printf ("  static SfiChoiceValue values[%u];\n", ei->contents.size());
          printf ("  static const SfiChoiceValues choice_values = {\n");
          printf ("    G_N_ELEMENTS (values), values,\n");
          printf ("  };\n");
          printf ("  if (!values[0].choice_ident)\n    {\n");
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
          
	  if (options.generateBoxedTypes)
	    printf("GType %s = 0;\n", makeGTypeName (ei->name).c_str());
	  printf("\n");

	  enumCount++;
	}

      if (options.generateBoxedTypes && enumCount)
	{
	  printf("static void\n");
	  printf("choice2enum (const GValue *src_value,\n");
	  printf("             GValue       *dest_value)\n");
	  printf("{\n");
	  printf("  sfi_value_choice2enum (src_value, dest_value, NULL);\n");
	  printf("}\n");
	}
      
      for(ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
	{
	  if (parser.fromInclude (ri->name)) continue;

	  string name = makeLowerName (ri->name);
	  
	  printf("static GParamSpec *%s_field[%d];\n", name.c_str(), ri->contents.size());
	  printf("SfiRecFields %s_fields = { %d, %s_field };\n", name.c_str(), ri->contents.size(), name.c_str());

	  if (options.generateBoxedTypes)
	    {
	      string mname = makeMixedName (ri->name);

	      // FIXME: stefan might want to implement the transforms differently

	      printf("static void\n");
	      printf("%s_boxed2rec (const GValue *src_value, GValue *dest_value)\n", name.c_str());
	      printf("{\n");
	      printf("  gpointer boxed = g_value_get_boxed (src_value);\n");
	      printf("  sfi_value_take_rec (dest_value, boxed ? %s_to_rec (boxed) : NULL);\n", name.c_str());
	      printf("}\n");
	      
	      printf("static void\n");
	      printf("%s_rec2boxed (const GValue *src_value, GValue *dest_value)\n", name.c_str());
	      printf("{\n");
	      printf("  SfiRec *rec = sfi_value_get_rec (src_value);\n");
	      printf("  g_value_set_boxed_take_ownership (dest_value,\n");
	      printf("    rec ? %s_from_rec (rec) : NULL);\n", name.c_str());
	      printf("}\n");
	      
	      printInfoStrings (name + "_info_strings", ri->infos);
	      printf("static SfiBoxedRecordInfo %s_boxed_info = {\n", name.c_str());
	      printf("  \"%s\",\n", mname.c_str());
	      printf("  { %d, %s_field },\n", ri->contents.size(), name.c_str());
	      printf("  %s_boxed2rec,\n", name.c_str());
	      printf("  %s_rec2boxed,\n", name.c_str());
	      printf("  %s_info_strings\n", name.c_str());
	      printf("};\n");
	      printf("GType %s = 0;\n", makeGTypeName (ri->name).c_str());
	    }
	  printf("\n");
	}
      for(si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
	{
	  if (parser.fromInclude (si->name)) continue;

	  string name = makeLowerName (si->name);
	  
	  printf("static GParamSpec *%s_content;\n", name.c_str());

	  if (options.generateBoxedTypes)
	    {
	      string mname = makeMixedName (si->name);
	      
              // FIXME: stefan might want to implement the transforms differently

	      printf("static void\n");
	      printf("%s_boxed2seq (const GValue *src_value, GValue *dest_value)\n", name.c_str());
	      printf("{\n");
              printf("  gpointer boxed = g_value_get_boxed (src_value);\n");
	      printf("  sfi_value_take_seq (dest_value, boxed ? %s_to_seq (boxed) : NULL);\n", name.c_str());
	      printf("}\n");
	      
	      printf("static void\n");
	      printf("%s_seq2boxed (const GValue *src_value, GValue *dest_value)\n", name.c_str());
	      printf("{\n");
              printf("  SfiSeq *seq = sfi_value_get_seq (src_value);\n");
	      printf("  g_value_set_boxed_take_ownership (dest_value,\n");
	      printf("    seq ? %s_from_seq (seq) : NULL);\n", name.c_str());
	      printf("}\n");
	      
	      printInfoStrings (name + "_info_strings", si->infos);
	      printf("static SfiBoxedSequenceInfo %s_boxed_info = {\n", name.c_str());
	      printf("  \"%s\",\n", mname.c_str());
	      printf("  NULL, /* %s_content */\n", name.c_str());
	      printf("  %s_boxed2seq,\n", name.c_str());
	      printf("  %s_seq2boxed,\n", name.c_str());
	      printf("  %s_info_strings\n", name.c_str());
	      printf("};\n");
	      printf("GType %s = 0;\n", makeGTypeName (si->name).c_str());
	    }
	  printf("\n");
	}
    }

  // if (options.doInterface && options.doSource)
  if (options.doSource) //  && !options.generateBoxedTypes)
    printChoiceConverters();

  if (options.initFunction != "")
    {
      bool first = true;
      printf("static void\n%s (void)\n", options.initFunction.c_str());
      printf("{\n");

      /*
       * It is important to follow the declaration order of the idl file here, as for
       * instance a Param inside a record might come from a sequence, and a Param
       * inside a Sequence might come from a record - to avoid using yet-unitialized
       * Params, we follow the getTypes() 
       */
      vector<string>::const_iterator ti;

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

	      string name = makeLowerName (rdef.name);
	      int f = 0;

	      for (pi = rdef.contents.begin(); pi != rdef.contents.end(); pi++, f++)
		{
		  if (options.generateIdlLineNumbers)
		    printf("#line %u \"%s\"\n", pi->line, parser.fileName().c_str());
		  printf("  %s_field[%d] = %s;\n", name.c_str(), f, makeParamSpec (*pi).c_str());
		}
	    }
	  if (parser.isSequence (*ti))
	    {
	      const Sequence& sdef = parser.findSequence (*ti);

	      string name = makeLowerName (sdef.name);
	      // int f = 0;

	      if (options.generateIdlLineNumbers)
		printf("#line %u \"%s\"\n", sdef.content.line, parser.fileName().c_str());
	      printf("  %s_content = %s;\n", name.c_str(), makeParamSpec (sdef.content).c_str());
	    }
	}
      if (options.generateBoxedTypes)
      {
	for(ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
	  {
	    if (parser.fromInclude (ei->name)) continue;

	    string gname = makeGTypeName(ei->name);
	    string name = makeLowerName(ei->name);
	    string mname = makeMixedName(ei->name);

	    printf("  %s = g_enum_register_static (\"%s\", %s_value);\n", gname.c_str(),
						      mname.c_str(), name.c_str());
	    printf("  g_value_register_transform_func (SFI_TYPE_CHOICE, %s, choice2enum);\n",
						      gname.c_str());
	    printf("  g_value_register_transform_func (%s, SFI_TYPE_CHOICE,"
		   " sfi_value_enum2choice);\n", gname.c_str());
	  }
	for(ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
	  {
	    if (parser.fromInclude (ri->name)) continue;

	    string gname = makeGTypeName(ri->name);
	    string name = makeLowerName(ri->name);

	    printf("  %s = sfi_boxed_make_record (&%s_boxed_info,\n", gname.c_str(), name.c_str());
	    printf("    (GBoxedCopyFunc) %s_copy_shallow,\n", name.c_str());
	    printf("    (GBoxedFreeFunc) %s_free);\n", name.c_str());
	  }
      	for(si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
	  {
	    if (parser.fromInclude (si->name)) continue;

	    string gname = makeGTypeName(si->name);
	    string name = makeLowerName(si->name);

	    printf("  %s_boxed_info.element = %s_content;\n", name.c_str(), name.c_str());
	    printf("  %s = sfi_boxed_make_sequence (&%s_boxed_info,\n", gname.c_str(), name.c_str());
	    printf("    (GBoxedCopyFunc) %s_copy_shallow,\n", name.c_str());
	    printf("    (GBoxedFreeFunc) %s_free);\n", name.c_str());
	  }
}
      printf("}\n");
    }

  printf("\n/*-------- end %s generated code --------*/\n\n\n", options.sfidlName.c_str());
  return true;
}

namespace {

class HostCFactory : public Factory {
public:
  string option() const	      { return "--host-c"; }
  string description() const  { return "generate host C language binding"; }
  
  void init (Options& options) const
  {
    options.doImplementation = true;
    options.doInterface = false;
  }
  
  CodeGenerator *create (const Parser& parser) const
  {
    return new CodeGeneratorHostC (parser);
  }
} host_c_factory;

}

/* vim:set ts=8 sts=2 sw=2: */
