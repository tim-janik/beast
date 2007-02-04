/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#if defined BSE_COMPILATION
#include "bsecxxclosure.hh"

namespace Bse {

CxxClosure::CxxClosure()
  : glib_closure (NULL), sig_tokens ()
{
}

CxxClosure::~CxxClosure()
{
}

GClosure*
CxxClosure::gclosure()
{
  return glib_closure;
}

} // Bse

#else /* !BSE_COMPILATION */    // program to generate bsegenclosures.h

#include <string>
#include <stdio.h>
using namespace std;
inline string
String (long long int i)
{
  char buf[16];
  sprintf (buf, "%lld", i);
  return string (buf);
}

void
print_closure (bool withreturn,
               int  n_args)
{
  const char *rpref = withreturn ? "Ret" : "Void";
  const char *rtype = withreturn ? "R" : "void";
  string s;
  /* template declaration: <typename R, class T, typename A1, typename A2, ...> */
  s = "";
  if (withreturn)
    s += "typename R, ";
  s += "class T";
  for (int i = 1; i <= n_args; i++)
    s += ", typename A" + String (i);
  const char *tmpldecl = strdup (s.c_str());
  /* template args: <R, T, A1, A2, ...> */
  s = "";
  if (withreturn)
    s += "R, ";
  s += "T";
  for (int i = 1; i <= n_args; i++)
    s += ", A" + String (i);
  const char *tmplargs = strdup (s.c_str());
  /* argtypes: (A1, A2, A3) */
  s = "";
  if (n_args)
    s += "A1";
  for (int i = 2; i <= n_args; i++)
    s += ", A" + String (i);
  const char *argtypes = strdup (s.c_str());
  /* introductionary comment */
  printf ("\n/* %s%uClosure */\n", rpref, n_args);
  /* class template */
  printf ("template<%s>\n", tmpldecl);
  printf ("class %s%uClosure : public CxxClosure {\n", rpref, n_args);
  printf ("  typedef %s (T::*MCb) (%s); /* Member Callback */\n", rtype, argtypes);
  printf ("  T      *o;\n");
  printf ("  MCb     f;\n");
  if (withreturn)
    printf ("  Arg<R>  r;\n");
  if (n_args)
    {
      printf (" ");
      for (int i = 1; i <= n_args; i++)
        printf (" Arg<A%u> a%u;", i, i);
      printf ("\n");
    }
  printf ("public:\n");
  printf ("  %s%uClosure (T *t, MCb _f) : o(t), f(_f) {\n", rpref, n_args);
  printf ("    sig_tokens = ");
  if (withreturn)
    printf ("r.token() + ");
  printf ("\"|\"");
  for (int i = 1; i <= n_args; i++)
    printf (" + a%u.token()", i);
  printf (";\n");
  printf ("  }\n");
  printf ("  void operator() (Value       *return_value,\n");
  printf ("                   const Value *param_values,\n");
  printf ("                   gpointer     invocation_hint,\n");
  printf ("                   gpointer     marshal_data) {\n");
  printf ("    ");
  if (withreturn)
    printf ("r.set (return_value, ");
  printf ("(o->*f) (");
  if (n_args)
    printf ("a1.get (param_values + 0)");
  for (int i = 2; i <= n_args; i++)
    printf (", a%u.get (param_values + %u)", i, i - 1);
  printf (")");
  if (withreturn)
    printf (")");
  printf (";\n");
  printf ("  }\n");
  printf ("};\n");
  /* function template */
  printf ("template<%s>\n", tmpldecl);
  printf ("%s%uClosure<%s>* Closure (T *t, %s (T::*f) (%s)) {\n", rpref, n_args, tmplargs, rtype, argtypes);
  printf ("  return new %s%uClosure<%s> (t, f);\n", rpref, n_args, tmplargs);
  printf ("}\n");
}

int
main (int argc,
      char *argv[])
{
  int n_closure_args = 8;
  for (int i = 0; i <= n_closure_args; i++)
    print_closure (false, i);
  for (int i = 0; i <= n_closure_args; i++)
    print_closure (true, i);
  return 0;
}
#endif
