// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsemidisynth.hh"

#include "bsemidievent.hh"	/* BSE_MIDI_MAX_CHANNELS */
#include "bsemidivoice.hh"
#include "bsemidireceiver.hh"
#include "bsecontextmerger.hh"
#include "bsesubsynth.hh"
#include "bsepcmoutput.hh"
#include "bseproject.hh"
#include "bsecsynth.hh"
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>


/* --- parameters --- */
enum
{
  PROP_0,
  PROP_MIDI_CHANNEL,
  PROP_N_VOICES,
  PROP_SNET,
  PROP_PNET,
  PROP_VOLUME_f,
  PROP_VOLUME_dB,
  PROP_VOLUME_PERC,
};


/* --- prototypes --- */
static void         bse_midi_synth_class_init          (BseMidiSynthClass *klass);
static void         bse_midi_synth_init                (BseMidiSynth      *msynth);
static void         bse_midi_synth_finalize            (GObject           *object);
static void         bse_midi_synth_set_property        (GObject           *object,
                                                        guint              param_id,
                                                        const GValue      *value,
                                                        GParamSpec        *pspec);
static void         bse_midi_synth_get_property        (GObject           *msynth,
                                                        guint              param_id,
                                                        GValue            *value,
                                                        GParamSpec        *pspec);
static void         bse_midi_synth_context_create      (BseSource         *source,
                                                        guint              context_handle,
                                                        BseTrans          *trans);
static void         bse_misi_synth_update_midi_channel (BseMidiSynth      *self);



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
                                              __FILE__, __LINE__,
                                              &snet_info);

  return midi_synth_type;
}

static void
bse_midi_synth_init (BseMidiSynth *self)
{
  BseSNet *snet = BSE_SNET (self);

  bse_item_set (self, "uname", _("Midi-Synth"), NULL);

  BSE_OBJECT_UNSET_FLAGS (self, BSE_SNET_FLAG_USER_SYNTH);
  BSE_OBJECT_SET_FLAGS (self, BSE_SUPER_FLAG_NEEDS_CONTEXT);
  self->midi_channel_id = 1;
  self->n_voices = 16;
  self->volume_factor = bse_db_to_factor (0);

  /* midi voice modules */
  self->voice_input = (BseSource*) bse_container_new_child (BSE_CONTAINER (self), BSE_TYPE_MIDI_VOICE_INPUT, NULL);
  bse_snet_intern_child (snet, self->voice_input);
  self->voice_switch = (BseSource*) bse_container_new_child (BSE_CONTAINER (self), BSE_TYPE_MIDI_VOICE_SWITCH, NULL);
  bse_snet_intern_child (snet, self->voice_switch);
  bse_midi_voice_input_set_voice_switch (BSE_MIDI_VOICE_INPUT (self->voice_input), BSE_MIDI_VOICE_SWITCH (self->voice_switch));

  /* context merger */
  self->context_merger = (BseSource*) bse_container_new_child (BSE_CONTAINER (self), BSE_TYPE_CONTEXT_MERGER, NULL);
  bse_snet_intern_child (snet, self->context_merger);

  /* midi voice switch <-> context merger */
  bse_source_must_set_input (self->context_merger, 0,
                             self->voice_switch, BSE_MIDI_VOICE_SWITCH_OCHANNEL_LEFT);
  bse_source_must_set_input (self->context_merger, 1,
                             self->voice_switch, BSE_MIDI_VOICE_SWITCH_OCHANNEL_RIGHT);

  /* post processing slot */
  self->postprocess = (BseSource*) bse_container_new_child (BSE_CONTAINER (self), BSE_TYPE_SUB_SYNTH, "uname", "Postprocess", NULL);
  bse_snet_intern_child (snet, self->postprocess);
  bse_sub_synth_set_null_shortcut (BSE_SUB_SYNTH (self->postprocess), TRUE);

  /* context merger <-> postprocess */
  bse_source_must_set_input (self->postprocess, 0,
                             self->context_merger, 0);
  bse_source_must_set_input (self->postprocess, 1,
                             self->context_merger, 1);

  /* output */
  self->output = (BseSource*) bse_container_new_child (BSE_CONTAINER (self), BSE_TYPE_PCM_OUTPUT, NULL);
  bse_snet_intern_child (snet, self->output);

  /* postprocess <-> output */
  bse_source_must_set_input (self->output, BSE_PCM_OUTPUT_ICHANNEL_LEFT,
                             self->postprocess, 0);
  bse_source_must_set_input (self->output, BSE_PCM_OUTPUT_ICHANNEL_RIGHT,
                             self->postprocess, 1);

  /* sub synth */
  self->sub_synth = (BseSource*) bse_container_new_child (BSE_CONTAINER (self), BSE_TYPE_SUB_SYNTH,
                                                          "in_port_1", "frequency",
                                                          "in_port_2", "gate",
                                                          "in_port_3", "velocity",
                                                          "in_port_4", "aftertouch",
                                                          "out_port_1", "left-audio",
                                                          "out_port_2", "right-audio",
                                                          "out_port_3", "unused",
                                                          "out_port_4", "synth-done",
                                                          NULL);
  bse_snet_intern_child (snet, self->sub_synth);

  /* voice input <-> sub-synth */
  bse_source_must_set_input (self->sub_synth, 0,
                             self->voice_input, BSE_MIDI_VOICE_INPUT_OCHANNEL_FREQUENCY);
  bse_source_must_set_input (self->sub_synth, 1,
                             self->voice_input, BSE_MIDI_VOICE_INPUT_OCHANNEL_GATE);
  bse_source_must_set_input (self->sub_synth, 2,
                             self->voice_input, BSE_MIDI_VOICE_INPUT_OCHANNEL_VELOCITY);
  bse_source_must_set_input (self->sub_synth, 3,
                             self->voice_input, BSE_MIDI_VOICE_INPUT_OCHANNEL_AFTERTOUCH);

  /* sub-synth <-> voice switch */
  bse_source_must_set_input (self->voice_switch, BSE_MIDI_VOICE_SWITCH_ICHANNEL_LEFT,
                             self->sub_synth, 0);
  bse_source_must_set_input (self->voice_switch, BSE_MIDI_VOICE_SWITCH_ICHANNEL_RIGHT,
                             self->sub_synth, 1);
  bse_source_must_set_input (self->voice_switch, BSE_MIDI_VOICE_SWITCH_ICHANNEL_DISCONNECT,
                             self->sub_synth, 3);

  bse_misi_synth_update_midi_channel (self);
}

