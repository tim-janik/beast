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
#ifndef __BSE_EFFECT_H__
#define __BSE_EFFECT_H__

#include <bse/bsecxxbase.h>
#include <bse/gslieee754.h>

namespace Bse {

/* enums/structures mirrored from gslengine.h */
enum ProcessCost {
  NORMAL,
  CHEAP,
  EXPENSIVE
};
struct JStream {
  const float **values;
  unsigned int  n_connections;
private:
  unsigned int  jcount; /* reserved */
  unsigned int  reserved : 16;
};
struct IStream {
  const float  *values;
private:
  unsigned int  reserved : 16;
public:
  unsigned int  connected : 1;
};
struct OStream {
  float        *values;
private:
  unsigned int  sub_sample_pattern : 16;
public:
  unsigned int  connected : 1;
};

class SynthesisModule {
  template<class T, typename P> class AccessorP1; /* 1-argument member function closure */
  void          *engine_module;
  const IStream *istreams;
  const JStream *jstreams;
  const OStream *ostreams;
public:
  explicit                  SynthesisModule ();
  virtual                  ~SynthesisModule () = 0;
  virtual void              reset           () = 0;
  virtual void              process         (unsigned int n_values) = 0;
  virtual const ProcessCost cost            ();
  inline const IStream&     istream         (unsigned int istream_index) const;
  inline const JStream&     jstream         (unsigned int jstream_index) const;
  inline const OStream&     ostream         (unsigned int ostream_index) const;
  inline const unsigned int mix_freq        () const;
  static inline int         dtoi            (double d) { return gsl_dtoi (d); }
  static inline int         ftoi            (float  f) { return gsl_ftoi (f); }
  /* member function closure base */
  struct Accessor {
    virtual void            operator()      (SynthesisModule*) = 0;
    virtual                ~Accessor        ()         {}
  };
  /* create a 1-argument member function closure, where C must be derived from SynthesisModule */
  template<class D, class C>
  static Accessor*          accessor        (void    (C::*accessor) (D*),
                                             const D     &data);
  /* internal */
  void                      set_module      (void*);
};

class Effect : public CxxBase {
public:
  explicit                  Effect              ();
  void                      set_property        (guint          prop_id,
                                                 const Value   &value,
                                                 GParamSpec    *pspec);
  void                      get_property        (guint          prop_id,
                                                 Value         &value,
                                                 GParamSpec    *pspec);
  virtual SynthesisModule*  create_module       (unsigned int   context_handle,
                                                 GslTrans      *trans) = 0;
  virtual SynthesisModule::Accessor*
                            module_configurator () = 0;
  void                      update_modules      (GslTrans      *trans = NULL);
  
  static void               class_init          (CxxBaseClass *klass);
};


/* --- implementation details --- */
extern "C" { extern guint gsl_externvar_sample_freq; }
inline const unsigned int
SynthesisModule::mix_freq () const {
  return gsl_externvar_sample_freq;
}
inline const IStream&
SynthesisModule::istream (unsigned int istream_index) const
{
  return istreams[istream_index];
}
inline const JStream&
SynthesisModule::jstream (unsigned int jstream_index) const
{
  return jstreams[jstream_index];
}
inline const OStream&
SynthesisModule::ostream (unsigned int ostream_index) const
{
  return ostreams[ostream_index];
}
template<class T, typename P>
class SynthesisModule::AccessorP1 : public SynthesisModule::Accessor {
  typedef void (T::*Member) (P*);
  Member    func;
  P        *data;
public:
  AccessorP1 (void (T::*f) (P*), P *p)
    : func (f), data (p)
  {
    assert_derivation<T,SynthesisModule>();
  }
  void operator() (SynthesisModule *p)
  {
    T *t = static_cast<T*> (p);
    (t->*func) (data);
  }
  ~AccessorP1 ()
  {
    delete data;
  }
};
template<class D, class C> SynthesisModule::Accessor*
SynthesisModule::accessor (void   (C::*accessor) (D*),
                           const D    &data)
{
  D *d = new D (data);
  AccessorP1<C,D> *ac = new AccessorP1<C,D> (accessor, d);
  return ac;
}

} // Bse

#endif /* __BSE_EFFECT_H__ */
