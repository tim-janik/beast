/* BseAdder - BSE Adder
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
#include "bseadder.h"

#include <bse/bsechunk.h>
#include <bse/bsehunkmixer.h>
#include <bse/gslengine.h>


/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_SUBTRACT,
};


/* --- prototypes --- */
static void	 bse_adder_init			(BseAdder	*adder);
static void	 bse_adder_class_init		(BseAdderClass	*class);
static void	 bse_adder_set_property		(BseAdder	*adder,
						 guint           param_id,
						 GValue         *value,
						 GParamSpec     *pspec);
static void	 bse_adder_get_property		(BseAdder	*adder,
						 guint           param_id,
						 GValue         *value,
						 GParamSpec     *pspec);
static BseIcon*	 bse_adder_do_get_icon		(BseObject	*object);
static void      bse_adder_context_create       (BseSource      *source,
						 guint           context_handle,
						 GslTrans       *trans);
static void	 bse_adder_update_modules	(BseAdder	*adder,
						 GslTrans	*trans);


/* --- variables --- */
static GType		 type_id_adder = 0;
static gpointer		 parent_class = NULL;
static const GTypeInfo   type_info_adder = {
  sizeof (BseAdderClass),
  
  (GBaseInitFunc) NULL,
  (GBaseFinalizeFunc) NULL,
  (GClassInitFunc) bse_adder_class_init,
  (GClassFinalizeFunc) NULL,
  NULL /* class_data */,
  
  sizeof (BseAdder),
  0 /* n_preallocs */,
  (GInstanceInitFunc) bse_adder_init,
};


/* --- functions --- */
static void
bse_adder_class_init (BseAdderClass *class)
{
#include "./icons/sub.c"
  BsePixdata sub_pix_data = { SUB_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
			      SUB_IMAGE_WIDTH, SUB_IMAGE_HEIGHT,
			      SUB_IMAGE_RLE_PIXEL_DATA, };
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ichannel, ochannel;
  
  parent_class = g_type_class_peek (BSE_TYPE_SOURCE);
  
  gobject_class->set_property = (GObjectSetPropertyFunc) bse_adder_set_property;
  gobject_class->get_property = (GObjectGetPropertyFunc) bse_adder_get_property;
  
  object_class->get_icon = bse_adder_do_get_icon;
  
  source_class->context_create = bse_adder_context_create;
  
  class->sub_icon = bse_icon_from_pixdata (&sub_pix_data);
  
  bse_object_class_add_param (object_class, "Features",
			      PARAM_SUBTRACT,
			      bse_param_spec_bool ("subtract", "Subtract instead",
						   "Use subtraction to combine sample"
						   "values (instead of addition)",
						   FALSE,
						   BSE_PARAM_DEFAULT));
  
  ichannel = bse_source_class_add_ichannel (source_class, "mono_in1", "Mono Input 1");
  g_assert (ichannel == BSE_ADDER_ICHANNEL_MONO1);
  ichannel = bse_source_class_add_ichannel (source_class, "mono_in2", "Mono Input 2");
  g_assert (ichannel == BSE_ADDER_ICHANNEL_MONO2);
  ichannel = bse_source_class_add_ichannel (source_class, "mono_in3", "Mono Input 3");
  g_assert (ichannel == BSE_ADDER_ICHANNEL_MONO3);
  ichannel = bse_source_class_add_ichannel (source_class, "mono_in4", "Mono Input 4");
  g_assert (ichannel == BSE_ADDER_ICHANNEL_MONO4);
  ochannel = bse_source_class_add_ochannel (source_class, "mono_out", "Mono Output");
  g_assert (ochannel == BSE_ADDER_OCHANNEL_MONO);
}

static void
bse_adder_class_finalize (BseAdderClass *class)
{
  bse_icon_unref (class->sub_icon);
  class->sub_icon = NULL;
}

static void
bse_adder_init (BseAdder *adder)
{
  adder->mix_buffer = NULL;
  adder->subtract = FALSE;
}

static BseIcon*
bse_adder_do_get_icon (BseObject *object)
{
  BseAdder *adder = BSE_ADDER (object);
  
  if (adder->subtract)
    return BSE_ADDER_GET_CLASS (adder)->sub_icon;
  else /* chain parent class' handler */
    return BSE_OBJECT_CLASS (parent_class)->get_icon (object);
}

