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
#include "sfidl-generator.h"
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

/*--- common functions ---*/

string CodeGenerator::makeNamespaceSubst (const string& name)
{
  if(name.substr (0, options.namespaceCut.length ()) == options.namespaceCut)
    return options.namespaceAdd + name.substr (options.namespaceCut.length ());
  else
    return name; /* pattern not matched */
}

vector<string> CodeGenerator::splitName (const string& name)
{
  bool lastupper = true, upper = true, lastunder = true, remove_caps = false;
  string::const_iterator i;
  string sname = makeNamespaceSubst (name);
  vector<string> words;
  string word;

  /*
   * we try to guess here whether we need to remove caps
   * or keep them
   *
   * if our input is BseSNet, it is vital to keep the caps
   * if our input is IO_ERROR, we need to remove it
   */
  for(i = sname.begin(); i != sname.end(); i++)
    {
      if (*i == '_')
	remove_caps = true;
    }

  for(i = sname.begin(); i != sname.end(); i++)
    {
      lastupper = upper;
      upper = isupper(*i);
      if (!lastupper && upper && !lastunder)
	{
	  words.push_back (word);
	  word = "";
	  lastunder = true;
	}
      if(*i == ':' || *i == '_')
	{
	  if(!lastunder)
	    {
	      words.push_back (word);
	      word = "";
	      lastunder = true;
	    }
	}
      else
	{
	  if (remove_caps)
	    word += tolower(*i);
	  else
	    word += *i;
	  lastunder = false;
	}
    }

  if (word != "")
    words.push_back (word);

  return words;
}


string CodeGenerator::makeLowerName (const string& name, char seperator)
{
  string result;
  const vector<string>& words = splitName (name);

  for (vector<string>::const_iterator wi = words.begin(); wi != words.end(); wi++)
    {
      if (result != "") result += seperator;

      for (string::const_iterator i = wi->begin(); i != wi->end(); i++)
	result += tolower (*i);
    }
  
  return result;
}

string CodeGenerator::makeUpperName (const string& name)
{
  string lname = makeLowerName (name);
  string result;
  string::const_iterator i;
  
  for(i = lname.begin(); i != lname.end(); i++)
    result += toupper(*i);
  return result;
}

string CodeGenerator::makeMixedName (const string& name)
{
  string result;
  const vector<string>& words = splitName (name);

  for (vector<string>::const_iterator wi = words.begin(); wi != words.end(); wi++)
    {
      bool first = true;

      for (string::const_iterator i = wi->begin(); i != wi->end(); i++)
	{
	  if (first)
	    result += toupper (*i);
	  else
	    result += *i;
	  first = false;
	}
    }
  
  return result;
}

string CodeGenerator::makeLMixedName (const string& name)
{
  string result = makeMixedName (name);

  if (!result.empty()) result[0] = tolower (result[0]);
  return result;
}

string CodeGenerator::makeStyleName (const string& name)
{
  if (options.style == Options::STYLE_MIXED)
    return makeLMixedName (name);
  return makeLowerName (name);
}

string CodeGenerator::toWordCase (const string& word, WordCase wc)
{
  string result;
  for (string::const_iterator si = word.begin(); si != word.end(); si++)
    {
      bool first = (si == word.begin());
      switch (wc)
	{
	  case lower:	      result += tolower (*si);
			      break;
	  case Capitalized:   result += first ? toupper (*si) : *si;
			      break;
	  case UPPER:	      result += toupper (*si);
			      break;
	  default:	      g_assert_not_reached();
	}
    }
  return result;
}

string CodeGenerator::joinName (const vector<string>& name, const string& seperator, WordCase wc)
{
  string result;

  for (vector<string>::const_iterator wi = name.begin(); wi != name.end(); wi++)
    {
      if (result != "")
	result += seperator;
      if (wc == semiCapitalized)
	{
	  if (result == "")
	    result += toWordCase (*wi, lower);
	  else
	    result += toWordCase (*wi, Capitalized);
	}
      else
	result += toWordCase (*wi, wc);
    }
  return result;
}

string
CodeGenerator::rename (NamespaceType namespace_type, const string& name, WordCase namespace_wc,
		       const string &namespace_join, const vector<string> &namespace_append,
		       WordCase typename_wc, const string &typename_join)
{
  string result;
  vector<string> namespace_words;

  if (namespace_type == ABSOLUTE)
    {
      /*
       * note that if namespace_join is "::", then "::" will also be prefixed to the result,
       * whereas if namespace_join is "_", it will only be used to seperate the namespaces
       * (this is required/useful for C++)
       */
      if (namespace_join == "::")
	result = namespace_join;
      namespace_words = splitName (NamespaceHelper::namespaceOf (name));
    }

  namespace_words.insert (namespace_words.end(), namespace_append.begin(), namespace_append.end());
  if (!namespace_words.empty())
    {
      result += joinName (namespace_words, namespace_join, namespace_wc);
      result += namespace_join;
    }

  vector<string> words = splitName (NamespaceHelper::nameOf (name));
  result += joinName (words, typename_join, typename_wc);
  return result;
}

string
CodeGenerator::rename (NamespaceHelper& nsh, const string& name, WordCase namespace_wc,
		       const string& namespace_join, const vector<string>& namespace_append,
		       WordCase typename_wc, const string& typename_join)
{
  g_assert_not_reached ();
  string pform = nsh.printableForm (name);
  return pform;
}

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
	  pspec += makeLowerName (pdef.type) + "_values)";
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

