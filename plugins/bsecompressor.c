/* BseCompressor - BSE Compressor
 * Copyright (C) 1999 Tim Janik
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

#include <bse/bsechunk.h>
#include <bse/bsehunkmixer.h>
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
static void	 bse_compressor_class_finalize	      (BseCompressorClass	*class);
static void	 bse_compressor_set_param	      (BseCompressor		*compr,
						       guint                     param_id,
						       GValue                   *value,
						       GParamSpec               *pspec,
						       const gchar              *trailer);
static void	 bse_compressor_get_param	      (BseCompressor		*compr,
						       guint                     param_id,
						       GValue                   *value,
						       GParamSpec               *pspec,
						       const gchar              *trailer);
static void	 bse_compressor_do_destroy	      (BseObject		*object);
static void	 bse_compressor_prepare		      (BseSource		*source,
						       BseIndex			 index);
static BseChunk* bse_compressor_calc_chunk	      (BseSource		*source,
						       guint			 ochannel_id);
static void	 bse_compressor_reset		      (BseSource		*source);


/* --- variables --- */
static GType		 type_id_compressor = 0;
static gpointer		 parent_class = NULL;
static const GTypeInfo type_info_compressor = {
  sizeof (BseCompressorClass),
  
  (GBaseInitFunc) NULL,
  (GBaseFinalizeFunc) NULL,
  (GClassInitFunc) bse_compressor_class_init,
  (GClassFinalizeFunc) bse_compressor_class_finalize,
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
  
  gobject_class->set_param = (GObjectSetParamFunc) bse_compressor_set_param;
  gobject_class->get_param = (GObjectGetParamFunc) bse_compressor_get_param;

  object_class->destroy = bse_compressor_do_destroy;
  
  source_class->prepare = bse_compressor_prepare;
  source_class->calc_chunk = bse_compressor_calc_chunk;
  source_class->reset = bse_compressor_reset;
  
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_PI_EXP,
			      b_param_spec_float ("pi_exp", "Strength",
						  "The compressor strength allowes for fine grained "
						  "adjustments from extenuated volume to maximum limiting",
						  -1.0, 5.0, 0.0, 0.25,
						  B_PARAM_DEFAULT | B_PARAM_HINT_SCALE));
  
  ichannel_id = bse_source_class_add_ichannel (source_class, "mono_in1", "Mono Input", 1, 1);
  g_assert (ichannel_id == BSE_COMPRESSOR_ICHANNEL_MONO1);
  ochannel_id = bse_source_class_add_ochannel (source_class, "mono_out1", "Mono Output", 1);
  g_assert (ochannel_id == BSE_COMPRESSOR_OCHANNEL_MONO1);
}

static void
bse_compressor_class_finalize (BseCompressorClass *class)
{
}

static void
bse_compressor_init (BseCompressor *compr)
{
  compr->pi_fact = 1.0;
}

static void
bse_compressor_do_destroy (BseObject *object)
{
  BseCompressor *compr;
  
  compr = BSE_COMPRESSOR (object);
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_compressor_set_param (BseCompressor *compr,
			  guint          param_id,
			  GValue        *value,
			  GParamSpec    *pspec,
			  const gchar   *trailer)
{
  switch (param_id)
    {
    case PARAM_PI_EXP:
      compr->pi_fact = pow (PI, b_value_get_float (value));
      break;
    default:
      G_WARN_INVALID_PARAM_ID (compr, param_id, pspec);
      break;
    }
}

static void
bse_compressor_get_param (BseCompressor *compr,
			  guint          param_id,
			  GValue        *value,
			  GParamSpec    *pspec,
			  const gchar   *trailer)
{
  switch (param_id)
    {
    case PARAM_PI_EXP:
      b_value_set_float (value, log (compr->pi_fact) / log (PI));
      break;
    default:
      G_WARN_INVALID_PARAM_ID (compr, param_id, pspec);
      break;
    }
}

static void
bse_compressor_prepare (BseSource *source,
			BseIndex   index)
{
  BseCompressor *compr;
  
  compr = BSE_COMPRESSOR (source);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source, index);
}

static BseChunk*
bse_compressor_calc_chunk (BseSource *source,
			   guint      ochannel_id)
{
  BseCompressor *compr = BSE_COMPRESSOR (source);
  BseSourceInput *input;
  BseChunk *chunk;
  BseSampleValue *hunk, *ihunk;
  gdouble isample_fact, osample_fact;
  guint i;
  
  g_return_val_if_fail (ochannel_id == BSE_COMPRESSOR_OCHANNEL_MONO1, NULL);
  
  input = bse_source_get_input (source, BSE_COMPRESSOR_ICHANNEL_MONO1);
  if (!input)
    return bse_chunk_new_static_zero (1);
  
  chunk = bse_source_ref_chunk (input->osource, input->ochannel_id, source->index);
  
  ihunk = bse_chunk_complete_hunk (chunk);
  hunk = bse_hunk_alloc (1);
  
  isample_fact = compr->pi_fact / BSE_MAX_SAMPLE_VALUE_f;
  osample_fact = 2.0 / PI * BSE_MAX_SAMPLE_VALUE_f;
  
  for (i = 0; i < BSE_TRACK_LENGTH; i++)
    hunk[i] = atan (ihunk[i] * isample_fact) * osample_fact;
  
  bse_chunk_unref (chunk);
  
  return bse_chunk_new_orphan (1, hunk);
}

static void
bse_compressor_reset (BseSource *source)
{
  BseCompressor *compr;
  
  compr = BSE_COMPRESSOR (source);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}


/* --- Export to BSE --- */
#include "./icons/atan.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_compressor, "BseCompressor", "BseSource",
    "BseCompressor compresses according to the current Strength setting using "
    "the formula: output = atan (input * (Pi ^ Strength)), which allowes for "
    "fine grained adjustments from extenuated volume to maximum limiting",
    &type_info_compressor,
    "/Source/Compressor",
    { ATAN_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      ATAN_IMAGE_WIDTH, ATAN_IMAGE_HEIGHT,
      ATAN_IMAGE_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
