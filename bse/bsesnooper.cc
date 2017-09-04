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
static void	 bse_snooper_init		(BseSnooper		*snooper);
static void	 bse_snooper_class_init		(BseSnooperClass	*klass);
static void      bse_snooper_set_property       (GObject		*object,
						 guint                   param_id,
						 const GValue           *value,
						 GParamSpec             *pspec);
static void      bse_snooper_get_property       (GObject                *object,
						 guint                   param_id,
						 GValue                 *value,
						 GParamSpec             *pspec);
static gboolean  bse_snooper_needs_storage      (BseItem                *item,
                                                 BseStorage             *storage);
static void	 bse_snooper_context_create	(BseSource		*source,
						 guint			 context_handle,
						 BseTrans		*trans);


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
bse_snooper_class_init (BseSnooperClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (klass);
  BseItemClass *item_class = BSE_ITEM_CLASS (klass);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (klass);
  guint ichannel;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->set_property = bse_snooper_set_property;
  gobject_class->get_property = bse_snooper_get_property;

  item_class->needs_storage = bse_snooper_needs_storage;

  source_class->context_create = bse_snooper_context_create;

  bse_object_class_add_param (object_class, "Context",
			      PARAM_CONTEXT_ID,
			      sfi_pspec_int ("context_id", "Context",
					     "If the snooper module is created multiple times, this is "
					     "the context id, which is used to actually snoop data.",
					     0, 0, 65535, 1,
					     SFI_PARAM_STANDARD));

  ichannel = bse_source_class_add_ichannel (source_class, "signal-in", _("Signal In"), _("Snoop Signal"));
  assert_return (ichannel == BSE_SNOOPER_ICHANNEL_MONO);
}

static void
bse_snooper_init (BseSnooper *snooper)
{
  snooper->active_context_id = 0;
}

static void
bse_snooper_set_property (GObject      *object,
			  guint         param_id,
			  const GValue *value,
			  GParamSpec   *pspec)
{
  BseSnooper *snooper = BSE_SNOOPER (object);
  switch (param_id)
    {
    case PARAM_CONTEXT_ID:
      snooper->active_context_id = sfi_value_get_int (value);
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
  BseSnooper *snooper = BSE_SNOOPER (object);
  switch (param_id)
    {
    case PARAM_CONTEXT_ID:
      sfi_value_set_int (value, snooper->active_context_id);
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
  guint           context_id;
  volatile guint *active_context_id;
} SnoopData;

static void
snooper_process (BseModule *module,
		 guint      n_values)
{
  const gfloat *wave_in = BSE_MODULE_IBUFFER (module, 0);
  SnoopData *data = (SnoopData*) module->user_data;

  if (data->context_id == *data->active_context_id &&
      module->istreams[0].connected)
    {
      gfloat min = wave_in[0], max = wave_in[0];
      gfloat avg = wave_in[0], first = wave_in[0], last = wave_in[n_values - 1];
      gboolean seen_nan = FALSE, seen_pinf = FALSE, seen_ninf = FALSE, seen_subn = FALSE;

      for (uint i = 1; i < n_values; i++)
	{
	  float v = wave_in[i];
	  max = MAX (max, v);
	  min = MIN (min, v);
	  avg += v;
	  max = MAX (max, v);
	  if (UNLIKELY (BSE_FLOAT_IS_NANINF (v)))
	    {
	      seen_nan |= BSE_FLOAT_IS_NAN (v);
	      seen_pinf |= BSE_FLOAT_IS_INF_POSITIVE (v);
	      seen_ninf |= BSE_FLOAT_IS_INF_POSITIVE (v);
	    }
	  else if (UNLIKELY (BSE_FLOAT_IS_SUBNORMAL (v)))
            seen_subn = TRUE;
	}
      avg /= (gdouble) n_values;
      Bse::printout ("C%2u: max=%+1.5f min=%+1.5f avg=%+1.5f %u[%+1.5f,..,%+1.5f] freq=%+1.2f %s%s%s%s\r",
                     data->context_id,
                     max, min, avg,
                     n_values,
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
    BSE_SNOOPER_N_ICHANNELS,    // n_istreams
    0,                          // n_jstreams
    0,                          // n_ostreams
    snooper_process,            // process
    NULL,                       // process_defer
    NULL,                       // reset
    (BseModuleFreeFunc) g_free, // free
    BSE_COST_CHEAP,             // mflags
  };
  BseSnooper *snooper = BSE_SNOOPER (source);
  SnoopData *data = g_new0 (SnoopData, 1);
  BseModule *module;

  data->context_id = context_handle;
  data->active_context_id = &snooper->active_context_id;
  module = bse_module_new (&snooper_class, data);

  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);

  /* commit module to engine */
  bse_trans_add (trans, bse_job_integrate (module));
  bse_trans_add (trans, bse_job_set_consumer (module, TRUE));

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}
