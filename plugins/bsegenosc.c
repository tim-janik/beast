/* BseGenOsc - BSE Generic Oscillator
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
#include "bsegenosc.h"

#include <bse/gslengine.h>

/* include generated enums
 */
#include "bsegenosc.enums"


#define FRAC_N_BITS     (19)
#define FRAC_BIT_MASK   ((1 << FRAC_N_BITS) - 1)
#define TABLE_SIZE      (1 << (32 - FRAC_N_BITS))


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
  PARAM_BASE_NOTE,
  PARAM_SELF_MODULATION,
  PARAM_SELF_PERC
};


/* --- prototypes --- */
static void	bse_gen_osc_init		(BseGenOsc	*gen_osc);
static void	bse_gen_osc_class_init		(BseGenOscClass	*class);
static void	bse_gen_osc_class_finalize	(BseGenOscClass	*class);
static void	bse_gen_osc_do_destroy		(BseObject	*object);
static void	bse_gen_osc_set_property	(GObject	*object,
						 guint           param_id,
						 const GValue   *value,
						 GParamSpec     *pspec);
static void	bse_gen_osc_get_property	(GObject        *object,
						 guint           param_id,
						 GValue         *value,
						 GParamSpec     *pspec);
static void	bse_gen_osc_prepare		(BseSource	*source);
static void	bse_gen_osc_context_create	(BseSource	*source,
						 guint		 context_handle,
						 GslTrans	*trans);
static void	bse_gen_osc_reset		(BseSource	*source);
static void	bse_gen_osc_update_vars		(BseGenOsc	*gosc);
static void	bse_gen_osc_update_modules	(BseGenOsc	*gen_osc);


/* --- variables --- */
static GType		 type_id_gen_osc = 0;
static gpointer		 parent_class = NULL;
static const GTypeInfo type_info_gen_osc = {
  sizeof (BseGenOscClass),
  
  (GBaseInitFunc) NULL,
  (GBaseFinalizeFunc) NULL,
  (GClassInitFunc) bse_gen_osc_class_init,
  (GClassFinalizeFunc) bse_gen_osc_class_finalize,
  NULL /* class_data */,
  
  sizeof (BseGenOsc),
  0 /* n_preallocs */,
  (GInstanceInitFunc) bse_gen_osc_init,
};


