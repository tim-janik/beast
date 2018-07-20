// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsesnooper.hh"

#include <bse/bseengine.hh>
#include <bse/bseieee754.hh>
#include <bse/bsecategories.hh>


enum {
  PARAM_0,
  PARAM_CONTEXT_ID
};


/* --- prototypes --- */
static void	bse_snooper_init		(BseSnooper		*snooper);
static void	bse_snooper_class_init		(BseSnooperClass	*klass);


/* --- variables --- */
static gpointer		 parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseSnooper)
{
  static const GTypeInfo type_info = {
    sizeof (BseSnooperClass),

    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_snooper_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,

    sizeof (BseSnooper),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_snooper_init,
  };
#include "./icons/snooper.c"
  GType type_id;

  type_id = bse_type_register_static (BSE_TYPE_SOURCE,
				      "BseSnooper",
				      "The Snooper module prints statistics about the incoming signal",
                                      __FILE__, __LINE__,
                                      &type_info);
  bse_categories_register_stock_module (N_("/Misc/Snooper"), type_id, snooper_pixstream);

  return type_id;
}

static void
bse_snooper_init (BseSnooper *snooper)
{
  snooper->module = NULL;
}

static void
bse_snooper_finalize (GObject *object)
{
  BseSnooper *snooper = BSE_SNOOPER (object);
  assert_return (snooper->module == NULL);
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bse_snooper_set_property (GObject      *object,
			  guint         param_id,
			  const GValue *value,
			  GParamSpec   *pspec)
{
  // BseSnooper *snooper = BSE_SNOOPER (object);
  switch (param_id)
    {
    case PARAM_CONTEXT_ID:
      // snooper->active_context_id = sfi_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
bse_snooper_get_property (GObject    *object,
			  guint       param_id,
			  GValue     *value,
			  GParamSpec *pspec)
{
  // BseSnooper *snooper = BSE_SNOOPER (object);
  switch (param_id)
    {
    case PARAM_CONTEXT_ID:
      // sfi_value_set_int (value, snooper->active_context_id);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static gboolean
bse_snooper_needs_storage (BseItem    *item,
                           BseStorage *storage)
{
  // BseSnooper *self = BSE_SNOOPER (item);
  return FALSE;
}

typedef struct {
  uint           ref_count;
  uint           context_id;
} SnoopData;

static void
snooper_process (BseModule *module,
		 guint      n_values)
{
  const BseJStream &jstream = BSE_MODULE_JSTREAM (module, 0);
  if (jstream.n_connections)
    {
      const float *wave_in = jstream.values[0];
      float min = wave_in[0], max = wave_in[0];
      float avg = wave_in[0], first = wave_in[0], last = wave_in[n_values - 1];
      bool seen_nan = false, seen_pinf = false, seen_ninf = false, seen_subn = false;

      for (uint j = 0; j < jstream.n_connections; j++)
        for (uint i = 0; i < n_values; i++)
          {
            const float v = wave_in[i];
            max = MAX (max, v);
            min = MIN (min, v);
            avg += v;
            if (UNLIKELY (BSE_FLOAT_IS_NANINF (v)))
              {
                seen_nan |= BSE_FLOAT_IS_NAN (v);
                seen_pinf |= BSE_FLOAT_IS_INF_POSITIVE (v);
                seen_ninf |= BSE_FLOAT_IS_INF_NEGATIVE (v);
              }
            else if (UNLIKELY (BSE_FLOAT_IS_SUBNORMAL (v)))
              seen_subn = true;
          }
      avg /= double (n_values) * jstream.n_connections;
      Bse::printout ("Snooper: max=%+1.5f min=%+1.5f avg=%+1.5f cons=%u values=%u [%+1.5f,..,%+1.5f] freq=%+1.2f %s%s%s%s\r",
                     max, min, avg,
                     jstream.n_connections, n_values,
                     first, last,
                     BSE_FREQ_FROM_VALUE (avg),
                     seen_nan ? " +NAN" : "",
                     seen_pinf ? " +PINF" : "",
                     seen_ninf ? " +NINF" : "",
                     seen_subn ? " +SUBNORM" : "");
    }
}

static void
bse_snooper_context_create (BseSource *source,
			    guint      context_handle,
			    BseTrans  *trans)
{
  static const BseModuleClass snooper_class = {
    0,                          // n_istreams
    BSE_SNOOPER_N_JCHANNELS,    // n_jstreams
    0,                          // n_ostreams
    snooper_process,            // process
    NULL,                       // process_defer
    NULL,                       // reset
    (BseModuleFreeFunc) g_free, // free
    Bse::ModuleFlag::CHEAP,             // mflags
  };
  BseSnooper *snooper = BSE_SNOOPER (source);

  SnoopData *snoop_data = NULL;
  if (!snooper->module)
    {
      snoop_data = g_new0 (SnoopData, 1);
      snoop_data->ref_count = 0;
      snooper->module = bse_module_new (&snooper_class, snoop_data);
      /* commit module to engine */
      bse_trans_add (trans, bse_job_integrate (snooper->module));
      bse_trans_add (trans, bse_job_set_consumer (snooper->module, TRUE));
    }
  else
    snoop_data = (SnoopData*) snooper->module->user_data;
  snoop_data->ref_count += 1;

  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_imodule (source, context_handle, snooper->module, trans);

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

static void
bse_snooper_context_dismiss (BseSource     *source,
                             uint           context_handle,
                             BseTrans      *trans)
{
  BseSnooper *snooper = BSE_SNOOPER (source);
  BseModule *module = bse_source_get_context_imodule (source, context_handle);
  assert_return (module != NULL);
  assert_return (module == snooper->module);
  SnoopData *snoop_data = (SnoopData*) module->user_data;
  assert_return (snoop_data->ref_count > 0);
  snoop_data->ref_count -= 1;
  if (snoop_data->ref_count)    /* prevent automatic discarding from engine */
    bse_source_set_context_imodule (source, context_handle, NULL, trans);
  else
    snooper->module = NULL; // module will be auto destroyed by BseSource->context_dismiss()
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_dismiss (source, context_handle, trans);
}

static void
bse_snooper_class_init (BseSnooperClass *klass)
{

  parent_class = g_type_class_peek_parent (klass);

  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = bse_snooper_finalize;
  gobject_class->set_property = bse_snooper_set_property;
  gobject_class->get_property = bse_snooper_get_property;

  BseItemClass *item_class = BSE_ITEM_CLASS (klass);
  item_class->needs_storage = bse_snooper_needs_storage;

  BseSourceClass *source_class = BSE_SOURCE_CLASS (klass);
  source_class->context_create = bse_snooper_context_create;
  source_class->context_dismiss = bse_snooper_context_dismiss;

  BseObjectClass *object_class = BSE_OBJECT_CLASS (klass);
  if (0) // unused
    bse_object_class_add_param (object_class, "Context",
                                PARAM_CONTEXT_ID,
                                sfi_pspec_int ("context_id", "Context",
                                               "If the snooper module is created multiple times, this is "
                                               "the context id, which is used to actually snoop data.",
                                               0, 0, 65535, 1,
                                               SFI_PARAM_STANDARD));

  const uint jchannel = bse_source_class_add_jchannel (source_class, "signal-in", _("Signal In"), _("Signal to snoop"));
  assert_return (jchannel == BSE_SNOOPER_JCHANNEL_MONO);
}
