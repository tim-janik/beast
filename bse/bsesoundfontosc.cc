// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsesoundfontosc.hh"
#include "bsecategories.hh"
#include "bseengine.hh"
#include "bseproject.hh"
#include "bsesoundfontrepo.hh"
#include "bsesoundfont.hh"
#include "bsesnet.hh"
#include "bsemidireceiver.hh"
#include "gslcommon.hh"
#include "bseblockutils.hh"
#include "bse/internal.hh"
#include <string.h>

/*------------------------------------------------------------------------------------------------
 * overview of how soundfont support is implemented
 *------------------------------------------------------------------------------------------------
 * What this does:
 *  - use existing soundfonts & fluidsynth replay code
 *  - be able to use soundfont presets (instruments) instead of synthesis instruments on a
 *    per-track basis
 *  - use the standard beast mixer with tracks that use soundfonts
 *
 * To be able to use the fluidsynth API, and let fluidsynth render the output,
 * this code will simply relay the events from the midi receiver to the
 * fluidsynth engine using bse_midi_receiver_add_event_handler.
 *
 * For each track, there is one fluid_synth_t instance (one BseSoundFontOsc),
 * which renders the audio for that track without using fluidsynth effects.
 * Effects can be added using our mixer.
 *------------------------------------------------------------------------------------------------
 */

/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_PRESET
};


/* --- prototypes --- */
static void	 bse_sound_font_osc_init	  (BseSoundFontOsc	 *sound_font_osc);
static void	 bse_sound_font_osc_class_init	  (BseSoundFontOscClass	 *klass);
static void	 bse_sound_font_osc_set_property  (GObject		 *object,
						   guint	          param_id,
						   const GValue		 *value,
						   GParamSpec		 *pspec);
static void	 bse_sound_font_osc_get_property  (GObject		 *object,
						   guint		  param_id,
						   GValue		 *value,
						   GParamSpec		 *pspec);
