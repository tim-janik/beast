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
	  printf("typedef Bse::SmartPtr<%s,CountablePointer<RefCountable> > %sPtr;\n",
	      name.c_str(), name.c_str());
	  printf("typedef Bse::SmartPtr<const %s,CountablePointer<const RefCountable> > %sCPtr;\n",
	      name.c_str(), name.c_str());
	}

      /* records */
      for (ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
	{
	  if (parser.fromInclude (ri->name)) continue;

	  nspace.setFromSymbol(ri->name);
	  string name = nspace.printableForm (ri->name);

	  printf("class %s : public RefCountable {\n", name.c_str());
	  printf("public:\n");
	  for (pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
	    {
	      printf("  %s %s;\n", createTypeCode(pi->type, MODEL_MEMBER).c_str(),
		                   pi->name.c_str());
	    }
	  printf("  static %sPtr _from_rec (SfiRec *rec) { return 0; }\n", name.c_str());
	  printf("  static SfiRec *_to_rec (%sPtr ptr) { return 0; }\n", name.c_str());
	  printf("};\n");
	}
    }

  for (mi = parser.getProcedures().begin(); mi != parser.getProcedures().end(); mi++)
    {
      if (parser.fromInclude (mi->name)) continue;

      if (options.doHeader)
	nspace.setFromSymbol (mi->name);
      printProcedure (*mi, options.doHeader);
    }
  nspace.leaveAll();
}

string CodeGeneratorCxx::makeProcName (const string& className, const string& procName)
{
  g_assert (className == "");
  return nspace.printableForm (nspace.namespaceOf (procName) + "::" +
                               makeStyleName (nspace.nameOf (procName)));
}

static string fail (const string& error)
{
  g_error ("%s\n", error.c_str());
  return "*fail(" + error + ")*";
}

string CodeGeneratorCxx::createTypeCode (const std::string& type, const std::string& name, 
				         TypeCodeModel model)
{
  /* FIXME: parameter validation */
  switch (parser.typeOf (type))
    {
      case STRING:
	switch (model)
	  {
	    case MODEL_ARG:         return "const std::string&";
	    case MODEL_MEMBER:      return "std::string";
	    case MODEL_RET:         return "std::string";
	    case MODEL_ARRAY:       return fail ("C specific");
	    case MODEL_FREE:        return "";
	    case MODEL_COPY:        return name;
	    case MODEL_NEW:         return "";
	    case MODEL_TO_VALUE:    return "sfi_value_string ("+name+".c_str())";
	    case MODEL_FROM_VALUE:  return "sfi_value_get_string ("+name+")";
	    case MODEL_VCALL:       return "sfi_glue_vcall_string";
	    case MODEL_VCALL_ARG:   return "'" + scatId (SFI_SCAT_STRING) + "', "+name+".c_str(),";
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
	    case MODEL_ARG:	    return type + "Ptr";
	    case MODEL_MEMBER:	    return type + "Ptr";
	    case MODEL_RET:	    return type + "Ptr";
	    case MODEL_ARRAY:       return fail ("C specific");
	    default: ;
	  }
	  break;
      default: ;
    }
  return CodeGeneratorCBase::createTypeCode (type, name, model);
}
/* vim:set ts=8 sts=2 sw=2: */
