/* BSE - Bedevilled Sound Engine                        -*-mode: c++;-*-
 * Copyright (C) 2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BSE_EXAMPLE_H__
#define __BSE_EXAMPLE_H__

#include <bse/bsecxxbase.h>

namespace Bse {

#define BSE_TYPE_EXAMPLE        (Example::get_type ())

class Example : CxxBase {
  SfiInt n1;
  SfiReal n2;
public:
  /*Con*/       Example();
  void          set_property      (guint          prop_id,
                                   const Value   &value,
                                   GParamSpec    *pspec);
  void          get_property      (guint          prop_id,
                                   Value         &value,
                                   GParamSpec    *pspec);
  virtual       ~Example();
  
  static void  class_init (CxxBaseClass *klass);
  static GType get_type   (); // needed by BSE_CXX_TYPE_REGISTER()
};

class Derive2 : Example {
  enum { PROP_LABEL = 1, PROP_TOGGLE, PROP_CHOICE };
  std::string label;
  int enu;
  bool   toggle;
public:
  /*Con*/       Derive2();
  void          set_property      (guint          prop_id,
                                   const Value   &value,
                                   GParamSpec    *pspec);
  void          get_property      (guint          prop_id,
                                   Value         &value,
                                   GParamSpec    *pspec);
  virtual       ~Derive2();
  
  static void  class_init (CxxBaseClass *klass);
  static GType get_type   (); // needed by BSE_CXX_TYPE_REGISTER()
};

} // Bse

#endif /* __BSE_EXAMPLE_H__ */
