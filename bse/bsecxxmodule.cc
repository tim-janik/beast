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
#include "gslengine.h"

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
access_trampoline (GslModule *module,
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
Effect::update_modules (GslTrans *trans)
{
  BseSource *source = cast (this);
  if (BSE_SOURCE_PREPARED (source))
    {
      SynthesisModule::Accessor *ac = module_configurator();
      if (ac)
        {
          GslTrans *atrans = trans ? trans : gsl_trans_open();
          bse_source_access_modules (source, access_trampoline, ac, access_data_free, atrans);
          if (!trans)
            gsl_trans_commit (atrans);
        }
    }
}

SynthesisModule::SynthesisModule()
{
  engine_module = NULL;
}

void
SynthesisModule::set_module (GslModule *gslmodule)
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
  GslModule *gslmodule = static_cast<GslModule*> (engine_module);
  gslmodule->ostreams[ostream_index].values = const_cast<float*> (values);
}

const float*
SynthesisModule::const_values (float value)
{
  return gsl_engine_const_values (value);
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
process_module (GslModule *gslmodule,
                guint      n_values)
{
  SynthesisModule *m = static_cast<SynthesisModule*> (gslmodule->user_data);
  m->process (n_values);
}

void
reset_module (GslModule *gslmodule)
{
  SynthesisModule *m = static_cast<SynthesisModule*> (gslmodule->user_data);
  m->reset();
}

void
delete_module (gpointer        data,
               const GslClass *klass)
{
  SynthesisModule *m = static_cast<SynthesisModule*> (data);
  delete m;
}

static GslModuleFlags
module_flags_from_process_cost (ProcessCost cost)
{
  switch (cost)
    {
    case EXPENSIVE:     return GSL_COST_EXPENSIVE;
    case CHEAP:         return GSL_COST_CHEAP;
    default:
    case NORMAL:        return GSL_COST_NORMAL;
    }
}

const GslClass*
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
      GslClass klass = {
        0,                      /* n_istreams */
        0,                      /* n_jstreams */
        0,                      /* n_ostreams */
        process_module,         /* process */
        NULL,                   /* process_defer */
        reset_module,           /* reset */
        delete_module,          /* free */
        GSL_COST_NORMAL,        /* mflags */
      };
      klass.mflags = GslModuleFlags (cost >= 0 ? cost : module_flags_from_process_cost (sample_module->cost()));
      klass.n_istreams = n_istreams >= 0 ? n_istreams : (BSE_SOURCE_N_ICHANNELS (source) -
                                                         BSE_SOURCE_N_JOINT_ICHANNELS (source));
      klass.n_jstreams = n_jstreams >= 0 ? n_jstreams : BSE_SOURCE_N_JOINT_ICHANNELS (source);
      klass.n_ostreams = n_ostreams >= 0 ? n_ostreams : BSE_SOURCE_N_OCHANNELS (source);
      bse_source_class_cache_gsl_class (source_class, &klass);
    }
  return source_class->gsl_class;
}

GslModule*
Effect::integrate_gsl_module (unsigned int   context_handle,
                              GslTrans      *trans)
{
  SynthesisModule *cxxmodule = create_module (context_handle, trans);
  GslModule *gslmodule = gsl_module_new (create_gsl_class (cxxmodule), cxxmodule);
  cxxmodule->set_module (gslmodule);
  /* intergrate module into engine */
  gsl_trans_add (trans, gsl_job_integrate (gslmodule));
  return gslmodule;
}

void
Effect::dismiss_gsl_module (GslModule       *gslmodule,
                            guint            context_handle,
                            GslTrans        *trans)
{
  if (gslmodule)
    gsl_trans_add (trans, gsl_job_discard (gslmodule));
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
                           GslTrans  *trans)
    {
      CxxBase *base = cast (source);
      Effect *self = static_cast<Effect*> (base);
      GslModule *gslmodule = self->integrate_gsl_module (context_handle, trans);
      
      /* setup module i/o streams with BseSource i/o channels */
      bse_source_set_context_module (source, context_handle, gslmodule);
      
      /* reset module */
      gsl_trans_add (trans, gsl_job_force_reset (gslmodule));
      /* configure module */
      SynthesisModule::Accessor *ac = self->module_configurator();
      if (ac)
        gsl_trans_add (trans, gsl_job_access (gslmodule, access_trampoline, ac, access_data_free));
      
      /* chain parent class' handler */
      BSE_SOURCE_CLASS (effect_parent_class)->context_create (source, context_handle, trans);
    }
    static void
    effect_context_dismiss (BseSource *source,
                            guint      context_handle,
                            GslTrans  *trans)
    {
      CxxBase *base = cast (source);
      Effect *self = static_cast<Effect*> (base);
      GslModule *gslmodule = NULL;
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

      self->dismiss_gsl_module (gslmodule, context_handle, trans);

      /* chain parent class' handler */
      BSE_SOURCE_CLASS (effect_parent_class)->context_dismiss (source, context_handle, trans);
    }
  };
  BseSourceClass *source_class = klass;

  effect_parent_class = g_type_class_peek_parent (klass);
  source_class->context_create = Local::effect_context_create;
  source_class->context_dismiss = Local::effect_context_dismiss;
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
  ASSERT (sizeof (JStream) == sizeof (GslJStream));
  ASSERT_FIELD (JStream, GslJStream, values);
  ASSERT_FIELD (JStream, GslJStream, n_connections);
  
  ASSERT (sizeof (IStream) == sizeof (GslIStream));
  ASSERT_FIELD (IStream, GslIStream, values);
  
  ASSERT (sizeof (OStream) == sizeof (GslOStream));
  ASSERT_FIELD (OStream, GslOStream, values);
}

extern "C" void
bse_cxx_checks (void)  // prototyped in bseutils.h
{
  check_mirror_structs ();
}

}
