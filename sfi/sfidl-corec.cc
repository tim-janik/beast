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
#include "sfidl-corec.h"
#include "sfidl-factory.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

namespace {

using namespace Sfidl;
using namespace std;

class CodeGeneratorCoreC : public CodeGenerator {
  const char*
  intern (const string &str)
  {
    return g_intern_string (str.c_str());
  }
  const char*
  make_type_id_symbol (const string &type_name,
                       const string &append = "")
  {
    vector<string> insertions;
    insertions.push_back ("_type_id_");
    string s = rename (ABSOLUTE, type_name, lower, "_", insertions, lower, "_");
    s += append;
    return intern (s);
  }
  const char*
  make_TYPE_MACRO (const string &type_name,
                   const string &append = "")
  {
    vector<string> insertions;
    insertions.push_back ("TYPE");
    string s = rename (ABSOLUTE, type_name, UPPER, "_", insertions, UPPER, "_");
    s += append;
    return intern (s);
  }
  const char*
  make_fqtn (const string &type_name,
             const string &append = "")
  {
    vector<string> empty;
    string s = rename (ABSOLUTE, type_name, Capitalized, "::", empty, Capitalized, "");
    s += append;
    return intern (s);
  }
  std::string generateInitFunction;
  void
  printInfoStrings (const string&              name,
                    const Map<string,IString> &infos)
  {
    printf ("static const gchar *%s[] = {\n", name.c_str());
    
    Map<string,IString>::const_iterator ii;
    for (ii = infos.begin(); ii != infos.end(); ii++)
      printf ("  \"%s=%s\",\n", ii->first.c_str(), ii->second.c_str());
    
    printf ("  NULL,\n");
    printf ("};\n");
  }
  
  void
  CodeGeneratorCoreC::help()
  {
    CodeGenerator::help();
    fprintf (stderr, " --init <name>               set the name of the init function\n");
  }
  OptionVector
  getOptions()
  {
    OptionVector opts = CodeGenerator::getOptions();
    
    opts.push_back (make_pair ("--init", true));
    
    return opts;
  }
  void
  setOption (const string &option,
             const string &value)
  {
    if (option == "--init")
      {
        generateInitFunction = value;
      }
    else
      {
        CodeGenerator::setOption (option, value);
      }
  }

  const char*
  TypeName (const string &type_name,
            const string &append = "")
  {
    vector<string> empty;
    string s = rename (ABSOLUTE, type_name, Capitalized, "", empty, Capitalized, "");
    s += append;
    return intern (s);
  }
  const char*
  TypeField (const string &type)
  {
    switch (parser.typeOf (type))
      {
      case VOID:        return "void";
      case BOOL:        return "SfiBool";
      case INT:         return "SfiInt";
      case NUM:         return "SfiNum";
      case REAL:        return "SfiReal";
      case CHOICE:      return TypeName (type);
      case STRING:      return "gchar*";
      case BBLOCK:      return "SfiBBlock*";
      case FBLOCK:      return "SfiFBlock*";
      case SFIREC:      return "SfiRec*";
      case RECORD:      return TypeName (type, "*");
      case SEQUENCE:    return TypeName (type, "*");
      case OBJECT:      return TypeName (type, "*");
      default:          g_assert_not_reached(); return NULL;
      }
  }
  const char*
  TypeArg (const string &type)
  {
    switch (parser.typeOf (type))
      {
      case STRING:      return "const char*";
      default:          return TypeField (type);
      }
  }
  const char*
  TypeRet (const string& type)
  {
    switch (parser.typeOf (type))
      {
      default:          return TypeArg (type);
      }
  }
  const char*
  cxx_handle (const string &type,
              const string &var)
  {
    switch (parser.typeOf (type))
      {
      case RECORD:
        {
          string s, cxxtype = string() + make_fqtn (type) + "Handle";
          /* generate: (var ? FooHandle(*var) : FooHandle(INIT_NULL)) */
          s = "(" +
              var + " ? " +
              cxxtype + " (*hack_cast (" + var + ")) : " +
              cxxtype + " (Sfi::INIT_NULL)" +
              ")";
          return intern (s.c_str());
        }
        // FIXME: this lacks variants for sequences, *blocks, etc.
      default:          return intern (var);
      }
  }
  string
  construct_pspec (const Param &pdef)
  {
    string pspec;
    const string group = (pdef.group != "") ? pdef.group.escaped() : "NULL";
    string pname, parg;
    switch (parser.typeOf (pdef.type))
      {
      case CHOICE:
        pname = "Choice";
        parg = makeLowerName (pdef.type) + "_get_values()";
        break;
      case RECORD:
        pname = "Record";
        parg = makeLowerName (pdef.type) + "_fields";
        break;
      case SEQUENCE:
        pname = "Sequence";
        parg = makeLowerName (pdef.type) + "_content";
        break;
      case OBJECT:
        /* FIXME: the ParamSpec doesn't transport the type of the objects we require */
        pname = "Proxy";
        break;
      default:
        pname = pdef.pspec;
        break;
      }
    pspec = "sfidl_pspec_" + pname;
    if (parg != "")
      parg = string (", ") + parg;
    if (pdef.args == "")
      pspec += "_default (" + group + ",\"" + pdef.name + parg + "\")";
    else
      pspec += " (" + group + ",\"" + pdef.name + "\"," + pdef.args + parg + ")";
    return pspec;
  }

