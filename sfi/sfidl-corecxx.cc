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
#include "sfidl-module.h"
#include "sfidl-namespace.h"
#include "sfidl-factory.h"

#include <string.h>
#include <stdio.h>
#include <list>
#include <string>
#include <map>
#include <sfi/glib-extra.h>

namespace {
using namespace std;
using namespace Sfidl;

static const gchar*
canonify_name (const string& s,
               const char    replace = '-')
{
  /* canonify type names which contain e.g. underscores (procedures) */
  gchar *tmp = g_strcanon (g_strdup (s.c_str()),
                           G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "+",
                           replace);
  string d = tmp;
  g_free (tmp);
  return g_intern_string (d.c_str());
}

static vector<string>
split_string (const string &ctype)
{
  vector<string> vs;
  string type = ctype;
  int i;
  while ((i = type.find (':')) >= 0)
    {
      vs.push_back (type.substr (0, i));
      if (type[i + 1] == ':')
        type = type.substr (i + 2);
      else
        type = type.substr (i + 1);
    }
  vs.push_back (type);
  return vs;
}

static string
join_string (const vector<string> &vs,
             const string         &delim)
{
  string r;
  for (vector<string>::const_iterator vi = vs.begin(); vi != vs.end(); vi++)
    {
      if (vi != vs.begin())
        r += delim;
      r += *vi;
    }
  return r;
}

static string
include_relative (string path,
                  string source_file)
{
  if (g_path_is_absolute (path.c_str()))
    return path;
  gchar *dir = g_path_get_dirname (source_file.c_str());
  string apath = string(dir) + G_DIR_SEPARATOR_S + path;
  g_free (dir);
  return apath;
}

static string
glue_untyped_pspec_constructor (const Parser &parser,
                                const Param  &param)
{
  const string group = (param.group != "") ? param.group.escaped() : "NULL";
  switch (parser.typeOf (param.type))
    {
    case OBJECT:
      {
        /* FIXME: the ParamSpec doesn't transport the type of the objects we require */
        string pspec = "sfidl_pspec_Proxy";
        if (param.args == "")
          pspec += "_default (" + group + ",\"" + param.name + "\")";
        else
          pspec += " (" + group + ",\"" + param.name + "\"," + param.args + ")";
        return pspec;
      }
    default:
      {
        string pspec = "sfidl_pspec_" + param.pspec;
        if (param.args == "")
          pspec += "_default (" + group + ",\"" + param.name + "\")";
        else
          pspec += " (" + group + ",\"" + param.name + "\"," + param.args + ")";
        return pspec;
      }
    }
}

class LanguageBindingCoreCxx : public CodeGenerator {
  struct Image {
    std::string file;
    std::string method;
    Image (const std::string &f = "",
           const std::string &m = "")
      : file (f), method (m)
    {}
  };
  vector<Image> images;
  vector<const Method*> procs;
public:
  LanguageBindingCoreCxx (const Parser &p)
    : CodeGenerator (p)
  {
  }
  const char*
  intern (const string &str)
  {
    return g_intern_string (str.c_str());
  }
  string
  untyped_pspec_constructor (const Param &param)
  {
    const string group = (param.group != "") ? param.group.escaped() : "NULL";
    switch (parser.typeOf (param.type))
      {
      case CHOICE:
        {
          string pspec = "sfidl_pspec_Choice";
          if (param.args == "")
            pspec += "_default";
          pspec += " (" + group + ", \"" + param.name + "\", ";
          if (param.args != "")
            pspec += param.args + ", ";
          pspec += param.type + "_choice_values()";
          pspec += ")";
          return pspec;
        }
      case SFIREC:
        {
          string pspec = "sfidl_pspec_Rec";
          if (param.args == "")
            pspec += "_default (" + group + ", \"" + param.name + "\", ";
          else
            pspec += " (" + group + ", \"" + param.name + "\", " + param.args;
          pspec += ")";
          return pspec;
        }
      case RECORD:
        {
          string pspec = "sfidl_pspec_Record";
          if (param.args == "")
            pspec += "_default (" + group + ", \"" + param.name + "\", ";
          else
            pspec += " (" + group + ", \"" + param.name + "\", " + param.args + ", ";
          pspec += param.type + "::get_fields()";
          pspec += ")";
          return pspec;
        }
      case SEQUENCE:
        {
          string pspec = "sfidl_pspec_Sequence";
          if (param.args == "")
            pspec += "_default (" + group + ", \"" + param.name + "\", ";
          else
            pspec += " (" + group + ", \"" + param.name + "\", " + param.args + ", ";
          pspec += param.type + "::get_element()";
          pspec += ")";
          return pspec;
        }
      default:  return glue_untyped_pspec_constructor (parser, param);
      }
  }
  const char*
  make_TYPE_NAME (const string &type_name)
  {
    vector<string> astrs;
    astrs.push_back ("TYPE");
    return intern (rename (ABSOLUTE, type_name, UPPER, "_", astrs, UPPER, "_").c_str());
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
  const char*
  make_PrefixedTypeName (const string &type_name)
  {
    vector<string> empty;
    string s = rename (ABSOLUTE, type_name, Capitalized, "", empty, Capitalized, "");
    return intern (s);
  }
  const char*
  pure_TypeName (const string &type_name)
  {
    vector<string> empty;
    string s = rename (NONE, type_name, UPPER, "", empty, Capitalized, "");
    return intern (s);
  }
  const char*
  make_FULL_UPPER (const string &type_name)
  {
    vector<string> empty;
    string s = rename (ABSOLUTE, type_name, UPPER, "_", empty, UPPER, "_");
    return intern (s);
  }
  const char*
  pure_UPPER (const string &type_name)
  {
    vector<string> empty;
    string s = rename (NONE, type_name, UPPER, "_", empty, UPPER, "_");
    return intern (s);
  }
  const char*
  make_scheme_name (const string &type_name)
  {
    vector<string> empty;
    return intern (rename (ABSOLUTE, type_name, lower, "-", empty, lower, "-").c_str());
  }
  const char*
  make_full_lower (const string &type_name)
  {
    vector<string> empty;
    return intern (rename (ABSOLUTE, type_name, lower, "_", empty, lower, "_").c_str());
  }
  const char*
  pure_lower (const string &type_name)
  {
    vector<string> empty;
    return intern (rename (NONE, type_name, lower, "_", empty, lower, "_").c_str());
  }
  string
  typed_pspec_constructor (const Param &param)
  {
    const string group = (param.group != "") ? param.group.escaped() : "NULL";
    switch (parser.typeOf (param.type))
      {
      case CHOICE:
        {
          string pspec = "sfidl_pspec_GEnum";
          if (param.args == "")
            pspec += "_default";
          pspec += " (" + group + ", \"" + param.name + "\", ";
          if (param.args != "")
            pspec += param.args + ", ";
          pspec += make_TYPE_NAME (param.type);
          pspec += ")";
          return pspec;
        }
      case RECORD:
        {
          string pspec = "sfidl_pspec_BoxedRec";
          if (param.args == "")
            pspec += "_default (" + group + ", \"" + param.name + "\", ";
          else
            pspec += " (" + group + ", \"" + param.name + "\", " + param.args + ", ";
          pspec += make_TYPE_NAME (param.type);
          pspec += ")";
          return pspec;
        }
      case SEQUENCE:
        {
          string pspec = "sfidl_pspec_BoxedSeq";
          if (param.args == "")
            pspec += "_default (" + group + ", \"" + param.name + "\", ";
          else
            pspec += " (" + group + ", \"" + param.name + "\", " + param.args + ", ";
          pspec += make_TYPE_NAME (param.type);
          pspec += ")";
          return pspec;
        }
      case OBJECT:
        {
          string pspec = "sfidl_pspec_Object";
          if (param.args == "")
            pspec += "_default";
          pspec += " (" + group + ", \"" + param.name + "\", ";
          if (param.args != "")
            pspec += param.args + ", ";
          pspec += make_TYPE_NAME (param.type);
          pspec += ")";
          return pspec;
        }
      default:    return untyped_pspec_constructor (param);
      }
  }
  const char*
  TypeField (const string& type)
  {
    switch (parser.typeOf (type))
      {
      case VOID:        return "void";
      case BOOL:        return "bool";
      case INT:         return "Sfi::Int";
      case NUM:         return "Sfi::Num";
      case REAL:        return "Sfi::Real";
      case CHOICE:      return make_fqtn (type);
      case STRING:      return "Sfi::String";
      case BBLOCK:      return "Sfi::BBlock";
      case FBLOCK:      return "Sfi::FBlock";
      case SFIREC:      return "Sfi::Rec";
      case RECORD:      return make_fqtn (type, "Handle");
      case SEQUENCE:    return make_fqtn (type);
      case OBJECT:      return make_fqtn (type, "*");
      default:          g_assert_not_reached(); return NULL;
      }
  }
  const char*
  TypeArg (const std::string &type)
  {
    switch (parser.typeOf (type))
      {
      case VOID:
      case BOOL:
      case INT:
      case NUM:
      case REAL:
      case CHOICE:      return TypeField (type);
      case STRING:
      case BBLOCK:
      case FBLOCK:
      case SFIREC:
      case RECORD:
      case SEQUENCE:    return intern (string ("const ") + TypeField (type) + " &");
      case OBJECT:      return make_fqtn (type, "*");
      default:          g_assert_not_reached(); return NULL;
      }
  }
  const char*
  TypeRet (const string& type)
  {
    switch (parser.typeOf (type))
      {
      default:          return TypeField (type);
      }
  }
  const char*
  func_value_set_param (const Param &param)
  {
    switch (parser.typeOf (param.type))
      {
      case BOOL:        return "sfi_value_set_bool";
      case INT:         return "sfi_value_set_int";
      case NUM:         return "sfi_value_set_num";
      case REAL:        return "sfi_value_set_real";
      case CHOICE:      return "g_value_set_enum";
      case STRING:      return "::Sfi::String::value_set_string";
      case BBLOCK:      return "::Sfi::BBlock::value_set_bblock";
      case FBLOCK:      return "::Sfi::FBlock::value_set_fblock";
      case SFIREC:      return "::Sfi::Rec::value_set_rec";
      case RECORD:
      case SEQUENCE:    return intern (make_fqtn (param.type) + string ("::value_set_boxed"));
      case OBJECT:      return "g_value_set_object";
      default:          g_assert_not_reached(); return NULL;
      }
  }
  const char*
  func_value_get_param (const Param &param,
                        const string dest = "")
  {
    switch (parser.typeOf (param.type))
      {
      case BOOL:        return "sfi_value_get_bool";
      case INT:         return "sfi_value_get_int";
      case NUM:         return "sfi_value_get_num";
      case REAL:        return "sfi_value_get_real";
      case CHOICE:      return intern (string ("(") + make_fqtn (param.type) + ") g_value_get_enum");
      case STRING:      return "::Sfi::String::value_get_string";
      case BBLOCK:      return "::Sfi::BBlock::value_get_bblock";
      case FBLOCK:      return "::Sfi::FBlock::value_get_fblock";
      case SFIREC:      return "::Sfi::Rec::value_get_rec";
      case RECORD:
      case SEQUENCE:    return intern (make_fqtn (param.type) + string ("::value_get_boxed"));
      case OBJECT:
        if (dest != "")
          return intern ("(" + dest + "*) ::Bse::g_value_get_object< " + dest + "Base*>");
        else
          return "(GObject*) g_value_get_object";
      default:          g_assert_not_reached(); return NULL;
      }
  }
  void
  generate_choice_prototypes (NamespaceHelper& nspace)
  {
    printf ("\n\n/* choice prototypes */\n");
    for (vector<Choice>::const_iterator ci = parser.getChoices().begin(); ci != parser.getChoices().end(); ci++)
      {
        if (parser.fromInclude (ci->name))
          continue;
        nspace.setFromSymbol(ci->name);
        const char *name = nspace.printable_form (ci->name);
        printf ("static inline SfiChoiceValues %s_choice_values();\n", name);
      }
  }
  void
  generate_choice_implementations (NamespaceHelper& nspace)
  {
    printf ("\n\n/* choice implementations */\n");
    for (vector<Choice>::const_iterator ci = parser.getChoices().begin(); ci != parser.getChoices().end(); ci++)
      {
        if (parser.fromInclude (ci->name))
          continue;
        nspace.setFromSymbol(ci->name);
        const char *name = nspace.printable_form (ci->name);
        printf ("static inline SfiChoiceValues\n");
        printf ("%s_choice_values()\n", name);
        printf ("{\n");
        printf ("  static const SfiChoiceValue values[%u] = {\n", ci->contents.size());
        for (vector<ChoiceValue>::const_iterator vi = ci->contents.begin(); vi != ci->contents.end(); vi++)
          printf ("    { \"%s\", \"%s\" },\n", make_FULL_UPPER (vi->name), vi->text.c_str());
        printf ("  };\n");
        printf ("  static const SfiChoiceValues choice_values = {\n");
        printf ("    sizeof (values), values,\n");
        printf ("  };\n");
        printf ("  return choice_values;\n");
        printf ("}\n\n");
      }
  }
  void
  generate_enum_prototypes (NamespaceHelper& nspace)
  {
    printf ("\n\n/* enum prototypes */\n");
    for (vector<Choice>::const_iterator ci = parser.getChoices().begin(); ci != parser.getChoices().end(); ci++)
      {
        if (parser.fromInclude (ci->name))
          continue;
        nspace.setFromSymbol(ci->name);
        // const char *name = nspace.printable_form (ci->name);
        printf ("#define %s (BSE_CXX_DECLARED_ENUM_TYPE (%s))\n",
                make_TYPE_NAME (ci->name), pure_TypeName (ci->name));
      }
  }
  void
  generate_enum_definitions (NamespaceHelper& nspace)
  {
    printf ("\n\n/* enum definitions */\n");
    for (vector<Choice>::const_iterator ci = parser.getChoices().begin(); ci != parser.getChoices().end(); ci++)
      {
        if (parser.fromInclude (ci->name))
          continue;
        nspace.setFromSymbol(ci->name);
        const char *name = nspace.printable_form (ci->name);
        printf ("enum %s {\n", name);
        for (vector<ChoiceValue>::const_iterator vi = ci->contents.begin(); vi != ci->contents.end(); vi++)
          printf ("  %s = %d,\n", pure_UPPER (vi->name), vi->sequentialValue);
        printf ("};\n");
      }
  }
  void
  generate_enum_implementations (NamespaceHelper& nspace)
  {
    printf ("\n\n/* enum implementations */\n");
    for (vector<Choice>::const_iterator ci = parser.getChoices().begin(); ci != parser.getChoices().end(); ci++)
      {
        if (parser.fromInclude (ci->name))
          continue;
        nspace.setFromSymbol(ci->name);
        // const char *name = nspace.printable_form (ci->name);
        printf ("BSE_CXX_DECLARE_ENUM (%s, \"%s\", %u,\n",
                pure_TypeName (ci->name), make_PrefixedTypeName (ci->name), ci->contents.size());
        for (vector<ChoiceValue>::const_iterator vi = ci->contents.begin(); vi != ci->contents.end(); vi++)
          printf ("                      *v++ = ::Bse::EnumValue (%d, \"%s\", \"%s\" );\n",
                  vi->sequentialValue, make_FULL_UPPER (vi->name), vi->text.c_str());
        printf ("                      );\n");
      }
  }
  void
  generate_record_prototypes (NamespaceHelper& nspace)
  {
    printf ("\n\n/* record prototypes */\n");
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name))
          continue;
        
