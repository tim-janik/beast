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
#include "sfidl-cxxbase.hh"
#include "sfidl-factory.hh"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include "sfidl-namespace.hh"
#include "sfidl-options.hh"
#include "sfidl-parser.hh"
#include "sfiparams.h" /* scatId (SFI_SCAT_*) */

using namespace Sfidl;
using namespace std;

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

/* vim:set ts=8 sts=2 sw=2: */
