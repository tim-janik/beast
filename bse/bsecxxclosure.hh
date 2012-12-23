/* BSE - Better Sound Engine
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
#ifndef __BSE_CXX_CLOSURE_H__
#define __BSE_CXX_CLOSURE_H__

#include <bse/bsecxxvalue.hh>
#include <bse/bsecxxarg.hh>

namespace Bse {

class CxxClosure {
  GClosure             *glib_closure;
  CxxClosure&           operator=       (const CxxClosure &c);
  explicit              CxxClosure      (const CxxClosure &c);
protected:
  String                sig_tokens;
  virtual void          operator()      (Value            *return_value,
                                         const Value      *param_values,
                                         gpointer          invocation_hint,
                                         gpointer          marshal_data) = 0;
public:
  explicit              CxxClosure      ();
  virtual               ~CxxClosure     ();
  GClosure*             gclosure        ();
  const String          signature       () { return sig_tokens; }
};

/* include generated CxxClosure* Closure (class T*, ... (T::*f) (...)); constructors */
#include <bse/bsegenclosures.hh>

} // Bse

#endif /* __BSE_CXX_CLOSURE_H__ */
