/* BseNoise - BSE Noise generator
 * Copyright (C) 1999,2000-2001 Tim Janik
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
#include "bsenoise.h"

#include <bse/gslengine.h>
#include <stdlib.h>
#include <time.h>


/* --- prototypes --- */
static void	 bse_noise_init			(BseNoise	*noise);
static void	 bse_noise_class_init		(BseNoiseClass	*class);
static void	 bse_noise_class_finalize	(BseNoiseClass	*class);
static void	 bse_noise_prepare		(BseSource	*source);
static void	 bse_noise_context_create	(BseSource	*source,
						 guint		 context_handle,
						 GslTrans	*trans);
static void	 bse_noise_reset		(BseSource	*source);


/* --- variables --- */
static GType		 type_id_noise = 0;
static gpointer		 parent_class = NULL;
static const GTypeInfo type_info_noise = {
  sizeof (BseNoiseClass),
  
  (GBaseInitFunc) NULL,
  (GBaseFinalizeFunc) NULL,
  (GClassInitFunc) bse_noise_class_init,
  (GClassFinalizeFunc) bse_noise_class_finalize,
  NULL /* class_data */,
  
  sizeof (BseNoise),
  0 /* n_preallocs */,
  (GInstanceInitFunc) bse_noise_init,
};


/* --- functions --- */
static void
bse_noise_class_init (BseNoiseClass *class)
{
  BseObjectClass *object_class;
  BseSourceClass *source_class;
  guint ochannel_id;
  
  parent_class = g_type_class_peek (BSE_TYPE_SOURCE);
  object_class = BSE_OBJECT_CLASS (class);
  source_class = BSE_SOURCE_CLASS (class);
  
  source_class->prepare = bse_noise_prepare;
  source_class->context_create = bse_noise_context_create;
  source_class->reset = bse_noise_reset;
  
  ochannel_id = bse_source_class_add_ochannel (source_class, "noise_out", "Noise Output");
  g_assert (ochannel_id == BSE_NOISE_OCHANNEL_NOISE);
}

static void
bse_noise_class_finalize (BseNoiseClass *class)
{
}

static void
bse_noise_init (BseNoise *noise)
{
}

#define N_STATIC_BLOCKS (19) /* FIXME: need n_blocks_per_second() */

static void
bse_noise_prepare (BseSource *source)
{
  BseNoise *noise = BSE_NOISE (source);
  guint i, l;
  
  l = BSE_BLOCK_N_VALUES * (N_STATIC_BLOCKS + 1);
  noise->static_noise = g_new (BseSampleValue, l);
  
  srand (time (NULL));
  for (i = 0; i < l; i++)
    noise->static_noise[i] = 1.0 - rand () / (0.5 * RAND_MAX);	// FIXME: should have class noise
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

static void
noise_process (GslModule *module,
	       guint      n_values)
{
  BseSampleValue *static_noise = module->user_data;

  g_return_if_fail (n_values <= BSE_BLOCK_N_VALUES); /* paranoid */

  GSL_MODULE_OBUFFER (module, 0) = static_noise + (rand () % (BSE_BLOCK_N_VALUES * N_STATIC_BLOCKS));
}

static void
bse_noise_context_create (BseSource *source,
			  guint      context_handle,
			  GslTrans  *trans)
{
  static const GslClass output_mclass = {
    0,			/* n_istreams */
    0,                  /* n_jstreams */
    1,			/* n_ostreams */
    noise_process,	/* process */
    NULL,		/* free */
    GSL_COST_CHEAP,	/* cost */
  };
  BseNoise *noise = BSE_NOISE (source);
  GslModule *module = gsl_module_new (&output_mclass, noise->static_noise);

  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);

  /* commit module to engine */
  gsl_trans_add (trans, gsl_job_integrate (module));

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

static void
bse_noise_reset (BseSource *source)
{
  BseNoise *noise = BSE_NOISE (source);
  
  g_free (noise->static_noise);
  noise->static_noise = NULL;
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}


/* --- Export to BSE --- */
#include "./icons/noise.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_noise, "BseNoise", "BseSource",
    "Noise is a generator of (supposedly) white noise",
    &type_info_noise,
    "/Modules/Noise",
    { NOISE_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      NOISE_IMAGE_WIDTH, NOISE_IMAGE_HEIGHT,
      NOISE_IMAGE_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
