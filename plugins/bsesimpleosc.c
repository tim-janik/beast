/* BseSimpleOsc - BSE Simple Oscillator
 * Copyright (C) 1999,2000-2001 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library Simpleeral Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU Simpleeral Public License for more details.
 *
 * You should have received a copy of the GNU Library Simpleeral Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bsesimpleosc.h"

#include <bse/gslengine.h>

/* include generated enums
 */
#include "bsesimpleosc.enums"


#define	FRAC_N_BITS	(19)
#define	FRAC_BIT_MASK	((1 << FRAC_N_BITS) - 1)
#define	TABLE_SIZE	(1 << (32 - FRAC_N_BITS))


/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_WAVE_FORM,
  PARAM_SINE,
  PARAM_GSAW,
  PARAM_SSAW,
  PARAM_PULSE,
  PARAM_TRIANGLE,
  PARAM_PHASE,
  PARAM_BASE_FREQ,
  PARAM_FM_PERC,
  PARAM_BASE_NOTE
};


/* --- prototypes --- */
static void	bse_simple_osc_init		(BseSimpleOsc		*simple_osc);
static void	bse_simple_osc_class_init	(BseSimpleOscClass	*class);
static void	bse_simple_osc_class_finalize	(BseSimpleOscClass	*class);
static void	bse_simple_osc_do_destroy	(BseObject		*object);
static void	bse_simple_osc_set_property	(BseSimpleOsc		*simple_osc,
						 guint		         param_id,
						 GValue		        *value,
						 GParamSpec		*pspec);
static void	bse_simple_osc_get_property	(BseSimpleOsc		*simple_osc,
						 guint			 param_id,
						 GValue			*value,
						 GParamSpec		*pspec);
static void	bse_simple_osc_prepare		(BseSource		*source);
static void	bse_simple_osc_context_create	(BseSource		*source,
						 guint			 context_handle,
						 GslTrans		*trans);
static void	bse_simple_osc_reset		(BseSource		*source);
static void	bse_simple_osc_update_modules	(BseSimpleOsc		*simple_osc,
						 GslTrans		*trans);


/* --- variables --- */
static GType		 type_id_simple_osc = 0;
static gpointer		 parent_class = NULL;
static const GTypeInfo type_info_simple_osc = {
  sizeof (BseSimpleOscClass),
  
  (GBaseInitFunc) NULL,
  (GBaseFinalizeFunc) NULL,
  (GClassInitFunc) bse_simple_osc_class_init,
  (GClassFinalizeFunc) bse_simple_osc_class_finalize,
  NULL /* class_data */,
  
  sizeof (BseSimpleOsc),
  0 /* n_preallocs */,
  (GInstanceInitFunc) bse_simple_osc_init,
};


