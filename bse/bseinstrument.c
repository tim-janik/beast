/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997, 1998, 1999, 2000 Olaf Hoehmann and Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include	"bseinstrument.h"

#include	"bsesample.h"
#include	"bseglobals.h"


enum {
  PARAM_0,
  PARAM_SYNTH,
  PARAM_SAMPLE,
  PARAM_INTERPOLATION,
  PARAM_POLYPHONY,
  PARAM_VOLUME_f,
  PARAM_VOLUME_dB,
  PARAM_VOLUME_PERC,
  PARAM_BALANCE,
  PARAM_TRANSPOSE,
  PARAM_FINE_TUNE,
  PARAM_ENVELOPE,
  PARAM_ATTACK_LEVEL,
  PARAM_SUSTAIN_LEVEL,
  PARAM_RELEASE_LEVEL,
  PARAM_DELAY_TIME,
  PARAM_ATTACK_TIME,
  PARAM_DECAY_TIME,
  PARAM_SUSTAIN_TIME,
  PARAM_RELEASE_TIME,
  PARAM_DURATION
};

/* --- envelope --- */
/*	       .
 *	      / \._____.
 *	 ._./		\.
 * dots: 0 1   2 3     4 5
 */
#define	ENV_N_DOTS		(6)
#define	ENV_ATTACK_LEVEL(dots)	(dots[2].y)
#define	ENV_SUSTAIN_LEVEL(dots)	(dots[3].y)
#define	ENV_RELEASE_LEVEL(dots)	(dots[4].y)
#define	ENV_DELAY_TIME(dots)	(dots[1].x)
#define	ENV_ATTACK_TIME(dots)	(dots[2].x)
#define	ENV_DECAY_TIME(dots)	(dots[3].x)
#define	ENV_SUSTAIN_TIME(dots)	(dots[4].x)
#define	ENV_RELEASE_TIME(dots)	(dots[5].x)
static BseDot env_dflt_dots[ENV_N_DOTS] = {
  /* x (time)	y (level) */
  { 0.0,	0.0 }	/* 0) 0,		0	      */,
  { 0.0,	0.0 }	/* 1) delay_time,	0	      */,
  { 0.2,	1.0 }	/* 2) attack_time,	attack_level  */,
  { 0.3,	0.6 }	/* 3) decay_time,	sustain_level */,
  { 0.9,	0.3 }	/* 4) sustain_time,	release_level */,
  { 1.0,	0.0 }	/* 5) release_time,	0	      */,
};

#define	ENVELOPE_TIME_TOTAL(env)	 \
    (env.delay_time + env.attack_time + \
     env.decay_time + env.sustain_time + \
     env.release_time)


/* --- prototypes --- */
static void	bse_instrument_class_init	(BseInstrumentClass	*class);
static void	bse_instrument_init		(BseInstrument		*instrument);
static void	bse_instrument_do_destroy	(BseObject		*object);
static void	bse_instrument_set_property	(BseInstrument		*instrument,
						 guint                   param_id,
						 GValue                 *value,
						 GParamSpec             *pspec,
						 const gchar            *trailer);
static void	bse_instrument_get_property	(BseInstrument		*instrument,
						 guint                   param_id,
						 GValue                 *value,
						 GParamSpec             *pspec,
						 const gchar            *trailer);
static void	bse_instrument_unlocked		(BseObject		*object);


/* --- variables --- */
static GTypeClass	*parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseInstrument)
{
  static const GTypeInfo instrument_info = {
    sizeof (BseInstrumentClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_instrument_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseInstrument),
    BSE_PREALLOC_N_INSTRUMENTS /* n_preallocs */,
    (GInstanceInitFunc) bse_instrument_init,
  };
  
  return bse_type_register_static (BSE_TYPE_ITEM,
				   "BseInstrument",
				   "BSE instrument type",
				   &instrument_info);
}

