/* BseSimpleADSR - BSE Simpl ADSR Envelope Generator
 * Copyright (C) 2001-2002 Tim Janik
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
#include "bsesimpleadsr.h"

#include <bse/gslengine.h>


#define	TIME_EPSILON	(0.00001)

/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_ATTACK_TIME,
  PARAM_DECAY_TIME,
  PARAM_SUSTAIN_LEVEL,
  PARAM_RELEASE_TIME,
  PARAM_TIME_RANGE
};


/* --- prototypes --- */
static void	bse_simple_adsr_init		(BseSimpleADSR		*senv);
static void	bse_simple_adsr_class_init	(BseSimpleADSRClass	*class);
static void	bse_simple_adsr_set_property	(GObject		*object,
						 guint			 param_id,
						 const GValue		*value,
						 GParamSpec		*pspec);
static void	bse_simple_adsr_get_property	(GObject                *object,
						 guint			 param_id,
						 GValue			*value,
						 GParamSpec		*pspec);
static void	bse_simple_adsr_context_create	(BseSource		*source,
						 guint			 context_handle,
						 GslTrans		*trans);
static void	bse_simple_adsr_update_modules	(BseSimpleADSR		*simple_adsr,
						 GslTrans		*trans);


/* --- variables --- */
static GType	       type_id_simple_adsr = 0;
static gpointer	       parent_class = NULL;
static const GTypeInfo type_info_simple_adsr = {
  sizeof (BseSimpleADSRClass),
  
  (GBaseInitFunc) NULL,
  (GBaseFinalizeFunc) NULL,
  (GClassInitFunc) bse_simple_adsr_class_init,
  (GClassFinalizeFunc) NULL,
  NULL /* class_data */,
  
  sizeof (BseSimpleADSR),
  0 /* n_preallocs */,
  (GInstanceInitFunc) bse_simple_adsr_init,
};


/* --- functions --- */
static void
bse_simple_adsr_class_init (BseSimpleADSRClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ochannel, ichannel;
  gchar *desc;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_simple_adsr_set_property;
  gobject_class->get_property = bse_simple_adsr_get_property;
  
  source_class->context_create = bse_simple_adsr_context_create;
  
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_ATTACK_TIME,
			      sfi_pspec_real ("attack_time", "Attack Time [%]", NULL,
					      10.0, 0.0, 100.0, 1.0,
					      SFI_PARAM_DEFAULT SFI_PARAM_HINT_DIAL SFI_PARAM_FLOAT));
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_DECAY_TIME,
			      sfi_pspec_real ("decay_time", "Decay Time [%]", NULL,
					      30.0, 0.0, 100.0, 1.0,
					      SFI_PARAM_DEFAULT SFI_PARAM_HINT_DIAL SFI_PARAM_FLOAT));
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_SUSTAIN_LEVEL,
			      sfi_pspec_real ("sustain_level", "Sustain Level [%]", NULL,
					      50.0, 0.0, 100.0, 1.0,
					      SFI_PARAM_DEFAULT SFI_PARAM_HINT_DIAL SFI_PARAM_FLOAT));
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_RELEASE_TIME,
			      sfi_pspec_real ("release_time", "Release Time [%]", NULL,
					      40.0, 0.0, 100.0, 1.0,
					      SFI_PARAM_DEFAULT SFI_PARAM_HINT_DIAL SFI_PARAM_FLOAT));
  desc = g_strdup_printf ("Time ranges in seconds: %.1f %.1f %.1f",
			  BSE_TIME_RANGE_SHORT_ms / 1000.0,
			  BSE_TIME_RANGE_MEDIUM_ms / 1000.0,
			  BSE_TIME_RANGE_LONG_ms / 1000.0);
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_TIME_RANGE,
			      bse_param_spec_genum ("time_range", "Time Range", desc,
						    BSE_TYPE_TIME_RANGE_TYPE,
						    BSE_TIME_RANGE_SHORT,
						    SFI_PARAM_DEFAULT));
  g_free (desc);
  
  ichannel = bse_source_class_add_ichannel (source_class, "Gate In", "Gate input (activates/deactivates envelope)");
  g_assert (ichannel == BSE_SIMPLE_ADSR_ICHANNEL_GATE);
  ichannel = bse_source_class_add_ichannel (source_class, "Retrigger In", "Retrigger input (raising edge retriggers envelope)");
  g_assert (ichannel == BSE_SIMPLE_ADSR_ICHANNEL_RETRIGGER);
  ochannel = bse_source_class_add_ochannel (source_class, "Ctrl Out", "Attack-Decay-Sustain-Release envelope output");
  g_assert (ochannel == BSE_SIMPLE_ADSR_OCHANNEL_OUT);
  ochannel = bse_source_class_add_ochannel (source_class, "Done Out", "This signal goes high after the release phase has completed");
  g_assert (ochannel == BSE_SIMPLE_ADSR_OCHANNEL_DONE);
}

