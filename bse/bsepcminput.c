/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1999, 2000-2002 Tim Janik
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
#include "bsepcminput.h"

#include "bsecategories.h"
#include "bseserver.h"
#include "./icons/mic.c"
#include "gslengine.h"



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
static void	 bse_pcm_input_class_init	(BsePcmInputClass	*class);
static void	 bse_pcm_input_class_finalize	(BsePcmInputClass	*class);
static void	 bse_pcm_input_set_property	(BsePcmInput		*scard,
						 guint			 param_id,
						 GValue			*value,
						 GParamSpec		*pspec,
						 const gchar		*trailer);
static void	 bse_pcm_input_get_property	(BsePcmInput		*scard,
						 guint			 param_id,
						 GValue			*value,
						 GParamSpec		*pspec,
						 const gchar		*trailer);
static void	 bse_pcm_input_do_destroy	(BseObject		*object);
static void	 bse_pcm_input_prepare		(BseSource		*source);
static void	 bse_pcm_input_context_create	(BseSource		*source,
						 guint			 instance_id,
						 GslTrans		*trans);
static void	 bse_pcm_input_context_connect	(BseSource		*source,
						 guint			 instance_id,
						 GslTrans		*trans);
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
  static const BsePixdata mic_pixdata = {
    MIC_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
    MIC_WIDTH, MIC_HEIGHT,
    MIC_RLE_PIXEL_DATA,
  };
  guint pcm_input_type_id;
  
  pcm_input_type_id = bse_type_register_static (BSE_TYPE_SOURCE,
						 "BsePcmInput",
						 "Stereo PCM sound input module, per default, signals from this "
						 "module originate from recording on the standard soundcard",
						 &pcm_input_info);
  bse_categories_register_icon ("/Modules/Input & Output/PCM Input",
				pcm_input_type_id,
				&mic_pixdata);
  return pcm_input_type_id;
}

static void
bse_pcm_input_class_init (BsePcmInputClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ochannel_id;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = (GObjectSetPropertyFunc) bse_pcm_input_set_property;
  gobject_class->get_property = (GObjectGetPropertyFunc) bse_pcm_input_get_property;
  
  object_class->destroy = bse_pcm_input_do_destroy;
  
  source_class->prepare = bse_pcm_input_prepare;
  source_class->context_create = bse_pcm_input_context_create;
  source_class->context_connect = bse_pcm_input_context_connect;
  source_class->reset = bse_pcm_input_reset;
  
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_MVOLUME_f,
			      bse_param_spec_float ("gain_volume_f", "Input Gain [float]", NULL,
						    0, bse_dB_to_factor (BSE_MAX_VOLUME_dB),
						    bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB), 0.1,
						    BSE_PARAM_STORAGE));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_MVOLUME_dB,
			      bse_param_spec_float ("gain_volume_dB", "Input Gain [dB]", NULL,
						    BSE_MIN_VOLUME_dB, BSE_MAX_VOLUME_dB,
						    BSE_DFL_MASTER_VOLUME_dB, BSE_STP_VOLUME_dB,
						    BSE_PARAM_GUI | BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_MVOLUME_PERC,
			      bse_param_spec_uint ("gain_volume_perc", "input Gain [%]", NULL,
						   0, bse_dB_to_factor (BSE_MAX_VOLUME_dB) * 100,
						   bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB) * 100, 1,
						   BSE_PARAM_GUI | BSE_PARAM_HINT_DIAL));
  
  ochannel_id = bse_source_class_add_ochannel (source_class, "Left Audio Out", "Left channel output");
  g_assert (ochannel_id == BSE_PCM_INPUT_OCHANNEL_LEFT);
  ochannel_id = bse_source_class_add_ochannel (source_class, "Right Audio Out", "Right channel output");
  g_assert (ochannel_id == BSE_PCM_INPUT_OCHANNEL_RIGHT);
}

static void
bse_pcm_input_class_finalize (BsePcmInputClass *class)
{
}

static void
bse_pcm_input_init (BsePcmInput *iput)
{
  iput->volume_factor = bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB);
}