static void
bse_instrument_class_init (BseInstrumentClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = (GObjectSetPropertyFunc) bse_instrument_set_property;
  gobject_class->get_property = (GObjectGetPropertyFunc) bse_instrument_get_property;
  
  object_class->unlocked = bse_instrument_unlocked;
  object_class->destroy = bse_instrument_do_destroy;
  
#if 0
  bse_object_class_add_param (object_class, "Synthesis Input",
			      PARAM_SYNTH,
			      g_param_spec_object ("sinstrument", "Synth", NULL,	// FIXME
						   BSE_TYPE_SINSTRUMENT,
						   (BSE_PARAM_DEFAULT) & ~BSE_PARAM_SERVE_GUI));
#endif
  bse_object_class_add_param (object_class, "Sample Input",
			      PARAM_SAMPLE,
			      g_param_spec_object ("sample", "Sample", NULL,	// FIXME
						   BSE_TYPE_SAMPLE,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Sample Input",
			      PARAM_INTERPOLATION,
			      bse_param_spec_enum ("interpolation", "Interpolation", NULL /* FIXME */,
						   BSE_TYPE_INTERPOL_TYPE,
						   BSE_INTERPOL_CUBIC,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Sample Input",
			      PARAM_POLYPHONY,
			      bse_param_spec_bool ("polyphony", "Polyphony instrument?", NULL,
						   FALSE,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_VOLUME_f,
			      bse_param_spec_float ("volume_f", "Volume [float]", NULL,
						    0, bse_dB_to_factor (BSE_MAX_VOLUME_dB),
						    bse_dB_to_factor (BSE_DFL_INSTRUMENT_VOLUME_dB), 0.1,
						    BSE_PARAM_STORAGE));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_VOLUME_dB,
			      bse_param_spec_float ("volume_dB", "Volume [dB]", NULL,
						    BSE_MIN_VOLUME_dB, BSE_MAX_VOLUME_dB,
						    BSE_DFL_INSTRUMENT_VOLUME_dB, BSE_STP_VOLUME_dB,
						    BSE_PARAM_GUI |
						    BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_VOLUME_PERC,
			      bse_param_spec_uint ("volume_perc", "Volume [%]", NULL,
						   0, bse_dB_to_factor (BSE_MAX_VOLUME_dB) * 100,
						   bse_dB_to_factor (BSE_DFL_INSTRUMENT_VOLUME_dB) * 100, 1,
						   BSE_PARAM_GUI |
						   BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_BALANCE,
			      bse_param_spec_int ("balance", "Balance", NULL,
						  BSE_MIN_BALANCE, BSE_MAX_BALANCE,
						  BSE_DFL_INSTRUMENT_BALANCE, BSE_STP_BALANCE,
						  BSE_PARAM_DEFAULT |
						  BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_TRANSPOSE,
			      bse_param_spec_int ("transpose", "Transpose", NULL,
						  BSE_MIN_TRANSPOSE, BSE_MAX_TRANSPOSE,
						  BSE_DFL_INSTRUMENT_TRANSPOSE, BSE_STP_TRANSPOSE,
						  BSE_PARAM_DEFAULT |
						  BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_FINE_TUNE,
			      bse_param_spec_int ("fine_tune", "Fine tune", NULL,
						  BSE_MIN_FINE_TUNE, BSE_MAX_FINE_TUNE,
						  BSE_DFL_INSTRUMENT_FINE_TUNE, BSE_STP_FINE_TUNE,
						  BSE_PARAM_DEFAULT |
						  BSE_PARAM_HINT_SCALE));
  /* envelope
   */
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_ENVELOPE,
			      bse_param_spec_dots ("envelope", "Envelope", NULL,
						   ENV_N_DOTS, env_dflt_dots,
						   BSE_PARAM_GUI));
  /* envelope parameters
   */
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_DELAY_TIME,
			      bse_param_spec_uint ("delay_time", "Delay Time", NULL,
						   0, BSE_MAX_ENV_TIME,
						   ENV_DELAY_TIME (env_dflt_dots) * BSE_MAX_ENV_TIME,
						   BSE_STP_ENV_TIME,
						   BSE_PARAM_DEFAULT |
						   BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_ATTACK_TIME,
			      bse_param_spec_uint ("attack_time", "Attack Time", NULL,
						   0, BSE_MAX_ENV_TIME,
						   ENV_ATTACK_TIME (env_dflt_dots) * BSE_MAX_ENV_TIME,
						   BSE_STP_ENV_TIME,
						   BSE_PARAM_DEFAULT |
						   BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_ATTACK_LEVEL,
			      bse_param_spec_uint ("attack_level", "Attack Level", NULL,
						   0, 100,
						   ENV_ATTACK_LEVEL (env_dflt_dots) * 100, 1,
						   BSE_PARAM_DEFAULT |
						   BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_DECAY_TIME,
			      bse_param_spec_uint ("decay_time", "Decay Time", NULL,
						   0, BSE_MAX_ENV_TIME,
						   ENV_DECAY_TIME (env_dflt_dots) * BSE_MAX_ENV_TIME,
						   BSE_STP_ENV_TIME,
						   BSE_PARAM_DEFAULT |
						   BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_SUSTAIN_LEVEL,
			      bse_param_spec_uint ("sustain_level", "Sustain Level", NULL,
						   0, 100,
						   ENV_SUSTAIN_LEVEL (env_dflt_dots) * 100, 1,
						   BSE_PARAM_DEFAULT |
						   BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_SUSTAIN_TIME,
			      bse_param_spec_uint ("sustain_time", "Sustain Time", NULL,
						   0, BSE_MAX_ENV_TIME,
						   ENV_SUSTAIN_TIME (env_dflt_dots) * BSE_MAX_ENV_TIME,
						   BSE_STP_ENV_TIME,
						   BSE_PARAM_DEFAULT |
						   BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_RELEASE_LEVEL,
			      bse_param_spec_uint ("release_level", "Release Level", NULL,
						   0, 100,
						   ENV_RELEASE_LEVEL (env_dflt_dots) * 100, 1,
						   BSE_PARAM_DEFAULT |
						   BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_RELEASE_TIME,
			      bse_param_spec_uint ("release_time", "Release Time", NULL,
						   0, BSE_MAX_ENV_TIME,
						   ENV_RELEASE_TIME (env_dflt_dots) * BSE_MAX_ENV_TIME,
						   BSE_STP_ENV_TIME,
						   BSE_PARAM_DEFAULT |
						   BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_DURATION,
			      bse_param_spec_uint ("duration", "Duration [ms]", NULL,
						   0, BSE_MAX_ENV_TIME * 5,
						   BSE_MAX_ENV_TIME,
						   BSE_STP_ENV_TIME * 5,
						   BSE_PARAM_GUI |
						   BSE_PARAM_HINT_SCALE));
}

static void
bse_instrument_init (BseInstrument *instrument)
{
  instrument->type = BSE_INSTRUMENT_NONE;
  
  instrument->volume_factor = bse_dB_to_factor (BSE_DFL_INSTRUMENT_VOLUME_dB);
  instrument->balance = BSE_DFL_INSTRUMENT_BALANCE;
  instrument->transpose = BSE_DFL_INSTRUMENT_TRANSPOSE;
  instrument->fine_tune = BSE_DFL_INSTRUMENT_FINE_TUNE;
  
  instrument->env.delay_time = ENV_DELAY_TIME (env_dflt_dots) * BSE_MAX_ENV_TIME;
  instrument->env.attack_time = ENV_ATTACK_TIME (env_dflt_dots) * BSE_MAX_ENV_TIME;
  instrument->env.attack_level = ENV_ATTACK_LEVEL (env_dflt_dots);
  instrument->env.decay_time = ENV_DECAY_TIME (env_dflt_dots) * BSE_MAX_ENV_TIME;
  instrument->env.sustain_level = ENV_SUSTAIN_LEVEL (env_dflt_dots);
  instrument->env.sustain_time = ENV_SUSTAIN_TIME (env_dflt_dots) * BSE_MAX_ENV_TIME;
  instrument->env.release_level = ENV_RELEASE_LEVEL (env_dflt_dots);
  instrument->env.release_time = ENV_RELEASE_TIME (env_dflt_dots) * BSE_MAX_ENV_TIME;
}

static void
bse_instrument_do_destroy (BseObject *object)
{
  BseInstrument *instrument = BSE_INSTRUMENT (object);
  
  g_assert (instrument->type == BSE_INSTRUMENT_NONE); /* paranoid */
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_instrument_unlocked (BseObject *object)
{
  BseInstrument *instrument;
  
  instrument = BSE_INSTRUMENT (object);
  
  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->unlocked)
    BSE_OBJECT_CLASS (parent_class)->unlocked (object);
}

static void
instrument_input_changed (BseInstrument *instrument)
{
  if (instrument->type == BSE_INSTRUMENT_SYNTH)
    ; // bse_object_param_changed (BSE_OBJECT (instrument), "sinstrument");
  else if (instrument->type == BSE_INSTRUMENT_SAMPLE)
    bse_object_param_changed (BSE_OBJECT (instrument), "sample");
  else
    g_assert_not_reached ();
}

static void
unset_input_cb (BseItem *owner,
		BseItem *input,
		gpointer data)
{
  BseInstrument *instrument = BSE_INSTRUMENT (owner);
  
  g_object_disconnect (input,
		       "any_signal", instrument_input_changed, instrument,
		       NULL);
  instrument_input_changed (instrument);
  instrument->input = NULL;
  instrument->type = BSE_INSTRUMENT_NONE;
#if 0
  if (BSE_IS_SINSTRUMENT (input))
    {
      BseSInstrument *sinstrument = BSE_SINSTRUMENT (input);
      
      bse_sinstrument_poke_foreigns (sinstrument, NULL, sinstrument->voice);
      if (sinstrument->voice)
	_bse_voice_fade_out (sinstrument->voice);
    }
#endif
}

static void
instrument_set_input (BseInstrument  *instrument,
		      BseSample      *sample)
{
  BseItem *item = BSE_ITEM (instrument);
  
  // g_return_if_fail (sample == NULL || sinstrument == NULL);
  
#if 0
  if (instrument->input)
    bse_item_cross_unref (item, BSE_ITEM (instrument->input));
#endif
  
  if (sample)
    {
      instrument->input = BSE_SOURCE (sample);
      instrument->type = BSE_INSTRUMENT_SAMPLE;
#if 0
      bse_item_cross_ref (item, BSE_ITEM (instrument->input), unset_input_cb, NULL);
#endif
      g_object_connect (instrument->input,
			"swapped_signal::notify::name", instrument_input_changed, instrument,
			NULL);
    }
#if 0
  else if (sinstrument && sinstrument->instrument == NULL)
    {
      instrument->input = BSE_SOURCE (sinstrument);
      instrument->type = BSE_INSTRUMENT_SYNTH;
      bse_item_cross_ref (item, BSE_ITEM (instrument->input), unset_input_cb, NULL);
      g_object_connect (instrument->input,
			"swapped_signal::notify::name", instrument_input_changed, instrument,
			NULL);
      bse_sinstrument_poke_foreigns (sinstrument, instrument, sinstrument->voice);
    }
#endif
}

static void
bse_instrument_set_property (BseInstrument *instrument,
			     guint          param_id,
			     GValue        *value,
			     GParamSpec    *pspec,
			     const gchar   *trailer)
{
  BseEnvelope *env = &instrument->env;
  
  switch (param_id)
    {
      guint total;
    case PARAM_SAMPLE:
      instrument_set_input (instrument, (BseSample*) g_value_get_object (value));
      // bse_object_param_changed (BSE_OBJECT (instrument), "sinstrument");
      break;
    case PARAM_SYNTH:
      // instrument_set_input (instrument, NULL, (BseSInstrument*) g_value_get_object (value));
      bse_object_param_changed (BSE_OBJECT (instrument), "sample");
      break;
    case PARAM_INTERPOLATION:
      instrument->interpolation = g_value_get_enum (value);
      break;
    case PARAM_POLYPHONY:
      instrument->polyphony = g_value_get_boolean (value);
      break;
    case PARAM_VOLUME_f:
      instrument->volume_factor = g_value_get_float (value);
      bse_object_param_changed (BSE_OBJECT (instrument), "volume_dB");
      bse_object_param_changed (BSE_OBJECT (instrument), "volume_perc");
      break;
    case PARAM_VOLUME_dB:
      instrument->volume_factor = bse_dB_to_factor (g_value_get_float (value));
      bse_object_param_changed (BSE_OBJECT (instrument), "volume_f");
      bse_object_param_changed (BSE_OBJECT (instrument), "volume_perc");
      break;
    case PARAM_VOLUME_PERC:
      instrument->volume_factor = ((gfloat) g_value_get_uint (value)) / 100;
      bse_object_param_changed (BSE_OBJECT (instrument), "volume_f");
      bse_object_param_changed (BSE_OBJECT (instrument), "volume_dB");
      break;
    case PARAM_BALANCE:
      instrument->balance = g_value_get_int (value);
      break;
    case PARAM_TRANSPOSE:
      instrument->transpose = g_value_get_int (value);
      break;
    case PARAM_FINE_TUNE:
      instrument->fine_tune = g_value_get_int (value);
      break;
    case PARAM_ATTACK_LEVEL:
      env->attack_level = ((gfloat) g_value_get_uint (value)) / 100;
      bse_object_param_changed (BSE_OBJECT (instrument), "envelope");
      break;
    case PARAM_SUSTAIN_LEVEL:
      env->sustain_level = ((gfloat) g_value_get_uint (value)) / 100;
      bse_object_param_changed (BSE_OBJECT (instrument), "envelope");
      break;
    case PARAM_RELEASE_LEVEL:
      env->release_level = ((gfloat) g_value_get_uint (value)) / 100;
      bse_object_param_changed (BSE_OBJECT (instrument), "envelope");
      break;
    case PARAM_ENVELOPE:
      if (bse_value_get_n_dots (value) != ENV_N_DOTS)
	G_OBJECT_WARN_INVALID_PROPERTY_ID (instrument, param_id, pspec);
      else
	{
	  BseDot *dots;
	  guint n_dots;
	  
	  total = ENVELOPE_TIME_TOTAL (instrument->env);
	  dots = bse_value_get_dots (value, &n_dots);
	  env->attack_level = ENV_ATTACK_LEVEL (dots);
	  bse_object_param_changed (BSE_OBJECT (instrument), "attack_level");
	  env->sustain_level = ENV_SUSTAIN_LEVEL (dots);
	  bse_object_param_changed (BSE_OBJECT (instrument), "sustain_level");
	  env->release_level = ENV_RELEASE_LEVEL (dots);
	  bse_object_param_changed (BSE_OBJECT (instrument), "release_level");
	  env->delay_time = ENV_DELAY_TIME (dots) * total + 0.5;
	  env->attack_time = CLAMP ((ENV_ATTACK_TIME (dots) -
				     ENV_DELAY_TIME (dots)) * total + 0.5,
				    0, BSE_MAX_ENV_TIME);
	  env->decay_time = CLAMP ((ENV_DECAY_TIME (dots) -
				    ENV_ATTACK_TIME (dots)) * total + 0.5,
				   0, BSE_MAX_ENV_TIME);
	  env->sustain_time = CLAMP ((ENV_SUSTAIN_TIME (dots) -
				      ENV_DECAY_TIME (dots)) * total + 0.5,
				     0, BSE_MAX_ENV_TIME);
	  env->release_time = CLAMP ((1.0 /* ENV_RELEASE_TIME (dots) */ -
				      ENV_SUSTAIN_TIME (dots)) * total + 0.5,
				     0, BSE_MAX_ENV_TIME);
	  bse_object_param_changed (BSE_OBJECT (instrument), "delay_time");
	  bse_object_param_changed (BSE_OBJECT (instrument), "attack_time");
	  bse_object_param_changed (BSE_OBJECT (instrument), "decay_time");
	  bse_object_param_changed (BSE_OBJECT (instrument), "sustain_time");
	  bse_object_param_changed (BSE_OBJECT (instrument), "release_time");
	  bse_object_param_changed (BSE_OBJECT (instrument), "duration");
	}
      break;
    case PARAM_DELAY_TIME:
      env->delay_time = g_value_get_uint (value);
      bse_object_param_changed (BSE_OBJECT (instrument), "envelope");
      bse_object_param_changed (BSE_OBJECT (instrument), "duration");
      break;
    case PARAM_ATTACK_TIME:
      env->attack_time = g_value_get_uint (value);
      bse_object_param_changed (BSE_OBJECT (instrument), "envelope");
      bse_object_param_changed (BSE_OBJECT (instrument), "duration");
      break;
    case PARAM_SUSTAIN_TIME:
      env->sustain_time = g_value_get_uint (value);
      bse_object_param_changed (BSE_OBJECT (instrument), "envelope");
      bse_object_param_changed (BSE_OBJECT (instrument), "duration");
      break;
    case PARAM_DECAY_TIME:
      env->decay_time = g_value_get_uint (value);
      bse_object_param_changed (BSE_OBJECT (instrument), "envelope");
      bse_object_param_changed (BSE_OBJECT (instrument), "duration");
      break;
    case PARAM_RELEASE_TIME:
      env->release_time = g_value_get_uint (value);
      bse_object_param_changed (BSE_OBJECT (instrument), "envelope");
      bse_object_param_changed (BSE_OBJECT (instrument), "duration");
      break;
    case PARAM_DURATION:
      total = ENVELOPE_TIME_TOTAL (instrument->env);
      if (env->delay_time)
	env->delay_time = CLAMP (((gfloat) env->delay_time) /
				 total * g_value_get_uint (value) + 0.5,
				 1, BSE_MAX_ENV_TIME);
      bse_object_param_changed (BSE_OBJECT (instrument), "delay_time");
      if (env->attack_time)
	env->attack_time = CLAMP (((gfloat) env->attack_time) /
				  total * g_value_get_uint (value) + 0.5,
				  1, BSE_MAX_ENV_TIME);
      bse_object_param_changed (BSE_OBJECT (instrument), "attack_time");
      if (env->decay_time)
	env->decay_time = CLAMP (((gfloat) env->decay_time) /
				 total * g_value_get_uint (value) + 0.5,
				 1, BSE_MAX_ENV_TIME);
      bse_object_param_changed (BSE_OBJECT (instrument), "decay_time");
      if (env->sustain_time)
	env->sustain_time = CLAMP (((gfloat) env->sustain_time) /
				   total * g_value_get_uint (value) + 0.5,
				   1, BSE_MAX_ENV_TIME);
      bse_object_param_changed (BSE_OBJECT (instrument), "sustain_time");
      if (env->release_time)
	env->release_time = CLAMP (((gfloat) env->release_time) /
				   total * g_value_get_uint (value) + 0.5,
				   1, BSE_MAX_ENV_TIME);
      bse_object_param_changed (BSE_OBJECT (instrument), "release_time");
      bse_object_param_changed (BSE_OBJECT (instrument), "envelope");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (instrument, param_id, pspec);
      break;
    }
}

static void
bse_instrument_get_property (BseInstrument *instrument,
			     guint          param_id,
			     GValue        *value,
			     GParamSpec    *pspec,
			     const gchar   *trailer)
{
  BseEnvelope *env = &instrument->env;
  
  switch (param_id)
    {
      BseDot dots[ENV_N_DOTS];
      guint total;
    case PARAM_SAMPLE:
      if (instrument->type == BSE_INSTRUMENT_SAMPLE && instrument->input)
	g_value_set_object (value, G_OBJECT (instrument->input));
      else
	g_value_set_object (value, NULL);
      break;
    case PARAM_SYNTH:
      if (instrument->type == BSE_INSTRUMENT_SYNTH && instrument->input)
	g_value_set_object (value, G_OBJECT (instrument->input));
      else
	g_value_set_object (value, NULL);
      break;
    case PARAM_INTERPOLATION:
      g_value_set_enum (value, instrument->interpolation);
      break;
    case PARAM_POLYPHONY:
      g_value_set_boolean (value, instrument->polyphony);
      break;
    case PARAM_VOLUME_f:
      g_value_set_float (value, instrument->volume_factor);
      break;
    case PARAM_VOLUME_dB:
      g_value_set_float (value, bse_dB_from_factor (instrument->volume_factor, BSE_MIN_VOLUME_dB));
      break;
    case PARAM_VOLUME_PERC:
      g_value_set_uint (value, instrument->volume_factor * ((gfloat) 100) + 0.5);
      break;
    case PARAM_BALANCE:
      g_value_set_int (value, instrument->balance);
      break;
    case PARAM_TRANSPOSE:
      g_value_set_int (value, instrument->transpose);
      break;
    case PARAM_FINE_TUNE:
      g_value_set_int (value, instrument->fine_tune);
      break;
    case PARAM_ATTACK_LEVEL:
      g_value_set_uint (value, env->attack_level * 100);
      break;
    case PARAM_SUSTAIN_LEVEL:
      g_value_set_uint (value, env->sustain_level * 100);
      break;
    case PARAM_RELEASE_LEVEL:
      g_value_set_uint (value, env->release_level * 100);
      break;
    case PARAM_ENVELOPE:
      memcpy (&dots, &env_dflt_dots, sizeof (dots));
      total = ENVELOPE_TIME_TOTAL (instrument->env);
      ENV_DELAY_TIME  (dots) = ((gfloat) env->delay_time) / total;
      ENV_ATTACK_TIME (dots) = (ENV_DELAY_TIME (dots) +
				((gfloat) env->attack_time) / total);
      ENV_DECAY_TIME (dots) = (ENV_ATTACK_TIME (dots) +
			       ((gfloat) env->decay_time) / total);
      ENV_SUSTAIN_TIME (dots) = (ENV_DECAY_TIME (dots) +
				 ((gfloat) env->sustain_time) / total);
      ENV_RELEASE_TIME (dots) = (ENV_SUSTAIN_TIME (dots) +
				 ((gfloat) env->release_time) / total);
      ENV_ATTACK_LEVEL (dots) = env->attack_level;
      ENV_SUSTAIN_LEVEL (dots) = env->sustain_level;
      ENV_RELEASE_LEVEL (dots) = env->release_level;
      bse_value_set_dots (value, ENV_N_DOTS, dots);
      break;
    case PARAM_DELAY_TIME:
      g_value_set_uint (value, env->delay_time);
      break;
    case PARAM_ATTACK_TIME:
      g_value_set_uint (value, env->attack_time);
      break;
    case PARAM_DECAY_TIME:
      g_value_set_uint (value, env->decay_time);
      break;
    case PARAM_SUSTAIN_TIME:
      g_value_set_uint (value, env->sustain_time);
      break;
    case PARAM_RELEASE_TIME:
      g_value_set_uint (value, env->release_time);
      break;
    case PARAM_DURATION:
      g_value_set_uint (value, ENVELOPE_TIME_TOTAL (instrument->env));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (instrument, param_id, pspec);
      break;
    }
}

void
bse_instrument_set_sample (BseInstrument  *instrument,
			   BseSample	  *sample)
{
  g_return_if_fail (BSE_IS_INSTRUMENT (instrument));
  if (sample)
    {
      g_return_if_fail (BSE_IS_SAMPLE (sample));
      g_return_if_fail (bse_item_get_project (BSE_ITEM (instrument)) ==
			bse_item_get_project (BSE_ITEM (sample)));
    }
  
  bse_object_set (BSE_OBJECT (instrument),
		  "sample", sample,
		  NULL);
}

#if 0
void
bse_instrument_set_sinstrument (BseInstrument  *instrument,
				BseSInstrument *sinstrument)
{
  g_return_if_fail (BSE_IS_INSTRUMENT (instrument));
  if (sinstrument)
    {
      g_return_if_fail (BSE_IS_SINSTRUMENT (sinstrument));
      g_return_if_fail (bse_item_get_project (BSE_ITEM (instrument)) ==
			bse_item_get_project (BSE_ITEM (sinstrument)));
    }
  
  bse_object_set (BSE_OBJECT (instrument),
		  "sinstrument", sinstrument,
		  NULL);
}
#endif
