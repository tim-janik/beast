/* BseFreeVerb - Free Verb Wrapper for BSE
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bsefreeverb.h"

#include <gsl/gslengine.h>


/* --- properties --- */
enum
{
  PROP_0,
  PROP_ROOM_SIZE,
  PROP_DAMPING,
  PROP_WET_LEVEL,
  PROP_DRY_LEVEL,
  PROP_WIDTH
};


/* --- prototypes --- */
static void	bse_free_verb_init		(BseFreeVerb		*self);
static void	bse_free_verb_class_init	(BseFreeVerbClass	*class);
static void	bse_free_verb_set_property	(GObject		*object,
						 guint			 param_id,
						 const GValue		*value,
						 GParamSpec		*pspec);
static void	bse_free_verb_get_property	(GObject		*object,
						 guint			 param_id,
						 GValue			*value,
						 GParamSpec		*pspec);
static void	bse_free_verb_context_create	(BseSource		*source,
						 guint			 context_handle,
						 GslTrans		*trans);
static void	bse_free_verb_update_modules	(BseFreeVerb		*self);


/* --- variables --- */
static GType           type_id_free_verb = 0;
static gpointer        parent_class = NULL;
static const GTypeInfo type_info_free_verb = {
  sizeof (BseFreeVerbClass),
  (GBaseInitFunc) NULL,
  (GBaseFinalizeFunc) NULL,
  (GClassInitFunc) bse_free_verb_class_init,
  (GClassFinalizeFunc) NULL,
  NULL /* class_data */,
  sizeof (BseFreeVerb),
  0 /* n_preallocs */,
  (GInstanceInitFunc) bse_free_verb_init,
};