  void
  generate_enum_type_id_prototypes ()
  {
    printf ("\n\n/* enum type ids */\n");
    for (vector<Choice>::const_iterator ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
      {
        if (parser.fromInclude (ei->name))
          continue;
        printf ("extern GType %s;\n", make_type_id_symbol (ei->name));
      }
  }
  void
  generate_enum_type_id_declarations ()
  {
    printf ("\n\n/* enum type ids */\n");
    for (vector<Choice>::const_iterator ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
      {
        if (parser.fromInclude (ei->name))
          continue;
        printf ("GType %s = 0;\n", make_type_id_symbol (ei->name));
      }
  }
  void
  generate_enum_type_id_initializations ()
  {
    printf ("\n\n  /* enum type ids */\n");
    for (vector<Choice>::const_iterator ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
      {
        if (parser.fromInclude (ei->name))
          continue;
        printf ("  %s = %s;\n", make_type_id_symbol (ei->name), make_TYPE_MACRO (ei->name));
      }
  }
  void
  generate_enum_type_macros ()
  {
    printf ("\n\n/* enum type macros */\n");
    for (vector<Choice>::const_iterator ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
      {
        if (parser.fromInclude (ei->name))
          continue;
        printf ("#define %s\t\t(%s)\n", make_TYPE_MACRO (ei->name), make_type_id_symbol (ei->name));
      }
  }
  void
  generate_enum_definitions ()
  {
    printf ("\n\n/* enums */\n");
    for (vector<Choice>::const_iterator ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
      {
        if (parser.fromInclude (ei->name))
          continue;
        string mname = makeMixedName (ei->name);
        string lname = makeLowerName (ei->name);
        printf ("\ntypedef enum {\n");
        for (vector<ChoiceValue>::const_iterator ci = ei->contents.begin(); ci != ei->contents.end(); ci++)
          {
            /* don't export server side assigned choice values to the client */
            gint value = ci->value;
            string ename = makeUpperName (ci->name);
            printf ("  %s = %d,\n", ename.c_str(), value);
          }
        printf ("} %s;\n", mname.c_str());
        
        printf ("const gchar* %s_to_choice (%s value);\n", lname.c_str(), mname.c_str());
        printf ("%s %s_from_choice (const gchar *choice);\n", mname.c_str(), lname.c_str());
      }
  }
  void
  generate_enum_value_array ()
  {
    printf ("\n\n/* enum values */\n");
    for (vector<Choice>::const_iterator ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
      {
        if (parser.fromInclude (ei->name))
          continue;
        string name = makeLowerName (ei->name);
        printf ("static const GEnumValue %s_value[%d] = {\n", name.c_str(), ei->contents.size() + 1); // FIXME: i18n
        for (vector<ChoiceValue>::const_iterator ci = ei->contents.begin(); ci != ei->contents.end(); ci++)
          {
            string ename = makeUpperName (ci->name);
            printf ("  { %d, \"%s\", \"%s\" },\n", ci->value, ename.c_str(), ci->label.c_str());
          }
        printf ("  { 0, NULL, NULL }\n");
        printf ("};\n");
      }
  }
  void
  generate_enum_method_prototypes ()
  {
    printf ("\n\n/* enum functions */\n");
    for (vector<Choice>::const_iterator ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
      {
        if (parser.fromInclude (ei->name))
          continue;
        printf ("const SfiChoiceValues %s_get_values (void);\n", makeLowerName (ei->name).c_str());
      }
  }
  void
  generate_enum_method_implementations ()
  {
    int enumCount = 0;
    printf ("\n\n/* enum functions */\n");
    for (vector<Choice>::const_iterator ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
      {
        if (parser.fromInclude (ei->name))
          continue;
        string name = makeLowerName (ei->name);
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
        
        printf ("GType %s = 0;\n", make_TYPE_MACRO (ei->name));
        printf ("\n");
        
        enumCount++;
      }
    if (enumCount)
      {
        printf ("static void\n");
        printf ("choice2enum (const GValue *src_value,\n");
        printf ("             GValue       *dest_value)\n");
        printf ("{\n");
        printf ("  sfi_value_choice2enum (src_value, dest_value, NULL);\n");
        printf ("}\n");
      }
  }
  void
  generate_record_prototypes ()
  {
    printf ("\n\n/* record typedefs */\n");
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name))
          continue;
        string mname = makeMixedName (ri->name);
        printf ("typedef struct _%s %s;\n", mname.c_str(), mname.c_str());
      }
  }
  void
  generate_record_definitions ()
  {
    printf ("\n\n/* records */\n");
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name)) continue;
        
        string mname = makeMixedName (ri->name.c_str());
        
        printf ("struct _%s {\n", mname.c_str());
        for (vector<Param>::const_iterator pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
          {
            printf ("  %s %s;\n", TypeField (pi->type), pi->name.c_str());
          }
        printf ("};\n");
      }
  }
  void
  generate_record_type_id_prototypes ()
  {
    printf ("\n\n/* record type ids */\n");
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name))
          continue;
        printf ("extern GType %s;\n", make_type_id_symbol (ri->name));
      }
  }
  void
  generate_record_type_id_declarations ()
  {
    printf ("\n\n/* record type ids */\n");
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name))
          continue;
        printf ("GType %s = 0;\n", make_type_id_symbol (ri->name));
      }
  }
  void
  generate_record_type_id_initializations ()
  {
    printf ("\n\n  /* record type ids */\n");
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name))
          continue;
        printf ("  %s = %s;\n", make_type_id_symbol (ri->name), make_TYPE_MACRO (ri->name));
      }
  }
  void
  generate_record_type_macros ()
  {
    printf ("\n\n/* record type macros */\n");
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name))
          continue;
        printf ("#define %s\t\t(%s)\n", make_TYPE_MACRO (ri->name), make_type_id_symbol (ri->name));
      }
  }
  void
  generate_record_method_prototypes ()
  {
    printf ("\n\n/* record functions */\n");
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name)) continue;
        
        string ret = TypeRet (ri->name);
        string arg = TypeArg (ri->name);
        string lname = makeLowerName (ri->name.c_str());
        
        printf ("SfiRecFields %s_get_fields (void);\n", lname.c_str());
        printf ("%s %s_new (void);\n", ret.c_str(), lname.c_str());
        printf ("%s %s_copy_shallow (%s rec);\n", ret.c_str(), lname.c_str(), arg.c_str());
        printf ("%s %s_from_rec (SfiRec *sfi_rec);\n", ret.c_str(), lname.c_str());
        printf ("SfiRec *%s_to_rec (%s rec);\n", lname.c_str(), arg.c_str());
        printf ("void %s_free (%s rec);\n", lname.c_str(), arg.c_str());
        printf ("\n");
      }
  }
  void
  generate_record_hack_cast_implementations ()
  {
    printf ("\n\n/* record functions */\n");
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name))
          continue;
        string ret = TypeRet (ri->name);
        const gchar *type = make_fqtn (ri->name);

        printf ("static inline %s\n", ret.c_str());
        printf ("hack_cast (%s *cxxstruct)\n", type);
        printf ("{\n");
        printf ("  return reinterpret_cast<%s> (cxxstruct);\n", ret.c_str());
        printf ("}\n");
        printf ("static inline %s*\n", type);
        printf ("hack_cast (%s cstruct)\n", ret.c_str());
        printf ("{\n");
        printf ("  return reinterpret_cast< %s*> (cstruct);\n", type);
        printf ("}\n");
      }
  }
  void
  generate_record_method_implementations ()
  {
    printf ("\n\n/* record functions */\n");
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name))
          continue;
        string ret = TypeRet (ri->name);
        string arg = TypeArg (ri->name);
        string lname = makeLowerName (ri->name.c_str());
        string mname = makeMixedName (ri->name.c_str());

        const gchar *type = make_fqtn (ri->name);

        printf ("SfiRecFields\n");
        printf ("%s_get_fields (void)\n", lname.c_str());
        printf ("{\n");
        printf ("  return %s::get_fields ();\n", type);
        printf ("}\n");

        printf ("%s\n", ret.c_str());
        printf ("%s_new (void)\n", lname.c_str());
        printf ("{\n");
        printf ("  %sHandle rh (Sfi::INIT_DEFAULT); \n", type);
        printf ("  return hack_cast (rh.steal());\n");
        printf ("}\n");
        
        printf ("%s\n", ret.c_str());
        printf ("%s_copy_shallow (%s cstruct)\n", lname.c_str(), arg.c_str());
        printf ("{\n");
        printf ("  %sHandle rh;\n", type);
        printf ("  rh.set_boxed (hack_cast (cstruct));\n");
        printf ("  return hack_cast (rh.steal());\n");
        printf ("}\n");
        
        printf ("%s\n", ret.c_str());
        printf ("%s_from_rec (SfiRec *rec)\n", lname.c_str());
        printf ("{\n");
        printf ("  %sHandle rh = %s::from_rec (rec);\n", type, type);
        printf ("  return hack_cast (rh.steal());\n");
        printf ("}\n");
        
        printf ("SfiRec*\n");
        printf ("%s_to_rec (%s cstruct)\n", lname.c_str(), arg.c_str());
        printf ("{\n");
        printf ("  %sHandle rh;\n", type);
        printf ("  rh.set_boxed (hack_cast (cstruct));\n");
        printf ("  return %s::to_rec (rh);\n", type);
        printf ("}\n");
        
        printf ("void\n");
        printf ("%s_free (%s cstruct)\n", lname.c_str(), arg.c_str());
        printf ("{\n");
        printf ("  %sHandle rh;\n", type);
        printf ("  rh.take (hack_cast (cstruct));\n");
        printf ("}\n");
        printf ("\n");
      }
  }
  void
  generate_record_converter_implementations ()
  {
    printf ("\n\n/* record converters */\n");
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name))
          continue;
        
        string name = makeLowerName (ri->name);
        
        printf ("static GParamSpec *%s_field[%d];\n", name.c_str(), ri->contents.size());
        printf ("SfiRecFields %s_fields = { %d, %s_field };\n", name.c_str(), ri->contents.size(), name.c_str());
        
        string mname = makeMixedName (ri->name);
        
        printf ("static void\n");
        printf ("%s_boxed2rec (const GValue *src_value, GValue *dest_value)\n", name.c_str());
        printf ("{\n");
        printf ("  gpointer boxed = g_value_get_boxed (src_value);\n");
        printf ("  sfi_value_take_rec (dest_value, boxed ? %s_to_rec (boxed) : NULL);\n", name.c_str());
        printf ("}\n");
        
        printf ("static void\n");
        printf ("%s_rec2boxed (const GValue *src_value, GValue *dest_value)\n", name.c_str());
        printf ("{\n");
        printf ("  SfiRec *rec = sfi_value_get_rec (src_value);\n");
        printf ("  g_value_set_boxed_take_ownership (dest_value,\n");
        printf ("    rec ? %s_from_rec (rec) : NULL);\n", name.c_str());
        printf ("}\n");
        
        printInfoStrings (name + "_info_strings", ri->infos);
        printf ("static SfiBoxedRecordInfo %s_boxed_info = {\n", name.c_str());
        printf ("  \"%s\",\n", mname.c_str());
        printf ("  { %d, %s_field },\n", ri->contents.size(), name.c_str());
        printf ("  %s_boxed2rec,\n", name.c_str());
        printf ("  %s_rec2boxed,\n", name.c_str());
        printf ("  %s_info_strings\n", name.c_str());
        printf ("};\n");
        printf ("GType %s = 0;\n", make_TYPE_MACRO (ri->name));
      }
  }
  void
  generate_sequence_prototypes ()
  {
    printf ("\n\n/* sequence typedefs */\n");
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name))
          continue;
        string mname = makeMixedName (si->name);
        printf ("typedef struct _%s %s;\n", mname.c_str(), mname.c_str());
      }
  }
  void
  generate_sequence_definitions ()
  {
    printf ("\n\n/* sequences */\n");
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name)) continue;
        
        string mname = makeMixedName (si->name.c_str());
        string array = string (TypeField (si->content.type)) + "*";
        string elements = si->content.name;
        
        printf ("struct _%s {\n", mname.c_str());
        printf ("  guint n_%s;\n", elements.c_str ());
        printf ("  %s %s;\n", array.c_str(), elements.c_str());
        printf ("};\n");
      }
  }
  void
  generate_sequence_type_id_prototypes ()
  {
    printf ("\n\n/* sequence type ids */\n");
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name))
          continue;
        printf ("extern GType %s;\n", make_type_id_symbol (si->name));
      }
  }
  void
  generate_sequence_type_id_declarations ()
  {
    printf ("\n\n/* sequence type ids */\n");
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name))
          continue;
        printf ("GType %s = 0;\n", make_type_id_symbol (si->name));
      }
  }
  void
  generate_sequence_type_id_initializations ()
  {
    printf ("\n\n  /* sequence type ids */\n");
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name))
          continue;
        printf ("  %s = %s;\n", make_type_id_symbol (si->name), make_TYPE_MACRO (si->name));
      }
  }
  void
  generate_sequence_type_macros ()
  {
    printf ("\n\n/* sequence type macros */\n");
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name))
          continue;
        printf ("#define %s\t\t(%s)\n", make_TYPE_MACRO (si->name), make_type_id_symbol (si->name));
      }
  }
  void
  generate_sequence_method_prototypes ()
  {
    printf ("\n\n/* sequence functions */\n");
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name)) continue;

        string ret = TypeRet (si->name);
        string arg = TypeArg (si->name);
        string element = TypeArg (si->content.type);
        string lname = makeLowerName (si->name.c_str());

        printf ("GParamSpec* %s_get_element (void);\n", lname.c_str());
        printf ("%s %s_new (void);\n", ret.c_str(), lname.c_str());
        printf ("void %s_append (%s seq, %s element);\n", lname.c_str(), arg.c_str(), element.c_str());
        printf ("%s %s_copy_shallow (%s seq);\n", ret.c_str(), lname.c_str(), arg.c_str());
        printf ("%s %s_from_seq (SfiSeq *sfi_seq);\n", ret.c_str(), lname.c_str());
        printf ("SfiSeq *%s_to_seq (%s seq);\n", lname.c_str(), arg.c_str());
        printf ("void %s_resize (%s seq, guint new_size);\n", lname.c_str(), arg.c_str());
        printf ("void %s_free (%s seq);\n", lname.c_str(), arg.c_str());
        printf ("\n");
      }
  }
  void
  generate_sequence_hack_cast_implementations ()
  {
    printf ("\n\n/* sequence C <-> C++ casts */\n");
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name))
          continue;
        string ret = TypeRet (si->name);
        const gchar *type = make_fqtn (si->name);

        /* the cast functions take an extra unused sequence argument, to distinguish
         * two sequences A and B which both have the same CSeq type (e.g. SfiInt and SfiNote).
         */
        printf ("static inline %s\n", ret.c_str());
        printf ("hack_cast (const %s &unused, %s::CSeq *cxxseq)\n", type, type);
        printf ("{\n");
        printf ("  return reinterpret_cast<%s> (cxxseq);\n", ret.c_str());
        printf ("}\n");
        printf ("static inline %s::CSeq*\n", type);
        printf ("hack_cast (%s cseq)\n", ret.c_str());
        printf ("{\n");
        printf ("  return reinterpret_cast< %s::CSeq*> (cseq);\n", type);
        printf ("}\n");
      }
  }
  void
  generate_sequence_method_implementations ()
  {
    printf ("\n\n/* sequence functions */\n");
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name))
          continue;
        string ret = TypeRet (si->name);
        string arg = TypeArg (si->name);
        string element = TypeArg (si->content.type);
        string elements = si->content.name;
        string lname = makeLowerName (si->name.c_str());
        
        const gchar *type = make_fqtn (si->name);

        printf ("GParamSpec*\n");
        printf ("%s_get_element (void)\n", lname.c_str());
        printf ("{\n");
        printf ("  return %s::get_element ();\n", type);
        printf ("}\n");

        printf ("%s\n", ret.c_str());
        printf ("%s_new (void)\n", lname.c_str());
        printf ("{\n");
        printf ("  %s sh (0);\n", type);
        printf ("  return hack_cast (sh, sh.steal());\n");
        printf ("}\n");
        
        printf ("void\n");
        printf ("%s_append (%s cseq, %s element)\n", lname.c_str(), arg.c_str(), element.c_str());
        printf ("{\n");
        printf ("  g_return_if_fail (cseq != NULL);\n");
        printf ("  %s sh (0);\n", type);
        printf ("  sh.take (hack_cast (cseq));\n");
        printf ("  sh += %s;\n", cxx_handle (si->content.type, "element"));
        printf ("  sh.steal(); /* prevent cseq deletion */\n");
        printf ("}\n");
        
        printf ("%s\n", ret.c_str());
        printf ("%s_copy_shallow (%s cseq)\n", lname.c_str(), arg.c_str());
        printf ("{\n");
        printf ("  %s sh (0);\n", type);
        printf ("  sh.set_boxed (hack_cast (cseq));\n");
        printf ("  return hack_cast (sh, sh.steal());\n");
        printf ("}\n");
        
        printf ("%s\n", ret.c_str());
        printf ("%s_from_seq (SfiSeq *seq)\n", lname.c_str());
        printf ("{\n");
        printf ("  %s sh = %s::from_seq (seq);\n", type, type);
        printf ("  return hack_cast (sh, sh.steal());\n");
        printf ("}\n");
        
        printf ("SfiSeq*\n");
        printf ("%s_to_seq (%s cseq)\n", lname.c_str(), arg.c_str());
        printf ("{\n");
        printf ("  %s sh (0);\n", type);
        printf ("  sh.take (hack_cast (cseq));\n");
        printf ("  SfiSeq *seq = %s::to_seq (sh);\n", type);
        printf ("  sh.steal(); /* prevent cseq deletion */\n");
        printf ("  return seq;\n");
        printf ("}\n");
        
        printf ("void\n");
        printf ("%s_resize (%s cseq, guint n)\n", lname.c_str(), arg.c_str());
        printf ("{\n");
        printf ("  g_return_if_fail (cseq != NULL);\n");
        printf ("  %s sh (0);\n", type);
        printf ("  sh.take (hack_cast (cseq));\n");
        printf ("  sh.resize (n);\n");
        printf ("  sh.steal(); /* prevent cseq deletion */\n");
        printf ("}\n");

        printf ("void\n");
        printf ("%s_free (%s cseq)\n", lname.c_str(), arg.c_str());
        printf ("{\n");
        printf ("  %s sh (0);\n", type);
        printf ("  sh.take (hack_cast (cseq));\n");
        printf ("}\n");
        printf ("\n");
      }
  }
  void
  generate_sequence_converter_implementations ()
  {
    printf ("\n\n/* sequence converters */\n");
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name)) continue;
        
        string name = makeLowerName (si->name);
        
        printf ("static GParamSpec *%s_content;\n", name.c_str());
        
        string mname = makeMixedName (si->name);
        
        printf ("static void\n");
        printf ("%s_boxed2seq (const GValue *src_value, GValue *dest_value)\n", name.c_str());
        printf ("{\n");
        printf ("  gpointer boxed = g_value_get_boxed (src_value);\n");
        printf ("  sfi_value_take_seq (dest_value, boxed ? %s_to_seq (boxed) : NULL);\n", name.c_str());
        printf ("}\n");
        
        printf ("static void\n");
        printf ("%s_seq2boxed (const GValue *src_value, GValue *dest_value)\n", name.c_str());
        printf ("{\n");
        printf ("  SfiSeq *seq = sfi_value_get_seq (src_value);\n");
        printf ("  g_value_set_boxed_take_ownership (dest_value,\n");
        printf ("    seq ? %s_from_seq (seq) : NULL);\n", name.c_str());
        printf ("}\n");
        
        printInfoStrings (name + "_info_strings", si->infos);
        printf ("static SfiBoxedSequenceInfo %s_boxed_info = {\n", name.c_str());
        printf ("  \"%s\",\n", mname.c_str());
        printf ("  NULL, /* %s_content */\n", name.c_str());
        printf ("  %s_boxed2seq,\n", name.c_str());
        printf ("  %s_seq2boxed,\n", name.c_str());
        printf ("  %s_info_strings\n", name.c_str());
        printf ("};\n");
        printf ("GType %s = 0;\n", make_TYPE_MACRO (si->name));
      }
  }
  void
  generate_init_function ()
  {
    bool first = true;
    printf ("\n\n/* type initialization function */\n");
    printf ("static void\n%s (void)\n", generateInitFunction.c_str());
    printf ("{\n");
    
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
            if (!first)
              printf ("\n");
            first = false;
          }
        if (parser.isRecord (*ti))
          {
            const Record& rdef = parser.findRecord (*ti);
            
            string name = makeLowerName (rdef.name);
            int f = 0;
            
            for (vector<Param>::const_iterator pi = rdef.contents.begin(); pi != rdef.contents.end(); pi++, f++)
              {
                if (generateIdlLineNumbers)
                  printf ("#line %u \"%s\"\n", pi->line, parser.fileName().c_str());
                printf ("  %s_field[%d] = %s;\n", name.c_str(), f, construct_pspec (*pi).c_str());
              }
          }
        if (parser.isSequence (*ti))
          {
            const Sequence& sdef = parser.findSequence (*ti);
            
            string name = makeLowerName (sdef.name);
            
            if (generateIdlLineNumbers)
              printf ("#line %u \"%s\"\n", sdef.content.line, parser.fileName().c_str());
            printf ("  %s_content = %s;\n", name.c_str(), construct_pspec (sdef.content).c_str());
          }
      }
    for (vector<Choice>::const_iterator ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
      {
        if (parser.fromInclude (ei->name)) continue;
        
        string gname = make_TYPE_MACRO (ei->name);
        string name = makeLowerName(ei->name);
        string mname = makeMixedName(ei->name);
        
        printf ("  %s = g_enum_register_static (\"%s\", %s_value);\n", gname.c_str(),
                mname.c_str(), name.c_str());
        printf ("  g_value_register_transform_func (SFI_TYPE_CHOICE, %s, choice2enum);\n",
                gname.c_str());
        printf ("  g_value_register_transform_func (%s, SFI_TYPE_CHOICE,"
                " sfi_value_enum2choice);\n", gname.c_str());
      }
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name)) continue;
        
        string gname = make_TYPE_MACRO (ri->name);
        string name = makeLowerName(ri->name);
        
        printf ("  %s = sfi_boxed_make_record (&%s_boxed_info,\n", gname.c_str(), name.c_str());
        printf ("    (GBoxedCopyFunc) %s_copy_shallow,\n", name.c_str());
        printf ("    (GBoxedFreeFunc) %s_free);\n", name.c_str());
      }
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name)) continue;
        
        string gname = make_TYPE_MACRO (si->name);
        string name = makeLowerName(si->name);
        
        printf ("  %s_boxed_info.element = %s_content;\n", name.c_str(), name.c_str());
        printf ("  %s = sfi_boxed_make_sequence (&%s_boxed_info,\n", gname.c_str(), name.c_str());
        printf ("    (GBoxedCopyFunc) %s_copy_shallow,\n", name.c_str());
        printf ("    (GBoxedFreeFunc) %s_free);\n", name.c_str());
      }
    printf ("}\n");
  }
  
