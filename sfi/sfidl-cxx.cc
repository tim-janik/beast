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

const gchar *
CodeGeneratorCxxBase::typeArg (const string& type)
{
  switch (parser.typeOf (type))
    {
      case STRING:    return "const std::string&";
      case RECORD:    return makeCStr (type + "Ptr");
      case SEQUENCE:  return makeCStr ("const " + type + "&");
      case CHOICE:    return makeCStr (type);
      case OBJECT:    return makeCStr (type);
      default:	      return CodeGeneratorCBase::typeArg (type);
    }
}

const gchar *
CodeGeneratorCxxBase::typeField (const string& type)
{
  switch (parser.typeOf (type))
    {
      case STRING:    return "std::string";
      case RECORD:    return makeCStr (type + "Ptr");
      case CHOICE:
      case OBJECT:
      case SEQUENCE:  return makeCStr (type);
      default:	      return CodeGeneratorCBase::typeArg (type);
    }
}

const gchar *
CodeGeneratorCxxBase::typeRet (const string& type)
{
  switch (parser.typeOf (type))
    {
      case STRING:    return "std::string";
      case RECORD:    return makeCStr (type + "Ptr");
      case CHOICE:
      case OBJECT:
      case SEQUENCE:  return makeCStr (type);
      default:	      return CodeGeneratorCBase::typeArg (type);
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
	    case MODEL_FREE:        return "";
	    case MODEL_COPY:        return name;
	    case MODEL_NEW:         return "";
	    case MODEL_TO_VALUE:    return "sfi_value_string ("+name+".c_str())";
	    case MODEL_FROM_VALUE:  return "sfi_value_get_cxxstring ("+name+")";
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
	    case MODEL_TO_VALUE:    return "sfi_value_new_take_rec ("+type+"::_to_rec ("+name+"))";
	    case MODEL_FROM_VALUE:  return type + "::_from_rec (sfi_value_get_rec ("+name+"))";
	    case MODEL_VCALL_CONV:  return type + "::_to_rec ("+name+")";
	    case MODEL_VCALL_RCONV: return type + "::_from_rec ("+name+")";
	    case MODEL_VCALL_RFREE: return "";
	    default: ;
	  }
	  break;
      case SEQUENCE:
	switch (model)
	  {
	    case MODEL_TO_VALUE:    return "sfi_value_new_take_seq ("+name+"._to_seq ())";
	    case MODEL_FROM_VALUE:  return type + "::_from_seq (sfi_value_get_seq ("+name+"))";
	    case MODEL_VCALL_CONV:  return type + "::_to_seq ("+name+")";
	    case MODEL_VCALL_RCONV: return type + "::_from_seq ("+name+")";
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

void CodeGeneratorCxx::run ()
{
  vector<Sequence>::const_iterator si;
  vector<Record>::const_iterator ri;
  vector<Choice>::const_iterator ei;
  vector<Param>::const_iterator pi;
  vector<Class>::const_iterator ci;
  vector<Method>::const_iterator mi;
 
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
              string ename = makeUpperName (ci->name);
              printf("  %s = %d,\n", ename.c_str(), value);
            }
          printf("};\n");
        }

      /* records and sequences */
      printf ("\n/* record/sequence types */\n");

      /* prototypes for records */
      for (ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
	{
	  if (parser.fromInclude (ri->name)) continue;

	  nspace.setFromSymbol(ri->name);
	  string name = nspace.printableForm (ri->name);

	  printf("\n");
	  printf("class %s;\n", name.c_str());
	  printf("typedef Bse::SmartPtr<%s,Bse::CountablePointer<Bse::RefCountable> > %sPtr;\n",
	      name.c_str(), name.c_str());
	  printf("typedef Bse::SmartPtr<const %s,Bse::CountablePointer<const Bse::RefCountable> >"
	      " %sCPtr;\n", name.c_str(), name.c_str());
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

      /* sequences */
      for (si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
	{
	  if (parser.fromInclude (si->name)) continue;

	  nspace.setFromSymbol(si->name);

	  /* FIXME: need optimized refcounted copy-on-write sequences as base types */

	  string name = nspace.printableForm (si->name);
	  string content = typeField (si->content.type);

	  printf("\n");
	  printf("class %s : public Bse::Sequence<%s> {\n", name.c_str(), content.c_str());
	  printf("public:\n");
	  /* TODO: make this a constructor? */
	  printf("  static %s _from_seq (SfiSeq *seq);\n", typeRet (si->name));
	  printf("  SfiSeq *_to_seq ();\n");
	  printf("};\n");
	}

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

      /* records */
      for (ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
	{
	  if (parser.fromInclude (ri->name)) continue;

	  nspace.setFromSymbol(ri->name);
	  string name = nspace.printableForm (ri->name);

	  printf("\n");
	  printf("class %s : public Bse::RefCountable {\n", name.c_str());
	  printf("public:\n");
	  for (pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
	    {
	      printf("  %s %s;\n", typeField (pi->type), pi->name.c_str());
	    }
	  printf("  static %sPtr _from_rec (SfiRec *rec);\n", name.c_str());
	  printf("  static SfiRec *_to_rec (%sPtr ptr);\n", name.c_str());
	  printf("};\n");
	}
    }

  if (options.doSource)
    {
      /* choice utils */
      if (options.doInterface)
	{
	  printf("namespace {\n");
	  printChoiceConverters();
	  printf("}\n");
	  printf("\n");
	}

      /* sequence members */
      for (si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
	{
	  if (parser.fromInclude (si->name)) continue;

	  string name = si->name;

          string elementFromValue = createTypeCode (si->content.type, "element", MODEL_FROM_VALUE);
          printf("%s\n", typeRet (si->name));
          printf("%s::_from_seq (SfiSeq *sfi_seq)\n", name.c_str());
          printf("{\n");
          printf("  %s seq;\n", typeRet (si->name));
          printf("  guint i, length;\n");
          printf("\n");
          printf("  g_return_val_if_fail (sfi_seq != NULL, seq);\n");
          printf("\n");
          printf("  length = sfi_seq_length (sfi_seq);\n");
	  printf("  seq.resize (length);\n");
          printf("  for (i = 0; i < length; i++)\n");
          printf("  {\n");
          printf("    GValue *element = sfi_seq_get (sfi_seq, i);\n");
          printf("    seq[i] = (%s);\n", elementFromValue.c_str());
          printf("  }\n");
          printf("  return seq;\n");
          printf("}\n\n");

	  /* FIXME: ugly code (*this[i]) */
	  string elementToValue = createTypeCode (si->content.type, "(*this)[i]", MODEL_TO_VALUE);
	  printf("SfiSeq *\n");
	  printf("%s::_to_seq ()\n", name.c_str());
	  printf("{\n");
	  printf("  SfiSeq *sfi_seq = sfi_seq_new ();\n");
	  printf("  for (guint i = 0; i < size(); i++)\n");
	  printf("  {\n");
	  printf("    GValue *element = %s;\n", elementToValue.c_str());
	  printf("    sfi_seq_append (sfi_seq, element);\n");
	  printf("    sfi_value_free (element);\n");        // FIXME: couldn't we have take_append
	  printf("  }\n");
	  printf("  return sfi_seq;\n");
	  printf("}\n\n");
	}

      /* record members */
      for (ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
	{
	  if (parser.fromInclude (ri->name)) continue;

	  string name = ri->name;

          printf("%s\n", typeRet (ri->name));
          printf("%s::_from_rec (SfiRec *sfi_rec)\n", name.c_str());
          printf("{\n");
          printf("  GValue *element;\n");
          printf("\n");
          printf("  if (!sfi_rec)\n");
	  printf("    return NULL;\n");
          printf("\n");
          printf("  %s rec = new %s();\n", typeArg (ri->name), name.c_str());
          for (pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
            {
              string elementFromValue = createTypeCode (pi->type, "element", MODEL_FROM_VALUE);

              printf("  element = sfi_rec_get (sfi_rec, \"%s\");\n", pi->name.c_str());
              printf("  if (element)\n");
              printf("    rec->%s = %s;\n", pi->name.c_str(), elementFromValue.c_str());
            }
          printf("  return rec;\n");
          printf("}\n\n");

	  printf("SfiRec *\n");
	  printf("%s::_to_rec (%s rec)\n", name.c_str(), typeArg (ri->name));
	  printf("{\n");
	  printf("  SfiRec *sfi_rec;\n");
	  printf("  GValue *element;\n");
	  printf("\n");
	  printf("  g_return_val_if_fail (rec, NULL);\n");
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

void CodeGeneratorCxx::printProperties(const Class& cdef)
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
	printf ("  void %s (%s %s);\n", setProperty.c_str(), typeArg (pi->type), newName.c_str());
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
				    typeArg (pi->type), newName.c_str());
	printf ("{\n");
	string conv = createTypeCode (pi->type, newName.c_str(), MODEL_VCALL_CONV);
	if (conv != "")
	  {
	    string carg = createTypeCode(pi->type, MODEL_VCALL_CARG);
	    newName += "__c"; // use the converted name new_x__c now, instead of new_x
	    printf("  %s %s = %s;\n", carg.c_str(), newName.c_str(), conv.c_str());
	  }
	string to_val = createTypeCode (pi->type, newName, MODEL_TO_VALUE).c_str();
	printf ("  GValue *val = %s;\n", to_val.c_str());
	printf ("  sfi_glue_proxy_set_property (_proxy(), \"%s\", val);\n", propName.c_str());
	printf ("  sfi_value_free (val);\n");
	string free = createTypeCode (pi->type, newName.c_str(), MODEL_VCALL_CFREE);
	if (free != "")
	  printf("  %s;\n", free.c_str());
	printf ("}\n");
	printf ("\n");
      }
    }
}

/* vim:set ts=8 sts=2 sw=2: */
