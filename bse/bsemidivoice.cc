// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsemidivoice.hh"

#include "bseserver.hh"
#include "bsemidireceiver.hh"
#include "bseengine.hh"
#include "gslcommon.hh"
#include "bsesnet.hh"


/* --- prototypes --- */
static void bse_midi_voice_input_init             (BseMidiVoiceInput        *self);
static void bse_midi_voice_input_class_init       (BseMidiVoiceInputClass   *klass);
static void bse_midi_voice_input_dispose          (GObject                  *object);
static void bse_midi_voice_input_context_create   (BseSource                *source,
                                                   guint                     context_handle,
                                                   BseTrans                 *trans);
static void bse_midi_voice_input_context_dismiss  (BseSource                *source,
                                                   guint                     context_handle,
                                                   BseTrans                 *trans);
static void bse_midi_voice_switch_init            (BseMidiVoiceSwitch       *self);
static void bse_midi_voice_switch_class_init      (BseMidiVoiceSwitchClass  *klass);
static void bse_midi_voice_switch_dispose         (GObject                  *object);
static void bse_midi_voice_switch_context_create  (BseSource                *source,
                                                   guint                     context_handle,
                                                   BseTrans                 *trans);
static void bse_midi_voice_switch_context_dismiss (BseSource                *source,
                                                   guint                     context_handle,
                                                   BseTrans                 *trans);


