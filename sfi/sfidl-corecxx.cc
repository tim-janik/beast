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

#include <string.h>
#include <stdio.h>
#include <list>
#include <string>
#include <map>
#include <sfi/glib-extra.h>

using namespace std;
using namespace Sfidl;

const Parser         *the_parser = 0;
CodeGeneratorC *the_cgc = 0;

static Type
sfidl_type (const string tname)
{
  /* URGlglglglglglglglglglgl *WHY* do i need to write this? */
  if (tname == "void")
    return VOID;
  if (tname == "Bool")
    return BOOL;
  if (tname == "Int")
    return INT;
  if (tname == "Num")
    return NUM;
  if (tname == "Real")
    return REAL;
  if (tname == "String")
    return STRING;
  if (the_parser->isChoice (tname))
    return CHOICE;
  if (tname == "BBlock")
    return BBLOCK;
  if (tname == "FBlock")
    return FBLOCK;
  if (tname == "PSpec")
    return FBLOCK;
  if (the_parser->isSequence (tname))
    return SEQUENCE;
  if (the_parser->isRecord (tname))
    return RECORD;
  if (the_parser->isClass (tname))
    return OBJECT;
  g_error (("invalid type: " + tname).c_str());
  return VOID;
}

static string
TypeName (const string &str)
{
  int pos = str.rfind (':');
  string s;
  if (pos < 0)  // not fully qualified, prolly an Sfi type
    return str == "void" ? "void" : "Sfi" + str;
  return str.substr (pos + 1);
}
#define cTypeName(s)    TypeName (s).c_str()

static string
TypeRef (const string &str)
{
  string s = TypeName (str);
  if (the_parser->isClass (str))
    s += " &";
  return s;
}
#define cTypeRef(s)    TypeRef (s).c_str()

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
UC_NAME (const string &cstr)
{
  vector<string> vs = split_string (cstr);
  string str = join_string (vs, "_");
  string r;
  char l = 0;
  for (string::const_iterator i = str.begin(); i != str.end(); i++)
    {
      if (islower (l) && isupper (*i))
        r += "_";
      r += toupper (l = *i);
    }
  return r;
}
#define cUC_NAME(s)    UC_NAME (s).c_str()

static string
pspec_constructor (const Param &param)
{
  switch (sfidl_type (param.type))
    {
    case CHOICE:
      {
        const string group = param.group == "" ? "NULL" : param.group;
        string pspec = "sfidl_pspec_GEnum (" + group + ", \"" + param.name + "\", ";
        if (param.args != "")
          pspec += param.args + ", ";
        vector<string> vs = split_string (param.type);
        string pname = vs.back();
        vs.pop_back();
        string nspace = join_string (vs, ":");
        pspec += UC_NAME (nspace) + "_TYPE_" + cUC_NAME (pname);
        pspec += ")";
        return pspec;
      }
    default:    return the_cgc->makeParamSpec (param);
    }
}

static string
func_param_free (const Param &param)
{
  switch (sfidl_type (param.type))
    {
    case BOOL:          return "";
    case INT:           return "";
    case NUM:           return "";
    case REAL:          return "";
    case STRING:        return "g_free";
    case CHOICE:        return "";      /* enum value */
    case BBLOCK:        return "sfi_bblock_unref";
    case FBLOCK:        return "sfi_fblock_unref";
    case PSPEC:         return "g_param_spec_unref";
    case SEQUENCE:      return "sfi_seq_unref";
    case RECORD:        return "sfi_rec_unref";
    case OBJECT:        return "g_object_unref";
    default:            return "*** ERROR ***";
    }
}

static string
func_value_dup_param (const Param &param)
{
  switch (sfidl_type (param.type))
    {
    case BOOL:          return "sfi_value_get_bool";
    case INT:           return "sfi_value_get_int";
    case NUM:           return "sfi_value_get_num";
    case REAL:          return "sfi_value_get_real";
    case STRING:        return "sfi_value_dup_string";
    case CHOICE:        return "(" + TypeName (param.type) + ") g_value_get_enum";
    case BBLOCK:        return "sfi_value_dup_bblock";
    case FBLOCK:        return "sfi_value_dup_fblock";
    case PSPEC:         return "sfi_value_dup_pspec";
    case SEQUENCE:      return "sfi_value_dup_seq";
    case RECORD:        return "sfi_value_dup_rec";
    case OBJECT:        return "g_value_dup_object";
    default:            return "*** ERROR ***";
    }
}