static void
bse_misi_synth_update_midi_channel (BseMidiSynth      *self)
{
  if (self->voice_switch)
    {
      bse_sub_synth_set_midi_channel (BSE_SUB_SYNTH (self->sub_synth), self->midi_channel_id);
      bse_sub_synth_set_midi_channel (BSE_SUB_SYNTH (self->postprocess), self->midi_channel_id);
      bse_midi_voice_switch_set_midi_channel (BSE_MIDI_VOICE_SWITCH (self->voice_switch), self->midi_channel_id);
    }
}

static void
bse_midi_synth_finalize (GObject *object)
{
  BseMidiSynth *self = BSE_MIDI_SYNTH (object);

  bse_container_remove_item (BSE_CONTAINER (self), BSE_ITEM (self->voice_input));
  self->voice_input = NULL;
  bse_container_remove_item (BSE_CONTAINER (self), BSE_ITEM (self->voice_switch));
  self->voice_switch = NULL;
  bse_container_remove_item (BSE_CONTAINER (self), BSE_ITEM (self->context_merger));
  self->context_merger = NULL;
  bse_container_remove_item (BSE_CONTAINER (self), BSE_ITEM (self->postprocess));
  self->postprocess = NULL;
  bse_container_remove_item (BSE_CONTAINER (self), BSE_ITEM (self->output));
  self->output = NULL;
  bse_container_remove_item (BSE_CONTAINER (self), BSE_ITEM (self->sub_synth));
  self->sub_synth = NULL;

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bse_midi_synth_get_candidates (BseItem *item, uint param_id, Bse::PropertyCandidates &pc, GParamSpec *pspec)
{
  BseMidiSynth *self = BSE_MIDI_SYNTH (item);
  switch (param_id)
    {
    case PROP_SNET:
      pc.label = _("Available Synthesizers");
      pc.tooltip = _("List of available synthesis networks to choose a MIDI instrument from");
      bse_item_gather_items_typed (item, pc.items, BSE_TYPE_CSYNTH, BSE_TYPE_PROJECT, FALSE);
      break;
    case PROP_PNET:
      pc.label = _("Available Postprocessors");
      pc.tooltip = _("List of available synthesis networks to choose a postprocessor from");
      bse_item_gather_items_typed (item, pc.items, BSE_TYPE_CSYNTH, BSE_TYPE_PROJECT, FALSE);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
midi_synth_uncross_snet (BseItem *owner,
                         BseItem *ref_item)
{
  BseMidiSynth *self = BSE_MIDI_SYNTH (owner);
  bse_item_set (self, "snet", NULL, NULL);
}

static void
midi_synth_uncross_pnet (BseItem *owner,
                         BseItem *ref_item)
{
  BseMidiSynth *self = BSE_MIDI_SYNTH (owner);
  bse_item_set (self, "pnet", NULL, NULL);
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
    case PROP_SNET:
      if (!BSE_SOURCE_PREPARED (self))
        {
          if (self->snet)
            {
              bse_object_unproxy_notifies (self->snet, self, "notify::snet");
              bse_item_cross_unlink (BSE_ITEM (self), BSE_ITEM (self->snet), midi_synth_uncross_snet);
              self->snet = NULL;
            }
          self->snet = (BseSNet*) bse_value_get_object (value);
          if (self->snet)
            {
              bse_item_cross_link (BSE_ITEM (self), BSE_ITEM (self->snet), midi_synth_uncross_snet);
              bse_object_proxy_notifies (self->snet, self, "notify::snet");
            }
          g_object_set (self->sub_synth, /* no undo */
                        "snet", self->snet,
                        NULL);
        }
      break;
    case PROP_PNET:
      if (!BSE_SOURCE_PREPARED (self))
        {
          if (self->pnet)
            {
              bse_object_unproxy_notifies (self->pnet, self, "notify::pnet");
              bse_item_cross_unlink (BSE_ITEM (self), BSE_ITEM (self->pnet), midi_synth_uncross_pnet);
              self->pnet = NULL;
            }
          self->pnet = (BseSNet*) bse_value_get_object (value);
          if (self->pnet)
            {
              bse_item_cross_link (BSE_ITEM (self), BSE_ITEM (self->pnet), midi_synth_uncross_pnet);
              bse_object_proxy_notifies (self->pnet, self, "notify::pnet");
            }
          if (self->postprocess)
            g_object_set (self->postprocess, /* no undo */
                          "snet", self->pnet,
                          NULL);
        }
      break;
    case PROP_MIDI_CHANNEL:
      if (!BSE_SOURCE_PREPARED (self))	/* midi channel is locked while prepared */
        {
          self->midi_channel_id = sfi_value_get_int (value);
          bse_misi_synth_update_midi_channel (self);
        }
      break;
    case PROP_N_VOICES:
      if (!BSE_OBJECT_IS_LOCKED (self))
	self->n_voices = sfi_value_get_int (value);
      break;
    case PROP_VOLUME_f:
      self->volume_factor = sfi_value_get_real (value);
      g_object_set (self->output, /* no undo */
                    "master_volume_f", self->volume_factor,
                    NULL);
      g_object_notify ((GObject*) self, "volume_dB");
      g_object_notify ((GObject*) self, "volume_perc");
      break;
    case PROP_VOLUME_dB:
      self->volume_factor = bse_db_to_factor (sfi_value_get_real (value));
      g_object_set (self->output, /* no undo */
                    "master_volume_f", self->volume_factor,
                    NULL);
      g_object_notify ((GObject*) self, "volume_f");
      g_object_notify ((GObject*) self, "volume_perc");
      break;
    case PROP_VOLUME_PERC:
      self->volume_factor = sfi_value_get_int (value) / 100.0;
      g_object_set (self->output, /* no undo */
                    "master_volume_f", self->volume_factor,
                    NULL);
      g_object_notify ((GObject*) self, "volume_f");
      g_object_notify ((GObject*) self, "volume_dB");
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
    case PROP_SNET:
      bse_value_set_object (value, self->snet);
      break;
    case PROP_PNET:
      bse_value_set_object (value, self->pnet);
      break;
    case PROP_MIDI_CHANNEL:
      sfi_value_set_int (value, self->midi_channel_id);
      break;
    case PROP_N_VOICES:
      sfi_value_set_int (value, self->n_voices);
      break;
    case PROP_VOLUME_f:
      sfi_value_set_real (value, self->volume_factor);
      break;
    case PROP_VOLUME_dB:
      sfi_value_set_real (value, bse_db_from_factor (self->volume_factor, BSE_MIN_VOLUME_dB));
      break;
    case PROP_VOLUME_PERC:
      sfi_value_set_int (value, self->volume_factor * 100.0 + 0.5);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_midi_synth_context_create (BseSource *source,
			       guint      context_handle,
			       BseTrans  *trans)
{
  BseMidiSynth *self = BSE_MIDI_SYNTH (source);
  BseSNet *snet = BSE_SNET (self);

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);

  if (!bse_snet_context_is_branch (snet, context_handle))	/* catch recursion */
    {
      BseMidiContext mcontext = bse_snet_get_midi_context (snet, context_handle);
      guint i;
      for (i = 0; i < self->n_voices; i++)
	bse_snet_context_clone_branch (snet, context_handle, self->context_merger, mcontext, trans);

      bse_midi_receiver_channel_enable_poly (mcontext.midi_receiver, mcontext.midi_channel);
    }
}

static void
bse_midi_synth_context_dismiss (BseSource *source,
                                guint      context_handle,
                                BseTrans  *trans)
{
  BseMidiSynth *self = BSE_MIDI_SYNTH (source);
  BseSNet *snet = BSE_SNET (self);

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_dismiss (source, context_handle, trans);

  if (!bse_snet_context_is_branch (snet, context_handle))
    {
      BseMidiContext mcontext = bse_snet_get_midi_context (snet, context_handle);
      bse_midi_receiver_channel_disable_poly (mcontext.midi_receiver, mcontext.midi_channel);
    }
}

static void
bse_midi_synth_class_init (BseMidiSynthClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (klass);
  BseItemClass *item_class = BSE_ITEM_CLASS (klass);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (klass);

  parent_class = (GTypeClass*) g_type_class_peek_parent (klass);
  gobject_class->set_property = bse_midi_synth_set_property;
  gobject_class->get_property = bse_midi_synth_get_property;
  gobject_class->finalize = bse_midi_synth_finalize;
  item_class->get_candidates = bse_midi_synth_get_candidates;

  source_class->context_create = bse_midi_synth_context_create;
  source_class->context_dismiss = bse_midi_synth_context_dismiss;

  bse_object_class_add_param (object_class, _("MIDI Instrument"),
			      PROP_MIDI_CHANNEL,
			      sfi_pspec_int ("midi_channel", _("MIDI Channel"), NULL,
					     1, 1, BSE_MIDI_MAX_CHANNELS, 1,
					     SFI_PARAM_GUI SFI_PARAM_STORAGE ":scale:skip-default:unprepared"));
  bse_object_class_add_param (object_class, _("MIDI Instrument"),
			      PROP_N_VOICES,
			      sfi_pspec_int ("n_voices", _("Max Voices"), _("Maximum number of voices for simultaneous playback"),
					     16, 1, 256, 1,
					     SFI_PARAM_GUI SFI_PARAM_STORAGE ":scale"));
  bse_object_class_add_param (object_class, _("MIDI Instrument"),
			      PROP_SNET,
			      bse_param_spec_object ("snet", _("Synthesizer"), _("Synthesis network to be used as MIDI instrument"),
						     BSE_TYPE_CSYNTH, SFI_PARAM_STANDARD ":unprepared"));
  bse_object_class_add_param (object_class, _("MIDI Instrument"),
                              PROP_PNET,
                              bse_param_spec_object ("pnet", _("Postprocessor"), _("Synthesis network to be used as postprocessor"),
                                                     BSE_TYPE_CSYNTH,
                                                     SFI_PARAM_STANDARD ":unprepared"));
  bse_object_class_add_param (object_class, _("Adjustments"),
			      PROP_VOLUME_f,
			      sfi_pspec_real ("volume_f", _("Master [float]"), NULL,
					      bse_db_to_factor (0),
					      0, bse_db_to_factor (BSE_MAX_VOLUME_dB), 0.1,
					      SFI_PARAM_STORAGE));
  bse_object_class_add_param (object_class, _("Adjustments"),
			      PROP_VOLUME_dB,
			      sfi_pspec_real ("volume_dB", _("Master [dB]"), NULL,
					      0,
					      BSE_MIN_VOLUME_dB, BSE_MAX_VOLUME_dB,
					      0.1,
					      SFI_PARAM_GUI ":dial"));
  bse_object_class_add_param (object_class, _("Adjustments"),
			      PROP_VOLUME_PERC,
			      sfi_pspec_int ("volume_perc", _("Master [%]"), NULL,
					     bse_db_to_factor (0) * 100,
					     0, bse_db_to_factor (BSE_MAX_VOLUME_dB) * 100, 1,
					     SFI_PARAM_GUI ":dial"));
}

namespace Bse {

MidiSynthImpl::MidiSynthImpl (BseObject *bobj) :
  SNetImpl (bobj)
{}

MidiSynthImpl::~MidiSynthImpl ()
{}

} // Bse
