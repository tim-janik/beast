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
#ifndef __BSE_CXX_UTILS_H__
#define __BSE_CXX_UTILS_H__

#include <bse/bseutils.h>
#include <string>
#include <new>

namespace Bse {

/* --- type alias frequently used standard lib things --- */
typedef std::string String;


/* --- template errors --- */
namespace TEMPLATE_ERROR {
template<typename What, typename Reason> void invalid_type () { TEMPLATE_ERROR::abort; }
template<typename TYPE> void invalid_argument_type () { TEMPLATE_ERROR::abort; }
}

/* --- generally usefull templates --- */
template<class Data> static void
delete_this (Data *d)
{
  delete d;
}
template<class Derived, class Parent> inline void
assert_derivation()
{
  register Derived *_derived = 0;
  register Parent *_parent;
  /* this generates a compiler error if Derived is not derived from Parent */
  _parent = _derived;
}


/* --- exceptions --- */
struct Exception : std::exception {
  explicit Exception (const char *_where) : loc (_where) {};
  virtual const char* where() { return loc; }
private:
  const char *loc;
};
struct InvalidArgument : Exception {
  InvalidArgument (const char *where) : Exception (where) {};
  const char* what() const throw() { return "invalid argument"; }
};
struct WrongTypeGValue : Exception {
  WrongTypeGValue (const char *where) : Exception (where) {};
  const char* what() const throw() { return "GValue contains wrong type for this kind of use"; }
};
struct DontReach : Exception {
  DontReach (const char *where) : Exception (where) {};
  const char* what() const throw() { return "Code section should not be reached"; }
};
struct InvalidConnection : Exception {
  InvalidConnection (const char *where) : Exception (where) {};
  const char* what() const throw() { return "Function to be connected has invalid signature"; }
};

/* --- class registration --- */
#define BSE_CXX_TYPE_REGISTER(ObjectType, parent, class_info)          \
          BSE_CXX_TYPE_REGISTER_INITIALIZED (ObjectType, parent, class_info, NULL, TypeRegistry::NONE)
#define BSE_CXX_TYPE_REGISTER_ABSTRACT(ObjectType, parent, class_info) \
          BSE_CXX_TYPE_REGISTER_INTERN (ObjectType, parent, class_info, NULL, NULL, TypeRegistry::ABSTRACT)

/* --- class information --- */
struct ClassInfo
{
  const char *category;
  const char *blurb;
  ClassInfo (const char *category,
             const char *blurb)
  {
    this->category = category;
    this->blurb = blurb;
  }
};


/* --- type registration internals --- */
struct CxxBaseClass;
class TypeRegistry
{
  GType gtype_id;
public:
  enum Flags {
    NONE        = 0,
    ABSTRACT    = G_TYPE_FLAG_ABSTRACT
  };
  /*Con*/       TypeRegistry (guint             instance_size,
                              const gchar      *name,
                              const gchar      *parent,
                              const ClassInfo  *cinfo,
                              GBaseInitFunc     binit,
                              void            (*class_init) (CxxBaseClass*),
                              GInstanceInitFunc iinit,
                              Flags             flags);
  GType         get_type ()
  {
    return gtype_id;
  }
  static void   init_types ();
};

#define BSE_CXX_TYPE_REGISTER_INITIALIZED(ObjectType, parent, cinfo, binit, flags) \
  BSE_CXX_DEFINE_INSTANCE_INIT (ObjectType);                                       \
  BSE_CXX_TYPE_REGISTER_INTERN (ObjectType, parent, cinfo, binit,                  \
                                BSE_CXX_SYM(ObjectType,instance_init), flags)
#define BSE_CXX_TYPE_REGISTER_INTERN(ObjectType, parent, cinfo, binit, iinit, flags) \
  BSE_CXX_DEFINE_SET_PROPERTY (ObjectType);                                     \
  BSE_CXX_DEFINE_GET_PROPERTY (ObjectType);                                     \
  BSE_CXX_DEFINE_CLASS_INIT (ObjectType, BSE_CXX_SYM(ObjectType,set_property),  \
                                        BSE_CXX_SYM(ObjectType,get_property));  \
  static Bse::TypeRegistry                                                      \
    ObjectType ## _type_keeper (sizeof (ObjectType), "Bse" #ObjectType, parent, \
                                cinfo, binit,                                   \
                                BSE_CXX_SYM(ObjectType,class_init),             \
                                iinit, flags);                                  \
  GType ObjectType::get_type ()                                                 \
  {                                                                             \
    return ObjectType ## _type_keeper . get_type ();                            \
  }
#define BSE_CXX_DEFINE_INSTANCE_INIT(ObjectType)                                \
  static void                                                                   \
  BSE_CXX_SYM(ObjectType,instance_init) (GTypeInstance *instance,               \
                                         gpointer       g_class)                \
  { /* invoke constructor upon _init of destination type */                     \
    if (G_TYPE_FROM_INSTANCE (instance) == G_TYPE_FROM_CLASS (g_class))         \
      new (BSE_CXX_INSTANCE_OFFSET + (char*) instance) ObjectType ();           \
  }
#define BSE_CXX_DEFINE_SET_PROPERTY(ObjectType)                                 \
  static void BSE_CXX_SYM(ObjectType,set_property) (GObject *o, guint prop_id,  \
                                                    const GValue *value,        \
                                                    GParamSpec *pspec)          \
  {                                                                             \
    CxxBase *cbase = cast (o);                                                  \
    const Bse::Value *v = (const Bse::Value*) value;                            \
    static_cast<ObjectType*> (cbase)->set_property (prop_id, *v, pspec);        \
  }
#define BSE_CXX_DEFINE_GET_PROPERTY(ObjectType)                                 \
  static void BSE_CXX_SYM(ObjectType,get_property) (GObject *o, guint prop_id,  \
                                                    GValue *value,              \
                                                    GParamSpec *pspec)          \
  {                                                                             \
    CxxBase *cbase = cast (o);  Bse::Value *v = (Bse::Value*) value;            \
    static_cast<ObjectType*> (cbase)->get_property (prop_id, *v, pspec);        \
  }
#define BSE_CXX_DEFINE_CLASS_INIT(ObjectType, set_prop, get_prop)               \
  static void BSE_CXX_SYM(ObjectType,class_init) (CxxBaseClass *klass)          \
  {                                                                             \
    GObjectClass *gobject_class = (GObjectClass*) klass;                        \
    gobject_class->set_property = set_prop;                                     \
    gobject_class->get_property = get_prop;                                     \
    ObjectType::class_init (klass);                                             \
  }
#define BSE_CXX_SYM(Type, func)         bse_cxx__ ## Type ## __ ## func
#define BSE_CXX_UTILS_ALIGNMENT         (2 * sizeof (gsize))
#define BSE_CXX_UTILS_ALIGN(offset)     ((offset + BSE_CXX_UTILS_ALIGNMENT - 1) & -BSE_CXX_UTILS_ALIGNMENT)
#define BSE_CXX_SIZEOF(Class)           BSE_CXX_UTILS_ALIGN (sizeof (Class))
#define BSE_CXX_COMMON_CLASS_SIZE       sizeof (CxxBaseClass)

} // Bse

#endif /* __BSE_CXX_UTILS_H__ */
