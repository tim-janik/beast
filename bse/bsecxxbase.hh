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
#ifndef __BSE_CXX_BASE_H__
#define __BSE_CXX_BASE_H__

#include <bse/bsesource.hh>
#include <bse/bsecxxvalue.hh>
#include <bse/bsecxxclosure.hh>

namespace Bse {
#define BSE_CXX_INSTANCE_OFFSET    BSE_CXX_SIZEOF (BseSource)

#define BSE_TYPE_CXX_BASE        (BSE_CXX_TYPE_GET_REGISTERED (Bse, CxxBase))

class CxxBaseClass : public BseSourceClass {
public:
  void  add_param     (const char  *group,
                       guint        prop_id,
                       GParamSpec  *pspec);
  void  add_param     (guint        prop_id,
                       GParamSpec  *grouped_pspec);
  void  set_accessors (void       (*get_property)      (GObject*,   guint,       GValue*,          GParamSpec*),
                       void       (*set_property)      (GObject*,   guint, const GValue*,          GParamSpec*) = NULL,
                       gboolean   (*editable_property) (BseObject*, guint,                         GParamSpec*) = NULL,
                       void       (*get_candidates)    (BseItem*,   guint, BsePropertyCandidates*, GParamSpec*) = NULL,
                       void       (*property_updated)  (BseSource*, guint, guint64, double,        GParamSpec*) = NULL);
  guint add_signal    (const gchar *signal_name,
                       GSignalFlags flags,
                       guint        n_params,
                       ...);
  void  add_ochannel  (const char  *ident,
                       const char  *label,
                       const char  *blurb,
                       int          assert_id = -1);
  void  add_ichannel  (const char  *ident,
                       const char  *label,
                       const char  *blurb,
                       int          assert_id = -1);
  void  add_jchannel  (const char  *ident,
                       const char  *label,
                       const char  *blurb,
                       int          assert_id = -1);
};
class CxxBase {
  void*           cast_to_gobject   ();
  static CxxBase* cast_from_gobject (void *o);
public:
  static CxxBase* base_from_gobject (GObject *o) { return cast_from_gobject (o); }
protected:
  GObject*        gobject           () const;
  BseItem*        item              ();
public:
  /*Con*/         CxxBase           ();
  CxxBase*        ref               ();
  void            unref             ();
  void            freeze_notify     ();
  void            notify            (const gchar   *property);
  void            thaw_notify       ();
  void            set               (const gchar   *first_property_name,
                                     ...) G_GNUC_NULL_TERMINATED;
  void            get               (const gchar   *first_property_name,
                                     ...) G_GNUC_NULL_TERMINATED;
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
  virtual void    restore_finished  (guint          vmajor,
                                     guint          vminor,
                                     guint          vmicro);
  virtual         ~CxxBase          ();

  static void     class_init        (CxxBaseClass *klass);

  static inline bool instance_is_a  (CxxBase       *cbase,
                                     GType          iface_type)
  {
    if (cbase)
      {
        GObject *gobject = cbase->gobject();
        return G_TYPE_CHECK_INSTANCE_TYPE (gobject, iface_type);
      }
    else
      return FALSE;
  }