void CodeGeneratorC::printInfoStrings (const string& name, const Map<string,IString>& infos)
{
  printf("static const gchar *%s[] = {\n", name.c_str());

  Map<string,IString>::const_iterator ii;
  for (ii = infos.begin(); ii != infos.end(); ii++)
    printf("  \"%s=%s\",\n", ii->first.c_str(), ii->second.c_str());

  printf("  NULL,\n");
  printf("};\n");
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
      if (parser.fromInclude (ei->name)) continue;

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

void CodeGeneratorC::run ()
{
  vector<Sequence>::const_iterator si;
  vector<Record>::const_iterator ri;
  vector<Choice>::const_iterator ei;
  vector<Param>::const_iterator pi;
  vector<Class>::const_iterator ci;
  vector<Method>::const_iterator mi;
 
  printf("\n/*-------- begin %s generated code --------*/\n\n\n", Options::the()->sfidlName.c_str());

  if (options.generateTypeC)
    printf("#include <string.h>\n");
  if (options.generateConstant)
    {
      vector<Constant>::const_iterator ci;
      for (ci = parser.getConstants().begin(); ci != parser.getConstants().end(); ci++)
	{
	  if (parser.fromInclude (ci->name)) continue;

	  string uname = makeUpperName(ci->name);
	  printf("#define %s ", uname.c_str());
	  switch (ci->type) {
	    case Constant::tString: printf("\"%s\"\n", ci->str.c_str());
	      break;
	    case Constant::tFloat: printf("%f\n", ci->f);
	      break;
	    case Constant::tInt: printf("%d\n", ci->i);
	      break;
	  }
	}
      printf("\n");
    }
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
	      gint value = options.doInterface ? ci->sequentialValue : ci->value;
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

	  printf("extern SfiChoiceValues %s_values;\n", makeLowerName (ei->name).c_str());
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
	  if (parser.fromInclude (ei->name)) continue;

	  string name = makeLowerName (ei->name);
	  printf("static const GEnumValue %s_value[%d] = {\n", name.c_str(), ei->contents.size() + 1);
	  for (vector<ChoiceValue>::const_iterator ci = ei->contents.begin(); ci != ei->contents.end(); ci++)
	    {
	      string ename = makeUpperName (ci->name);
	      printf("  { %d, \"%s\", \"%s\" },\n", ci->value, ename.c_str(), ci->text.c_str());
	    }
	  printf("  { 0, NULL, NULL }\n");
	  printf("};\n");
	  printf("static const SfiChoiceValue %s_cvalue[%d] = {\n", name.c_str(), ei->contents.size());
	  for (vector<ChoiceValue>::const_iterator ci = ei->contents.begin(); ci != ei->contents.end(); ci++)
	    {
	      string ename = makeUpperName (ci->name);
	      printf("  { \"%s\", \"%s\" },\n", ename.c_str(), ci->text.c_str());
	    }
	  printf("};\n");
	  printf("SfiChoiceValues %s_values = { %d, %s_cvalue };\n", name.c_str(), ei->contents.size(), name.c_str());
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

  if (options.doInterface && options.doSource)
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

  if (options.generateSignalStuff)
    {
      for (ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
	{
	  if (parser.fromInclude (ci->name)) continue;

	  vector<Method>::const_iterator si;
	  for (si = ci->signals.begin(); si != ci->signals.end(); si++)
	    {
	      string fullname = makeLowerName (ci->name + "::" + si->name);

	      printf("void %s_frobnicator (SignalContext *sigcontext) {\n", fullname.c_str());
	      printf("  /* TODO: do something meaningful here */\n");
	      for (pi = si->params.begin(); pi != si->params.end(); pi++)
		{
		  printf("  %s %s;\n", cTypeArg (pi->type), pi->name.c_str());
		}
	      printf("}\n");
	    }
	}
    }

  if (options.doInterface && options.doHeader)
    {
      for (ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
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

  bool protoProcedures = options.doHeader;
  if (options.generateProcedures || protoProcedures)
    {
      for (ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
	{
	  if (parser.fromInclude (ci->name)) continue;

	  for (mi = ci->methods.begin(); mi != ci->methods.end(); mi++)
	    {
	      Method md;
	      md.name = mi->name;
	      md.result = mi->result;

	      Param class_as_param;
	      class_as_param.name = makeLowerName(ci->name) + "_object";
	      class_as_param.type = ci->name;
	      md.params.push_back (class_as_param);

	      for(pi = mi->params.begin(); pi != mi->params.end(); pi++)
		md.params.push_back (*pi);

	      printProcedure (md, protoProcedures, ci->name);
	    }
	}
      for (mi = parser.getProcedures().begin(); mi != parser.getProcedures().end(); mi++)
	{
	  if (parser.fromInclude (mi->name)) continue;
	  printProcedure (*mi, protoProcedures);
	}
    }

  printf("\n/*-------- end %s generated code --------*/\n\n\n", Options::the()->sfidlName.c_str());
}

void CodeGeneratorQt::run ()
{
  printf("\n/*-------- begin %s generated code --------*/\n\n\n", Options::the()->sfidlName.c_str());
  NamespaceHelper nspace(stdout);

  if (options.generateProcedures)
    {
      vector<Method>::const_iterator mi;
      for (mi = parser.getProcedures().begin(); mi != parser.getProcedures().end(); mi++)
	{
	  if (parser.fromInclude (mi->name)) continue;

	  if (mi->result.type == "void" && mi->params.empty())
	    {
	      nspace.setFromSymbol (mi->name);
	      string mname = makeLMixedName(nspace.printableForm (mi->name));
	      string dname = makeLowerName(mi->name, '-');

	      printf("void %s () {\n", mname.c_str());
	      printf("  sfi_glue_vcall_void (\"%s\", 0);\n", dname.c_str());
	      printf("}\n");
	    }
	}
    }

  nspace.leaveAll();
  printf("\n/*-------- end %s generated code --------*/\n\n\n", Options::the()->sfidlName.c_str());
}

/* vim:set ts=8 sts=2 sw=2: */
