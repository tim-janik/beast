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
#include "sfidl-cbase.h"
#include "sfidl-factory.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include "sfidl-namespace.h"
#include "sfidl-options.h"
#include "sfidl-parser.h"
#include "sfiparams.h" /* scatId (SFI_SCAT_*) */

using namespace Sfidl;
using namespace std;

/*--- functions for "C and C++"-like languages ---*/

const gchar *CodeGeneratorCBase::makeCStr (const std::string& str)
{
  return g_intern_string (str.c_str());
}

string CodeGeneratorCBase::makeGTypeName(const string& name)
{
  return makeUpperName (NamespaceHelper::namespaceOf (name)
                      + "::Type" + NamespaceHelper::nameOf(name));
}

string CodeGeneratorCBase::makeParamSpec(const Param& pdef)
{
  string pspec;
  const string group = (pdef.group != "") ? pdef.group.escaped() : "NULL";
 
  switch (parser.typeOf (pdef.type))
    {
      case CHOICE:
	{
	  pspec = "sfidl_pspec_Choice";
	  if (pdef.args == "")
	    pspec += "_default (" + group + ",\"" + pdef.name + "\",";
	  else
	    pspec += " (" + group + ", \"" + pdef.name + "\"," + pdef.args + ",";
	  pspec += makeLowerName (pdef.type) + "_get_values())";
	}
	break;
      case RECORD:
	{
	  pspec = "sfidl_pspec_Record";
	  if (pdef.args == "")
	    pspec += "_default (" + group + ", \"" + pdef.name + "\",";
	  else
	    pspec += " (" + group + ",\"" + pdef.name + "\"," + pdef.args + ",";
	  pspec += makeLowerName (pdef.type) + "_fields)";
	}
	break;
      case SEQUENCE:
	{
	  pspec = "sfidl_pspec_Sequence";
	  if (pdef.args == "")
	    pspec += "_default (" + group + ",\"" + pdef.name + "\",";
	  else
	    pspec += " (" + group + ",\"" + pdef.name + "\"," + pdef.args + ",";
	  pspec += makeLowerName (pdef.type) + "_content)";
	}
	break;
      case OBJECT:
	{
	  /* FIXME: the ParamSpec doesn't transport the type of the objects we require */
	  pspec = "sfidl_pspec_Proxy";
	  if (pdef.args == "")
	    pspec += "_default (" + group + ",\"" + pdef.name + "\")";
	  else
	    pspec += " (" + group + ",\"" + pdef.name + "\"," + pdef.args + ")";
	}
	break;
      default:
	{
	  pspec = "sfidl_pspec_" + pdef.pspec;
	  if (pdef.args == "")
	    pspec += "_default (" + group + ",\"" + pdef.name + "\")";
	  else
	    pspec += " (" + group + ",\"" + pdef.name + "\"," + pdef.args + ")";
	}
    }
  return pspec;
}

string CodeGeneratorCBase::scatId (SfiSCategory c)
{
  string s; s += (char) c;
  return s;
}

// how "type" looks like when passed as argument to a function
string
CodeGeneratorCBase::typeArg (const string& type)
{
  switch (parser.typeOf (type))
    {
      case VOID:      return "void";
      case BOOL:
      case INT:
      case NUM:
      case REAL:
      case CHOICE:    return makeMixedName (type);
      case STRING:    return "const gchar*";
      case BBLOCK:    
      case FBLOCK:    
      case SFIREC:    
      case RECORD:
      case SEQUENCE:  return makeMixedName (type)+"*";
      case OBJECT:    return "SfiProxy";
    }
  return "*error*";
}

// how "type" looks like when stored as member in a struct or class
string
CodeGeneratorCBase::typeField (const string& type)
{
  switch (parser.typeOf (type))
    {
      case VOID:      return "void";
      case BOOL:
      case INT:
      case NUM:
      case REAL:
      case CHOICE:    return makeMixedName (type);
      case STRING:    return "gchar*";
      case BBLOCK:
      case FBLOCK:
      case SFIREC:
      case RECORD:
      case SEQUENCE:  return makeMixedName (type)+"*";
      case OBJECT:    return "SfiProxy";
    }
  return "*error*";
}