static void	 bse_sound_font_osc_get_candidates (BseItem		 *item,
						    uint		  param_id,
						    Bse::PropertyCandidates &pc,
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
bse_sound_font_osc_class_init (BseSoundFontOscClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (klass);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (klass);
  BseItemClass *item_class = BSE_ITEM_CLASS (klass);
  guint ochannel;

  parent_class = g_type_class_peek_parent (klass);

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
  assert_return (ochannel == BSE_SOUND_FONT_OSC_OCHANNEL_LEFT_OUT);
  ochannel = bse_source_class_add_ochannel (source_class, "right-out", _("Right Out"), _("Output of the fluid synth soundfont synthesizer"));
  assert_return (ochannel == BSE_SOUND_FONT_OSC_OCHANNEL_RIGHT_OUT);
  ochannel = bse_source_class_add_ochannel (source_class, "done-out", _("Done Out"), _("Done Output"));
  assert_return (ochannel == BSE_SOUND_FONT_OSC_OCHANNEL_DONE_OUT);
}

static void
bse_sound_font_osc_init (BseSoundFontOsc *self)
{
  new (&self->data) BseSoundFontOsc::Data();

  memset (&self->config, 0, sizeof (self->config));
  self->config.silence_bound = bse_engine_sample_freq() * 0.020;  /* output is zero for 20 ms => set done output */
  self->preset = NULL;
}

static void
bse_sound_font_osc_dispose (GObject *object)
{
  BseSoundFontOsc *self = BSE_SOUND_FONT_OSC (object);

  if (self->config.sfrepo)
    self->config.sfrepo = nullptr;

  if (self->data.cached_fluid_synth)
    {
      delete_fluid_synth (self->data.cached_fluid_synth);
      self->data.cached_fluid_synth = nullptr;
    }

  if (self->data.cached_fluid_settings)
    {
      delete_fluid_settings (self->data.cached_fluid_settings);
      self->data.cached_fluid_settings = nullptr;
    }

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
bse_sound_font_osc_finalize (GObject *object)
{
  BseSoundFontOsc *self = BSE_SOUND_FONT_OSC (object);

  self->data.~Data();

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
	}
      else
	{
	  Bse::warning ("BseSoundFontOsc: could not find sfrepo\n");
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
      preset = BSE_SOUND_FONT_PRESET (bse_value_get_object (value));
      if (preset != self->preset)
        {
          self->preset = preset;
          if (self->preset)
            {
              bse_item_cross_link (BSE_ITEM (self), BSE_ITEM (self->preset), bse_sound_font_osc_uncross_preset);
	      self->config.bank = self->preset->bank;
	      self->config.program = self->preset->program;
	      self->config.update_preset++;
	      self->data.filename = bse_sound_font_get_filename (BSE_SOUND_FONT (BSE_ITEM (self->preset)->parent));
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
bse_sound_font_osc_get_candidates (BseItem *item, uint param_id, Bse::PropertyCandidates &pc, GParamSpec *pspec)
{
  BseSoundFontOsc *self = BSE_SOUND_FONT_OSC (item);
  switch (param_id)
    {
    case PARAM_PRESET:
      pc.label = _("Available Presets");
      pc.tooltip = _("List of available sound font presets to choose as fluid synth preset");
      bse_sound_font_repo_list_all_presets (get_sfrepo (self), pc.items);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}


typedef struct
{
  BseSoundFontOscConfig	config;
  SfiRing              *fluid_events;
  fluid_synth_t        *fluid_synth;
  int                   sfont_id;
  int			last_update_preset;
  guint64               n_silence_samples;    // for done detection
} SoundFontOscModule;

static void
bse_sound_font_osc_update_modules (BseSoundFontOsc *sound_font_osc,
				   BseTrans        *trans)
{
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
  SoundFontOscModule *flmod = (SoundFontOscModule *) module->user_data;

  flmod->last_update_preset = -1;
}

static void
sound_font_osc_process (BseModule *module,
		        guint      n_values)
{
  SoundFontOscModule *flmod = (SoundFontOscModule *) module->user_data;

  guint values_remaining = n_values;
  guint64 now_tick_stamp = bse_module_tick_stamp (module);
  fluid_synth_t *fluid_synth = flmod->fluid_synth;
  float *channel_values[2];

  channel_values[0] = BSE_MODULE_OBUFFER (module, BSE_SOUND_FONT_OSC_OCHANNEL_LEFT_OUT);
  channel_values[1] = BSE_MODULE_OBUFFER (module, BSE_SOUND_FONT_OSC_OCHANNEL_RIGHT_OUT);

  Bse::Block::fill (values_remaining, channel_values[0], 0.0);
  Bse::Block::fill (values_remaining, channel_values[1], 0.0);

  while (values_remaining)
    {
      /* get 1st event tick stamp */
      BseFluidEvent *event = NULL;
      guint64 event_tick_stamp;
      if (flmod->fluid_events)
	{
	  event = (BseFluidEvent *) flmod->fluid_events->data;
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
					flmod->n_silence_samples = 0;
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
	  flmod->fluid_events = sfi_ring_remove_node (flmod->fluid_events, flmod->fluid_events);
	  g_free (event);
	}
      else						     /* future event tick stamp: process audio until then */
	{
	  gint64 values_todo = MIN (values_remaining, event_tick_stamp - now_tick_stamp);
          fluid_synth_process (fluid_synth, values_todo,
                               0, nullptr, /* no effects */
                               2, channel_values);

	  values_remaining -= values_todo;
	  now_tick_stamp += values_todo;

	  /* increment fluid synth output buffer pointers */
          channel_values[0] += values_todo;
          channel_values[1] += values_todo;
	}
    }
  if (flmod->config.update_preset != flmod->last_update_preset)
    {
      fluid_synth_program_select (fluid_synth, 0, flmod->sfont_id, flmod->config.bank, flmod->config.program);
      flmod->last_update_preset = flmod->config.update_preset;
    }

  const float *left_output = BSE_MODULE_OBUFFER (module, BSE_SOUND_FONT_OSC_OCHANNEL_LEFT_OUT);
  const float *right_output = BSE_MODULE_OBUFFER (module, BSE_SOUND_FONT_OSC_OCHANNEL_RIGHT_OUT);

  if (BSE_MODULE_OSTREAM (module, BSE_SOUND_FONT_OSC_OCHANNEL_DONE_OUT).connected)
    {
      guint i;
      for (i = 0; i < n_values && left_output[i] == 0.0 && right_output[i] == 0.0; i++)
	;
      if (i == n_values)
	flmod->n_silence_samples += n_values;
      else
	flmod->n_silence_samples = 0;
      float done = (flmod->n_silence_samples > flmod->config.silence_bound && flmod->fluid_events == NULL) ? 1.0 : 0.0;
      BSE_MODULE_OSTREAM (module, BSE_SOUND_FONT_OSC_OCHANNEL_DONE_OUT).values = bse_engine_const_values (done);
    }
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
  SoundFontOscModule *flmod = (SoundFontOscModule *) module->user_data;

  int note = bse_note_from_freq (Bse::MusicalTuning::OD_12_TET, event->data.note.frequency);
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
      fluid_event->channel = 0;
      flmod->fluid_events = sfi_ring_insert_sorted (flmod->fluid_events, fluid_event, event_cmp, NULL);
    }
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
}

static void
sound_font_osc_free_data (void                 *data,
                          const BseModuleClass *klass)
{
  /* remove old events from the event queue */
  SoundFontOscModule *flmod = (SoundFontOscModule *) data;
  SfiRing *fluid_events = flmod->fluid_events;
  SfiRing *node = fluid_events;
  while (node)
    {
      SfiRing *next_node = sfi_ring_walk (node, fluid_events);
      BseFluidEvent *event = (BseFluidEvent *) node->data;
      g_free (event);
      fluid_events = sfi_ring_remove_node (fluid_events, node);
      node = next_node;
    }
  flmod->fluid_events = fluid_events;
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
    (BseModuleFreeFunc) sound_font_osc_free_data,  /* free */
    Bse::ModuleFlag::CHEAP,	    /* flags */
  };
  SoundFontOscModule *sound_font_osc = g_new0 (SoundFontOscModule, 1);
  BseModule *module;

  module = bse_module_new (&sound_font_osc_class, sound_font_osc);

  /* commit module to engine */
  bse_trans_add (trans, bse_job_integrate (module));

  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module, trans);

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

  BseSoundFontOsc *self = BSE_SOUND_FONT_OSC (source);

  /* loading a soundfont can take a while, so instead of freeing the
   * fluid_synth afer using it, we keep it in data.cached_fluid_synth
   *
   * if the settings still match the next time the user hits play, we reuse the
   * existing instance
   *
   * since the module uses the fluid_synth instance, we cannot modify it
   * while the project is playing
   */
  uint mix_freq = bse_engine_sample_freq();
  if (self->data.cached_filename != self->data.filename || self->data.cached_mix_freq != mix_freq)
    {
      if (self->data.cached_fluid_synth)
        delete_fluid_synth (self->data.cached_fluid_synth);

      if (self->data.cached_fluid_settings)
        delete_fluid_settings (self->data.cached_fluid_settings);

      fluid_settings_t *fluid_settings = self->data.cached_fluid_settings = new_fluid_settings();

      fluid_settings_setnum (fluid_settings, "synth.sample-rate", mix_freq);
      /* soundfont instruments should be as loud as beast synthesis network instruments */
      fluid_settings_setnum (fluid_settings, "synth.gain", 1.0);
      fluid_settings_setint (fluid_settings, "synth.midi-channels", 16);
      fluid_settings_setint (fluid_settings, "synth.audio-channels", 1);
      fluid_settings_setint (fluid_settings, "synth.audio-groups", 1);
      fluid_settings_setint (fluid_settings, "synth.reverb.active", 0);
      fluid_settings_setint (fluid_settings, "synth.chorus.active", 0);
      /* we ensure that our fluid_synth instance is only used by one thread at a time
       *  => we can disable automated locks that protect all fluid synth API calls
       */
      fluid_settings_setint (fluid_settings, "synth.threadsafe-api", 0);

      self->data.cached_filename = self->data.filename;
      self->data.cached_fluid_synth = new_fluid_synth (fluid_settings);
      self->data.cached_sfont_id = fluid_synth_sfload (self->data.cached_fluid_synth, self->data.filename.c_str(), 0);
      self->data.cached_mix_freq = mix_freq;
    }
  sound_font_osc->fluid_synth = self->data.cached_fluid_synth;
  sound_font_osc->sfont_id    = self->data.cached_sfont_id;

  fluid_synth_system_reset (sound_font_osc->fluid_synth);
  fluid_synth_program_select (sound_font_osc->fluid_synth, 0, sound_font_osc->sfont_id, self->config.bank, self->config.program);
}

static void
bse_sound_font_osc_context_dismiss (BseSource		 *source,
			            guint		  context_handle,
			            BseTrans		 *trans)
{
  BseModule *module = bse_source_get_context_omodule (source, context_handle);
  BseMidiContext mc = bse_snet_get_midi_context (bse_item_get_snet (BSE_ITEM (source)), context_handle);
  bse_midi_receiver_remove_event_handler (mc.midi_receiver,
                                          mc.midi_channel,
					  sound_font_osc_process_midi,
                                          NULL,
                                          module);
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_dismiss (source, context_handle, trans);
}
