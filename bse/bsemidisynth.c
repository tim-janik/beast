/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1996-1999, 2000-2001 Tim Janik
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
#include        "bsemidisynth.h"

#include        "bsemidievent.h"	/* BSE_MIDI_MAX_CHANNELS */
#include        "bsemidivoice.h"
#include        "bsecontextmerger.h"
#include        "bsesubsynth.h"
#include        "bsepcmoutput.h"
#include        <string.h>
#include        <time.h>
#include        <fcntl.h>
#include        <unistd.h>

#include        "./icons/snet.c"


/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_MIDI_CHANNEL,
  PARAM_N_VOICES,
  PARAM_SNET,
  PARAM_VOLUME_f,
  PARAM_VOLUME_dB,
  PARAM_VOLUME_PERC,
  PARAM_AUTO_ACTIVATE
};


/* --- prototypes --- */
static void	bse_midi_synth_class_init	(BseMidiSynthClass	*class);
static void	bse_midi_synth_init		(BseMidiSynth		*msynth);
static void	bse_midi_synth_destroy		(BseObject		*object);
static void	bse_midi_synth_set_property	(GObject		*object,
						 guint			 param_id,
						 const GValue		*value,
						 GParamSpec		*pspec);
static void	bse_midi_synth_get_property	(GObject		*msynth,
						 guint			 param_id,
						 GValue			*value,
						 GParamSpec		*pspec);
static void	bse_midi_synth_context_create	(BseSource		*source,
						 guint			 context_handle,
						 GslTrans		*trans);


/* --- variables --- */
static GTypeClass     *parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseMidiSynth)
{
  GType midi_synth_type;

  static const GTypeInfo snet_info = {
    sizeof (BseMidiSynthClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_midi_synth_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseMidiSynth),
    0,
    (GInstanceInitFunc) bse_midi_synth_init,
  };
  
  midi_synth_type = bse_type_register_static (BSE_TYPE_SNET,
					      "BseMidiSynth",
					      "BSE Midi Synthesizer",
					      &snet_info);

  return midi_synth_type;
}

