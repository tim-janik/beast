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
#ifndef __BSE_CXX_BASE_H__
#define __BSE_CXX_BASE_H__

#include <bse/bsecxxutils.h>
#include <bse/bsesource.h>
#include <string>
#include <stdexcept>

namespace Bse {
#define BSE_CXX_INSTANCE_OFFSET    BSE_CXX_SIZEOF (BseSource)

#define BSE_TYPE_CXX_BASE        (CxxBase::get_type ())

struct Value : GValue {
  bool                  get_bool    () const { return get_num(); }
  SfiInt                get_int     () const { return get_num(); }
  SfiInt                get_enum    () const { return get_num(); }
  SfiNum                get_num     () const;
  SfiReal               get_real    () const;
  const SfiString       get_string  () const;
  const SfiString       get_choice  () const { return get_string(); }
  GValue*               gvalue  () const { return (GValue*) this; }
  void set_bool    (bool             b) { set_num (b); }
  void set_int     (SfiInt           i) { set_num (i); }
  void set_enum    (SfiInt           e) { set_num (e); }
  void set_num     (SfiNum           n);
  void set_real    (SfiReal          r);
  void set_string  (const char      *s);
  void set_choice  (const char      *c) { set_string (c); }
  void operator= (bool               b) { set_bool (b); }
  void operator= (SfiInt             i) { set_int (i); }
  void operator= (SfiNum             n) { set_num (n); }
  void operator= (SfiReal            r) { set_real (r); }
  void operator= (const std::string &s) { set_string (s.c_str()); }
};

class CxxBaseClass : public BseSourceClass {
public:
  void add (const char *group,
            guint       prop_id,
            GParamSpec *pspec)
  {
    bse_object_class_add_param ((BseObjectClass*) this, group, prop_id, pspec);
  }
};
class CxxBase {
  void*           cast_to_gobject   ();
  static CxxBase* cast_from_gobject (void *o);
protected:
  GObject*        gobject           ();
  BseItem*        item              ();
public:
  /*Con*/         CxxBase           ();
  CxxBase*        ref               ();
  void            unref             ();
  void            freeze_notify     ();
  void            notify            (const gchar   *property);
  void            thaw_notify       ();
  void            set               (const gchar   *first_property_name,
                                     ...);
  void            get               (const gchar   *first_property_name,
                                     ...);
  void            set_property      (guint          prop_id,
                                     const Value   &value,
                                     GParamSpec    *pspec);
  void            get_property      (guint          prop_id,
                                     Value         &value,
                                     GParamSpec    *pspec);
  virtual         ~CxxBase          ();

  static void     class_init        (CxxBaseClass *klass);
  static GType    get_type          (); // needed by BSE_CXX_TYPE_REGISTER()
  
  class Pointer {
    CxxBase *p;
  public:
    Pointer (CxxBase *t) { p = t; }
    /* second part of to-GObject* casts: */
    operator GObject*   () { return (GObject*)   p->cast_to_gobject (); }
    operator BseObject* () { return (BseObject*) p->cast_to_gobject (); }
    operator BseItem*   () { return (BseItem*)   p->cast_to_gobject (); }
    operator BseSource* () { return (BseSource*) p->cast_to_gobject (); }
  };
  /* from-GObject* casts: */
  static CxxBase* cast (GObject   *o) { return cast_from_gobject (o); }
  static CxxBase* cast (BseSource *o) { return cast_from_gobject (o); }
  static CxxBase* cast (BseItem   *o) { return cast_from_gobject (o); }
  static CxxBase* cast (BseObject *o) { return cast_from_gobject (o); }
};
/* first part of to-GObject* casts: */
static inline CxxBase::Pointer cast (CxxBase *c) { return CxxBase::Pointer (c); }
/* match from-GObject* casts: */
template<class T> CxxBase*     cast (T *t)       { return CxxBase::cast (t); }

} // Bse


#endif /* __BSE_CXX_BASE_H__ */
