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
#include <sfi/sficxx.h>
#include <vector>
#include <algorithm>

namespace Bse {

/* --- Procedure namespace work arounds --- */
namespace Procedure {
typedef SfiBool     Bool;
typedef SfiInt      Int;
typedef SfiNum      Num;
typedef SfiTime     Time;
typedef SfiNote     Note;
typedef SfiReal     Real;
typedef SfiChoice   Choice;
typedef std::string String;  /* not using SfiString resp. gchar* here */
typedef SfiBBlock   BBlock;
typedef SfiFBlock   FBlock;
typedef SfiSeq      Seq;
typedef SfiRec      Rec;
typedef SfiProxy    Proxy;
};

/* --- type alias frequently used standard lib things --- */
typedef std::string String;


/* --- template errors --- */
namespace TEMPLATE_ERROR {
template<typename What, typename Reason> void invalid_type () { bool force_compiler_error = void (0); }
template<typename TYPE> void invalid_argument_type () { bool force_compiler_error = void (0); }
}

/* --- generally useful templates --- */
template<class Data> static void
delete_this (Data *d)
{
  delete d;
}
/* check derivation of Derived from Base */
template<class Derived, class Base>     // ex: EnforceDerivedFrom<Child, Base> assertion;
struct EnforceDerivedFrom {
  EnforceDerivedFrom (Derived *derived = 0,
                      Base    *base = 0)
  {
    base = derived;
  }
};
/* check derivation of Derived* from Base* */
template<class Derived, class Base>     // ex: EnforceDerivedFrom<Child*, Base*> assertion;
struct EnforceDerivedFrom<Derived*, Base*> {
  EnforceDerivedFrom (Derived *derived = 0,
                      Base    *base = 0)
  {
    base = derived;
  }
};
/* check derivation through EnforceDerivedFrom<>; */
template<class Derived, class Base> void
assert_derived_from (void)
{
  EnforceDerivedFrom<Derived, Base> assertion;
}


/* --- exceptions --- */
struct Exception : std::exception {
  explicit Exception (const char *_where) : loc (_where) {};
  virtual const char* where() { return loc; }
private:
  const char *loc;
};
struct InvalidArgument2 : Exception {
  const char *item;
  InvalidArgument2 (const char *where, const char *item) : Exception (where), item (item) {};
  const char* what() const throw() { return g_intern_strconcat ("invalid argument: ", item, NULL); }
};
#define InvalidArgument(WHAT)   InvalidArgument2 (G_STRFUNC, #WHAT)
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


/* --- binary lookups --- */
template<typename RandIter, class Cmp, typename Arg, int case_lookup_or_sibling_or_insertion>
static inline std::pair<RandIter,bool>
binary_lookup_fuzzy (RandIter  begin,
                     RandIter  end,
                     Cmp       cmp_elements,
                     const Arg &arg)
{
  RandIter current = end;
  ssize_t cmp = 0, n_elements = end - begin, offs = 0;
  const bool want_lookup = case_lookup_or_sibling_or_insertion == 0;
  //const bool want_sibling = case_lookup_or_sibling_or_insertion == 1;
  const bool want_insertion_pos = case_lookup_or_sibling_or_insertion > 1;
  while (offs < n_elements)
    {
      size_t i = (offs + n_elements) >> 1;
      current = begin + i;
      cmp = cmp_elements (arg, *current);
      if (cmp == 0)
        return want_insertion_pos ? std::make_pair (current, true) : std::make_pair (current, /*ignored*/false);
      else if (cmp < 0)
        n_elements = i;
      else /* (cmp > 0) */
        offs = i + 1;
    }
  /* check is last mismatch, cmp > 0 indicates greater key */
  return (want_lookup
          ? make_pair (end, /*ignored*/false)
          : (want_insertion_pos && cmp > 0)
          ? make_pair (current + 1, false)
          : make_pair (current, false));
}
template<typename RandIter, class Cmp, typename Arg>
static inline std::pair<RandIter,bool>
binary_lookup_insertion_pos (RandIter  begin,
                             RandIter  end,
                             Cmp       cmp_elements,
                             const Arg &arg)
{
  /* return (end,false) for end-begin==0, or return (position,true) for exact match,
   * otherwise return (position,false) where position indicates the location for
   * the key to be inserted (and may equal end).
   */
  return binary_lookup_fuzzy<RandIter,Cmp,Arg,2> (begin, end, cmp_elements, arg);
}
template<typename RandIter, class Cmp, typename Arg>
static inline RandIter
binary_lookup_sibling (RandIter  begin,
                       RandIter  end,
                       Cmp       cmp_elements,
                       const Arg &arg)
{
  /* return end for end-begin==0, otherwise return the exact match element, or,
   * if there's no such element, return the element last visited, which is pretty
   * close to an exact match (will be one off into either direction).
   */
  return binary_lookup_fuzzy<RandIter,Cmp,Arg,1> (begin, end, cmp_elements, arg).first;
}
template<typename RandIter, class Cmp, typename Arg>
static inline RandIter
binary_lookup (RandIter  begin,
               RandIter  end,
               Cmp       cmp_elements,
               const Arg &arg)
{
  /* return end or exact match */
  return binary_lookup_fuzzy<RandIter,Cmp,Arg,0> (begin, end, cmp_elements, arg).first;
}


/* --- records & sequences --- */
class Record {
  Record&          operator= (const Record&);
  explicit         Record    (const Record&);
public:
  explicit         Record    ();
  virtual SfiRec*  to_rec    ();
  virtual         ~Record    ();
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
  TypeRegistry (guint             instance_size,
                const gchar      *name,
                const gchar      *parent,
                const ClassInfo  *cinfo,
                GBaseInitFunc     binit,
                void            (*class_init) (CxxBaseClass*),
                GInstanceInitFunc iinit,
                Flags             flags);
  const GType
  get_type () const
  {
    return gtype_id;
  }
  static void
  init_types ();
};

template<class C> const GType
bse_type_id_wrapper (const char *type_name)
{
  static GType type = 0;
  if (!type)
    {
      type = g_type_from_name (type_name);
      g_assert (type);
    }
  return type;
}

#define BSE_CXX_TYPE_GET_REGISTERED(NameSpace, ObjectType) \
  (::Bse::bse_type_id_wrapper<ObjectType> (#NameSpace #ObjectType))
#define BSE_CXX_TYPE_REGISTER_INITIALIZED(ObjectType, parent, cinfo, binit, flags) \
  BSE_CXX_TYPE_REGISTER_INTERN (ObjectType, parent, cinfo, binit,                  \
                                ::Bse::cxx_instance_init_trampoline<ObjectType>, flags)
#define BSE_CXX_TYPE_REGISTER_INTERN(ObjectType, parent, cinfo, binit, iinit, flags) \
  static Bse::TypeRegistry                                                      \
    ObjectType ## _type_keeper (sizeof (ObjectType), "Bse" #ObjectType, parent, \
                                cinfo, binit,                                   \
                                ::Bse::cxx_class_init_trampoline<ObjectType>,   \
                                iinit, flags);
#define BSE_CXX_UTILS_ALIGNMENT         (2 * sizeof (gsize))
#define BSE_CXX_UTILS_ALIGN(offset)     ((offset + BSE_CXX_UTILS_ALIGNMENT - 1) & -BSE_CXX_UTILS_ALIGNMENT)
#define BSE_CXX_SIZEOF(Class)           BSE_CXX_UTILS_ALIGN (sizeof (Class))
#define BSE_CXX_COMMON_CLASS_SIZE       sizeof (CxxBaseClass)

} // Bse

#endif /* __BSE_CXX_UTILS_H__ */