// how the return type of a function returning "type" looks like
string
CodeGeneratorCBase::typeRet (const string& type)
{
  switch (parser.typeOf (type))
    {
      case VOID:      return "void";
      case BOOL:
      case INT:
      case NUM:
      case REAL:      
      case CHOICE:    return makeMixedName (type);   
      case STRING:    return "const gchar*";
      case BBLOCK:
      case FBLOCK:
      case SFIREC:
      case RECORD:
      case SEQUENCE:  return makeMixedName (type)+"*";
      case OBJECT:    return "SfiProxy";
    }
  return "*error*";
}

// how an array of "type"s looks like ( == MODEL_MEMBER + "*" ?)
string
CodeGeneratorCBase::typeArray (const string& type)
{
  return CodeGeneratorCBase::typeField (type) + "*";
}

// how to create a new "type" called "name" (blank return value allowed)
string
CodeGeneratorCBase::funcNew (const string& type)
{
  switch (parser.typeOf (type))
    {
      case VOID:
      case BOOL:
      case INT:
      case NUM:
      case REAL:      
      case STRING:    
      case CHOICE:    return "";
      case BBLOCK:    return "sfi_bblock_new";
      case FBLOCK:    return "sfi_fblock_new";
      case SFIREC:    return "sfi_rec_new";
      case RECORD:
      case SEQUENCE:  return makeLowerName (type)+"_new";
      case OBJECT:    return "";
    }
  return NULL;
}

string
CodeGeneratorCBase::funcCopy (const string& type)
{
  switch (parser.typeOf (type))
    {
      case VOID:
      case BOOL:
      case INT:
      case NUM:
      case REAL:      return "";
      case STRING:    return "g_strdup";
      case CHOICE:    return "";
      case BBLOCK:    return "sfi_bblock_ref";
      case FBLOCK:    return "sfi_fblock_ref";
      case SFIREC:    return "sfi_rec_ref";
      case RECORD:
      case SEQUENCE:  return makeLowerName (type)+"_copy_shallow";
      case OBJECT:    return "";
    }
  return NULL;
}

string
CodeGeneratorCBase::funcFree (const string& type)
{
  switch (parser.typeOf (type))
    {
      case VOID:
      case BOOL:
      case INT:
      case NUM:
      case REAL:      return "";
      case STRING:    return "g_free";
      case CHOICE:    return "";
      case BBLOCK:    return "sfi_bblock_unref";
      case FBLOCK:    return "sfi_fblock_unref";
      case SFIREC:    return "sfi_rec_unref";
      case RECORD:
      case SEQUENCE:  return makeLowerName (type)+"_free";
      case OBJECT:    return "";
    }
  return NULL;
}

string CodeGeneratorCBase::createTypeCode (const std::string& type, TypeCodeModel model)
{
  return createTypeCode (type, "", model);
}