        nspace.setFromSymbol(ri->name);
        const char *name = nspace.printable_form (ri->name);
        
        printf ("class %s;\n", pure_TypeName (ri->name));
        printf ("typedef Sfi::RecordHandle<%s> %sHandle;\n", name, name);
        printf ("#define %s BSE_CXX_DECLARED_RECORD_TYPE (%s)\n",
                make_TYPE_NAME (ri->name), pure_TypeName (ri->name));
      }
  }
  void
  generate_sequence_prototypes (NamespaceHelper& nspace)
  {
    printf ("\n\n/* sequence prototypes */\n");
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name))
          continue;
        nspace.setFromSymbol(si->name);
        
        printf ("class %s;\n", pure_TypeName (si->name));
        printf ("#define %s BSE_CXX_DECLARED_SEQUENCE_TYPE (%s)\n",
                make_TYPE_NAME (si->name), pure_TypeName (si->name));
      }
  }
  void
  generate_record_definitions (NamespaceHelper& nspace)
  {
    printf ("\n\n/* record definitions */\n");
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name))
          continue;
        nspace.setFromSymbol(ri->name);
        
        printf ("class %s : public ::Sfi::GNewable {\n", pure_TypeName (ri->name));
        printf ("public:\n");
        for (vector<Param>::const_iterator pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
          {
            printf ("  %s %s;\n", TypeField (pi->type), pi->name.c_str());
          }
        printf ("  static inline %s from_rec (SfiRec *rec);\n", TypeRet (ri->name));
        printf ("  static inline SfiRec *to_rec (%s ptr);\n", TypeArg (ri->name));
        printf ("  static inline %s value_get_boxed (const GValue *value);\n", TypeRet (ri->name));
        printf ("  static inline void value_set_boxed (GValue *value, %s self);\n", TypeArg (ri->name));
        printf ("  static inline const char* options   () { return %s; }\n", ri->infos.get("options").escaped().c_str());
        printf ("  static inline const char* blurb     () { return %s; }\n", ri->infos.get("blurb").escaped().c_str());
        printf ("  static inline const char* authors   () { return %s; }\n", ri->infos.get("authors").escaped().c_str());
        printf ("  static inline const char* license   () { return %s; }\n", ri->infos.get("license").escaped().c_str());
        printf ("  static inline const char* type_name () { return \"%s\"; }\n", make_PrefixedTypeName (ri->name));
        printf ("  static inline SfiRecFields get_fields ();\n");
        printf ("};\n");
        printf ("\n");
      }
  }
  void
  generate_sequence_definitions (NamespaceHelper& nspace)
  {
    printf ("\n\n/* sequence definitions */\n");
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name))
          continue;
        nspace.setFromSymbol(si->name);
        
        printf ("class %s : public Sfi::Sequence< %s > {\n", pure_TypeName (si->name), TypeField (si->content.type));
        printf ("public:\n");
        /* TODO: make this a constructor? */
        printf ("  static inline %s from_seq (SfiSeq *seq);\n", TypeRet (si->name));
        printf ("  static inline SfiSeq *to_seq (%s seq);\n", TypeArg (si->name));
        printf ("  static inline %s value_get_boxed (const GValue *value);\n", TypeRet (si->name));
        printf ("  static inline void value_set_boxed (GValue *value, %s self);\n", TypeArg (si->name));
        printf ("  static inline const char* options   () { return %s; }\n", si->infos.get("options").escaped().c_str());
        printf ("  static inline const char* blurb     () { return %s; }\n", si->infos.get("blurb").escaped().c_str());
        printf ("  static inline const char* authors   () { return %s; }\n", si->infos.get("authors").escaped().c_str());
        printf ("  static inline const char* license   () { return %s; }\n", si->infos.get("license").escaped().c_str());
        printf ("  static inline const char* type_name () { return \"%s\"; }\n", make_PrefixedTypeName (si->name));
        printf ("  static inline GParamSpec* get_element ();\n");
        printf ("};\n");
        printf ("\n");
      }
  }
  void
  generate_record_implementations (NamespaceHelper& nspace)
  {
    printf ("\n\n/* record implementations */\n");
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name))
          continue;
        nspace.setFromSymbol(ri->name);
        const char *nname = nspace.printable_form (ri->name);
        
        printf ("BSE_CXX_DECLARE_RECORD (%s);\n", pure_TypeName (ri->name));
        printf ("%s\n", TypeRet (ri->name));
        printf ("%s::from_rec (SfiRec *sfi_rec)\n", nname);
        printf ("{\n");
        printf ("  GValue *element;\n");
        printf ("\n");
        printf ("  if (!sfi_rec)\n");
        printf ("    return Sfi::INIT_NULL;\n");
        printf ("\n");
        printf ("  %s rec = Sfi::INIT_DEFAULT;\n", TypeField (ri->name));
        for (vector<Param>::const_iterator pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
          {
            printf ("  element = sfi_rec_get (sfi_rec, \"%s\");\n", pi->name.c_str());
            printf ("  if (element)\n");
            printf ("    rec->%s = %s (element);\n", pi->name.c_str(), func_value_get_param (*pi));
          }
        printf ("  return rec;\n");
        printf ("}\n\n");
        
        printf ("SfiRec *\n");
        printf ("%s::to_rec (%s rec)\n", nname, TypeArg (ri->name));
        printf ("{\n");
        printf ("  SfiRec *sfi_rec;\n");
        printf ("  GValue *element;\n");
        printf ("\n");
        printf ("  if (!rec)\n");
        printf ("    return NULL;\n");
        printf ("\n");
        printf ("  sfi_rec = sfi_rec_new ();\n");
        for (vector<Param>::const_iterator pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
          {
            printf ("  element = sfi_rec_forced_get (sfi_rec, \"%s\", %s);\n", pi->name.c_str(), make_TYPE_NAME (pi->type));
            printf ("  %s (element, rec->%s);\n", func_value_set_param (*pi), pi->name.c_str());
          }
        printf ("  return sfi_rec;\n");
        printf ("}\n\n");
        
        /* FIXME: client only, core needs type system support */
        printf ("%s\n", TypeRet (ri->name));
        printf ("%s::value_get_boxed (const GValue *value)\n", nname);
        printf ("{\n");
        printf ("  return ::Sfi::cxx_value_get_boxed_record< %s> (value);\n", nname);
        printf ("}\n\n");
        printf ("void\n");
        printf ("%s::value_set_boxed (GValue *value, %s self)\n", nname, TypeArg (ri->name));
        printf ("{\n");
        printf ("  ::Sfi::cxx_value_set_boxed_record< %s> (value, self);\n", nname);
        printf ("}\n\n");
        
        printf ("SfiRecFields\n");
        printf ("%s::get_fields()\n", nname);
        printf ("{\n");
        printf ("  static SfiRecFields rfields = { 0, NULL };\n");
        printf ("  if (!rfields.n_fields)\n");
        printf ("    {\n");
        printf ("      static GParamSpec *fields[%u + 1];\n", ri->contents.size());
        printf ("      rfields.n_fields = %u;\n", ri->contents.size());
        guint j = 0;
        for (vector<Param>::const_iterator pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
          {
            // printf ("#line %u \"%s\"\n", pi->line, parser.fileName().c_str());
            printf ("      fields[%u] = %s;\n", j++, untyped_pspec_constructor (*pi).c_str());
          }
        printf ("      rfields.fields = fields;\n");
        printf ("    }\n");
        printf ("  return rfields;\n");
        printf ("}\n");
      }
  }
  void
  generate_sequence_implementations (NamespaceHelper& nspace)
  {
    printf ("\n\n/* sequence implementations */\n");
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name))
          continue;
        nspace.setFromSymbol(si->name);
        const char *nname = nspace.printable_form (si->name);
        
        printf ("BSE_CXX_DECLARE_SEQUENCE (%s);\n", pure_TypeName (si->name));
        printf ("%s\n", TypeRet (si->name));
        printf ("%s::from_seq (SfiSeq *sfi_seq)\n", nname);
        printf ("{\n");
        printf ("  %s seq;\n", TypeRet (si->name));
        printf ("  guint i, length;\n");
        printf ("\n");
        printf ("  g_return_val_if_fail (sfi_seq != NULL, seq);\n");
        printf ("\n");
        printf ("  length = sfi_seq_length (sfi_seq);\n");
        printf ("  seq.resize (length);\n");
        printf ("  for (i = 0; i < length; i++)\n");
        printf ("  {\n");
        printf ("    GValue *element = sfi_seq_get (sfi_seq, i);\n");
        printf ("    seq[i] = %s (element);\n", func_value_get_param (si->content));
        printf ("  }\n");
        printf ("  return seq;\n");
        printf ("}\n\n");
        
        printf ("SfiSeq *\n");
        printf ("%s::to_seq (%s seq)\n", nname, TypeArg (si->name));
        printf ("{\n");
        printf ("  SfiSeq *sfi_seq = sfi_seq_new ();\n");
        printf ("  for (guint i = 0; i < seq.length(); i++)\n");
        printf ("  {\n");
        printf ("    GValue *element = sfi_seq_append_empty (sfi_seq, %s);\n", make_TYPE_NAME (si->content.type));
        printf ("    %s (element, seq[i]);\n", func_value_set_param (si->content));
        printf ("  }\n");
        printf ("  return sfi_seq;\n");
        printf ("}\n\n");
        
        printf ("%s\n", TypeRet (si->name));
        printf ("%s::value_get_boxed (const GValue *value)\n", nname);
        printf ("{\n");
        printf ("  return ::Sfi::cxx_value_get_boxed_sequence< %s> (value);\n", nname);
        printf ("}\n\n");
        printf ("void\n");
        printf ("%s::value_set_boxed (GValue *value, %s self)\n", nname, TypeArg (si->name));
        printf ("{\n");
        printf ("  ::Sfi::cxx_value_set_boxed_sequence< %s> (value, self);\n", nname);
        printf ("}\n\n");
        
        printf ("GParamSpec*\n");
        printf ("%s::get_element()\n", nname);
        printf ("{\n");
        printf ("  static GParamSpec *element = NULL;\n");
        printf ("  if (!element)\n");
        // printf ("#line %u \"%s\"\n", si->content.line, parser.fileName().c_str());
        printf ("    element = %s;\n", untyped_pspec_constructor (si->content).c_str());
        printf ("  return element;\n");
        printf ("}\n\n");
      }
  }
  void
  generate_class_prototypes (NamespaceHelper& nspace)
  {
    printf ("\n\n/* class prototypes */\n");
    for (vector<Class>::const_iterator ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
      {
        if (parser.fromInclude (ci->name))
          continue;
        nspace.setFromSymbol(ci->name);
        printf ("class %sBase;\n", pure_TypeName (ci->name));
        printf ("class %s;\n", pure_TypeName (ci->name));
        printf ("#define %s (BSE_CXX_DECLARED_CLASS_TYPE (%s))\n",
                make_TYPE_NAME (ci->name), pure_TypeName (ci->name));
      }
  }
  void
  generate_class_definitions (NamespaceHelper& nspace)
  {
    printf ("\n\n/* class definitions */\n");
    for (vector<Class>::const_iterator ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
      {
        if (parser.fromInclude (ci->name))
          continue;
        nspace.setFromSymbol(ci->name);
        // const char *name = nspace.printable_form (ci->name);
        const char *ctName = pure_TypeName (ci->name);
        const char *ctNameBase = intern (ctName + string ("Base"));
        const char *ctProperties = intern (ctName + string ("Properties"));
        const char *ctPropertyID = intern (ctName + string ("PropertyID"));
        vector<string> destroy_jobs;
        
        /* skeleton class declaration + type macro */
        printf ("BSE_CXX_DECLARE_CLASS (%s);\n", pure_TypeName (ci->name));
        printf ("class %s : public %s {\n", ctNameBase, make_fqtn (ci->inherits));
        
        /* class Info strings */
        /* pixstream(), this is a bit of a hack, we make it a template rather than
         * a normal inline method to avoid huge images in debugging code
         */
        string icon = ci->infos.get("icon");
        string pstream = "NULL";
        if (icon != "")
          {
            printf ("  template<bool> static inline const unsigned char* inlined_pixstream();\n");
            images.push_back (Image (include_relative (icon, ci->file),
                                     string ("template<bool> const unsigned char*\n") +
                                     make_fqtn (ci->name) + "Base" +
                                     "::inlined_pixstream()"));
            pstream = "inlined_pixstream<true>()";
          }
        printf ("public:\n");
        printf ("  static inline const unsigned char* pixstream () { return %s; }\n", pstream.c_str());
        printf ("  static void               class_init (::Bse::CxxBaseClass *klass);\n");
        printf ("  static inline const char* options   () { return %s; }\n", ci->infos.get("options").escaped().c_str());
        printf ("  static inline const char* category  () { static const char *c = NULL; const char *category = %s; \n",
                ci->infos.get("category").escaped().c_str());
        printf ("    if (!c && category[0]) c = g_intern_strconcat (\"/Modules\", category[0] == '/' ? \"\" : \"/\", category, NULL);\n");
        printf ("    return c; }\n");
        printf ("  static inline const char* blurb     () { return %s; }\n", ci->infos.get("blurb").escaped().c_str());
        printf ("  static inline const char* authors   () { return %s; }\n", ci->infos.get("authors").escaped().c_str());
        printf ("  static inline const char* license   () { return %s; }\n", ci->infos.get("license").escaped().c_str());
        printf ("  static inline const char* type_name () { return \"%s\"; }\n", make_PrefixedTypeName (ci->name));
        
        /* i/j/o channel names */
        int is_public = 0;
        if (ci->istreams.size())
          {
            if (!is_public++)
              printf ("public:\n");
            printf ("  enum {\n");
            for (vector<Stream>::const_iterator si = ci->istreams.begin(); si != ci->istreams.end(); si++)
              printf ("    ICHANNEL_%s,\n", pure_UPPER (si->ident));
            printf ("    N_ICHANNELS\n  };\n");
          }
        if (ci->jstreams.size())
          {
            if (!is_public++)
              printf ("public:\n");
            printf ("  enum {\n");
            for (vector<Stream>::const_iterator si = ci->jstreams.begin(); si != ci->jstreams.end(); si++)
              printf ("    JCHANNEL_%s,\n", pure_UPPER (si->ident));
            printf ("    N_JCHANNELS\n  };\n");
          }
        if (ci->ostreams.size())
          {
            if (!is_public++)
              printf ("public:\n");
            printf ("  enum {\n");
            for (vector<Stream>::const_iterator si = ci->ostreams.begin(); si != ci->ostreams.end(); si++)
              printf ("    OCHANNEL_%s,\n", pure_UPPER (si->ident));
            printf ("    N_OCHANNELS\n  };\n");
          }
        
        /* "Properties" structure for synthesis modules */
        if (ci->properties.size() && ci->istreams.size() + ci->jstreams.size() + ci->ostreams.size())
          {
            if (!is_public++)
              printf ("public:\n");
            printf ("  /* \"transport\" structure to configure synthesis modules from properties */\n");
            printf ("  struct %s {\n", ctProperties);
            for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
              printf ("    %s %s;\n", TypeField (pi->type), pi->name.c_str());
            printf ("    explicit %s (%s *p) ", ctProperties, ctNameBase);
            for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
              printf ("%c\n      %s (p->%s)", pi == ci->properties.begin() ? ':' : ',', pi->name.c_str(), pi->name.c_str());
            printf ("\n    {\n");
            printf ("    }\n");
            printf ("  };\n");
          }
        
        /* property fields */
        printf ("protected:\n");
        for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
          printf ("  %s %s;\n", TypeField (pi->type), pi->name.c_str());
        
        /* property IDs */
        if (ci->properties.begin() != ci->properties.end())
          {
            printf ("protected:\n  enum %s {\n", ctPropertyID);
            vector<Param>::const_iterator pi = ci->properties.begin();
            printf ("    PROP_%s = 1,\n", pure_UPPER (pi->name));
            for (pi++; pi != ci->properties.end(); pi++)
              printf ("    PROP_%s,\n", pure_UPPER (pi->name));
            printf ("  };\n");
          }
        
        /* property setter */
        printf ("public:\n");
        printf ("  void set_property (guint prop_id, const ::Bse::Value &value, GParamSpec *pspec)\n");
        printf ("  {\n");
        printf ("    switch (prop_id) {\n");
        for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
          {
            printf ("    case PROP_%s:\n", pure_UPPER (pi->name));
            printf ("      %s = %s (&value);\n", pi->name.c_str(), func_value_get_param (*pi));
            printf ("    break;\n");
          }
        printf ("    };\n");
        printf ("    property_changed ((%s) prop_id);\n", ctPropertyID);
        printf ("    update_modules();\n");
        /* reset triggers */
        printf ("    switch (prop_id) {\n");
        for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
          {
            if (pi->pspec != "Trigger")
              continue;
            printf ("    case PROP_%s:\n", pure_UPPER (pi->name));
            printf ("      %s = FALSE;\n", pi->name.c_str());
            printf ("    break;\n");
          }
        printf ("    default: ;\n");
        printf ("    };\n");
        printf ("  }\n");
        
        /* property getter */
        printf ("  void get_property (guint prop_id, ::Bse::Value &value, GParamSpec *pspec)\n");
        printf ("  {\n");
        printf ("    switch (prop_id) {\n");
        for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
          {
            printf ("    case PROP_%s:\n", pure_UPPER (pi->name));
            printf ("      %s (&value, %s);\n", func_value_set_param (*pi), pi->name.c_str());
            printf ("    break;\n");
          }
        printf ("    };\n");
        printf ("  }\n");
        
        /* static data */
        printf ("private:\n");
        printf ("  static struct StaticData {\n");
        printf ("    int dummy;\n");
        for (vector<Method>::const_iterator si = ci->signals.begin(); si != ci->signals.end(); si++)
          {
            const gchar *sig_name = canonify_name (si->name, '_');
            printf ("    guint signal_%s;\n", sig_name);
          }
        printf ("  } static_data;\n");
        
        /* property-changed hooking */
        printf ("protected:\n");
        printf ("  virtual void property_changed (%s) {}\n", ctPropertyID);
        
        /* methods */
        for (vector<Method>::const_iterator mi = ci->methods.begin(); mi != ci->methods.end(); mi++)
          procs.push_back (&(*mi));
        
        /* destructor */
        printf ("  virtual ~%s ()\n", ctNameBase);
        printf ("  {\n");
        /* property deletion */
        for (vector<string>::const_iterator vi = destroy_jobs.begin(); vi != destroy_jobs.end(); vi++)
          printf ("    %s;\n", vi->c_str());
        printf ("  }\n");
        
        /* signal emission methods */
        printf ("public:\n");
        for (vector<Method>::const_iterator si = ci->signals.begin(); si != ci->signals.end(); si++)
          {
            const gchar *sig_name = canonify_name (si->name, '_');
            printf ("  void emit_%s (", sig_name);
            for (vector<Param>::const_iterator ai = si->params.begin(); ai != si->params.end(); ai++)
              {
                if (ai != si->params.begin())
                  printf (", ");
                printf ("%s %s", TypeArg (ai->type), ai->name.c_str());
              }
            printf (")\n");
            printf ("  {\n");
            printf ("    GValue args[1 + %u];\n", si->params.size());
            printf ("    args[0].g_type = 0, g_value_init (args + 0, %s);\n", make_TYPE_NAME (ci->name));
            printf ("    g_value_set_object (args + 0, gobject());\n");
            guint i = 1;
            for (vector<Param>::const_iterator ai = si->params.begin(); ai != si->params.end(); ai++, i++)
              {
                printf ("    args[%u].g_type = 0, g_value_init (args + %u, %s);\n", i, i, make_TYPE_NAME (ai->type));
                printf ("    %s (args + %u, %s);\n", func_value_set_param (*ai), i, ai->name.c_str());
              }
            printf ("    g_signal_emitv (args, static_data.signal_%s, 0, NULL);\n", sig_name);
            for (i = 0; i <= si->params.size(); i++)
              printf ("    g_value_unset (args + %u);\n", i);
            printf ("  }\n");
          }
        
        /* done */
        printf ("};\n"); /* finish: class ... { }; */
      }
  }
  void
  generate_class_implementations (NamespaceHelper& nspace)
  {
    printf ("\n\n/* class implementations */\n");
    for (vector<Class>::const_iterator ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
      {
        if (parser.fromInclude (ci->name))
          continue;
        nspace.setFromSymbol(ci->name);
        const char *nname = nspace.printable_form (ci->name);
        vector<string> destroy_jobs;
        
        /* class_init */
        printf ("void\n");
        printf ("%sBase::class_init (::Bse::CxxBaseClass *klass)\n", nname);
        printf ("{\n");
        for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
          printf ("  klass->add_param (PROP_%s, %s);\n", pure_UPPER (pi->name), typed_pspec_constructor (*pi).c_str());
        for (vector<Stream>::const_iterator si = ci->istreams.begin(); si != ci->istreams.end(); si++)
          printf ("  klass->add_ichannel (%s, %s, ICHANNEL_%s);\n",
                  si->name.escaped().c_str(), si->blurb.escaped().c_str(), pure_UPPER (si->ident));
        for (vector<Stream>::const_iterator si = ci->jstreams.begin(); si != ci->jstreams.end(); si++)
          printf ("  klass->add_jchannel (%s, %s, JCHANNEL_%s);\n",
                  si->name.escaped().c_str(), si->blurb.escaped().c_str(), pure_UPPER (si->ident));
        for (vector<Stream>::const_iterator si = ci->ostreams.begin(); si != ci->ostreams.end(); si++)
          printf ("  klass->add_ochannel (%s, %s, OCHANNEL_%s);\n",
                  si->name.escaped().c_str(), si->blurb.escaped().c_str(), pure_UPPER (si->ident));
        for (vector<Method>::const_iterator si = ci->signals.begin(); si != ci->signals.end(); si++)
          {
            const gchar *sig_name = canonify_name (si->name, '_');
            const gchar *sig_string = canonify_name (si->name);
            printf ("  static_data.signal_%s =\n      klass->add_signal (\"%s\", (GSignalFlags) 0, %u",
                    sig_name, sig_string, si->params.size());
            for (vector<Param>::const_iterator ai = si->params.begin(); ai != si->params.end(); ai++)
              printf (",\n                       %s", make_TYPE_NAME (ai->type));
            printf (");\n");
          }
        printf ("}\n");
      }
  }
  void
  generate_procedure_prototypes (NamespaceHelper&            nspace)
  {
    printf ("\n\n/* procedure prototypes */\n");
    for (vector<const Method*>::const_iterator ppi = procs.begin(); ppi != procs.end(); ppi++)
      {
        const Method *mi = *ppi;
        if (parser.fromInclude (mi->name))
          continue;
        nspace.setFromSymbol(mi->name);

        printf ("namespace Procedure {\n");
        printf ("class %s;\n", pure_lower (mi->name));
        printf ("#define %s (BSE_CXX_DECLARED_PROC_TYPE (%s))\n",
                make_TYPE_NAME (mi->name), pure_lower (mi->name));
        printf ("} // Procedure\n\n");
      }
  }
  void
  generate_procedure_implementations (NamespaceHelper &nspace)
  {
    printf ("\n\n/* procedure implementations */\n");
    for (vector<const Method*>::const_iterator ppi = procs.begin(); ppi != procs.end(); ppi++)
      {
        const Method *mi = *ppi;
        if (parser.fromInclude (mi->name))
          continue;
        nspace.setFromSymbol(mi->name);
        const char *name = nspace.printable_form (mi->name);
        const Map<std::string, IString> &infos = mi->infos;
        bool is_void = mi->result.type == "void";
        printf ("namespace Procedure {\n");
        printf ("BSE_CXX_DECLARE_PROC (%s);\n", pure_lower (mi->name));
        printf ("class %s {\n", pure_lower (mi->name));
        
        /* class Info strings */
        /* pixstream(), this is a bit of a hack, we make it a template rather than
         * a normal inline method to avoid huge images in debugging code
         */
        string icon = infos.get("icon");
        string pstream = "NULL";
        if (icon != "")
          {
            printf ("  template<bool> static inline const unsigned char* inlined_pixstream();\n");
            images.push_back (Image (include_relative (icon, mi->file),
                                     string ("template<bool> const unsigned char*\n") +
                                     make_full_lower (mi->name) +
                                     "::inlined_pixstream()"));
            pstream = "inlined_pixstream<true>()";
          }
        printf ("public:\n");
        printf ("  static inline const unsigned char* pixstream () { return %s; }\n", pstream.c_str());
        printf ("  static inline const char* options   () { return %s; }\n", infos.get("options").escaped().c_str());
        printf ("  static inline const char* category  () { static const char *c = NULL; \n");
        printf ("    const char *root_category = %s, *category = %s;\n",
                infos.get("root_category").escaped().c_str(), infos.get("category").escaped().c_str());
        printf ("    if (!c && category[0]) c = g_intern_strconcat (root_category && root_category[0] ? root_category : \"/Proc\",\n");
        printf ("                                    category[0] == '/' ? \"\" : \"/\", category, NULL);\n");
        printf ("    return c; }\n");
        printf ("  static inline const char* blurb     () { return %s; }\n", infos.get("blurb").escaped().c_str());
        printf ("  static inline const char* authors   () { return %s; }\n", infos.get("authors").escaped().c_str());
        printf ("  static inline const char* license   () { return %s; }\n", infos.get("license").escaped().c_str());
        printf ("  static inline const char* type_name () { return \"%s\"; }\n", make_scheme_name (mi->name));
        
        /* return type */
        printf ("  static %s exec (", TypeRet (mi->result.type));
        /* args */
        for (vector<Param>::const_iterator ai = mi->params.begin(); ai != mi->params.end(); ai++)
          {
            if (ai != mi->params.begin())
              printf (", ");
            printf ("%s %s", TypeArg (ai->type), ai->name.c_str());
          }
        printf (");\n");
        
        /* marshal */
        printf ("  static BseErrorType marshal (BseProcedureClass *procedure,\n"
                "                               const GValue      *in_values,\n"
                "                               GValue            *out_values)\n");
        printf ("  {\n");
        printf ("    try {\n");
        if (!is_void)
          printf ("      %s __return_value =\n", TypeRet (mi->result.type));
        printf ("        exec (\n");
        int i = 0;
        for (vector<Param>::const_iterator pi = mi->params.begin(); pi != mi->params.end(); pi++)
          printf ("              %s (in_values + %u)%c\n",
                  func_value_get_param (*pi, make_fqtn (pi->type)), i++,
                  &(*pi) == &(mi->params.back()) ? ' ' : ',');
        printf ("             );\n");
        if (!is_void)
          printf ("      %s (out_values, __return_value);\n", func_value_set_param (mi->result));
        printf ("    } catch (std::exception &e) {\n");
        printf ("      sfi_debug (\"%%s: %%s\", \"%s\", e.what());\n", name);
        printf ("      return BSE_ERROR_PROC_EXECUTION;\n");
        printf ("    } catch (...) {\n");
        printf ("      sfi_debug (\"%%s: %%s\", \"%s\", \"unknown exception\");\n", name);
        printf ("      return BSE_ERROR_PROC_EXECUTION;\n");
        printf ("    }\n");
        printf ("    return BSE_ERROR_NONE;\n");
        printf ("  }\n");
        
        /* init */
        printf ("  static void init (BseProcedureClass *proc,\n"
                "                    GParamSpec       **in_pspecs,\n"
                "                    GParamSpec       **out_pspecs)\n");
        printf ("  {\n");
        for (vector<Param>::const_iterator ai = mi->params.begin(); ai != mi->params.end(); ai++)
          printf ("    *(in_pspecs++) = %s;\n", typed_pspec_constructor (*ai).c_str());
        if (!is_void)
          printf ("    *(out_pspecs++) = %s;\n", typed_pspec_constructor (mi->result).c_str());
        printf ("  }\n");
        
        /* done */
        printf ("};\n"); /* finish: class ... { }; */
        printf ("} // Procedure\n\n");
      }
  }
  void
  run ()
  {
    printf ("\n/*-------- begin %s generated code --------*/\n\n\n", Options::the()->sfidlName.c_str());
    
    /* standard includes */
    printf ("\n#include <bse/bsecxxplugin.h>\n");
    
    /* reset auxillary structures */
    images.resize (0);
    procs.resize (0);

    /* setup namespace state */
    NamespaceHelper nsh(stdout);

    /* prototypes */
    generate_enum_prototypes (nsh);
    generate_choice_prototypes (nsh);
    generate_record_prototypes (nsh);
    generate_sequence_prototypes (nsh);
    generate_class_prototypes (nsh);

    /* definitions */
    generate_enum_definitions (nsh);
    generate_record_definitions (nsh);
    generate_sequence_definitions (nsh);
    generate_class_definitions (nsh);           /* adds to images, procs */

    /* procedure handling */
    for (vector<Method>::const_iterator mi = parser.getProcedures().begin(); mi != parser.getProcedures().end(); mi++)
      procs.push_back (&(*mi));                 /* collect procedures */
    generate_procedure_prototypes (nsh);

    /* implementations */
    generate_enum_implementations (nsh);
    generate_choice_implementations (nsh);
    generate_record_implementations (nsh);
    generate_sequence_implementations (nsh);
    generate_class_implementations (nsh);
    generate_procedure_implementations (nsh);   /* adds to images */

    /* close namespace state */
    nsh.leaveAll();

    // printf ("\nnamespace ... {\n"); // FIXME: do images need a namespace?

    /* image method implementations */
    for (vector<Image>::const_iterator ii = images.begin(); ii != images.end(); ii++)
      {
        printf ("%s\n", ii->method.c_str());
        printf ("{\n");
        gint estatus = 0;
        GError *error = NULL;
        gchar *out, *err = NULL;
        string cmd = string() + "gdk-pixbuf-csource " + "--name=local_pixstream " + ii->file;
        g_spawn_command_line_sync (cmd.c_str(), &out, &err, &estatus, &error);
        if (err && *err)
          g_printerr ("gdk-pixbuf-csource: %s", err);
        if (error || estatus)
          {
            if (error)
              g_printerr ("failed to convert image file \"%s\" with gdk-pixbuf-csource%c %s",
                          ii->file.c_str(), error ? ':' : ' ', error->message);
            exit (estatus & 255 ? estatus : 1);
          }
        g_clear_error (&error);
        g_free (err);
        printf ("  %s\n", out);
        g_free (out);
        printf ("  return local_pixstream;\n");
        printf ("}\n");
      }

    // printf ("\n}; // ...\n"); // FIXME: do images need a namespace?
    
    /* done */
    printf ("\n/*-------- end %s generated code --------*/\n\n\n", Options::the()->sfidlName.c_str());
  }
};

class LanguageBindingCoreCxxFactory : public Factory {
public:
  string option() const	      { return "--module"; }
  string description() const  { return "generate core C++ language binding"; }
  
  void init (Options& options) const
  {
    /* FIXME: keep in sync with bse-plugin-generator.cc */
    options.doImplementation = true;
    options.doInterface = false;
    options.doHeader = true;
    options.doSource = false;
    options.generateBoxedTypes = true;
  }
  
  CodeGenerator *create (const Parser& parser) const
  {
    return new LanguageBindingCoreCxx (parser);
  }
} static_factory;

} // anon

/* vim:set ts=8 sts=2 sw=2: */
