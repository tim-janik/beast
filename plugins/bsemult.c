/* BseMult - BSE Multiplier
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
#include "bsemult.h"

#include <bse/gslengine.h>

#include <string.h>

/* --- prototypes --- */
static void	 bse_mult_init			(BseMult	*mult);
static void	 bse_mult_class_init		(BseMultClass	*class);
static void	 bse_mult_context_create	(BseSource	*source,
						 guint		 context_handle,
						 GslTrans	*trans);


/* --- variables --- */
static GType		 type_id_mult = 0;
static gpointer		 parent_class = NULL;
static const GTypeInfo type_info_mult = {
  sizeof (BseMultClass),
  
  (GBaseInitFunc) NULL,
  (GBaseFinalizeFunc) NULL,
  (GClassInitFunc) bse_mult_class_init,
  (GClassFinalizeFunc) NULL,
  NULL /* class_data */,
  
  sizeof (BseMult),
  0 /* n_preallocs */,
  (GInstanceInitFunc) bse_mult_init,
};


/* --- functions --- */
static void
bse_mult_class_init (BseMultClass *class)
{
  BseObjectClass *object_class;
  BseSourceClass *source_class;
  guint ichannel, ochannel;
  
  parent_class = g_type_class_peek (BSE_TYPE_SOURCE);
  object_class = BSE_OBJECT_CLASS (class);
  source_class = BSE_SOURCE_CLASS (class);
  
  source_class->context_create = bse_mult_context_create;
  
  ichannel = bse_source_class_add_ichannel (source_class, "Audio In1", "Audio Input 1");
  g_assert (ichannel == BSE_MULT_ICHANNEL_MONO1);
  ichannel = bse_source_class_add_ichannel (source_class, "Audio In2", "Audio Input 2");
  g_assert (ichannel == BSE_MULT_ICHANNEL_MONO2);
  ichannel = bse_source_class_add_ichannel (source_class, "Audio In3", "Audio Input 3");
  g_assert (ichannel == BSE_MULT_ICHANNEL_MONO3);
  ichannel = bse_source_class_add_ichannel (source_class, "Audio In4", "Audio Input 4");
  g_assert (ichannel == BSE_MULT_ICHANNEL_MONO4);
  ochannel = bse_source_class_add_ochannel (source_class, "Audio Out", "Audio Output");
  g_assert (ochannel == BSE_MULT_OCHANNEL_MONO);
}

static void
bse_mult_init (BseMult *mult)
{
}

static void
multiply_process (GslModule *module,
		  guint      n_values)
{
  // = module->user_data;
  gfloat *wave_out = GSL_MODULE_OBUFFER (module, 0);
  gfloat *bound = wave_out + n_values;
  guint i;
  
  if (!module->ostreams[0].connected)
    return;	/* nothing to process */
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
  for (i += 1; i < GSL_MODULE_N_ISTREAMS (module); i++)
    if (module->istreams[i].connected)
      {
	const gfloat *in = GSL_MODULE_IBUFFER (module, i);
	gfloat *out = wave_out;
	
	/* found 1+nth channel to multiply with */
	do
	  *out++ *= *in++;
	while (out < bound);
      }
}

static void
bse_mult_context_create (BseSource *source,
			 guint      context_handle,
			 GslTrans  *trans)
{
  static const GslClass multiply_class = {
    4,                          /* n_istreams */
    0,                          /* n_jstreams */
    1,                          /* n_ostreams */
    multiply_process,           /* process */
    NULL,                       /* process_defer */
    NULL,                       /* reset */
    NULL,                       /* free */
    GSL_COST_CHEAP,             /* cost */
  };
  // BseMult *mult = BSE_MULT (source);
  GslModule *module;
  
  module = gsl_module_new (&multiply_class, NULL);
  
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);
  
  /* commit module to engine */
  gsl_trans_add (trans, gsl_job_integrate (module));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}


/* --- Export to BSE --- */
#include "./icons/prod.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_mult, "BseMult", "BseSource",
    "Mult is a channel multiplier for ring-modulating incoming signals",
    &type_info_mult,
    "/Modules/Routing/Mult",
    { PROD_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      PROD_IMAGE_WIDTH, PROD_IMAGE_HEIGHT,
      PROD_IMAGE_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
