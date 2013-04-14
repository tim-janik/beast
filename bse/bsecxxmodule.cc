// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsecxxmodule.hh"
#include "bseengine.hh"
#include "bsemidireceiver.hh"
#include "bsesnet.hh"
namespace Bse {
static const ClassInfo class_info (NULL, "BseEffect implements an abstract C++ effect base.", __FILE__, __LINE__);
BSE_CXX_TYPE_REGISTER_ABSTRACT (Effect, "BseCxxBase", &class_info);
Effect::Effect() :
  last_module_update (0)
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
static void
access_trampoline (BseModule *module,
                   gpointer   data)
{
  SynthesisModule::Closure *clo = static_cast<SynthesisModule::Closure*> (data);
  SynthesisModule *m = static_cast<SynthesisModule*> (module->user_data);
  (*clo) (m);
}
static void
access_data_free (gpointer data)
{
  SynthesisModule::Closure *clo = static_cast<SynthesisModule::Closure*> (data);
  delete clo;
}
void
Effect::update_modules (BseTrans *trans)
{
  BseSource *source = cast (this);
  if (BSE_SOURCE_PREPARED (source))
    {
      SynthesisModule::Closure *clo = make_module_config_closure();
      if (clo)
        {
          BseTrans *atrans = trans ? trans : bse_trans_open();
          bse_source_access_modules (source, access_trampoline, clo, access_data_free, atrans);
          if (!trans)
            last_module_update = bse_trans_commit (atrans);
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
  /* assert validity of the above casts */
  RAPICORN_STATIC_ASSERT (sizeof   (JStream)                    == sizeof   (BseJStream));
  RAPICORN_STATIC_ASSERT (offsetof (JStream, values)            == offsetof (BseJStream, values));
  RAPICORN_STATIC_ASSERT (sizeof (((JStream*)0)->values)        == sizeof (((BseJStream*)0)->values));
  RAPICORN_STATIC_ASSERT (offsetof (JStream, n_connections)     == offsetof (BseJStream, n_connections));
  RAPICORN_STATIC_ASSERT (sizeof (((JStream*)0)->n_connections) == sizeof (((BseJStream*)0)->n_connections));
  RAPICORN_STATIC_ASSERT (offsetof (JStream, jcount)            == offsetof (BseJStream, jcount));
  RAPICORN_STATIC_ASSERT (sizeof (((JStream*)0)->jcount)        == sizeof (((BseJStream*)0)->jcount));
  RAPICORN_STATIC_ASSERT (sizeof   (IStream)                == sizeof   (BseIStream));
  RAPICORN_STATIC_ASSERT (offsetof (IStream, values)        == offsetof (BseIStream, values));
  RAPICORN_STATIC_ASSERT (sizeof (((IStream*)0)->values)    == sizeof (((BseIStream*)0)->values));
  RAPICORN_STATIC_ASSERT (offsetof (IStream, connected)     == offsetof (BseIStream, connected));
  RAPICORN_STATIC_ASSERT (sizeof (((IStream*)0)->connected) == sizeof (((BseIStream*)0)->connected));
  RAPICORN_STATIC_ASSERT (sizeof   (OStream)                == sizeof   (BseOStream));
  RAPICORN_STATIC_ASSERT (offsetof (OStream, values)        == offsetof (BseOStream, values));
  RAPICORN_STATIC_ASSERT (sizeof (((OStream*)0)->values)    == sizeof (((BseOStream*)0)->values));
  RAPICORN_STATIC_ASSERT (offsetof (OStream, connected)     == offsetof (BseOStream, connected));
  RAPICORN_STATIC_ASSERT (sizeof (((OStream*)0)->connected) == sizeof (((BseOStream*)0)->connected));
}
void
SynthesisModule::ostream_set (uint         ostream_index,
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
static void
process_module (BseModule *engine_module,
                guint      n_values)
{
  SynthesisModule *m = static_cast<SynthesisModule*> (engine_module->user_data);
  m->process (n_values);
}
static void
reset_module (BseModule *engine_module)
{
  SynthesisModule *m = static_cast<SynthesisModule*> (engine_module->user_data);
  m->reset();
}
static void
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
struct HandlerData {
  SynthesisModule::AutoUpdate auto_update;
  double minimum, maximum;
  bool   bool_quantize;
  Effect *effect;
  HandlerData() :
    auto_update (NULL),
    minimum (0),
    maximum (0),
    bool_quantize (false),
    effect (NULL)
  {
  }
  static void
  destroy (gpointer data)
  {
    HandlerData *hd = reinterpret_cast<HandlerData*> (data);
    delete hd;
  }
};
static void
auto_update_data_free (gpointer data)   /* UserThread */
{
  SynthesisModule::AutoUpdateData *adata = static_cast<SynthesisModule::AutoUpdateData*> (data);
  GObject *object = cast (static_cast<CxxBase*> (adata->effect)); // FIXME: rename cast() and get rid of static_cast<>()
  GObjectClass *klass = (GObjectClass*) g_type_class_peek (adata->pspec->owner_type);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (klass);
  if (source_class->property_updated)
    {
      GParamSpec *pspec = g_param_spec_get_redirect_target (adata->pspec);
      if (!pspec)
        pspec = adata->pspec;
      source_class->property_updated ((BseSource*) object, adata->prop_id, adata->tick_stamp, adata->prop_value, pspec);
      g_object_notify (object, pspec->name);
    }
  delete adata;
}
static void
midi_control_handler (gpointer                  handler_data,  /* MIDI Device Thread (and possibly others) */
                      guint64                   tick_stamp,
                      BseMidiSignalType         signal_type,
                      gfloat                    control_value, /* -1 .. +1 */
                      guint                     n_mcdatas,
                      BseModule          *const*modules,
                      gpointer                  user_data,
                      BseTrans                 *trans)
{
  GParamSpec *pspec = static_cast<GParamSpec*> (handler_data);
  g_return_if_fail (n_mcdatas > 0);
  if (!user_data)
    return;     /* ignore events before bse_midi_receiver_set_control_handler_data() */
  HandlerData *hd = static_cast<HandlerData*> (user_data);
  SynthesisModule::AutoUpdateData *adata = new SynthesisModule::AutoUpdateData;
  adata->prop_id = pspec->param_id;
  if (hd->bool_quantize)
    adata->prop_value = control_value >= 0.5 ? 1 : 0;
  else
    adata->prop_value = 0.5 * (hd->minimum + hd->maximum + control_value * (hd->maximum - hd->minimum));
  adata->tick_stamp = tick_stamp;
  adata->pspec = pspec;
  adata->effect = hd->effect;
  for (guint i = 0; i < n_mcdatas; i++)
    bse_trans_add (trans,
                   bse_job_flow_access (modules[i],
                                        tick_stamp,
                                        hd->auto_update,
                                        adata,
                                        i + 1 >= n_mcdatas ? auto_update_data_free : NULL));
}
static void
get_midi_control_range (GParamSpec *pspec,
                        double     &minimum,
                        double     &maximum,
                        bool       &bool_quantize)
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
      bool_quantize = true;
    }
  else if (SFI_IS_PSPEC_NUM (pspec))
    {
      SfiParamSpecNum *nspec = SFI_PSPEC_NUM (pspec);
      minimum = nspec->minimum;
      maximum = nspec->maximum;
    }
}
struct HandlerSetup {
  Effect                *effect;
  bool                   add_handler;
  guint                  n_aprops;
  BseAutomationProperty *aprops;
  BseMidiReceiver       *midi_receiver;
  guint                  midi_channel;
  static void
  free (gpointer data)  /* User Thread */
  {
    HandlerSetup *hs = reinterpret_cast<HandlerSetup*> (data);
    g_free (hs->aprops);
    g_free (hs);
  }
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
        bool has_data = bse_midi_receiver_add_control_handler (hs->midi_receiver,
                                                               hs->aprops[i].midi_channel ? hs->aprops[i].midi_channel : hs->midi_channel,
                                                               hs->aprops[i].signal_type,
                                                               midi_control_handler,
                                                               hs->aprops[i].pspec,
                                                               module);
        if (!has_data)
          {
            HandlerData *hd = new HandlerData;
            get_midi_control_range (hs->aprops[i].pspec, hd->minimum, hd->maximum, hd->bool_quantize);
            hd->auto_update = hs->effect->get_module_auto_update();
            hd->effect = hs->effect;
            bse_midi_receiver_set_control_handler_data (hs->midi_receiver,
                                                        hs->aprops[i].midi_channel ? hs->aprops[i].midi_channel : hs->midi_channel,
                                                        hs->aprops[i].signal_type,
                                                        midi_control_handler,
                                                        hs->aprops[i].pspec,
                                                        hd, HandlerData::destroy);
          }
      }
    else
      bse_midi_receiver_remove_control_handler (hs->midi_receiver,
                                                hs->aprops[i].midi_channel ? hs->aprops[i].midi_channel : hs->midi_channel,
                                                hs->aprops[i].signal_type,
                                                midi_control_handler,
                                                hs->aprops[i].pspec,
                                                module);
}
BseModule*
Effect::integrate_engine_module (uint           context_handle,
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
      hs->effect = this;
      hs->add_handler = true;
      hs->n_aprops = n_props;
      hs->aprops = aprops;
      BseMidiContext mc = bse_snet_get_midi_context (bse_item_get_snet (BSE_ITEM (source)), context_handle);
      hs->midi_receiver = mc.midi_receiver;
      hs->midi_channel = mc.midi_channel;
      bse_trans_add (trans, bse_job_access (engine_module, handler_setup_func, hs, HandlerSetup::free));
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
          hs->effect = this;
          hs->add_handler = false;
          hs->n_aprops = n_props;
          hs->aprops = aprops;
          BseMidiContext mc = bse_snet_get_midi_context (bse_item_get_snet (BSE_ITEM (source)), context_handle);
          hs->midi_receiver = mc.midi_receiver;
          hs->midi_channel = mc.midi_channel;
          bse_trans_add (trans, bse_job_access (engine_module, handler_setup_func, hs, HandlerSetup::free));
        }
      /* discard module */
      bse_trans_add (trans, bse_job_discard (engine_module));
    }
}
uint
Effect::block_size() const
{
  g_return_val_if_fail (is_prepared(), 0);
  return bse_engine_block_size();
}
uint
Effect::max_block_size() const
{
  return BSE_STREAM_MAX_VALUES;
}
BseMusicalTuningType
Effect::current_musical_tuning() const
{
  BseSource *source = cast (const_cast <Effect*> (this));
  return bse_item_current_musical_tuning (BSE_ITEM (source));
}
void
Effect::class_init (CxxBaseClass *klass)
{
  static gpointer effect_parent_class = NULL;
  struct Trampoline
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
      SynthesisModule::Closure *clo = self->make_module_config_closure();
      if (clo)
        bse_trans_add (trans, bse_job_access (engine_module, access_trampoline, clo, access_data_free));
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
  source_class->context_create = Trampoline::effect_context_create;
  source_class->context_dismiss = Trampoline::effect_context_dismiss;
  source_class->prepare = Trampoline::effect_prepare;
  source_class->reset = Trampoline::effect_reset;
}
} // Bse