static void
bse_midi_synth_class_init (BseMidiSynthClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_midi_synth_set_property;
  gobject_class->get_property = bse_midi_synth_get_property;

  object_class->destroy = bse_midi_synth_destroy;

  source_class->context_create = bse_midi_synth_context_create;
  
  bse_object_class_add_param (object_class, "MIDI Instrument",
			      PARAM_MIDI_CHANNEL,
			      bse_param_spec_uint ("midi_channel", "MIDI Channel", NULL,
						   1, BSE_MIDI_MAX_CHANNELS,
						   1, 1,
						   BSE_PARAM_GUI | BSE_PARAM_STORAGE | BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "MIDI Instrument",
			      PARAM_N_VOICES,
			      bse_param_spec_uint ("n_voices", "Max # Voixes", NULL,
						   1, 256,
						   1, 1,
						   BSE_PARAM_GUI | BSE_PARAM_STORAGE | BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "MIDI Instrument",
			      PARAM_SNET,
			      g_param_spec_object ("snet", "Synthesis Network", "The MIDI instrument synthesis network",
						   BSE_TYPE_SNET, BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_VOLUME_f,
			      bse_param_spec_float ("volume_f", "Master [float]", NULL,
						    0, bse_dB_to_factor (BSE_MAX_VOLUME_dB),
						    bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB), 0.1,
						    BSE_PARAM_STORAGE));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_VOLUME_dB,
			      bse_param_spec_float ("volume_dB", "Master [dB]", NULL,
						    BSE_MIN_VOLUME_dB, BSE_MAX_VOLUME_dB,
						    BSE_DFL_MASTER_VOLUME_dB, BSE_STP_VOLUME_dB,
						    BSE_PARAM_GUI |
						    BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_VOLUME_PERC,
			      bse_param_spec_uint ("volume_perc", "Master [%]", NULL,
						   0, bse_dB_to_factor (BSE_MAX_VOLUME_dB) * 100,
						   bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB) * 100, 1,
						   BSE_PARAM_GUI |
						   BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Playback Settings",
			      PARAM_AUTO_ACTIVATE,
			      g_param_spec_boolean ("auto_activate", NULL, NULL,
						    TRUE, /* change default */
						    /* override parent property */ 0));
}

static void
bse_midi_synth_init (BseMidiSynth *self)
{
  BseErrorType error;

  BSE_OBJECT_UNSET_FLAGS (self, BSE_SNET_FLAG_USER_SYNTH);
  BSE_SUPER (self)->auto_activate = TRUE;
  self->midi_channel_id = 1;
  self->n_voices = 1;
  self->volume_factor = bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB);

  /* midi voice modules */
  self->voice_input = bse_container_new_item (BSE_CONTAINER (self), BSE_TYPE_MIDI_VOICE_INPUT, NULL);
  BSE_OBJECT_SET_FLAGS (self->voice_input, BSE_ITEM_FLAG_STORAGE_IGNORE);
  self->voice_switch = bse_container_new_item (BSE_CONTAINER (self), BSE_TYPE_MIDI_VOICE_SWITCH, NULL);
  BSE_OBJECT_SET_FLAGS (self->voice_switch, BSE_ITEM_FLAG_STORAGE_IGNORE);
  bse_midi_voice_switch_set_voice_input (BSE_MIDI_VOICE_SWITCH (self->voice_switch), BSE_MIDI_VOICE_INPUT (self->voice_input));
  
  /* context merger */
  self->context_merger = bse_container_new_item (BSE_CONTAINER (self), BSE_TYPE_CONTEXT_MERGER, NULL);
  BSE_OBJECT_SET_FLAGS (self->context_merger, BSE_ITEM_FLAG_STORAGE_IGNORE);

  /* midi voice switch <-> context merger */
  error = bse_source_set_input (self->context_merger, 0,
				self->voice_switch, BSE_MIDI_VOICE_SWITCH_OCHANNEL_LEFT);
  g_assert (error == BSE_ERROR_NONE);
  error = bse_source_set_input (self->context_merger, 1,
				self->voice_switch, BSE_MIDI_VOICE_SWITCH_OCHANNEL_RIGHT);
  g_assert (error == BSE_ERROR_NONE);

  /* output */
  self->output = bse_container_new_item (BSE_CONTAINER (self), BSE_TYPE_PCM_OUTPUT, NULL);
  BSE_OBJECT_SET_FLAGS (self->output, BSE_ITEM_FLAG_STORAGE_IGNORE);

  /* context merger <-> output */
  error = bse_source_set_input (self->output, BSE_PCM_OUTPUT_ICHANNEL_LEFT,
				self->context_merger, 0);
  g_assert (error == BSE_ERROR_NONE);
  error = bse_source_set_input (self->output, BSE_PCM_OUTPUT_ICHANNEL_RIGHT,
				self->context_merger, 1);
  g_assert (error == BSE_ERROR_NONE);

  /* sub synth */
  self->sub_synth = bse_container_new_item (BSE_CONTAINER (self), BSE_TYPE_SUB_SYNTH,
					    "in_port_1", "frequency",
					    "in_port_2", "gate",
					    "in_port_3", "velocity",
					    "in_port_4", "aftertouch",
					    "out_port_1", "left-audio",
					    "out_port_2", "right-audio",
					    "out_port_3", "unused",
					    "out_port_4", "synth-done",
					    NULL);
  BSE_OBJECT_SET_FLAGS (self->sub_synth, BSE_ITEM_FLAG_STORAGE_IGNORE);

  /* voice input <-> sub-synth */
  error = bse_source_set_input (self->sub_synth, 0,
				self->voice_input, BSE_MIDI_VOICE_INPUT_OCHANNEL_FREQUENCY);
  g_assert (error == BSE_ERROR_NONE);
  error = bse_source_set_input (self->sub_synth, 1,
				self->voice_input, BSE_MIDI_VOICE_INPUT_OCHANNEL_GATE);
  g_assert (error == BSE_ERROR_NONE);
  error = bse_source_set_input (self->sub_synth, 2,
				self->voice_input, BSE_MIDI_VOICE_INPUT_OCHANNEL_VELOCITY);
  g_assert (error == BSE_ERROR_NONE);
  error = bse_source_set_input (self->sub_synth, 3,
				self->voice_input, BSE_MIDI_VOICE_INPUT_OCHANNEL_AFTERTOUCH);
  g_assert (error == BSE_ERROR_NONE);

  /* sub-synth <-> voice switch */
  error = bse_source_set_input (self->voice_switch, BSE_MIDI_VOICE_SWITCH_ICHANNEL_LEFT,
				self->sub_synth, 0);
  g_assert (error == BSE_ERROR_NONE);
  error = bse_source_set_input (self->voice_switch, BSE_MIDI_VOICE_SWITCH_ICHANNEL_RIGHT,
				self->sub_synth, 1);
  g_assert (error == BSE_ERROR_NONE);
  error = bse_source_set_input (self->voice_switch, BSE_MIDI_VOICE_SWITCH_ICHANNEL_DISCONNECT,
				self->sub_synth, 3);
  g_assert (error == BSE_ERROR_NONE);
}

static void
bse_midi_synth_destroy (BseObject *object)
{
  BseMidiSynth *self = BSE_MIDI_SYNTH (object);

  bse_container_remove_item (BSE_CONTAINER (self), BSE_ITEM (self->voice_input));
  self->voice_input = NULL;
  bse_container_remove_item (BSE_CONTAINER (self), BSE_ITEM (self->voice_switch));
  self->voice_switch = NULL;
  bse_container_remove_item (BSE_CONTAINER (self), BSE_ITEM (self->context_merger));
  self->context_merger = NULL;
  bse_container_remove_item (BSE_CONTAINER (self), BSE_ITEM (self->output));
  self->output = NULL;
  bse_container_remove_item (BSE_CONTAINER (self), BSE_ITEM (self->sub_synth));
  self->sub_synth = NULL;
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_midi_synth_set_property (GObject      *object,
			     guint         param_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
  BseMidiSynth *self = BSE_MIDI_SYNTH (object);
  switch (param_id)
    {
    case PARAM_SNET:
      g_object_set (self->sub_synth, "snet", g_value_get_object (value), NULL);
      break;
    case PARAM_MIDI_CHANNEL:
      if (!BSE_SOURCE_PREPARED (self))	/* midi channel is locked while prepared */
	self->midi_channel_id = g_value_get_uint (value);
      break;
    case PARAM_N_VOICES:
      if (!BSE_OBJECT_IS_LOCKED (self))
	self->n_voices = g_value_get_uint (value);
      break;
    case PARAM_VOLUME_f:
      self->volume_factor = g_value_get_float (value);
      g_object_set (self->output, "master_volume_f", self->volume_factor, NULL);
      bse_object_param_changed (BSE_OBJECT (self), "volume_dB");
      bse_object_param_changed (BSE_OBJECT (self), "volume_perc");
      break;
    case PARAM_VOLUME_dB:
      self->volume_factor = bse_dB_to_factor (g_value_get_float (value));
      g_object_set (self->output, "master_volume_f", self->volume_factor, NULL);
      bse_object_param_changed (BSE_OBJECT (self), "volume_f");
      bse_object_param_changed (BSE_OBJECT (self), "volume_perc");
      break;
    case PARAM_VOLUME_PERC:
      self->volume_factor = g_value_get_uint (value) / 100.0;
      g_object_set (self->output, "master_volume_f", self->volume_factor, NULL);
      bse_object_param_changed (BSE_OBJECT (self), "volume_f");
      bse_object_param_changed (BSE_OBJECT (self), "volume_dB");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_midi_synth_get_property (GObject    *object,
			     guint       param_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
  BseMidiSynth *self = BSE_MIDI_SYNTH (object);
  switch (param_id)
    {
    case PARAM_SNET:
      g_object_get_property (G_OBJECT (self->sub_synth), "snet", value);
      break;
    case PARAM_MIDI_CHANNEL:
      g_value_set_uint (value, self->midi_channel_id);
      break;
    case PARAM_N_VOICES:
      g_value_set_uint (value, self->n_voices);
      break;
    case PARAM_VOLUME_f:
      g_value_set_float (value, self->volume_factor);
      break;
    case PARAM_VOLUME_dB:
      g_value_set_float (value, bse_dB_from_factor (self->volume_factor, BSE_MIN_VOLUME_dB));
      break;
    case PARAM_VOLUME_PERC:
      g_value_set_uint (value, self->volume_factor * 100.0 + 0.5);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_midi_synth_context_create (BseSource *source,
			       guint      context_handle,
			       GslTrans  *trans)
{
  BseMidiSynth *self = BSE_MIDI_SYNTH (source);
  BseSNet *snet = BSE_SNET (self);

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);

  if (!bse_snet_context_is_branch (snet, context_handle))	/* catch recursion */
    {
      BseMidiReceiver *mrec;
      guint i, midi_channel;

      mrec = bse_snet_get_midi_receiver (snet, context_handle, &midi_channel);
      for (i = 0; i < self->n_voices; i++)
	bse_snet_context_clone_branch (snet, context_handle, self->context_merger, mrec, midi_channel, trans);
    }
}