static void
bse_adder_set_property (BseAdder    *adder,
			guint        param_id,
			GValue      *value,
			GParamSpec  *pspec)
{
  switch (param_id)
    {
    case PARAM_SUBTRACT:
      adder->subtract = g_value_get_boolean (value);
      bse_adder_update_modules (adder, NULL);
      bse_object_notify_icon_changed (BSE_OBJECT (adder));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (adder, param_id, pspec);
      break;
    }
}

static void
bse_adder_get_property (BseAdder    *adder,
			guint        param_id,
			GValue      *value,
			GParamSpec  *pspec)
{
  switch (param_id)
    {
    case PARAM_SUBTRACT:
      g_value_set_boolean (value, adder->subtract);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (adder, param_id, pspec);
      break;
    }
}

typedef struct
{
  gboolean subtract;
} Adder;

static void
adder_process (GslModule *module,
	       guint      n_values)
{
  Adder *add = module->user_data;
  BseSampleValue *wave_out = GSL_MODULE_OBUFFER (module, 0);
  BseSampleValue *bound = wave_out + n_values;
  guint i;

  if (!module->ostreams[0].connected)
    return;     /* nothing to process */
  for (i = 0; i < GSL_MODULE_N_ISTREAMS (module); i++)
    if (module->istreams[i].connected)
      {
	/* found first channel */
	memcpy (wave_out, GSL_MODULE_IBUFFER (module, i), n_values * sizeof (wave_out[0]));
	break;
      }
  if (i >= GSL_MODULE_N_ISTREAMS (module))
    {
      /* no input, FIXME: should set static-0 here */
      memset (wave_out, 0, n_values * sizeof (wave_out[0]));
    }
  if (!add->subtract)
    {
      for (; i < GSL_MODULE_N_ISTREAMS (module); i++)
	if (module->istreams[i].connected)
	  {
	    const BseSampleValue *in = GSL_MODULE_IBUFFER (module, i);
	    BseSampleValue *out = wave_out;
	    
	    /* found 1+nth channel to add on */
	    do
	      *out++ += *in++;
	    while (out < bound);
	  }
    }
  else
    {
      for (; i < GSL_MODULE_N_ISTREAMS (module); i++)
	if (module->istreams[i].connected)
	  {
	    const BseSampleValue *in = GSL_MODULE_IBUFFER (module, i);
	    BseSampleValue *out = wave_out;
	    
	    /* found 1+nth channel to subtract */
	    do
	      *out++ -= *in++;
	    while (out < bound);
	  }
    }
}

static void
bse_adder_update_modules (BseAdder *adder,
			  GslTrans *trans)
{
  if (BSE_SOURCE_PREPARED (adder))
    bse_source_update_omodules (BSE_SOURCE (adder),
				BSE_ADDER_OCHANNEL_MONO,
				G_STRUCT_OFFSET (Adder, subtract),
				&adder->subtract,
				sizeof (adder->subtract),
				trans);
}

static void
bse_adder_context_create (BseSource *source,
			  guint      context_handle,
			  GslTrans  *trans)
{
  static const GslClass add_class = {
    4,				/* n_istreams */
    0,				/* n_jstreams */
    1,				/* n_ostreams */
    adder_process,		/* process */
    (GslModuleFreeFunc) g_free,	/* free */
    GSL_COST_CHEAP,		/* cost */
  };
  BseAdder *adder = BSE_ADDER (source);
  Adder *add = g_new0 (Adder, 1);
  GslModule *module;

  module = gsl_module_new (&add_class, add);

  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);

  /* commit module to engine */
  gsl_trans_add (trans, gsl_job_integrate (module));

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);

  /* update module data */
  bse_adder_update_modules (adder, trans);
}


/* --- Export to BSE --- */
#include "./icons/sum.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_adder, "BseAdder", "BseSource",
    "The Adder is a very simplisitic prototype mixer that just sums up "
    "incomiong signals (it does allow for switching to subtract mode though)",
    &type_info_adder,
    "/Modules/Adder",
    { SUM_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      SUM_IMAGE_WIDTH, SUM_IMAGE_HEIGHT,
      SUM_IMAGE_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