/* --- functions --- */
static void
bse_free_verb_class_init (BseFreeVerbClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  BseFreeVerbConstants *constants = &class->constants;
  BseFreeVerbConfig defaults;
  guint channel;

  parent_class = g_type_class_peek_parent (class);

  gobject_class->set_property = bse_free_verb_set_property;
  gobject_class->get_property = bse_free_verb_get_property;

  source_class->context_create = bse_free_verb_context_create;

  bse_free_verb_cpp_defaults (&defaults, constants);

  bse_object_class_add_param (object_class, "Reverb Style",
			      PROP_ROOM_SIZE,
			      bse_param_spec_float ("room_size", "Room Size", NULL,
						    constants->room_offset,
						    constants->room_offset + constants->room_scale * 1.0,
						    constants->room_offset + constants->room_scale * defaults.room_size,
						    0.1 * constants->room_scale,
						    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Reverb Style",
			      PROP_DAMPING,
			      bse_param_spec_float ("damping", "Damping [%]", NULL,
						    0, constants->damp_scale * 1.0,
						    constants->damp_scale * defaults.damp,
						    0.1 * constants->damp_scale,
						    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Reverb Style",
			      PROP_WET_LEVEL,
			      bse_param_spec_float ("wet_level", "Wet Level [dB]", NULL,
						    0, constants->wet_scale * 1.0,
						    constants->wet_scale * defaults.wet,
						    0.1 * constants->wet_scale,
						    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Reverb Style",
			      PROP_DRY_LEVEL,
			      bse_param_spec_float ("dry_level", "Dry Level [dB]", NULL,
						    0, constants->dry_scale * 1.0,
						    constants->dry_scale * defaults.dry,
						    0.1 * constants->dry_scale,
						    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Reverb Style",
			      PROP_WIDTH,
			      bse_param_spec_float ("width", "Width [%]", NULL,
						    0, constants->width_scale * 1.0,
						    constants->width_scale * defaults.width,
						    0.1 * constants->width_scale,
						    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_DIAL));
  channel = bse_source_class_add_ichannel (source_class, "Left Audio In", "Left Input");
  channel = bse_source_class_add_ichannel (source_class, "Right Audio In", "Right Input");
  channel = bse_source_class_add_ochannel (source_class, "Left Audio Out", "Left Output");
  channel = bse_source_class_add_ochannel (source_class, "Right Audio Out", "Right Output");
}

static void
bse_free_verb_init (BseFreeVerb *self)
{
  bse_free_verb_cpp_defaults (&self->config, NULL);
}

static void
bse_free_verb_set_property (GObject      *object,
			    guint         param_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
  BseFreeVerb *self = BSE_FREE_VERB (object);
  BseFreeVerbConstants *constants = &BSE_FREE_VERB_GET_CLASS (self)->constants;

  switch (param_id)
    {
    case PROP_ROOM_SIZE:
      self->config.room_size = (g_value_get_float (value) - constants->room_offset) / constants->room_scale;
      bse_free_verb_update_modules (self);
      break;
    case PROP_DAMPING:
      self->config.damp = g_value_get_float (value) / constants->damp_scale;
      bse_free_verb_update_modules (self);
      break;
    case PROP_WET_LEVEL:
      self->config.wet = g_value_get_float (value) / constants->wet_scale;
      bse_free_verb_update_modules (self);
      break;
    case PROP_DRY_LEVEL:
      self->config.dry = g_value_get_float (value) / constants->dry_scale;
      bse_free_verb_update_modules (self);
      break;
    case PROP_WIDTH:
      self->config.width = g_value_get_float (value) / constants->width_scale;
      bse_free_verb_update_modules (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_free_verb_get_property (GObject    *object,
			    guint       param_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
  BseFreeVerb *self = BSE_FREE_VERB (object);
  BseFreeVerbConstants *constants = &BSE_FREE_VERB_GET_CLASS (self)->constants;

  switch (param_id)
    {
    case PROP_ROOM_SIZE:
      g_value_set_float (value, self->config.room_size * constants->room_scale + constants->room_offset);
      break;
    case PROP_DAMPING:
      g_value_set_float (value, self->config.damp * constants->damp_scale);
      break;
    case PROP_WET_LEVEL:
      g_value_set_float (value, self->config.wet * constants->wet_scale);
      break;
    case PROP_DRY_LEVEL:
      g_value_set_float (value, self->config.dry * constants->dry_scale);
      break;
    case PROP_WIDTH:
      g_value_set_float (value, self->config.width * constants->width_scale);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
free_verb_access (GslModule *module,
		  gpointer   data)
{
  BseFreeVerbCpp *cpp = module->user_data;
  BseFreeVerbConfig *config = data;

  /* this runs in the Gsl Engine threads */
  bse_free_verb_cpp_configure (cpp, config);

  /* save config for _reset() */
  bse_free_verb_cpp_save_config (cpp, config);
}

static void
bse_free_verb_update_modules (BseFreeVerb *self)
{
  if (BSE_SOURCE_PREPARED (self))
    bse_source_access_modules (BSE_SOURCE (self),
			       free_verb_access,
			       g_memdup (&self->config, sizeof (self->config)),
			       g_free,
			       NULL);
}

static void
free_verb_process (GslModule *module,
		   guint      n_values)
{
  BseFreeVerbCpp *cpp = module->user_data;
  const gfloat *ileft = GSL_MODULE_IBUFFER (module, BSE_FREE_VERB_ICHANNEL_LEFT);
  const gfloat *iright = GSL_MODULE_IBUFFER (module, BSE_FREE_VERB_ICHANNEL_RIGHT);
  gfloat *oleft = GSL_MODULE_OBUFFER (module, BSE_FREE_VERB_OCHANNEL_LEFT);
  gfloat *oright = GSL_MODULE_OBUFFER (module, BSE_FREE_VERB_OCHANNEL_RIGHT);

  bse_free_verb_cpp_process (cpp, n_values, ileft, iright, oleft, oright);
}

static void
free_verb_reset (GslModule *module)
{
  BseFreeVerbCpp *cpp = module->user_data;
  BseFreeVerbConfig config;

  bse_free_verb_cpp_restore_config (cpp, &config);
  bse_free_verb_cpp_destroy (cpp);
  bse_free_verb_cpp_create (cpp);
  bse_free_verb_cpp_configure (cpp, &config);
  bse_free_verb_cpp_save_config (cpp, &config);
}

static void
free_verb_destroy (gpointer        data,
		   const GslClass *klass)
{
  BseFreeVerbCpp *cpp = data;

  bse_free_verb_cpp_destroy (cpp);
  g_free (cpp);
}

static void
bse_free_verb_context_create (BseSource *source,
			      guint      context_handle,
			      GslTrans  *trans)
{
  static const GslClass free_verb_class = {
    BSE_FREE_VERB_N_ICHANNELS,	/* n_istreams */
    0,				/* n_jstreams */
    BSE_FREE_VERB_N_OCHANNELS,	/* n_ostreams */
    free_verb_process,		/* process */
    NULL,			/* process_defer */
    free_verb_reset,		/* reset */
    free_verb_destroy,		/* free */
    GSL_COST_EXPENSIVE,		/* cost */
  };
  BseFreeVerb *self = BSE_FREE_VERB (source);
  BseFreeVerbCpp *cpp = g_new0 (BseFreeVerbCpp, 1);
  GslModule *module;

  /* initialize module data */
  bse_free_verb_cpp_create (cpp);
  bse_free_verb_cpp_configure (cpp, &self->config);
  bse_free_verb_cpp_save_config (cpp, &self->config);
  module = gsl_module_new (&free_verb_class, cpp);

  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);

  /* commit module to engine */
  gsl_trans_add (trans, gsl_job_integrate (module));

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}


/* --- Export to BSE --- */
#include "../icons/reverb.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_free_verb, "BseFreeVerb", "BseSource",
    "BseFreeVerb - Free, studio-quality reverb (SOURCE CODE in the public domain) "
    "Written by Jezar at Dreampoint - http://www.dreampoint.co.uk",
    &type_info_free_verb,
    "/Modules/Filters/Free Verb",
    { REVERB_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      REVERB_IMAGE_WIDTH, REVERB_IMAGE_HEIGHT,
      REVERB_IMAGE_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
