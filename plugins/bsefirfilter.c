/* BseFIRFilter - BSE Finite Impulse Response Filter
 * Copyright (C) 1999 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library FIReral Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU FIReral Public License for more details.
 *
 * You should have received a copy of the GNU Library FIReral Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bsefirfilter.h"

#include <bse/bsechunk.h>


/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_ALLPASS,
  PARAM_LOWPASS,
  PARAM_HIGHPASS,
  PARAM_DEGREE,
  PARAM_LANCZOS,
  PARAM_HANN,
  PARAM_CUT_OFF_FREQ,
  PARAM_CUT_OFF_NOTE
};


/* --- prototypes --- */
static void	   bse_fir_filter_init		 (BseFIRFilter	    *fir_filter);
static void	   bse_fir_filter_class_init	 (BseFIRFilterClass *class);
static void	   bse_fir_filter_class_finalize (BseFIRFilterClass *class);
static void	   bse_fir_filter_do_destroy	 (BseObject	    *object);
static void	   bse_fir_filter_set_property	 (BseFIRFilter	    *fir_filter,
						  guint              param_id,
						  GValue            *value,
						  GParamSpec        *pspec);
static void	   bse_fir_filter_get_property	 (BseFIRFilter	    *fir_filter,
						  guint              param_id,
						  GValue            *value,
						  GParamSpec        *pspec);
static void	   bse_fir_filter_prepare	 (BseSource	    *source,
						  BseIndex	     index);
static BseChunk*   bse_fir_filter_calc_chunk	 (BseSource	    *source,
						  guint		     ochannel_id);
static void	   bse_fir_filter_reset		 (BseSource	    *source);
static inline void bse_fir_filter_update_locals	 (BseFIRFilter	    *filter);


/* --- variables --- */
static GType		 type_id_fir_filter = 0;
static gpointer		 parent_class = NULL;
static const GTypeInfo type_info_fir_filter = {
  sizeof (BseFIRFilterClass),
  
  (GBaseInitFunc) NULL,
  (GBaseFinalizeFunc) NULL,
  (GClassInitFunc) bse_fir_filter_class_init,
  (GClassFinalizeFunc) bse_fir_filter_class_finalize,
  NULL /* class_data */,
  
  sizeof (BseFIRFilter),
  0 /* n_preallocs */,
  (GInstanceInitFunc) bse_fir_filter_init,
};


