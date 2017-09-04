// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseladspamodule.hh"
#include "bseblockutils.hh"
#include "bsecategories.hh"
#include "bseengine.hh"
#include "bsemathsignal.hh"
#include <string.h>


/* --- prototypes --- */
static void	bse_ladspa_module_class_init	 (BseLadspaModuleClass	*klass);
static void     ladspa_derived_init		 (BseLadspaModule	*self);
static void     ladspa_derived_finalize		 (GObject		*object);
static void	ladspa_derived_set_property	 (GObject		*object,
						  uint			 param_id,
						  const GValue		*value,
						  GParamSpec		*pspec);
static void	ladspa_derived_get_property	 (GObject		*object,
						  uint			 param_id,
						  GValue		*value,
						  GParamSpec		*pspec);
static void	ladspa_derived_context_create	 (BseSource		*source,
						  uint			 context_handle,
						  BseTrans		*trans);
static void	bse_ladspa_module_class_init_from_info (BseLadspaModuleClass *ladspa_module_class);


/* --- variables --- */
static void    *derived_parent_class = NULL;
static GQuark   quark_value_index = 0;
static GQuark   quark_notify_sibling = 0;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseLadspaModule)
{
  static const GTypeInfo type_info = {
    sizeof (BseLadspaModuleClass),

    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_ladspa_module_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,

    sizeof (BseLadspaModule),
    0 /* n_preallocs */,
    (GInstanceInitFunc) NULL,
  };
  GType type;

  type = bse_type_register_static (BSE_TYPE_SOURCE,
				   "BseLadspaModule",
				   "LADSPA Module base type",
                                   __FILE__, __LINE__,
                                   &type_info);
  return type;
}

static void
bse_ladspa_module_class_init (BseLadspaModuleClass *klass)
{
  quark_value_index = g_quark_from_static_string ("BseLadspaValueIndex");
  quark_notify_sibling = g_quark_from_static_string ("BseLadspaNotifySibling");

  klass->bli = NULL;
}

static void
ladspa_derived_class_init (BseLadspaModuleClass *klass,
			   void                 *class_data)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (klass);

  derived_parent_class = g_type_class_peek_parent (klass);

  assert_return (class_data != NULL);
  klass->bli = (BseLadspaInfo*) class_data;

  gobject_class->finalize = ladspa_derived_finalize;

  source_class->context_create = ladspa_derived_context_create;

  bse_ladspa_module_class_init_from_info (klass);
}

static void
ladspa_derived_class_finalize (BseLadspaModuleClass *klass,
			       void                 *class_data)
{
  g_free (klass->gsl_class);
}

void
bse_ladspa_module_derived_type_info (GType                  type,
				     BseLadspaInfo         *bli,
				     GTypeInfo             *type_info)
{
  assert_return (bli != NULL);
  type_info->class_size = sizeof (BseLadspaModuleClass);
  type_info->class_init = (GClassInitFunc) ladspa_derived_class_init;
  type_info->class_finalize = (GClassFinalizeFunc) ladspa_derived_class_finalize;
  type_info->class_data = bli;
  type_info->instance_size = sizeof (BseLadspaModule);
  type_info->instance_init = (GInstanceInitFunc) ladspa_derived_init;
}

