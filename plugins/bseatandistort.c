/* BseAtanDistort - BSE Arcus Tangens alike Distortion
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
#include "bseatandistort.h"

#include <bse/gslengine.h>
#include <bse/gslsignal.h>


/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_BOOST_AMOUNT
};


/* --- prototypes --- */
static void	 bse_atan_distort_init		      (BseAtanDistort		*self);
static void	 bse_atan_distort_class_init	      (BseAtanDistortClass	*class);
static void	 bse_atan_distort_set_property	      (GObject			*object,
						       guint                     param_id,
						       const GValue             *value,
						       GParamSpec               *pspec);
static void	 bse_atan_distort_get_property	      (GObject			*object,
						       guint                     param_id,
						       GValue                   *value,
						       GParamSpec               *pspec);
static void	 bse_atan_distort_context_create      (BseSource		*source,
						       guint			 context_handle,
						       GslTrans			*trans);
static void	 bse_atan_distort_update_modules      (BseAtanDistort		*comp);


/* --- variables --- */
static GType	       type_id_atan_distort = 0;
static gpointer	       parent_class = NULL;
static const GTypeInfo type_info_atan_distort = {
  sizeof (BseAtanDistortClass),
  
  (GBaseInitFunc) NULL,
  (GBaseFinalizeFunc) NULL,
  (GClassInitFunc) bse_atan_distort_class_init,
  (GClassFinalizeFunc) NULL,
  NULL /* class_data */,
  
  sizeof (BseAtanDistort),
  0 /* n_preallocs */,
  (GInstanceInitFunc) bse_atan_distort_init,
};


/* --- functions --- */
static void
bse_atan_distort_class_init (BseAtanDistortClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint channel_id;
  
  parent_class = g_type_class_peek_parent (class);

  gobject_class->set_property = bse_atan_distort_set_property;
  gobject_class->get_property = bse_atan_distort_get_property;
  
  source_class->context_create = bse_atan_distort_context_create;
  
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_BOOST_AMOUNT,
			      sfi_pspec_real ("boost_amount", "Boost Amount [%]",
						   "The atan distortion boost amount (strength) ranges "
						   "from maximum attenuation (0%) to maximum boost (100%).",
						   50, 0, 100.0, 5,
						   SFI_PARAM_DEFAULT SFI_PARAM_HINT_SCALE));

  channel_id = bse_source_class_add_ichannel (source_class, "Audio In", "Audio Input SIgnal");
  g_assert (channel_id == BSE_ATAN_DISTORT_ICHANNEL_MONO1);
  channel_id = bse_source_class_add_ochannel (source_class, "Audio Out", "Distorted Audio Output");
  g_assert (channel_id == BSE_ATAN_DISTORT_OCHANNEL_MONO1);
}

static void
bse_atan_distort_init (BseAtanDistort *self)
{
  self->boost_amount = 0.5;
  self->prescale = gsl_approx_atan1_prescale (self->boost_amount);
}

