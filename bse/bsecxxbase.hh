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

#include <bse/bsesource.h>
#include <bse/bsecxxvalue.h>
#include <bse/bsecxxclosure.h>

namespace Bse {
#define BSE_CXX_INSTANCE_OFFSET    BSE_CXX_SIZEOF (BseSource)

#define BSE_TYPE_CXX_BASE        (BSE_CXX_TYPE_GET_REGISTERED (Bse, CxxBase))

class CxxBaseClass : public BseSourceClass {
public:
  void  add_param    (const char  *group,
                      guint        prop_id,
                      GParamSpec  *pspec);
  void  add_param    (guint        prop_id,
                      GParamSpec  *grouped_pspec);
  guint add_signal   (const gchar *signal_name,
                      GSignalFlags flags,
                      guint        n_params,
                      ...);
  void  add_ochannel (const char  *name,
                      const char  *blurb,
                      int          assert_id = -1);
  void  add_ichannel (const char  *name,
                      const char  *blurb,
                      int          assert_id = -1);
  void  add_jchannel (const char  *name,
                      const char  *blurb,
                      int          assert_id = -1);
};
class CxxBase {
  void*           cast_to_gobject   ();
  static CxxBase* cast_from_gobject (void *o);
public:
  static CxxBase* base_from_gobject (GObject *o) { return cast_from_gobject (o); }
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
#if 0
  gulong          connect           (const gchar   *signal,
                                     GClosure      *closure,
                                     bool           after);
  gulong          connect           (const gchar   *signal,
                                     GClosure      *closure) { return connect (signal, closure, false); }
#endif
  gulong          connect           (const gchar   *signal,
                                     CxxClosure    *closure,
                                     bool           after);
  gulong          connect           (const gchar   *signal,
                                     CxxClosure    *closure) { return connect (signal, closure, false); }
  const String    tokenize_signal   (const gchar   *signal);
  GType           type              ();
  virtual void    compat_setup      (guint          vmajor,
                                     guint          vminor,
                                     guint          vmicro);
  virtual         ~CxxBase          ();

  static void     class_init        (CxxBaseClass *klass);
  
  class Pointer {
    CxxBase *p;
  public:
    Pointer (CxxBase *t)       { p = t; }
    /* second part of to-GObject* casts: */
    operator GObject*   () { return (GObject*)   p->cast_to_gobject (); }
    operator BseObject* () { return (BseObject*) p->cast_to_gobject (); }
    operator BseItem*   () { return (BseItem*)   p->cast_to_gobject (); }
    operator BseSource* () { return (BseSource*) p->cast_to_gobject (); }
  };
  /* first part of to-GObject* casts: */
  static inline CxxBase::Pointer cast (CxxBase *c) { return CxxBase::Pointer (c); }
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