static void
bse_ladspa_module_class_init_from_info (BseLadspaModuleClass *ladspa_module_class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (ladspa_module_class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (ladspa_module_class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (ladspa_module_class);
  BseLadspaInfo *bli = ladspa_module_class->bli;

  assert_return (ladspa_module_class->bli != NULL &&
                 gobject_class->set_property == NULL &&
                 gobject_class->get_property == NULL);

  gobject_class->set_property = ladspa_derived_set_property;
  gobject_class->get_property = ladspa_derived_get_property;

  for (size_t i = 0; i < bli->n_cports; i++)
    {
      BseLadspaPort *port = bli->cports + i;
      GParamSpec *pspec, *pspec2 = NULL;
      const char *group;
      // printout ("LADSPA-PORT: %s\n", bse_ladspa_info_port_2str (port));
      if (port->boolean)
	{
	  group = "Switches";
	  pspec = sfi_pspec_bool (port->ident, port->name, NULL,
				  port->default_value, SFI_PARAM_GUI);
	}
      else if (port->integer_stepping)
	{
	  const char *hints;
	  /* try to guess when scales are going to be useful */
	  if (port->minimum < 0 ||
	      port->maximum - port->minimum > 10)
	    hints = SFI_PARAM_GUI ":scale";
	  else
	    hints = SFI_PARAM_GUI;
	  group = "Adjustments";
	  pspec = sfi_pspec_int (port->ident, port->name, NULL,
				 port->default_value, port->minimum, port->maximum, 1,
				 hints);
	}
      else if (port->frequency)
	{
	  float maximum = port->maximum;
	  float minimum = port->minimum;
	  float dfvalue = port->default_value;
	  if (port->rate_relative)
	    {
	      /* we relate a maximum value of 0.5 (sample_freq/2) to BSE_MAX_OSC_FREQUENCY */
	      maximum *= 2.0 * BSE_MAX_OSC_FREQUENCY;
	      minimum *= 2.0 * BSE_MAX_OSC_FREQUENCY;
	      dfvalue *= 2.0 * BSE_MAX_OSC_FREQUENCY;
	    }
	  if (port->concert_a)
	    dfvalue = BSE_KAMMER_FREQUENCY;
	  minimum = CLAMP (minimum, BSE_MIN_OSC_FREQUENCY, BSE_MAX_OSC_FREQUENCY);
	  maximum = CLAMP (maximum, BSE_MIN_OSC_FREQUENCY, BSE_MAX_OSC_FREQUENCY);
	  dfvalue = CLAMP (dfvalue, minimum, maximum);
	  group = "Frequencies";
	  pspec = sfi_pspec_log_scale (port->ident, port->name, NULL,
				       dfvalue, minimum, maximum, 10.0,
				       2 * BSE_KAMMER_FREQUENCY, 2, 4,
				       SFI_PARAM_GUI ":f:scale:dial");
	  if (port->concert_a)
	    {
	      /* when defaulting to A', we probably have note-aligned port values */
	      int min_note = bse_note_from_freq_bounded (Bse::MusicalTuning::OD_12_TET, minimum);
	      int max_note = bse_note_from_freq_bounded (Bse::MusicalTuning::OD_12_TET, maximum);
	      if (max_note - min_note > 2)
		{
		  char *ident2 = g_strconcat (port->ident, "-note", NULL);
		  pspec2 = sfi_pspec_note (ident2, port->name, _("Note values are converted to Hertz according to the current musical tuning"),
					   BSE_KAMMER_NOTE, min_note, max_note, FALSE,
					   SFI_PARAM_GUI);
		  g_param_spec_set_qdata (pspec2, quark_notify_sibling, pspec);
		  g_param_spec_set_qdata (pspec, quark_notify_sibling, pspec2);
		  g_free (ident2);
		}
	    }
	}
      else /* normal float */
	{
	  float stepping;
	  if (port->maximum - port->minimum > 3 * 10.0)
	    stepping = 10.0;
	  else if (port->maximum - port->minimum > 3 * 1.0)
	    stepping = 1.0;
	  else
	    stepping = 0;
	  group = "Adjustments";
	  pspec = sfi_pspec_real (port->ident, port->name, NULL,
				  port->default_value, port->minimum, port->maximum, stepping,
				  SFI_PARAM_GUI ":f:scale");
	}
      if (port->input)
	sfi_pspec_add_option (pspec, "S", "+");         /* serializable */
      else /* port->output */
	sfi_pspec_add_option (pspec, "ro", "+");        /* read-only at the GUI */
      bse_object_class_add_param (object_class, group, i + 1, pspec);
      if (pspec2)
	{
	  g_param_spec_set_qdata (pspec2, quark_value_index, (void *) i);
	  if (port->output)
	    sfi_pspec_add_option (pspec2, "ro", "+");   /* read-only at the GUI */
	  bse_object_class_add_param (object_class, group, bli->n_cports + i + 1, pspec2);
	}
    }

  uint ochannel, ichannel;
  for (size_t i = 0; i < bli->n_aports; i++)
    {
      BseLadspaPort *port = bli->aports + i;
      if (port->input)
	ichannel = bse_source_class_add_ichannel (source_class, port->ident, port->name, NULL);
      else /* port->output */
	ochannel = bse_source_class_add_ochannel (source_class, port->ident, port->name, NULL);
    }
  (void) ochannel;
  (void) ichannel;
}

static float
ladspa_value_get_float (BseLadspaModule *self,
                        const GValue    *value,
			BseLadspaPort   *port)
{
  switch (sfi_categorize_type (G_VALUE_TYPE (value)))
    {
    case SFI_SCAT_BOOL:
      return sfi_value_get_bool (value);
    case SFI_SCAT_INT:
      if (port->frequency && port->concert_a)	/* is note */
	return bse_note_to_freq (bse_item_current_musical_tuning (BSE_ITEM (self)), sfi_value_get_int (value));
      else
	return sfi_value_get_int (value);
    case SFI_SCAT_REAL:
      return sfi_value_get_real (value);
    default:
      assert_return_unreached (0);
      return 0;
    }
}

static void
ladspa_value_set_float (BseLadspaModule *self,
                        GValue          *value,
			BseLadspaPort   *port,
			float            v_float)
{
  switch (sfi_categorize_type (G_VALUE_TYPE (value)))
    {
    case SFI_SCAT_BOOL:
      sfi_value_set_bool (value, v_float >= 0.5);
      break;
    case SFI_SCAT_INT:
      if (port->frequency && port->concert_a)	/* is note */
	sfi_value_set_int (value, bse_note_from_freq (bse_item_current_musical_tuning (BSE_ITEM (self)), v_float));
      else
	sfi_value_set_int (value, v_float >= 0 ? v_float + 0.5 : v_float - 0.5);
      break;
    case SFI_SCAT_REAL:
      sfi_value_set_real (value, v_float);
      break;
    default:
      assert_return_unreached ();
    }
}

static void
ladspa_derived_init (BseLadspaModule *self)
{
  BseLadspaModuleClass *klass = BSE_LADSPA_MODULE_GET_CLASS (self);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  uint i;
  self->cvalues = g_new (float, klass->bli->n_cports);
  for (i = 0; i < klass->bli->n_cports; i++)
    {
      GParamSpec *pspec = g_object_class_find_property (gobject_class, klass->bli->cports[i].ident);
      GValue tmp = { 0, };
      g_value_init (&tmp, G_PARAM_SPEC_VALUE_TYPE (pspec));
      g_param_value_set_default (pspec, &tmp);
      self->cvalues[i] = ladspa_value_get_float (self, &tmp, klass->bli->cports + i);
      g_value_unset (&tmp);
    }
}

static void
ladspa_derived_finalize (GObject *object)
{
  BseLadspaModule *self = BSE_LADSPA_MODULE (object);
  g_free (self->cvalues);
  G_OBJECT_CLASS (derived_parent_class)->finalize (object);
}

static void
ladspa_derived_get_property (GObject    *object,
			     uint        param_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
  BseLadspaModule *self = BSE_LADSPA_MODULE (object);
  BseLadspaModuleClass *klass = BSE_LADSPA_MODULE_GET_CLASS (self);
  uint i = param_id - 1;
  if (i >= klass->bli->n_cports)
    i = (ptrdiff_t) g_param_spec_get_qdata (pspec, quark_value_index);
  ladspa_value_set_float (self, value, klass->bli->cports + i, self->cvalues[i]);
}

typedef struct
{
  BseLadspaInfo *bli;
  void          *handle;
  uint	         activated : 1;
  float	        *ibuffers;
  float          cvalues[1];	/* flexible array */
} LadspaData;
#define	LADSPA_DATA_SIZE(bli)	  (sizeof (LadspaData) + (MAX (bli->n_cports, 1) - 1) * sizeof (float))
#define	LADSPA_CVALUES_COUNT(bli) (bli->n_cports /* * sizeof (float) */)

static void
ladspa_module_access (BseModule *module,        /* EngineThread */
		      void      *data)
{
  LadspaData *ldata = (LadspaData*) module->user_data;
  LadspaData *cdata = (LadspaData*) data;
  /* this runs in the Gsl Engine threads */
  bse_block_copy_float (LADSPA_CVALUES_COUNT (ldata->bli), ldata->cvalues, cdata->cvalues);
}

static void
ladspa_derived_set_property (GObject      *object,
			     uint          param_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
  BseLadspaModule *self = BSE_LADSPA_MODULE (object);
  BseLadspaModuleClass *klass = BSE_LADSPA_MODULE_GET_CLASS (self);
  GParamSpec *pspec2 = (GParamSpec*) g_param_spec_get_qdata (pspec, quark_notify_sibling);
  /* store value */
  uint i = param_id - 1;
  if (i >= klass->bli->n_cports)
    i = (ptrdiff_t) g_param_spec_get_qdata (pspec, quark_value_index);
  self->cvalues[i] = ladspa_value_get_float (self, value, klass->bli->cports + i);
  if (pspec2)
    g_object_notify (object, pspec2->name);
  /* update modules in all contexts with the new control values */
  if (BSE_SOURCE_PREPARED (self))
    {
      LadspaData *cdata = (LadspaData*) g_malloc0 (LADSPA_DATA_SIZE (klass->bli));
      bse_block_copy_float (LADSPA_CVALUES_COUNT (klass->bli), cdata->cvalues, self->cvalues);
      bse_source_access_modules (BSE_SOURCE (self),
				 ladspa_module_access,
				 cdata, g_free,
				 NULL);
    }
}

static void
ladspa_module_reset (BseModule *module)
{
  LadspaData *ldata = (LadspaData*) module->user_data;
  if (ldata->activated && ldata->bli->deactivate)
    ldata->bli->deactivate (ldata->handle);
  ldata->activated = FALSE;
  if (ldata->bli->activate)
    {
      ldata->bli->activate (ldata->handle);
      ldata->activated = TRUE;
    }
}

static void
ladspa_module_process (BseModule *module,
		       uint       n_values)
{
  LadspaData *ldata = (LadspaData*) module->user_data;
  BseLadspaInfo *bli = ldata->bli;
  uint i, nis = 0, nos = 0, bsize = bse_engine_block_size ();
  /* connect audio ports and copy audio buffers */
  for (i = 0; i < bli->n_aports; i++)
    if (bli->aports[i].output)
      {
	bli->connect_port (ldata->handle, bli->aports[i].port_index, BSE_MODULE_OBUFFER (module, nos));
	nos++;
      }
    else
      {
	float *ibuffer = ldata->ibuffers + nis * bsize;
	const float *srcbuf = BSE_MODULE_IBUFFER (module, nis);
	uint j;
	if (bli->aports[i].rate_relative)
	  for (j = 0; j < n_values; j++)
	    ibuffer[j] = srcbuf[j] * BSE_SIGNAL_TO_FREQ_FACTOR;
	else
	  memcpy (ibuffer, srcbuf, sizeof (ibuffer[0]) * n_values);
	nis++;
      }
  /* process ladspa plugin */
  ldata->bli->run (ldata->handle, n_values);
  /* adjust rate_relative output buffers */
  for (i = 0, nos = 0; i < bli->n_aports; i++)
    if (bli->aports[i].output && bli->aports[i].rate_relative)
      {
	float *obuf = BSE_MODULE_OBUFFER (module, nos);
	uint j;
	for (j = 0; j < n_values; j++)
	  obuf[j] *= BSE_SIGNAL_FROM_FREQ_FACTOR;
	nos++;
      }
}

static void
ladspa_module_free_data (void                 *data,
			 const BseModuleClass *klass)
{
  LadspaData *ldata = (LadspaData*) data;
  if (ldata->activated && ldata->bli->deactivate)
    ldata->bli->deactivate (ldata->handle);
  ldata->activated = FALSE;
  /* destroy ladspa plugin instance */
  ldata->bli->cleanup (ldata->handle);
  ldata->handle = NULL;
  g_free (ldata->ibuffers);
}

static void
ladspa_derived_context_create (BseSource *source,
			       uint       context_handle,
			       BseTrans  *trans)
{
  static const BseModuleClass ladspa_module_class = {
    0,				/* n_istreams */
    0,				/* n_jstreams */
    0,				/* n_ostreams */
    ladspa_module_process,	/* process */
    NULL,			/* process_defer */
    ladspa_module_reset,	/* reset */
    ladspa_module_free_data,	/* free */
    BSE_COST_EXPENSIVE,		/* cost */
  };
  BseLadspaModule *self = BSE_LADSPA_MODULE (source);
  BseLadspaModuleClass *klass = BSE_LADSPA_MODULE_GET_CLASS (self);
  BseLadspaInfo *bli = klass->bli;
  LadspaData *ldata = (LadspaData*) g_malloc0 (LADSPA_DATA_SIZE (bli));
  BseModule *module;
  uint i, nis;

  ldata->bli = bli;
  /* setup audio streams */
  if (!klass->gsl_class)
    {
      uint nos = 0;
      for (i = 0, nis = 0; i < bli->n_aports; i++)
	if (bli->aports[i].output)
	  nos++;
	else
	  nis++;
      klass->gsl_class = (BseModuleClass*) g_memdup (&ladspa_module_class, sizeof (ladspa_module_class));
      klass->gsl_class->n_istreams = nis;
      klass->gsl_class->n_ostreams = nos;
    }
  /* create ladspa plugin instance */
  ldata->handle = bli->instantiate (bli->descdata, bse_engine_sample_freq ());
  /* connect control ports */
  for (i = 0; i < bli->n_cports; i++)
    bli->connect_port (ldata->handle, bli->cports[i].port_index, ldata->cvalues + i);
  /* initialize control ports */
  bse_block_copy_float (LADSPA_CVALUES_COUNT (bli), ldata->cvalues, self->cvalues);
  /* allocate input audio buffers */
  ldata->ibuffers = g_new (float, klass->gsl_class->n_istreams * bse_engine_block_size ());
  /* connect input audio ports */
  for (i = 0, nis = 0; i < bli->n_aports; i++)
    if (bli->aports[i].input)
      bli->connect_port (ldata->handle, bli->aports[i].port_index, ldata->ibuffers + nis++ * bse_engine_block_size ());

  module = bse_module_new (klass->gsl_class, ldata);
  bse_source_set_context_module (source, context_handle, module);
  bse_trans_add (trans, bse_job_integrate (module));

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (derived_parent_class)->context_create (source, context_handle, trans);
}