/* --- variables --- */
static gpointer voice_input_parent_class = NULL;
static gpointer voice_switch_parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseMidiVoiceInput)
{
  static const GTypeInfo type_info = {
    sizeof (BseMidiVoiceInputClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_midi_voice_input_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    sizeof (BseMidiVoiceInput),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_midi_voice_input_init,
  };

  return bse_type_register_static (BSE_TYPE_SOURCE,
				   "BseMidiVoiceInput",
				   "Internal MIDI Voice glue object (input)",
                                   __FILE__, __LINE__,
                                   &type_info);
}

BSE_BUILTIN_TYPE (BseMidiVoiceSwitch)
{
  static const GTypeInfo type_info = {
    sizeof (BseMidiVoiceSwitchClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_midi_voice_switch_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    sizeof (BseMidiVoiceSwitch),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_midi_voice_switch_init,
  };

  return bse_type_register_static (BSE_TYPE_SOURCE,
				   "BseMidiVoiceSwitch",
				   "Internal MIDI Voice glue object (switch)",
                                   __FILE__, __LINE__,
                                   &type_info);
}

static void
bse_midi_voice_input_class_init (BseMidiVoiceInputClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (klass);
  guint channel_id;

  voice_input_parent_class = g_type_class_peek_parent (klass);

  gobject_class->dispose = bse_midi_voice_input_dispose;

  source_class->context_create = bse_midi_voice_input_context_create;
  source_class->context_dismiss = bse_midi_voice_input_context_dismiss;

  channel_id = bse_source_class_add_ochannel (source_class, "freq-out", _("Freq Out"), NULL);
  assert_return (channel_id == BSE_MIDI_VOICE_INPUT_OCHANNEL_FREQUENCY);
  channel_id = bse_source_class_add_ochannel (source_class, "gate-out", _("Gate Out"), NULL);
  assert_return (channel_id == BSE_MIDI_VOICE_INPUT_OCHANNEL_GATE);
  channel_id = bse_source_class_add_ochannel (source_class, "velocity-out", _("Velocity Out"), NULL);
  assert_return (channel_id == BSE_MIDI_VOICE_INPUT_OCHANNEL_VELOCITY);
  channel_id = bse_source_class_add_ochannel (source_class, "aftertouch-out", _("Aftertouch Out"), NULL);
  assert_return (channel_id == BSE_MIDI_VOICE_INPUT_OCHANNEL_AFTERTOUCH);
}

static void
bse_midi_voice_switch_class_init (BseMidiVoiceSwitchClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (klass);
  guint channel_id;

  voice_switch_parent_class = g_type_class_peek_parent (klass);

  gobject_class->dispose = bse_midi_voice_switch_dispose;

  source_class->context_create = bse_midi_voice_switch_context_create;
  source_class->context_dismiss = bse_midi_voice_switch_context_dismiss;

  channel_id = bse_source_class_add_ichannel (source_class, "left-in", _("Left In"), NULL);
  assert_return (channel_id == BSE_MIDI_VOICE_SWITCH_ICHANNEL_LEFT);
  channel_id = bse_source_class_add_ichannel (source_class, "right-in", _("Right In"), NULL);
  assert_return (channel_id == BSE_MIDI_VOICE_SWITCH_ICHANNEL_RIGHT);
  channel_id = bse_source_class_add_ichannel (source_class, "disconnect-in", _("Disconnect In"), NULL);
  assert_return (channel_id == BSE_MIDI_VOICE_SWITCH_ICHANNEL_DISCONNECT);
  channel_id = bse_source_class_add_ochannel (source_class, "left-out", _("Left Out"), NULL);
  assert_return (channel_id == BSE_MIDI_VOICE_SWITCH_ICHANNEL_LEFT);
  channel_id = bse_source_class_add_ochannel (source_class, "right-out", _("Right Out"), NULL);
  assert_return (channel_id == BSE_MIDI_VOICE_SWITCH_ICHANNEL_RIGHT);
  channel_id = bse_source_class_add_ochannel (source_class, "disconnect-out", _("Disconnect Out"), NULL);
  assert_return (channel_id == BSE_MIDI_VOICE_SWITCH_ICHANNEL_DISCONNECT);
}

static void
bse_midi_voice_input_init (BseMidiVoiceInput *self)
{
}

static void
bse_midi_voice_switch_init (BseMidiVoiceSwitch *self)
{
}

void
bse_midi_voice_input_set_voice_switch (BseMidiVoiceInput  *self,
				       BseMidiVoiceSwitch *voice_switch)
{
  assert_return (BSE_IS_MIDI_VOICE_INPUT (self));
  assert_return (!BSE_SOURCE_PREPARED (self));
  if (voice_switch)
    assert_return (BSE_IS_MIDI_VOICE_SWITCH (voice_switch));

  if (self->voice_switch)
    g_object_unref (self->voice_switch);
  self->voice_switch = voice_switch;
  if (self->voice_switch)
    g_object_ref (self->voice_switch);
}

static void
bse_midi_voice_input_dispose (GObject *object)
{
  BseMidiVoiceInput *self = BSE_MIDI_VOICE_INPUT (object);

  bse_midi_voice_input_set_voice_switch (self, NULL);

  /* chain parent class' handler */
  G_OBJECT_CLASS (voice_input_parent_class)->dispose (object);
}

static void
bse_midi_voice_switch_dispose (GObject *object)
{
  BseMidiVoiceSwitch *self = BSE_MIDI_VOICE_SWITCH (object);

  if (self->midi_voices)
    Bse::warning ("disposing voice-switch with active midi voices");

  /* chain parent class' handler */
  G_OBJECT_CLASS (voice_switch_parent_class)->dispose (object);
}

static void
bse_midi_voice_input_context_create (BseSource *source,
				     guint      context_handle,
				     BseTrans  *trans)
{
  BseMidiVoiceInput *self = BSE_MIDI_VOICE_INPUT (source);
  BseMidiContext mcontext = bse_midi_voice_switch_ref_poly_voice (self->voice_switch, context_handle, trans);
  // FIXME: handle no voice-switch

  /* we simply wrap the module from BseMidiReceiver */
  bse_source_set_context_omodule (source, context_handle,
				  bse_midi_receiver_create_sub_voice (mcontext.midi_receiver,
                                                                      mcontext.midi_channel,
                                                                      mcontext.voice_id, trans));

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (voice_input_parent_class)->context_create (source, context_handle, trans);
}

static void
bse_midi_voice_input_context_dismiss (BseSource *source,
				      guint      context_handle,
				      BseTrans  *trans)
{
  BseMidiVoiceInput *self = BSE_MIDI_VOICE_INPUT (source);
  BseMidiContext mcontext = bse_midi_voice_switch_peek_poly_voice (self->voice_switch, context_handle);
  BseModule *module;

  /* the BseModule isn't ours, so theoretically we would just need
   * to disconnect and not discard it.
   * but since connecting/disconnecting src to dest modules is handled by
   * the dest modules, we actually have to do nothing besides preventing
   * BseSource to discard the foreign module.
   */

  module = bse_source_get_context_omodule (source, context_handle);
  bse_midi_receiver_discard_sub_voice (mcontext.midi_receiver, mcontext.midi_channel, mcontext.voice_id, module, trans);
  bse_source_set_context_omodule (source, context_handle, NULL);

  bse_midi_voice_switch_unref_poly_voice (self->voice_switch, context_handle, trans);

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (voice_input_parent_class)->context_dismiss (source, context_handle, trans);
}

static void
bse_midi_voice_switch_context_create (BseSource *source,
				      guint      context_handle,
				      BseTrans  *trans)
{
  BseMidiVoiceSwitch *self = BSE_MIDI_VOICE_SWITCH (source);
  BseMidiContext mcontext = bse_midi_voice_switch_ref_poly_voice (self, context_handle, trans);

  /* we simply wrap the modules from BseMidiReceiver */
  bse_source_set_context_imodule (source, context_handle,
                                  bse_midi_receiver_get_poly_voice_input (mcontext.midi_receiver,
                                                                          mcontext.midi_channel, mcontext.voice_id));
  bse_source_set_context_omodule (source, context_handle,
				  bse_midi_receiver_get_poly_voice_output (mcontext.midi_receiver,
                                                                           mcontext.midi_channel, mcontext.voice_id));

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (voice_switch_parent_class)->context_create (source, context_handle, trans);
}

static void
bse_midi_voice_switch_context_dismiss (BseSource *source,
				       guint      context_handle,
				       BseTrans  *trans)
{
  BseMidiVoiceSwitch *self = BSE_MIDI_VOICE_SWITCH (source);

  /* the BseModules aren't ours, so we need to disconnect the input module
   * and prevent discarding the modules.
   */

  bse_trans_add (trans, bse_job_kill_inputs (bse_source_get_context_imodule (source, context_handle)));
  bse_source_set_context_imodule (source, context_handle, NULL);
  bse_source_set_context_omodule (source, context_handle, NULL);

  bse_midi_voice_switch_unref_poly_voice (self, context_handle, trans);

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (voice_switch_parent_class)->context_dismiss (source, context_handle, trans);
}

void
bse_midi_voice_switch_set_midi_channel (BseMidiVoiceSwitch *self,
                                        guint               midi_channel)
{
  assert_return (BSE_IS_MIDI_VOICE_SWITCH (self));
  assert_return (!BSE_SOURCE_PREPARED (self));

  self->midi_channel = midi_channel;
}

typedef struct {
  guint		   context_handle;
  guint		   ref_count;
  guint            voice_id;
} MidiVoice;

BseMidiContext
bse_midi_voice_switch_ref_poly_voice (BseMidiVoiceSwitch     *self,
                                      guint                   context_handle,
                                      BseTrans               *trans)
{
  BseMidiContext mcontext = { 0, };
  MidiVoice *mvoice;
  GSList *slist;

  assert_return (BSE_IS_MIDI_VOICE_SWITCH (self), mcontext);
  assert_return (BSE_SOURCE_PREPARED (self), mcontext);
  assert_return (trans != NULL, mcontext);

  mcontext = bse_snet_get_midi_context (BSE_SNET (BSE_ITEM (self)->parent), context_handle);
  mcontext.midi_channel = self->midi_channel;
  for (slist = self->midi_voices; slist; slist = slist->next)
    {
      mvoice = (MidiVoice*) slist->data;
      if (mvoice->context_handle == context_handle)
	break;
    }
  if (!slist)
    {
      mvoice = sfi_new_struct (MidiVoice, 1);
      mvoice->context_handle = context_handle;
      mvoice->ref_count = 1;
      mvoice->voice_id = bse_midi_receiver_create_poly_voice (mcontext.midi_receiver, mcontext.midi_channel, trans);
      self->midi_voices = g_slist_prepend (self->midi_voices, mvoice);
    }
  else
    mvoice->ref_count++;
  mcontext.voice_id = mvoice->voice_id;
  return mcontext;
}

BseMidiContext
bse_midi_voice_switch_peek_poly_voice (BseMidiVoiceSwitch     *self,
                                       guint                   context_handle)
{
  BseMidiContext mcontext = { 0, };
  MidiVoice *mvoice;
  GSList *slist;

  assert_return (BSE_IS_MIDI_VOICE_SWITCH (self), mcontext);
  assert_return (BSE_SOURCE_PREPARED (self), mcontext);

  for (slist = self->midi_voices; slist; slist = slist->next)
    {
      mvoice = (MidiVoice*) slist->data;
      if (mvoice->context_handle == context_handle)
	break;
    }
  if (slist)
    {
      mcontext = bse_snet_get_midi_context (BSE_SNET (BSE_ITEM (self)->parent), context_handle);
      mcontext.midi_channel = self->midi_channel;
      mcontext.voice_id = mvoice->voice_id;
    }
  return mcontext;
}

void
bse_midi_voice_switch_unref_poly_voice (BseMidiVoiceSwitch *self,
                                        guint               context_handle,
                                        BseTrans           *trans)
{
  BseMidiContext mcontext;
  MidiVoice *mvoice;
  GSList *slist;

  assert_return (BSE_IS_MIDI_VOICE_SWITCH (self));
  assert_return (BSE_SOURCE_PREPARED (self));
  assert_return (trans != NULL);

  mcontext = bse_snet_get_midi_context (BSE_SNET (BSE_ITEM (self)->parent), context_handle);
  mcontext.midi_channel = self->midi_channel;
  for (slist = self->midi_voices; slist; slist = slist->next)
    {
      mvoice = (MidiVoice*) slist->data;
      if (mvoice->context_handle == context_handle)
	break;
    }
  if (!slist)
    Bse::warning ("module %s has no midi voice for context %u",
                  bse_object_debug_name (self), context_handle);
  else
    {
      mvoice->ref_count--;
      if (!mvoice->ref_count)
	{
          self->midi_voices = g_slist_remove (self->midi_voices, mvoice);
	  bse_midi_receiver_discard_poly_voice (mcontext.midi_receiver, mcontext.midi_channel, mvoice->voice_id, trans);
	  sfi_delete_struct (MidiVoice, mvoice);
	}
    }
}
