/* BseIIRFilter - BSE Infinite Impulse Response Filter
 * Copyright (C) 1999-2002 Tim Janik
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
#include "bseiirfilter.h"

#include <bse/gslengine.h>
#include <bse/gslfilter.h>


/* include generated enums
 */
#include "bseiirfilter.enums"

#define	_(x)	(x)
#define	FREQ_DELTA	0.1

/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_FILTER_ALGO,
  PARAM_FILTER_TYPE,
  PARAM_ORDER,
  PARAM_EPSILON,
  PARAM_CUT_OFF_FREQ1,
  PARAM_CUT_OFF_NOTE1,
  PARAM_CUT_OFF_FREQ2,
  PARAM_CUT_OFF_NOTE2
};


/* --- prototypes --- */
static void	   bse_iir_filter_init			(BseIIRFilter		*iir_filter);
static void	   bse_iir_filter_class_init		(BseIIRFilterClass	*class);
static void	   bse_iir_filter_class_finalize	(BseIIRFilterClass	*class);
static void	   bse_iir_filter_set_property		(GObject		*object,
							 guint			 param_id,
							 const GValue		*value,
							 GParamSpec		*pspec);
static void	   bse_iir_filter_get_property		(GObject		*object,
							 guint			 param_id,
							 GValue			*value,
							 GParamSpec		*pspec);
static void	   bse_iir_filter_prepare		(BseSource		*source);
static void	   bse_iir_filter_context_create	(BseSource		*source,
							 guint			 context_handle,
							 GslTrans		*trans);
static void	   bse_iir_filter_update_modules	(BseIIRFilter		*filt);


/* --- variables --- */
static GType	       type_id_iir_filter = 0;
static gpointer	       parent_class = NULL;
static const GTypeInfo type_info_iir_filter = {
  sizeof (BseIIRFilterClass),
  
  (GBaseInitFunc) NULL,
  (GBaseFinalizeFunc) NULL,
  (GClassInitFunc) bse_iir_filter_class_init,
  (GClassFinalizeFunc) bse_iir_filter_class_finalize,
  NULL /* class_data */,
  
  sizeof (BseIIRFilter),
  0 /* n_preallocs */,
  (GInstanceInitFunc) bse_iir_filter_init,
};


