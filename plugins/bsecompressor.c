/* BseCompressor - BSE Compressor
 * Copyright (C) 1999, 2000-2001 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bsecompressor.h"

#include <bse/gslengine.h>
#include <math.h>


/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_PI_EXP
};


/* --- prototypes --- */
static void	 bse_compressor_init		      (BseCompressor		*compr);
static void	 bse_compressor_class_init	      (BseCompressorClass	*class);
static void	 bse_compressor_set_property	      (GObject			*object,
						       guint                     param_id,
						       const GValue             *value,
						       GParamSpec               *pspec);
static void	 bse_compressor_get_property	      (GObject			*object,
						       guint                     param_id,
						       GValue                   *value,
						       GParamSpec               *pspec);
static void	 bse_compressor_context_create	      (BseSource		*source,
						       guint			 context_handle,
						       GslTrans			*trans);
static void	 bse_compressor_update_modules	      (BseCompressor		*comp);


/* --- variables --- */
static GType		 type_id_compressor = 0;
static gpointer		 parent_class = NULL;
static const GTypeInfo type_info_compressor = {
  sizeof (BseCompressorClass),
  
  (GBaseInitFunc) NULL,
  (GBaseFinalizeFunc) NULL,
  (GClassInitFunc) bse_compressor_class_init,
  (GClassFinalizeFunc) NULL,
  NULL /* class_data */,
  
  sizeof (BseCompressor),
  0 /* n_preallocs */,
  (GInstanceInitFunc) bse_compressor_init,
};


/* --- functions --- */
static void
bse_compressor_class_init (BseCompressorClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ichannel_id, ochannel_id;
  
  parent_class = g_type_class_peek (BSE_TYPE_SOURCE);
  
  gobject_class->set_property = bse_compressor_set_property;
  gobject_class->get_property = bse_compressor_get_property;
  
  source_class->context_create = bse_compressor_context_create;
  
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_PI_EXP,
			      bse_param_spec_float ("pi_exp", "Strength",
						  "The compressor strength allowes for fine grained "
						  "adjustments from extenuated volume to maximum limiting",
						  -1.0, 5.0, 0.0, 0.25,
						  BSE_PARAM_DEFAULT | BSE_PARAM_HINT_SCALE));
  
  ichannel_id = bse_source_class_add_ichannel (source_class, "mono_input1", "Input SIgnal");
  g_assert (ichannel_id == BSE_COMPRESSOR_ICHANNEL_MONO1);
  ochannel_id = bse_source_class_add_ochannel (source_class, "mono_output1", "Mono Output");
  g_assert (ochannel_id == BSE_COMPRESSOR_OCHANNEL_MONO1);
}

static void
bse_compressor_init (BseCompressor *compr)
{
  compr->pi_fact = 1.0;
  bse_compressor_update_modules (compr);
}

