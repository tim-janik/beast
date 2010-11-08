/* BseSoundFontOsc - BSE Fluid Synth
 * Copyright (C) 1999-2002 Tim Janik
 * Copyright (C) 2009 Stefan Westerfeld
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "bsesoundfontosc.h"

#include <bse/bsecategories.h>
#include <bse/bseengine.h>
#include <bse/bseproject.h>
#include <bse/bsesoundfontrepo.h>
#include <bse/bsesoundfont.h>
#include <bse/bsesnet.h>
#include <bse/bsemidireceiver.h>
#include "gslcommon.h"

#include <string.h>

/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_PRESET
};


/* --- prototypes --- */
static void	 bse_sound_font_osc_init	  (BseSoundFontOsc	 *sound_font_osc);
static void	 bse_sound_font_osc_class_init	  (BseSoundFontOscClass	 *class);
static void	 bse_sound_font_osc_set_property  (GObject		 *object,
						   guint	          param_id,
						   const GValue		 *value,
						   GParamSpec		 *pspec);
static void	 bse_sound_font_osc_get_property  (GObject		 *object,
						   guint		  param_id,
						   GValue		 *value,
						   GParamSpec		 *pspec);
static void	 bse_sound_font_osc_get_candidates (BseItem		 *item,
						    guint		  param_id,
						    BsePropertyCandidates *pc,
						    GParamSpec		 *pspec);
static void	 bse_sound_font_osc_context_create (BseSource		 *source,
						    guint		  context_handle,
						    BseTrans		 *trans);
static void	 bse_sound_font_osc_context_dismiss (BseSource		 *source,
						     guint		  context_handle,
						     BseTrans		 *trans);
static void	 bse_sound_font_osc_update_modules (BseSoundFontOsc	 *sound_font_osc,
						    BseTrans		 *trans);
static void      bse_sound_font_osc_dispose        (GObject              *object);
static void      bse_sound_font_osc_finalize       (GObject              *object);


/* --- variables --- */
static gpointer	 parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseSoundFontOsc)
{
  static const GTypeInfo type_info = {
    sizeof (BseSoundFontOscClass),

    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_sound_font_osc_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,

    sizeof (BseSoundFontOsc),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_sound_font_osc_init,
  };
  GType type_id;

  type_id = bse_type_register_static (BSE_TYPE_SOURCE,
				      "BseSoundFontOsc",
				      "This internal module wraps fluid synth which plays sound font contents",
                                      __FILE__, __LINE__,
                                      &type_info);
  return type_id;
}

static void
bse_sound_font_osc_class_init (BseSoundFontOscClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  BseItemClass *item_class = BSE_ITEM_CLASS (class);
  guint ochannel;

  parent_class = g_type_class_peek_parent (class);

  gobject_class->set_property = bse_sound_font_osc_set_property;
  gobject_class->get_property = bse_sound_font_osc_get_property;
  gobject_class->finalize = bse_sound_font_osc_finalize;
  gobject_class->dispose = bse_sound_font_osc_dispose;

  item_class->get_candidates = bse_sound_font_osc_get_candidates;

  source_class->context_create = bse_sound_font_osc_context_create;
  source_class->context_dismiss = bse_sound_font_osc_context_dismiss;

  bse_object_class_add_param (object_class, _("Sound Font Preset"),
                              PARAM_PRESET,
                              bse_param_spec_object ("preset", _("Preset"), _("Sound Font Preset to be used during replay"),
                                                     BSE_TYPE_SOUND_FONT_PRESET, SFI_PARAM_STANDARD));

  ochannel = bse_source_class_add_ochannel (source_class, "left-out", _("Left Out"), _("Output of the fluid synth soundfont synthesizer"));
  g_assert (ochannel == BSE_SOUND_FONT_OSC_OCHANNEL_LEFT_OUT);
  ochannel = bse_source_class_add_ochannel (source_class, "right-out", _("Right Out"), _("Output of the fluid synth soundfont synthesizer"));
  g_assert (ochannel == BSE_SOUND_FONT_OSC_OCHANNEL_RIGHT_OUT);
  ochannel = bse_source_class_add_ochannel (source_class, "done-out", _("Done Out"), _("Done Output"));
  g_assert (ochannel == BSE_SOUND_FONT_OSC_OCHANNEL_DONE_OUT);
}