static void
bse_pcm_input_do_destroy (BseObject *object)
{
  BsePcmInput *iput;
  
  iput = BSE_PCM_INPUT (object);
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_pcm_input_set_property (BsePcmInput   *iput,
			     guint        param_id,
			     GValue      *value,
			     GParamSpec  *pspec,
			     const gchar *trailer)
{
  switch (param_id)
    {
    case PARAM_MVOLUME_f:
      iput->volume_factor = g_value_get_float (value);
      bse_object_param_changed (BSE_OBJECT (iput), "gain_volume_dB");
      bse_object_param_changed (BSE_OBJECT (iput), "gain_volume_perc");
      break;
    case PARAM_MVOLUME_dB:
      iput->volume_factor = bse_dB_to_factor (g_value_get_float (value));
      bse_object_param_changed (BSE_OBJECT (iput), "gain_volume_f");
      bse_object_param_changed (BSE_OBJECT (iput), "gain_volume_perc");
      break;
    case PARAM_MVOLUME_PERC:
      iput->volume_factor = g_value_get_uint (value) / 100.0;
      bse_object_param_changed (BSE_OBJECT (iput), "gain_volume_f");
      bse_object_param_changed (BSE_OBJECT (iput), "gain_volume_dB");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (iput, param_id, pspec);
      break;
    }
}

static void
bse_pcm_input_get_property (BsePcmInput   *iput,
			     guint        param_id,
			     GValue      *value,
			     GParamSpec  *pspec,
			     const gchar *trailer)
{
  switch (param_id)
    {
    case PARAM_MVOLUME_f:
      g_value_set_float (value, iput->volume_factor);
      break;
    case PARAM_MVOLUME_dB:
      g_value_set_float (value, bse_dB_from_factor (iput->volume_factor, BSE_MIN_VOLUME_dB));
      break;
    case PARAM_MVOLUME_PERC:
      g_value_set_uint (value, iput->volume_factor * 100.0 + 0.5);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (iput, param_id, pspec);
      break;
    }
}

static void
bse_pcm_input_prepare (BseSource *source)
{
  BsePcmInput *iput = BSE_PCM_INPUT (source);
  
  iput->uplink = bse_server_retrive_pcm_input_module (bse_server_get (), source, "MasterIn");
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

typedef struct {
  gfloat             volume;
  gboolean           volume_set;
} ModData;

static void
pcm_input_process (GslModule *module,
		    guint      n_values)
{
  ModData *mdata = module->user_data;
  const gfloat *ls = GSL_MODULE_IBUFFER (module, BSE_PCM_INPUT_OCHANNEL_LEFT);
  const gfloat *rs = GSL_MODULE_IBUFFER (module, BSE_PCM_INPUT_OCHANNEL_RIGHT);
  gfloat *ld = GSL_MODULE_OBUFFER (module, BSE_PCM_INPUT_OCHANNEL_LEFT);
  gfloat *rd = GSL_MODULE_OBUFFER (module, BSE_PCM_INPUT_OCHANNEL_RIGHT);
  gfloat v = mdata->volume;
  
  if (mdata->volume_set)
    while (n_values--)
      {
	*ld++ = v * *ls++;
	*rd++ = v * *rs++;
      }
  else
    {
      GSL_MODULE_OBUFFER (module, BSE_PCM_INPUT_OCHANNEL_LEFT) = (gfloat*) GSL_MODULE_IBUFFER (module, BSE_PCM_INPUT_OCHANNEL_LEFT);
      GSL_MODULE_OBUFFER (module, BSE_PCM_INPUT_OCHANNEL_RIGHT) = (gfloat*) GSL_MODULE_IBUFFER (module, BSE_PCM_INPUT_OCHANNEL_RIGHT);
    }
}

static void
bse_pcm_input_context_create (BseSource *source,
			       guint      context_handle,
			       GslTrans  *trans)
{
  static const GslClass pcm_input_mclass = {
    BSE_PCM_INPUT_N_OCHANNELS,	/* n_istreams */
    0,				/* n_jstreams */
    BSE_PCM_INPUT_N_OCHANNELS,	/* n_ostreams */
    pcm_input_process,		/* process */
    NULL,                       /* process_defer */
    NULL,                       /* reset */
    (GslModuleFreeFunc) g_free,	/* free */
    GSL_COST_CHEAP,		/* cost */
  };
  ModData *mdata = g_new0 (ModData, 1);
  GslModule *module = gsl_module_new (&pcm_input_mclass, mdata);
  
  mdata->volume = 1.0;
  mdata->volume_set = BSE_EPSILON_CMP (mdata->volume, 1.0) != 0;
  
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_omodule (source, context_handle, module);
  
  /* commit module to engine */
  gsl_trans_add (trans, gsl_job_integrate (module));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

static void
bse_pcm_input_context_connect (BseSource *source,
				guint      context_handle,
				GslTrans  *trans)
{
  BsePcmInput *iput = BSE_PCM_INPUT (source);
  GslModule *module;
  
  /* get context specific module */
  module = bse_source_get_context_omodule (source, context_handle);
  
  /* connect module to server uplink */
  gsl_trans_add (trans, gsl_job_connect (iput->uplink, 0, module, 0));
  gsl_trans_add (trans, gsl_job_connect (iput->uplink, 1, module, 1));
  
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
