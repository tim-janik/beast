/* BseIIRFilter - BSE Infinite Impulse Response Filter
 * Copyright (C) 1999-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "bseiirfilter.h"
#include <bse/bseengine.h>
#include <bse/gslfilter.h>
#include <bse/bsecxxplugin.hh>

#include <string.h>

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
static void	   bse_iir_filter_class_init		(BseIIRFilterClass	*klass);
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
							 BseTrans		*trans);
static void	   bse_iir_filter_update_modules	(BseIIRFilter		*filt);


/* --- Export to BSE --- */
#include "./icons/filter.c"
BSE_RESIDENT_TYPE_DEF (BseIIRFilter, bse_iir_filter, N_("Filters/IIR Filter"),
                       "BseIIRFilter is an infinite impulse response filter of variable order",
                       filter_icon);

/* --- variables --- */
static gpointer	       parent_class = NULL;


/* --- functions --- */
static void
bse_iir_filter_class_init (BseIIRFilterClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (klass);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (klass);
  guint ochannel_id, ichannel_id;
  
  parent_class = g_type_class_peek_parent (klass);
  
  gobject_class->set_property = bse_iir_filter_set_property;
  gobject_class->get_property = bse_iir_filter_get_property;
  
  source_class->prepare = bse_iir_filter_prepare;
  source_class->context_create = bse_iir_filter_context_create;
  
  bse_object_class_add_param (object_class, _("Filter Choice"),
			      PARAM_FILTER_ALGO,
			      bse_param_spec_genum ("filter_algorithm", _("Filter Algorithm"), _("The filter design type"),
						    BSE_TYPE_IIR_FILTER_ALGORITHM,
						    BSE_IIR_FILTER_BUTTERWORTH,
						    SFI_PARAM_STANDARD));
  bse_object_class_add_param (object_class, _("Filter Choice"),
			      PARAM_FILTER_TYPE,
			      bse_param_spec_genum ("filter_type", _("Filter Type"), _("The type of filter to use"),
						    BSE_TYPE_IIR_FILTER_TYPE,
						    BSE_IIR_FILTER_LOW_PASS,
						    SFI_PARAM_STANDARD));
  bse_object_class_add_param (object_class, _("Filter Specification"),
			      PARAM_ORDER,
			      sfi_pspec_int ("order", _("Order"), _("Order of Filter"),
					     6, 1, BSE_IIR_FILTER_MAX_ORDER, 2,
					     SFI_PARAM_STANDARD));
  bse_object_class_add_param (object_class, _("Filter Specification"),
			      PARAM_EPSILON,
			      sfi_pspec_real ("epsilon", _("Epsilon"), _("Passband falloff at cutoff frequency"),
					      0.1, 0.0, 0.98,0.01,
					      SFI_PARAM_STANDARD));
  bse_object_class_add_param (object_class, _("Cutoff Frequency (All Filters)"),
			      PARAM_CUT_OFF_FREQ1,
			      bse_param_spec_freq ("cut_off_freq", _("Cutoff [Hz]"), _("Filter cutoff frequency"),
						   BSE_KAMMER_FREQUENCY / 2, BSE_MIN_OSC_FREQUENCY, BSE_MAX_OSC_FREQUENCY,
						   SFI_PARAM_STANDARD ":f:dial"));
  bse_object_class_add_param (object_class, _("Cutoff Frequency (All Filters)"),
			      PARAM_CUT_OFF_NOTE1,
			      bse_pspec_note ("cut_off_note", _("Note"),
                                              _("Filter cutoff frequency as note, converted to Hertz according to the current musical tuning"),
					      bse_note_from_freq (BSE_MUSICAL_TUNING_12_TET, BSE_KAMMER_FREQUENCY / 2),
					      SFI_PARAM_GUI));
  bse_object_class_add_param (object_class, _("Cutoff Frequency 2 (Band Pass/Stop)"),
			      PARAM_CUT_OFF_FREQ2,
			      bse_param_spec_freq ("cut_off_freq_2", _("Cutoff [Hz]"), _("Second filter cutoff frequency"),
						   BSE_KAMMER_FREQUENCY / 2 + FREQ_DELTA, BSE_MIN_OSC_FREQUENCY, BSE_MAX_OSC_FREQUENCY,
						   SFI_PARAM_STANDARD ":f:dial"));
  bse_object_class_add_param (object_class, _("Cutoff Frequency 2 (Band Pass/Stop)"),
			      PARAM_CUT_OFF_NOTE2,
			      bse_pspec_note ("cut_off_note_2", _("Note"),
                                              _("Filter cutoff frequency as note, converted to Hertz according to the current musical tuning"),
					      bse_note_from_freq (BSE_MUSICAL_TUNING_12_TET, BSE_KAMMER_FREQUENCY / 2 + FREQ_DELTA),
					      SFI_PARAM_GUI));
  
  ichannel_id = bse_source_class_add_ichannel (source_class, "audio-in", _("Audio In"), _("Unfiltered Input"));
  g_assert (ichannel_id == BSE_IIR_FILTER_ICHANNEL_MONO);
  ochannel_id = bse_source_class_add_ochannel (source_class, "audio-out", _("Audio Out"), _("Filtered Output"));
  g_assert (ochannel_id == BSE_IIR_FILTER_OCHANNEL_MONO);
}