/* --- functions --- */
static void
bse_gen_osc_class_init (BseGenOscClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ochannel, ichannel;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_gen_osc_set_property;
  gobject_class->get_property = bse_gen_osc_get_property;
  
  object_class->destroy = bse_gen_osc_do_destroy;
  
  source_class->prepare = bse_gen_osc_prepare;
  source_class->context_create = bse_gen_osc_context_create;
  source_class->reset = bse_gen_osc_reset;
  
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
						   BSE_TYPE_GEN_OSC_WAVE_TYPE,
						   BSE_GEN_OSC_SINE,
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
  bse_object_class_set_param_log_scale (object_class, "base_freq", 880.0, 2, 4);
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
  bse_object_class_add_param (object_class, "Modulation",
			      PARAM_SELF_MODULATION,
			      bse_param_spec_bool ("self_modulation", "Self Modulation", "Modulate oscillator with itself",
						   FALSE,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Modulation",
			      PARAM_SELF_PERC,
			      bse_param_spec_float ("self_perc", "Self Modulation [%]", "Modualtion Strength for self modulation",
						    0, 100.0,
						    10.0, 5.0,
						    BSE_PARAM_DEFAULT |
						    BSE_PARAM_HINT_SCALE));
  
  ochannel = bse_source_class_add_ochannel (source_class, "osc_out", "Oscillated Output");
  g_assert (ochannel == BSE_GEN_OSC_OCHANNEL_OSC);
  ochannel = bse_source_class_add_ochannel (source_class, "sync_out", "Syncronization Output");
  g_assert (ochannel == BSE_GEN_OSC_OCHANNEL_SYNC);
  ichannel = bse_source_class_add_ichannel (source_class, "freq_mod_in", "Frequency Modulation Input");
  g_assert (ichannel == BSE_GEN_OSC_ICHANNEL_FREQ_MOD);
  ichannel = bse_source_class_add_ichannel (source_class, "sync_in", "Syncronization Input");
  g_assert (ichannel == BSE_GEN_OSC_ICHANNEL_SYNC);
}

static void
bse_gen_osc_class_finalize (BseGenOscClass *class)
{
}

static void
bse_gen_osc_class_ref_tables (BseGenOsc *gen_osc)
{
  BseGenOscClass *class = BSE_GEN_OSC_GET_CLASS (gen_osc);
  
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
bse_gen_osc_class_unref_tables (BseGenOsc *gen_osc)
{
  BseGenOscClass *class = BSE_GEN_OSC_GET_CLASS (gen_osc);

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
bse_gen_osc_init (BseGenOsc *gosc)
{
  gosc->wave = BSE_GEN_OSC_SINE;
  gosc->phase = 0.0;
  gosc->base_freq = BSE_KAMMER_FREQ;
  gosc->fm_perc = 10;
  gosc->self_modulation = FALSE;
  gosc->self_perc = 10;
  /* gosc->vars = 0; */
}

static void
bse_gen_osc_do_destroy (BseObject *object)
{
  BseGenOsc *gen_osc;
  
  gen_osc = BSE_GEN_OSC (object);
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_gen_osc_set_property (GObject      *object,
			  guint         param_id,
			  const GValue *value,
			  GParamSpec   *pspec)
{
  BseGenOsc *self = BSE_GEN_OSC (object);
  guint wave = 0;
  
  switch (param_id)
    {
    case PARAM_WAVE_FORM:
      self->wave = g_value_get_enum (value);
      bse_gen_osc_update_vars (self);
      bse_object_param_changed (BSE_OBJECT (self), "sine_table");
      bse_object_param_changed (BSE_OBJECT (self), "gsaw_table");
      bse_object_param_changed (BSE_OBJECT (self), "ssaw_table");
      bse_object_param_changed (BSE_OBJECT (self), "pulse_table");
      bse_object_param_changed (BSE_OBJECT (self), "triangle_table");
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
	  self->wave = wave;
	  bse_gen_osc_update_vars (self);
	  bse_object_param_changed (BSE_OBJECT (self), "wave_form");
	  bse_object_param_changed (BSE_OBJECT (self), "sine_table");
	  bse_object_param_changed (BSE_OBJECT (self), "gsaw_table");
	  bse_object_param_changed (BSE_OBJECT (self), "ssaw_table");
	  bse_object_param_changed (BSE_OBJECT (self), "pulse_table");
	  bse_object_param_changed (BSE_OBJECT (self), "triangle_table");
	}
      break;
    case PARAM_PHASE:
      self->phase = g_value_get_float (value);
      bse_gen_osc_update_vars (self);
      break;
    case PARAM_BASE_NOTE:
      self->base_freq = bse_note_to_freq (bse_value_get_note (value));
      self->base_freq = MAX (self->base_freq, BSE_MIN_OSC_FREQ_d);
      bse_gen_osc_update_vars (self);
      bse_object_param_changed (BSE_OBJECT (self), "base_freq");
      if (bse_note_from_freq (self->base_freq) != bse_value_get_note (value))
	bse_object_param_changed (BSE_OBJECT (self), "base_note");
      break;
    case PARAM_BASE_FREQ:
      self->base_freq = g_value_get_float (value);
      bse_gen_osc_update_vars (self);
      bse_object_param_changed (BSE_OBJECT (self), "base_note");
      break;
    case PARAM_FM_PERC:
      self->fm_perc = g_value_get_float (value);
      bse_gen_osc_update_vars (self);
      break;
    case PARAM_SELF_MODULATION:
      self->self_modulation = g_value_get_boolean (value);
      bse_gen_osc_update_vars (self);
      break;
    case PARAM_SELF_PERC:
      self->self_perc = g_value_get_float (value);
      bse_gen_osc_update_vars (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_gen_osc_get_property (GObject    *object,
			  guint       param_id,
			  GValue     *value,
			  GParamSpec *pspec)
{
  BseGenOsc *self = BSE_GEN_OSC (object);

  switch (param_id)
    {
    case PARAM_WAVE_FORM:
      g_value_set_enum (value, self->wave);
      break;
    case PARAM_SINE:
      g_value_set_boolean (value, self->wave == BSE_GEN_OSC_SINE);
      break;
    case PARAM_GSAW:
      g_value_set_boolean (value, self->wave == BSE_GEN_OSC_GSAW);
      break;
    case PARAM_SSAW:
      g_value_set_boolean (value, self->wave == BSE_GEN_OSC_SSAW);
      break;
    case PARAM_PULSE:
      g_value_set_boolean (value, self->wave == BSE_GEN_OSC_PULSE);
      break;
    case PARAM_TRIANGLE:
      g_value_set_boolean (value, self->wave == BSE_GEN_OSC_TRIANGLE);
      break;
    case PARAM_BASE_NOTE:
      bse_value_set_note (value, bse_note_from_freq (self->base_freq));
      break;
    case PARAM_BASE_FREQ:
      g_value_set_float (value, self->base_freq);
      break;
    case PARAM_PHASE:
      g_value_set_float (value, self->phase);
      break;
    case PARAM_FM_PERC:
      g_value_set_float (value, self->fm_perc);
      break;
    case PARAM_SELF_MODULATION:
      g_value_set_boolean (value, self->self_modulation);
      break;
    case PARAM_SELF_PERC:
      g_value_set_float (value, self->self_perc);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_gen_osc_update_vars (BseGenOsc *gosc)
{
  BseGenOscClass *class = BSE_GEN_OSC_GET_CLASS (gosc);
  gdouble d;
  
  switch (BSE_SOURCE_PREPARED (gosc) ? gosc->wave : BSE_GEN_OSC_NOWAVE)
    {
    case BSE_GEN_OSC_SINE:
      gosc->vars.table = class->sine_table;
      break;
    case BSE_GEN_OSC_GSAW:
      gosc->vars.table = class->gsaw_table;
      break;
    case BSE_GEN_OSC_SSAW:
      gosc->vars.table = class->ssaw_table;
      break;
    case BSE_GEN_OSC_PULSE:
      gosc->vars.table = class->pulse_table;
      break;
    case BSE_GEN_OSC_TRIANGLE:
      gosc->vars.table = class->triangle_table;
      break;
    default:
      gosc->vars.table = NULL;
      break;
    }
  
  gosc->vars.sync_pos = (gosc->phase + 360.0) / 360.0 * TABLE_SIZE * ((double) FRAC_BIT_MASK);
  d = TABLE_SIZE;
  d /= BSE_MIX_FREQ_f;
  d *= gosc->base_freq;
  gosc->vars.pos_inc = d * ((double) FRAC_BIT_MASK);
  gosc->vars.fm_strength = gosc->vars.pos_inc * (gosc->fm_perc / 100.0);
  gosc->vars.with_fm_mod = BSE_EPSILON_CMP (gosc->fm_perc, 0) != 0;
  gosc->vars.self_strength = gosc->vars.pos_inc * (gosc->self_perc / 100.0);
  gosc->vars.with_self_mod = gosc->self_modulation && BSE_EPSILON_CMP (gosc->self_perc, 0) != 0;
  if (!gosc->vars.with_self_mod)
    gosc->vars.self_strength = 0.0;
  if (!gosc->vars.with_fm_mod)
    gosc->vars.fm_strength = 0.0;

  /* update modules in all contexts with the new vars */
  bse_gen_osc_update_modules (gosc);
}

static void
bse_gen_osc_prepare (BseSource *source)
{
  BseGenOsc *gen_osc = BSE_GEN_OSC (source);
  
  bse_gen_osc_class_ref_tables (gen_osc);
  bse_gen_osc_update_vars (gen_osc);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

typedef struct
{
  BseGenOscVars vars;
  guint32       last_pos;
  guint32       cur_pos;
} GenOsc;

static void
bse_gen_osc_update_modules (BseGenOsc *gen_osc)
{
  if (BSE_SOURCE_PREPARED (gen_osc))
    bse_source_update_modules (BSE_SOURCE (gen_osc),
			       G_STRUCT_OFFSET (GenOsc, vars),
			       &gen_osc->vars,
			       sizeof (gen_osc->vars),
			       NULL);
}

static void
gen_osc_process (GslModule *module,
		 guint      n_values)
{
  GenOsc *gosc = module->user_data;
  BseGenOscVars *vars = &gosc->vars;
  const BseSampleValue *wave_in = GSL_MODULE_IBUFFER (module, BSE_GEN_OSC_ICHANNEL_FREQ_MOD);
  const BseSampleValue *sync_in = GSL_MODULE_IBUFFER (module, BSE_GEN_OSC_ICHANNEL_SYNC);
  // gboolean with_fm_mod = vars->with_fm_mod && module->istreams[0].connected;
  // gboolean with_sync = module->istreams[1].connected;
  // gboolean with_self_mod = vars->with_self_mod;
  BseSampleValue *wave_out = GSL_MODULE_OBUFFER (module, BSE_GEN_OSC_OCHANNEL_OSC);
  BseSampleValue *wave_bound = wave_out + n_values;
  BseSampleValue *sync_out = GSL_MODULE_OBUFFER (module, BSE_GEN_OSC_OCHANNEL_SYNC);
  BseSampleValue *table = vars->table;
  gfloat fm_strength = vars->fm_strength;
  gfloat self_strength = vars->self_strength;
  guint32 last_pos = gosc->last_pos;
  guint32 cur_pos = gosc->cur_pos;
  guint32 pos_inc = vars->pos_inc;
  guint32 sync_pos = vars->sync_pos;
  gfloat sync_level = 0;
  
  /* do the mixing */
  do
    {
      gfloat v, ffrac, sync = *sync_in++;
      guint32 tpos, frac;
      
      if (sync > sync_level) /* honour input sync */
	{ cur_pos = sync_pos; *sync_out++ = 1.0; }
      else /* check for output sync */
	{
	  guint is_sync = (sync_pos <= cur_pos) + (last_pos < sync_pos) + (cur_pos < last_pos);
	  *sync_out++ = is_sync >= 2 ? 1.0 : 0.0;
	}
      sync_level = sync;
      last_pos = cur_pos;

      /* pos determination, linear ipol */
      tpos = cur_pos >> FRAC_N_BITS; frac = cur_pos & FRAC_BIT_MASK;
      ffrac = frac;
      ffrac *= 1.0 / (gfloat) FRAC_BIT_MASK;
      v = table[tpos] * (1.0 - ffrac) + table[tpos + 1] * ffrac;
      *wave_out++ = v;
      /* pos inc for next value */
      cur_pos += pos_inc + fm_strength * *wave_in++ + self_strength * v;
    }
  while (wave_out < wave_bound);
  
  /* preserve current position */
  gosc->cur_pos = cur_pos;
}

static void
bse_gen_osc_context_create   (BseSource *source,
			      guint      context_handle,
			      GslTrans  *trans)
{
  static const GslClass gosc_class = {
    BSE_GEN_OSC_N_ICHANNELS,	/* n_istreams */
    0,                          /* n_jstreams */
    BSE_GEN_OSC_N_OCHANNELS,	/* n_ostreams */
    gen_osc_process,		/* process */
    (GslModuleFreeFunc) g_free,	/* free */
    GSL_COST_CHEAP,		/* cost */
  };
  BseGenOsc *gen_osc = BSE_GEN_OSC (source);
  GenOsc *gosc = g_new (GenOsc, 1);
  GslModule *module;

  gosc->cur_pos = gen_osc->vars.sync_pos;
  gosc->last_pos = gosc->cur_pos;
  gosc->vars = gen_osc->vars;
  module = gsl_module_new (&gosc_class, gosc);

  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);

  /* commit module to engine */
  gsl_trans_add (trans, gsl_job_integrate (module));

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

static void
bse_gen_osc_reset (BseSource *source)
{
  BseGenOsc *gen_osc = BSE_GEN_OSC (source);
  
  gen_osc->vars.table = NULL;
  bse_gen_osc_class_unref_tables (gen_osc);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}


/* --- Export to BSE --- */
#include "./icons/genosc.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_gen_osc, "BseGenOsc", "BseSource",
    "BseGenOsc is a generic oscillator source",
    &type_info_gen_osc,
    "/Modules/Oscillators/Generic Oscillator",
    { GEN_OSC_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      GEN_OSC_IMAGE_WIDTH, GEN_OSC_IMAGE_HEIGHT,
      GEN_OSC_IMAGE_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORT_AND_GENERATE_ENUMS ();
BSE_EXPORTS_END;