/* --- functions --- */
static void
bse_simple_osc_class_init (BseSimpleOscClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ochannel, ichannel;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = (GObjectSetPropertyFunc) bse_simple_osc_set_property;
  gobject_class->get_property = (GObjectGetPropertyFunc) bse_simple_osc_get_property;
  
  object_class->destroy = bse_simple_osc_do_destroy;
  
  source_class->prepare = bse_simple_osc_prepare;
  source_class->context_create = bse_simple_osc_context_create;
  source_class->reset = bse_simple_osc_reset;
  
  class->ref_count = 0;
  class->sine_table_size = 0;
  class->sine_table = NULL;
  class->gsaw_table_size = 0;
  class->gsaw_table = NULL;
  class->ssaw_table_size = 0;
  class->ssaw_table = NULL;
  class->pulse_table_size = 0;
  class->pulse_table = NULL;
  class->triangle_table_size = 0;
  class->triangle_table = NULL;
  
  bse_object_class_add_param (object_class, "Wave Form",
			      PARAM_WAVE_FORM,
			      bse_param_spec_enum ("wave_form", "Wave", "Oscillator wave form",
						   BSE_TYPE_SIMPLE_OSC_WAVE_TYPE,
						   BSE_SIMPLE_OSC_SINE,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Wave Form",
			      PARAM_PHASE,
			      bse_param_spec_float ("phase", "Phase", NULL,
						    -180.0, 180.0,
						    0.0, 5.0,
						    BSE_PARAM_DEFAULT |
						    BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Wave Form",
			      PARAM_SINE,
			      bse_param_spec_bool ("sine_table", "Sine Wave", NULL,
						   TRUE,
						   BSE_PARAM_GUI | BSE_PARAM_HINT_RADIO));
  bse_object_class_add_param (object_class, "Wave Form",
			      PARAM_PULSE,
			      bse_param_spec_bool ("pulse_table", "Pulse", NULL,
						   FALSE,
						   BSE_PARAM_GUI | BSE_PARAM_HINT_RADIO));
  bse_object_class_add_param (object_class, "Wave Form",
			      PARAM_GSAW,
			      bse_param_spec_bool ("gsaw_table", "Growing Saw", NULL,
						   FALSE,
						   BSE_PARAM_GUI | BSE_PARAM_HINT_RADIO));
  bse_object_class_add_param (object_class, "Wave Form",
			      PARAM_SSAW,
			      bse_param_spec_bool ("ssaw_table", "Shrinking Saw", NULL,
						   FALSE,
						   BSE_PARAM_GUI | BSE_PARAM_HINT_RADIO));
  bse_object_class_add_param (object_class, "Wave Form",
			      PARAM_TRIANGLE,
			      bse_param_spec_bool ("triangle_table", "Triangle", NULL,
						   FALSE,
						   BSE_PARAM_GUI | BSE_PARAM_HINT_RADIO));
  bse_object_class_add_param (object_class, "Base Frequency",
			      PARAM_BASE_FREQ,
			      bse_param_spec_float ("base_freq", "Frequency", NULL,
						    BSE_MIN_OSC_FREQ_d, BSE_MAX_OSC_FREQ_d,
						    BSE_KAMMER_FREQ, 10.0,
						    BSE_PARAM_DEFAULT |
						    BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Base Frequency",
			      PARAM_BASE_NOTE,
			      bse_param_spec_note ("base_note", "Note", NULL,
						   BSE_MIN_NOTE, BSE_MAX_NOTE,
						   BSE_KAMMER_NOTE, 1, TRUE,
						   BSE_PARAM_GUI));
  bse_object_class_add_param (object_class, "Modulation",
			      PARAM_FM_PERC,
			      bse_param_spec_float ("fm_perc", "Input Modulation [%]", "Modulation Strength for input channel",
						    0, 100.0,
						    10.0, 5.0,
						    BSE_PARAM_DEFAULT |
						    BSE_PARAM_HINT_SCALE));
  
  ochannel = bse_source_class_add_ochannel (source_class, "osc_out", "Oscillated Output");
  g_assert (ochannel == BSE_SIMPLE_OSC_OCHANNEL_OSC);
  ochannel = bse_source_class_add_ochannel (source_class, "sync_out", "Syncronization Output");
  g_assert (ochannel == BSE_SIMPLE_OSC_OCHANNEL_SYNC);
  ichannel = bse_source_class_add_ichannel (source_class, "freq_in", "Oscillating Frequency Input");
  g_assert (ichannel == BSE_SIMPLE_OSC_ICHANNEL_FREQ);
  ichannel = bse_source_class_add_ichannel (source_class, "freq_mod_in", "Frequency Modulation Input");
  g_assert (ichannel == BSE_SIMPLE_OSC_ICHANNEL_FREQ_MOD);
  ichannel = bse_source_class_add_ichannel (source_class, "sync_in", "Syncronization Input");
  g_assert (ichannel == BSE_SIMPLE_OSC_ICHANNEL_SYNC);
}

static void
bse_simple_osc_class_finalize (BseSimpleOscClass *class)
{
}

static void
bse_simple_osc_class_ref_tables (BseSimpleOsc *simple_osc)
{
  BseSimpleOscClass *class = BSE_SIMPLE_OSC_GET_CLASS (simple_osc);
  
  if (class->ref_count == 0)
    {
      gdouble table_size;
      BseSampleValue *table;
      guint i;
      
      /* sine */
      class->sine_table_size = TABLE_SIZE;
      table_size = class->sine_table_size;
      table = g_new (BseSampleValue, table_size + 1);
      for (i = 0; i < table_size; i += 1)
	{
	  gdouble d = i;
	  
	  table[i] = sin (2 * PI * (d / table_size));
	}
      table[(guint) table_size] = table[0];
      class->sine_table = table;
      
      /* gsaw */
      class->gsaw_table_size = TABLE_SIZE;
      table_size = class->gsaw_table_size;
      table = g_new (BseSampleValue, table_size + 1);
      for (i = 0; i < table_size; i++)
	{
	  gdouble d = i;
	  
	  table[i] = d * 2.0 / table_size - 1.0;
	}
      table[(guint) table_size] = table[0];
      class->gsaw_table = table;
      
      /* ssaw */
      class->ssaw_table_size = TABLE_SIZE;
      table_size = class->ssaw_table_size;
      table = g_new (BseSampleValue, table_size + 1);
      for (i = 0; i < table_size; i++)
	{
	  gdouble d = i;
	  
	  table[i] = (table_size - d) * 2.0 / table_size - 1.0;
	}
      table[(guint) table_size] = table[0];
      class->ssaw_table = table;
      
      /* pulse */
      class->pulse_table_size = TABLE_SIZE;
      table_size = class->pulse_table_size;
      table = g_new (BseSampleValue, table_size + 1);
      for (i = 0; i < table_size; i++)
	{
	  table[i] = i < table_size / 2 ? 1.0 : -1.0;
	}
      table[(guint) table_size] = table[0];
      class->pulse_table = table;
      
      /* triangle */
      class->triangle_table_size = TABLE_SIZE;
      table_size = class->triangle_table_size;
      table = g_new (BseSampleValue, table_size + 1);
      for (i = 0; i < table_size; i++)
	{
	  gdouble d = i * 2;
	  
	  if (d < table_size)
	    table[i] = d * 2.0 / table_size - 1.0;
	  else
	    {
	      d -= table_size;
	      table[i] = (table_size - d) * 2.0 / table_size - 1.0;
	    }
	}
      table[(guint) table_size] = table[0];
      class->triangle_table = table;
    }
  class->ref_count += 1;
}

static void
bse_simple_osc_class_unref_tables (BseSimpleOsc *simple_osc)
{
  BseSimpleOscClass *class = BSE_SIMPLE_OSC_GET_CLASS (simple_osc);

  class->ref_count -= 1;
  if (!class->ref_count)
    {
      class->sine_table_size = 0;
      g_free (class->sine_table);
      class->sine_table = NULL;
      class->gsaw_table_size = 0;
      g_free (class->gsaw_table);
      class->gsaw_table = NULL;
      class->ssaw_table_size = 0;
      g_free (class->ssaw_table);
      class->ssaw_table = NULL;
      class->pulse_table_size = 0;
      g_free (class->pulse_table);
      class->pulse_table = NULL;
      class->triangle_table_size = 0;
      g_free (class->triangle_table);
      class->triangle_table = NULL;
    }
}

static void
bse_simple_osc_init (BseSimpleOsc *simple_osc)
{
  simple_osc->wave = BSE_SIMPLE_OSC_SINE;
  simple_osc->phase = 0.0;
  simple_osc->base_freq = BSE_KAMMER_FREQ;
  simple_osc->fm_perc = 10;
}

static void
bse_simple_osc_do_destroy (BseObject *object)
{
  BseSimpleOsc *simple_osc;
  
  simple_osc = BSE_SIMPLE_OSC (object);
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_simple_osc_set_property (BseSimpleOsc *simple_osc,
			     guint         param_id,
			     GValue       *value,
			     GParamSpec   *pspec)
{
  guint wave = 0;
  
  switch (param_id)
    {
    case PARAM_WAVE_FORM:
      simple_osc->wave = g_value_get_enum (value);
      bse_simple_osc_update_modules (simple_osc, NULL);
      bse_object_param_changed (BSE_OBJECT (simple_osc), "sine_table");
      bse_object_param_changed (BSE_OBJECT (simple_osc), "gsaw_table");
      bse_object_param_changed (BSE_OBJECT (simple_osc), "ssaw_table");
      bse_object_param_changed (BSE_OBJECT (simple_osc), "pulse_table");
      bse_object_param_changed (BSE_OBJECT (simple_osc), "triangle_table");
      break;
    case PARAM_TRIANGLE:
      wave++;
      /* fall through */
    case PARAM_SSAW:
      wave++;
      /* fall through */
    case PARAM_GSAW:
      wave++;
      /* fall through */
    case PARAM_PULSE:
      wave++;
      /* fall through */
    case PARAM_SINE:
      wave++;
      if (g_value_get_boolean (value))
	{
	  simple_osc->wave = wave;
	  bse_simple_osc_update_modules (simple_osc, NULL);
	  bse_object_param_changed (BSE_OBJECT (simple_osc), "wave_form");
	  bse_object_param_changed (BSE_OBJECT (simple_osc), "sine_table");
	  bse_object_param_changed (BSE_OBJECT (simple_osc), "gsaw_table");
	  bse_object_param_changed (BSE_OBJECT (simple_osc), "ssaw_table");
	  bse_object_param_changed (BSE_OBJECT (simple_osc), "pulse_table");
	  bse_object_param_changed (BSE_OBJECT (simple_osc), "triangle_table");
	}
      break;
    case PARAM_PHASE:
      simple_osc->phase = g_value_get_float (value);
      bse_simple_osc_update_modules (simple_osc, NULL);
      break;
    case PARAM_BASE_NOTE:
      simple_osc->base_freq = bse_note_to_freq (bse_value_get_note (value));
      simple_osc->base_freq = MAX (simple_osc->base_freq, BSE_MIN_OSC_FREQ_d);
      bse_simple_osc_update_modules (simple_osc, NULL);
      bse_object_param_changed (BSE_OBJECT (simple_osc), "base_freq");
      if (bse_note_from_freq (simple_osc->base_freq) != bse_value_get_note (value))
	bse_object_param_changed (BSE_OBJECT (simple_osc), "base_note");
      break;
    case PARAM_BASE_FREQ:
      simple_osc->base_freq = g_value_get_float (value);
      bse_simple_osc_update_modules (simple_osc, NULL);
      bse_object_param_changed (BSE_OBJECT (simple_osc), "base_note");
      break;
    case PARAM_FM_PERC:
      simple_osc->fm_perc = g_value_get_float (value);
      bse_simple_osc_update_modules (simple_osc, NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (simple_osc, param_id, pspec);
      break;
    }
}

static void
bse_simple_osc_get_property (BseSimpleOsc *simple_osc,
			     guint         param_id,
			     GValue       *value,
			     GParamSpec   *pspec)
{
  switch (param_id)
    {
    case PARAM_WAVE_FORM:
      g_value_set_enum (value, simple_osc->wave);
      break;
    case PARAM_SINE:
      g_value_set_boolean (value, simple_osc->wave == BSE_SIMPLE_OSC_SINE);
      break;
    case PARAM_GSAW:
      g_value_set_boolean (value, simple_osc->wave == BSE_SIMPLE_OSC_GSAW);
      break;
    case PARAM_SSAW:
      g_value_set_boolean (value, simple_osc->wave == BSE_SIMPLE_OSC_SSAW);
      break;
    case PARAM_PULSE:
      g_value_set_boolean (value, simple_osc->wave == BSE_SIMPLE_OSC_PULSE);
      break;
    case PARAM_TRIANGLE:
      g_value_set_boolean (value, simple_osc->wave == BSE_SIMPLE_OSC_TRIANGLE);
      break;
    case PARAM_BASE_NOTE:
      bse_value_set_note (value, bse_note_from_freq (simple_osc->base_freq));
      break;
    case PARAM_BASE_FREQ:
      g_value_set_float (value, simple_osc->base_freq);
      break;
    case PARAM_PHASE:
      g_value_set_float (value, simple_osc->phase);
      break;
    case PARAM_FM_PERC:
      g_value_set_float (value, simple_osc->fm_perc);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (simple_osc, param_id, pspec);
      break;
    }
}

static void
bse_simple_osc_prepare (BseSource *source)
{
  BseSimpleOsc *simple_osc = BSE_SIMPLE_OSC (source);
  
  bse_simple_osc_class_ref_tables (simple_osc);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

typedef struct
{
  BseSimpleOscVars vars;
  gfloat           last_freq;
  guint32          cur_pos;
  guint32	   pos_inc;
} SimpleOsc;

static void
bse_simple_osc_update_modules (BseSimpleOsc *simple_osc,
			       GslTrans     *trans)
{
  BseSimpleOscClass *class = BSE_SIMPLE_OSC_GET_CLASS (simple_osc);
  BseSimpleOscVars vars = { 0, };
  
  if (!BSE_SOURCE_PREPARED (simple_osc))
    return;

  switch (simple_osc->wave)
    {
    case BSE_SIMPLE_OSC_SINE:
      vars.n_table_values = class->sine_table_size;
      vars.table = class->sine_table;
      break;
    case BSE_SIMPLE_OSC_GSAW:
      vars.n_table_values = class->gsaw_table_size;
      vars.table = class->gsaw_table;
      break;
    case BSE_SIMPLE_OSC_SSAW:
      vars.n_table_values = class->ssaw_table_size;
      vars.table = class->ssaw_table;
      break;
    case BSE_SIMPLE_OSC_PULSE:
      vars.n_table_values = class->pulse_table_size;
      vars.table = class->pulse_table;
      break;
    case BSE_SIMPLE_OSC_TRIANGLE:
      vars.n_table_values = class->triangle_table_size;
      vars.table = class->triangle_table;
      break;
    default:
      g_assert_not_reached ();
    }
  
  vars.sync_pos = (simple_osc->phase + 360.0) / 360.0 * vars.n_table_values;
  while (vars.sync_pos >= vars.n_table_values)
    vars.sync_pos -= vars.n_table_values;
  vars.fm_strength = simple_osc->fm_perc / 100.0;
  vars.with_fm_mod = BSE_EPSILON_CMP (simple_osc->fm_perc, 0) != 0;
  if (!vars.with_fm_mod)
    vars.fm_strength = 0.0;

  /* update modules in all contexts with the new vars */
  bse_source_update_imodules (BSE_SOURCE (simple_osc),
			      BSE_SIMPLE_OSC_ICHANNEL_FREQ,
			      G_STRUCT_OFFSET (SimpleOsc, vars),
			      &vars,
			      sizeof (vars),
			      trans);
}

static void
simple_osc_process (GslModule *module,
		    guint      n_values)
{
  SimpleOsc *sosc = module->user_data;
  BseSimpleOscVars *vars = &sosc->vars;
  const gfloat *freq_in = module->istreams[0].connected ? GSL_MODULE_IBUFFER (module, 0) : NULL;
  gfloat *wave_out = module->ostreams[0].connected ? GSL_MODULE_OBUFFER (module, 0) : NULL;
  gfloat *wave_bound = wave_out + n_values;
  gfloat *table = vars->table;
  guint32 cur_pos = sosc->cur_pos;
  guint32 pos_inc = sosc->pos_inc;
  gfloat last_freq = sosc->last_freq;
  gfloat step_fact = (((gfloat) vars->n_table_values) *
		      ((gfloat) FRAC_N_BITS) *
		      BSE_MAX_FREQUENCY_d / BSE_MIX_FREQ_d); /* freq_in factor for pos increment */

  if (!wave_out)
    return;	/* nothing to process */
  if (!freq_in)
    {
      /* no freq input supplied, FIXME: status-0 support */
      memset (wave_out, 0, n_values * sizeof (wave_out[0]));
      return;
    }
  
  /* do the mixing */
  do
    {
      guint32 tpos, frac;
      gfloat v, ffrac, new_freq;
      
      /* pos determination, linear ipol */
      tpos = cur_pos >> FRAC_N_BITS; frac = cur_pos & FRAC_BIT_MASK;
      ffrac = frac;
      ffrac *= 1.0 / (FRAC_BIT_MASK + 1.);
      v = table[tpos] * (1.0 - ffrac) + table[tpos + 1] * ffrac;
      /* put value */
      *wave_out++ = v;

      /* get input freq, determine step width */
      new_freq = *freq_in++;
      if (new_freq != last_freq)
	{
	  guint64 l64;

	  last_freq = new_freq;
	  new_freq *= BSE_MAX_FREQUENCY_d; // FIXME
	  if (1)
	    g_print ("NEWFREQ:::::: new_val=%f freq=%f factor=%f\n",
		     last_freq, new_freq, new_freq);
	  // new_freq = CLAMP (new_freq, 0, 1.0);
	  l64 = new_freq * ((gfloat) vars->n_table_values) / BSE_MIX_FREQ_d * BSE_MAX_FREQUENCY_d * ((gfloat) (1<<FRAC_N_BITS));
	  pos_inc = l64 & 0xffffffff;
	  if (0)
	    g_print ("NEWFREQ:::::: tp:%u cp=%u inc=%u new_freq=%f max_freq=%f mix_freq=%f\n",
		   tpos, cur_pos, pos_inc, new_freq, BSE_MAX_FREQUENCY_d, BSE_MIX_FREQ_d);
	}

      /* step */
      cur_pos += pos_inc;
    }
  while (wave_out < wave_bound);

  /* preserve state */
  sosc->last_freq = last_freq;
  sosc->cur_pos = cur_pos;
  sosc->pos_inc = pos_inc;
}

static void
bse_simple_osc_context_create (BseSource *source,
			       guint      context_handle,
			       GslTrans  *trans)
{
  static const GslClass sosc_class = {
    3,				/* n_istreams */
    0,                          /* n_jstreams */
    2,				/* n_ostreams */
    simple_osc_process,		/* process */
    (GslModuleFreeFunc) g_free,	/* free */
    GSL_COST_CHEAP,		/* cost */
  };
  BseSimpleOsc *simple_osc = BSE_SIMPLE_OSC (source);
  SimpleOsc *sosc = g_new0 (SimpleOsc, 1);
  GslModule *module;

  sosc->last_freq = 0;
  sosc->cur_pos = 0;
  sosc->pos_inc = 0;
  module = gsl_module_new (&sosc_class, sosc);

  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);

  /* commit module to engine */
  gsl_trans_add (trans, gsl_job_integrate (module));

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);

  /* update module data */
  bse_simple_osc_update_modules (simple_osc, trans);
}

static void
bse_simple_osc_reset (BseSource *source)
{
  BseSimpleOsc *simple_osc = BSE_SIMPLE_OSC (source);
  
  bse_simple_osc_class_unref_tables (simple_osc);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}


/* --- Export to BSE --- */
#include "./icons/osc.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_simple_osc, "BseSimpleOsc", "BseSource",
    "SimpleOsc is a basis oscillator that supports frequency, "
    "modulation and sync inputs",
    &type_info_simple_osc,
    "/Source/Oscillators/Simple Oscillator",
    { OSC_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      OSC_IMAGE_WIDTH, OSC_IMAGE_HEIGHT,
      OSC_IMAGE_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORT_AND_GENERATE_ENUMS ();
BSE_EXPORTS_END;