static void
bse_iir_filter_init (BseIIRFilter *filt)
{
  filt->filter_algo = BSE_IIR_FILTER_BUTTERWORTH;
  filt->filter_type = BSE_IIR_FILTER_LOW_PASS;
  filt->order = 6;
  filt->epsilon = 0.1;
  filt->cut_off_freq1 = BSE_KAMMER_FREQUENCY / 2;
  filt->cut_off_freq2 = filt->cut_off_freq1 + FREQ_DELTA;
  bse_iir_filter_update_modules (filt);
}

static void
bse_iir_filter_set_property (GObject	  *object,
			     guint	   param_id,
			     const GValue *value,
			     GParamSpec	  *pspec)
{
  BseIIRFilter *self = BSE_IIR_FILTER (object);
  
  switch (param_id)
    {
    case PARAM_FILTER_ALGO:
      self->filter_algo = (BseIIRFilterAlgorithm) g_value_get_enum (value);
      self->algo_type_change = TRUE;
      bse_iir_filter_update_modules (self);
      break;
    case PARAM_FILTER_TYPE:
      self->filter_type = (BseIIRFilterType) g_value_get_enum (value);
      self->algo_type_change = TRUE;
      bse_iir_filter_update_modules (self);
      break;
    case PARAM_ORDER:
      self->order = sfi_value_get_int (value);
      bse_iir_filter_update_modules (self);
      break;
    case PARAM_EPSILON:
      self->epsilon = sfi_value_get_real (value);
      bse_iir_filter_update_modules (self);
      break;
    case PARAM_CUT_OFF_FREQ1:
      self->cut_off_freq1 = sfi_value_get_real (value);
      if (self->cut_off_freq1 + FREQ_DELTA > self->cut_off_freq2)
	{
	  self->cut_off_freq2 = self->cut_off_freq1 + FREQ_DELTA;
	  g_object_notify ((GObject*) self, "cut_off_freq_2");
	  g_object_notify ((GObject*) self, "cut_off_note_2");
	}
      bse_iir_filter_update_modules (self);
      g_object_notify ((GObject*) self, "cut_off_note");
      break;
    case PARAM_CUT_OFF_NOTE1:
      self->cut_off_freq1 = bse_note_to_freq (bse_item_current_musical_tuning (BSE_ITEM (self)), sfi_value_get_note (value));
      self->cut_off_freq1 = MAX (self->cut_off_freq1, BSE_MIN_OSC_FREQUENCY);
      if (self->cut_off_freq1 + FREQ_DELTA > self->cut_off_freq2)
	{
	  self->cut_off_freq2 = self->cut_off_freq1 + FREQ_DELTA;
	  g_object_notify ((GObject*) self, "cut_off_freq_2");
	  g_object_notify ((GObject*) self, "cut_off_note_2");
	}
      bse_iir_filter_update_modules (self);
      g_object_notify ((GObject*) self, "cut_off_freq");
      break;
    case PARAM_CUT_OFF_FREQ2:
      self->cut_off_freq2 = sfi_value_get_real (value);
      if (self->cut_off_freq1 + FREQ_DELTA > self->cut_off_freq2)
	{
	  self->cut_off_freq1 = self->cut_off_freq2 - FREQ_DELTA;
	  g_object_notify ((GObject*) self, "cut_off_freq");
	  g_object_notify ((GObject*) self, "cut_off_note");
	}
      bse_iir_filter_update_modules (self);
      g_object_notify ((GObject*) self, "cut_off_note_2");
      break;
    case PARAM_CUT_OFF_NOTE2:
      self->cut_off_freq2 = bse_note_to_freq (bse_item_current_musical_tuning (BSE_ITEM (self)), sfi_value_get_note (value));
      self->cut_off_freq2 = MAX (self->cut_off_freq2, BSE_MIN_OSC_FREQUENCY);
      if (self->cut_off_freq1 + FREQ_DELTA > self->cut_off_freq2)
	{
	  self->cut_off_freq1 = self->cut_off_freq2 - FREQ_DELTA;
	  g_object_notify ((GObject*) self, "cut_off_freq");
	  g_object_notify ((GObject*) self, "cut_off_note");
	}
      bse_iir_filter_update_modules (self);
      g_object_notify ((GObject*) self, "cut_off_freq_2");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_iir_filter_get_property (GObject	*object,
			     guint	 param_id,
			     GValue	*value,
			     GParamSpec	*pspec)
{
  BseIIRFilter *self = BSE_IIR_FILTER (object);
  
  switch (param_id)
    {
    case PARAM_FILTER_ALGO:
      g_value_set_enum (value, self->filter_algo);
      break;
    case PARAM_FILTER_TYPE:
      g_value_set_enum (value, self->filter_type);
      break;
    case PARAM_ORDER:
      sfi_value_set_int (value, self->order);
      break;
    case PARAM_EPSILON:
      sfi_value_set_real (value, self->epsilon);
      break;
    case PARAM_CUT_OFF_FREQ1:
      sfi_value_set_real (value, self->cut_off_freq1);
      break;
    case PARAM_CUT_OFF_NOTE1:
      sfi_value_set_note (value, bse_note_from_freq (bse_item_current_musical_tuning (BSE_ITEM (self)), self->cut_off_freq1));
      break;
    case PARAM_CUT_OFF_FREQ2:
      sfi_value_set_real (value, self->cut_off_freq2);
      break;
    case PARAM_CUT_OFF_NOTE2:
      sfi_value_set_note (value, bse_note_from_freq (bse_item_current_musical_tuning (BSE_ITEM (self)), self->cut_off_freq2));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_iir_filter_prepare (BseSource *source)
{
  BseIIRFilter *filt = BSE_IIR_FILTER (source);
  
  /* need to call update_modules() because we only now have bse_engine_sample_freq() */
  bse_iir_filter_update_modules (filt);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

typedef struct {
  GslIIRFilter	iir;
  gdouble	dummy[(BSE_IIR_FILTER_MAX_ORDER + 1) * 4];
} FilterModule;

static void
iir_filter_access (BseModule *module,
		   gpointer   data)
{
  FilterModule *fmod = (FilterModule*) module->user_data;
  FilterModule *src = (FilterModule*) data;

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
      gfloat nyquist_fact = PI / (0.5 * bse_engine_sample_freq ());
      gfloat freq1 = MIN (filt->cut_off_freq1, 0.5 * bse_engine_sample_freq ());
      gfloat freq2 = MIN (filt->cut_off_freq2, 0.5 * bse_engine_sample_freq ());
      gfloat steepness = 1.1;
      
      freq1 *= nyquist_fact;
      freq2 *= nyquist_fact;
      
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
iir_filter_process (BseModule *module,
		    guint      n_values)
{
  FilterModule *fmod = (FilterModule*) module->user_data;
  const gfloat *sig_in = BSE_MODULE_IBUFFER (module, BSE_IIR_FILTER_ICHANNEL_MONO);
  gfloat *sig_out = BSE_MODULE_OBUFFER (module, BSE_IIR_FILTER_OCHANNEL_MONO);
  
  gsl_iir_filter_eval (&fmod->iir, n_values, sig_in, sig_out);
}

static void
bse_iir_filter_context_create (BseSource *source,
			       guint      context_handle,
			       BseTrans  *trans)
{
  static const BseModuleClass iir_filter_class = {
    BSE_IIR_FILTER_N_ICHANNELS,	/* n_istreams */
    0,				/* n_jstreams */
    BSE_IIR_FILTER_N_OCHANNELS,	/* n_ostreams */
    iir_filter_process,		/* process */
    NULL,                       /* process_defer */
    NULL,                       /* reset */
    (BseModuleFreeFunc) g_free,	/* free */
    BSE_COST_NORMAL,		/* flags */
  };
  BseIIRFilter *filt = BSE_IIR_FILTER (source);
  FilterModule *fmod = g_new0 (FilterModule, 1);
  BseModule *module;
  
  gsl_iir_filter_setup (&fmod->iir, filt->order, filt->a, filt->b, fmod->dummy);
  
  module = bse_module_new (&iir_filter_class, fmod);
  
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);
  
  /* commit module to engine */
  bse_trans_add (trans, bse_job_integrate (module));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}
