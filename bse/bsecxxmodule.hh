// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_CXX_MODULE_H__
#define __BSE_CXX_MODULE_H__

#include <bse/bsecxxbase.hh>
#include <bse/bseieee754.hh>

namespace Bse {

/* enums/structures mirrored from bseengine.hh */
enum ProcessCost {
  NORMAL,
  CHEAP,
  EXPENSIVE
};
struct JStream {
  const float **values;
  uint          n_connections;
  /* private: */
  uint          jcount; /* reserved */
};
struct IStream {
  const float  *values;
  gboolean      connected;
};
struct OStream {
  float        *values;
  gboolean      connected;
};

class Effect;

class SynthesisModule {
  template<class T, typename P> class ClosureP1; /* 1-argument member function closure */
  BseModule     *intern_module;
public:
  explicit                  SynthesisModule ();
  virtual                  ~SynthesisModule () = 0;
  virtual void              reset           () = 0;
  virtual void              process         (uint n_values) = 0;
  virtual const ProcessCost cost            ();
  inline const IStream&     istream         (uint istream_index) const;
  inline const JStream&     jstream         (uint jstream_index) const;
  inline const OStream&     ostream         (uint ostream_index) const;
  void                      ostream_set     (uint ostream_index,
                                             const float *values);
  const float*              const_values    (float  value);
  inline const uint         mix_freq        () const;
  inline const uint         block_size      () const;
  inline guint64            tick_stamp      ();
  inline BseModule*         engine_module   ();
  static inline int         dtoi            (double d) { return bse_dtoi (d); }
  static inline int         ftoi            (float  f) { return bse_ftoi (f); }
  /* member functions and closures */
  struct Closure {
    virtual void            operator()      (SynthesisModule*) = 0;
    virtual                ~Closure         ()         {}
  };
  /* create a 1-argument member function closure, where C must be derived from SynthesisModule */
  template<class D, class C>
  static Closure*           make_closure    (void    (C::*method) (D*),
                                             const D     &data);
  /* internal */
  void                      set_module      (BseModule *module);
  /* auto_update() trampoline */
public:
  typedef void     (*AutoUpdate)            (BseModule*, gpointer);
  struct AutoUpdateData {
    guint       prop_id;
    double      prop_value;
    /* required by back propagation */
    guint64     tick_stamp;
    GParamSpec *pspec;
    Effect     *effect;
  };    
  struct NeedAutoUpdateTag {};
protected:
  template<class M, class P, class C> struct Trampoline {
    static void auto_update_accessor (BseModule*, gpointer);
  };
  /* partial trampoline specializations */
  template<class M, class P> struct Trampoline<M,P,NeedAutoUpdateTag> {
    static void auto_update_accessor (BseModule*, gpointer);
  };
  template<class M, class P> struct Trampoline<M,P,void> {
    static void auto_update_accessor (BseModule*, gpointer);
  };
};

#define BSE_TYPE_EFFECT         (BSE_CXX_TYPE_GET_REGISTERED (Bse, Effect))
class EffectBase : public CxxBase {};
class Effect : public EffectBase {
private:
  guint64                   last_module_update;
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
  const gchar*  ichannel_ident    (guint i) const { return BSE_SOURCE_ICHANNEL_IDENT (gobject(), i); }
  const gchar*  ichannel_label    (guint i) const { return BSE_SOURCE_ICHANNEL_LABEL (gobject(), i); }
  const gchar*  ichannel_blurb    (guint i) const { return BSE_SOURCE_ICHANNEL_BLURB (gobject(), i); }
  const gchar*  ochannel_ident    (guint i) const { return BSE_SOURCE_OCHANNEL_IDENT (gobject(), i); }
  const gchar*  ochannel_label    (guint i) const { return BSE_SOURCE_OCHANNEL_LABEL (gobject(), i); }
  const gchar*  ochannel_blurb    (guint i) const { return BSE_SOURCE_OCHANNEL_BLURB (gobject(), i); }
  virtual SynthesisModule*  create_module              (uint             context_handle,
                                                        BseTrans        *trans) = 0;
  virtual SynthesisModule::
  Closure*                  make_module_config_closure () = 0;
  virtual SynthesisModule::
  AutoUpdate                get_module_auto_update     () = 0;
  void                      update_modules             (BseTrans        *trans = NULL);
  guint64                   module_update_tick_stamp   () { return last_module_update; }
  /* prepare & dismiss pre and post invocation hooks */
  virtual void  prepare1()      { /* override this to do something before parent class prepare */ }
  virtual void  prepare2()      { /* override this to do something after parent class prepare */ }
  virtual void  reset1()        { /* override this to do something before parent class dismiss */ }
  virtual void  reset2()        { /* override this to do something after parent class dismiss */ }