static void
bse_simple_adsr_init (BseSimpleADSR *adsr)
{
  adsr->attack_time   = 0.1;
  adsr->decay_time    = 0.3;
  adsr->sustain_level = 0.5;
  adsr->release_time  = 0.4;
  adsr->time_range    = BSE_TIME_RANGE_SHORT;
}

static void
bse_simple_adsr_set_property (GObject      *object,
			      guint	    param_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
  BseSimpleADSR *self = BSE_SIMPLE_ADSR (object);
  switch (param_id)
    {
    case PARAM_ATTACK_TIME:
      self->attack_time = sfi_value_get_real (value) / 100.0;
      bse_simple_adsr_update_modules (self, NULL);
      break;
    case PARAM_DECAY_TIME:
      self->decay_time = sfi_value_get_real (value) / 100.0;
      bse_simple_adsr_update_modules (self, NULL);
      break;
    case PARAM_SUSTAIN_LEVEL:
      self->sustain_level = sfi_value_get_real (value) / 100.0;
      bse_simple_adsr_update_modules (self, NULL);
      break;
    case PARAM_RELEASE_TIME:
      self->release_time = sfi_value_get_real (value) / 100.0;
      bse_simple_adsr_update_modules (self, NULL);
      break;
    case PARAM_TIME_RANGE:
      self->time_range = g_value_get_enum (value);
      bse_simple_adsr_update_modules (self, NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
bse_simple_adsr_get_property (GObject    *object,
			      guint	  param_id,
			      GValue	 *value,
			      GParamSpec *pspec)
{
  BseSimpleADSR *self = BSE_SIMPLE_ADSR (object);
  switch (param_id)
    {
    case PARAM_ATTACK_TIME:
      sfi_value_set_real (value, self->attack_time * 100.0);
      break;
    case PARAM_DECAY_TIME:
      sfi_value_set_real (value, self->decay_time * 100.0);
      break;
    case PARAM_SUSTAIN_LEVEL:
      sfi_value_set_real (value, self->sustain_level * 100.0);
      break;
    case PARAM_RELEASE_TIME:
      sfi_value_set_real (value, self->release_time * 100.0);
      break;
    case PARAM_TIME_RANGE:
      g_value_set_enum (value, self->time_range);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

#define	BSE_MIX_VARIANT_NAME	ramp_mix_gate_inc
#define	BSE_MIX_VARIANT	(BSE_MIX_RAMP_WITH_GATE | BSE_MIX_RAMP_WITH_INC)
#include "bsesimpleadsr-aux.c"

#define	BSE_MIX_VARIANT_NAME	ramp_mix_inc
#define	BSE_MIX_VARIANT	(BSE_MIX_RAMP_WITH_INC)
#include "bsesimpleadsr-aux.c"

#define	BSE_MIX_VARIANT_NAME	ramp_mix_gate_trig_dec
#define	BSE_MIX_VARIANT	(BSE_MIX_RAMP_WITH_GATE | BSE_MIX_RAMP_WITH_TRIG | BSE_MIX_RAMP_WITH_DEC)
#include "bsesimpleadsr-aux.c"

#define	BSE_MIX_VARIANT_NAME	const_mix_gate_trig
#define	BSE_MIX_VARIANT	(BSE_MIX_RAMP_WITH_GATE | BSE_MIX_RAMP_WITH_TRIG)
#include "bsesimpleadsr-aux.c"

#define	BSE_MIX_VARIANT_NAME	ramp_mix_gate_dec
#define	BSE_MIX_VARIANT	(BSE_MIX_RAMP_WITH_GATE | BSE_MIX_RAMP_WITH_DEC)
#include "bsesimpleadsr-aux.c"

#define	BSE_MIX_VARIANT_NAME	ramp_mix_trig_dec
#define	BSE_MIX_VARIANT	(BSE_MIX_RAMP_WITH_TRIG | BSE_MIX_RAMP_WITH_DEC)
#include "bsesimpleadsr-aux.c"

#define	BSE_MIX_VARIANT_NAME	ramp_mix_invgate_dec
#define	BSE_MIX_VARIANT	(BSE_MIX_RAMP_WITH_IGATE | BSE_MIX_RAMP_WITH_DEC)
#include "bsesimpleadsr-aux.c"

#define	BSE_MIX_VARIANT_NAME	const_mix_invgate
#define	BSE_MIX_VARIANT	(BSE_MIX_RAMP_WITH_IGATE)
#include "bsesimpleadsr-aux.c"

#define	BSE_MIX_VARIANT_NAME	const_mix_trig
#define	BSE_MIX_VARIANT	(BSE_MIX_RAMP_WITH_TRIG)
#include "bsesimpleadsr-aux.c"

enum {
  ATTACK,
  DECAY,
  SUSTAIN,
  RELEASE,
  POST_RELEASE
};

typedef struct
{
  BseMixRampLinear  ramp;
  BseSimpleADSRVars vars;
  guint		    phase;
} SimpleADSR;

static void
simple_adsr_process (GslModule *module,
		     guint      n_values)
{
  SimpleADSR *env = module->user_data;
  BseSimpleADSRVars *vars = &env->vars;
  BseMixRampLinear *ramp = &env->ramp;
  const gfloat *gate_in = GSL_MODULE_IBUFFER (module, BSE_SIMPLE_ADSR_ICHANNEL_GATE);
  const gfloat *trig_in = GSL_MODULE_IBUFFER (module, BSE_SIMPLE_ADSR_ICHANNEL_RETRIGGER);
  gfloat *wave_out = GSL_MODULE_OBUFFER (module, BSE_SIMPLE_ADSR_OCHANNEL_OUT);
  gboolean have_gate = GSL_MODULE_ISTREAM (module, BSE_SIMPLE_ADSR_ICHANNEL_GATE).connected;
  guint state = 0;
  
  if (!GSL_MODULE_OSTREAM (module, BSE_SIMPLE_ADSR_OCHANNEL_OUT).connected &&
      !GSL_MODULE_OSTREAM (module, BSE_SIMPLE_ADSR_OCHANNEL_DONE).connected)
    return;	/* no output */
  
  if (env->phase == POST_RELEASE &&
      !GSL_MODULE_ISTREAM (module, BSE_SIMPLE_ADSR_ICHANNEL_GATE).connected &&
      !GSL_MODULE_ISTREAM (module, BSE_SIMPLE_ADSR_ICHANNEL_RETRIGGER).connected)
    {
      /* no trigger input possible */
      GSL_MODULE_OSTREAM (module, BSE_SIMPLE_ADSR_OCHANNEL_OUT).values = gsl_engine_const_values (0.0);
      if (env->phase == POST_RELEASE)
	GSL_MODULE_OSTREAM (module, BSE_SIMPLE_ADSR_OCHANNEL_DONE).values = gsl_engine_const_values (1.0);
      else
	GSL_MODULE_OSTREAM (module, BSE_SIMPLE_ADSR_OCHANNEL_DONE).values = gsl_engine_const_values (0.0);
      return;
    }
  
  ramp->wave_out = wave_out;
  ramp->bound = wave_out + n_values;
  do
    {
      /* since we're not always mixing with gate/trigger, adjust
       * buffer pointers here
       */
      ramp->gate_in = gate_in + (ramp->wave_out - wave_out);
      ramp->trig_in = trig_in + (ramp->wave_out - wave_out);
      switch (env->phase)
	{
	case ATTACK:
	  ramp->level_step = vars->attack_inc;
	  ramp->level_border = vars->attack_level;
	  state = (have_gate ? ramp_mix_gate_inc : ramp_mix_inc) (ramp);
	  /* update last trigger val because we mixed without it */
	  ramp->last_trigger = trig_in[ramp->wave_out - wave_out - 1];
	  switch (state)
	    {
	    case BSE_MIX_RAMP_REACHED_BORDER:	env->phase = DECAY;	break;
	    case BSE_MIX_RAMP_GATE_CHANGE:	env->phase = RELEASE;	break;
	    case BSE_MIX_RAMP_REACHED_BOUND:					break;
	    default:	g_error ("should not be reached: state: %d\n", state);
	    }
	  break;
	case DECAY:
	  ramp->level_step = vars->decay_dec;
	  ramp->level_border = vars->sustain_level;
	  state = (have_gate ? ramp_mix_gate_trig_dec : ramp_mix_trig_dec) (ramp);
	  switch (state)
	    {
	    case BSE_MIX_RAMP_REACHED_BORDER:	env->phase = SUSTAIN;		break;
	    case BSE_MIX_RAMP_GATE_CHANGE:	env->phase = RELEASE;		break;
	    case BSE_MIX_RAMP_RETRIGGER:	env->phase = ATTACK;		break;
	    case BSE_MIX_RAMP_REACHED_BOUND:					break;
	    default:	g_error ("should not be reached: state: %d\n", state);
	    }
	  break;
	case SUSTAIN:
	  if (have_gate)
	    state = const_mix_gate_trig (ramp);
	  else
	    state = BSE_MIX_RAMP_GATE_CHANGE;
	  switch (state)
	    {
	    case BSE_MIX_RAMP_GATE_CHANGE:	env->phase = RELEASE;		break;
	    case BSE_MIX_RAMP_RETRIGGER:        env->phase = ATTACK;            break;
	    case BSE_MIX_RAMP_REACHED_BOUND:					break;
	    default:	g_error ("should not be reached: state: %d\n", state);
	    }
	  break;
	case RELEASE:
	  ramp->level_step = vars->release_dec;
	  ramp->level_border = 0.0;
	  /* with gate input, mix without trigger as gate is low */
	  state = (have_gate ? ramp_mix_invgate_dec : ramp_mix_trig_dec) (ramp);
          ramp->last_trigger = 0.0;
	  switch (state)
	    {
	    case BSE_MIX_RAMP_REACHED_BORDER:	env->phase = POST_RELEASE;	break;
	    case BSE_MIX_RAMP_RETRIGGER:
	    case BSE_MIX_RAMP_GATE_CHANGE:	env->phase = ATTACK;		break;
	    case BSE_MIX_RAMP_REACHED_BOUND:					break;
	    default:	g_error ("should not be reached: state: %d\n", state);
	    }
	  break;
	case POST_RELEASE:
          /* with gate input, mix without trigger as gate is low */
	  state = (have_gate ? const_mix_invgate : const_mix_trig) (ramp);
          ramp->last_trigger = 0.0;
	  switch (state)
	    {
	    case BSE_MIX_RAMP_RETRIGGER:
	    case BSE_MIX_RAMP_GATE_CHANGE:	env->phase = ATTACK;		break;
	    case BSE_MIX_RAMP_REACHED_BOUND:					break;
	    default:	g_error ("should not be reached: state: %d\n", state);
	    }
	  break;
	}
    }
  while (state != BSE_MIX_RAMP_REACHED_BOUND);
  
  if (env->phase == POST_RELEASE)
    GSL_MODULE_OSTREAM (module, BSE_SIMPLE_ADSR_OCHANNEL_DONE).values = gsl_engine_const_values (1.0);
  else
    GSL_MODULE_OSTREAM (module, BSE_SIMPLE_ADSR_OCHANNEL_DONE).values = gsl_engine_const_values (0.0);
}

static void
bse_simple_adsr_update_modules (BseSimpleADSR *adsr,
				GslTrans      *trans)
{
  BseSimpleADSRVars vars;
  gfloat ms = bse_time_range_to_ms (adsr->time_range);
  
  ms *= gsl_engine_sample_freq () / 1000.0;
  if (adsr->attack_time < TIME_EPSILON)
    {
      vars.attack_level = adsr->sustain_level;
      vars.attack_inc = 1.0;
    }
  else
    {
      vars.attack_level = 1.0;
      vars.attack_inc = 1.0 / (ms * adsr->attack_time);
    }
  vars.sustain_level = adsr->sustain_level;
  if (adsr->decay_time < TIME_EPSILON)
    vars.decay_dec = 1.0;
  else
    vars.decay_dec = (1.0 - vars.sustain_level) / (ms * adsr->decay_time);
  if (adsr->release_time < TIME_EPSILON)
    vars.release_dec = 1.0;
  else
    vars.release_dec = vars.sustain_level / (ms * adsr->release_time);
  
  if (BSE_SOURCE_PREPARED (adsr))
    bse_source_update_modules (BSE_SOURCE (adsr),
			       G_STRUCT_OFFSET (SimpleADSR, vars),
			       &vars,
			       sizeof (vars),
			       trans);
}

static void
bse_simple_adsr_context_create (BseSource *source,
				guint      context_handle,
				GslTrans  *trans)
{
  static const GslClass env_class = {
    BSE_SIMPLE_ADSR_N_ICHANNELS,	/* n_istreams */
    0,					/* n_jstreams */
    BSE_SIMPLE_ADSR_N_OCHANNELS,	/* n_ostreams */
    simple_adsr_process,		/* process */
    NULL,                       	/* process_defer */
    NULL,                       	/* reset */
    (GslModuleFreeFunc) g_free,		/* free */
    GSL_COST_CHEAP,			/* cost */
  };
  BseSimpleADSR *simple_adsr = BSE_SIMPLE_ADSR (source);
  SimpleADSR *env = g_new0 (SimpleADSR, 1);
  GslModule *module;
  
  env->ramp.last_trigger = 0;
  env->ramp.level = 0;
  module = gsl_module_new (&env_class, env);
  
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);
  
  /* commit module to engine */
  gsl_trans_add (trans, gsl_job_integrate (module));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
  
  /* update module data */
  bse_simple_adsr_update_modules (simple_adsr, trans);
}


/* --- Export to BSE --- */
#include "./icons/adsr.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_simple_adsr, "BseSimpleADSR", "BseSource",
    "Simple ADSR envelope generator",
    &type_info_simple_adsr,
    "/Modules/Other Sources/Simple ADSR",
    { ADSR_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      ADSR_IMAGE_WIDTH, ADSR_IMAGE_HEIGHT,
      ADSR_IMAGE_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
