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
#include "bsepcmoutput.h"

#include "bsecategories.h"
#include "bseserver.h"
#include "./icons/speaker.c"
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
static void	 bse_pcm_output_init		(BsePcmOutput		*scard);
static void	 bse_pcm_output_class_init	(BsePcmOutputClass	*class);
static void	 bse_pcm_output_class_finalize	(BsePcmOutputClass	*class);
static void	 bse_pcm_output_set_property	(BsePcmOutput		*scard,
						 guint			 param_id,
						 GValue			*value,
						 GParamSpec		*pspec,
						 const gchar		*trailer);
static void	 bse_pcm_output_get_property	(BsePcmOutput		*scard,
						 guint			 param_id,
						 GValue			*value,
						 GParamSpec		*pspec,
						 const gchar		*trailer);
static void	 bse_pcm_output_do_destroy	(BseObject		*object);
static void	 bse_pcm_output_prepare		(BseSource		*source);
static void	 bse_pcm_output_context_create	(BseSource		*source,
						 guint			 instance_id,
						 GslTrans		*trans);
static void	 bse_pcm_output_context_connect	(BseSource		*source,
						 guint			 instance_id,
						 GslTrans		*trans);
static void	 bse_pcm_output_reset		(BseSource		*source);


/* --- variables --- */
static gpointer		 parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BsePcmOutput)
{
  static const GTypeInfo pcm_output_info = {
    sizeof (BsePcmOutputClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_pcm_output_class_init,
    (GClassFinalizeFunc) bse_pcm_output_class_finalize,
    NULL /* class_data */,
    
    sizeof (BsePcmOutput),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_pcm_output_init,
  };
  static const BsePixdata pixdata = {
    SPEAKER_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
    SPEAKER_WIDTH, SPEAKER_HEIGHT,
    SPEAKER_RLE_PIXEL_DATA,
  };
  guint pcm_output_type_id;
  
  pcm_output_type_id = bse_type_register_static (BSE_TYPE_SOURCE,
						 "BsePcmOutput",
						 "Stereo PCM sound output module, per default, signals routed into "
						 "this module are played back on the standard soundcard",
						 &pcm_output_info);
  bse_categories_register_icon ("/Modules/Input & Output/PCM Output",
				pcm_output_type_id,
				&pixdata);
  return pcm_output_type_id;
}

static void
bse_pcm_output_class_init (BsePcmOutputClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ichannel_id;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = (GObjectSetPropertyFunc) bse_pcm_output_set_property;
  gobject_class->get_property = (GObjectGetPropertyFunc) bse_pcm_output_get_property;
  
  object_class->destroy = bse_pcm_output_do_destroy;
  
  source_class->prepare = bse_pcm_output_prepare;
  source_class->context_create = bse_pcm_output_context_create;
  source_class->context_connect = bse_pcm_output_context_connect;
  source_class->reset = bse_pcm_output_reset;
  
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_MVOLUME_f,
			      bse_param_spec_float ("master_volume_f", "Master [float]", NULL,
						    0, bse_dB_to_factor (BSE_MAX_VOLUME_dB),
						    bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB), 0.1,
						    BSE_PARAM_STORAGE));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_MVOLUME_dB,
			      bse_param_spec_float ("master_volume_dB", "Master [dB]", NULL,
						    BSE_MIN_VOLUME_dB, BSE_MAX_VOLUME_dB,
						    BSE_DFL_MASTER_VOLUME_dB, BSE_STP_VOLUME_dB,
						    BSE_PARAM_GUI | BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_MVOLUME_PERC,
			      bse_param_spec_uint ("master_volume_perc", "Master [%]", NULL,
						   0, bse_dB_to_factor (BSE_MAX_VOLUME_dB) * 100,
						   bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB) * 100, 1,
						   BSE_PARAM_GUI | BSE_PARAM_HINT_DIAL));
  
  ichannel_id = bse_source_class_add_ichannel (source_class, "Left In", "Left channel input");
  g_assert (ichannel_id == BSE_PCM_OUTPUT_ICHANNEL_LEFT);
  ichannel_id = bse_source_class_add_ichannel (source_class, "Right In", "Right channel Input");
  g_assert (ichannel_id == BSE_PCM_OUTPUT_ICHANNEL_RIGHT);
}

static void
bse_pcm_output_class_finalize (BsePcmOutputClass *class)
{
}

static void
bse_pcm_output_init (BsePcmOutput *oput)
{
  oput->volume_factor = bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB);
}

