// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
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
