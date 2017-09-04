// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsepcminput.hh"

#include "bsecategories.hh"
#include "bseserver.hh"
#include "bseengine.hh"



/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_MVOLUME_f,
  PARAM_MVOLUME_dB,
  PARAM_MVOLUME_PERC
};


/* --- prototypes --- */
static void	 bse_pcm_input_init		(BsePcmInput		*scard);
static void	 bse_pcm_input_class_init	(BsePcmInputClass	*klass);
static void	 bse_pcm_input_class_finalize	(BsePcmInputClass	*klass);
static void	 bse_pcm_input_set_property	(GObject		*object,
						 guint			 param_id,
						 const GValue		*value,
						 GParamSpec		*pspec);
static void	 bse_pcm_input_get_property	(GObject                *object,
						 guint			 param_id,
						 GValue			*value,
						 GParamSpec		*pspec);
static void	 bse_pcm_input_prepare		(BseSource		*source);
static void	 bse_pcm_input_context_create	(BseSource		*source,
						 guint			 instance_id,
						 BseTrans		*trans);
static void	 bse_pcm_input_context_connect	(BseSource		*source,
						 guint			 instance_id,
						 BseTrans		*trans);
static void	 bse_pcm_input_reset		(BseSource		*source);


/* --- variables --- */
static gpointer		 parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BsePcmInput)
{
  static const GTypeInfo pcm_input_info = {
    sizeof (BsePcmInputClass),

    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_pcm_input_class_init,
    (GClassFinalizeFunc) bse_pcm_input_class_finalize,
    NULL /* class_data */,

    sizeof (BsePcmInput),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_pcm_input_init,
  };
#include "./icons/mic.c"
  GType type = bse_type_register_static (BSE_TYPE_SOURCE,
                                         "BsePcmInput",
                                         "Stereo PCM sound input module, per default, signals from this "
                                         "module originate from recording on the standard soundcard",
                                         __FILE__, __LINE__,
                                         &pcm_input_info);
  bse_categories_register_stock_module (N_("/Input & Output/PCM Input"), type, mic_pixstream);
  return type;
}

static void
bse_pcm_input_class_init (BsePcmInputClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (klass);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (klass);
  guint ochannel_id;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->set_property = bse_pcm_input_set_property;
  gobject_class->get_property = bse_pcm_input_get_property;

  source_class->prepare = bse_pcm_input_prepare;
  source_class->context_create = bse_pcm_input_context_create;
  source_class->context_connect = bse_pcm_input_context_connect;
  source_class->reset = bse_pcm_input_reset;

  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_MVOLUME_f,
			      sfi_pspec_real ("gain_volume_f", "Input Gain [float]", NULL,
					      bse_db_to_factor (0),
					      0, bse_db_to_factor (BSE_MAX_VOLUME_dB),
					      0.1,
					      SFI_PARAM_STORAGE ":skip-default")); // FIXME: don't skip-default
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_MVOLUME_dB,
			      sfi_pspec_real ("gain_volume_dB", "Input Gain [dB]", NULL,
					      0,
					      BSE_MIN_VOLUME_dB, BSE_MAX_VOLUME_dB,
                                              0.1,
					      SFI_PARAM_GUI ":dial"));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_MVOLUME_PERC,
			      sfi_pspec_int ("gain_volume_perc", "input Gain [%]", NULL,
					     bse_db_to_factor (0) * 100,
					     0, bse_db_to_factor (BSE_MAX_VOLUME_dB) * 100,
					     1, SFI_PARAM_GUI ":dial"));

  ochannel_id = bse_source_class_add_ochannel (source_class, "left-audio-out", _("Left Audio Out"), _("Left channel output"));
  assert_return (ochannel_id == BSE_PCM_INPUT_OCHANNEL_LEFT);
  ochannel_id = bse_source_class_add_ochannel (source_class, "right-audio-out", _("Right Audio Out"), _("Right channel output"));
  assert_return (ochannel_id == BSE_PCM_INPUT_OCHANNEL_RIGHT);
}

static void
bse_pcm_input_class_finalize (BsePcmInputClass *klass)
{
}

static void
bse_pcm_input_init (BsePcmInput *iput)
{
  iput->volume_factor = bse_db_to_factor (0);
}

