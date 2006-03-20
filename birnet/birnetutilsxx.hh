/* Birnet
 * Copyright (C) 2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __BIRNET_UTILS_XX_HH__
#define __BIRNET_UTILS_XX_HH__

#include <birnet/birnetcore.h>
#include <birnet/birnetmsg.h> // FIXME
#include <string>
#include <vector>
#include <map>

namespace Birnet {

/* --- convenient type shorthands --- */
typedef BirnetUInt		uint;
typedef BirnetUInt8		uint8;
typedef BirnetUInt16		uint16;
typedef BirnetUInt32		uint32;
typedef BirnetUInt64		uint64;
typedef BirnetInt8		int8;
typedef BirnetInt16		int16;
typedef BirnetInt32		int32;
typedef BirnetInt64		int64;
typedef BirnetUniChar		unichar;

/* --- convenient stdc++ types --- */
typedef std::string String;
using std::vector;
using std::map;       

/* --- private copy constructor and assignment operator --- */
#define BIRNET_PRIVATE_CLASS_COPY(Class)        private: Class (const Class&); Class& operator= (const Class&);

/* --- birnet_init() hook --- */
class InitHook {
  typedef void (*InitHookFunc)  (void);
  InitHook    *next;
  int          priority;
  InitHookFunc hook;
  BIRNET_PRIVATE_CLASS_COPY (InitHook);
  static void  invoke_hooks (void);
public:
  explicit InitHook (InitHookFunc       _func,
                     int                _priority = 0);
};

/* --- Deletable --- */
class Deletable {
protected:
  virtual ~Deletable() {}
};

/* --- ReferenceCountImpl --- */
class ReferenceCountImpl : public virtual Deletable
{
  volatile mutable uint32 ref_field;
  static const uint32     FLOATING_FLAG = 1 << 31;
  inline bool
  ref_cas (uint32       oldv,
           uint32       newv) const
  {
#if GLIB_CHECK_VERSION (2, 9, 1)
    return g_atomic_int_compare_and_exchange ((volatile int*) &ref_field, oldv, newv);
#else
    return g_atomic_int_compare_and_exchange ((int*) &ref_field, oldv, newv);
#endif
  }
  inline uint32
  ref_get() const
  {
    return g_atomic_int_get ((volatile int*) &ref_field);
  }
protected:
  inline uint32
  ref_count() const
  {
    return ref_get() & ~FLOATING_FLAG;
  }
public:
  ReferenceCountImpl() :
    ref_field (FLOATING_FLAG + 1)
  {}
  bool
  floating() const
  {
    return 0 != (ref_get() & FLOATING_FLAG);
  }
  void
  ref() const
  {
    BIRNET_ASSERT (ref_count() > 0);
    uint32 old_ref, new_ref;
    do {
      old_ref = ref_get();
      new_ref = old_ref + 1;
      BIRNET_ASSERT (new_ref & ~FLOATING_FLAG); /* catch overflow */
    } while (!ref_cas (old_ref, new_ref));
  }
  void
  ref_sink() const
  {
    BIRNET_ASSERT (ref_count() > 0);
    ref();
    uint32 old_ref, new_ref;
    do {
      old_ref = ref_get();
      new_ref = old_ref & ~FLOATING_FLAG;
    } while (!ref_cas (old_ref, new_ref));
    if (old_ref & FLOATING_FLAG)
      unref();
  }
  bool
  finalizing() const
  {
    return ref_count() < 1;
  }
  void
  unref() const
  {
    BIRNET_ASSERT (ref_count() > 0);
    uint32 old_ref, new_ref;
    do {
      old_ref = ref_get();
      BIRNET_ASSERT (old_ref & ~FLOATING_FLAG); /* catch underflow */
      new_ref = old_ref - 1;
    } while (!ref_cas (old_ref, new_ref));
    if (0 == (new_ref & ~FLOATING_FLAG))
      {
        ReferenceCountImpl *self = const_cast<ReferenceCountImpl*> (this);
        self->finalize();
        self->delete_this(); // effectively: delete this;
      }
  }
  void
  ref_diag (const char *msg = NULL) const
  {
    birnet_diag ("%s: this=%p ref_count=%d floating=%d", msg ? msg : "ReferenceCountImpl", this, ref_count(), floating()); // FIXME: use diag()
  }
  template<class Obj> static Obj& ref      (Obj &obj) { obj.ref();       return obj; }
  template<class Obj> static Obj* ref      (Obj *obj) { obj->ref();      return obj; }
  template<class Obj> static Obj& ref_sink (Obj &obj) { obj.ref_sink();  return obj; }
  template<class Obj> static Obj* ref_sink (Obj *obj) { obj->ref_sink(); return obj; }
  template<class Obj> static void unref    (Obj &obj) { obj.unref(); }
  template<class Obj> static void unref    (Obj *obj) { obj->unref(); }
  template<class Obj> static void sink     (Obj &obj) { obj.ref_sink(); obj.unref(); }
  template<class Obj> static void sink     (Obj *obj) { obj->ref_sink(); obj->unref(); }
protected:
  virtual void
  finalize()
  {}
  virtual void
  delete_this()
  {
    delete this;
  }
  virtual
  ~ReferenceCountImpl()
  {
    BIRNET_ASSERT (ref_count() == 0);
  }
};
template<class Obj> static Obj& ref      (Obj &obj) { obj.ref();       return obj; }
template<class Obj> static Obj* ref      (Obj *obj) { obj->ref();      return obj; }
template<class Obj> static Obj& ref_sink (Obj &obj) { obj.ref_sink();  return obj; }
template<class Obj> static Obj* ref_sink (Obj *obj) { obj->ref_sink(); return obj; }
template<class Obj> static void unref    (Obj &obj) { obj.unref(); }
template<class Obj> static void unref    (Obj *obj) { obj->unref(); }
template<class Obj> static void sink     (Obj &obj) { obj.ref_sink(); obj.unref(); }
template<class Obj> static void sink     (Obj *obj) { obj->ref_sink(); obj->unref(); }

} // Birnet
#endif /* __BIRNET_UTILS_XX_HH__ */
/* vim:set ts=8 sts=2 sw=2: */
