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
#include        "bsemidisynthinput.h"
#include        "bsesubsynth.h"
#include        "bsepcmoutput.h"
#include        <string.h>
#include        <time.h>
#include        <fcntl.h>
#include        <unistd.h>

#include        "./icons/snet.c"


#define	BSE_MIXER_OCHANNEL_MONO		0		// FIXME: hackage!!

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
static void	bse_midi_synth_set_property	(BseMidiSynth		*msynth,
						 guint			 param_id,
						 GValue			*value,
						 GParamSpec		*pspec);
static void	bse_midi_synth_get_property	(BseMidiSynth		*msynth,
						 guint			 param_id,
						 GValue			*value,
						 GParamSpec		*pspec);
static void	midi_synth_set_n_voices		(BseMidiSynth		*msynth,
						 guint			 n_voices);


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
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = (GObjectSetPropertyFunc) bse_midi_synth_set_property;
  gobject_class->get_property = (GObjectGetPropertyFunc) bse_midi_synth_get_property;

  object_class->destroy = bse_midi_synth_destroy;
  
  bse_object_class_add_param (object_class, "MIDI Instrument",
			      PARAM_MIDI_CHANNEL,
			      bse_param_spec_uint ("midi_channel", "MIDI Channel", NULL,
						   1, BSE_MIDI_MAX_CHANNELS,
						   1, 1,
						   BSE_PARAM_GUI | BSE_PARAM_STORAGE | BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "MIDI Instrument",
			      PARAM_N_VOICES,
			      bse_param_spec_uint ("n_voices", "Max # Voixes", NULL,
						   1, 4,
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
bse_midi_synth_init (BseMidiSynth *msynth)
{
  BseErrorType error;

  BSE_OBJECT_UNSET_FLAGS (msynth, BSE_SNET_FLAG_USER_SYNTH);
  BSE_SUPER (msynth)->auto_activate = TRUE;
  msynth->midi_channel_id = 1;
  msynth->n_voices = 0;
  msynth->volume_factor = bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB);
  msynth->snet = NULL;

  /* left channel multiport */
  msynth->lmixer = g_object_new (g_type_from_name ("BseMixer"), "uname", "left-mixer", NULL);
  BSE_OBJECT_SET_FLAGS (msynth->lmixer, BSE_ITEM_FLAG_STORAGE_IGNORE);
  bse_container_add_item (BSE_CONTAINER (msynth), BSE_ITEM (msynth->lmixer));
  g_object_unref (msynth->lmixer);

  /* right channel multiport */
  msynth->rmixer = g_object_new (g_type_from_name ("BseMixer"), "uname", "right-mixer", NULL);
  BSE_OBJECT_SET_FLAGS (msynth->rmixer, BSE_ITEM_FLAG_STORAGE_IGNORE);
  bse_container_add_item (BSE_CONTAINER (msynth), BSE_ITEM (msynth->rmixer));
  g_object_unref (msynth->rmixer);

  /* pcm output */
  msynth->pcm_out = g_object_new (g_type_from_name ("BsePcmOutput"),
				  "uname", "pcm-output",
				  "master_volume_f", msynth->volume_factor,
				  NULL);
  BSE_OBJECT_SET_FLAGS (msynth->pcm_out, BSE_ITEM_FLAG_STORAGE_IGNORE);
  bse_container_add_item (BSE_CONTAINER (msynth), BSE_ITEM (msynth->pcm_out));
  g_object_unref (msynth->pcm_out);

  /* multiport <-> pcm output connections */
  error = bse_source_set_input (msynth->pcm_out, BSE_PCM_OUTPUT_ICHANNEL_LEFT,
				msynth->lmixer, BSE_MIXER_OCHANNEL_MONO);
  g_assert (error == BSE_ERROR_NONE);
  error = bse_source_set_input (msynth->pcm_out, BSE_PCM_OUTPUT_ICHANNEL_RIGHT,
				msynth->rmixer, BSE_MIXER_OCHANNEL_MONO);
  g_assert (error == BSE_ERROR_NONE);
  
  midi_synth_set_n_voices (msynth, 1);
}

static void
midi_synth_set_n_voices (BseMidiSynth *msynth,
			 guint         n_voices)
{
  BseErrorType error;
  guint i, org_voices;
  GSList *slist = NULL;

  org_voices = msynth->n_voices;

  msynth->n_voices = MIN (n_voices, msynth->n_voices);
  for (i = msynth->n_voices; i < org_voices; i++)
    {
      BseMidiSynthVoice *voice = msynth->voices + i;

      slist = g_slist_prepend (slist, voice->midi_synth_input);
      slist = g_slist_prepend (slist, voice->sub_synth);
    }
  while (slist)
    {
      GSList *tmp = slist->next;

      bse_container_remove_item (BSE_CONTAINER (msynth), slist->data);
      g_slist_free_1 (slist);
      slist = tmp;
    }

  msynth->n_voices = n_voices;
  msynth->voices = g_renew (BseMidiSynthVoice, msynth->voices,  msynth->n_voices);
  for (i = org_voices; i < msynth->n_voices; i++)
    {
      BseMidiSynthVoice *voice = msynth->voices + i;

      voice->midi_synth_input = g_object_new (BSE_TYPE_MIDI_SYNTH_INPUT, NULL);
      bse_midi_synth_input_set_params (BSE_MIDI_SYNTH_INPUT (voice->midi_synth_input), msynth->midi_channel_id, i);
      BSE_OBJECT_SET_FLAGS (voice->midi_synth_input, BSE_ITEM_FLAG_STORAGE_IGNORE);
      bse_container_add_item (BSE_CONTAINER (msynth), BSE_ITEM (voice->midi_synth_input));
      g_object_unref (voice->midi_synth_input);
      
      voice->sub_synth = g_object_new (BSE_TYPE_SUB_SYNTH,
				       "in_port_1", "frequency",
				       "in_port_2", "gate",
				       "in_port_3", "velocity",
				       "in_port_4", "aftertouch",
				       "out_port_1", "left_out",
				       "out_port_2", "right_out",
				       "out_port_3", "unused",
				       "out_port_4", "synth_done",
				       "snet", msynth->snet,
				       NULL);
      BSE_OBJECT_SET_FLAGS (voice->sub_synth, BSE_ITEM_FLAG_STORAGE_IGNORE);
      bse_container_add_item (BSE_CONTAINER (msynth), BSE_ITEM (voice->sub_synth));
      g_object_unref (voice->sub_synth);

      error = bse_source_set_input (voice->sub_synth, 0,
				    voice->midi_synth_input, BSE_MIDI_SYNTH_INPUT_OCHANNEL_FREQUENCY);
      g_assert (error == BSE_ERROR_NONE);
      error = bse_source_set_input (voice->sub_synth, 1,
				    voice->midi_synth_input, BSE_MIDI_SYNTH_INPUT_OCHANNEL_GATE);
      g_assert (error == BSE_ERROR_NONE);
      error = bse_source_set_input (voice->sub_synth, 2,
				    voice->midi_synth_input, BSE_MIDI_SYNTH_INPUT_OCHANNEL_VELOCITY);
      g_assert (error == BSE_ERROR_NONE);
      error = bse_source_set_input (voice->sub_synth, 3,
				    voice->midi_synth_input, BSE_MIDI_SYNTH_INPUT_OCHANNEL_AFTERTOUCH);
      g_assert (error == BSE_ERROR_NONE);

      error = bse_source_set_input (msynth->lmixer, i, voice->sub_synth, 0);
      error = bse_source_set_input (msynth->rmixer, i, voice->sub_synth, 1);
      g_assert (error == BSE_ERROR_NONE);
    }
}

static void
bse_midi_synth_destroy (BseObject *object)
{
  BseMidiSynth *msynth = BSE_MIDI_SYNTH (object);

  midi_synth_set_n_voices (msynth, 0);

  if (msynth->snet)
    {
      g_object_unref (msynth->snet);
      msynth->snet = NULL;
    }

  bse_container_remove_item (BSE_CONTAINER (msynth), BSE_ITEM (msynth->lmixer));
  bse_container_remove_item (BSE_CONTAINER (msynth), BSE_ITEM (msynth->rmixer));
  bse_container_remove_item (BSE_CONTAINER (msynth), BSE_ITEM (msynth->pcm_out));
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_midi_synth_set_property (BseMidiSynth *msynth,
			     guint         param_id,
			     GValue       *value,
			     GParamSpec   *pspec)
{
  switch (param_id)
    {
      guint i;
    case PARAM_SNET:
      if (msynth->snet)
	g_object_unref (msynth->snet);
      msynth->snet = g_value_get_object (value);
      if (msynth->snet)
	g_object_ref (msynth->snet);
      for (i = 0; i < msynth->n_voices; i++)
	{
	  BseMidiSynthVoice *voice = msynth->voices + i;

	  g_object_set (voice->sub_synth, "snet", msynth->snet, NULL);
	}
      break;
    case PARAM_MIDI_CHANNEL:
      if (!BSE_SOURCE_PREPARED (msynth))	/* midi channel is locked while prepared */
	{
	  msynth->midi_channel_id = g_value_get_uint (value);
	  for (i = 0; i < msynth->n_voices; i++)
	    {
	      BseMidiSynthVoice *voice = msynth->voices + i;
	      
	      bse_midi_synth_input_set_params (BSE_MIDI_SYNTH_INPUT (voice->midi_synth_input), msynth->midi_channel_id, i);
	    }
	}
      break;
    case PARAM_N_VOICES:
      midi_synth_set_n_voices (msynth, g_value_get_uint (value));
      break;
    case PARAM_VOLUME_f:
      msynth->volume_factor = g_value_get_float (value);
      g_object_set (msynth->pcm_out, "master_volume_f", msynth->volume_factor, NULL);
      bse_object_param_changed (BSE_OBJECT (msynth), "volume_dB");
      bse_object_param_changed (BSE_OBJECT (msynth), "volume_perc");
      break;
    case PARAM_VOLUME_dB:
      msynth->volume_factor = bse_dB_to_factor (g_value_get_float (value));
      g_object_set (msynth->pcm_out, "master_volume_f", msynth->volume_factor, NULL);
      bse_object_param_changed (BSE_OBJECT (msynth), "volume_f");
      bse_object_param_changed (BSE_OBJECT (msynth), "volume_perc");
      break;
    case PARAM_VOLUME_PERC:
      msynth->volume_factor = g_value_get_uint (value) / 100.0;
      g_object_set (msynth->pcm_out, "master_volume_f", msynth->volume_factor, NULL);
      bse_object_param_changed (BSE_OBJECT (msynth), "volume_f");
      bse_object_param_changed (BSE_OBJECT (msynth), "volume_dB");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (msynth, param_id, pspec);
      break;
    }
}

static void
bse_midi_synth_get_property (BseMidiSynth *msynth,
			     guint         param_id,
			     GValue       *value,
			     GParamSpec   *pspec)
{
  switch (param_id)
    {
    case PARAM_SNET:
      g_value_set_object (value, msynth->snet);
      break;
    case PARAM_MIDI_CHANNEL:
      g_value_set_uint (value, msynth->midi_channel_id);
      break;
    case PARAM_N_VOICES:
      g_value_set_uint (value, msynth->n_voices);
      break;
    case PARAM_VOLUME_f:
      g_value_set_float (value, msynth->volume_factor);
      break;
    case PARAM_VOLUME_dB:
      g_value_set_float (value, bse_dB_from_factor (msynth->volume_factor, BSE_MIN_VOLUME_dB));
      break;
    case PARAM_VOLUME_PERC:
      g_value_set_uint (value, msynth->volume_factor * 100.0 + 0.5);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (msynth, param_id, pspec);
      break;
    }
}