static void
bse_pcm_input_set_property (GObject      *object,
			    guint         param_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
  BsePcmInput *self = BSE_PCM_INPUT (object);
  switch (param_id)
    {
    case PARAM_MVOLUME_f:
      self->volume_factor = sfi_value_get_real (value);
      g_object_notify ((GObject*) self, "gain_volume_dB");
      g_object_notify ((GObject*) self, "gain_volume_perc");
      break;
    case PARAM_MVOLUME_dB:
      self->volume_factor = bse_db_to_factor (sfi_value_get_real (value));
      g_object_notify ((GObject*) self, "gain_volume_f");
      g_object_notify ((GObject*) self, "gain_volume_perc");
      break;
    case PARAM_MVOLUME_PERC:
      self->volume_factor = sfi_value_get_int (value) / 100.0;
      g_object_notify ((GObject*) self, "gain_volume_f");
      g_object_notify ((GObject*) self, "gain_volume_dB");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
bse_pcm_input_get_property (GObject    *object,
			    guint       param_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
  BsePcmInput *self = BSE_PCM_INPUT (object);
  switch (param_id)
    {
    case PARAM_MVOLUME_f:
      sfi_value_set_real (value, self->volume_factor);
      break;
    case PARAM_MVOLUME_dB:
      sfi_value_set_real (value, bse_db_from_factor (self->volume_factor, BSE_MIN_VOLUME_dB));
      break;
    case PARAM_MVOLUME_PERC:
      sfi_value_set_int (value, self->volume_factor * 100.0 + 0.5);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
bse_pcm_input_prepare (BseSource *source)
{
  BsePcmInput *iput = BSE_PCM_INPUT (source);

  iput->uplink = bse_server_retrieve_pcm_input_module (bse_server_get (), source, "MasterIn");

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

typedef struct {
  gfloat             volume;
  gboolean           volume_set;
} ModData;

static void
pcm_input_process (BseModule *module,
		   guint      n_values)
{
  ModData *mdata = (ModData*) module->user_data;
  const gfloat *ls = BSE_MODULE_IBUFFER (module, BSE_PCM_INPUT_OCHANNEL_LEFT);
  const gfloat *rs = BSE_MODULE_IBUFFER (module, BSE_PCM_INPUT_OCHANNEL_RIGHT);
  gfloat *ld = BSE_MODULE_OBUFFER (module, BSE_PCM_INPUT_OCHANNEL_LEFT);
  gfloat *rd = BSE_MODULE_OBUFFER (module, BSE_PCM_INPUT_OCHANNEL_RIGHT);
  gfloat v = mdata->volume;

  if (mdata->volume_set)
    while (n_values--)
      {
	*ld++ = v * *ls++;
	*rd++ = v * *rs++;
      }
  else
    {
      BSE_MODULE_OBUFFER (module, BSE_PCM_INPUT_OCHANNEL_LEFT) = (gfloat*) BSE_MODULE_IBUFFER (module, BSE_PCM_INPUT_OCHANNEL_LEFT);
      BSE_MODULE_OBUFFER (module, BSE_PCM_INPUT_OCHANNEL_RIGHT) = (gfloat*) BSE_MODULE_IBUFFER (module, BSE_PCM_INPUT_OCHANNEL_RIGHT);
    }
}

static void
bse_pcm_input_context_create (BseSource *source,
			      guint      context_handle,
			      BseTrans  *trans)
{
  static const BseModuleClass pcm_input_mclass = {
    BSE_PCM_INPUT_N_OCHANNELS,	/* n_istreams */
    0,				/* n_jstreams */
    BSE_PCM_INPUT_N_OCHANNELS,	/* n_ostreams */
    pcm_input_process,		/* process */
    NULL,                       /* process_defer */
    NULL,                       /* reset */
    (BseModuleFreeFunc) g_free,	/* free */
    BSE_COST_CHEAP,		/* cost */
  };
  ModData *mdata = g_new0 (ModData, 1);
  BseModule *module = bse_module_new (&pcm_input_mclass, mdata);

  mdata->volume = 1.0;
  mdata->volume_set = mdata->volume != 1.0;

  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_omodule (source, context_handle, module);

  /* commit module to engine */
  bse_trans_add (trans, bse_job_integrate (module));

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

static void
bse_pcm_input_context_connect (BseSource *source,
			       guint      context_handle,
			       BseTrans  *trans)
{
  BsePcmInput *iput = BSE_PCM_INPUT (source);
  BseModule *module;

  /* get context specific module */
  module = bse_source_get_context_omodule (source, context_handle);

  /* connect module to server uplink */
  bse_trans_add (trans, bse_job_connect (iput->uplink, 0, module, 0));
  bse_trans_add (trans, bse_job_connect (iput->uplink, 1, module, 1));

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_connect (source, context_handle, trans);
}

static void
bse_pcm_input_reset (BseSource *source)
{
  BsePcmInput *iput = BSE_PCM_INPUT (source);

  bse_server_discard_pcm_input_module (bse_server_get (), iput->uplink);
  iput->uplink = NULL;

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}
