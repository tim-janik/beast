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
#include "bsemidireceiver.h"
#include "bsesnet.h"

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
  intern_module = NULL;
}

void
SynthesisModule::set_module (BseModule *engine_module)
{
  g_return_if_fail (intern_module == NULL);
  g_return_if_fail (engine_module != NULL);
  
  intern_module = engine_module;
  /* see check_mirror_structs() on why these casts are valid */
  istreams = reinterpret_cast<IStream*> (engine_module->istreams);
  jstreams = reinterpret_cast<JStream*> (engine_module->jstreams);
  ostreams = reinterpret_cast<OStream*> (engine_module->ostreams);
}

void
SynthesisModule::ostream_set (unsigned int ostream_index,
                              const float *values)
{
  BseModule *m = engine_module();
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
process_module (BseModule *engine_module,
                guint      n_values)
{
  SynthesisModule *m = static_cast<SynthesisModule*> (engine_module->user_data);
  m->process (n_values);
}

void
reset_module (BseModule *engine_module)
{
  SynthesisModule *m = static_cast<SynthesisModule*> (engine_module->user_data);
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
Effect::create_engine_class (SynthesisModule *sample_module,
                             int              cost,
                             int              n_istreams,
                             int              n_jstreams,
                             int              n_ostreams)
{
  BseSource *source = cast (this);
  BseSourceClass *source_class = BSE_SOURCE_GET_CLASS (source);
  if (!source_class->engine_class)
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
      bse_source_class_cache_engine_class (source_class, &klass);
    }
  return source_class->engine_class;
}

struct MidiControlJobData {
  guint  prop_id;
  gfloat control_value;
  static void
  free (gpointer data)
  {
    MidiControlJobData *jdata = static_cast<MidiControlJobData*> (data);
    delete jdata;
  }
};

static void
midi_control_flow_access (BseModule      *module,       /* Engine Thread */
                          gpointer        data)
{
  MidiControlJobData *jdata = static_cast<MidiControlJobData*> (data);
  g_printerr ("midi_control: module=%p prop_id=%u value=%f\n", module, jdata->prop_id, jdata->control_value);
}

static void
midi_control_handler (gpointer                  handler_data, /* MIDI Device Thread (and possibly others) */
                      guint64                   tick_stamp,
                      BseMidiSignalType         signal_type,
                      gfloat                    control_value,
                      guint                     n_mcdatas,
                      BseModule          *const*modules,
                      BseTrans                 *trans)
{
  GParamSpec *pspec = static_cast<GParamSpec*> (handler_data);
  g_return_if_fail (n_mcdatas > 0);
  MidiControlJobData *jdata = new MidiControlJobData;
  jdata->prop_id = pspec->param_id;
  jdata->control_value = control_value;
  for (guint i = 0; i < n_mcdatas; i++)
    bse_trans_add (trans,
                   bse_job_flow_access (modules[i],
                                        tick_stamp,
                                        midi_control_flow_access,
                                        jdata,
                                        i + 1 >= n_mcdatas ? MidiControlJobData::free : NULL));
}

static void
get_midi_control_range (GParamSpec *pspec,
                        float      &minimum,
                        float      &maximum)
{
  if (SFI_IS_PSPEC_REAL (pspec))
    {
      SfiParamSpecReal *rspec = SFI_PSPEC_REAL (pspec);
      minimum = rspec->minimum;
      maximum = rspec->maximum;
    }
  else if (SFI_IS_PSPEC_INT (pspec))
    {
      SfiParamSpecInt *ispec = SFI_PSPEC_INT (pspec);
      minimum = ispec->minimum;
      maximum = ispec->maximum;
    }
  else if (SFI_IS_PSPEC_BOOL (pspec))
    {
      minimum = 0;
      maximum = 1;
    }
  else if (SFI_IS_PSPEC_NUM (pspec))
    {
      SfiParamSpecNum *nspec = SFI_PSPEC_NUM (pspec);
      minimum = nspec->minimum;
      maximum = nspec->maximum;
    }
}

struct HandlerSetup {
  bool                   add_handler;
  guint                  n_aprops;
  BseAutomationProperty *aprops;
  BseMidiReceiver       *midi_receiver;
  guint                  midi_channel;
};

static void
handler_setup_func (BseModule      *module,   /* Engine Thread */
                    gpointer        data)
{
  HandlerSetup *hs = static_cast<HandlerSetup*> (data);
  guint i;
  for (i = 0; i < hs->n_aprops; i++)
    if (hs->add_handler)
      {
        float minimum = 0, maximum = 0;
        get_midi_control_range (hs->aprops[i].pspec, minimum, maximum);
        bse_midi_receiver_add_control_handler (hs->midi_receiver,
                                               hs->midi_channel,
                                               hs->aprops[i].signal_type,
                                               minimum, maximum,
                                               midi_control_handler,
                                               hs->aprops[i].pspec,
                                               module);
      }
    else
      bse_midi_receiver_remove_control_handler (hs->midi_receiver,
                                                hs->midi_channel,
                                                hs->aprops[i].signal_type,
                                                midi_control_handler,
                                                hs->aprops[i].pspec,
                                                module);
}

static void
handler_setup_free (gpointer        data)       /* User Thread */
{
  HandlerSetup *hs = reinterpret_cast<HandlerSetup*> (data);
  g_free (hs->aprops);
  g_free (hs);
}

BseModule*
Effect::integrate_engine_module (unsigned int   context_handle,
                                 BseTrans      *trans)
{
  BseSource *source = cast (this);
  SynthesisModule *cxxmodule = create_module (context_handle, trans);
  BseModule *engine_module = bse_module_new (create_engine_class (cxxmodule), cxxmodule);
  cxxmodule->set_module (engine_module);
  /* intergrate module into engine */
  bse_trans_add (trans, bse_job_integrate (engine_module));
  /* register MIDI control handlers */
  guint n_props = 0;
  BseAutomationProperty *aprops = bse_source_get_automation_properties (source, &n_props);
  if (n_props)
    {
      HandlerSetup *hs = g_new0 (HandlerSetup, 1);
      hs->add_handler = true;
      hs->n_aprops = n_props;
      hs->aprops = aprops;
      BseMidiContext mc = bse_snet_get_midi_context (bse_item_get_snet (BSE_ITEM (source)), context_handle);
      hs->midi_receiver = mc.midi_receiver;
      hs->midi_channel = mc.midi_channel;
      bse_trans_add (trans, bse_job_access (engine_module, handler_setup_func, hs, handler_setup_free));
    }
  return engine_module;
}

void
Effect::dismiss_engine_module (BseModule       *engine_module,
                               guint            context_handle,
                               BseTrans        *trans)
{
  BseSource *source = cast (this);
  if (engine_module)
    {
      /* unregister MIDI control handlers */
      guint n_props = 0;
      BseAutomationProperty *aprops = bse_source_get_automation_properties (source, &n_props);
      if (n_props)
        {
          HandlerSetup *hs = g_new0 (HandlerSetup, 1);
          hs->add_handler = false;
          hs->n_aprops = n_props;
          hs->aprops = aprops;
          BseMidiContext mc = bse_snet_get_midi_context (bse_item_get_snet (BSE_ITEM (source)), context_handle);
          hs->midi_receiver = mc.midi_receiver;
          hs->midi_channel = mc.midi_channel;
          bse_trans_add (trans, bse_job_access (engine_module, handler_setup_func, hs, handler_setup_free));
        }
      /* discard module */
      bse_trans_add (trans, bse_job_discard (engine_module));
    }
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
      BseModule *engine_module = self->integrate_engine_module (context_handle, trans);
      
      /* setup module i/o streams with BseSource i/o channels */
      bse_source_set_context_module (source, context_handle, engine_module);
      
      /* reset module */
      bse_trans_add (trans, bse_job_force_reset (engine_module));
      /* configure module */
      SynthesisModule::Accessor *ac = self->module_configurator();
      if (ac)
        bse_trans_add (trans, bse_job_access (engine_module, access_trampoline, ac, access_data_free));
      
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
      BseModule *engine_module = NULL;
      if (BSE_SOURCE_N_ICHANNELS (source))
        {
          engine_module = bse_source_get_context_imodule (source, context_handle);
          bse_source_set_context_imodule (source, context_handle, NULL);
        }
      if (BSE_SOURCE_N_OCHANNELS (source))
        {
          engine_module = bse_source_get_context_omodule (source, context_handle);
          bse_source_set_context_omodule (source, context_handle, NULL);
        }

      self->dismiss_engine_module (engine_module, context_handle, trans);

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