string CodeGeneratorCBase::createTypeCode (const string& type, const string &name,
                                           TypeCodeModel model)
{
  switch (model)
    {
      /*
       * GValues: the following models deal with getting types into and out of
       * GValues
       */
      // how to create a new "type" called "name" from a GValue*
      case MODEL_FROM_VALUE:  g_assert (name != ""); break;
      // how to convert the "type" called "name" to a GValue*
      case MODEL_TO_VALUE:    g_assert (name != ""); break;
      
      /*
       * vcall interface: the following models deal with how to perform a
       * method/procedure invocation using a given data type
       */
      // the name of the VCALL function for calling functions returning "type"
      case MODEL_VCALL:	      g_assert (name == ""); break;
      // how to pass a "type" called "name" to the VCALL function
      case MODEL_VCALL_ARG:   g_assert (name != ""); break;
      // what type a conversion results in (== MODEL_VCALL_RET ?)
      case MODEL_VCALL_CARG:  g_assert (name == ""); break;
      // how to perform the conversion of a vcall parameter called "name" (optional: "" if unused)
      case MODEL_VCALL_CONV:  g_assert (name != ""); break;
      // how to free the conversion result of "name" (optional: "" if unused)
      case MODEL_VCALL_CFREE: g_assert (name != ""); break;
      // what type a vcall result is
      case MODEL_VCALL_RET:   g_assert (name == ""); break;
      // how to convert the result of a vcall called "name" (optional: name if unused)
      case MODEL_VCALL_RCONV: g_assert (name != ""); break;
      // how to free (using GC) the result of the conversion (optional: "" if unused)
      case MODEL_VCALL_RFREE: g_assert (name != ""); break;
    }

  switch (parser.typeOf (type))
    {
      case RECORD:
      case SEQUENCE:
	{
	  if (model == MODEL_VCALL_RFREE)
	    return "if ("+name+" != NULL) sfi_glue_gc_add ("+name+", "+makeLowerName (type)+"_free)";

	  if (parser.isSequence (type))
	  {
	    if (model == MODEL_TO_VALUE)
	      return "sfi_value_new_take_seq (" + makeLowerName (type)+"_to_seq ("+name+"))";
	    if (model == MODEL_FROM_VALUE) 
	      return makeLowerName (type)+"_from_seq (sfi_value_get_seq ("+name+"))";
	    if (model == MODEL_VCALL) 
	      return "sfi_glue_vcall_seq";
	    if (model == MODEL_VCALL_ARG) 
	      return "'" + scatId (SFI_SCAT_SEQ) + "', "+name+",";
	    if (model == MODEL_VCALL_CARG) 
	      return "SfiSeq*";
	    if (model == MODEL_VCALL_CONV) 
	      return makeLowerName (type)+"_to_seq ("+name+")";
	    if (model == MODEL_VCALL_CFREE) 
	      return "sfi_seq_unref ("+name+")";
	    if (model == MODEL_VCALL_RET) 
	      return "SfiSeq*";
	    if (model == MODEL_VCALL_RCONV) 
	      return makeLowerName (type)+"_from_seq ("+name+")";
	  }
	  else
	  {
	    if (model == MODEL_TO_VALUE)   
	      return "sfi_value_new_take_rec (" + makeLowerName (type)+"_to_rec ("+name+"))";
	    if (model == MODEL_FROM_VALUE)
	      return makeLowerName (type)+"_from_rec (sfi_value_get_rec ("+name+"))";
	    if (model == MODEL_VCALL) 
	      return "sfi_glue_vcall_rec";
	    if (model == MODEL_VCALL_ARG) 
	      return "'" + scatId (SFI_SCAT_REC) + "', "+name+",";
	    if (model == MODEL_VCALL_CARG) 
	      return "SfiRec*";
	    if (model == MODEL_VCALL_CONV) 
	      return makeLowerName (type)+"_to_rec ("+name+")";
	    if (model == MODEL_VCALL_CFREE) 
	      return "sfi_rec_unref ("+name+")";
	    if (model == MODEL_VCALL_RET) 
	      return "SfiRec*";
	    if (model == MODEL_VCALL_RCONV) 
	      return makeLowerName (type)+"_from_rec ("+name+")";
	  }
	}
	break;
      case CHOICE:
	{
	  if (options.generateBoxedTypes)
	    {
	      if (model == MODEL_TO_VALUE)
		return "sfi_value_choice_genum ("+name+", "+makeGTypeName(type)+")";
	      if (model == MODEL_FROM_VALUE) 
		return "(" + typeField(type) + ") sfi_choice2enum (sfi_value_get_choice ("+name+"), "+makeGTypeName(type)+")";
	    }
	  else /* client code */
	    {
	      if (model == MODEL_TO_VALUE)
		return "sfi_value_choice (" + makeLowerName (type) + "_to_choice ("+name+"))";
	      if (model == MODEL_FROM_VALUE) 
		return makeLowerName (type) + "_from_choice (sfi_value_get_choice ("+name+"))";
	    }
	  if (model == MODEL_VCALL)       return "sfi_glue_vcall_choice";
	  if (model == MODEL_VCALL_ARG)   return "'" + scatId (SFI_SCAT_CHOICE) + "', "+makeLowerName (type)+"_to_choice ("+name+"),";
	  if (model == MODEL_VCALL_CARG)  return "";
	  if (model == MODEL_VCALL_CONV)  return "";
	  if (model == MODEL_VCALL_CFREE) return "";
	  if (model == MODEL_VCALL_RET)   return "const gchar *";
	  if (model == MODEL_VCALL_RCONV) return makeLowerName (type)+"_from_choice ("+name+")";
	  if (model == MODEL_VCALL_RFREE) return "";
	}
	break;
      case OBJECT:
	{
	  /*
	   * FIXME: we're currently not using the type of the proxy anywhere
	   * it might for instance be worthwile being able to ensure that if
	   * we're expecting a "SfkServer" object, we will have one
	   */
	  if (model == MODEL_TO_VALUE)    return "sfi_value_proxy ("+name+")";
	  if (model == MODEL_FROM_VALUE)  return "sfi_value_get_proxy ("+name+")";
	  if (model == MODEL_VCALL)       return "sfi_glue_vcall_proxy";
	  if (model == MODEL_VCALL_ARG)   return "'" + scatId (SFI_SCAT_PROXY) + "', "+name+",";
	  if (model == MODEL_VCALL_CARG)  return "";
	  if (model == MODEL_VCALL_CONV)  return "";
	  if (model == MODEL_VCALL_CFREE) return "";
	  if (model == MODEL_VCALL_RET)   return "SfiProxy";
	  if (model == MODEL_VCALL_RCONV) return name;
	  if (model == MODEL_VCALL_RFREE) return "";
	}
	break;
      case STRING:
	{
	  switch (model)
	    {
	      case MODEL_TO_VALUE:    return "sfi_value_string ("+name+")";
	      case MODEL_FROM_VALUE:  return "sfi_value_dup_string ("+name+")";
	      case MODEL_VCALL:       return "sfi_glue_vcall_string";
	      case MODEL_VCALL_ARG:   return "'" + scatId (SFI_SCAT_STRING) + "', "+name+",";
	      case MODEL_VCALL_CARG:  return "";
	      case MODEL_VCALL_CONV:  return "";
	      case MODEL_VCALL_CFREE: return "";
	      case MODEL_VCALL_RET:   return "const gchar*";
	      case MODEL_VCALL_RCONV: return name;
	      case MODEL_VCALL_RFREE: return "";
	    }
	}
	break;
      case BBLOCK:
	{
	  if (model == MODEL_TO_VALUE)    return "sfi_value_bblock ("+name+")";
	  if (model == MODEL_FROM_VALUE)  return "sfi_bblock_ref (sfi_value_get_bblock ("+name+"))";
	  if (model == MODEL_VCALL)       return "sfi_glue_vcall_bblock";
	  if (model == MODEL_VCALL_ARG)   return "'" + scatId (SFI_SCAT_BBLOCK) + "', "+name+",";
	  if (model == MODEL_VCALL_CARG)  return "";
	  if (model == MODEL_VCALL_CONV)  return "";
	  if (model == MODEL_VCALL_CFREE) return "";
	  if (model == MODEL_VCALL_RET)   return "SfiBBlock*";
	  if (model == MODEL_VCALL_RCONV) return name;
	  if (model == MODEL_VCALL_RFREE) return "";
	}
	break;
      case FBLOCK:
	{
	  if (model == MODEL_TO_VALUE)    return "sfi_value_fblock ("+name+")";
	  if (model == MODEL_FROM_VALUE)  return "sfi_fblock_ref (sfi_value_get_fblock ("+name+"))";
	  if (model == MODEL_VCALL)       return "sfi_glue_vcall_fblock";
	  if (model == MODEL_VCALL_ARG)   return "'" + scatId (SFI_SCAT_FBLOCK) + "', "+name+",";
	  if (model == MODEL_VCALL_CARG)  return "";
	  if (model == MODEL_VCALL_CONV)  return "";
	  if (model == MODEL_VCALL_CFREE) return "";
	  if (model == MODEL_VCALL_RET)   return "SfiFBlock*";
	  if (model == MODEL_VCALL_RCONV) return name;
	  if (model == MODEL_VCALL_RFREE) return "";
	}
	break;
      case SFIREC:
	{
	  /* FIXME: review this for correctness */
	  if (model == MODEL_TO_VALUE)    return "sfi_value_rec ("+name+")";
	  if (model == MODEL_FROM_VALUE)  return "sfi_rec_ref (sfi_value_get_rec ("+name+"))";
	  if (model == MODEL_VCALL)       return "sfi_glue_vcall_rec";
	  if (model == MODEL_VCALL_ARG)   return "'" + scatId (SFI_SCAT_REC) + "', "+name+",";
	  if (model == MODEL_VCALL_CARG)  return "";
	  if (model == MODEL_VCALL_CONV)  return "";
	  if (model == MODEL_VCALL_CFREE) return "";
	  if (model == MODEL_VCALL_RET)   return "SfiRec*";
	  if (model == MODEL_VCALL_RCONV) return name;
	  if (model == MODEL_VCALL_RFREE) return "";
	}
	break;
      default:
	{
	  /* get rid of the Sfi:: (the code wasn't written for it) */
	  string ptype = NamespaceHelper::nameOf (type);

	  string sfi = (ptype == "void") ? "" : "Sfi"; /* there is no such thing as an SfiVoid */

	  if (model == MODEL_TO_VALUE)    return "sfi_value_" + makeLowerName(ptype) + " ("+name+")";
	  if (model == MODEL_FROM_VALUE)  return "sfi_value_get_" + makeLowerName(ptype) + " ("+name+")";
	  if (model == MODEL_VCALL)       return "sfi_glue_vcall_" + makeLowerName(ptype);
	  if (model == MODEL_VCALL_ARG)
	  {
	    if (ptype == "Real")	  return "'" + scatId (SFI_SCAT_REAL) + "', "+name+",";
	    if (ptype == "Bool")	  return "'" + scatId (SFI_SCAT_BOOL) + "', "+name+",";
	    if (ptype == "Int")		  return "'" + scatId (SFI_SCAT_INT) + "', "+name+",";
	    if (ptype == "Num")		  return "'" + scatId (SFI_SCAT_NUM) + "', "+name+",";
	  }
	  if (model == MODEL_VCALL_CARG)  return "";
	  if (model == MODEL_VCALL_CONV)  return "";
	  if (model == MODEL_VCALL_CFREE) return "";
	  if (model == MODEL_VCALL_RET)   return sfi + ptype;
	  if (model == MODEL_VCALL_RCONV) return name;
	  if (model == MODEL_VCALL_RFREE) return "";
	}
	break;
    }
  return "*createTypeCode*unknown*";
}

