/* BseAmplifier - BSE Amplifier
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bseamplifier.h"

#include <bse/bsecategories.h>
#include <bse/gslengine.h>
#include <bse/gslsignal.h>


/* --- parameters --- */
enum
{
  PROP_0,
  PROP_CTRL_MUL,
  PROP_CTRL_BALANCE,
  PROP_CTRL_STRENGTH_f,
  PROP_CTRL_STRENGTH_dB,
  PROP_CTRL_STRENGTH_PERC,
  PROP_CTRL_EXPONENTIAL,
  PROP_AUDIO_BALANCE,
  PROP_GAIN_f,
  PROP_GAIN_dB,
  PROP_GAIN_PERC,
  PROP_MASTER_f,
  PROP_MASTER_dB,
  PROP_MASTER_PERC
};


/* --- prototypes --- */
static void	 bse_amplifier_init		(BseAmplifier		*self);
static void	 bse_amplifier_class_init	(BseAmplifierClass	*class);
static void	 bse_amplifier_set_property	(GObject		*object,
						 guint			 param_id,
						 const GValue		*value,
						 GParamSpec		*pspec);
static void	 bse_amplifier_get_property	(GObject		*object,
						 guint		         param_id,
						 GValue		        *value,
						 GParamSpec		*pspec);
static void	 bse_amplifier_context_create	(BseSource		*source,
						 guint		         context_handle,
						 GslTrans		*trans);
static void	 bse_amplifier_update_modules	(BseAmplifier		*self,
						 GslTrans		*trans);


/* --- variables --- */
static gpointer		 parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseAmplifier)
{
  static const GTypeInfo type_info = {
    sizeof (BseAmplifierClass),

    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_amplifier_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,

    sizeof (BseAmplifier),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_amplifier_init,
  };
#include "./icons/ampctrl.c"
  static const BsePixdata pixdata = {
    AMP_CTRL_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
    AMP_CTRL_IMAGE_WIDTH, AMP_CTRL_IMAGE_HEIGHT,
    AMP_CTRL_IMAGE_RLE_PIXEL_DATA,
  };
  GType type;

  type = bse_type_register_static (BSE_TYPE_SOURCE,
				   "BseAmplifier",
				   "BseAmplifier provides input signal controlled amplification or "
				   "attenuation. "
				   "It supports two control inputs and two audio inputs which "
				   "are mixed together according to a balance setting for each. "
				   "The gain setting controls preamplification of the mixed audio "
				   "signal. The mixed control signal, weighted by a strength "
				   "setting, determines additional amplification, allowing for "
				   "external sources to modulate the overall volume (tremolo). "
				   "The mixed control signal can influence the amplification "
				   "linearly (to amplify other control signals) or exponentially "
				   "(to amplify audio signals). "
				   "Finally, the master volume controls amplification of the "
				   "resulting output signal.",
				   &type_info);
  bse_categories_register_icon ("/Modules/Routing/Amplifier",
				type, &pixdata);
  return type;
}