static void
bse_pcm_output_do_destroy (BseObject *object)
{
  BsePcmOutput *oput;
  
  oput = BSE_PCM_OUTPUT (object);
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_pcm_output_set_property (BsePcmOutput   *oput,
			     guint        param_id,
			     GValue      *value,
			     GParamSpec  *pspec,
			     const gchar *trailer)
{
  switch (param_id)
    {
    case PARAM_MVOLUME_f:
      oput->volume_factor = g_value_get_float (value);
      bse_object_param_changed (BSE_OBJECT (oput), "master_volume_dB");
      bse_object_param_changed (BSE_OBJECT (oput), "master_volume_perc");
      break;
    case PARAM_MVOLUME_dB:
      oput->volume_factor = bse_dB_to_factor (g_value_get_float (value));
      bse_object_param_changed (BSE_OBJECT (oput), "master_volume_f");
      bse_object_param_changed (BSE_OBJECT (oput), "master_volume_perc");
      break;
    case PARAM_MVOLUME_PERC:
      oput->volume_factor = g_value_get_uint (value) / 100.0;
      bse_object_param_changed (BSE_OBJECT (oput), "master_volume_f");
      bse_object_param_changed (BSE_OBJECT (oput), "master_volume_dB");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (oput, param_id, pspec);
      break;
    }
}

static void
bse_pcm_output_get_property (BsePcmOutput   *oput,
			     guint        param_id,
			     GValue      *value,
			     GParamSpec  *pspec,
			     const gchar *trailer)
{
  switch (param_id)
    {
    case PARAM_MVOLUME_f:
      g_value_set_float (value, oput->volume_factor);
      break;
    case PARAM_MVOLUME_dB:
      g_value_set_float (value, bse_dB_from_factor (oput->volume_factor, BSE_MIN_VOLUME_dB));
      break;
    case PARAM_MVOLUME_PERC:
      g_value_set_uint (value, oput->volume_factor * 100.0 + 0.5);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (oput, param_id, pspec);
      break;
    }
}

static void
bse_pcm_output_prepare (BseSource *source)
{
  BsePcmOutput *oput = BSE_PCM_OUTPUT (source);
  
  oput->uplink = bse_server_retrive_pcm_output_module (bse_server_get (), source, "MasterOut");
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

typedef struct {
  gfloat             volume;
  gboolean           volume_set;
} ModData;

static void
pcm_output_process (GslModule *module,
		    guint      n_values)
{
  ModData *mdata = module->user_data;
  const gfloat *ls = GSL_MODULE_IBUFFER (module, BSE_PCM_OUTPUT_ICHANNEL_LEFT);
  const gfloat *rs = GSL_MODULE_IBUFFER (module, BSE_PCM_OUTPUT_ICHANNEL_RIGHT);
  gfloat *ld = GSL_MODULE_OBUFFER (module, BSE_PCM_OUTPUT_ICHANNEL_LEFT);
  gfloat *rd = GSL_MODULE_OBUFFER (module, BSE_PCM_OUTPUT_ICHANNEL_RIGHT);
  gfloat v = mdata->volume;
  
  if (mdata->volume_set)
    while (n_values--)
      {
	*ld++ = v * *ls++;
	*rd++ = v * *rs++;
      }
  else
    {
      GSL_MODULE_OBUFFER (module, BSE_PCM_OUTPUT_ICHANNEL_LEFT) = (gfloat*) GSL_MODULE_IBUFFER (module, BSE_PCM_OUTPUT_ICHANNEL_LEFT);
      GSL_MODULE_OBUFFER (module, BSE_PCM_OUTPUT_ICHANNEL_RIGHT) = (gfloat*) GSL_MODULE_IBUFFER (module, BSE_PCM_OUTPUT_ICHANNEL_RIGHT);
    }
}

static void
bse_pcm_output_context_create (BseSource *source,
			       guint      context_handle,
			       GslTrans  *trans)
{
  static const GslClass pcm_output_mclass = {
    BSE_PCM_OUTPUT_N_ICHANNELS,	/* n_istreams */
    0,				/* n_jstreams */
    BSE_PCM_OUTPUT_N_ICHANNELS,	/* n_ostreams */
    pcm_output_process,		/* process */
    (GslModuleFreeFunc) g_free,	/* free */
    GSL_COST_CHEAP,		/* cost */
  };
  ModData *mdata = g_new0 (ModData, 1);
  GslModule *module = gsl_module_new (&pcm_output_mclass, mdata);
  
  mdata->volume = 1.0;
  mdata->volume_set = BSE_EPSILON_CMP (mdata->volume, 1.0) != 0;
  
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_imodule (source, context_handle, module);
  
  /* commit module to engine */
  gsl_trans_add (trans, gsl_job_integrate (module));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

static void
bse_pcm_output_context_connect (BseSource *source,
				guint      context_handle,
				GslTrans  *trans)
{
  BsePcmOutput *oput = BSE_PCM_OUTPUT (source);
  GslModule *module;
  
  /* get context specific module */
  module = bse_source_get_context_imodule (source, context_handle);
  
  /* connect module to server uplink */
  gsl_trans_add (trans, gsl_job_connect (module, 0, oput->uplink, 0));
  gsl_trans_add (trans, gsl_job_connect (module, 1, oput->uplink, 1));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_connect (source, context_handle, trans);
}

static void
bse_pcm_output_reset (BseSource *source)
{
  BsePcmOutput *oput = BSE_PCM_OUTPUT (source);
  
  bse_server_discard_pcm_output_module (bse_server_get (), oput->uplink);
  oput->uplink = NULL;
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}
