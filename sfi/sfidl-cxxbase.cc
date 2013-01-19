// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "sfidl-cxxbase.hh"
#include "sfidl-factory.hh"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include "sfidl-namespace.hh"
#include "sfidl-options.hh"
#include "sfidl-parser.hh"
#include "sfiparams.hh" /* scatId (SFI_SCAT_*) */
using namespace Sfidl;
static String
CxxNameToSymbol (const String &str)     // FIXME: need mammut renaming function
{
  static const char *cset = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyz";
  String s;
  for (guint i = 0; i < s.size(); i++)
    if (!strchr (cset, s[i]))
      s[i] = '_';
  return s;
}
static vector<String>
split_string (const String &ctype)      // FIXME: remove once we have general renamer
{
  vector<String> vs;
  String type = ctype;
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
static String
join_string (const vector<String> &vs,  // FIXME: remove once we have general renamer
             const String         &delim)
{
  String r;
  for (vector<String>::const_iterator vi = vs.begin(); vi != vs.end(); vi++)
    {
      if (vi != vs.begin())
        r += delim;
      r += *vi;
    }
  return r;
}
static String
UC_NAME (const String &cstr)    // FIXME: need mammut renaming function
{
  vector<String> vs = split_string (cstr);
  String str = join_string (vs, "_");
  String r;
  char l = 0;
  for (String::const_iterator i = str.begin(); i != str.end(); i++)
    {
      if (islower (l) && isupper (*i))
        r += "_";
      r += toupper (l = *i);
    }
  return r;
}
static const char*
cUC_NAME (const String &cstr) // FIXME: need mammut renaming function
{
  return g_intern_string (cstr.c_str());
}
static String // FIXME: need mammut renaming function
UC_TYPE_NAME (const String &tname)
{
  vector<String> vs = split_string (tname);
  String lname = vs.back();
  vs.pop_back();
  String nspace = join_string (vs, ":");
  String result = UC_NAME (nspace) + "_TYPE_" + UC_NAME (lname);
  return result;
}
static const char*
cUC_TYPE_NAME (const String &cstr) // FIXME: need mammut renaming function
{
  return g_intern_string (UC_TYPE_NAME (cstr).c_str());
}
/* produce type-system-independant pspec constructors */
String
CodeGeneratorCxxBase::untyped_pspec_constructor (const Param &param)
{
  switch (parser.typeOf (param.type))
    {
    case CHOICE:
      {
        const String group = (param.group != "") ? param.group.escaped() : "NULL";
        String pspec = "sfidl_pspec_Choice";
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
        const String group = (param.group != "") ? param.group.escaped() : "NULL";
        String pspec = "sfidl_pspec_Record";
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
        const String group = (param.group != "") ? param.group.escaped() : "NULL";
        String pspec = "sfidl_pspec_Sequence";
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
String
CodeGeneratorCxxBase::typed_pspec_constructor (const Param &param)
{
  switch (parser.typeOf (param.type))
    {
    case CHOICE:
      {
        const String group = (param.group != "") ? param.group.escaped() : "NULL";
        String pspec = "sfidl_pspec_GEnum";
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
        const String group = (param.group != "") ? param.group.escaped() : "NULL";
        String pspec = "sfidl_pspec_BoxedRec";
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
        const String group = (param.group != "") ? param.group.escaped() : "NULL";
        String pspec = "sfidl_pspec_BoxedSeq";
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
        const String group = (param.group != "") ? param.group.escaped() : "NULL";
        String pspec = "sfidl_pspec_Object";
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