/*--- the C language binding ---*/

std::string CodeGeneratorCBase::makeProcName (const std::string& className,
	                                      const std::string& procName)
{
  if (className == "")
    return makeLowerName(procName);
  else
    return makeLowerName(className) + "_" + makeLowerName(procName);
}

void CodeGeneratorCBase::printProcedure (const Method& mdef, bool proto, const string& className)
{
  vector<Param>::const_iterator pi;
  string dname, mname = makeProcName (className, mdef.name);
  
  if (className == "")
    {
      dname = makeLowerName(mdef.name, '-');
    }
  else
    {
      dname = makeMixedName(className) + "+" + makeLowerName(mdef.name, '-');
    }

  bool first = true;
  printf("%s%s%s (", cTypeRet (mdef.result.type), proto?" ":"\n", mname.c_str());
  for(pi = mdef.params.begin(); pi != mdef.params.end(); pi++)
    {
      if (pi->name == "_object_id") continue; // C++ binding: get _object_id from class

      if(!first) printf(", ");
      first = false;
      printf("%s %s", cTypeArg (pi->type), pi->name.c_str());
    }
  if (first)
    printf("void");
  printf(")");
  if (proto)
    {
      printf(";\n");
      return;
    }

  printf(" {\n");

  string vret = createTypeCode (mdef.result.type, MODEL_VCALL_RET);
  if (mdef.result.type != "void")
    printf ("  %s _retval;\n", vret.c_str());

  string rfree = createTypeCode (mdef.result.type, "_retval_conv", MODEL_VCALL_RFREE);
  if (rfree != "")
    printf ("  %s _retval_conv;\n", cTypeRet (mdef.result.type));

  map<string, string> cname;
  for(pi = mdef.params.begin(); pi != mdef.params.end(); pi++)
    {
      string conv = createTypeCode (pi->type, pi->name, MODEL_VCALL_CONV);
      if (conv != "")
	{
	  cname[pi->name] = pi->name + "__c";

	  string arg = createTypeCode(pi->type, MODEL_VCALL_CARG);
	  printf("  %s %s__c = %s;\n", arg.c_str(), pi->name.c_str(), conv.c_str());
	}
      else
	cname[pi->name] = pi->name;
    }

  printf("  ");
  if (mdef.result.type != "void")
    printf("_retval = ");
  string vcall = createTypeCode(mdef.result.type, "", MODEL_VCALL);
  printf("%s (\"%s\", ", vcall.c_str(), dname.c_str());

  for(pi = mdef.params.begin(); pi != mdef.params.end(); pi++)
    printf("%s ", createTypeCode(pi->type, cname[pi->name], MODEL_VCALL_ARG).c_str());
  printf("0);\n");

  for(pi = mdef.params.begin(); pi != mdef.params.end(); pi++)
    {
      string cfree = createTypeCode (pi->type, cname[pi->name], MODEL_VCALL_CFREE);
      if (cfree != "")
	printf("  %s;\n", cfree.c_str());
    }

  if (mdef.result.type != "void")
    {
      string rconv = createTypeCode (mdef.result.type, "_retval", MODEL_VCALL_RCONV);

      if (rfree != "")
	{
	  printf ("  _retval_conv = %s;\n", rconv.c_str());
	  printf ("  %s;\n", rfree.c_str());
	  printf ("  return _retval_conv;\n");
	}
      else
	{
	  printf ("  return %s;\n", rconv.c_str());
	}
    }
  printf("}\n\n");
}