/* --- functions --- */
static void
bse_fir_filter_class_init (BseFIRFilterClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ochannel_id, ichannel_id;
  
  parent_class = g_type_class_peek (BSE_TYPE_SOURCE);
  
  gobject_class->set_property = (GObjectSetPropertyFunc) bse_fir_filter_set_property;
  gobject_class->get_property = (GObjectGetPropertyFunc) bse_fir_filter_get_property;

  object_class->destroy = bse_fir_filter_do_destroy;
  
  source_class->prepare = bse_fir_filter_prepare;
  source_class->calc_chunk = bse_fir_filter_calc_chunk;
  source_class->reset = bse_fir_filter_reset;
  
  bse_object_class_add_param (object_class, "Filter Type",
			      PARAM_ALLPASS,
			      bse_param_spec_bool ("allpass", "AllPass", NULL,
						 FALSE,
						 BSE_PARAM_DEFAULT | BSE_PARAM_HINT_RADIO));
  bse_object_class_add_param (object_class, "Filter Type",
			      PARAM_LOWPASS,
			      bse_param_spec_bool ("lowpass", "LowPass", NULL,
						 TRUE,
						 BSE_PARAM_DEFAULT | BSE_PARAM_HINT_RADIO));
  bse_object_class_add_param (object_class, "Filter Type",
			      PARAM_HIGHPASS,
			      bse_param_spec_bool ("highpass", "HighPass", NULL,
						 FALSE,
						 BSE_PARAM_DEFAULT | BSE_PARAM_HINT_RADIO));
  bse_object_class_add_param (object_class, NULL,
			      PARAM_DEGREE,
			      bse_param_spec_uint ("degree", "Degree", "Number of filter coefficients",
						 2, 256,
						 6, 4,
						 BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Smoothing",
			      PARAM_HANN,
			      bse_param_spec_bool ("hann_smooth", "von Hann", NULL,
						 FALSE,
						 BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Smoothing",
			      PARAM_LANCZOS,
			      bse_param_spec_bool ("lanczos_smooth", "C. Lanczos", NULL,
						 FALSE,
						 BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Cut off",
			      PARAM_CUT_OFF_FREQ,
			      bse_param_spec_float ("cut_off_freq", "Frequency", NULL,
						  BSE_MIN_OSC_FREQ_d, BSE_MAX_OSC_FREQ_d,
						  BSE_KAMMER_FREQ / 2, 5.0,
						  BSE_PARAM_DEFAULT |
						  BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Cut off",
			      PARAM_CUT_OFF_NOTE,
			      bse_param_spec_note ("cut_off_note", "Note", NULL,
						 BSE_MIN_NOTE, BSE_MAX_NOTE,
						 bse_note_from_freq (BSE_KAMMER_FREQ / 2), 1,
						 TRUE,
						 BSE_PARAM_GUI));
  
  ochannel_id = bse_source_class_add_ochannel (source_class, "mono_out", "Mono Filtered Output", 1);
  g_assert (ochannel_id == BSE_FIR_FILTER_OCHANNEL_MONO);
  ichannel_id = bse_source_class_add_ichannel (source_class, "mono_in", "Mono Signal Input", 1, 1);
  g_assert (ichannel_id == BSE_FIR_FILTER_ICHANNEL_MONO);
  // BSE_SOURCE_CLASS_ICHANNEL_DEF (class, BSE_FIR_FILTER_ICHANNEL_MONO)->history = 2;
}

static void
bse_fir_filter_class_finalize (BseFIRFilterClass *class)
{
}

static void
bse_fir_filter_init (BseFIRFilter *filter)
{
  filter->degree = 6;
  filter->lanczos_smoothing = FALSE;
  filter->hann_smoothing = FALSE;
  filter->cut_off_freq = BSE_KAMMER_FREQ / 2;
  filter->filter_type = BSE_FIR_FILTER_LOWPASS;
  filter->n_coeffs = 0;
  filter->coeffs = NULL;
  filter->history_pos = 0;
  filter->history = NULL;
}

static void
bse_fir_filter_do_destroy (BseObject *object)
{
  BseFIRFilter *fir_filter;
  
  fir_filter = BSE_FIR_FILTER (object);
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static inline void
bse_fir_filter_update_locals (BseFIRFilter *filter)
{
  gint i;
  
  if (BSE_SOURCE_PREPARED (filter))
    {
      gdouble d, c;
      gint z = filter->n_coeffs;
      
      filter->n_coeffs = MIN (filter->degree, BSE_TRACK_LENGTH) * 2 + 1;
      filter->history = g_renew (BseSampleValue, filter->history, filter->n_coeffs);
      for (i = z; i < filter->n_coeffs; i++)
	filter->history[i] = 0;
      z = filter->n_coeffs / 2;
      filter->history_pos %= filter->n_coeffs;
      
      /* setup allpass
       */
      g_free (filter->coeffs);
      filter->coeffs = g_new (gfloat, filter->n_coeffs);
      for (i = 0; i < filter->n_coeffs; i++)
	filter->coeffs[i] = 0;
      filter->coeffs[z] = 1;
      
      /* setup lowpass (precalculate highpass)
       */
      if (filter->filter_type == BSE_FIR_FILTER_LOWPASS ||
	  filter->filter_type == BSE_FIR_FILTER_HIGHPASS)
	{
	  d = 2 * filter->cut_off_freq;
	  d /= BSE_MIX_FREQ;
	  c = d * PI;
	  for (i = 0; i < filter->n_coeffs; i++)
	    {
	      if (i == z)
		filter->coeffs[i] = d;
	      else
		{
		  gdouble k = i - z;
		  
		  filter->coeffs[i] = d * sin (c * k) / (c * k);
		}
	    }
	}
      
      if (filter->lanczos_smoothing)
	{
	  c = z;
	  c = PI / c;
	  for (i = 0; i < filter->n_coeffs; i++)
	    {
	      gdouble k = (i - z);
	      
	      if (k)
		filter->coeffs[i] *= sin (c * k) / (c * k);
	    }
	}
      
      if (filter->hann_smoothing)
	{
	  c = z;
	  c = PI / c;
	  for (i = 0; i < filter->n_coeffs; i++)
	    {
	      gdouble k = (i - z);
	      
	      filter->coeffs[i] *= 0.5 * (1.0 + cos (c * k));
	    }
	}
      
      /* setup highpass (from lowpass)
       */
      if (filter->filter_type == BSE_FIR_FILTER_HIGHPASS)
	for (i = 0; i < filter->n_coeffs; i++)
	  {
	    if (i == z)
	      filter->coeffs[i] = 1.0 - filter->coeffs[i];
	    else
	      filter->coeffs[i] = - filter->coeffs[i];
	  }
      
#if 0
      for (i = 0; i < filter->n_coeffs; i++)
	g_print ("a[%d]=%f ", i - z, filter->coeffs[i]);
      g_print ("\n=======\n");
#endif
    }
}

static void
bse_fir_filter_set_property (BseFIRFilter *filter,
			     guint         param_id,
			     GValue       *value,
			     GParamSpec   *pspec)
{
  BseFIRFilterType filter_type = BSE_FIR_FILTER_ALLPASS;
  
  switch (param_id)
    {
      guint v_uint;
    case PARAM_DEGREE:
      v_uint = g_value_get_uint (value);
      filter->degree = (v_uint + (v_uint > 2 * filter->degree)) / 2;
      bse_fir_filter_update_locals (filter);
      break;
    case PARAM_CUT_OFF_FREQ:
      filter->cut_off_freq = g_value_get_float (value);
      bse_fir_filter_update_locals (filter);
      bse_object_param_changed (BSE_OBJECT (filter), "cut_off_note");
      break;
    case PARAM_CUT_OFF_NOTE:
      filter->cut_off_freq = bse_note_to_freq (bse_value_get_note (value));
      filter->cut_off_freq = MAX (filter->cut_off_freq, BSE_MIN_OSC_FREQ_d);
      bse_fir_filter_update_locals (filter);
      bse_object_param_changed (BSE_OBJECT (filter), "cut_off_freq");
      if (bse_note_from_freq (filter->cut_off_freq) != bse_value_get_note (value))
	bse_object_param_changed (BSE_OBJECT (filter), "cut_off_note");
      break;
    case PARAM_LANCZOS:
      filter->lanczos_smoothing = g_value_get_boolean (value);
      bse_fir_filter_update_locals (filter);
      break;
    case PARAM_HANN:
      filter->hann_smoothing = g_value_get_boolean (value);
      bse_fir_filter_update_locals (filter);
      break;
    case PARAM_HIGHPASS:
      filter_type++;
      /* fall through */
    case PARAM_LOWPASS:
      filter_type++;
      /* fall through */
    case PARAM_ALLPASS:
      if (g_value_get_boolean (value))
	{
	  filter->filter_type = filter_type;
	  bse_fir_filter_update_locals (filter);
	  bse_object_param_changed (BSE_OBJECT (filter), "allpass");
	  bse_object_param_changed (BSE_OBJECT (filter), "lowpass");
	  bse_object_param_changed (BSE_OBJECT (filter), "highpass");
	}
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (filter, param_id, pspec);
      break;
    }
}

static void
bse_fir_filter_get_property (BseFIRFilter *filter,
			     guint         param_id,
			     GValue       *value,
			     GParamSpec   *pspec)
{
  switch (param_id)
    {
    case PARAM_DEGREE:
      g_value_set_uint (value, filter->degree * 2);
      break;
    case PARAM_CUT_OFF_FREQ:
      g_value_set_float (value, filter->cut_off_freq);
      break;
    case PARAM_CUT_OFF_NOTE:
      bse_value_set_note (value, bse_note_from_freq (filter->cut_off_freq));
      break;
    case PARAM_LANCZOS:
      g_value_set_boolean (value, filter->lanczos_smoothing);
      break;
    case PARAM_HANN:
      g_value_set_boolean (value, filter->hann_smoothing);
      break;
    case PARAM_ALLPASS:
      g_value_set_boolean (value, filter->filter_type == BSE_FIR_FILTER_ALLPASS);
      break;
    case PARAM_LOWPASS:
      g_value_set_boolean (value, filter->filter_type == BSE_FIR_FILTER_LOWPASS);
      break;
    case PARAM_HIGHPASS:
      g_value_set_boolean (value, filter->filter_type == BSE_FIR_FILTER_HIGHPASS);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (filter, param_id, pspec);
      break;
    }
}

static void
bse_fir_filter_prepare (BseSource *source,
			BseIndex   index)
{
  BseFIRFilter *filter = BSE_FIR_FILTER (source);
  
  bse_fir_filter_update_locals (filter);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source, index);
}

static BseChunk*
bse_fir_filter_calc_chunk (BseSource *source,
			   guint      ochannel_id)
{
  BseFIRFilter *filter = BSE_FIR_FILTER (source);
  BseSourceInput *input;
  BseChunk *ichunk;
  BseSampleValue *hunk, *ihunk, *hvals;
  gfloat *coeffs;
  guint i, n_coeffs, history_pos;
  
  g_return_val_if_fail (ochannel_id == BSE_FIR_FILTER_OCHANNEL_MONO, NULL);
  
  input = bse_source_get_input (source, BSE_FIR_FILTER_ICHANNEL_MONO); /* mono */
  if (!input) /* silence */
    return bse_chunk_new_static_zero (1);
  
  ichunk = bse_source_ref_chunk (input->osource, input->ochannel_id, source->index);
  bse_chunk_complete_hunk (ichunk);
  ihunk = ichunk->hunk;
  
  if (filter->filter_type == BSE_FIR_FILTER_ALLPASS)
    return ichunk;
  
  hunk = bse_hunk_alloc (1);
  
  n_coeffs = filter->n_coeffs;
  coeffs = filter->coeffs;
  history_pos = filter->history_pos;
  hvals = filter->history;
  for (i = 0; i < BSE_TRACK_LENGTH; i++)
    {
      gfloat *a;
      guint n;
      
      a = coeffs;
      hunk[i] = 0;
      for (n = history_pos; n < n_coeffs; n++)
	hunk[i] += (BseSampleValue) (*(a++) * hvals[n]);
      for (n = 0; n < history_pos; n++)
	hunk[i] += (BseSampleValue) (*(a++) * hvals[n]);
      
      hvals[history_pos++] = ihunk[i];
      if (history_pos >= n_coeffs)
	history_pos -= n_coeffs;
    }
  filter->history_pos = history_pos;
  
  bse_chunk_unref (ichunk);
  
  return bse_chunk_new_orphan (1, hunk);
}

static void
bse_fir_filter_reset (BseSource *source)
{
  BseFIRFilter *filter = BSE_FIR_FILTER (source);
  
  filter->n_coeffs = 0;
  g_free (filter->coeffs);
  filter->coeffs = NULL;
  filter->history_pos = 0;
  g_free (filter->history);
  filter->history = NULL;
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}


/* --- Export to BSE --- */
#include "./icons/filter.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_fir_filter, "BseFIRFilter", "BseSource",
    "BseFIRFilter is a finite impulse response filter",
    &type_info_fir_filter,
    "/Source/FIRFilter",
    { FILTER_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      FILTER_IMAGE_WIDTH, FILTER_IMAGE_HEIGHT,
      FILTER_IMAGE_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