static void
bse_amplifier_class_init (BseAmplifierClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint channel;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_amplifier_set_property;
  gobject_class->get_property = bse_amplifier_get_property;
  
  source_class->context_create = bse_amplifier_context_create;
  
  bse_object_class_add_param (object_class, "Control Input",
			      PROP_CTRL_MUL,
			      bse_param_spec_boolean ("ctrl_mul", "Multiply Controls", "Multiply the two control inputs with each other, rather than weighting them by balance",
						      FALSE,
						      BSE_PARAM_DEFAULT | BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Control Input",
			      PROP_CTRL_BALANCE,
			      bse_param_spec_float ("ctrl_balance", "Control Balance", "Determine balance of the two control inputs",
						    -BSE_MAX_BALANCE_f, BSE_MAX_BALANCE_f,
						    0, BSE_STP_BALANCE_f,
						    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Control Input",
			      PROP_CTRL_STRENGTH_f,
			      bse_param_spec_float ("ctrl_strength_f", "Control Strength [float]", NULL,
						    0, 1.0, 0.5, 0.01,
						    BSE_PARAM_STORAGE | BSE_PARAM_FORCE_DIRTY));
  bse_object_class_add_param (object_class, "Control Input",
			      PROP_CTRL_STRENGTH_dB,
			      bse_param_spec_float ("ctrl_strength_dB", "Strength [dB]", "Amount of impact of the control inputs",
						    bse_dB_from_factor (0, BSE_MIN_VOLUME_dB), bse_dB_from_factor (1.0, BSE_MIN_VOLUME_dB),
						    bse_dB_from_factor (0.5, BSE_MIN_VOLUME_dB), 0.1,
						    BSE_PARAM_GUI | BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Control Input",
			      PROP_CTRL_STRENGTH_PERC,
			      bse_param_spec_float ("ctrl_strength_perc", "Strength [%]", "Amount of impact of the control inputs",
						    0, 100, 50.0, 1,
						    BSE_PARAM_GUI | BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Control Input",
			      PROP_CTRL_EXPONENTIAL,
			      bse_param_spec_boolean ("ctrl_exp", "Exponential Control", "Toggle exponential and linear control response",
						      TRUE,
						      BSE_PARAM_DEFAULT | BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Audio Input",
			      PROP_AUDIO_BALANCE,
			      bse_param_spec_float ("audio_balance", "Audio Balance", "Determine balance of the two audio inputs",
						    -BSE_MAX_BALANCE_f, BSE_MAX_BALANCE_f,
						    0, BSE_STP_BALANCE_f,
						    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Audio Input",
			      PROP_GAIN_f,
			      bse_param_spec_float ("audio_gain_f", "Gain [float]", NULL,
						    0, 1.0, 0.5, 0.01,
						    BSE_PARAM_STORAGE | BSE_PARAM_FORCE_DIRTY));
  bse_object_class_add_param (object_class, "Audio Input",
			      PROP_GAIN_dB,
			      bse_param_spec_float ("audio_gain_dB", "Gain [dB]", "Base amplification (the control signal adds up to this)",
						    bse_dB_from_factor (0, BSE_MIN_VOLUME_dB), bse_dB_from_factor (1.0, BSE_MIN_VOLUME_dB),
						    bse_dB_from_factor (0.5, BSE_MIN_VOLUME_dB), 0.1,
						    BSE_PARAM_GUI | BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Audio Input",
			      PROP_GAIN_PERC,
			      bse_param_spec_float ("audio_gain_perc", "Gain [%]", "Base amplification (the control signal adds up to this)",
						    0, 100, 50.0, 1,
						    BSE_PARAM_GUI | BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Output",
			      PROP_MASTER_f,
			      bse_param_spec_float ("master_gain_f", "Master [float]", NULL,
						    0, 1.0, 1.0, 0.01,
						    BSE_PARAM_STORAGE | BSE_PARAM_FORCE_DIRTY));
  bse_object_class_add_param (object_class, "Output",
			      PROP_MASTER_dB,
			      bse_param_spec_float ("master_gain_dB", "Master [dB]", "Output stage amplification",
						    bse_dB_from_factor (0, BSE_MIN_VOLUME_dB), bse_dB_from_factor (1.0, BSE_MIN_VOLUME_dB),
						    bse_dB_from_factor (1.0, BSE_MIN_VOLUME_dB), 0.1,
						    BSE_PARAM_GUI | BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Output",
			      PROP_MASTER_PERC,
			      bse_param_spec_float ("master_gain_perc", "Master [%]", "Output stage amplification",
						    0, 100, 100, 1,
						    BSE_PARAM_GUI | BSE_PARAM_HINT_DIAL));

  channel = bse_source_class_add_ichannel (source_class, "control1", "Control Input 1");
  channel = bse_source_class_add_ichannel (source_class, "control2", "Control Input 2");
  channel = bse_source_class_add_ichannel (source_class, "audio1", "Audio Input 1");
  channel = bse_source_class_add_ichannel (source_class, "audio2", "Audio Input 2");
  channel = bse_source_class_add_ochannel (source_class, "audio out", "Amplified Audio Output");
}

static void
bse_amplifier_init (BseAmplifier *self)
{
  self->config.ctrl_balance = 0.5;
  self->config.ctrl_strength = 0.5;
  self->config.audio_balance = 0.5;
  self->config.audio_gain = 0.5;
  self->config.master_gain = 1.0;
  self->config.ctrl_mul = FALSE;
  self->config.exp_ctrl = TRUE;
}

static void
bse_amplifier_set_property (GObject      *object,
			    guint         param_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
  BseAmplifier *self = BSE_AMPLIFIER (object);

  switch (param_id)
    {
    case PROP_CTRL_MUL:
      self->config.ctrl_mul = g_value_get_boolean (value);
      bse_amplifier_update_modules (self, NULL);
      break;
    case PROP_CTRL_BALANCE:
      self->config.ctrl_balance = g_value_get_float (value) / (BSE_MAX_BALANCE_f * 2) + 0.5;
      bse_amplifier_update_modules (self, NULL);
      break;
    case PROP_CTRL_STRENGTH_f:
      self->config.ctrl_strength = g_value_get_float (value);
      bse_amplifier_update_modules (self, NULL);
      g_object_notify (object, "ctrl_strength_dB");
      g_object_notify (object, "ctrl_strength_perc");
      break;
    case PROP_CTRL_STRENGTH_dB:
      self->config.ctrl_strength = bse_dB_to_factor (g_value_get_float (value));
      bse_amplifier_update_modules (self, NULL);
      g_object_notify (object, "ctrl_strength_f");
      g_object_notify (object, "ctrl_strength_perc");
      break;
    case PROP_CTRL_STRENGTH_PERC:
      self->config.ctrl_strength = g_value_get_float (value) / 100.0;
      bse_amplifier_update_modules (self, NULL);
      g_object_notify (object, "ctrl_strength_f");
      g_object_notify (object, "ctrl_strength_dB");
      break;
    case PROP_CTRL_EXPONENTIAL:
      self->config.exp_ctrl = g_value_get_boolean (value);
      bse_amplifier_update_modules (self, NULL);
      break;
    case PROP_AUDIO_BALANCE:
      self->config.audio_balance = g_value_get_float (value) / (BSE_MAX_BALANCE_f * 2) + 0.5;
      bse_amplifier_update_modules (self, NULL);
      break;
    case PROP_GAIN_f:
      self->config.audio_gain = g_value_get_float (value);
      bse_amplifier_update_modules (self, NULL);
      g_object_notify (object, "audio_gain_dB");
      g_object_notify (object, "audio_gain_perc");
      break;
    case PROP_GAIN_dB:
      self->config.audio_gain = bse_dB_to_factor (g_value_get_float (value));
      bse_amplifier_update_modules (self, NULL);
      g_object_notify (object, "audio_gain_f");
      g_object_notify (object, "audio_gain_perc");
      break;
    case PROP_GAIN_PERC:
      self->config.audio_gain = g_value_get_float (value) / 100.0;
      bse_amplifier_update_modules (self, NULL);
      g_object_notify (object, "audio_gain_f");
      g_object_notify (object, "audio_gain_dB");
      break;
    case PROP_MASTER_f:
      self->config.master_gain = g_value_get_float (value);
      bse_amplifier_update_modules (self, NULL);
      g_object_notify (object, "master_gain_dB");
      g_object_notify (object, "master_gain_perc");
      break;
    case PROP_MASTER_dB:
      self->config.master_gain = bse_dB_to_factor (g_value_get_float (value));
      bse_amplifier_update_modules (self, NULL);
      g_object_notify (object, "master_gain_f");
      g_object_notify (object, "master_gain_perc");
      break;
    case PROP_MASTER_PERC:
      self->config.master_gain = g_value_get_float (value) / 100.0;
      bse_amplifier_update_modules (self, NULL);
      g_object_notify (object, "master_gain_f");
      g_object_notify (object, "master_gain_dB");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_amplifier_get_property (GObject    *object,
			    guint       param_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
  BseAmplifier *self = BSE_AMPLIFIER (object);

  switch (param_id)
    {
    case PROP_CTRL_MUL:
      g_value_set_boolean (value, self->config.ctrl_mul);
      break;
    case PROP_CTRL_BALANCE:
      g_value_set_float (value, (self->config.ctrl_balance - 0.5) * 2 * BSE_MAX_BALANCE_f);
      break;
    case PROP_CTRL_STRENGTH_f:
      g_value_set_float (value, self->config.ctrl_strength);
      break;
    case PROP_CTRL_STRENGTH_dB:
      g_value_set_float (value, bse_dB_from_factor (self->config.ctrl_strength, BSE_MIN_VOLUME_dB));
      break;
    case PROP_CTRL_STRENGTH_PERC:
      g_value_set_float (value, self->config.ctrl_strength * 100.0);
      break;
    case PROP_CTRL_EXPONENTIAL:
      g_value_set_boolean (value, self->config.exp_ctrl);
      break;
    case PROP_AUDIO_BALANCE:
      g_value_set_float (value, (self->config.audio_balance - 0.5) * 2 * BSE_MAX_BALANCE_f);
      break;
    case PROP_GAIN_f:
      g_value_set_float (value, self->config.audio_gain);
      break;
    case PROP_GAIN_dB:
      g_value_set_float (value, bse_dB_from_factor (self->config.audio_gain, BSE_MIN_VOLUME_dB));
      break;
    case PROP_GAIN_PERC:
      g_value_set_float (value, self->config.audio_gain * 100.0);
      break;
    case PROP_MASTER_f:
      g_value_set_float (value, self->config.master_gain);
      break;
    case PROP_MASTER_dB:
      g_value_set_float (value, bse_dB_from_factor (self->config.master_gain, BSE_MIN_VOLUME_dB));
      break;
    case PROP_MASTER_PERC:
      g_value_set_float (value, self->config.master_gain * 100.0);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

typedef struct
{
  BseAmplifierConfig config;
} Amplifier;

static void
bse_amplifier_update_modules (BseAmplifier *self,
			      GslTrans     *trans)
{
  if (BSE_SOURCE_PREPARED (self))
    bse_source_update_modules (BSE_SOURCE (self),
			       G_STRUCT_OFFSET (Amplifier, config),
			       &self->config,
			       sizeof (self->config),
			       trans);
}


/* --- generate processing function variants --- */
#define AMP_FLAG_MASTER		(1)
#define	AMP_FLAGS_AUDIO		(2 + 4)
#define	AMP_FLAGS_A1n_A2n	(0 << 1)
#define	AMP_FLAGS_A1n_A2y	(1 << 1)
#define	AMP_FLAGS_A1y_A2n	(2 << 1)
#define	AMP_FLAGS_A1b_A2b	(3 << 1)
#define	AMP_FLAGS_CONTROL	(8 + 16)
#define	AMP_FLAGS_C1m_C2m	(0 << 3)
#define	AMP_FLAGS_C1n_C2y	(1 << 3)
#define	AMP_FLAGS_C1y_C2n	(2 << 3)
#define	AMP_FLAGS_C1b_C2b	(3 << 3)
#define	AMP_FLAG_EXP_CONTROLS	(32)
#define	AMP_FLAG_SIMPLE_CONTROL	(64)

/* simple amplifier variants */
#define AMP_INCLUDER_FLAGS	AMP_FLAG_SIMPLE_CONTROL
#define GSL_INCLUDER_FIRST_CASE 0
#define GSL_INCLUDER_LAST_CASE  7
#define	GSL_INCLUDER_REJECT(ic)	((ic & AMP_FLAGS_AUDIO) == AMP_FLAGS_A1n_A2n)
#define GSL_INCLUDER_NAME       amplifier_process_simple
#define GSL_INCLUDER_TABLE      static void (*amp_process_simple_table[]) (Amplifier*,guint, \
                                const gfloat*,const gfloat*,const gfloat*, \
                                const gfloat*,gfloat*)
#define GSL_INCLUDER_FILE       "bseamplifier-aux.c"
#include "gslincluder.c"
#undef  AMP_INCLUDER_FLAGS

/* amplifier variants with control input */
#define AMP_INCLUDER_FLAGS	0
#define GSL_INCLUDER_FIRST_CASE 0
#define GSL_INCLUDER_LAST_CASE  63
#define	GSL_INCLUDER_REJECT(ic)	((ic & AMP_FLAGS_AUDIO) == AMP_FLAGS_A1n_A2n)
#define GSL_INCLUDER_NAME       amplifier_process_control
#define GSL_INCLUDER_TABLE      static void (*amp_process_control_table[]) (Amplifier*,guint, \
                                const gfloat*,const gfloat*,const gfloat*, \
                                const gfloat*,gfloat*)
#define GSL_INCLUDER_FILE       "bseamplifier-aux.c"
#include "gslincluder.c"
#undef  AMP_INCLUDER_FLAGS


static void
amplifier_process (GslModule *module,
		   guint      n_values)
{
  Amplifier *amplifier = module->user_data;
  const gfloat *cv1in = GSL_MODULE_IBUFFER (module, BSE_AMPLIFIER_ICHANNEL_CONTROL1);
  const gfloat *cv2in = GSL_MODULE_IBUFFER (module, BSE_AMPLIFIER_ICHANNEL_CONTROL2);
  const gfloat *au1in = GSL_MODULE_IBUFFER (module, BSE_AMPLIFIER_ICHANNEL_AUDIO1);
  const gfloat *au2in = GSL_MODULE_IBUFFER (module, BSE_AMPLIFIER_ICHANNEL_AUDIO2);
  gfloat *audio_out = GSL_MODULE_OBUFFER (module, BSE_AMPLIFIER_OCHANNEL_AUDIO_OUT);
  guint mode = 0, no_controls = FALSE;

  if (GSL_MODULE_ISTREAM (module, BSE_AMPLIFIER_ICHANNEL_AUDIO1).connected &&
      GSL_MODULE_ISTREAM (module, BSE_AMPLIFIER_ICHANNEL_AUDIO2).connected)
    mode |= AMP_FLAGS_A1b_A2b;
  else if (GSL_MODULE_ISTREAM (module, BSE_AMPLIFIER_ICHANNEL_AUDIO1).connected) /* !audio2 */
    mode |= AMP_FLAGS_A1y_A2n;
  else if (GSL_MODULE_ISTREAM (module, BSE_AMPLIFIER_ICHANNEL_AUDIO2).connected) /* !audio1 */
    mode |= AMP_FLAGS_A1n_A2y;
  else /* !audio1 && !audio2 */
    {
      module->ostreams[BSE_AMPLIFIER_OCHANNEL_AUDIO_OUT].values = gsl_engine_const_values (0);
      return;
    }
  if (amplifier->config.master_gain < 1.0)	//FIXME: epsilon
    mode |= AMP_FLAG_MASTER;
  if (GSL_MODULE_ISTREAM (module, BSE_AMPLIFIER_ICHANNEL_CONTROL1).connected &&
      GSL_MODULE_ISTREAM (module, BSE_AMPLIFIER_ICHANNEL_CONTROL2).connected)
    mode |= amplifier->config.ctrl_mul ? AMP_FLAGS_C1m_C2m : AMP_FLAGS_C1b_C2b;
  else if (GSL_MODULE_ISTREAM (module, BSE_AMPLIFIER_ICHANNEL_CONTROL1).connected) /* !control2 */
    mode |= AMP_FLAGS_C1y_C2n;
  else if (GSL_MODULE_ISTREAM (module, BSE_AMPLIFIER_ICHANNEL_CONTROL2).connected) /* !control1 */
    mode |= AMP_FLAGS_C1n_C2y;
  else /* !control1 && !control2 */
    no_controls = TRUE;
  if (!no_controls && amplifier->config.exp_ctrl)
    mode |= AMP_FLAG_EXP_CONTROLS;

  if (no_controls)
    amp_process_simple_table[mode] (amplifier, n_values,
				    cv1in, cv2in, au1in, au2in,
				    audio_out);
  else
    amp_process_control_table[mode] (amplifier, n_values,
				     cv1in, cv2in, au1in, au2in,
				     audio_out);
}

static void
bse_amplifier_context_create (BseSource *source,
			      guint      context_handle,
			      GslTrans  *trans)
{
  static const GslClass amplifier_class = {
    BSE_AMPLIFIER_N_ICHANNELS,	/* n_istreams */
    0,                          /* n_jstreams */
    BSE_AMPLIFIER_N_OCHANNELS,	/* n_ostreams */
    amplifier_process,		/* process */
    (GslModuleFreeFunc) g_free,	/* free */
    GSL_COST_CHEAP,		/* flags */
  };
  Amplifier *amplifier = g_new0 (Amplifier, 1);
  GslModule *module;
  
  module = gsl_module_new (&amplifier_class, amplifier);
  
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);
  
  /* commit module to engine */
  gsl_trans_add (trans, gsl_job_integrate (module));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
  
  /* update module data */
  bse_amplifier_update_modules (BSE_AMPLIFIER (source), trans);
}
