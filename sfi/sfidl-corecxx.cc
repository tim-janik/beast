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
  if (pos < 0)  // not fully qualified, prolly an Sfi type
    return str == "void" ? "void" : "Sfi" + str;
  return str.substr (pos + 1);
}
#define cTypeName(s)    TypeName (s).c_str()

static string
canonify_type (const string s)
{
  /* canonify type names which contain e.g. underscores (procedures) */
  gchar *tmp = g_strcanon (g_strdup (s.c_str()),
                           G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "+",
                           '-');
  string d = tmp;
  g_free (tmp);
  return d;
}

static string
Qualified (const string &str)
{
  int pos = str.rfind (':');
  if (pos < 0)  // not fully qualified, prolly an Sfi type
    return TypeName (str);
  // return fully C++ qualified name
  return str;
}
#define cQualified(s)    Qualified (s).c_str()

static string
TypeRef (const string &type)
{
  string tname = Qualified (type);
  switch (sfidl_type (type))
    {
    case BBLOCK:
    case FBLOCK:
    case PSPEC:
    case SEQUENCE:
    case RECORD:
    case OBJECT:        return tname + "*";
    default:            return tname;
    }
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
pspec_constructor (const Param &param)
{
  switch (sfidl_type (param.type))
    {
    case OBJECT:
      {
        const string group = param.group == "" ? "NULL" : param.group;
        string pspec = "sfidl_pspec_Object";
        if (param.args == "")
          pspec += "_default";
        pspec += " (\"" + group + "\", \"" + param.name + "\", ";
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
    case CHOICE:
      {
        const string group = param.group == "" ? "NULL" : param.group;
        string pspec = "sfidl_pspec_GEnum";
        if (param.args == "")
          pspec += "_default";
        pspec += " (\"" + group + "\", \"" + param.name + "\", ";
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

static string
func_value_get_param (const Param &param,
                      const string dest = "")
{
  switch (sfidl_type (param.type))
    {
    case BOOL:          return "sfi_value_get_bool";
    case INT:           return "sfi_value_get_int";
    case NUM:           return "sfi_value_get_num";
    case REAL:          return "sfi_value_get_real";
    case STRING:        return "sfi_value_get_string";
    case CHOICE:        return "(" + TypeName (param.type) + ") g_value_get_enum";
    case BBLOCK:        return "sfi_value_get_bblock";
    case FBLOCK:        return "sfi_value_get_fblock";
    case PSPEC:         return "sfi_value_get_pspec";
    case SEQUENCE:      return "sfi_value_get_seq";
    case RECORD:        return "sfi_value_get_rec";
    case OBJECT:
      if (dest != "")
        return "::Bse::g_value_get_object<"+ dest +">";
      else
        return "(GObject*) g_value_get_object";
    default:            return "*** ERROR ***";
    }
}

static string
func_value_dup_param (const Param &param)
{
  switch (sfidl_type (param.type))
    {
    case STRING:        return "sfi_value_dup_string";
    case BBLOCK:        return "sfi_value_dup_bblock";
    case FBLOCK:        return "sfi_value_dup_fblock";
    case PSPEC:         return "sfi_value_dup_pspec";
    case SEQUENCE:      return "sfi_value_dup_seq";
    case RECORD:        return "sfi_value_dup_rec";
    case OBJECT:        return "g_value_dup_object";
    default:            return func_value_get_param (param);
    }
}

static string
func_param_return_free (const Param &param)
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
    case OBJECT:        return "";
    default:            return "*** ERROR ***";
    }
}

static string
func_param_free (const Param &param)
{
  switch (sfidl_type (param.type))
    {
    case OBJECT:        return "g_object_unref";
    default:            return func_param_return_free (param);
    }
}

static string
escape (const string& str)
{
  char *x = g_strescape (str.c_str(), 0);
  string result = x;
  g_free (x);
  return result;
}

struct Image {
  string file;
  string method;
  Image (const string &f,
         const string &m)
    : file (f), method (m)
  {}
};

void
CodeGeneratorModule::run ()
{
  // FIXME: shouldn't have the following global vars at all
  the_parser = &parser;
  the_cgc = new CodeGeneratorC (parser);
  
  string nspace = "Foo";
  vector<Image> images;
  std::vector<const Method*> procs;
  
  /* standard includes */
  printf ("\n#include <bse/bsecxxplugin.h>\n");
  
  /* sigh, we can't query things by namespace from the parser. // FIXME
   * so here's a gross hack, figure the namespace from the
   * first class to output (cross fingers there is any) and
   * assume the rest goes into the same namespace ;-(
   */
  for (vector<Class>::const_iterator ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
    if (!parser.fromInclude (ci->name))
      {
        nspace = ci->name.substr (0, ci->name.find (':'));
        break;
      }
  printf ("\nnamespace %s {\n", nspace.c_str());

  /* class prototypes */
  printf ("\n/* class prototypes */\n");
  for (vector<Class>::const_iterator ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
    {
      if (parser.fromInclude (ci->name))
        continue;
      /* class prototypes */
      printf ("class %s%s;\n", cTypeName (ci->name), "Base");
      printf ("class %s;\n", cTypeName (ci->name));
    }
  
  /* enumerations */
  printf ("\n/* choice/enum types */\n");
  for (vector<Choice>::const_iterator ci = parser.getChoices().begin(); ci != parser.getChoices().end(); ci++)
    {
      if (parser.fromInclude (ci->name))
        continue;

      printf ("#define %s_TYPE_%s (BSE_CXX_DECLARED_ENUM_TYPE (%s))\n",
              cUC_NAME (nspace), cUC_NAME (cTypeName (ci->name)), TypeName (ci->name).c_str());
      printf ("enum %s {\n", cTypeName (ci->name));
      for (vector<ChoiceValue>::const_iterator vi = ci->contents.begin(); vi != ci->contents.end(); vi++)
        printf ("  %s = %d,\n", cUC_NAME (vi->name), vi->sequentialValue );
      printf ("};\n");
      printf ("BSE_CXX_DECLARE_ENUM (%s, \"%s\" \"%s\", %u,\n",
              cTypeName (ci->name), nspace.c_str(), cTypeName (ci->name), ci->contents.size());
      for (vector<ChoiceValue>::const_iterator vi = ci->contents.begin(); vi != ci->contents.end(); vi++)
        printf ("                      *v++ = ::Bse::EnumValue (%d, \"%s\", \"%s\" );\n",
                vi->sequentialValue, cUC_NAME (vi->name), vi->text.c_str());
      printf ("                      );\n");
    }

  printf ("\n}; /* namespace %s */\n", nspace.c_str()); /* XXX */

  /* records and sequences */

  /*
   * FIXME: we follow the declaration order of the idl file for generating records and sequences.
   * This is quite good, as if no prototypes are used, we won't refer to undefined types.
   * However, this breaks with prototypes.
   */
  NamespaceHelper nsh(stdout);
  printf ("\n/* record/sequence types */\n");
  bool first = true;
  for(vector<string>::const_iterator ti = parser.getTypes().begin(); ti != parser.getTypes().end(); ti++)
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

	  nsh.setFromSymbol(rdef.name);
	  string name = nsh.printableForm (rdef.name);

	  printf("class %s {\n", name.c_str());

	  for (vector<Param>::const_iterator pi = rdef.contents.begin(); pi != rdef.contents.end(); pi++)
	    {
	      string name = the_cgc->createTypeCode(pi->type,pi->name,MODEL_MEMBER);
	      printf("\t%s;\n",name.c_str());
	    }
	  printf("};");
	}
      if (parser.isSequence (*ti))
	{
	  const Sequence& sdef = parser.findSequence (*ti);

	  printf("//%s\n", sdef.name.c_str());
#if 0
	  string name = makeLowerName (sdef.name);
	  // int f = 0;

	  if (options.generateIdlLineNumbers)
	    printf("#line %u \"%s\"\n", sdef.content.line, parser.fileName().c_str());
	  printf("  %s_content = %s;\n", name.c_str(), makeParamSpec (sdef.content).c_str());
#endif
	}
    }

  nsh.leaveAll();

  printf ("\nnamespace %s {\n", nspace.c_str()); /* XXX */

  /* class definitions */
  printf ("\n/* classes */\n");
  for (vector<Class>::const_iterator ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
    {
      string ctName = TypeName (ci->name);
      string ctFullName = nspace + ctName;
      string ctNameBase = TypeName (ci->name) + "Base";
      string ctProperties = TypeName (ci->name) + "Properties";
      string ctPropertyID = TypeName (ci->name) + "PropertyID";
      vector<string> destroy_jobs;
      if (parser.fromInclude (ci->name))
        continue;
      
      /* skeleton class declaration + type macro */
      printf ("class %s : public ::%s {\n", ctNameBase.c_str(), cQualified (ci->inherits));
      printf ("#define %s_TYPE_%s (BSE_CXX_DECLARED_CLASS_TYPE (%s))\n",
              cUC_NAME (nspace), cUC_NAME (cTypeName (ci->name)), TypeName (ci->name).c_str());
      
      /* class Info strings */
      /* pixstream(), this is a bit of a hack, we make it a template rather than
       * a normal inline method to avoid huge images in debugging code
       */
      string icon = ci->infos.get("icon");
      string pstream = "NULL";
      if (icon != "")
        {
          printf ("  template<bool> static inline const unsigned char* pixstream();\n");
          images.push_back (Image (include_relative (icon, ci->file),
                                   "template<bool> const unsigned char*\n" +
                                   ctNameBase +
                                   "::pixstream()"));
          pstream = "pixstream<true>()";
        }
      printf ("public:\n");
      printf ("  static inline const unsigned char* pixstream () { return %s; }\n", pstream.c_str());
      printf ("  static inline const char* category  () { return \"%s\"; }\n", escape(ci->infos.get("category")).c_str());
      printf ("  static inline const char* blurb     () { return \"%s\"; }\n", escape(ci->infos.get("blurb")).c_str());
      printf ("  static inline const char* type_name () { return \"%s\"; }\n", ctFullName.c_str());
      
      /* i/j/o channel names */
      int is_public = 0;
      if (ci->istreams.size())
        {
          if (!is_public++)
            printf ("public:\n");
          printf ("  enum {\n");
          for (vector<Stream>::const_iterator si = ci->istreams.begin(); si != ci->istreams.end(); si++)
            printf ("    ICHANNEL_%s,\n", cUC_NAME (si->ident));
          printf ("    N_ICHANNELS\n  };\n");
        }
      if (ci->jstreams.size())
        {
          if (!is_public++)
            printf ("public:\n");
          printf ("  enum {\n");
          for (vector<Stream>::const_iterator si = ci->jstreams.begin(); si != ci->jstreams.end(); si++)
            printf ("    JCHANNEL_%s,\n", cUC_NAME (si->ident));
          printf ("    N_JCHANNELS\n  };\n");
        }
      if (ci->ostreams.size())
        {
          if (!is_public++)
            printf ("public:\n");
          printf ("  enum {\n");
          for (vector<Stream>::const_iterator si = ci->ostreams.begin(); si != ci->ostreams.end(); si++)
            printf ("    OCHANNEL_%s,\n", cUC_NAME (si->ident));
          printf ("    N_OCHANNELS\n  };\n");
        }

      /* "Properties" structure for synthesis modules */
      if (ci->properties.size() && ci->istreams.size() + ci->jstreams.size() + ci->ostreams.size())
        {
          if (!is_public++)
            printf ("public:\n");
          printf ("  /* \"transport\" structure to configure synthesis modules from properties */\n");
          printf ("  struct %s {\n", ctProperties.c_str());
          for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
            printf ("    %s %s;\n", cTypeName (pi->type), pi->name.c_str());
          printf ("    explicit %s (%s *p) ", ctProperties.c_str(), ctNameBase.c_str());
          for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
            printf ("%c\n      %s (p->%s)", pi == ci->properties.begin() ? ':' : ',', pi->name.c_str(), pi->name.c_str());
          printf ("\n    {\n");
          printf ("    }\n");
          printf ("  };\n");
        }
      
      /* property members */
      printf ("protected:\n");
      for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
        printf ("  %s %s;\n", cTypeRef (pi->type), pi->name.c_str());
      
      /* property IDs */
      if (ci->properties.begin() != ci->properties.end())
        {
          printf ("protected:\n  enum %s {\n", ctPropertyID.c_str());
          vector<Param>::const_iterator pi = ci->properties.begin();
          printf ("    PROP_%s = 1,\n", cUC_NAME (pi->name));
          for (pi++; pi != ci->properties.end(); pi++)
            printf ("    PROP_%s,\n", cUC_NAME (pi->name));
          printf ("  };\n");
        }
      
      /* property setter */
      printf ("public:\n");
      printf ("  void set_property (guint prop_id, const ::Bse::Value &value, GParamSpec *pspec)\n");
      printf ("  {\n");
      printf ("    switch (prop_id) {\n");
      for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
        {
          printf ("    case PROP_%s:\n", cUC_NAME (pi->name));
          string f = func_param_free (*pi).c_str();
          if (f.size())
            printf ("      %s (%s);\n", f.c_str(), pi->name.c_str());
          printf ("      %s = %s (&value);\n", pi->name.c_str(), func_value_dup_param (*pi).c_str());
          printf ("    break;\n");
        }
      printf ("    };\n");
      printf ("    property_changed ((%s) prop_id);\n", ctPropertyID.c_str());
      printf ("    update_modules();\n");
      /* reset triggers */
      printf ("    switch (prop_id) {\n");
      for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
        {
          if (pi->pspec != "Trigger")
            continue;
          printf ("    case PROP_%s:\n", cUC_NAME (pi->name));
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
          printf ("    case PROP_%s:\n", cUC_NAME (pi->name));
          printf ("      %s (&value, %s);\n", func_value_set_param (*pi).c_str(), pi->name.c_str());
          printf ("    break;\n");
        }
      printf ("    };\n");
      printf ("  }\n");

      /* static data */
      printf ("private:\n");
      printf ("  static struct StaticData {\n");
      printf ("    int dummy;\n");
      printf ("  } static_data;\n");

      /* property-changed hooking */
      printf ("protected:\n");
      printf ("  virtual void property_changed (%s) {}\n", ctPropertyID.c_str());

      /* methods */
      for (vector<Method>::const_iterator mi = ci->methods.begin(); mi != ci->methods.end(); mi++)
        procs.push_back (&(*mi));
      
      /* destructor */
      printf ("  virtual ~%s ()\n", ctNameBase.c_str());
      printf ("  {\n");
      /* property deletion */
      for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
        {
          string f = func_param_free (*pi).c_str();
          if (f.size())
            printf ("    %s (%s);\n", f.c_str(), pi->name.c_str());
        }
      for (vector<string>::const_iterator vi = destroy_jobs.begin(); vi != destroy_jobs.end(); vi++)
        printf ("    %s;\n", vi->c_str());
      printf ("  }\n");

      /* class_init */
      printf ("public:\n");
      printf ("  static void class_init (::Bse::CxxBaseClass *klass)\n");
      printf ("  {\n");
      for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
        printf ("    klass->add (PROP_%s, %s);\n", cUC_NAME (pi->name), pspec_constructor (*pi).c_str());
      for (vector<Stream>::const_iterator si = ci->istreams.begin(); si != ci->istreams.end(); si++)
        printf ("    klass->add_ichannel (\"%s\", \"%s\", ICHANNEL_%s);\n",
                escape(si->name).c_str(), escape(si->blurb).c_str(), cUC_NAME (si->ident));
      for (vector<Stream>::const_iterator si = ci->jstreams.begin(); si != ci->jstreams.end(); si++)
        printf ("    klass->add_jchannel (\"%s\", \"%s\", JCHANNEL_%s);\n",
                escape(si->name).c_str(), escape(si->blurb).c_str(), cUC_NAME (si->ident));
      for (vector<Stream>::const_iterator si = ci->ostreams.begin(); si != ci->ostreams.end(); si++)
        printf ("    klass->add_ochannel (\"%s\", \"%s\", OCHANNEL_%s);\n",
                escape(si->name).c_str(), escape(si->blurb).c_str(), cUC_NAME (si->ident));
      printf ("  }\n");

      /* done */
      printf ("};\n"); /* finish: class ... { }; */
      printf ("BSE_CXX_DECLARE_CLASS (%s);\n", cTypeName (ci->name));
    }
  printf ("\n");
  
  /* collect procedures */
  for (vector<Method>::const_iterator mi = parser.getProcedures().begin(); mi != parser.getProcedures().end(); mi++)
    procs.push_back (&(*mi));

  /* generate procedures */
  for (vector<const Method*>::const_iterator ppi = procs.begin(); ppi != procs.end(); ppi++)
    {
      const Method *mi = *ppi;  // FIXME: things containing maps shouldn't be constant
      const Map<std::string, std::string> &infos = mi->infos;
      string ptName = string ("Procedure_") + TypeName (mi->name);
      string ptFullName = canonify_type (nspace + ptName);
      bool is_void = mi->result.type == "void";
      if (parser.fromInclude (mi->name))
        continue;
      
      /* skeleton class declaration + type macro */
      printf ("class %s {\n", ptName.c_str());
      printf ("#define %s_TYPE_%s (BSE_CXX_DECLARED_PROC_TYPE (%s))\n",
              cUC_NAME (nspace), cUC_NAME (cTypeName (mi->name)), ptName.c_str());

      /* class Info strings */
      /* pixstream(), this is a bit of a hack, we make it a template rather than
       * a normal inline method to avoid huge images in debugging code
       */
      string icon = infos.get("icon");
      string pstream = "NULL";
      if (icon != "")
        {
          printf ("  template<bool> static inline const unsigned char* pixstream();\n");
          images.push_back (Image (include_relative (icon, mi->file),
                                   "template<bool> const unsigned char*\n" +
                                   ptName +
                                   "::pixstream()"));
          pstream = "pixstream<true>()";
        }
      printf ("public:\n");
      printf ("  static inline const unsigned char* pixstream () { return %s; }\n", pstream.c_str());
      printf ("  static inline const char* category  () { return \"%s\"; }\n", escape(infos.get("category")).c_str());
      printf ("  static inline const char* blurb     () { return \"%s\"; }\n", escape(infos.get("blurb")).c_str());
      printf ("  static inline const char* type_name () { return \"%s\"; }\n", ptFullName.c_str());
      
      /* return type */
      printf ("  static %s exec (", cTypeRef (mi->result.type));
      /* args */
      for (vector<Param>::const_iterator ai = mi->params.begin(); ai != mi->params.end(); ai++)
        {
          if (ai != mi->params.begin())
            printf (", ");
          printf ("%s %s", cTypeRef (ai->type), ai->name.c_str());
        }
      printf (");\n");
      
      /* marshal */
      printf ("  static BseErrorType marshal (BseProcedureClass *procedure,\n"
              "                               const GValue      *in_values,\n"
              "                               GValue            *out_values)\n");
      printf ("  {\n");
      printf ("    try {\n");
      if (!is_void)
        printf ("      %s __return_value =\n", cTypeRef (mi->result.type));
      printf ("        exec (\n");
      int i = 0;
      for (vector<Param>::const_iterator pi = mi->params.begin(); pi != mi->params.end(); pi++)
        printf ("              %s (in_values + %u)%c\n", func_value_get_param (*pi, cTypeRef (pi->type)).c_str(), i++,
                &(*pi) == &(mi->params.back()) ? ' ' : ',');
      printf ("             );\n");
      if (!is_void)
        printf ("      %s (out_values, __return_value);\n", func_value_set_param (mi->result).c_str());
      if (!is_void && func_param_return_free (mi->result) != "")
        printf ("      if (__return_value) %s (__return_value);\n", func_param_return_free (mi->result).c_str());
      printf ("    } catch (std::exception &e) {\n");
      printf ("      sfi_debug (\"%%s: %%s\", \"%s\", e.what());\n", ptName.c_str());
      printf ("      return BSE_ERROR_PROC_EXECUTION;\n");
      printf ("    } catch (...) {\n");
      printf ("      sfi_debug (\"%%s: %%s\", \"%s\", \"unknown exception\");\n", ptName.c_str());
      printf ("      return BSE_ERROR_PROC_EXECUTION;\n");
      printf ("    }\n");
      printf ("    return BSE_ERROR_NONE;\n");
      printf ("  }\n");

      /* init */
      printf ("  static void init (BseProcedureClass *proc,\n"
              "                    GParamSpec       **in_pspecs,\n"
              "                    GParamSpec       **out_pspecs)");
      printf ("  {\n");
      printf ("    proc->help = \"%s\";\n", escape(infos.get("help")).c_str());
      printf ("    proc->authors = \"%s\";\n", escape(infos.get("authors")).c_str());
      printf ("    proc->license = \"%s\";\n", escape(infos.get("license")).c_str());
      for (vector<Param>::const_iterator ai = mi->params.begin(); ai != mi->params.end(); ai++)
        printf ("    *(in_pspecs++) = %s;\n", pspec_constructor (*ai).c_str());
          if (!is_void)
        printf ("    *(out_pspecs++) = %s;\n", pspec_constructor (mi->result).c_str());
      printf ("  }\n");
      
      /* done */
      printf ("};\n"); /* finish: class ... { }; */
      printf ("BSE_CXX_DECLARE_PROC (%s);\n", ptName.c_str());
    }
  printf ("\n");
  
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
  
  /* close namespace */
  printf ("\n}; /* %s */\n", nspace.c_str());
}
