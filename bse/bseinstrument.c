/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2002 Tim Janik
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

#include	"bseglobals.h"
#include	"bsewave.h"
#include	"bsesnet.h"
#include	"bsemain.h"
#include	"bsestandardsynths.h"


enum {
  PARAM_0,
  PARAM_SYNTH_TYPE,
  PARAM_WAVE,
  PARAM_SYNTH_NET,
  PARAM_SEQ_SYNTH,
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
static void	bse_instrument_set_property	(GObject		*object,
						 guint                   param_id,
						 const GValue           *value,
						 GParamSpec             *pspec);
static void	bse_instrument_get_property	(GObject		*object,
						 guint                   param_id,
						 GValue                 *value,
						 GParamSpec             *pspec);
static void	instrument_set_synth_type	(BseInstrument		*instrument,
						 BseInstrumentType	 type);


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
    0 /* n_preallocs */,
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
  
  gobject_class->set_property = bse_instrument_set_property;
  gobject_class->get_property = bse_instrument_get_property;
  
  object_class->destroy = bse_instrument_do_destroy;
  
  bse_object_class_add_param (object_class, "Synth Input",
			      PARAM_SYNTH_TYPE,
			      bse_param_spec_genum ("synth_type", "Synth Type",
						    "The synthesis type specifies the synthesis kind to be "
						    "used for this instrument",
						    BSE_TYPE_INSTRUMENT_TYPE,
						    BSE_INSTRUMENT_STANDARD_PIANO,
						    SFI_PARAM_STANDARD));
  bse_object_class_add_param (object_class, "Synth Input",
			      PARAM_WAVE,
			      g_param_spec_object ("wave", "Custom Wave", "The wave to be used for wave synthesis",
						   BSE_TYPE_WAVE,
						   SFI_PARAM_STANDARD));
  bse_object_class_add_param (object_class, "Synth Input",
			      PARAM_SYNTH_NET,
			      g_param_spec_object ("user_snet", "Custom Synth Net", "Synthesis network for customized synthesis",
						   BSE_TYPE_SNET,
						   SFI_PARAM_STANDARD));
  bse_object_class_add_param (object_class, "Synth Input",
			      PARAM_SEQ_SYNTH,
			      g_param_spec_object ("seq_snet", "Seq Synth", NULL,
						   BSE_TYPE_SNET,
						   SFI_PARAM_SERVE_GUI | SFI_PARAM_READABLE));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_VOLUME_f,
			      sfi_pspec_real ("volume_f", "Volume [float]", NULL,
					      bse_dB_to_factor (BSE_DFL_INSTRUMENT_VOLUME_dB),
					      0, bse_dB_to_factor (BSE_MAX_VOLUME_dB), 0.1,
					      SFI_PARAM_STORAGE));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_VOLUME_dB,
			      sfi_pspec_real ("volume_dB", "Volume [dB]", NULL,
					      BSE_DFL_INSTRUMENT_VOLUME_dB,
					      BSE_MIN_VOLUME_dB, BSE_MAX_VOLUME_dB,
					      BSE_STP_VOLUME_dB,
					      SFI_PARAM_GUI SFI_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_VOLUME_PERC,
			      bse_param_spec_uint ("volume_perc", "Volume [%]", NULL,
						   0, bse_dB_to_factor (BSE_MAX_VOLUME_dB) * 100,
						   bse_dB_to_factor (BSE_DFL_INSTRUMENT_VOLUME_dB) * 100, 1,
						   SFI_PARAM_GUI |
						   SFI_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_BALANCE,
			      sfi_pspec_int ("balance", "Balance", NULL,
					     0, BSE_MIN_BALANCE_f, BSE_MAX_BALANCE_f, 10,
					     SFI_PARAM_STANDARD SFI_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_TRANSPOSE,
			      sfi_pspec_int ("transpose", "Transpose", NULL,
					     0, BSE_MIN_TRANSPOSE, BSE_MAX_TRANSPOSE, 10,
					     SFI_PARAM_STANDARD SFI_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_FINE_TUNE,
			      sfi_pspec_int ("fine_tune", "Fine tune", NULL,
					     0, BSE_MIN_FINE_TUNE, BSE_MAX_FINE_TUNE, 10,
					     SFI_PARAM_STANDARD SFI_PARAM_HINT_SCALE));
  /* envelope
   */
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_ENVELOPE,
			      bse_param_spec_dots ("envelope", "Envelope", NULL,
						   ENV_N_DOTS, env_dflt_dots,
						   SFI_PARAM_GUI));
  /* envelope parameters
   */
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_DELAY_TIME,
			      bse_param_spec_uint ("delay_time", "Delay Time", NULL,
						   0, BSE_MAX_ENV_TIME,
						   ENV_DELAY_TIME (env_dflt_dots) * BSE_MAX_ENV_TIME,
						   BSE_STP_ENV_TIME,
						   SFI_PARAM_STANDARD |
						   SFI_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_ATTACK_TIME,
			      bse_param_spec_uint ("attack_time", "Attack Time", NULL,
						   0, BSE_MAX_ENV_TIME,
						   ENV_ATTACK_TIME (env_dflt_dots) * BSE_MAX_ENV_TIME,
						   BSE_STP_ENV_TIME,
						   SFI_PARAM_STANDARD |
						   SFI_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_ATTACK_LEVEL,
			      bse_param_spec_uint ("attack_level", "Attack Level", NULL,
						   0, 100,
						   ENV_ATTACK_LEVEL (env_dflt_dots) * 100, 1,
						   SFI_PARAM_STANDARD |
						   SFI_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_DECAY_TIME,
			      bse_param_spec_uint ("decay_time", "Decay Time", NULL,
						   0, BSE_MAX_ENV_TIME,
						   ENV_DECAY_TIME (env_dflt_dots) * BSE_MAX_ENV_TIME,
						   BSE_STP_ENV_TIME,
						   SFI_PARAM_STANDARD |
						   SFI_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_SUSTAIN_LEVEL,
			      bse_param_spec_uint ("sustain_level", "Sustain Level", NULL,
						   0, 100,
						   ENV_SUSTAIN_LEVEL (env_dflt_dots) * 100, 1,
						   SFI_PARAM_STANDARD |
						   SFI_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_SUSTAIN_TIME,
			      bse_param_spec_uint ("sustain_time", "Sustain Time", NULL,
						   0, BSE_MAX_ENV_TIME,
						   ENV_SUSTAIN_TIME (env_dflt_dots) * BSE_MAX_ENV_TIME,
						   BSE_STP_ENV_TIME,
						   SFI_PARAM_STANDARD |
						   SFI_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_RELEASE_LEVEL,
			      bse_param_spec_uint ("release_level", "Release Level", NULL,
						   0, 100,
						   ENV_RELEASE_LEVEL (env_dflt_dots) * 100, 1,
						   SFI_PARAM_STANDARD |
						   SFI_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_RELEASE_TIME,
			      bse_param_spec_uint ("release_time", "Release Time", NULL,
						   0, BSE_MAX_ENV_TIME,
						   ENV_RELEASE_TIME (env_dflt_dots) * BSE_MAX_ENV_TIME,
						   BSE_STP_ENV_TIME,
						   SFI_PARAM_STANDARD |
						   SFI_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "Envelope",
			      PARAM_DURATION,
			      bse_param_spec_uint ("duration", "Duration [ms]", NULL,
						   0, BSE_MAX_ENV_TIME * 5,
						   BSE_MAX_ENV_TIME,
						   BSE_STP_ENV_TIME * 5,
						   SFI_PARAM_GUI |
						   SFI_PARAM_HINT_SCALE));
}

static void
bse_instrument_init (BseInstrument *instrument)
{
  instrument->type = BSE_INSTRUMENT_NONE;
  instrument->wave = NULL;
  instrument->user_snet = NULL;
  instrument->seq_snet = NULL;
  
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
  
  instrument_set_synth_type (instrument, BSE_INSTRUMENT_STANDARD_PIANO);
}

static void
bse_instrument_do_destroy (BseObject *object)
{
  BseInstrument *instrument = BSE_INSTRUMENT (object);
  
  instrument_set_synth_type (instrument, BSE_INSTRUMENT_NONE);
  g_object_set (instrument,
		"wave", NULL,
		"user_snet", NULL,
		NULL);
  
  /* automatically uncrossed: */
  g_assert (instrument->wave == NULL);
  g_assert (instrument->user_snet == NULL);
  g_assert (instrument->seq_snet == NULL);
  
  /* chain parent class' handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
notify_wave_changed (BseInstrument *instrument)
{
  g_object_notify (G_OBJECT (instrument), "wave");
}

static void
notify_user_snet_changed (BseInstrument *instrument)
{
  g_object_notify (G_OBJECT (instrument), "user_snet");
}

static void
wave_uncross (BseItem *owner,
	      BseItem *ref_item)
{
  BseInstrument *instrument = BSE_INSTRUMENT (owner);
  
  g_object_disconnect (instrument->wave,
		       "any_signal", notify_wave_changed, instrument,
		       NULL);
  instrument->wave = NULL;
  g_object_notify (G_OBJECT (instrument), "wave");
}

static void
user_snet_uncross (BseItem *owner,
		   BseItem *ref_item)
{
  BseInstrument *instrument = BSE_INSTRUMENT (owner);
  
  g_object_disconnect (instrument->user_snet,
		       "any_signal", notify_user_snet_changed, instrument,
		       NULL);
  instrument->user_snet = NULL;
  g_object_notify (G_OBJECT (instrument), "user_snet");
}

static void
bse_instrument_set_property (GObject      *object,
			     guint         param_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
  BseInstrument *instrument = BSE_INSTRUMENT (object);
  BseEnvelope *env = &instrument->env;
  
  switch (param_id)
    {
      guint total;
    case PARAM_SYNTH_TYPE:
      instrument_set_synth_type (instrument, g_value_get_enum (value));
      break;
    case PARAM_WAVE:
      if (instrument->wave)
	{
	  bse_item_uncross (BSE_ITEM (instrument), BSE_ITEM (instrument->wave)); // FIXME: use uncross_ref
	  g_assert (instrument->wave == NULL);
	}
      instrument->wave = bse_value_get_object (value);
      if (instrument->wave)
	{
	  bse_item_cross_link (BSE_ITEM (instrument), BSE_ITEM (instrument->wave), wave_uncross);
	  g_object_connect (instrument->wave,
			    "swapped_signal::notify::name", notify_wave_changed, instrument,
			    NULL);
	}
      break;
    case PARAM_SYNTH_NET:
      if (instrument->user_snet)
	{
	  bse_item_uncross (BSE_ITEM (instrument), BSE_ITEM (instrument->user_snet));
	  g_assert (instrument->user_snet == NULL);
	}
      instrument->user_snet = bse_value_get_object (value);
      if (instrument->user_snet)
	{
	  bse_item_cross_link (BSE_ITEM (instrument), BSE_ITEM (instrument->user_snet), user_snet_uncross);
	  g_object_connect (instrument->user_snet,
			    "swapped_signal::notify::name", notify_user_snet_changed, instrument,
			    NULL);
	}
      break;
    case PARAM_VOLUME_f:
      instrument->volume_factor = sfi_value_get_real (value);
      g_object_notify (instrument, "volume_dB");
      g_object_notify (instrument, "volume_perc");
      break;
    case PARAM_VOLUME_dB:
      instrument->volume_factor = bse_dB_to_factor (sfi_value_get_real (value));
      g_object_notify (instrument, "volume_f");
      g_object_notify (instrument, "volume_perc");
      break;
    case PARAM_VOLUME_PERC:
      instrument->volume_factor = ((gfloat) sfi_value_get_int (value)) / 100;
      g_object_notify (instrument, "volume_f");
      g_object_notify (instrument, "volume_dB");
      break;
    case PARAM_BALANCE:
      instrument->balance = sfi_value_get_int (value);
      break;
    case PARAM_TRANSPOSE:
      instrument->transpose = sfi_value_get_int (value);
      break;
    case PARAM_FINE_TUNE:
      instrument->fine_tune = sfi_value_get_int (value);
      break;
    case PARAM_ATTACK_LEVEL:
      env->attack_level = ((gfloat) sfi_value_get_int (value)) / 100;
      g_object_notify (instrument, "envelope");
      break;
    case PARAM_SUSTAIN_LEVEL:
      env->sustain_level = ((gfloat) sfi_value_get_int (value)) / 100;
      g_object_notify (instrument, "envelope");
      break;
    case PARAM_RELEASE_LEVEL:
      env->release_level = ((gfloat) sfi_value_get_int (value)) / 100;
      g_object_notify (instrument, "envelope");
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
	  g_object_notify (instrument, "attack_level");
	  env->sustain_level = ENV_SUSTAIN_LEVEL (dots);
	  g_object_notify (instrument, "sustain_level");
	  env->release_level = ENV_RELEASE_LEVEL (dots);
	  g_object_notify (instrument, "release_level");
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
	  g_object_notify (instrument, "delay_time");
	  g_object_notify (instrument, "attack_time");
	  g_object_notify (instrument, "decay_time");
	  g_object_notify (instrument, "sustain_time");
	  g_object_notify (instrument, "release_time");
	  g_object_notify (instrument, "duration");
	}
      break;
    case PARAM_DELAY_TIME:
      env->delay_time = sfi_value_get_int (value);
      g_object_notify (instrument, "envelope");
      g_object_notify (instrument, "duration");
      break;
    case PARAM_ATTACK_TIME:
      env->attack_time = sfi_value_get_int (value);
      g_object_notify (instrument, "envelope");
      g_object_notify (instrument, "duration");
      break;
    case PARAM_SUSTAIN_TIME:
      env->sustain_time = sfi_value_get_int (value);
      g_object_notify (instrument, "envelope");
      g_object_notify (instrument, "duration");
      break;
    case PARAM_DECAY_TIME:
      env->decay_time = sfi_value_get_int (value);
      g_object_notify (instrument, "envelope");
      g_object_notify (instrument, "duration");
      break;
    case PARAM_RELEASE_TIME:
      env->release_time = sfi_value_get_int (value);
      g_object_notify (instrument, "envelope");
      g_object_notify (instrument, "duration");
      break;
    case PARAM_DURATION:
      total = ENVELOPE_TIME_TOTAL (instrument->env);
      if (env->delay_time)
	env->delay_time = CLAMP (((gfloat) env->delay_time) /
				 total * sfi_value_get_int (value) + 0.5,
				 1, BSE_MAX_ENV_TIME);
      g_object_notify (instrument, "delay_time");
      if (env->attack_time)
	env->attack_time = CLAMP (((gfloat) env->attack_time) /
				  total * sfi_value_get_int (value) + 0.5,
				  1, BSE_MAX_ENV_TIME);
      g_object_notify (instrument, "attack_time");
      if (env->decay_time)
	env->decay_time = CLAMP (((gfloat) env->decay_time) /
				 total * sfi_value_get_int (value) + 0.5,
				 1, BSE_MAX_ENV_TIME);
      g_object_notify (instrument, "decay_time");
      if (env->sustain_time)
	env->sustain_time = CLAMP (((gfloat) env->sustain_time) /
				   total * sfi_value_get_int (value) + 0.5,
				   1, BSE_MAX_ENV_TIME);
      g_object_notify (instrument, "sustain_time");
      if (env->release_time)
	env->release_time = CLAMP (((gfloat) env->release_time) /
				   total * sfi_value_get_int (value) + 0.5,
				   1, BSE_MAX_ENV_TIME);
      g_object_notify (instrument, "release_time");
      g_object_notify (instrument, "envelope");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (instrument, param_id, pspec);
      break;
    }
}

static void
bse_instrument_get_property (GObject    *object,
			     guint       param_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
  BseInstrument *instrument = BSE_INSTRUMENT (object);
  BseEnvelope *env = &instrument->env;
  
  switch (param_id)
    {
      BseDot dots[ENV_N_DOTS];
      guint total;
    case PARAM_SYNTH_TYPE:
      g_value_set_enum (value, instrument->type);
      break;
    case PARAM_WAVE:
      bse_value_set_object (value, instrument->wave);
      break;
    case PARAM_SYNTH_NET:
      bse_value_set_object (value, instrument->user_snet);
      break;
    case PARAM_SEQ_SYNTH:
      bse_value_set_object (value, instrument->seq_snet);
      break;
    case PARAM_VOLUME_f:
      sfi_value_set_real (value, instrument->volume_factor);
      break;
    case PARAM_VOLUME_dB:
      sfi_value_set_real (value, bse_dB_from_factor (instrument->volume_factor, BSE_MIN_VOLUME_dB));
      break;
    case PARAM_VOLUME_PERC:
      sfi_value_set_int (value, instrument->volume_factor * ((gfloat) 100) + 0.5);
      break;
    case PARAM_BALANCE:
      sfi_value_set_int (value, instrument->balance);
      break;
    case PARAM_TRANSPOSE:
      sfi_value_set_int (value, instrument->transpose);
      break;
    case PARAM_FINE_TUNE:
      sfi_value_set_int (value, instrument->fine_tune);
      break;
    case PARAM_ATTACK_LEVEL:
      sfi_value_set_int (value, env->attack_level * 100);
      break;
    case PARAM_SUSTAIN_LEVEL:
      sfi_value_set_int (value, env->sustain_level * 100);
      break;
    case PARAM_RELEASE_LEVEL:
      sfi_value_set_int (value, env->release_level * 100);
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
      sfi_value_set_int (value, env->delay_time);
      break;
    case PARAM_ATTACK_TIME:
      sfi_value_set_int (value, env->attack_time);
      break;
    case PARAM_DECAY_TIME:
      sfi_value_set_int (value, env->decay_time);
      break;
    case PARAM_SUSTAIN_TIME:
      sfi_value_set_int (value, env->sustain_time);
      break;
    case PARAM_RELEASE_TIME:
      sfi_value_set_int (value, env->release_time);
      break;
    case PARAM_DURATION:
      sfi_value_set_int (value, ENVELOPE_TIME_TOTAL (instrument->env));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (instrument, param_id, pspec);
      break;
    }
}

static void
instrument_set_synth_type (BseInstrument    *instrument,
			   BseInstrumentType type)
{
  BseProject *project = bse_item_get_project (BSE_ITEM (instrument));
  
  if (instrument->seq_snet)
    {
      BSE_SEQUENCER_LOCK ();
      instrument->seq_snet = NULL;
      BSE_SEQUENCER_UNLOCK ();
    }
  instrument->type = type;
  if (project)
    {
      switch (instrument->type)
	{
	  BseStandardSynth *synth;
	case BSE_INSTRUMENT_STANDARD_PIANO:
	  synth = bse_project_standard_piano (project);
	  BSE_SEQUENCER_LOCK ();
	  instrument->seq_snet = synth->snet;
	  BSE_SEQUENCER_UNLOCK ();
	  break;
	default:
	  break;
	}
    }
  g_object_notify (G_OBJECT (instrument), "synth_type");
  g_object_notify (G_OBJECT (instrument), "seq_snet");
}
