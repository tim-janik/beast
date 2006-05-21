/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1999, 2000-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bsepcmoutput.h"

#include "bsecategories.h"
#include "bseserver.h"
#include "bseengine.h"



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
static void	 bse_pcm_output_set_property	(GObject		*object,
						 guint			 param_id,
						 const GValue		*value,
						 GParamSpec		*pspec);
static void	 bse_pcm_output_get_property	(GObject		*object,
						 guint			 param_id,
						 GValue			*value,
						 GParamSpec		*pspec);
static void	 bse_pcm_output_prepare		(BseSource		*source);
static void	 bse_pcm_output_context_create	(BseSource		*source,
						 guint			 instance_id,
						 BseTrans		*trans);
static void	 bse_pcm_output_context_connect	(BseSource		*source,
						 guint			 instance_id,
						 BseTrans		*trans);
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
#include "./icons/speaker.c"
  GType type = bse_type_register_static (BSE_TYPE_SOURCE,
                                         "BsePcmOutput",
                                         "Stereo PCM sound output module, per default, signals routed into "
                                         "this module are played back on the standard soundcard",
                                         __FILE__, __LINE__,
                                         &pcm_output_info);
  bse_categories_register_stock_module (N_("/Input & Output/PCM Output"), type, speaker_pixstream);
  return type;
}

static void
bse_pcm_output_class_init (BsePcmOutputClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ichannel_id;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_pcm_output_set_property;
  gobject_class->get_property = bse_pcm_output_get_property;
  
  source_class->prepare = bse_pcm_output_prepare;
  source_class->context_create = bse_pcm_output_context_create;
  source_class->context_connect = bse_pcm_output_context_connect;
  source_class->reset = bse_pcm_output_reset;
  
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_MVOLUME_f,
			      sfi_pspec_real ("master_volume_f", "Master [float]", NULL,
					      bse_db_to_factor (0),
					      0, bse_db_to_factor (BSE_MAX_VOLUME_dB),
					      0.1,
                                              SFI_PARAM_STORAGE ":skip-default")); // FIXME: don't skip-default
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_MVOLUME_dB,
			      sfi_pspec_real ("master_volume_dB", "Master [dB]", NULL,
					      0,
					      BSE_MIN_VOLUME_dB, BSE_MAX_VOLUME_dB,
                                              0.1,
					      SFI_PARAM_GUI ":dial"));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_MVOLUME_PERC,
			      sfi_pspec_int ("master_volume_perc", "Master [%]", NULL,
					     bse_db_to_factor (0) * 100,
					     0, bse_db_to_factor (BSE_MAX_VOLUME_dB) * 100,
					     1, SFI_PARAM_GUI ":dial"));
  
  ichannel_id = bse_source_class_add_ichannel (source_class, "left-audio-in", _("Left Audio In"), _("Left channel input"));
  g_assert (ichannel_id == BSE_PCM_OUTPUT_ICHANNEL_LEFT);
  ichannel_id = bse_source_class_add_ichannel (source_class, "right-audio-in", _("Right Audio In"), _("Right channel Input"));
  g_assert (ichannel_id == BSE_PCM_OUTPUT_ICHANNEL_RIGHT);
}

static void
bse_pcm_output_class_finalize (BsePcmOutputClass *class)
{
}

static void
bse_pcm_output_init (BsePcmOutput *oput)
{
  oput->volume_factor = bse_db_to_factor (0);
}

