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
Qualified (const string &str)
{
  // return fully C++ qualified name
  return str;
}
#define cQualified(s)    Qualified (s).c_str()

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
  vector<string> enum_exports;
  vector<Image> images;
  
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

  /* type list declaration */
  printf ("extern \"C\" ::BseExportNode *BSE_EXPORT_CHAIN_SYMBOL;\n");

  /* class prototypes */
  printf ("\n/* class prototypes */\n");
  for (vector<Class>::const_iterator ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
    {
      if (parser.fromInclude (ci->name))
        continue;
      /* class prototypes */
      printf ("class %s;\n", cTypeName (ci->name));
    }
  
  /* enumerations */
  printf ("\n/* choice/enum types */\n");
  for (vector<Choice>::const_iterator ci = parser.getChoices().begin(); ci != parser.getChoices().end(); ci++)
    {
      if (parser.fromInclude (ci->name))
        continue;

      string enum_type_keeper = TypeName (ci->name) + string ("__TypeKeeper");
      printf ("#define %s_TYPE_%s (%s::get_type())\n",
              cUC_NAME (nspace), cUC_NAME (cTypeName (ci->name)),
              enum_type_keeper.c_str());
      printf ("enum %s {\n", cTypeName (ci->name));
      int i = 1; // FIXME: vi->value needs to be set != 0
      for (vector<ChoiceValue>::const_iterator vi = ci->contents.begin(); vi != ci->contents.end(); vi++)
        printf ("  %s = %d,\n", cUC_NAME (vi->name), i++ /* vi->value */ );
      printf ("};\n");
      printf ("BSE_CXX_ENUM_TYPE_KEEPER (%s, \"%s\" \"%s\", %u,\n",
              cTypeName (ci->name), nspace.c_str(), cTypeName (ci->name), ci->contents.size());
      i = 1;
      for (vector<ChoiceValue>::const_iterator vi = ci->contents.begin(); vi != ci->contents.end(); vi++)
        printf ("                          *v++ = ::Bse::EnumValue (%d, \"%s\", \"%s\" );\n",
                i++ /* vi->value */, cUC_NAME (vi->name), vi->text.c_str());
      printf ("                          );\n");
      enum_exports.push_back (enum_type_keeper + "::enter_type_chain (export_identity)");
    }

  /* class declarations */
  printf ("\n/* classes */\n");
  for (vector<Class>::iterator ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
    {
      string ctName = TypeName (ci->name);
      vector<string> destroy_jobs;
      if (parser.fromInclude (ci->name))
        continue;
      
      /* skeleton class declaration */
      printf ("class %s : public ::%s {\n", ctName.c_str(), cQualified (ci->inherits));

      /* prototype module class for effects */
      if (ci->properties.size() && ci->istreams.size() + ci->jstreams.size() + ci->ostreams.size())
        printf ("  class %sModule;\n", ctName.c_str());

      /* pixstream() prototype */
      string icon = ci->infos["icon"];
      if (icon == "")
        printf ("  static const unsigned char* pixstream() { return NULL; }\n");
      else
        {
          printf ("  static const unsigned char* pixstream();\n");
          images.push_back (Image (icon,
                                   "const unsigned char*\n" +
                                   ctName +
                                   "::pixstream()"));
        }

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

      /* "Parameters" structure for synthesis modules */
      if (ci->properties.size() && ci->istreams.size() + ci->jstreams.size() + ci->ostreams.size())
        {
          if (!is_public++)
            printf ("public:\n");
          printf ("  /* \"transport\" structure to configure synthesis modules from properties */\n");
          printf ("  struct Parameters {\n");
          for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
            printf ("    %s %s;\n", cTypeName (pi->type), pi->name.c_str());
          printf ("    explicit Parameters (%s *p) ", ctName.c_str());
          for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
            printf ("%c\n      %s (p->%s)", pi == ci->properties.begin() ? ':' : ',', pi->name.c_str(), pi->name.c_str());
          printf ("\n    {\n");
          printf ("    }\n");
          printf ("  };\n");
        }
      
      /* property members */
      printf ("protected:\n");
      for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
        printf ("  %s %s;\n", cTypeName (pi->type), pi->name.c_str());
      
      /* property IDs */
      if (ci->properties.begin() != ci->properties.end())
        {
          printf ("private:\n  enum PropertyIDs {\n");
          vector<Param>::const_iterator pi = ci->properties.begin();
          printf ("    PROP_%s = 1,\n", cUC_NAME (pi->name));
          for (pi++; pi != ci->properties.end(); pi++)
            printf ("    PROP_%s,\n", cUC_NAME (pi->name));
          printf ("  };\n");
        }
      
      /* property setter */
      printf ("public:\n");
      printf ("  void set_property (guint prop_id, const Bse::Value &value, GParamSpec *pspec)\n");
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
      printf ("    update_modules();\n");
      printf ("  }\n");
      
      /* property getter */
      printf ("  void get_property (guint prop_id, Bse::Value &value, GParamSpec *pspec)\n");
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

      /* methods */
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
      
      /* effect prototypes */
      printf ("protected:\n");
      printf ("  virtual Bse::Module*           create_module       (unsigned int context_handle, GslTrans *trans);\n");
      printf ("  virtual Bse::Module::Accessor* module_configurator ();\n");

      /* destructor */
      printf ("  virtual ~%s ()\n", ctName.c_str());
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
                si->name.c_str(), si->blurb.c_str(), cUC_NAME (si->ident));
      for (vector<Stream>::const_iterator si = ci->jstreams.begin(); si != ci->jstreams.end(); si++)
        printf ("    klass->add_jchannel (\"%s\", \"%s\", JCHANNEL_%s);\n",
                si->name.c_str(), si->blurb.c_str(), cUC_NAME (si->ident));
      for (vector<Stream>::const_iterator si = ci->ostreams.begin(); si != ci->ostreams.end(); si++)
        printf ("    klass->add_ochannel (\"%s\", \"%s\", OCHANNEL_%s);\n",
                si->name.c_str(), si->blurb.c_str(), cUC_NAME (si->ident));
      printf ("  }\n");

      /* type registration */
      printf ("private:\n");
      printf ("  static ::BseExportNodeClass export_node;\n");
      printf ("  static void enter_type_chain (BseExportIdentity *export_identity)\n");
      printf ("  {\n");
      printf ("    if (!export_node.node.next) {\n");
      printf ("      export_node.node.category = \"%s\";\n", ci->infos["category"].c_str());
      printf ("      export_node.node.blurb = \"%s\";\n", ci->infos["blurb"].c_str());
      printf ("      export_node.node.pixstream = pixstream();\n");
      printf ("      export_node.node.next = export_identity->type_chain;\n");
      printf ("      export_identity->type_chain = &export_node.node;\n");
      printf ("    }\n");
      printf ("  }\n");
      printf ("public:\n");
      printf ("  static GType      get_type();\n");
      printf ("  struct TypeInit {\n");
      printf ("    TypeInit (BseExportIdentity *export_identity)\n");
      printf ("    {\n");
      printf ("      enter_type_chain (export_identity);\n");
      for (vector<string>::const_iterator ei = enum_exports.begin(); ei != enum_exports.end(); ei++)
        printf ("      %s;\n", ei->c_str());
      printf ("    }\n");
      printf ("  };\n");

      printf ("};\n"); /* finish: class ... { }; */
    }

  /* function definitions */
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