static string
func_value_set_param (const Param &param)
{
  switch (sfidl_type (param.type))
    {
    case BOOL:          return "sfi_value_set_bool";
    case INT:           return "sfi_value_set_int";
    case NUM:           return "sfi_value_set_num";
    case REAL:          return "sfi_value_set_real";
    case STRING:        return "sfi_value_set_string";
    case CHOICE:        return "g_value_set_enum";
    case BBLOCK:        return "sfi_value_set_bblock";
    case FBLOCK:        return "sfi_value_set_fblock";
    case PSPEC:         return "sfi_value_set_pspec";
    case SEQUENCE:      return "sfi_value_set_seq";
    case RECORD:        return "sfi_value_set_rec";
    case OBJECT:        return "g_value_set_object";
    default:            return "*** ERROR ***";
    }
}

void
CodeGeneratorModule::run ()
{
  // FIXME: shouldn't have the following global vars at all
  the_parser = &parser;
  the_cgc = new CodeGeneratorC (parser);
  
  string nspace = "Foo";
  vector<string> enum_exports;
  
  /* standard includes */
  if (options.doHeader)
    printf ("\n#include <bse/bsecxxplugin.h>\n", nspace.c_str());
  
  /* sigh, we can't query things by namespace from the parser. // FIXME
   * so here's a gross hack, figure the namespace from the
   * first class to output (cross fingers there is any) and
   * assume the rest went into the same namespace ;-(
   */
  for (vector<Class>::const_iterator ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
    if (!parser.fromInclude (ci->name))
      {
        nspace = ci->name.substr (0, ci->name.find (':'));
        break;
      }
  printf ("\nnamespace %s {\n", nspace.c_str());
  
  if (options.doHeader)
    {
      printf ("\n/* class prototypes */\n");
      for (vector<Class>::const_iterator ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
        {
          if (parser.fromInclude (ci->name))
            continue;
          /* class prototypes */
          printf ("class %sSkel;\n", cTypeName (ci->name));
        }
    }
  
  printf ("\n/* choice/enum types */\n");
  for (vector<Choice>::const_iterator ci = parser.getChoices().begin(); ci != parser.getChoices().end(); ci++)
    {
      if (parser.fromInclude (ci->name))
        continue;
      
      if (options.doSource)
        printf ("extern \"C\" { GType BSE_TYPE_ID (%s) = 0; }\n", cTypeName (ci->name));
      else
        {
          printf ("extern \"C\" { extern GType BSE_TYPE_ID (%s); }\n", cTypeName (ci->name));
          printf ("enum %s {\n", cTypeName (ci->name));
          int i = 1; // FIXME: vi->value needs to be set != 0
          for (vector<ChoiceValue>::const_iterator vi = ci->contents.begin(); vi != ci->contents.end(); vi++)
            printf ("  %s = %d,\n", cUC_NAME (vi->name), i++ /* vi->value */ );
          printf ("};\n");
          printf ("#define %s_TYPE_%s (BSE_TYPE_ID (%s))\n",
                  cUC_NAME (nspace), cUC_NAME (cTypeName (ci->name)),
                  cTypeName (ci->name));
        }
    }
  
  if (options.doSource)
    {
      printf ("\n/* choice/enum type value lists */\n");
      for (vector<Choice>::const_iterator ci = parser.getChoices().begin(); ci != parser.getChoices().end(); ci++)
        {
          if (parser.fromInclude (ci->name))
            continue;
          
          printf ("static const GEnumValue %s_GEnumValues[%u] = {\n", cTypeName (ci->name), ci->contents.size() + 1);
          int i = 1; // FIXME: vi->value needs to be set != 0
          for (vector<ChoiceValue>::const_iterator vi = ci->contents.begin(); vi != ci->contents.end(); vi++)
            printf ("  { %d, \"%s\", \"%s\" },\n", i++ /*vi->value*/, cUC_NAME (vi->name), vi->text.c_str());
          printf ("  { 0, NULL, NULL }\n");
          printf ("};\n");
          enum_exports.push_back (string ("{ ") +
                                  "&BSE_TYPE_ID (" + TypeName (ci->name) + "), " +
                                  "\"" + ci->name + "\", " + // FIXME: get rid of :: in type-name
                                  "G_TYPE_ENUM, " +
                                  TypeName (ci->name) + "_GEnumValues " +
                                  "}");
        }
    }
  
  printf ("\n/* class skeletons */\n");
  for (vector<Class>::const_iterator ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
    {
      string ctName = TypeName (ci->name) + "Skel";
      vector<string> destroy_jobs;
      if (parser.fromInclude (ci->name))
        continue;
      
      /* skeleton class declaration */
      if (options.doHeader)
        printf ("class %s : %s {\n", ctName.c_str(), cTypeName (ci->inherits));
      
      /* properties */
      if (ci->properties.begin() != ci->properties.end())
        {
          /* property enum */
          if (options.doHeader)
            {
              printf ("protected:\n  enum PropertyIDs {\n");
              vector<Param>::const_iterator pi = ci->properties.begin();
              printf ("    PROP_%s = 1,\n", cUC_NAME (pi->name));
              for (pi++; pi != ci->properties.end(); pi++)
                printf ("    PROP_%s,\n", cUC_NAME (pi->name));
              printf ("  };\n");
            }

          /* property fields */
          if (options.doHeader)
            {
              for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
                {
                  /* type */
                  printf ("  %s ", cTypeName (pi->type));
                  /* name */
                  printf ("%s;\n", pi->name.c_str());
                }
            }

          /* property setter */
          if (options.doHeader)
            printf ("  void set_property (unsigned int  prop_id, const GValue *value);\n");
          else
            {
              printf ("void\n%s::set_property (unsigned int  prop_id, const GValue *value)\n", ctName.c_str());
              printf ("{\n");
              printf ("  switch (prop_id) {\n");
              for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
                {
                  printf ("  case PROP_%s:\n", cUC_NAME (pi->name));
                  string f = func_param_free (*pi).c_str();
                  if (f.size())
                    printf ("    %s (%s);\n", f.c_str(), pi->name.c_str());
                  printf ("    %s = %s (value);\n", pi->name.c_str(), func_value_dup_param (*pi).c_str());
                  printf ("  break;\n");
                }
              printf ("  };\n");
              printf ("}\n");
            }

          /* property getter */
          if (options.doHeader)
            printf ("  void get_property (unsigned int  prop_id, GValue *value);\n");
          else
            {
              printf ("void\n%s::get_property (unsigned int  prop_id, GValue *value)\n", ctName.c_str());
              printf ("{\n");
              printf ("  switch (prop_id) {\n");
              for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
                {
                  printf ("  case PROP_%s:\n", cUC_NAME (pi->name));
                  printf ("    %s (value, %s);\n", func_value_set_param (*pi).c_str(), pi->name.c_str());
                  printf ("  break;\n");
                }
              printf ("  };\n");
              printf ("}\n");
            }
          
          /* pspec construction */
          if (options.doHeader)
            printf ("private:\n  GParamSpec* create_pspec (unsigned int  prop_id);\n");
          else
            {
              printf ("GParamSpec*\n%s::create_pspec (unsigned int  prop_id)\n", ctName.c_str());
              printf ("{\n");
              printf ("  switch (prop_id) {\n");
              for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
                {
                  printf ("  case PROP_%s:\n", cUC_NAME (pi->name));
                  printf ("    return %s;\n", pspec_constructor (*pi).c_str());
                }
              printf ("  default:\n");
              printf ("    return NULL;\n");
              printf ("  };\n");
              printf ("}\n");
            }
          
          /* property deletion */
          if (options.doHeader)
            printf ("private:\n  void clear_properties ();\n");
          else
            {
              printf ("void\n%s::clear_properties ()\n", ctName.c_str());
              printf ("{\n");
              for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
                {
                  string f = func_param_free (*pi).c_str();
                  if (f.size())
                    printf ("  %s (%s);\n", f.c_str(), pi->name.c_str());
                }
              printf ("}\n");
              destroy_jobs.push_back ("clear_properties ();");
            }
        }
      
      /* methods */
      if (options.doHeader)
        {
          if (ci->methods.begin() != ci->methods.end())
            printf ("protected:\n");
          for (vector<Method>::const_iterator mi = ci->methods.begin(); mi != ci->methods.end(); mi++)
            {
              /* return type */
              printf ("  virtual %s ", cTypeName (mi->result.type));
              /* method name */
              printf ("%s (", mi->name.c_str());
              /* args */
              for (vector<Param>::const_iterator pi = mi->params.begin(); pi != mi->params.end(); pi++)
                {
                  if (pi != mi->params.begin())
                    printf (", ");
                  printf ("%s %s", cTypeRef (pi->type), pi->name.c_str());
                }
              printf (") = 0;\n");
            }
        }
      
      /* destructor */
      if (options.doHeader)
        printf ("protected:\n  virtual ~%s ();\n", ctName.c_str());
      else
        {
          printf ("%s::~%s ()\n", ctName.c_str(), ctName.c_str());
          printf ("{\n");
          for (vector<string>::const_iterator vi = destroy_jobs.begin(); vi != destroy_jobs.end(); vi++)
            printf ("  %s\n", vi->c_str());
          printf ("}\n");
        }
      
      if (options.doHeader)
        printf ("};\n"); /* finish: class ... { }; */
    }
  
  if (options.doSource)
    {
      printf ("\n/* generate exports */\n");
      if (enum_exports.size ())
        {
          printf ("#include <bse/bseexports.h>\n");
          printf ("BSE_EXPORT_STATIC_ENUMS = {\n");
          for (vector<string>::const_iterator ei = enum_exports.begin(); ei != enum_exports.end(); ei++)
            printf ("  %s,\n", ei->c_str());
          printf ("  { NULL, },\n");
          printf ("};\n");
        }
    }
  
  /* close namespace */
  printf ("\n}; /* %s */\n", nspace.c_str());
}