/* --- functions --- */
static void
bse_iir_filter_class_init (BseIIRFilterClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ochannel_id, ichannel_id;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_iir_filter_set_property;
  gobject_class->get_property = bse_iir_filter_get_property;

  source_class->prepare = bse_iir_filter_prepare;
  source_class->context_create = bse_iir_filter_context_create;
  
  bse_object_class_add_param (object_class, _("Filter Choice"),
			      PARAM_FILTER_ALGO,
			      bse_param_spec_enum ("filter_algorithm", _("Filter Algorithm"), _("The filter design type"),
						   BSE_TYPE_IIR_FILTER_ALGORITHM,
						   BSE_IIR_FILTER_BUTTERWORTH,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, _("Filter Choice"),
			      PARAM_FILTER_TYPE,
			      bse_param_spec_enum ("filter_type", _("Filter Type"), _("The type of filter to use"),
						   BSE_TYPE_IIR_FILTER_TYPE,
						   BSE_IIR_FILTER_LOW_PASS,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, _("Filter Specification"),
			      PARAM_ORDER,
			      bse_param_spec_uint ("order", _("Order"), _("Order of Filter"),
						   1, BSE_IIR_FILTER_MAX_ORDER,
						   6, 2,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, _("Filter Specification"),
			      PARAM_EPSILON,
			      bse_param_spec_float ("epsilon", _("Epsilon"), _("Passband falloff at cutoff frequency"),
						    0.0, 0.98,
						    0.1, 0.01,
						    BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, _("Cutoff Frequency (All Filters)"),
			      PARAM_CUT_OFF_FREQ1,
			      bse_param_spec_float ("cut_off_freq", _("Cutoff [Hz]"), NULL,
						    BSE_MIN_OSC_FREQ_d, BSE_MAX_OSC_FREQ_d - FREQ_DELTA,
						    BSE_KAMMER_FREQ / 2, 5.0,
						    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_DIAL));
  bse_object_class_set_param_log_scale (object_class, "cut_off_freq", 880.0, 2, 4);
  bse_object_class_add_param (object_class, _("Cutoff Frequency (All Filters)"),
			      PARAM_CUT_OFF_NOTE1,
			      bse_param_spec_note ("cut_off_note", _("Note"), NULL,
						   BSE_MIN_NOTE, BSE_MAX_NOTE,
						   bse_note_from_freq (BSE_KAMMER_FREQ / 2), 1,
						   TRUE,
						   BSE_PARAM_GUI));
  bse_object_class_add_param (object_class, _("Cutoff Frequency 2 (Band Pass/Stop)"),
			      PARAM_CUT_OFF_FREQ2,
			      bse_param_spec_float ("cut_off_freq_2", _("Cutoff [Hz]"), NULL,
						    BSE_MIN_OSC_FREQ_d + FREQ_DELTA, BSE_MAX_OSC_FREQ_d,
						    BSE_KAMMER_FREQ / 2 + FREQ_DELTA, 5.0,
						    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_DIAL));
  bse_object_class_set_param_log_scale (object_class, "cut_off_freq_2", 880.0, 2, 4);
  bse_object_class_add_param (object_class, _("Cutoff Frequency 2 (Band Pass/Stop)"),
			      PARAM_CUT_OFF_NOTE2,
			      bse_param_spec_note ("cut_off_note_2", _("Note"), NULL,
						   BSE_MIN_NOTE, BSE_MAX_NOTE,
						   bse_note_from_freq (BSE_KAMMER_FREQ / 2 + FREQ_DELTA), 1,
						   TRUE,
						   BSE_PARAM_GUI));
  
  ochannel_id = bse_source_class_add_ochannel (source_class, "mono_out", _("Filtered Output"));
  g_assert (ochannel_id == BSE_IIR_FILTER_OCHANNEL_MONO);
  ichannel_id = bse_source_class_add_ichannel (source_class, "mono_in", _("Unfiltered Input"));
  g_assert (ichannel_id == BSE_IIR_FILTER_ICHANNEL_MONO);
}

static void
bse_iir_filter_class_finalize (BseIIRFilterClass *class)
{
}

static void
bse_iir_filter_init (BseIIRFilter *filt)
{
  filt->filter_algo = BSE_IIR_FILTER_BUTTERWORTH;
  filt->filter_type = BSE_IIR_FILTER_LOW_PASS;
  filt->order = 6;
  filt->epsilon = 0.1;
  filt->cut_off_freq1 = BSE_KAMMER_FREQ / 2;
  filt->cut_off_freq2 = filt->cut_off_freq1 + FREQ_DELTA;
  filt->filter_type = BSE_IIR_FILTER_BUTTERWORTH;
  bse_iir_filter_update_modules (filt);
}

static void
bse_iir_filter_set_property (GObject	  *object,
			     guint	   param_id,
			     const GValue *value,
			     GParamSpec	  *pspec)
{
  BseIIRFilter *filt = BSE_IIR_FILTER (object);
  
  switch (param_id)
    {
    case PARAM_FILTER_ALGO:
      filt->filter_algo = g_value_get_enum (value);
      filt->algo_type_change = TRUE;
      bse_iir_filter_update_modules (filt);
      break;
    case PARAM_FILTER_TYPE:
      filt->filter_type = g_value_get_enum (value);
      filt->algo_type_change = TRUE;
      bse_iir_filter_update_modules (filt);
      break;
    case PARAM_ORDER:
      filt->order = g_value_get_uint (value);
      bse_iir_filter_update_modules (filt);
      break;
    case PARAM_EPSILON:
      filt->epsilon = g_value_get_float (value);
      bse_iir_filter_update_modules (filt);
      break;
    case PARAM_CUT_OFF_FREQ1:
      filt->cut_off_freq1 = g_value_get_float (value);
      if (filt->cut_off_freq1 + FREQ_DELTA > filt->cut_off_freq2)
	{
	  filt->cut_off_freq2 = filt->cut_off_freq1 + FREQ_DELTA;
	  g_object_notify (filt, "cut_off_freq_2");
	  g_object_notify (filt, "cut_off_note_2");
	}
      bse_iir_filter_update_modules (filt);
      g_object_notify (filt, "cut_off_note");
      break;
    case PARAM_CUT_OFF_NOTE1:
      filt->cut_off_freq1 = bse_note_to_freq (bse_value_get_note (value));
      filt->cut_off_freq1 = MAX (filt->cut_off_freq1, BSE_MIN_OSC_FREQ_d);
      if (filt->cut_off_freq1 + FREQ_DELTA > filt->cut_off_freq2)
	{
	  filt->cut_off_freq2 = filt->cut_off_freq1 + FREQ_DELTA;
	  g_object_notify (filt, "cut_off_freq_2");
	  g_object_notify (filt, "cut_off_note_2");
	}
      bse_iir_filter_update_modules (filt);
      g_object_notify (filt, "cut_off_freq");
      break;
    case PARAM_CUT_OFF_FREQ2:
      filt->cut_off_freq2 = g_value_get_float (value);
      if (filt->cut_off_freq1 + FREQ_DELTA > filt->cut_off_freq2)
	{
	  filt->cut_off_freq1 = filt->cut_off_freq2 - FREQ_DELTA;
	  g_object_notify (filt, "cut_off_freq");
	  g_object_notify (filt, "cut_off_note");
	}
      bse_iir_filter_update_modules (filt);
      g_object_notify (filt, "cut_off_note_2");
      break;
    case PARAM_CUT_OFF_NOTE2:
      filt->cut_off_freq2 = bse_note_to_freq (bse_value_get_note (value));
      filt->cut_off_freq2 = MAX (filt->cut_off_freq2, BSE_MIN_OSC_FREQ_d);
      if (filt->cut_off_freq1 + FREQ_DELTA > filt->cut_off_freq2)
	{
	  filt->cut_off_freq1 = filt->cut_off_freq2 - FREQ_DELTA;
	  g_object_notify (filt, "cut_off_freq");
	  g_object_notify (filt, "cut_off_note");
	}
      bse_iir_filter_update_modules (filt);
      g_object_notify (filt, "cut_off_freq_2");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (filt, param_id, pspec);
      break;
    }
}

static void
bse_iir_filter_get_property (GObject	*object,
			     guint	 param_id,
			     GValue	*value,
			     GParamSpec	*pspec)
{
  BseIIRFilter *filt = BSE_IIR_FILTER (object);

  switch (param_id)
    {
    case PARAM_FILTER_ALGO:
      g_value_set_enum (value, filt->filter_algo);
      break;
    case PARAM_FILTER_TYPE:
      g_value_set_enum (value, filt->filter_type);
      break;
    case PARAM_ORDER:
      g_value_set_uint (value, filt->order);
      break;
    case PARAM_EPSILON:
      g_value_set_float (value, filt->epsilon);
      break;
    case PARAM_CUT_OFF_FREQ1:
      g_value_set_float (value, filt->cut_off_freq1);
      break;
    case PARAM_CUT_OFF_NOTE1:
      bse_value_set_note (value, bse_note_from_freq (filt->cut_off_freq1));
      break;
    case PARAM_CUT_OFF_FREQ2:
      g_value_set_float (value, filt->cut_off_freq2);
      break;
    case PARAM_CUT_OFF_NOTE2:
      bse_value_set_note (value, bse_note_from_freq (filt->cut_off_freq2));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (filt, param_id, pspec);
      break;
    }
}

static void
bse_iir_filter_prepare (BseSource *source)
{
  BseIIRFilter *filt = BSE_IIR_FILTER (source);

  /* need to call update_modules() because we only now have gsl_engine_sample_freq() */
  bse_iir_filter_update_modules (filt);

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

typedef struct {
  GslIIRFilter	iir;
  gdouble	dummy[(BSE_IIR_FILTER_MAX_ORDER + 1) * 4];
} FilterModule;

static void
iir_filter_access (GslModule *module,
		   gpointer   data)
{
  FilterModule *fmod = module->user_data;
  FilterModule *src = data;

  if (src->iir.w)	/* algo_type_change */
    gsl_iir_filter_setup (&fmod->iir, src->iir.order, src->iir.a, src->iir.b, fmod->dummy);
  else
    gsl_iir_filter_change (&fmod->iir, src->iir.order, src->iir.a, src->iir.b, fmod->dummy);
}

static void
bse_iir_filter_update_modules (BseIIRFilter *filt)
{

  if (BSE_SOURCE_PREPARED (filt))
    {
      FilterModule *fmod = g_new0 (FilterModule, 1);
      gfloat nyquist_fact = GSL_PI / (0.5 * gsl_engine_sample_freq ());
      gfloat freq1 = MIN (filt->cut_off_freq1, 0.5 * gsl_engine_sample_freq ());
      gfloat freq2 = MIN (filt->cut_off_freq2, 0.5 * gsl_engine_sample_freq ());
      gfloat steepness = 1.1;

      freq1 *= nyquist_fact;
      freq2 *= nyquist_fact;

      g_print ("%f %f\n", freq1, freq2);
      
      switch (filt->filter_algo << 16 | filt->filter_type)
	{
	case BSE_IIR_FILTER_BUTTERWORTH << 16 | BSE_IIR_FILTER_LOW_PASS:
	  gsl_filter_butter_lp (filt->order, freq1, filt->epsilon, filt->a, filt->b);
	  break;
	case BSE_IIR_FILTER_CHEBYCHEFF1 << 16 | BSE_IIR_FILTER_LOW_PASS:
	  gsl_filter_tscheb1_lp (filt->order, freq1, filt->epsilon, filt->a, filt->b);
	  break;
	case BSE_IIR_FILTER_CHEBYCHEFF2 << 16 | BSE_IIR_FILTER_LOW_PASS:
	  gsl_filter_tscheb2_lp (filt->order, freq1, steepness, filt->epsilon, filt->a, filt->b);
	  break;
	case BSE_IIR_FILTER_BUTTERWORTH << 16 | BSE_IIR_FILTER_HIGH_PASS:
	  gsl_filter_butter_hp (filt->order, freq1, filt->epsilon, filt->a, filt->b);
	  break;
	case BSE_IIR_FILTER_CHEBYCHEFF1 << 16 | BSE_IIR_FILTER_HIGH_PASS:
	  gsl_filter_tscheb1_hp (filt->order, freq1, filt->epsilon, filt->a, filt->b);
	  break;
	case BSE_IIR_FILTER_CHEBYCHEFF2 << 16 | BSE_IIR_FILTER_HIGH_PASS:
	  gsl_filter_tscheb2_hp (filt->order, freq1, steepness, filt->epsilon, filt->a, filt->b);
	  break;
	case BSE_IIR_FILTER_BUTTERWORTH << 16 | BSE_IIR_FILTER_BAND_PASS:
	  gsl_filter_butter_bp (filt->order & ~1, freq1, freq2, filt->epsilon, filt->a, filt->b);
	  break;
	case BSE_IIR_FILTER_CHEBYCHEFF1 << 16 | BSE_IIR_FILTER_BAND_PASS:
	  gsl_filter_tscheb1_bp (filt->order & ~1, freq1, freq2, filt->epsilon, filt->a, filt->b);
	  break;
	case BSE_IIR_FILTER_CHEBYCHEFF2 << 16 | BSE_IIR_FILTER_BAND_PASS:
	  gsl_filter_tscheb2_bp (filt->order & ~1, freq1, freq2, steepness, filt->epsilon, filt->a, filt->b);
	  break;
	case BSE_IIR_FILTER_BUTTERWORTH << 16 | BSE_IIR_FILTER_BAND_STOP:
	  gsl_filter_butter_bs (filt->order & ~1, freq1, freq2, filt->epsilon, filt->a, filt->b);
	  break;
	case BSE_IIR_FILTER_CHEBYCHEFF1 << 16 | BSE_IIR_FILTER_BAND_STOP:
	  gsl_filter_tscheb1_bs (filt->order & ~1, freq1, freq2, filt->epsilon, filt->a, filt->b);
	  break;
	case BSE_IIR_FILTER_CHEBYCHEFF2 << 16 | BSE_IIR_FILTER_BAND_STOP:
	  gsl_filter_tscheb2_bs (filt->order & ~1, freq1, freq2, steepness, filt->epsilon, filt->a, filt->b);
	  break;
	default:
	  g_assert_not_reached ();
	}
      if (1)
	g_print ("F(z)=%s/%s\n",
		 gsl_poly_str (filt->order, filt->a, "z"),
		 gsl_poly_str (filt->order, filt->b, "z"));
      
      fmod->iir.order = filt->order;
      fmod->iir.a = fmod->dummy;
      fmod->iir.b = fmod->iir.a + filt->order + 1;
      memcpy (fmod->iir.a, filt->a, sizeof (filt->a[0]) * (filt->order + 1));
      memcpy (fmod->iir.b, filt->b, sizeof (filt->b[0]) * (filt->order + 1));

      /* abusing f->w as simple flag for algo_type_change */
      fmod->iir.w = filt->algo_type_change ? fmod->dummy : NULL;
      filt->algo_type_change = FALSE;

      bse_source_access_modules (BSE_SOURCE (filt),
				 iir_filter_access, fmod, g_free,
				 NULL);
    }
}

static void
iir_filter_process (GslModule *module,
		    guint      n_values)
{
  FilterModule *fmod = module->user_data;
  const gfloat *sig_in = GSL_MODULE_IBUFFER (module, BSE_IIR_FILTER_ICHANNEL_MONO);
  gfloat *sig_out = GSL_MODULE_OBUFFER (module, BSE_IIR_FILTER_OCHANNEL_MONO);

  gsl_iir_filter_eval (&fmod->iir, sig_in, sig_out, n_values);
}

static void
bse_iir_filter_context_create (BseSource *source,
			       guint      context_handle,
			       GslTrans  *trans)
{
  static const GslClass iir_filter_class = {
    BSE_IIR_FILTER_N_ICHANNELS,	/* n_istreams */
    0,				/* n_jstreams */
    BSE_IIR_FILTER_N_OCHANNELS,	/* n_ostreams */
    iir_filter_process,		/* process */
    (gpointer) g_free,		/* free */
    GSL_COST_NORMAL,		/* flags */
  };
  BseIIRFilter *filt = BSE_IIR_FILTER (source);
  FilterModule *fmod = g_new0 (FilterModule, 1);
  GslModule *module;

  gsl_iir_filter_setup (&fmod->iir, filt->order, filt->a, filt->b, fmod->dummy);

  module = gsl_module_new (&iir_filter_class, fmod);

  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);

  /* commit module to engine */
  gsl_trans_add (trans, gsl_job_integrate (module));

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}


/* --- Export to BSE --- */
#include "./icons/filter.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_iir_filter, "BseIIRFilter", "BseSource",
    "BseIIRFilter is an infinite impulse response filter of variable order",
    &type_info_iir_filter,
    "/Modules/IIR Filter",
    { FILTER_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      FILTER_IMAGE_WIDTH, FILTER_IMAGE_HEIGHT,
      FILTER_IMAGE_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORT_AND_GENERATE_ENUMS ();
BSE_EXPORTS_END;