static void
bse_sound_font_osc_init (BseSoundFontOsc *self)
{
  memset (&self->config, 0, sizeof (self->config));
  self->preset = NULL;
}

static void
bse_sound_font_osc_dispose (GObject *object)
{
  BseSoundFontOsc *self = BSE_SOUND_FONT_OSC (object);

  if (self->config.sfrepo)
    {
      bse_sound_font_repo_remove_osc (self->config.sfrepo, self->config.osc_id);

      self->config.sfrepo = NULL;
    }

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
bse_sound_font_osc_finalize (GObject *object)
{
  //BseSoundFontOsc *self = BSE_SOUND_FONT_OSC (object);

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static BseSoundFontRepo*
get_sfrepo (BseSoundFontOsc *self)
{
  if (!self->config.sfrepo)
    {
      BseProject *project = bse_item_get_project (BSE_ITEM (self));
      if (project)
	{
	  self->config.sfrepo = bse_project_get_sound_font_repo (project);
	  self->config.osc_id = bse_sound_font_repo_add_osc (self->config.sfrepo, self);
	}
      else
	{
	  g_warning ("BseSoundFontOsc: could not find sfrepo\n");
	  self->config.sfrepo = NULL;
	}
    }
  return self->config.sfrepo;
}

static void
bse_sound_font_osc_uncross_preset (BseItem *owner,
				   BseItem *ref_item)
{
  BseSoundFontOsc *self = BSE_SOUND_FONT_OSC (owner);
  bse_item_set (self, "preset", NULL, NULL);
}


static void
bse_sound_font_osc_set_property (GObject      *object,
				 guint         param_id,
				 const GValue *value,
				 GParamSpec   *pspec)
{
  BseSoundFontOsc *self = BSE_SOUND_FONT_OSC (object);

  switch (param_id)
    {
      BseSoundFontPreset *preset;
    case PARAM_PRESET:
      preset = bse_value_get_object (value);
      if (preset != self->preset)
        {
          self->preset = preset;
          if (self->preset)
            {
              bse_item_cross_link (BSE_ITEM (self), BSE_ITEM (self->preset), bse_sound_font_osc_uncross_preset);
              bse_object_proxy_notifies (self->preset, self, "notify::preset");
	      self->config.sfont_id = BSE_SOUND_FONT (BSE_ITEM (self->preset)->parent)->sfont_id;
	      self->config.bank = self->preset->bank;
	      self->config.program = self->preset->program;
	      self->config.update_preset++;
              bse_sound_font_osc_update_modules (self, NULL);
            }
        }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_sound_font_osc_get_property (GObject     *object,
			         guint        param_id,
			         GValue      *value,
			         GParamSpec  *pspec)
{
  BseSoundFontOsc *self = BSE_SOUND_FONT_OSC (object);

  switch (param_id)
    {
    case PARAM_PRESET:
      bse_value_set_object (value, self->preset);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}


static void
bse_sound_font_osc_get_candidates (BseItem               *item,
                                   guint                  param_id,
                                   BsePropertyCandidates *pc,
                                   GParamSpec            *pspec)
{
  BseSoundFontOsc *self = BSE_SOUND_FONT_OSC (item);
  switch (param_id)
    {
    case PARAM_PRESET:
      bse_property_candidate_relabel (pc, _("Available Presets"), _("List of available sound font presets to choose as fluid synth preset"));
      bse_sound_font_repo_list_all_presets (get_sfrepo (self), pc->items);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}


typedef struct
{
  BseSoundFontOscConfig	config;
  int			last_update_preset;
  guint64               n_silence_samples;    // for done detection
} SoundFontOscModule;

static void
bse_sound_font_osc_update_modules (BseSoundFontOsc *sound_font_osc,
				   BseTrans        *trans)
{
  get_sfrepo (sound_font_osc);

  if (BSE_SOURCE_PREPARED (sound_font_osc))
    {
      bse_source_update_modules (BSE_SOURCE (sound_font_osc),
				 G_STRUCT_OFFSET (SoundFontOscModule, config),
				 &sound_font_osc->config,
				 sizeof (sound_font_osc->config),
				 trans);
    }
}

static void
sound_font_osc_reset (BseModule *module)
{
  SoundFontOscModule *flmod = module->user_data;

  flmod->last_update_preset = -1;
}

/* process_fluid is only called once per block, not once per module
 */
static void
process_fluid_L (BseSoundFontRepo   *sfrepo,
	         fluid_synth_t	    *fluid_synth,
                 guint64	     now_tick_stamp)
{
  float **channel_values_left = g_alloca (sfrepo->n_fluid_channels * sizeof (float *));
  float **channel_values_right = g_alloca (sfrepo->n_fluid_channels * sizeof (float *));
  float null_fx[BSE_STREAM_MAX_VALUES];
  float *channel_fx_null[2] = { null_fx, null_fx };
  int i;

  g_return_if_fail (now_tick_stamp > sfrepo->channel_values_tick_stamp);
  sfrepo->channel_values_tick_stamp = now_tick_stamp;

  /* Sample precise timing: If events don't occur at block boundary, the block
     is partially calculated, then the event is executed, and then the rest of
     the block (until the next event) is calculated, and so on */
  for (i = 0; i < sfrepo->n_fluid_channels; i++)
    {
      channel_values_left[i] = sfrepo->channel_values_left[i];
      channel_values_right[i] = sfrepo->channel_values_right[i];
    }
  guint values_remaining = bse_engine_block_size();
  while (values_remaining)
    {
      /* get 1st event tick stamp */
      BseFluidEvent *event = NULL;
      guint64 event_tick_stamp;
      if (sfrepo->fluid_events)
	{
	  event = (BseFluidEvent *) sfrepo->fluid_events->data;
	  event_tick_stamp = event->tick_stamp;
	}
      else
	{
	  /* if no event is present, the earliest event that can occur is after this block */
	  event_tick_stamp = now_tick_stamp + values_remaining;
	}
      if (event_tick_stamp <= now_tick_stamp)	     /* past or present event -> process it */
	{
	  switch (event->command)
	    {
	      case BSE_MIDI_NOTE_ON:    fluid_synth_noteon (fluid_synth, event->channel, event->arg1, event->arg2);
					sfrepo->n_silence_samples[event->channel] = 0;
					break;
	      case BSE_MIDI_NOTE_OFF:   fluid_synth_noteoff (fluid_synth, event->channel, event->arg1);
					break;
	      case BSE_MIDI_PITCH_BEND: fluid_synth_pitch_bend (fluid_synth, event->channel, event->arg1);
					break;
	      case BSE_FLUID_SYNTH_PROGRAM_SELECT:
					fluid_synth_program_select (fluid_synth, event->channel,
								    event->sfont_id,
								    event->arg1, event->arg2);
					break;
	      case BSE_MIDI_X_CONTINUOUS_CHANGE:
					fluid_synth_cc (fluid_synth, event->channel,
							event->arg1, event->arg2);
					break;
	    }
	  sfrepo->fluid_events = sfi_ring_remove_node (sfrepo->fluid_events, sfrepo->fluid_events);
	  g_free (event);
	}
      else						     /* future event tick stamp: process audio until then */
	{
	  gint64 values_todo = MIN (values_remaining, event_tick_stamp - now_tick_stamp);
	  fluid_synth_nwrite_float (fluid_synth, values_todo,
				    channel_values_left, channel_values_right,
				    channel_fx_null, channel_fx_null);
	  values_remaining -= values_todo;
	  now_tick_stamp += values_todo;
	  for (i = 0; i < sfrepo->n_fluid_channels; i++)          /* increment fluid synth output buffer pointers */
	    {
	      channel_values_left[i] += values_todo;
	      channel_values_right[i] += values_todo;
	    }
	}
    }
}

static void
sound_font_osc_process (BseModule *module,
		        guint      n_values)
{
  SoundFontOscModule *flmod = module->user_data;
  BseSoundFontRepo *sfrepo = flmod->config.sfrepo;
  fluid_synth_t *fluid_synth = bse_sound_font_repo_lock_fluid_synth (sfrepo);
  guint i;
  if (flmod->config.update_preset != flmod->last_update_preset)
    {
      fluid_synth_program_select (fluid_synth,	flmod->config.sfrepo->channel_map[flmod->config.osc_id],
						flmod->config.sfont_id, flmod->config.bank, flmod->config.program);
      flmod->last_update_preset = flmod->config.update_preset;
    }
  gint64 now_tick_stamp = GSL_TICK_STAMP;
  if (sfrepo->channel_values_tick_stamp != now_tick_stamp)
    process_fluid_L (sfrepo, fluid_synth, now_tick_stamp);

  float *left_output = sfrepo->channel_values_left[sfrepo->channel_map[flmod->config.osc_id]];
  float *right_output = sfrepo->channel_values_right[sfrepo->channel_map[flmod->config.osc_id]];

  int delta = bse_module_tick_stamp (module) - now_tick_stamp;
  if (delta + n_values <= bse_engine_block_size())    /* paranoid check, should always pass */
    {
      left_output += delta;
      right_output += delta;
      BSE_MODULE_OSTREAM (module, BSE_SOUND_FONT_OSC_OCHANNEL_LEFT_OUT).values = left_output;
      BSE_MODULE_OSTREAM (module, BSE_SOUND_FONT_OSC_OCHANNEL_RIGHT_OUT).values = right_output;
    }
  else
    {
      g_warning (G_STRLOC ": access past end of channel_values buffer");
    }
  if (BSE_MODULE_OSTREAM (module, BSE_SOUND_FONT_OSC_OCHANNEL_DONE_OUT).connected)
    {
      for (i = 0; i < n_values && left_output[i] == 0.0 && right_output[i] == 0.0; i++)
	;
      if (i == n_values)
	sfrepo->n_silence_samples[sfrepo->channel_map[flmod->config.osc_id]] += n_values;
      else
	sfrepo->n_silence_samples[sfrepo->channel_map[flmod->config.osc_id]] = 0;
      float done = (sfrepo->n_silence_samples[sfrepo->channel_map[flmod->config.osc_id]] > 1024 && sfrepo->fluid_events == NULL) ? 1.0 : 0.0;
      BSE_MODULE_OSTREAM (module, BSE_SOUND_FONT_OSC_OCHANNEL_DONE_OUT).values = bse_engine_const_values (done);
    }

  bse_sound_font_repo_unlock_fluid_synth (sfrepo);
}

static int
event_cmp (gconstpointer  a,
           gconstpointer  b,
           gpointer       data)
{
  const BseFluidEvent *event1 = (const BseFluidEvent *) a;
  const BseFluidEvent *event2 = (const BseFluidEvent *) b;

  if (event1->tick_stamp < event2->tick_stamp)
    return -1;
  return (event1->tick_stamp > event2->tick_stamp);
}

static void
sound_font_osc_process_midi (gpointer            null,
                             BseModule          *module,
                             const BseMidiEvent *event,
                             BseTrans           *trans)
{
  SoundFontOscModule *flmod = module->user_data;
  bse_sound_font_repo_lock_fluid_synth (flmod->config.sfrepo);
  int note = bse_note_from_freq (BSE_MUSICAL_TUNING_12_TET, event->data.note.frequency);
  BseFluidEvent *fluid_event = NULL;
  switch (event->status)
    {
      case BSE_MIDI_NOTE_ON:
	fluid_event = g_new0 (BseFluidEvent, 1);
	fluid_event->command = BSE_MIDI_NOTE_ON;
	fluid_event->arg1 = note;
	fluid_event->arg2 = event->data.note.velocity * 127;
	break;
      case BSE_MIDI_NOTE_OFF:
	fluid_event = g_new0 (BseFluidEvent, 1);
	fluid_event->command = BSE_MIDI_NOTE_OFF;
	fluid_event->arg1 = note;
	break;
      case BSE_MIDI_PITCH_BEND:
	fluid_event = g_new0 (BseFluidEvent, 1);
	fluid_event->command = BSE_MIDI_PITCH_BEND;
	/* since midi uses 14 bits, the range is 0x0000 ... 0x3fff
	 * however, since beast uses -1 ... 0 ...  1, we use the formula
	 * below with an output range of  0x0000 ... 0x4000 (but fluid synth
	 * seems to accept these values without trouble) - its also the
	 * inverse of whats done to the input in bsemididecoder.c
	 */
	fluid_event->arg1 = (event->data.pitch_bend * 0x2000) + 0x2000;
	break;
      case BSE_MIDI_CONTROL_CHANGE:
      case BSE_MIDI_X_CONTINUOUS_CHANGE:
	fluid_event = g_new0 (BseFluidEvent, 1);
	fluid_event->command = BSE_MIDI_X_CONTINUOUS_CHANGE;
	fluid_event->arg1 = event->data.control.control;
	/* we do the inverse of what the BEAST midi file reading code does;
         * this means we should be able to replay midi files without loosing
         * any information - however, since midi information has no sign,
         * we truncate negative numbers to zero
         *
         * FIXME: it would be possible to do an almost loss free conversion
         * if the beast representation of controls would be more MIDI like
         */
	fluid_event->arg2 = CLAMP (event->data.control.value * 127, 0, 127);
	break;
      case BSE_MIDI_PROGRAM_CHANGE:
	/* programs should be set at track level, and are thus not changeable here */
	break;
      default:
	printf ("BseSoundFontOsc: unhandled status %02x\n", event->status);
	break;
    }
  if (fluid_event)
    {
      fluid_event->tick_stamp = event->delta_time;
      fluid_event->channel = flmod->config.sfrepo->channel_map[flmod->config.osc_id];
      flmod->config.sfrepo->fluid_events = sfi_ring_insert_sorted (flmod->config.sfrepo->fluid_events, fluid_event, event_cmp, NULL);
    }
  bse_sound_font_repo_unlock_fluid_synth (flmod->config.sfrepo);
}

typedef struct
{
  BseMidiReceiver  *midi_receiver;
  guint		    midi_channel;
  BseModule        *module;
} EventHandlerSetup;

static void
event_handler_setup_func (BseModule *module,
                          void *ehs_data)
{
  EventHandlerSetup *ehs = (EventHandlerSetup *)ehs_data;
  bse_midi_receiver_add_event_handler (ehs->midi_receiver,
                                       ehs->midi_channel,
                                       sound_font_osc_process_midi,
                                       NULL,
                                       ehs->module);

  /* setup program before first midi event */
  SoundFontOscModule *flmod = module->user_data;

  BseFluidEvent *fluid_event = g_new0 (BseFluidEvent, 1);
  fluid_event->command = BSE_FLUID_SYNTH_PROGRAM_SELECT;
  fluid_event->channel = flmod->config.sfrepo->channel_map[flmod->config.osc_id];
  fluid_event->arg1 = flmod->config.bank;
  fluid_event->arg2 = flmod->config.program;
  fluid_event->sfont_id = flmod->config.sfont_id;
  fluid_event->tick_stamp = 0; /* now */
  flmod->config.sfrepo->fluid_events = sfi_ring_insert_sorted (flmod->config.sfrepo->fluid_events, fluid_event, event_cmp, NULL);
}

static void
bse_sound_font_osc_context_create (BseSource *source,
				   guint      context_handle,
				   BseTrans  *trans)
{
  static const BseModuleClass sound_font_osc_class = {
    0,				    /* n_istreams */
    0,				    /* n_jstreams */
    BSE_SOUND_FONT_OSC_N_OCHANNELS, /* n_ostreams */
    sound_font_osc_process,	    /* process */
    NULL,			    /* process_defer */
    sound_font_osc_reset,	    /* reset */
    (BseModuleFreeFunc) g_free,	    /* free */
    BSE_COST_CHEAP,		    /* flags */
  };
  SoundFontOscModule *sound_font_osc = g_new0 (SoundFontOscModule, 1);
  BseModule *module;

  module = bse_module_new (&sound_font_osc_class, sound_font_osc);

  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);

  /* commit module to engine */
  bse_trans_add (trans, bse_job_integrate (module));

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);

  /* update (initialize) module data */
  bse_sound_font_osc_update_modules (BSE_SOUND_FONT_OSC (source), trans);

  /* setup midi event handler */
  EventHandlerSetup *ehs = g_new0 (EventHandlerSetup, 1);
  BseMidiContext mc = bse_snet_get_midi_context (bse_item_get_snet (BSE_ITEM (source)), context_handle);
  ehs->midi_receiver = mc.midi_receiver;
  ehs->midi_channel = mc.midi_channel;
  ehs->module = module;
  bse_trans_add (trans, bse_job_access (module, event_handler_setup_func, ehs, g_free));

  /* reset fluid synth if necessary */
  BseSoundFontOsc *self = BSE_SOUND_FONT_OSC (source);
  fluid_synth_t *fluid_synth = bse_sound_font_repo_lock_fluid_synth (self->config.sfrepo);
  if (self->config.sfrepo->n_channel_oscs_active == 0)
    fluid_synth_system_reset (fluid_synth);
  self->config.sfrepo->n_channel_oscs_active++;
  bse_sound_font_repo_unlock_fluid_synth (self->config.sfrepo);
}

static void
bse_sound_font_osc_context_dismiss (BseSource		 *source,
			            guint		  context_handle,
			            BseTrans		 *trans)
{
  BseSoundFontOsc *self = BSE_SOUND_FONT_OSC (source);
  BseModule *module = bse_source_get_context_omodule (source, context_handle);
  BseMidiContext mc = bse_snet_get_midi_context (bse_item_get_snet (BSE_ITEM (source)), context_handle);
  bse_midi_receiver_remove_event_handler (mc.midi_receiver,
                                          mc.midi_channel,
					  sound_font_osc_process_midi,
                                          NULL,
                                          module);
  /* remove old events from the event queue */
  bse_sound_font_repo_lock_fluid_synth (self->config.sfrepo);
  SfiRing *fluid_events = self->config.sfrepo->fluid_events;
  SfiRing *node = fluid_events;
  while (node)
    {
      SfiRing *next_node = sfi_ring_walk (node, fluid_events);
      BseFluidEvent *event = node->data;
      if (event->channel == self->config.sfrepo->channel_map[self->config.osc_id])
	{
	  g_free (event);
	  fluid_events = sfi_ring_remove_node (fluid_events, node);
	}
      node = next_node;
    }
  self->config.sfrepo->n_channel_oscs_active--;
  self->config.sfrepo->fluid_events = fluid_events;
  bse_sound_font_repo_unlock_fluid_synth (self->config.sfrepo);
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_dismiss (source, context_handle, trans);
}
