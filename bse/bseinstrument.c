/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997, 1998, 1999 Olaf Hoehmann and Tim Janik
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
static BseDot env_dots[ENV_N_DOTS] = {
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
static void	bse_instrument_destroy		(BseObject		*object);
static void	bse_instrument_set_param	(BseInstrument		*instrument,
						 BseParam		*param);
static void	bse_instrument_get_param	(BseInstrument		*instrument,
						 BseParam		*param);
static void	bse_instrument_unlocked		(BseObject		*object);


/* --- variables --- */
static BseTypeClass	*parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseInstrument)
{
  static const BseTypeInfo instrument_info = {
    sizeof (BseInstrumentClass),
    
    (BseClassInitBaseFunc) NULL,
    (BseClassDestroyBaseFunc) NULL,
    (BseClassInitFunc) bse_instrument_class_init,
    (BseClassDestroyFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseInstrument),
    8 /* n_preallocs */,
    (BseObjectInitFunc) bse_instrument_init,
  };
  
  return bse_type_register_static (BSE_TYPE_ITEM,
				   "BseInstrument",
				   "BSE instrument type",
				   &instrument_info);
}

static void
bse_instrument_class_init (BseInstrumentClass *class)
{
  BseObjectClass *object_class;
  BseItemClass *item_class;
  
  parent_class = bse_type_class_peek (BSE_TYPE_ITEM);
  object_class = BSE_OBJECT_CLASS (class);
  item_class = BSE_ITEM_CLASS (class);
  
  object_class->set_param = (BseObjectSetParamFunc) bse_instrument_set_param;
  object_class->get_param = (BseObjectGetParamFunc) bse_instrument_get_param;
  object_class->unlocked = bse_instrument_unlocked;
  object_class->destroy = bse_instrument_destroy;
  
  bse_object_class_add_param (object_class, NULL,
			      PARAM_SAMPLE,
			      bse_param_spec_item ("sample", "Sample",
						   BSE_TYPE_SAMPLE,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, NULL,
			      PARAM_INTERPOLATION,
			      bse_param_spec_bool ("interpolation", "Use interpolation?",
						   TRUE,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, NULL,
			      PARAM_POLYPHONY,
			      bse_param_spec_bool ("polyphony", "Polyphony instrument?",
						   FALSE,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_VOLUME_f,
			      bse_param_spec_float ("volume_f", "Volume [float]",
						    0, bse_dB_to_factor (BSE_MAX_VOLUME_dB),
						    0.1,
						    bse_dB_to_factor (BSE_DFL_INSTRUMENT_VOLUME_dB),
						    BSE_PARAM_STORAGE));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_VOLUME_dB,
			      bse_param_spec_float ("volume_dB", "Volume [dB]",
						    BSE_MIN_VOLUME_dB, BSE_MAX_VOLUME_dB,
						    BSE_STP_VOLUME_dB,
						    BSE_DFL_INSTRUMENT_VOLUME_dB,
						    BSE_PARAM_GUI |
						    BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_VOLUME_PERC,
			      bse_param_spec_uint ("volume_perc", "Volume [%]",
						   0, bse_dB_to_factor (BSE_MAX_VOLUME_dB) * 100,
						   1,
						   bse_dB_to_factor (BSE_DFL_SONG_VOLUME_dB) * 100,
						   BSE_PARAM_GUI |
						   BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_BALANCE,
			      bse_param_spec_int ("balance", "Balance",
						  BSE_MIN_BALANCE, BSE_MAX_BALANCE,
						  BSE_STP_BALANCE,
						  BSE_DFL_INSTRUMENT_BALANCE,
						  BSE_PARAM_DEFAULT |
						  BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_TRANSPOSE,
			      bse_param_spec_int ("transpose", "Transpose",
						  BSE_MIN_TRANSPOSE, BSE_MAX_TRANSPOSE,
						  BSE_STP_TRANSPOSE,
						  BSE_DFL_INSTRUMENT_TRANSPOSE,
						  BSE_PARAM_DEFAULT |
						  BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_FINE_TUNE,
			      bse_param_spec_int ("fine_tune", "Fine tune",
						  BSE_MIN_FINE_TUNE, BSE_MAX_FINE_TUNE,
						  BSE_STP_FINE_TUNE,
						  BSE_DFL_INSTRUMENT_FINE_TUNE,
						  BSE_PARAM_DEFAULT |
						  BSE_PARAM_HINT_SCALE));
  /* envelope
   */
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_ENVELOPE,
			      bse_param_spec_dots ("envelope", "Envelope",
						   ENV_N_DOTS, env_dots,
						   BSE_PARAM_GUI));
  /* envelope parameters
   */
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_DELAY_TIME,
			      bse_param_spec_uint ("delay_time", "Delay Time",
						   0, BSE_MAX_ENV_TIME, BSE_STP_ENV_TIME,
						   ENV_DELAY_TIME (env_dots) * BSE_MAX_ENV_TIME,
						   BSE_PARAM_DEFAULT |
						   BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_ATTACK_TIME,
			      bse_param_spec_uint ("attack_time", "Attack Time",
						   0, BSE_MAX_ENV_TIME, BSE_STP_ENV_TIME,
						   ENV_ATTACK_TIME (env_dots) * BSE_MAX_ENV_TIME,
						   BSE_PARAM_DEFAULT |
						   BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_ATTACK_LEVEL,
			      bse_param_spec_uint ("attack_level", "Attack Level",
						   0, 100, 1,
						   ENV_ATTACK_LEVEL (env_dots) * 100,
						   BSE_PARAM_DEFAULT |
						   BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_DECAY_TIME,
			      bse_param_spec_uint ("decay_time", "Decay Time",
						   0, BSE_MAX_ENV_TIME, BSE_STP_ENV_TIME,
						   ENV_DECAY_TIME (env_dots) * BSE_MAX_ENV_TIME,
						   BSE_PARAM_DEFAULT |
						   BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_SUSTAIN_LEVEL,
			      bse_param_spec_uint ("sustain_level", "Sustain Level",
						   0, 100, 1,
						   ENV_SUSTAIN_LEVEL (env_dots) * 100,
						   BSE_PARAM_DEFAULT |
						   BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_SUSTAIN_TIME,
			      bse_param_spec_uint ("sustain_time", "Sustain Time",
						   0, BSE_MAX_ENV_TIME, BSE_STP_ENV_TIME,
						   ENV_SUSTAIN_TIME (env_dots) * BSE_MAX_ENV_TIME,
						   BSE_PARAM_DEFAULT |
						   BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_RELEASE_LEVEL,
			      bse_param_spec_uint ("release_level", "Release Level",
						   0, 100, 1,
						   ENV_RELEASE_LEVEL (env_dots) * 100,
						   BSE_PARAM_DEFAULT |
						   BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_RELEASE_TIME,
			      bse_param_spec_uint ("release_time", "Release Time",
						   0, BSE_MAX_ENV_TIME, BSE_STP_ENV_TIME,
						   ENV_RELEASE_TIME (env_dots) * BSE_MAX_ENV_TIME,
						   BSE_PARAM_DEFAULT |
						   BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_DURATION,
			      bse_param_spec_uint ("duration", "Duration [ms]",
						   0, BSE_MAX_ENV_TIME * 5, BSE_STP_ENV_TIME * 5,
						   BSE_MAX_ENV_TIME,
						   BSE_PARAM_GUI |
						   BSE_PARAM_HINT_SCALE));
}

static void
bse_instrument_init (BseInstrument *instrument)
{
  instrument->type = BSE_INSTRUMENT_NONE;
  instrument->sample = NULL;
  instrument->locked_sample = NULL;
  
  instrument->interpolation = TRUE;
  instrument->polyphony = FALSE;
  instrument->volume_factor = bse_dB_to_factor (BSE_DFL_INSTRUMENT_VOLUME_dB);
  instrument->balance = BSE_DFL_INSTRUMENT_BALANCE;
  instrument->transpose = BSE_DFL_INSTRUMENT_TRANSPOSE;
  instrument->fine_tune = BSE_DFL_INSTRUMENT_FINE_TUNE;
  
  instrument->env.delay_time = ENV_DELAY_TIME (env_dots) * BSE_MAX_ENV_TIME;
  instrument->env.attack_time = ENV_ATTACK_TIME (env_dots) * BSE_MAX_ENV_TIME;
  instrument->env.attack_level = ENV_ATTACK_LEVEL (env_dots);
  instrument->env.decay_time = ENV_DECAY_TIME (env_dots) * BSE_MAX_ENV_TIME;
  instrument->env.sustain_level = ENV_SUSTAIN_LEVEL (env_dots);
  instrument->env.sustain_time = ENV_SUSTAIN_TIME (env_dots) * BSE_MAX_ENV_TIME;
  instrument->env.release_level = ENV_RELEASE_LEVEL (env_dots);
  instrument->env.release_time = ENV_RELEASE_TIME (env_dots) * BSE_MAX_ENV_TIME;
}

static void
bse_instrument_destroy (BseObject *object)
{
  BseInstrument *instrument = BSE_INSTRUMENT (object);
  
  if (instrument->sample)
    {
      bse_object_unref (BSE_OBJECT (instrument->sample));
      instrument->sample = NULL;
    }
  if (instrument->locked_sample)
    {
      bse_object_unref (BSE_OBJECT (instrument->locked_sample));
      instrument->locked_sample = NULL;
    }
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_instrument_unlocked (BseObject *object)
{
  BseInstrument *instrument = BSE_INSTRUMENT (object);
  
  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->unlocked)
    BSE_OBJECT_CLASS (parent_class)->unlocked (object);
  
  if (instrument->sample != instrument->locked_sample)
    {
      if (instrument->locked_sample)
	bse_object_unref (BSE_OBJECT (instrument->locked_sample));
      instrument->locked_sample = instrument->sample;
      if (instrument->locked_sample)
	bse_object_ref (BSE_OBJECT (instrument->locked_sample));
    }
}

static void
bse_instrument_set_param (BseInstrument *instrument,
			  BseParam	*param)
{
  BseItem *item = BSE_ITEM (instrument);
  BseEnvelope *env = &instrument->env;
  
  switch (param->pspec->any.param_id)
    {
      guint total;
    case PARAM_SAMPLE:
      if (!param->value.v_item ||
	  bse_super_get_project (BSE_SUPER (param->value.v_item)) == bse_item_get_project (item))
	{
	  if (instrument->sample)
	    {
	      bse_object_unref (BSE_OBJECT (instrument->sample));
	      instrument->type = BSE_INSTRUMENT_NONE;
	    }
	  instrument->sample = (BseSample*) param->value.v_item;
	  if (instrument->sample)
	    {
	      bse_object_ref (BSE_OBJECT (instrument->sample));
	      instrument->type = BSE_INSTRUMENT_SAMPLE;
	    }
	}
      if (!BSE_OBJECT_IS_LOCKED (instrument) &&
	  instrument->sample != instrument->locked_sample)
	{
	  if (instrument->locked_sample)
	    bse_object_unref (BSE_OBJECT (instrument->locked_sample));
	  instrument->locked_sample = instrument->sample;
	  if (instrument->locked_sample)
	    bse_object_ref (BSE_OBJECT (instrument->locked_sample));
	}
      break;
    case PARAM_INTERPOLATION:
      instrument->interpolation = param->value.v_bool;
      break;
    case PARAM_POLYPHONY:
      instrument->polyphony = param->value.v_bool;
      break;
    case PARAM_VOLUME_f:
      instrument->volume_factor = param->value.v_float;
      bse_object_param_changed (BSE_OBJECT (instrument), "volume_dB");
      bse_object_param_changed (BSE_OBJECT (instrument), "volume_perc");
      break;
    case PARAM_VOLUME_dB:
      instrument->volume_factor = bse_dB_to_factor (param->value.v_float);
      bse_object_param_changed (BSE_OBJECT (instrument), "volume_f");
      bse_object_param_changed (BSE_OBJECT (instrument), "volume_perc");
      break;
    case PARAM_VOLUME_PERC:
      instrument->volume_factor = ((gfloat) param->value.v_uint) / 100;
      bse_object_param_changed (BSE_OBJECT (instrument), "volume_f");
      bse_object_param_changed (BSE_OBJECT (instrument), "volume_dB");
      break;
    case PARAM_BALANCE:
      instrument->balance = param->value.v_int;
      break;
    case PARAM_TRANSPOSE:
      instrument->transpose = param->value.v_int;
      break;
    case PARAM_FINE_TUNE:
      instrument->fine_tune = param->value.v_int;
      break;
    case PARAM_ATTACK_LEVEL:
      env->attack_level = ((gfloat) param->value.v_uint) / 100;
      bse_object_param_changed (BSE_OBJECT (instrument), "envelope");
      break;
    case PARAM_SUSTAIN_LEVEL:
      env->sustain_level = ((gfloat) param->value.v_uint) / 100;
      bse_object_param_changed (BSE_OBJECT (instrument), "envelope");
      break;
    case PARAM_RELEASE_LEVEL:
      env->release_level = ((gfloat) param->value.v_uint) / 100;
      bse_object_param_changed (BSE_OBJECT (instrument), "envelope");
      break;
    case PARAM_ENVELOPE:
      total = ENVELOPE_TIME_TOTAL (instrument->env);
      env->attack_level = ENV_ATTACK_LEVEL (param->value.v_dots);
      bse_object_param_changed (BSE_OBJECT (instrument), "attack_level");
      env->sustain_level = ENV_SUSTAIN_LEVEL (param->value.v_dots);
      bse_object_param_changed (BSE_OBJECT (instrument), "sustain_level");
      env->release_level = ENV_RELEASE_LEVEL (param->value.v_dots);
      bse_object_param_changed (BSE_OBJECT (instrument), "release_level");
      env->delay_time = ENV_DELAY_TIME (param->value.v_dots) * total + 0.5;
      env->attack_time = CLAMP ((ENV_ATTACK_TIME (param->value.v_dots) -
				 ENV_DELAY_TIME (param->value.v_dots)) * total + 0.5,
				0, BSE_MAX_ENV_TIME);
      env->decay_time = CLAMP ((ENV_DECAY_TIME (param->value.v_dots) -
				ENV_ATTACK_TIME (param->value.v_dots)) * total + 0.5,
			       0, BSE_MAX_ENV_TIME);
      env->sustain_time = CLAMP ((ENV_SUSTAIN_TIME (param->value.v_dots) -
				  ENV_DECAY_TIME (param->value.v_dots)) * total + 0.5,
				 0, BSE_MAX_ENV_TIME);
      env->release_time = CLAMP ((1.0 /* ENV_RELEASE_TIME (param->value.v_dots) */ -
				  ENV_SUSTAIN_TIME (param->value.v_dots)) * total + 0.5,
				 0, BSE_MAX_ENV_TIME);
      bse_object_param_changed (BSE_OBJECT (instrument), "delay_time");
      bse_object_param_changed (BSE_OBJECT (instrument), "attack_time");
      bse_object_param_changed (BSE_OBJECT (instrument), "decay_time");
      bse_object_param_changed (BSE_OBJECT (instrument), "sustain_time");
      bse_object_param_changed (BSE_OBJECT (instrument), "release_time");
      bse_object_param_changed (BSE_OBJECT (instrument), "duration");
      break;
    case PARAM_DELAY_TIME:
      env->delay_time = param->value.v_uint;
      bse_object_param_changed (BSE_OBJECT (instrument), "envelope");
      bse_object_param_changed (BSE_OBJECT (instrument), "duration");
      break;
    case PARAM_ATTACK_TIME:
      env->attack_time = param->value.v_uint;
      bse_object_param_changed (BSE_OBJECT (instrument), "envelope");
      bse_object_param_changed (BSE_OBJECT (instrument), "duration");
      break;
    case PARAM_SUSTAIN_TIME:
      env->sustain_time = param->value.v_uint;
      bse_object_param_changed (BSE_OBJECT (instrument), "envelope");
      bse_object_param_changed (BSE_OBJECT (instrument), "duration");
      break;
    case PARAM_DECAY_TIME:
      env->decay_time = param->value.v_uint;
      bse_object_param_changed (BSE_OBJECT (instrument), "envelope");
      bse_object_param_changed (BSE_OBJECT (instrument), "duration");
      break;
    case PARAM_RELEASE_TIME:
      env->release_time = param->value.v_uint;
      bse_object_param_changed (BSE_OBJECT (instrument), "envelope");
      bse_object_param_changed (BSE_OBJECT (instrument), "duration");
      break;
    case PARAM_DURATION:
      total = ENVELOPE_TIME_TOTAL (instrument->env);
      if (env->delay_time)
	env->delay_time = CLAMP (((gfloat) env->delay_time) /
				 total * param->value.v_uint + 0.5,
				 1, BSE_MAX_ENV_TIME);
      bse_object_param_changed (BSE_OBJECT (instrument), "delay_time");
      if (env->attack_time)
	env->attack_time = CLAMP (((gfloat) env->attack_time) /
				  total * param->value.v_uint + 0.5,
				  1, BSE_MAX_ENV_TIME);
      bse_object_param_changed (BSE_OBJECT (instrument), "attack_time");
      if (env->decay_time)
	env->decay_time = CLAMP (((gfloat) env->decay_time) /
				 total * param->value.v_uint + 0.5,
				 1, BSE_MAX_ENV_TIME);
      bse_object_param_changed (BSE_OBJECT (instrument), "decay_time");
      if (env->sustain_time)
	env->sustain_time = CLAMP (((gfloat) env->sustain_time) /
				   total * param->value.v_uint + 0.5,
				   1, BSE_MAX_ENV_TIME);
      bse_object_param_changed (BSE_OBJECT (instrument), "sustain_time");
      if (env->release_time)
	env->release_time = CLAMP (((gfloat) env->release_time) /
				   total * param->value.v_uint + 0.5,
				   1, BSE_MAX_ENV_TIME);
      bse_object_param_changed (BSE_OBJECT (instrument), "release_time");
      bse_object_param_changed (BSE_OBJECT (instrument), "envelope");
      break;
    default:
      g_warning ("%s(\"%s\"): invalid attempt to set parameter \"%s\" of type `%s'",
                 BSE_OBJECT_TYPE_NAME (instrument),
                 BSE_OBJECT_NAME (instrument),
		 param->pspec->any.name,
		 bse_type_name (param->pspec->type));
      break;
    }
}

static void
bse_instrument_get_param (BseInstrument *instrument,
			  BseParam	*param)
{
  BseEnvelope *env = &instrument->env;
  
  switch (param->pspec->any.param_id)
    {
      guint total;
    case PARAM_SAMPLE:
      if (param->value.v_item)
	bse_object_unref (BSE_OBJECT (param->value.v_item));
      param->value.v_item = (BseItem*) instrument->sample;
      if (param->value.v_item)
	bse_object_ref (BSE_OBJECT (param->value.v_item));
      break;
    case PARAM_INTERPOLATION:
      param->value.v_bool = instrument->interpolation;
      break;
    case PARAM_POLYPHONY:
      param->value.v_bool = instrument->polyphony;
      break;
    case PARAM_VOLUME_f:
      param->value.v_float = instrument->volume_factor;
      break;
    case PARAM_VOLUME_dB:
      param->value.v_float = bse_dB_from_factor (instrument->volume_factor, BSE_MIN_VOLUME_dB);
      break;
    case PARAM_VOLUME_PERC:
      param->value.v_uint = instrument->volume_factor * ((gfloat) 100) + 0.5;
      break;
    case PARAM_BALANCE:
      param->value.v_int = instrument->balance;
      break;
    case PARAM_TRANSPOSE:
      param->value.v_int = instrument->transpose;
      break;
    case PARAM_FINE_TUNE:
      param->value.v_int = instrument->fine_tune;
      break;
    case PARAM_ATTACK_LEVEL:
      param->value.v_int = env->attack_level * 100;
      break;
    case PARAM_SUSTAIN_LEVEL:
      param->value.v_int = env->sustain_level * 100;
      break;
    case PARAM_RELEASE_LEVEL:
      param->value.v_int = env->release_level * 100;
      break;
    case PARAM_ENVELOPE:
      bse_param_reset_value (param); /* ensure the param contains a valid dot array */
      total = ENVELOPE_TIME_TOTAL (instrument->env);
      ENV_DELAY_TIME  (param->value.v_dots) = ((gfloat) env->delay_time) / total;
      ENV_ATTACK_TIME  (param->value.v_dots) = (ENV_DELAY_TIME	(param->value.v_dots) +
						((gfloat) env->attack_time) / total);
      ENV_DECAY_TIME  (param->value.v_dots) = (ENV_ATTACK_TIME	(param->value.v_dots) +
					       ((gfloat) env->decay_time) / total);
      ENV_SUSTAIN_TIME	(param->value.v_dots) = (ENV_DECAY_TIME	 (param->value.v_dots) +
						 ((gfloat) env->sustain_time) / total);
      ENV_RELEASE_TIME	(param->value.v_dots) = (ENV_SUSTAIN_TIME  (param->value.v_dots) +
						 ((gfloat) env->release_time) / total);
      ENV_ATTACK_LEVEL	(param->value.v_dots) = env->attack_level;
      ENV_SUSTAIN_LEVEL (param->value.v_dots) = env->sustain_level;
      ENV_RELEASE_LEVEL (param->value.v_dots) = env->release_level;
      break;
    case PARAM_DELAY_TIME:
      param->value.v_uint = env->delay_time;
      break;
    case PARAM_ATTACK_TIME:
      param->value.v_uint = env->attack_time;
      break;
    case PARAM_DECAY_TIME:
      param->value.v_uint = env->decay_time;
      break;
    case PARAM_SUSTAIN_TIME:
      param->value.v_uint = env->sustain_time;
      break;
    case PARAM_RELEASE_TIME:
      param->value.v_uint = env->release_time;
      break;
    case PARAM_DURATION:
      param->value.v_uint = ENVELOPE_TIME_TOTAL (instrument->env);
      break;
    default:
      g_warning ("%s(\"%s\"): invalid attempt to get parameter \"%s\" of type `%s'",
                 BSE_OBJECT_TYPE_NAME (instrument),
                 BSE_OBJECT_NAME (instrument),
		 param->pspec->any.name,
		 bse_type_name (param->pspec->type));
      break;
    }
}

void
bse_instrument_set_sample (BseInstrument  *instrument,
			   BseSample	  *sample)
{
  g_return_if_fail (BSE_IS_INSTRUMENT (instrument));
  g_return_if_fail (BSE_IS_SAMPLE (sample));
  g_return_if_fail (bse_super_get_project (BSE_SUPER (sample)) ==
		    bse_item_get_project (BSE_ITEM (instrument)));
  
  bse_object_set (BSE_OBJECT (instrument),
		  "sample", sample,
		  NULL);
}
