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
#ifndef __BSE_CXX_MODULE_H__
#define __BSE_CXX_MODULE_H__

#include <bse/bsecxxbase.h>
#include <bse/gslieee754.h>

namespace Bse {

/* enums/structures mirrored from bseengine.h */
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
};
struct IStream {
  const float  *values;
  gboolean      connected;
};
struct OStream {
  float        *values;
  gboolean      connected;
};

class SynthesisModule {
  template<class T, typename P> class AccessorP1; /* 1-argument member function closure */
  BseModule     *engine_module;
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
  void                      ostream_set     (unsigned int ostream_index,
                                             const float *values);
  const float*              const_values    (float  value);
  inline const unsigned int mix_freq        () const;
  inline const unsigned int block_size      () const;
  inline guint64            tick_stamp      ();
  inline BseModule*         gslmodule       ();
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
  void                      set_module      (BseModule *module);
};

#define BSE_TYPE_EFFECT         (BSE_CXX_TYPE_GET_REGISTERED (Bse, Effect))
class EffectBase : public CxxBase {};
class Effect : public EffectBase {
public:
  /* BseObject functionality */
  explicit                  Effect               ();
  void                      set_property         (guint            prop_id,
                                                  const Value     &value,
                                                  GParamSpec      *pspec);
  void                      get_property         (guint            prop_id,
                                                  Value           &value,
                                                  GParamSpec      *pspec);
  /* BseSource accessors */
  bool          is_prepared()               const { return BSE_SOURCE_PREPARED (gobject()); }
  guint         n_ichannels()               const { return BSE_SOURCE_N_ICHANNELS (gobject()); }
  guint         n_joint_ichannels()         const { return BSE_SOURCE_N_JOINT_ICHANNELS (gobject()); }
  guint         n_ochannels()               const { return BSE_SOURCE_N_OCHANNELS (gobject()); }
  bool          is_joint_ichannel (guint i) const { return BSE_SOURCE_IS_JOINT_ICHANNEL (gobject(), i); }
  guint         ichannels_istream (guint i) const { return BSE_SOURCE_ICHANNEL_ISTREAM (gobject(), i); }
  guint         ichannels_jstream (guint i) const { return BSE_SOURCE_ICHANNEL_JSTREAM (gobject(), i); }
  guint         ochannels_ostream (guint i) const { return BSE_SOURCE_OCHANNEL_OSTREAM (gobject(), i); }
  const gchar*  ichannel_ident (guint i)    const { return BSE_SOURCE_ICHANNEL_IDENT (gobject(), i); }
  const gchar*  ichannel_label (guint i)    const { return BSE_SOURCE_ICHANNEL_LABEL (gobject(), i); }
  const gchar*  ichannel_blurb (guint i)    const { return BSE_SOURCE_ICHANNEL_BLURB (gobject(), i); }
  const gchar*  ochannel_ident (guint i)    const { return BSE_SOURCE_OCHANNEL_IDENT (gobject(), i); }
  const gchar*  ochannel_label (guint i)    const { return BSE_SOURCE_OCHANNEL_LABEL (gobject(), i); }
  const gchar*  ochannel_blurb (guint i)    const { return BSE_SOURCE_OCHANNEL_BLURB (gobject(), i); }
  virtual SynthesisModule*  create_module        (unsigned int     context_handle,
                                                  BseTrans        *trans) = 0;
  virtual SynthesisModule::
  Accessor*                 module_configurator  () = 0;
  void                      update_modules       (BseTrans        *trans = NULL);
  /* prepare & dismiss pre and post invocation hooks */
  virtual void  prepare1()      { /* override this to do something before parent class prepare */ }
  virtual void  prepare2()      { /* override this to do something after parent class prepare */ }
  virtual void  reset1()        { /* override this to do something before parent class dismiss */ }
  virtual void  reset2()        { /* override this to do something after parent class dismiss */ }
  
  static void               class_init           (CxxBaseClass    *klass);
protected:
  const BseModuleClass*     create_gsl_class     (SynthesisModule *sample_module,
                                                  int              cost = -1,
                                                  int              n_istreams = -1,
                                                  int              n_jstreams = -1,
                                                  int              n_ostreams = -1);
  virtual BseModule*        integrate_bse_module (unsigned int     context_handle,
                                                  BseTrans        *trans);
  virtual void              dismiss_bse_module   (BseModule       *gslmodule,
                                                  guint            context_handle,
                                                  BseTrans        *trans);
  unsigned int              block_size() const;
};
/* effect method: create_module(); */
#define BSE_CXX_DEFINE_CREATE_MODULE(ObjectType,ModuleType,ParamType)           \
  Bse::SynthesisModule*                                                         \
  ObjectType::create_module (unsigned int context_handle,                       \
                             BseTrans    *trans)                                \
  { /* create a synthesis module */                                             \
    return new ModuleType();                                                    \
  }
/* effect method: module_configurator(); */
#define BSE_CXX_DEFINE_MODULE_CONFIGURATOR(ObjectType,ModuleType,ParamType)     \
Bse::SynthesisModule::Accessor*                                                 \
ObjectType::module_configurator()                                               \
{                                                                               \
  return SynthesisModule::accessor (&ModuleType::config, ParamType (this));     \
}
/* convenience macro to define BseEffect module methods */
#define BSE_EFFECT_INTEGRATE_MODULE(ObjectType,ModuleType,ParamType)            \
  BSE_CXX_DEFINE_CREATE_MODULE (ObjectType,ModuleType,ParamType);               \
  BSE_CXX_DEFINE_MODULE_CONFIGURATOR (ObjectType,ModuleType,ParamType);


/* --- implementation details --- */
namespace externC { extern "C" {
extern guint bse_engine_exvar_sample_freq;
extern guint bse_engine_exvar_block_size;
extern guint64 bse_module_tick_stamp (BseModule*);
} }
inline BseModule*
SynthesisModule::gslmodule ()
{
  return engine_module;
}
inline const unsigned int
SynthesisModule::mix_freq () const
{
  return externC::bse_engine_exvar_sample_freq;
}
inline const unsigned int
SynthesisModule::block_size () const
{
  return externC::bse_engine_exvar_block_size;
}
inline guint64
SynthesisModule::tick_stamp ()
{
  return externC::bse_module_tick_stamp (gslmodule());
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

#endif /* __BSE_CXX_MODULE_H__ */