public:
  CodeGeneratorCoreC (const Parser& parser) :
    CodeGenerator (parser)
  {
  }

  bool
  run ()
  {
    printf ("\n/*-------- begin %s generated code --------*/\n\n\n", options.sfidlName.c_str());
    if (generateSource)
      printf ("#include <string.h>\n");
    
    if (generateHeader)
      {
        generate_enum_definitions ();
        generate_record_prototypes ();
        generate_sequence_prototypes ();
        generate_sequence_definitions ();
        generate_record_definitions ();
        generate_record_method_prototypes ();
        generate_sequence_method_prototypes ();
        generate_enum_method_prototypes ();
        generate_enum_type_id_prototypes ();
        generate_record_type_id_prototypes ();
        generate_sequence_type_id_prototypes ();
        printf ("\n#ifndef __cplusplus\n");
        generate_enum_type_macros ();
        generate_record_type_macros ();
        generate_sequence_type_macros ();
        printf ("\n#endif\n");
      }

    if (generateSource)
      {
        // generate_enum_value_array ();
        // generate_enum_method_implementations ();
        generate_record_hack_cast_implementations ();
        generate_sequence_hack_cast_implementations ();
        generate_record_method_implementations ();
        generate_sequence_method_implementations ();
        generate_enum_type_id_declarations ();
        generate_record_type_id_declarations ();
        generate_sequence_type_id_declarations ();
        // generate_record_converter_implementations ();
        // generate_sequence_converter_implementations ();
        // printChoiceConverters ();
        if (generateInitFunction != "")
          {     // generate_init_function();
            printf ("\n\n/* type initialization function */\n");
            printf ("static void\n%s (void)\n{\n", generateInitFunction.c_str());
            generate_enum_type_id_initializations ();
            generate_record_type_id_initializations ();
            generate_sequence_type_id_initializations ();
            printf ("}\n");
          }
      }

    printf ("\n/*-------- end %s generated code --------*/\n\n\n", options.sfidlName.c_str());
    return true;
  }
};

class CoreCFactory : public Factory {
public:
  string option() const	      { return "--core-c"; }
  string description() const  { return "generate core C language binding"; }
  
  CodeGenerator *create (const Parser& parser) const
  {
    return new CodeGeneratorCoreC (parser);
  }
} core_c_factory;

} // anon

/* vim:set ts=8 sts=2 sw=2: */
