/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002-2003 Stefan Westerfeld
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
#include "sfidl-cxx.h"
#include "sfidl-factory.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include "sfidl-namespace.h"
#include "sfidl-options.h"
#include "sfidl-parser.h"
#include "sfidl-module.h"
#include "sfiparams.h" /* scatId (SFI_SCAT_*) */

using namespace Sfidl;
using namespace std;

static string fail (const string& error)
{
  g_error ("%s\n", error.c_str());
  return "*fail(" + error + ")*";
}

string
CodeGeneratorCxxBase::typeArg (const string& type)
{
  switch (parser.typeOf (type))
    {
      case STRING:    return "const Sfi::String&";
      case RECORD:    return "const " + type + "Handle&";
      case SEQUENCE:  return "const " + type + "&";
      case CHOICE:    return type;
      case OBJECT:    return type;
      default:	      return CodeGeneratorCBase::typeArg (type);
    }
}

string
CodeGeneratorCxxBase::typeField (const string& type)
{
  switch (parser.typeOf (type))
    {
      case STRING:    return "Sfi::String";
      case RECORD:    return type + "Handle";
      case CHOICE:
      case OBJECT:
      case SEQUENCE:  return type;
      default:	      return CodeGeneratorCBase::typeArg (type);
    }
}

string
CodeGeneratorCxxBase::typeRet (const string& type)
{
  switch (parser.typeOf (type))
    {
      case STRING:    return "Sfi::String";
      case RECORD:    return type + "Handle";
      case CHOICE:
      case OBJECT:
      case SEQUENCE:  return type;
      default:	      return CodeGeneratorCBase::typeArg (type);
    }
}

string
CodeGeneratorCxxBase::funcNew (const string& type)
{
  switch (parser.typeOf (type))
    {
      case STRING:    return "";
      default:	      return CodeGeneratorCBase::funcNew (type);
    }
}

string
CodeGeneratorCxxBase::funcCopy (const string& type)
{
  switch (parser.typeOf (type))
    {
      case STRING:    return "";
      default:	      return CodeGeneratorCBase::funcCopy (type);
    }
}

string
CodeGeneratorCxxBase::funcFree (const string& type)
{
  switch (parser.typeOf (type))
    {
      case STRING:    return "";
      default:	      return CodeGeneratorCBase::funcFree (type);
    }
}

string CodeGeneratorCxxBase::createTypeCode (const std::string& type, const std::string& name, 
				             TypeCodeModel model)
{
  /* FIXME: parameter validation */
  switch (parser.typeOf (type))
    {
      case STRING:
	switch (model)
	  {
	    case MODEL_TO_VALUE:    return "sfi_value_string ("+name+".c_str())";
	    case MODEL_FROM_VALUE:  return "::Sfi::String::value_get_string ("+name+")";
	    case MODEL_VCALL:       return "sfi_glue_vcall_string";
	    case MODEL_VCALL_ARG:   return "'" + scatId (SFI_SCAT_STRING) + "', "+name+".c_str(),";
	    case MODEL_VCALL_CARG:  return "";
	    case MODEL_VCALL_CONV:  return "";
	    case MODEL_VCALL_CFREE: return "";
	    case MODEL_VCALL_RET:   return "std::string";
	    case MODEL_VCALL_RCONV: return name;
	    case MODEL_VCALL_RFREE: return "";
	  }
	  break;
      case RECORD:
	switch (model)
	  {
	    case MODEL_TO_VALUE:    return "sfi_value_new_take_rec ("+type+"::to_rec ("+name+"))";
	    case MODEL_FROM_VALUE:  return type + "::from_rec (sfi_value_get_rec ("+name+"))";
	    case MODEL_VCALL_CONV:  return type + "::to_rec ("+name+")";
	    case MODEL_VCALL_RCONV: return type + "::from_rec ("+name+")";
	    case MODEL_VCALL_RFREE: return "";
	    default: ;
	  }
	  break;
      case SEQUENCE:
	switch (model)
	  {
	    case MODEL_TO_VALUE:    return "sfi_value_new_take_seq ("+type+"::to_seq ("+name+"))";
	    case MODEL_FROM_VALUE:  return type + "::from_seq (sfi_value_get_seq ("+name+"))";
	    case MODEL_VCALL_CONV:  return type + "::to_seq ("+name+")";
	    case MODEL_VCALL_RCONV: return type + "::from_seq ("+name+")";
	    case MODEL_VCALL_RFREE: return "";
	    default: ;
	  }
	break;
      case OBJECT:
	switch (model)
	  {
	    case MODEL_VCALL_ARG:
	      {
		/* this is a bit hacky */
		if (name == "_object_id")
		  return "'" + scatId (SFI_SCAT_PROXY) + "', "+name+",";
		else
		  return "'" + scatId (SFI_SCAT_PROXY) + "', "+name+"._proxy(),";
	      }
	    default: ;
	  }
      default: ;
    }
  return CodeGeneratorCBase::createTypeCode (type, name, model);
}

