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
#include "bsecxxmodule.h"
#include "bseengine.h"

namespace {
using namespace Bse;

const ClassInfo cinfo (NULL, "BseEffect implements an abstract C++ effect base.");
BSE_CXX_TYPE_REGISTER_ABSTRACT (Effect, "BseCxxBase", &cinfo);

Effect::Effect()
{
}

void
Effect::set_property (guint        prop_id,
                      const Value &value,
                      GParamSpec  *pspec)
{
}

void
Effect::get_property (guint       prop_id,
                      Value      &value,
                      GParamSpec *pspec)
{
}

void
access_trampoline (BseModule *module,
                   gpointer   data)
{
  SynthesisModule::Accessor *ac = static_cast<SynthesisModule::Accessor*> (data);
  SynthesisModule *m = static_cast<SynthesisModule*> (module->user_data);
  (*ac) (m);
}

void
access_data_free (gpointer data)
{
  SynthesisModule::Accessor *ac = static_cast<SynthesisModule::Accessor*> (data);
  delete ac;
}

void
Effect::update_modules (BseTrans *trans)
{
  BseSource *source = cast (this);
  if (BSE_SOURCE_PREPARED (source))
    {
      SynthesisModule::Accessor *ac = module_configurator();
      if (ac)
        {
          BseTrans *atrans = trans ? trans : bse_trans_open();
          bse_source_access_modules (source, access_trampoline, ac, access_data_free, atrans);
          if (!trans)
            bse_trans_commit (atrans);
        }
    }
}

SynthesisModule::SynthesisModule()
{
  engine_module = NULL;
}

void
SynthesisModule::set_module (BseModule *gslmodule)
{
  g_return_if_fail (engine_module == NULL);
  g_return_if_fail (gslmodule != NULL);
  
  engine_module = gslmodule;
  /* see check_mirror_structs() on why these casts are valid */
  istreams = reinterpret_cast<IStream*> (gslmodule->istreams);
  jstreams = reinterpret_cast<JStream*> (gslmodule->jstreams);
  ostreams = reinterpret_cast<OStream*> (gslmodule->ostreams);
}

void
SynthesisModule::ostream_set (unsigned int ostream_index,
                              const float *values)
{
  BseModule *m = gslmodule();
  m->ostreams[ostream_index].values = const_cast<float*> (values);
}

const float*
SynthesisModule::const_values (float value)
{
  return bse_engine_const_values (value);
}

SynthesisModule::~SynthesisModule()
{
}

const ProcessCost
SynthesisModule::cost()
{
  return NORMAL;
}

void
process_module (BseModule *gslmodule,
                guint      n_values)
{
  SynthesisModule *m = static_cast<SynthesisModule*> (gslmodule->user_data);
  m->process (n_values);
}

void
reset_module (BseModule *gslmodule)
{
  SynthesisModule *m = static_cast<SynthesisModule*> (gslmodule->user_data);
  m->reset();
}

void
delete_module (gpointer        data,
               const BseModuleClass *klass)
{
  SynthesisModule *m = static_cast<SynthesisModule*> (data);
  delete m;
}

static BseCostType
module_flags_from_process_cost (ProcessCost cost)
{
  switch (cost)
    {
    case EXPENSIVE:     return BSE_COST_EXPENSIVE;
    case CHEAP:         return BSE_COST_CHEAP;
    default:
    case NORMAL:        return BSE_COST_NORMAL;
    }
}

const BseModuleClass*
Effect::create_gsl_class (SynthesisModule *sample_module,
                          int              cost,
                          int              n_istreams,
                          int              n_jstreams,
                          int              n_ostreams)
{
  BseSource *source = cast (this);
  BseSourceClass *source_class = BSE_SOURCE_GET_CLASS (source);
  if (!source_class->gsl_class)
    {
      BseModuleClass klass = {
        0,                      /* n_istreams */
        0,                      /* n_jstreams */
        0,                      /* n_ostreams */
        process_module,         /* process */
        NULL,                   /* process_defer */
        reset_module,           /* reset */
        delete_module,          /* free */
        BSE_COST_NORMAL,        /* mflags */
      };
      klass.mflags = BseCostType (cost >= 0 ? cost : module_flags_from_process_cost (sample_module->cost()));
      klass.n_istreams = n_istreams >= 0 ? n_istreams : (BSE_SOURCE_N_ICHANNELS (source) -
                                                         BSE_SOURCE_N_JOINT_ICHANNELS (source));
      klass.n_jstreams = n_jstreams >= 0 ? n_jstreams : BSE_SOURCE_N_JOINT_ICHANNELS (source);
      klass.n_ostreams = n_ostreams >= 0 ? n_ostreams : BSE_SOURCE_N_OCHANNELS (source);
      bse_source_class_cache_gsl_class (source_class, &klass);
    }
  return source_class->gsl_class;
}

BseModule*
Effect::integrate_bse_module (unsigned int   context_handle,
                              BseTrans      *trans)
{
  SynthesisModule *cxxmodule = create_module (context_handle, trans);
  BseModule *gslmodule = bse_module_new (create_gsl_class (cxxmodule), cxxmodule);
  cxxmodule->set_module (gslmodule);
  /* intergrate module into engine */
  bse_trans_add (trans, bse_job_integrate (gslmodule));
  return gslmodule;
}

void
Effect::dismiss_bse_module (BseModule       *gslmodule,
                            guint            context_handle,
                            BseTrans        *trans)
{
  if (gslmodule)
    bse_trans_add (trans, bse_job_discard (gslmodule));
}

unsigned int
Effect::block_size() const
{
  g_return_val_if_fail (is_prepared(), 0);

  return bse_engine_block_size();
}

void
Effect::class_init (CxxBaseClass *klass)
{
  static gpointer effect_parent_class = NULL;
  struct Local
  {
    static void
    effect_context_create (BseSource *source,
                           guint      context_handle,
                           BseTrans  *trans)
    {
      CxxBase *base = cast (source);
      Effect *self = static_cast<Effect*> (base);
      BseModule *gslmodule = self->integrate_bse_module (context_handle, trans);
      
      /* setup module i/o streams with BseSource i/o channels */
      bse_source_set_context_module (source, context_handle, gslmodule);
      
      /* reset module */
      bse_trans_add (trans, bse_job_force_reset (gslmodule));
      /* configure module */
      SynthesisModule::Accessor *ac = self->module_configurator();
      if (ac)
        bse_trans_add (trans, bse_job_access (gslmodule, access_trampoline, ac, access_data_free));
      
      /* chain parent class' handler */
      BSE_SOURCE_CLASS (effect_parent_class)->context_create (source, context_handle, trans);
    }
    static void
    effect_context_dismiss (BseSource *source,
                            guint      context_handle,
                            BseTrans  *trans)
    {
      CxxBase *base = cast (source);
      Effect *self = static_cast<Effect*> (base);
      BseModule *gslmodule = NULL;
      if (BSE_SOURCE_N_ICHANNELS (source))
        {
          gslmodule = bse_source_get_context_imodule (source, context_handle);
          bse_source_set_context_imodule (source, context_handle, NULL);
        }
      if (BSE_SOURCE_N_OCHANNELS (source))
        {
          gslmodule = bse_source_get_context_omodule (source, context_handle);
          bse_source_set_context_omodule (source, context_handle, NULL);
        }

      self->dismiss_bse_module (gslmodule, context_handle, trans);

      /* chain parent class' handler */
      BSE_SOURCE_CLASS (effect_parent_class)->context_dismiss (source, context_handle, trans);
    }
    static void effect_prepare (BseSource *source)
    {
      CxxBase *base = cast (source);
      Effect *self = static_cast<Effect*> (base);

      /* invoke code that the effect might want to execute before prepare */
      self->prepare1();

      /* chain parent class' handler */
      BSE_SOURCE_CLASS (effect_parent_class)->prepare (source);

      /* invoke code that the effect might want to execute after prepare */
      self->prepare2();
    }
    static void effect_reset (BseSource *source)
    {
      CxxBase *base = cast (source);
      Effect *self = static_cast<Effect*> (base);

      /* invoke code that the effect might want to execute before reset */
      self->reset1();

      /* chain parent class' handler */
      BSE_SOURCE_CLASS (effect_parent_class)->reset (source);

      /* invoke code that the effect might want to execute after reset */
      self->reset2();
    }
  };
  BseSourceClass *source_class = klass;

  effect_parent_class = g_type_class_peek_parent (klass);
  source_class->context_create = Local::effect_context_create;
  source_class->context_dismiss = Local::effect_context_dismiss;
  source_class->prepare = Local::effect_prepare;
  source_class->reset = Local::effect_reset;
}


#define ASSERT(foo)           if (foo) ; else g_error ("failed to assert: %s", # foo )
#define FIELD_OFFSET(S,F)     (-1024 + (size_t) &((S*) 1024)->F)      // stop gcc warning about acessing NULL object fields
#define FIELD_SIZE(S,F)       (sizeof (&((S*) 1024)->F))              // stop gcc warning about referencing non-static members
#define ASSERT_FIELD(CxxS, CS, F)       do { \
  ASSERT (FIELD_OFFSET (CxxS, F) == G_STRUCT_OFFSET (CS, F)); \
  ASSERT (FIELD_SIZE (CxxS, F) == FIELD_SIZE (CS, F)); \
} while (0)

static void
check_mirror_structs ()
{
  ASSERT (sizeof (JStream) == sizeof (BseJStream));
  ASSERT_FIELD (JStream, BseJStream, values);
  ASSERT_FIELD (JStream, BseJStream, n_connections);
  
  ASSERT (sizeof (IStream) == sizeof (BseIStream));
  ASSERT_FIELD (IStream, BseIStream, values);
  
  ASSERT (sizeof (OStream) == sizeof (BseOStream));
  ASSERT_FIELD (OStream, BseOStream, values);
}

extern "C" void
bse_cxx_checks (void)  // prototyped in bseutils.h
{
  check_mirror_structs ();
}

}