static void
bse_compressor_set_property (GObject      *object,
			     guint         param_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
  BseCompressor *compr = BSE_COMPRESSOR (object);

  switch (param_id)
    {
    case PARAM_PI_EXP:
      compr->pi_fact = pow (PI, g_value_get_float (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (compr, param_id, pspec);
      break;
    }
  bse_compressor_update_modules (compr);
}

static void
bse_compressor_get_property (GObject    *object,
			     guint       param_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
  BseCompressor *compr = BSE_COMPRESSOR (object);

  switch (param_id)
    {
    case PARAM_PI_EXP:
      g_value_set_float (value, log (compr->pi_fact) / log (PI));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (compr, param_id, pspec);
      break;
    }
}

typedef struct
{
  BseCompressorVars vars;
} CompressorModule;

static void
bse_compressor_update_modules (BseCompressor *comp)
{
  /* BseCompressor's settings changed, so we need to update
   * the ->vars portion which is duplicated by CompressorModule
   */
  comp->vars.isample_factor = comp->pi_fact;
  comp->vars.osample_factor = 2.0 / PI;

  if (BSE_SOURCE_PREPARED (comp))
    {
      /* we're prepared, that means we have engine modules currently
       * processing data. now we need to let each of these modules
       * know about the new settings. bse_source_update_omodules()
       * will visit all modules that we created in context_create()
       * and which are connected with their ostream to
       * BSE_COMPRESSOR_OCHANNEL_MONO1.
       * upon visiting each, it'll copy the contents of comp->vars into
       * CompressorModule.vars when that module is not currently busy in
       * compressor_process().
       */
      bse_source_update_omodules (BSE_SOURCE (comp),
				  BSE_COMPRESSOR_OCHANNEL_MONO1,
				  G_STRUCT_OFFSET (CompressorModule, vars),
				  &comp->vars, sizeof (comp->vars),
				  NULL);
    }
}

static void
compressor_process (GslModule *module,
		    guint      n_values)
{
  CompressorModule *cmod = module->user_data;
  BseCompressorVars *vars = &cmod->vars;
  const gfloat *wave_in = module->istreams[BSE_COMPRESSOR_ICHANNEL_MONO1].values;
  gfloat *wave_out = module->ostreams[BSE_COMPRESSOR_OCHANNEL_MONO1].values;
  gfloat *wave_bound = wave_out + n_values;
  gfloat isample_factor = vars->isample_factor;
  gfloat osample_factor = vars->osample_factor;

  /* we don't need to process any data if our input or
   * output stream isn't connected
   */
  if (!module->istreams[BSE_COMPRESSOR_ICHANNEL_MONO1].connected ||
      !module->ostreams[BSE_COMPRESSOR_OCHANNEL_MONO1].connected)
    {
      /* reset our output buffer to static-0s, this is faster
       * than using memset()
       */
      module->ostreams[BSE_COMPRESSOR_OCHANNEL_MONO1].values = gsl_engine_const_values (0);
      return;
    }

  /* do the mixing */
  if (BSE_EPSILON_CMP (1.0, isample_factor))
    {
      if (BSE_EPSILON_CMP (1.0, osample_factor))
	do
	  *wave_out++ = atan (*wave_in++ * isample_factor) * osample_factor;
	while (wave_out < wave_bound);
      else /* osample_factor==1.0 */
	do
	  *wave_out++ = atan (*wave_in++ * isample_factor);
	while (wave_out < wave_bound);
    }
  else /* isample_factor==1.0 */
    {
      if (BSE_EPSILON_CMP (1.0, osample_factor))
	do
	  *wave_out++ = atan (*wave_in++) * osample_factor;
	while (wave_out < wave_bound);
      else /* osample_factor==1.0 */
	module->ostreams[BSE_COMPRESSOR_OCHANNEL_MONO1].values = (gfloat*)
	  module->istreams[BSE_COMPRESSOR_ICHANNEL_MONO1].values;
    }
}

static void
bse_compressor_context_create (BseSource *source,
			       guint      context_handle,
			       GslTrans  *trans)
{
  static const GslClass cmod_class = {
    1,				/* n_istreams */
    0,                          /* n_jstreams */
    1,				/* n_ostreams */
    compressor_process,		/* process */
    (GslModuleFreeFunc) g_free,	/* free */
    GSL_COST_NORMAL,		/* cost */
  };
  BseCompressor *comp = BSE_COMPRESSOR (source);
  CompressorModule *cmod;
  GslModule *module;

  /* for each context that BseCompressor is used in, we create
   * a GslModule with data portion CompressorModule, that runs
   * in the synthesis engine
   */
  cmod = g_new0 (CompressorModule, 1);

  /* initial setup of module parameters */
  cmod->vars = comp->vars;

  /* create a GslModule with CompressorModule user_data */
  module = gsl_module_new (&cmod_class, cmod);

  /* the istreams and ostreams of our GslModule map 1:1 to
   * BseCompressor's input/output channels, so we can call
   * bse_source_set_context_module() which does all the internal
   * crap of mapping istreams/ostreams of the module to
   * input/output channels of BseCompressor
   */
  bse_source_set_context_module (source, context_handle, module);

  /* commit module to engine */
  gsl_trans_add (trans, gsl_job_integrate (module));

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}


/* --- Export to BSE --- */
#include "./icons/atan.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);	/* macro magic start for BSE plugins */
BSE_EXPORT_OBJECTS = {		/* our plugin implements an object */
  { &type_id_compressor,	/* variable to store object type id in */
    "BseCompressor",		/* name of our object */
    "BseSource",		/* parent type */
    "BseCompressor compresses according to the current Strength setting using "
    "the formula: output = atan (input * (Pi ^ Strength)), which allowes for "
    "fine grained adjustments from extenuated volume to maximum limiting",
    &type_info_compressor,	/* type information */
    "/Source/Compressor",	/* category (menu heirachy entry) */
    { ATAN_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      ATAN_IMAGE_WIDTH, ATAN_IMAGE_HEIGHT,
      ATAN_IMAGE_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;			/* macro magic end for BSE plugins */
