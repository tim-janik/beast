// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_CXX_ARG_H__
#define __BSE_CXX_ARG_H__

#include <bse/bsecxxvalue.hh>

/* Closure Argument implementation. For a given type, these templates
 * provide a class Arg with get() and set() functions on class Value,
 * and a token() function which returns a single character string to
 * identify the argument type.
 */

namespace Bse {

/* default Arg type, this either supports a CxxBase* pointer or errors out */
template<typename T>
struct Arg {
  T            get   (const Value *v) { return (T) v->get_base(); }
  void         set   (Value *v, T  t) { v->set_base (t); }
  const String token ()               { void (*f) (T) = 0; return tokenize (f); }
private:
  template<typename U> static const String
  ptokenize (CxxBase*)
  {     // CxxBase* is a supported pointer type
    return "X";
  }
  template<typename U> static const String
  ptokenize (void const *)
  {     // other pointer types are not supported
    return "?";
    Birnet::TEMPLATE_ERROR::invalid_type<U*>();
  }
  template<typename U> const String
  tokenize (void (*) (U*))
  {     // relay to pointer type tokenizer
    U *p = 0;
    return ptokenize<U> (p);
  }
  template<typename U> const String
  tokenize (void (*) (U))
  {     // non-pointer type, not supported
    return "?";
    Birnet::TEMPLATE_ERROR::invalid_type<U>();
  }
};
const String tokenize_gtype (GType t);


/* specialize Arg template for standard primitive types */
#define BSE__SPECIALIZE(TYPE, vtype, tok, GCast, SCast) \
template<> struct Arg<TYPE> {                           \
  TYPE         get   (const Value *v)                   \
  { return GCast (v->get_##vtype ()); }                 \
  void         set   (Value *v, TYPE t)                 \
  { v->set_##vtype (SCast (t)); }                       \
  const String token ()                                 \
  { return tok; }                                       \
private:                                                \
  template<typename T> static inline T no_cast (T t)    \
  { return t; }                                         \
}
BSE__SPECIALIZE(bool,                 bool,   "b", no_cast, no_cast);
// BSE__SPECIALIZE(char,              int,    "i", no_cast, no_cast);
// BSE__SPECIALIZE(signed char,       int,    "i", no_cast, no_cast);
// BSE__SPECIALIZE(unsigned char,     int,    "i", no_cast, no_cast);
// BSE__SPECIALIZE(signed short,      int,    "i", no_cast, no_cast);
// BSE__SPECIALIZE(unsigned short,    int,    "i", no_cast, no_cast);
BSE__SPECIALIZE(signed int,           int,    "i", no_cast, no_cast);
BSE__SPECIALIZE(uint,                 int,    "i", no_cast, no_cast);
BSE__SPECIALIZE(signed long,          int,    "i", no_cast, no_cast);
BSE__SPECIALIZE(unsigned long,        int,    "i", no_cast, no_cast);
BSE__SPECIALIZE(signed long long,     num,    "n", no_cast, no_cast);
BSE__SPECIALIZE(unsigned long long,   num,    "n", no_cast, no_cast);
BSE__SPECIALIZE(float,                real,   "r", no_cast, no_cast);
BSE__SPECIALIZE(double,               real,   "r", no_cast, no_cast);
BSE__SPECIALIZE(gpointer,             pointer,"*", no_cast, no_cast);
BSE__SPECIALIZE(GParamSpec*,          pspec,  "P", no_cast, no_cast);
//BSE__SPECIALIZE(unsigned char*,       string, "s", no_cast, no_cast);
//BSE__SPECIALIZE(signed char*,         string, "s", no_cast, no_cast);
BSE__SPECIALIZE(String,               string, "s", no_cast, no_cast);
//BSE__SPECIALIZE(const unsigned char*, string, "s", no_cast, no_cast);
//BSE__SPECIALIZE(const signed char*,   string, "s", no_cast, no_cast);
//BSE__SPECIALIZE(const char*,          string, "s", no_cast, no_cast);
BSE__SPECIALIZE(const String,         string, "s", no_cast, no_cast);
BSE__SPECIALIZE(char*,                string, "s", const_cast<char*>, no_cast);
BSE__SPECIALIZE(GObject*,             object, "O", no_cast, no_cast);
BSE__SPECIALIZE(BseObject*,           object, "O", reinterpret_cast<BseObject*>, reinterpret_cast<GObject*>);
BSE__SPECIALIZE(BseItem*,             object, "O", reinterpret_cast<BseItem*>, reinterpret_cast<GObject*>);
BSE__SPECIALIZE(BseSource*,           object, "O", reinterpret_cast<BseSource*>, reinterpret_cast<GObject*>);
#undef BSE__SPECIALIZE

} // Bse


#endif /* __BSE_CXX_ARG_H__ */