static string
CxxNameToSymbol (const string &str)     // FIXME: need mammut renaming function
{
  static const char *cset = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyz";
  string s;
  for (guint i = 0; i < s.size(); i++)
    if (!strchr (cset, s[i]))
      s[i] = '_';
  return s;
}

static vector<string>
split_string (const string &ctype)      // FIXME: remove once we have general renamer
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
join_string (const vector<string> &vs,  // FIXME: remove once we have general renamer
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
UC_NAME (const string &cstr)    // FIXME: need mammut renaming function
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
static const char*
cUC_NAME (const string &cstr) // FIXME: need mammut renaming function
{
  return g_intern_string (cstr.c_str());
}

static string // FIXME: need mammut renaming function
UC_TYPE_NAME (const string &tname)
{
  vector<string> vs = split_string (tname);
  string lname = vs.back();
  vs.pop_back();
  string nspace = join_string (vs, ":");
  string result = UC_NAME (nspace) + "_TYPE_" + UC_NAME (lname);
  return result;
}

static const char*
cUC_TYPE_NAME (const string &cstr) // FIXME: need mammut renaming function
{
  return g_intern_string (UC_TYPE_NAME (cstr).c_str());
}

/* produce type-system-independant pspec constructors */
std::string
CodeGeneratorCxxBase::untyped_pspec_constructor (const Param &param)
{
  switch (parser.typeOf (param.type))
    {
    case CHOICE:
      {
        const string group = (param.group != "") ? param.group.escaped() : "NULL";
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
    case RECORD:
      {
        const string group = (param.group != "") ? param.group.escaped() : "NULL";
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
        const string group = (param.group != "") ? param.group.escaped() : "NULL";
        string pspec = "sfidl_pspec_Sequence";
        if (param.args == "")
          pspec += "_default (" + group + ", \"" + param.name + "\", ";
        else
          pspec += " (" + group + ", \"" + param.name + "\", " + param.args + ", ";
        pspec += param.type + "::get_element()";
        pspec += ")";
        return pspec;
      }
    default:    return makeParamSpec (param);
    }
}

/* produce type-system-dependant pspec constructors */
std::string
CodeGeneratorCxxBase::typed_pspec_constructor (const Param &param)
{
  switch (parser.typeOf (param.type))
    {
    case CHOICE:
      {
        const string group = (param.group != "") ? param.group.escaped() : "NULL";
        string pspec = "sfidl_pspec_GEnum";
        if (param.args == "")
          pspec += "_default";
        pspec += " (" + group + ", \"" + param.name + "\", ";
        if (param.args != "")
          pspec += param.args + ", ";
        pspec += cUC_TYPE_NAME (param.type);
        pspec += ")";
        return pspec;
      }
    case RECORD:
      {
        const string group = (param.group != "") ? param.group.escaped() : "NULL";
        string pspec = "sfidl_pspec_BoxedRec";
        if (param.args == "")
          pspec += "_default (" + group + ", \"" + param.name + "\", ";
        else
          pspec += " (" + group + ", \"" + param.name + "\", " + param.args + ", ";
        pspec += cUC_TYPE_NAME (param.type);
        pspec += ")";
        return pspec;
      }
    case SEQUENCE:
      {
        const string group = (param.group != "") ? param.group.escaped() : "NULL";
        string pspec = "sfidl_pspec_BoxedSeq";
        if (param.args == "")
          pspec += "_default (" + group + ", \"" + param.name + "\", ";
        else
          pspec += " (" + group + ", \"" + param.name + "\", " + param.args + ", ";
        pspec += cUC_TYPE_NAME (param.type);
        pspec += ")";
        return pspec;
      }
    case OBJECT:
      {
        const string group = (param.group != "") ? param.group.escaped() : "NULL";
        string pspec = "sfidl_pspec_Object";
        if (param.args == "")
          pspec += "_default";
        pspec += " (" + group + ", \"" + param.name + "\", ";
        if (param.args != "")
          pspec += param.args + ", ";
        pspec += cUC_TYPE_NAME (param.type);
        pspec += ")";
        return pspec;
      }
    default:    return makeParamSpec (param);
    }
}

void
CodeGeneratorCxxBase::printChoicePrototype (NamespaceHelper& nspace)
{
  printf ("\n/* choice prototypes */\n");
  for (vector<Choice>::const_iterator ci = parser.getChoices().begin(); ci != parser.getChoices().end(); ci++)
    {
      if (parser.fromInclude (ci->name))
        continue;
      nspace.setFromSymbol(ci->name);
      string name = nspace.printableForm (ci->name);
      printf ("\n");
      printf ("static inline SfiChoiceValues %s_choice_values();\n", name.c_str());
    }
}

void
CodeGeneratorCxxBase::printChoiceImpl (NamespaceHelper& nspace)
{
  printf ("\n/* choice implementations */\n");
  for (vector<Choice>::const_iterator ci = parser.getChoices().begin(); ci != parser.getChoices().end(); ci++)
    {
      if (parser.fromInclude (ci->name))
        continue;
      nspace.setFromSymbol(ci->name);
      string name = nspace.printableForm (ci->name);
      printf ("\n");
      printf ("static inline SfiChoiceValues\n");
      printf ("%s_choice_values()\n", name.c_str());
      printf ("{\n");
      printf ("  static const SfiChoiceValue values[%u] = {\n", ci->contents.size());
      for (vector<ChoiceValue>::const_iterator vi = ci->contents.begin(); vi != ci->contents.end(); vi++)
        printf ("    { \"%s\", \"%s\" },\n", cUC_NAME (vi->name), vi->text.c_str());
      printf ("  };\n");
      printf ("  static const SfiChoiceValues choice_values = {\n");
      printf ("    sizeof (values), values,\n");
      printf ("  };\n");
      printf ("  return choice_values;\n");
      printf ("}\n\n");
    }
}

void
CodeGeneratorCxxBase::printRecSeqForwardDecl (NamespaceHelper& nspace)
{
  vector<Sequence>::const_iterator si;
  vector<Record>::const_iterator ri;

  printf ("\n/* record/sequence prototypes */\n");

  /* forward declarations for records */
  for (ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
    {
      if (parser.fromInclude (ri->name))
        continue;

      nspace.setFromSymbol(ri->name);
      string name = nspace.printableForm (ri->name);

      printf("\n");
      printf("class %s;\n", name.c_str());
      printf("typedef Sfi::RecordHandle<%s> %sHandle;\n", name.c_str(), name.c_str());
    }

  /* forward declarations for sequences */
  for (si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
    {
      if (parser.fromInclude (si->name)) continue;

      nspace.setFromSymbol(si->name);
      string name = nspace.printableForm (si->name);

      printf("\n");
      printf("class %s;\n", name.c_str());
    }
}

void CodeGeneratorCxxBase::printRecSeqDefinition (NamespaceHelper& nspace)
{
  vector<Param>::const_iterator pi;

  printf ("\n/* record/sequence definitions */\n");

  /* sequences */
  for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
    {
      if (parser.fromInclude (si->name)) continue;

      nspace.setFromSymbol(si->name);

      /* FIXME: need optimized refcounted copy-on-write sequences as base types */

      string name = nspace.printableForm (si->name);
      string content = typeField (si->content.type);
      
      printf ("\n");
      if (options.doImplementation)
        {
          printf ("#define %s BSE_CXX_DECLARED_SEQUENCE_TYPE (%s)\n", makeGTypeName (si->name).c_str(), name.c_str());
        }
      printf ("class %s : public Sfi::Sequence<%s> {\n", name.c_str(), content.c_str());
      printf ("public:\n");
      printf ("  static inline %s from_seq (SfiSeq *seq);\n", cTypeRet (si->name));
      printf ("  static inline SfiSeq *to_seq (%s seq);\n", cTypeArg (si->name));
      printf ("  static inline %s value_get_seq (const GValue *value);\n", cTypeRet (si->name));
      printf ("  static inline void value_set_seq (GValue *value, %s self);\n", cTypeArg (si->name));
      printf ("  static inline const char* options   () { return %s; }\n", si->infos.get("options").escaped().c_str());
      printf ("  static inline const char* blurb     () { return %s; }\n", si->infos.get("blurb").escaped().c_str());
      printf ("  static inline const char* authors   () { return %s; }\n", si->infos.get("authors").escaped().c_str());
      printf ("  static inline const char* license   () { return %s; }\n", si->infos.get("license").escaped().c_str());
      printf ("  static inline const char* type_name () { return \"%s\"; }\n", makeMixedName (si->name).c_str());
      if (options.doImplementation)
        printf ("  static inline GParamSpec* get_element ();\n");
      printf("};\n");
      printf ("\n");
    }

  /* records */
  for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
    {
      if (parser.fromInclude (ri->name)) continue;

      nspace.setFromSymbol(ri->name);
      string name = nspace.printableForm (ri->name);
      string type_name = makeMixedName (ri->name).c_str();

      printf ("\n");
      if (options.doImplementation)
	{
	  printf ("#define %s BSE_CXX_DECLARED_RECORD_TYPE (%s)\n", makeGTypeName (ri->name).c_str(), name.c_str());
	}
      printf ("class %s : public ::Sfi::GNewable {\n", name.c_str());
      printf ("public:\n");
      for (pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
	{
	  printf ("  %s %s;\n", cTypeField (pi->type), pi->name.c_str());
	}
      printf ("  static inline %s from_rec (SfiRec *rec);\n", cTypeRet(ri->name));
      printf ("  static inline SfiRec *to_rec (%s ptr);\n", cTypeArg(ri->name));
      printf ("  static inline %s value_get_rec (const GValue *value);\n", cTypeRet(ri->name));
      printf ("  static inline void value_set_rec (GValue *value, %s self);\n", cTypeArg (ri->name));
      printf ("  static inline const char* options   () { return %s; }\n", ri->infos.get("options").escaped().c_str());
      printf ("  static inline const char* blurb     () { return %s; }\n", ri->infos.get("blurb").escaped().c_str());
      printf ("  static inline const char* authors   () { return %s; }\n", ri->infos.get("authors").escaped().c_str());
      printf ("  static inline const char* license   () { return %s; }\n", ri->infos.get("license").escaped().c_str());
      printf ("  static inline const char* type_name () { return \"%s\"; }\n", type_name.c_str());
      if (options.doImplementation)
        printf ("  static inline SfiRecFields get_fields ();\n");
      printf ("};\n");
      printf ("\n");
    }
}

void CodeGeneratorCxxBase::printRecSeqImpl (NamespaceHelper& nspace)
{
  printf ("\n/* record/sequence implementations */\n");

  /* sequence members */
  for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
    {
      if (parser.fromInclude (si->name))
        continue;
      nspace.setFromSymbol(si->name);
      string name = nspace.printableForm (si->name);
      string nname = si->name;
      string type_name = makeMixedName (si->name).c_str();

      if (options.doImplementation)
        printf ("BSE_CXX_DECLARE_SEQUENCE (%s);\n", name.c_str());
      string elementFromValue = createTypeCode (si->content.type, "element", MODEL_FROM_VALUE);
      printf("%s\n", cTypeRet (si->name));
      printf("%s::from_seq (SfiSeq *sfi_seq)\n", nname.c_str());
      printf("{\n");
      printf("  %s seq;\n", cTypeRet (si->name));
      printf("  guint i, length;\n");
      printf("\n");
      printf("  g_return_val_if_fail (sfi_seq != NULL, seq);\n");
      printf("\n");
      printf("  length = sfi_seq_length (sfi_seq);\n");
      printf("  seq.resize (length);\n");
      printf("  for (i = 0; i < length; i++)\n");
      printf("  {\n");
      printf("    GValue *element = sfi_seq_get (sfi_seq, i);\n");
      printf("    seq[i] = %s;\n", elementFromValue.c_str());
      printf("  }\n");
      printf("  return seq;\n");
      printf("}\n\n");

      string elementToValue = createTypeCode (si->content.type, "seq[i]", MODEL_TO_VALUE);
      printf("SfiSeq *\n");
      printf("%s::to_seq (%s seq)\n", nname.c_str(), cTypeArg (si->name));
      printf("{\n");
      printf("  SfiSeq *sfi_seq = sfi_seq_new ();\n");
      printf("  for (guint i = 0; i < seq.length(); i++)\n");
      printf("  {\n");
      printf("    GValue *element = %s;\n", elementToValue.c_str());
      printf("    sfi_seq_append (sfi_seq, element);\n");
      printf("    sfi_value_free (element);\n");        // FIXME: couldn't we have take_append
      printf("  }\n");
      printf("  return sfi_seq;\n");
      printf("}\n\n");

      printf ("%s\n", cTypeRet (si->name));
      printf ("%s::value_get_seq (const GValue *value)\n", nname.c_str());
      printf ("{\n");
      printf ("  return ::Sfi::cxx_value_get_seq< %s> (value);\n", nname.c_str());
      printf ("}\n\n");
      printf ("void\n");
      printf ("%s::value_set_seq (GValue *value, %s self)\n", nname.c_str(), cTypeArg (si->name));
      printf ("{\n");
      printf ("  ::Sfi::cxx_value_set_seq< %s> (value, self);\n", nname.c_str());
      printf ("}\n\n");

      if (options.doImplementation)
	{
	  printf ("GParamSpec*\n");
	  printf ("%s::get_element()\n", nname.c_str());
	  printf ("{\n");
	  printf ("  static GParamSpec *element = NULL;\n");
	  printf ("  if (!element)\n");
          // printf("#line %u \"%s\"\n", si->content.line, parser.fileName().c_str());
          printf ("    element = %s;\n", untyped_pspec_constructor (si->content).c_str());
	  printf ("  return element;\n");
	  printf ("}\n\n");
	}
    }

  /* record members */
  for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
    {
      if (parser.fromInclude (ri->name))
        continue;
      nspace.setFromSymbol(ri->name);
      string name = nspace.printableForm (ri->name);
      string nname = ri->name;
      string type_name = makeMixedName (ri->name).c_str();
      
      if (options.doImplementation)
        printf ("BSE_CXX_DECLARE_RECORD (%s);\n", name.c_str());
      printf("%s\n", cTypeRet (ri->name));
      printf("%s::from_rec (SfiRec *sfi_rec)\n", nname.c_str());
      printf("{\n");
      printf("  GValue *element;\n");
      printf("\n");
      printf("  if (!sfi_rec)\n");
      printf("    return Sfi::INIT_NULL;\n");
      printf("\n");
      printf("  %s rec = Sfi::INIT_DEFAULT;\n", cTypeField (ri->name));
      for (vector<Param>::const_iterator pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
	{
	  string elementFromValue = createTypeCode (pi->type, "element", MODEL_FROM_VALUE);

	  printf("  element = sfi_rec_get (sfi_rec, \"%s\");\n", pi->name.c_str());
	  printf("  if (element)\n");
	  printf("    rec->%s = %s;\n", pi->name.c_str(), elementFromValue.c_str());
	}
      printf("  return rec;\n");
      printf("}\n\n");

      printf("SfiRec *\n");
      printf("%s::to_rec (%s rec)\n", nname.c_str(), cTypeArg (ri->name));
      printf("{\n");
      printf("  SfiRec *sfi_rec;\n");
      printf("  GValue *element;\n");
      printf("\n");
      printf("  if (!rec)\n");
      printf("    return NULL;\n");
      printf("\n");
      printf("  sfi_rec = sfi_rec_new ();\n");
      for (vector<Param>::const_iterator pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
	{
	  string elementToValue = createTypeCode (pi->type, "rec->" + pi->name, MODEL_TO_VALUE);
	  printf("  element = %s;\n", elementToValue.c_str());
	  printf("  sfi_rec_set (sfi_rec, \"%s\", element);\n", pi->name.c_str());
	  printf("  sfi_value_free (element);\n");        // FIXME: couldn't we have take_set
	}
      printf("  return sfi_rec;\n");
      printf("}\n\n");

      printf ("%s\n", cTypeRet(ri->name));
      printf ("%s::value_get_rec (const GValue *value)\n", nname.c_str());
      printf ("{\n");
      printf ("  return ::Sfi::cxx_value_get_rec< %s> (value);\n", nname.c_str());
      printf ("}\n\n");
      printf ("void\n");
      printf ("%s::value_set_rec (GValue *value, %s self)\n", nname.c_str(), cTypeArg (ri->name));
      printf ("{\n");
      printf ("  ::Sfi::cxx_value_set_rec< %s> (value, self);\n", nname.c_str());
      printf ("}\n\n");

      if (options.doImplementation)
	{
	  printf ("SfiRecFields\n");
	  printf ("%s::get_fields()\n", nname.c_str());
	  printf ("{\n");
	  printf ("  static SfiRecFields rfields = { 0, NULL };\n");
	  printf ("  if (!rfields.n_fields)\n");
	  printf ("    {\n");
	  printf ("      static GParamSpec *fields[%u + 1];\n", ri->contents.size());
	  printf ("      rfields.n_fields = %u;\n", ri->contents.size());
	  guint j = 0;
	  for (vector<Param>::const_iterator pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
	    {
	      // printf("#line %u \"%s\"\n", pi->line, parser.fileName().c_str());
	      printf("      fields[%u] = %s;\n", j++, untyped_pspec_constructor (*pi).c_str());
	    }
	  printf ("      rfields.fields = fields;\n");
	  printf ("    }\n");
	  printf ("  return rfields;\n");
	  printf ("}\n\n");
	}
    }
}

bool CodeGeneratorCxx::run ()
{
  vector<Choice>::const_iterator ei;
  vector<Param>::const_iterator pi;
  vector<Class>::const_iterator ci;
  vector<Method>::const_iterator mi;
 
  printf("\n/*-------- begin %s generated code --------*/\n\n\n", options.sfidlName.c_str());

  if (options.doHeader)
    {
      /* choices */
      for(ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
        {
          if (parser.fromInclude (ei->name)) continue;

          nspace.setFromSymbol (ei->name);

          printf("\nenum %s {\n", nspace.printableForm (ei->name).c_str());
          for (vector<ChoiceValue>::const_iterator ci = ei->contents.begin(); ci != ei->contents.end(); ci++)
            {
              /* don't export server side assigned choice values to the client */
              gint value = options.doInterface ? ci->sequentialValue : ci->value;
              string ename = makeUpperName (nspace.printableForm (ci->name));
              printf("  %s = %d,\n", ename.c_str(), value);
            }
          printf("};\n");
	}
      nspace.leaveAll();

      /* choice converters */
      for(ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
	{
	  string name = nspace.printableForm (ei->name);
	  string lname = makeLowerName (ei->name);

	  printf("const gchar* %s_to_choice (%s value);\n", lname.c_str(), name.c_str());
	  printf("%s %s_from_choice (const gchar *choice);\n", name.c_str(), lname.c_str());
        }

      printf("\n");
      /* prototypes for classes */
      for (ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
	{
	  if (parser.fromInclude (ci->name)) continue;

	  nspace.setFromSymbol (ci->name);
	  string name = nspace.printableForm (ci->name);

	  printf("class %s;\n", name.c_str());
	}

      printRecSeqForwardDecl (nspace);

      /* classes */
      for (ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
	{
	  if (parser.fromInclude (ci->name)) continue;

	  nspace.setFromSymbol (ci->name);
	  string name = nspace.printableForm (ci->name);

	  string init;
	  printf("\n");
	  if (ci->inherits == "")
	    {
	      printf("class %s {\n", name.c_str());
	      printf("protected:\n");
	      printf("  SfiProxy _object_id;\n");
	      init = "_object_id";
	    }
	  else
	    {
	      printf("class %s : public %s {\n", name.c_str(), ci->inherits.c_str());
	      init = ci->inherits;
	    }
	  printf("public:\n");
	  printf("  %s() : %s (0) {}\n", name.c_str(), init.c_str());
	  printf("  %s(SfiProxy p) : %s (p) {}\n", name.c_str(), init.c_str());
	  printf("  %s(const %s& other) : %s (other._object_id) {}\n",
	                                  name.c_str(), name.c_str(), init.c_str());
	  printf("  SfiProxy _proxy() const { return _object_id; }\n");
	  printf("  operator bool() const { return _object_id != 0; }\n");
	  printMethods(*ci);
	  printProperties(*ci);
	  printf("};\n");
	}
      printRecSeqDefinition (nspace);
      printRecSeqImpl (nspace);
    }

  if (options.doSource)
    {
      /* choice utils */
      if (options.doInterface)
	{
	  printChoiceConverters();
	  printf("\n");
	}

      /* methods */
      for (ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
	{
	  if (parser.fromInclude (ci->name)) continue;
	  printMethods(*ci);
	  printProperties(*ci);
	}
    }

  printf("\n");
  for (mi = parser.getProcedures().begin(); mi != parser.getProcedures().end(); mi++)
    {
      if (parser.fromInclude (mi->name)) continue;

      if (options.doHeader)
	nspace.setFromSymbol (mi->name);
      printProcedure (*mi, options.doHeader);
    }
  printf("\n");
  nspace.leaveAll();
  printf("\n/*-------- end %s generated code --------*/\n\n\n", options.sfidlName.c_str());

  return 1;
}

string CodeGeneratorCxx::makeProcName (const string& className, const string& procName)
{
  if (className == "")
    {
      return nspace.printableForm (nspace.namespaceOf (procName) + "::" +
				   makeStyleName (nspace.nameOf (procName)));
    }
  else
    {
      if (options.doHeader)
	return makeStyleName (procName);
      else
	return className + "::" + makeStyleName (procName);
    }
}

void CodeGeneratorCxx::printMethods (const Class& cdef)
{
  vector<Method>::const_iterator mi;
  vector<Param>::const_iterator pi;
  bool proto = options.doHeader;

  for (mi = cdef.methods.begin(); mi != cdef.methods.end(); mi++)
    {
      Method md;
      md.name = mi->name;
      md.result = mi->result;

      Param class_as_param;
      class_as_param.name = "_object_id";
      class_as_param.type = cdef.name;
      md.params.push_back (class_as_param);

      for(pi = mi->params.begin(); pi != mi->params.end(); pi++)
	md.params.push_back (*pi);

      if (proto) printf ("  ");
      printProcedure (md, proto, cdef.name);
    }
}

void CodeGeneratorCxx::printProperties (const Class& cdef)
{
  vector<Param>::const_iterator pi;
  bool proto = options.doHeader;

  for (pi = cdef.properties.begin(); pi != cdef.properties.end(); pi++)
    {
      string setProperty = makeStyleName ("set_" + pi->name);
      string getProperty = makeStyleName (pi->name);
      string newName = makeLowerName ("new_" + pi->name);
      string propName = makeLowerName (pi->name, '-');
      string ret = typeRet (pi->type);
      if (proto) {
	/* property getter */
	printf ("  %s %s ();\n", ret.c_str(), getProperty.c_str());

	/* property setter */
	printf ("  void %s (%s %s);\n", setProperty.c_str(), cTypeArg (pi->type), newName.c_str());
      }
      else {
	/* property getter */
	printf ("%s\n", ret.c_str());
	printf ("%s::%s ()\n", cdef.name.c_str(), getProperty.c_str());
	printf ("{\n");
	printf ("  const GValue *val;\n");
	printf ("  val = sfi_glue_proxy_get_property (_proxy(), \"%s\");\n", propName.c_str());
	printf ("  return %s;\n", createTypeCode (pi->type, "val", MODEL_FROM_VALUE).c_str());
	printf ("}\n");
	printf ("\n");
	/* property setter */
	printf ("void\n");
	printf ("%s::%s (%s %s)\n", cdef.name.c_str(), setProperty.c_str(),
				    cTypeArg (pi->type), newName.c_str());
	printf ("{\n");
	string to_val = createTypeCode (pi->type, newName, MODEL_TO_VALUE).c_str();
	printf ("  GValue *val = %s;\n", to_val.c_str());
	printf ("  sfi_glue_proxy_set_property (_proxy(), \"%s\", val);\n", propName.c_str());
	printf ("  sfi_value_free (val);\n");
	printf ("}\n");
	printf ("\n");
      }
    }
}

namespace {

class CxxFactory : public Factory {
public:
  string option() const	      { return "--cxx"; }
  string description() const  { return "generate core C++ language binding"; }

  void init (Options& options) const
  {
    options.doImplementation = false;
    options.doInterface = true;
  }

  CodeGenerator *create (const Parser& parser) const
  {
    return new CodeGeneratorCxx (parser);
  }
} cxx_factory;

}
/* vim:set ts=8 sts=2 sw=2: */