static void
bse_atan_distort_set_property (GObject      *object,
			       guint         param_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
  BseAtanDistort *self = BSE_ATAN_DISTORT (object);
  
  switch (param_id)
    {
    case PARAM_BOOST_AMOUNT:
      self->boost_amount = sfi_value_get_real (value) / 100.0;
      self->prescale = gsl_approx_atan1_prescale (self->boost_amount);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
  bse_atan_distort_update_modules (self);
}

static void
bse_atan_distort_get_property (GObject    *object,
			       guint       param_id,
			       GValue     *value,
			       GParamSpec *pspec)
{
  BseAtanDistort *self = BSE_ATAN_DISTORT (object);
  
  switch (param_id)
    {
    case PARAM_BOOST_AMOUNT:
      sfi_value_set_real (value, self->boost_amount * 100.0);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

typedef struct
{
  gdouble prescale;
} AtanDistortModule;

static void
bse_atan_distort_update_modules (BseAtanDistort *self)
{
  /* BseAtanDistort's settings changed, so we need to update
   * all associated AtanDistortModules, created by this
   * BseAtanDistort object.
   */
  if (BSE_SOURCE_PREPARED (self))
    {
      /* we're prepared, that means we have engine modules currently
       * processing data (each associated with a unique context ID).
       * now we need to let each of these modules know about the new
       * settings. bse_source_update_modules() will visit all modules
       * that got created in bse_atan_distort_context_create().
       * upon visiting each, it'll copy the contents of &self->prescale
       * into AtanDistortModule->prescale as soon as that module goes
       * idle (stops calculating sound data in atan_distort_process()).
       */
      bse_source_update_modules (BSE_SOURCE (self),
				 G_STRUCT_OFFSET (AtanDistortModule, prescale),
				 &self->prescale, sizeof (self->prescale),
				 NULL);
    }
}

static void
atan_distort_process (GslModule *module,
		      guint      n_values)
{
  AtanDistortModule *admod = module->user_data;
  const gfloat *sig_in = module->istreams[BSE_ATAN_DISTORT_ICHANNEL_MONO1].values;
  gfloat *sig_out = module->ostreams[BSE_ATAN_DISTORT_OCHANNEL_MONO1].values;
  gfloat *bound = sig_out + n_values;
  gdouble prescale = admod->prescale;

  /* we don't need to process any data if our input or
   * output stream isn't connected
   */
  if (!module->istreams[BSE_ATAN_DISTORT_ICHANNEL_MONO1].connected ||
      !module->ostreams[BSE_ATAN_DISTORT_OCHANNEL_MONO1].connected)
    {
      /* reset our output buffer to static-0s, this is faster
       * than using memset()
       */
      module->ostreams[BSE_ATAN_DISTORT_OCHANNEL_MONO1].values = gsl_engine_const_values (0);
      return;
    }

  /* do the mixing */
  do
    *sig_out++ = gsl_approx_atan1 (prescale * *sig_in++);
  while (sig_out < bound);
}

static void
bse_atan_distort_context_create (BseSource *source,
				 guint      context_handle,
				 GslTrans  *trans)
{
  static const GslClass admod_class = {
    BSE_ATAN_DISTORT_N_ICHANNELS, /* n_istreams */
    0,                            /* n_jstreams */
    BSE_ATAN_DISTORT_N_OCHANNELS, /* n_ostreams */
    atan_distort_process,	  /* process */
    NULL,                         /* process_defer */
    NULL,                         /* reset */
    (GslModuleFreeFunc) g_free,	  /* free */
    GSL_COST_NORMAL,		  /* cost */
  };
  BseAtanDistort *self = BSE_ATAN_DISTORT (source);
  AtanDistortModule *admod;
  GslModule *module;

  /* for each context that BseAtanDistort is used in, we create
   * a GslModule with data portion AtanDistortModule, that runs
   * in the synthesis engine.
   */
  admod = g_new0 (AtanDistortModule, 1);

  /* initial setup of module parameters */
  admod->prescale = self->prescale;

  /* create a GslModule with AtanDistortModule as user_data */
  module = gsl_module_new (&admod_class, admod);

  /* the istreams and ostreams of our GslModule map 1:1 to
   * BseAtanDistort's input/output channels, so we can call
   * bse_source_set_context_module() which does all the internal
   * crap of mapping istreams/ostreams of the module to
   * input/output channels of BseAtanDistort
   */
  bse_source_set_context_module (source, context_handle, module);

  /* commit module to engine */
  gsl_trans_add (trans, gsl_job_integrate (module));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}


/* --- Export to BSE --- */
#include "./icons/atan.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);	/* start of macro magic for BSE plugins */
BSE_EXPORT_OBJECTS = {			/* this plugin implements an object type */
  { &type_id_atan_distort,		/* variable to store object type id in */
    "BseAtanDistort",			/* name of this object type */
    "BseSource",			/* parent type */
    "BseAtanDistort compresses or expands the input signal with distortion "
    "(in a manner similar to the atan(3) mathematical function, thus it's name). "
    "The strength with which the input signal is treated is adjustable from "
    "maximum attenuation to maximum boost.",
    &type_info_atan_distort,		/* type information */
    "/Modules/Distortion/Atan Distort",	/* category (menu hierachy entry) */
    { ATAN_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      ATAN_IMAGE_WIDTH, ATAN_IMAGE_HEIGHT,
      ATAN_IMAGE_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;			/* end of macro magic for BSE plugins */