static bool choiceReverseSort(const ChoiceValue& e1, const ChoiceValue& e2)
{
  string ename1 = e1.name;
  string ename2 = e2.name;

  reverse (ename1.begin(), ename1.end());
  reverse (ename2.begin(), ename2.end());

  return ename1 < ename2;
}

void CodeGeneratorCBase::printChoiceConverters()
{
  vector<Choice>::const_iterator ei;

  for(ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
    {
      if (parser.fromInclude (ei->name))
        continue;

      int minval = 1, maxval = 1;
      vector<ChoiceValue>::iterator ci;
      string name = makeLowerName (ei->name);
      string arg = typeArg (ei->name);

      /* produce reverse sorted enum array */
      vector<ChoiceValue> components = ei->contents;
      for (ci = components.begin(); ci != components.end(); ci++)
	ci->name = makeLowerName (ci->name, '-');
      sort (components.begin(), components.end(), ::choiceReverseSort);

      printf("static const SfiConstants %s_vals[%d] = {\n",name.c_str(), ei->contents.size());
      for (ci = components.begin(); ci != components.end(); ci++)
	{
	  int value = ci->sequentialValue;
	  minval = min (value, minval);
	  maxval = max (value, maxval);
	  printf("  { \"%s\", %d, %d },\n", ci->name.c_str(), ci->name.size(), value);
	}
      printf("};\n\n");

      printf("const gchar*\n");
      printf("%s_to_choice (%s value)\n", name.c_str(), arg.c_str());
      printf("{\n");
      printf("  g_return_val_if_fail (value >= %d && value <= %d, NULL);\n", minval, maxval);
      printf("  return sfi_constants_get_name (G_N_ELEMENTS (%s_vals), %s_vals, value);\n",
	  name.c_str(), name.c_str());
      printf("}\n\n");

      printf("%s\n", cTypeRet (ei->name));
      printf("%s_from_choice (const gchar *choice)\n", name.c_str());
      printf("{\n");
      printf("  return (%s) (choice ? sfi_constants_get_index (G_N_ELEMENTS (%s_vals), "
	                    "%s_vals, choice) : 0);\n", cTypeRet (ei->name), name.c_str(), name.c_str());
      printf("}\n");
      printf("\n");
    }
}

/* vim:set ts=8 sts=2 sw=2: */