  static void               class_init                 (CxxBaseClass    *klass);
protected:
  const BseModuleClass*     create_engine_class        (SynthesisModule *sample_module,
                                                        int              cost = -1,
                                                        int              n_istreams = -1,
                                                        int              n_jstreams = -1,
                                                        int              n_ostreams = -1);
  virtual BseModule*        integrate_engine_module    (uint             context_handle,
                                                        BseTrans        *trans);
  virtual void              dismiss_engine_module      (BseModule       *engine_module,
                                                        guint            context_handle,
                                                        BseTrans        *trans);
  uint                      block_size                 () const;
  uint                      max_block_size             () const;
public: /* FIXME: make this protected as soon as the modules have their own current_musical_tuning() accessor */
  BseMusicalTuningType      current_musical_tuning     () const;
};
/* implement Bse::Effect and Bse::SynthesisModule methods */
#define BSE_EFFECT_INTEGRATE_MODULE(ObjectType,ModuleType,ParamType)            \
Bse::SynthesisModule*                                                           \
create_module (uint         context_handle,                                     \
               BseTrans    *trans)                                              \
{                                                                               \
  /* check that 'this' is a ObjectType* */                                      \
  (void) const_cast<ObjectType*> (this);                                        \
  /* create a synthesis module */                                               \
  return new ModuleType();                                                      \
}                                                                               \
Bse::SynthesisModule::Closure*                                                  \
make_module_config_closure()                                                    \
{                                                                               \
  return SynthesisModule::make_closure (&ModuleType::config, ParamType (this)); \
}                                                                               \
Bse::SynthesisModule::AutoUpdate                                                \
get_module_auto_update()                                                        \
{                                                                               \
  return SynthesisModule::Trampoline<ModuleType,ParamType,                      \
                  ObjectType::AutoUpdateCategory>::auto_update_accessor;        \
}
template<class M, class P>
void
SynthesisModule::Trampoline<M,P,SynthesisModule::NeedAutoUpdateTag>::
auto_update_accessor (BseModule *bmodule,      /* Engine Thread */
                      gpointer   data)
{
  M *m = static_cast<M*> (BSE_MODULE_GET_USER_DATA (bmodule));
  AutoUpdateData *au = static_cast<AutoUpdateData*> (data);
  typename P::IDType prop_id = static_cast<typename P::IDType> (au->prop_id);
  if (0)        // check M::auto_update() member and prototype
    (void) static_cast<void (M::*) (typename P::IDType, double)> (&M::auto_update);
  m->auto_update (prop_id, au->prop_value);
}
template<class M, class P>
void
SynthesisModule::Trampoline<M,P,void>::
auto_update_accessor (BseModule *bmodule,
                      gpointer   data)
{
}


/* --- implementation details --- */
namespace externC { extern "C" {
extern guint bse_engine_exvar_sample_freq;
extern guint bse_engine_exvar_block_size;
extern guint64 bse_module_tick_stamp (BseModule*);
} }
inline BseModule*
SynthesisModule::engine_module ()
{
  return intern_module;
}
inline const uint
SynthesisModule::mix_freq () const
{
  return externC::bse_engine_exvar_sample_freq;
}
inline const uint
SynthesisModule::block_size () const
{
  return externC::bse_engine_exvar_block_size;
}
inline guint64
SynthesisModule::tick_stamp ()
{
  return externC::bse_module_tick_stamp (engine_module());
}
inline const IStream&
SynthesisModule::istream (uint         istream_index) const
{
  void *istreams = BSE_MODULE_GET_ISTREAMSP (intern_module);
  return reinterpret_cast<IStream*> (istreams)[istream_index];
}
inline const JStream&
SynthesisModule::jstream (uint         jstream_index) const
{
  void *jstreams = BSE_MODULE_GET_JSTREAMSP (intern_module);
  return reinterpret_cast<JStream*> (jstreams)[jstream_index];
}
inline const OStream&
SynthesisModule::ostream (uint         ostream_index) const
{
  void *ostreams = BSE_MODULE_GET_OSTREAMSP (intern_module);
  return reinterpret_cast<OStream*> (ostreams)[ostream_index];
}
template<class T, typename P>
class SynthesisModule::ClosureP1 : public SynthesisModule::Closure {
  typedef void (T::*Member) (P*);
  Member    func;
  P        *data;
public:
  ClosureP1 (void (T::*f) (P*), P *p)
    : func (f), data (p)
  {
    assert_derived_from<T,SynthesisModule>();
  }
  void operator() (SynthesisModule *p)
  {
    T *t = static_cast<T*> (p);
    (t->*func) (data);
  }
  ~ClosureP1 ()
  {
    delete data;
  }
};
template<class D, class C> SynthesisModule::Closure*
SynthesisModule::make_closure (void   (C::*method) (D*),
                               const D    &data)
{
  D *d = new D (data);
  ClosureP1<C,D> *ac = new ClosureP1<C,D> (method, d);
  return ac;
}

} // Bse

#endif /* __BSE_CXX_MODULE_H__ */
