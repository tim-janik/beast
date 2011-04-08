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
#ifndef __BSE_CXX_VALUE_H__
#define __BSE_CXX_VALUE_H__

#include <bse/bsecxxutils.hh>

namespace Bse {

class CxxBase; // prototype CxxBase since we deal with pointers thereof

/* Generic Value keeping, basically a convenient wrapper around GValue */

struct Value : GValue {
  bool                  get_bool    () const { return get_num(); }
  SfiInt                get_int     () const { return get_num(); }
  SfiInt                get_enum    () const { return get_num(); }
  SfiNum                get_num     () const;
  SfiReal               get_real    () const;
  const SfiString       get_string  () const;
  const SfiString       get_choice  () const { return get_string(); }
  gpointer              get_pointer () const;
  CxxBase*              get_base    () const;
  GObject*              get_object  () const;
  GParamSpec*           get_pspec   () const;
  GValue*               gvalue  () const { return (GValue*) this; }
  void set_bool    (bool          b) { set_num (b); }
  void set_int     (SfiInt        i) { set_num (i); }
  void set_enum    (SfiInt        e) { set_num (e); }
  void set_num     (SfiNum        n);
  void set_real    (SfiReal       r);
  void set_string  (const char   *s);
  void set_string  (const String &s) { set_string (s.c_str()); }
  void set_choice  (const char   *c) { set_string (c); }
  void set_pointer (gpointer      p);
  void set_base    (CxxBase      *b);
  void set_object  (GObject      *o);
  void set_pspec   (GParamSpec   *p);
  void operator=   (bool          b) { set_bool (b); }
  void operator=   (SfiInt        i) { set_int (i); }
  void operator=   (SfiNum        n) { set_num (n); }
  void operator=   (SfiReal       r) { set_real (r); }
  void operator=   (const String &s) { set_string (s.c_str()); }
};

} // Bse


#endif /* __BSE_CXX_VALUE_H__ */
