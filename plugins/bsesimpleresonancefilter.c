/* BseSimpleResonanceFilter - BSE IIR Resonance Filter
 * Copyright (C) 2002 Tim Janik
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
#include "bsesimpleresonancefilter.h"

#include <bse/gslengine.h>
#include <bse/gslmath.h>

#define	_(x)	(x)
#define	STABILITY_EPSILON	(0.001)


/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_C1,
  PARAM_C2,
  PARAM_RFREQ,
  PARAM_RGAIN,
  PARAM_RBWIDTH
};


/* --- prototypes --- */
static void	bse_simple_resonance_filter_init		(BseSimpleResonanceFilter	*fil);
static void	bse_simple_resonance_filter_class_init		(BseSimpleResonanceFilterClass	*class);
static void	bse_simple_resonance_filter_class_finalize	(BseSimpleResonanceFilterClass	*class);
static void	bse_simple_resonance_filter_set_property	(GObject			*object,
								 guint				 param_id,
								 const GValue			*value,
								 GParamSpec			*pspec);
static void	bse_simple_resonance_filter_get_property	(GObject			*object,
								 guint				 param_id,
								 GValue				*value,
								 GParamSpec			*pspec);
static void	bse_simple_resonance_filter_prepare		(BseSource			*source);
static void	bse_simple_resonance_filter_context_create	(BseSource			*source,
								 guint				 context_handle,
								 GslTrans			*trans);
static void	bse_simple_resonance_filter_update_modules	(BseSimpleResonanceFilter	*filt);


/* --- variables --- */
static GType	       type_id_simple_resonance_filter = 0;
static gpointer	       parent_class = NULL;
static const GTypeInfo type_info_simple_resonance_filter = {
  sizeof (BseSimpleResonanceFilterClass),
  
  (GBaseInitFunc) NULL,
  (GBaseFinalizeFunc) NULL,
  (GClassInitFunc) bse_simple_resonance_filter_class_init,
  (GClassFinalizeFunc) bse_simple_resonance_filter_class_finalize,
  NULL /* class_data */,
  
  sizeof (BseSimpleResonanceFilter),
  0 /* n_preallocs */,
  (GInstanceInitFunc) bse_simple_resonance_filter_init,
};