  template<class OType> static inline OType*
  value_get_gobject (const GValue *v)
  {
    gpointer p;
    if (SFI_VALUE_HOLDS_PROXY (v))
      p = bse_object_from_id (sfi_value_get_proxy (v));
    else
      p = g_value_get_object (v);
    return (OType*) p;
  }
  template<class CxxType> static inline CxxType
  value_get_object (const GValue *v)
  {
    assert_derived_from<CxxType, CxxBase*>();
    GObject *p = value_get_gobject<GObject> (v);
    CxxBase *b = CxxBase::base_from_gobject (p);
    CxxType to = static_cast<CxxType> (b);
    return to;
  }
  static inline void
  value_set_gobject (GValue  *value,
                     gpointer object)
  {
    if (SFI_VALUE_HOLDS_PROXY (value))
      sfi_value_set_proxy (value, BSE_IS_OBJECT (object) ? ((BseObject*) object)->unique_id : 0);
    else
      g_value_set_object (value, object);
  }
  static inline void
  value_set_object (GValue        *value,
                    const CxxBase *self)
  {
    value_set_gobject (value, self->gobject());
  }
  template<class Accepted, class Casted>
  static inline void
  value_set_casted (GValue         *value,
                    const Accepted *obj)
  {
    const Casted *self = static_cast<const Casted*> (obj);
    value_set_object (value, self);
  }

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

/* --- trampoline templates --- */
template<class ObjectType> static void
cxx_class_init_trampoline (CxxBaseClass *klass)
{
  ObjectType::class_init (klass);
}

template<class ObjectType> static void
cxx_instance_init_trampoline (GTypeInstance *instance,
                              gpointer       g_class)
{ /* invoke C++ constructor upon _init of destination type */
  if (G_TYPE_FROM_INSTANCE (instance) == G_TYPE_FROM_CLASS (g_class))
    new (BSE_CXX_INSTANCE_OFFSET + (char*) instance) ObjectType ();
}

template<class ObjectType, typename PropertyID> static void
cxx_get_property_trampoline (GObject    *o,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  CxxBase *cbase = cast (o);
  Value *v = (Value*) value;
  ObjectType *instance = static_cast<ObjectType*> (cbase);
  if (0)        // check ObjectType::get_property() member and prototype
    (void) static_cast<void (ObjectType::*) (PropertyID, Value&, GParamSpec*)> (&ObjectType::get_property);
  instance->get_property (static_cast<PropertyID> (prop_id), *v, pspec);
}

template<class ObjectType, typename PropertyID> static void
cxx_set_property_trampoline (GObject      *o,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  CxxBase *cbase = cast (o);
  const Value *v = (const Value*) value;
  ObjectType *instance = static_cast<ObjectType*> (cbase);
  if (0)        // check ObjectType::set_property() member and prototype
    (void) static_cast<void (ObjectType::*) (PropertyID, const Value&, GParamSpec*)> (&ObjectType::set_property);
  instance->set_property (static_cast<PropertyID> (prop_id), *v, pspec);
}

template<class ObjectType, typename PropertyID> static gboolean
cxx_editable_property_trampoline (BseObject    *o,
                                  guint         prop_id,
                                  GParamSpec   *pspec)
{
  CxxBase *cbase = cast (o);
  ObjectType *instance = static_cast<ObjectType*> (cbase);
  if (0)        // check ObjectType::editable_property() member and prototype
    (void) static_cast<bool (ObjectType::*) (PropertyID, GParamSpec*)> (&ObjectType::editable_property);
  return instance->editable_property (static_cast<PropertyID> (prop_id), pspec);
}

template<class ObjectType, typename PropertyID> static void
cxx_get_candidates_trampoline (BseItem               *item,
                               guint                  prop_id,
                               BsePropertyCandidates *pc,
                               GParamSpec            *pspec);   /* defined in bsecxxplugin.hh */

template<class ObjectType, typename PropertyID> static void
cxx_property_updated_trampoline (BseSource             *source,
                                 guint                  prop_id,
                                 guint64                tick_stamp,
                                 double                 prop_value,
                                 GParamSpec            *pspec)
{
  CxxBase *cbase = cast (source);
  ObjectType *instance = static_cast<ObjectType*> (cbase);
  if (0)        // check ObjectType::property_updated() member and prototype
    (void) static_cast<void (ObjectType::*) (PropertyID, guint64, double, GParamSpec*)> (&ObjectType::property_updated);
  instance->property_updated (static_cast<PropertyID> (prop_id), tick_stamp, prop_value, pspec);
}

} // Bse


#endif /* __BSE_CXX_BASE_H__ */