static void
bse_pcm_output_set_property (GObject      *object,
			     guint         param_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
  BsePcmOutput *self = BSE_PCM_OUTPUT (object);
  switch (param_id)
    {
    case PARAM_MVOLUME_f:
      self->volume_factor = sfi_value_get_real (value);
      g_object_notify (self, "master_volume_dB");
      g_object_notify (self, "master_volume_perc");
      break;
    case PARAM_MVOLUME_dB:
      self->volume_factor = bse_db_to_factor (sfi_value_get_real (value));
      g_object_notify (self, "master_volume_f");
      g_object_notify (self, "master_volume_perc");
      break;
    case PARAM_MVOLUME_PERC:
      self->volume_factor = sfi_value_get_int (value) / 100.0;
      g_object_notify (self, "master_volume_f");
      g_object_notify (self, "master_volume_dB");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
bse_pcm_output_get_property (GObject    *object,
			     guint       param_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
  BsePcmOutput *self = BSE_PCM_OUTPUT (object);
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
bse_pcm_output_prepare (BseSource *source)
{
  BsePcmOutput *oput = BSE_PCM_OUTPUT (source);
  
  oput->uplink = bse_server_retrieve_pcm_output_module (bse_server_get (), source, "MasterOut");
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

typedef struct {
  gfloat             volume;
  gboolean           volume_set;
} ModData;

static void
pcm_output_process (BseModule *module,
		    guint      n_values)
{
  ModData *mdata = module->user_data;
  const gfloat *ls = BSE_MODULE_IBUFFER (module, BSE_PCM_OUTPUT_ICHANNEL_LEFT);
  const gfloat *rs = BSE_MODULE_IBUFFER (module, BSE_PCM_OUTPUT_ICHANNEL_RIGHT);
  gfloat *ld = BSE_MODULE_OBUFFER (module, BSE_PCM_OUTPUT_ICHANNEL_LEFT);
  gfloat *rd = BSE_MODULE_OBUFFER (module, BSE_PCM_OUTPUT_ICHANNEL_RIGHT);
  gfloat v = mdata->volume;
  
  if (mdata->volume_set)
    while (n_values--)
      {
	*ld++ = v * *ls++;
	*rd++ = v * *rs++;
      }
  else
    {
      BSE_MODULE_OBUFFER (module, BSE_PCM_OUTPUT_ICHANNEL_LEFT) = (gfloat*) BSE_MODULE_IBUFFER (module, BSE_PCM_OUTPUT_ICHANNEL_LEFT);
      BSE_MODULE_OBUFFER (module, BSE_PCM_OUTPUT_ICHANNEL_RIGHT) = (gfloat*) BSE_MODULE_IBUFFER (module, BSE_PCM_OUTPUT_ICHANNEL_RIGHT);
    }
}

static void
bse_pcm_output_context_create (BseSource *source,
			       guint      context_handle,
			       BseTrans  *trans)
{
  static const BseModuleClass pcm_output_mclass = {
    BSE_PCM_OUTPUT_N_ICHANNELS,	/* n_istreams */
    0,				/* n_jstreams */
    BSE_PCM_OUTPUT_N_ICHANNELS,	/* n_ostreams */
    pcm_output_process,		/* process */
    NULL,                       /* process_defer */
    NULL,                       /* reset */
    (BseModuleFreeFunc) g_free,	/* free */
    BSE_COST_CHEAP,		/* cost */
  };
  ModData *mdata = g_new0 (ModData, 1);
  BseModule *module = bse_module_new (&pcm_output_mclass, mdata);
  
  mdata->volume = 1.0;
  mdata->volume_set = mdata->volume != 1.0;
  
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_imodule (source, context_handle, module);
  
  /* commit module to engine */
  bse_trans_add (trans, bse_job_integrate (module));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

static void
bse_pcm_output_context_connect (BseSource *source,
				guint      context_handle,
				BseTrans  *trans)
{
  BsePcmOutput *oput = BSE_PCM_OUTPUT (source);
  BseModule *module;
  
  /* get context specific module */
  module = bse_source_get_context_imodule (source, context_handle);
  
  /* connect module to server uplink */
  bse_trans_add (trans, bse_job_jconnect (module, 0, oput->uplink, 0));
  bse_trans_add (trans, bse_job_jconnect (module, 1, oput->uplink, 1));
  
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