/* --- functions --- */
static void
bse_simple_resonance_filter_class_init (BseSimpleResonanceFilterClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ochannel_id, ichannel_id;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_simple_resonance_filter_set_property;
  gobject_class->get_property = bse_simple_resonance_filter_get_property;

  source_class->prepare = bse_simple_resonance_filter_prepare;
  source_class->context_create = bse_simple_resonance_filter_context_create;
  
  bse_object_class_add_param (object_class, _("Resonance Settings"),
			      PARAM_C1,
			      bse_param_spec_float ("coeff_1", _("Coefficient 1"), NULL,
						    0, 1.0 - STABILITY_EPSILON,
						    0.25, 0.001,
						    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, _("Resonance Settings"),
			      PARAM_C2,
			      bse_param_spec_float ("coeff_2", _("Coefficient 2"), NULL,
						    -1, +1,
						    0.1, 0.01,
						    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_SCALE));

  bse_object_class_add_param (object_class, _("Resonance Behaviour"),
			      PARAM_RFREQ,
			      bse_param_spec_float ("rfreq", _("Frequency [Hz]"), NULL,
						    -G_MAXFLOAT, G_MAXFLOAT,
						    0, 10.0,
						    (BSE_PARAM_GUI | BSE_PARAM_HINT_SCALE) & ~G_PARAM_WRITABLE));
  bse_object_class_add_param (object_class, _("Resonance Behaviour"),
			      PARAM_RGAIN,
			      bse_param_spec_float ("rgain", _("Gain"), NULL,
						    -G_MAXFLOAT, G_MAXFLOAT,
						    0, 10.0,
						    (BSE_PARAM_GUI | BSE_PARAM_HINT_SCALE) & ~G_PARAM_WRITABLE));
  bse_object_class_add_param (object_class, _("Resonance Behaviour"),
			      PARAM_RBWIDTH,
			      bse_param_spec_float ("rbwidth", _("Band Width"), _("Aproximate Bandwidth at -3dB damping"),
						    -G_MAXFLOAT, G_MAXFLOAT,
						    0, 10.0,
						    (BSE_PARAM_GUI | BSE_PARAM_HINT_SCALE) & ~G_PARAM_WRITABLE));

  ochannel_id = bse_source_class_add_ochannel (source_class, "mono_out", _("Filtered Output"));
  g_assert (ochannel_id == BSE_SIMPLE_RESONANCE_FILTER_OCHANNEL_MONO);
  ichannel_id = bse_source_class_add_ichannel (source_class, "mono_in", _("Unfiltered Input"));
  g_assert (ichannel_id == BSE_SIMPLE_RESONANCE_FILTER_ICHANNEL_MONO);
}

static void
bse_simple_resonance_filter_class_finalize (BseSimpleResonanceFilterClass *class)
{
}

static void
bse_simple_resonance_filter_init (BseSimpleResonanceFilter *filt)
{
  filt->c1 = 0.25;
  filt->c2 = 0.1;
}

static void
bse_simple_resonance_filter_set_property (GObject      *object,
					  guint	        param_id,
					  const GValue *value,
					  GParamSpec   *pspec)
{
  BseSimpleResonanceFilter *filt = BSE_SIMPLE_RESONANCE_FILTER (object);
  
  switch (param_id)
    {
    case PARAM_C1:
      filt->c1 = g_value_get_float (value);
      bse_simple_resonance_filter_update_modules (filt);
      g_object_notify (filt, "rfreq");
      g_object_notify (filt, "rgain");
      g_object_notify (filt, "rbwidth");
      break;
    case PARAM_C2:
      filt->c2 = g_value_get_float (value);
      bse_simple_resonance_filter_update_modules (filt);
      g_object_notify (filt, "rfreq");
      g_object_notify (filt, "rgain");
      g_object_notify (filt, "rbwidth");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (filt, param_id, pspec);
      break;
    }
}

static void
bse_simple_resonance_filter_get_property (GObject    *object,
					  guint	      param_id,
					  GValue     *value,
					  GParamSpec *pspec)
{
  BseSimpleResonanceFilter *filt = BSE_SIMPLE_RESONANCE_FILTER (object);
  gdouble r_p = filt->c1, phi_p = acos (filt->c2);

  switch (param_id)
    {
    case PARAM_C1:
      g_value_set_float (value, filt->c1);
      break;
    case PARAM_C2:
      g_value_set_float (value, filt->c2);
      break;
    case PARAM_RFREQ:
      g_value_set_float (value, gsl_engine_sample_freq () * (0.5 / GSL_PI) * phi_p);
      break;
    case PARAM_RGAIN:
      g_value_set_float (value, 1. / ((1 - r_p) * sqrt (1 + 2 * r_p * cos (2 * phi_p) + r_p * r_p)));
      break;
    case PARAM_RBWIDTH:
      g_value_set_float (value, 2 * (1 - r_p) * gsl_engine_sample_freq () / (2. * GSL_PI));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (filt, param_id, pspec);
      break;
    }
}

static void
bse_simple_resonance_filter_prepare (BseSource *source)
{
  BseSimpleResonanceFilter *filt = BSE_SIMPLE_RESONANCE_FILTER (source);

  /* need to call update_modules() because we only now have gsl_engine_sample_freq() */
  bse_simple_resonance_filter_update_modules (filt);

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

typedef struct {
  gdouble	c1, c2;
  gdouble	s1, s2;	/* filter state */
} FilterModule;

static void
simple_resonance_filter_access (GslModule *module,
				gpointer   data)
{
  FilterModule *fmod = module->user_data;
  FilterModule *src = data;

  fmod->c1 = src->c1;
  fmod->c2 = src->c2;
}

static void
bse_simple_resonance_filter_update_modules (BseSimpleResonanceFilter *filt)
{
  if (BSE_SOURCE_PREPARED (filt))
    {
      FilterModule *fmod = g_new0 (FilterModule, 1);

      fmod->c1 = filt->c1;
      fmod->c2 = filt->c2;

      bse_source_access_modules (BSE_SOURCE (filt),
				 simple_resonance_filter_access, fmod, g_free,
				 NULL);
    }
}

static inline gdouble
simple_resonance_filter_step (FilterModule *fmod,
			      gdouble       x)
{
#if 1
  gdouble a1, a2, a3, a4, a5;

  a2 = x + fmod->s2 * fmod->c1;
  a3 = fmod->s1 * -fmod->c1;
  a5 = (a2 - a3) * fmod->c2;
  a1 = a5 + a2;
  a4 = a3 + a5;
  fmod->s1 = a1;
  fmod->s2 = a4;
  return a1;
#else
  gdouble k1, k2, k3, k4, k5, t1;

  k3 = fmod->s1 * -fmod->c1;
  k2 = x + fmod->c1 * fmod->s2;
  k5 = k2 - k3;
  t1 = k5 * fmod->c2;
  k4 = k3 + t1;
  k1 = t1 + k2;
  fmod->s2 = k4;
  fmod->s1 = k1;

  return k1;
#endif
}

static void
simple_resonance_filter_process (GslModule *module,
				 guint      n_values)
{
  FilterModule *fmod = module->user_data;
  const gfloat *sig_in = GSL_MODULE_IBUFFER (module, BSE_SIMPLE_RESONANCE_FILTER_ICHANNEL_MONO);
  gfloat *sig_out = GSL_MODULE_OBUFFER (module, BSE_SIMPLE_RESONANCE_FILTER_OCHANNEL_MONO);
  const gfloat *bound = sig_in + n_values;

  while (sig_in < bound)
    {
      *sig_out = simple_resonance_filter_step (fmod, *sig_in);
      sig_in++;
      sig_out++;
    }
}

static void
bse_simple_resonance_filter_context_create (BseSource *source,
					    guint      context_handle,
					    GslTrans  *trans)
{
  static const GslClass simple_resonance_filter_class = {
    BSE_SIMPLE_RESONANCE_FILTER_N_ICHANNELS,	/* n_istreams */
    0,						/* n_jstreams */
    BSE_SIMPLE_RESONANCE_FILTER_N_OCHANNELS,	/* n_ostreams */
    simple_resonance_filter_process,		/* process */
    (gpointer) g_free,				/* free */
    GSL_COST_NORMAL,				/* flags */
  };
  BseSimpleResonanceFilter *filt = BSE_SIMPLE_RESONANCE_FILTER (source);
  FilterModule *fmod = g_new0 (FilterModule, 1);
  GslModule *module;

  fmod->c1 = filt->c1;
  fmod->c2 = filt->c2;
  fmod->s1 = 0.0;
  fmod->s2 = 0.0;

  module = gsl_module_new (&simple_resonance_filter_class, fmod);

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
  { &type_id_simple_resonance_filter, "BseSimpleResonanceFilter", "BseSource",
    "BseSimpleResonanceFilter is an infinite impulse response filter with variable resonance",
    &type_info_simple_resonance_filter,
    "/Modules/Filters/Simple Resonance Filter",
    { FILTER_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      FILTER_IMAGE_WIDTH, FILTER_IMAGE_HEIGHT,
      FILTER_IMAGE_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
