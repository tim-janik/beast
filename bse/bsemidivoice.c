/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1999, 2000-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bsemidivoice.h"

#include "bseserver.h"
#include "bsemidireceiver.h"
#include "gslengine.h"
#include "gslcommon.h"
#include "bsesnet.h"


/* --- prototypes --- */
static void	 bse_midi_voice_input_init		(BseMidiVoiceInput	 *self);
static void	 bse_midi_voice_input_class_init	(BseMidiVoiceInputClass	 *class);
static void      bse_midi_voice_input_dispose		(GObject                 *object);
static void	 bse_midi_voice_input_context_create	(BseSource		 *source,
							 guint			  context_handle,
							 GslTrans		 *trans);
static void	 bse_midi_voice_input_context_dismiss	(BseSource		 *source,
							 guint			  context_handle,
							 GslTrans		 *trans);
static void	 bse_midi_voice_switch_init		(BseMidiVoiceSwitch	 *self);
static void	 bse_midi_voice_switch_class_init	(BseMidiVoiceSwitchClass *class);
static void      bse_midi_voice_switch_dispose		(GObject                 *object);
static void	 bse_midi_voice_switch_context_create	(BseSource		 *source,
							 guint			  context_handle,
							 GslTrans		 *trans);
static void	 bse_midi_voice_switch_context_dismiss	(BseSource		 *source,
							 guint			  context_handle,
							 GslTrans		 *trans);


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
				   &type_info);
}

static void
bse_midi_voice_input_class_init (BseMidiVoiceInputClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint channel_id;
  
  voice_input_parent_class = g_type_class_peek_parent (class);
  
  gobject_class->dispose = bse_midi_voice_input_dispose;
  
  source_class->context_create = bse_midi_voice_input_context_create;
  source_class->context_dismiss = bse_midi_voice_input_context_dismiss;
  
  channel_id = bse_source_class_add_ochannel (source_class, "Freq Out", NULL);
  g_assert (channel_id == BSE_MIDI_VOICE_INPUT_OCHANNEL_FREQUENCY);
  channel_id = bse_source_class_add_ochannel (source_class, "Gate Out", NULL);
  g_assert (channel_id == BSE_MIDI_VOICE_INPUT_OCHANNEL_GATE);
  channel_id = bse_source_class_add_ochannel (source_class, "Velocity Out", NULL);
  g_assert (channel_id == BSE_MIDI_VOICE_INPUT_OCHANNEL_VELOCITY);
  channel_id = bse_source_class_add_ochannel (source_class, "Aftertouch Out", NULL);
  g_assert (channel_id == BSE_MIDI_VOICE_INPUT_OCHANNEL_AFTERTOUCH);
}

static void
bse_midi_voice_switch_class_init (BseMidiVoiceSwitchClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint channel_id;
  
  voice_switch_parent_class = g_type_class_peek_parent (class);
  
  gobject_class->dispose = bse_midi_voice_switch_dispose;
  
  source_class->context_create = bse_midi_voice_switch_context_create;
  source_class->context_dismiss = bse_midi_voice_switch_context_dismiss;
  
  channel_id = bse_source_class_add_ichannel (source_class, "Left In", NULL);
  g_assert (channel_id == BSE_MIDI_VOICE_SWITCH_ICHANNEL_LEFT);
  channel_id = bse_source_class_add_ichannel (source_class, "Right In", NULL);
  g_assert (channel_id == BSE_MIDI_VOICE_SWITCH_ICHANNEL_RIGHT);
  channel_id = bse_source_class_add_ichannel (source_class, "Disconnect In", NULL);
  g_assert (channel_id == BSE_MIDI_VOICE_SWITCH_ICHANNEL_DISCONNECT);
  channel_id = bse_source_class_add_ochannel (source_class, "Left Out", NULL);
  g_assert (channel_id == BSE_MIDI_VOICE_SWITCH_ICHANNEL_LEFT);
  channel_id = bse_source_class_add_ochannel (source_class, "Right Out", NULL);
  g_assert (channel_id == BSE_MIDI_VOICE_SWITCH_ICHANNEL_RIGHT);
  channel_id = bse_source_class_add_ochannel (source_class, "Disconnect Out", NULL);
  g_assert (channel_id == BSE_MIDI_VOICE_SWITCH_ICHANNEL_DISCONNECT);
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
bse_midi_voice_input_set_midi_receiver (BseMidiVoiceInput *self,
					BseMidiReceiver   *midi_receiver,
					guint              midi_channel)
{
  g_return_if_fail (BSE_IS_MIDI_VOICE_INPUT (self));
  g_return_if_fail (!BSE_SOURCE_PREPARED (self));
  
  if (self->midi_receiver)
    bse_midi_receiver_unref (self->midi_receiver);
  self->midi_receiver = midi_receiver;
  self->midi_channel = midi_channel;
  if (self->midi_receiver)
    bse_midi_receiver_ref (self->midi_receiver);
}

void
bse_midi_voice_switch_set_voice_input (BseMidiVoiceSwitch *self,
				       BseMidiVoiceInput  *voice_input)
{
  g_return_if_fail (BSE_IS_MIDI_VOICE_SWITCH (self));
  g_return_if_fail (!BSE_SOURCE_PREPARED (self));
  if (voice_input)
    g_return_if_fail (BSE_IS_MIDI_VOICE_INPUT (voice_input));
  
  if (self->voice_input)
    g_object_unref (self->voice_input);
  self->voice_input = voice_input;
  if (self->voice_input)
    g_object_ref (self->voice_input);
}

static void
bse_midi_voice_input_dispose (GObject *object)
{
  BseMidiVoiceInput *self = BSE_MIDI_VOICE_INPUT (object);
  
  bse_midi_voice_input_set_midi_receiver (self, NULL, 0);
  
  /* chain parent class' destroy handler */
  G_OBJECT_CLASS (voice_input_parent_class)->dispose (object);
}

static void
bse_midi_voice_switch_dispose (GObject *object)
{
  BseMidiVoiceSwitch *self = BSE_MIDI_VOICE_SWITCH (object);
  
  bse_midi_voice_switch_set_voice_input (self, NULL);
  
  /* chain parent class' destroy handler */
  G_OBJECT_CLASS (voice_switch_parent_class)->dispose (object);
}

static void
bse_midi_voice_input_context_create (BseSource *source,
				     guint      context_handle,
				     GslTrans  *trans)
{
  BseMidiVoiceInput *self = BSE_MIDI_VOICE_INPUT (source);
  BseMidiReceiver *midi_receiver;
  guint voice;
  
  bse_midi_voice_input_ref_midi_voice (self, context_handle,
				       &midi_receiver, &voice, trans);
  
  /* we simply wrap the module from BseMidiReceiver */
  bse_source_set_context_omodule (source, context_handle,
				  bse_midi_receiver_get_note_module (midi_receiver, voice));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (voice_input_parent_class)->context_create (source, context_handle, trans);
}

static void
bse_midi_voice_switch_context_create (BseSource *source,
				      guint      context_handle,
				      GslTrans  *trans)
{
  BseMidiVoiceSwitch *self = BSE_MIDI_VOICE_SWITCH (source);
  BseMidiReceiver *midi_receiver;
  guint voice;
  
  g_return_if_fail (self->voice_input != NULL);
  
  bse_midi_voice_input_ref_midi_voice (self->voice_input, context_handle,
				       &midi_receiver, &voice, trans);
  
  /* we simply wrap the modules from BseMidiReceiver */
  bse_source_set_context_imodule (source, context_handle,
				  bse_midi_receiver_get_input_module (midi_receiver, voice));
  bse_source_set_context_omodule (source, context_handle,
				  bse_midi_receiver_get_output_module (midi_receiver, voice));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (voice_switch_parent_class)->context_create (source, context_handle, trans);
}

static void
bse_midi_voice_input_context_dismiss (BseSource *source,
				      guint      context_handle,
				      GslTrans  *trans)
{
  BseMidiVoiceInput *self = BSE_MIDI_VOICE_INPUT (source);
  
  /* the GslModule isn't ours, so theoretically we would just need
   * to disconnect and not discard it.
   * but since connecting/disconnecting src to dest modules is handled by
   * the dest modules, we actually have to do nothing besides preventing
   * BseSource to discard the foreign module.
   */
  
  bse_source_set_context_omodule (source, context_handle, NULL);
  
  bse_midi_voice_input_unref_midi_voice (self, context_handle, trans);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (voice_input_parent_class)->context_dismiss (source, context_handle, trans);
}

static void
bse_midi_voice_switch_context_dismiss (BseSource *source,
				       guint      context_handle,
				       GslTrans  *trans)
{
  BseMidiVoiceSwitch *self = BSE_MIDI_VOICE_SWITCH (source);
  
  g_return_if_fail (self->voice_input != NULL);
  
  /* the GslModules aren't ours, so we need to disconnect the input module
   * and prevent discarding the modules.
   */
  
  gsl_trans_add (trans, gsl_job_kill_inputs (bse_source_get_context_imodule (source, context_handle)));
  bse_source_set_context_imodule (source, context_handle, NULL);
  bse_source_set_context_omodule (source, context_handle, NULL);
  
  bse_midi_voice_input_unref_midi_voice (self->voice_input, context_handle, trans);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (voice_switch_parent_class)->context_dismiss (source, context_handle, trans);
}

typedef struct {
  guint		   context_handle;
  guint		   ref_count;
  BseMidiReceiver *midi_receiver;
  guint            voice;
} MidiVoice;

void
bse_midi_voice_input_ref_midi_voice (BseMidiVoiceInput *self,
				     guint              context_handle,
				     BseMidiReceiver  **midi_receiver_p,
				     guint             *voice_p,
				     GslTrans	       *trans)
{
  SfiRing *ring;
  MidiVoice *mvoice;
  
  g_return_if_fail (BSE_IS_MIDI_VOICE_INPUT (self));
  g_return_if_fail (BSE_SOURCE_PREPARED (self));
  g_return_if_fail (midi_receiver_p && voice_p);
  g_return_if_fail (trans != NULL);
  
  for (ring = self->midi_voices; ring; ring = sfi_ring_walk (ring, self->midi_voices))
    {
      mvoice = ring->data;
      if (mvoice->context_handle == context_handle)
	break;
    }
  if (!ring)
    {
      guint midi_channel;
      mvoice = sfi_new_struct (MidiVoice, 1);
      mvoice->context_handle = context_handle;
      mvoice->ref_count = 1;
      if (self->midi_receiver)
	{
	  mvoice->midi_receiver = bse_midi_receiver_ref (self->midi_receiver);
	  midi_channel = self->midi_channel;
	}
      else
	mvoice->midi_receiver = bse_midi_receiver_ref (bse_snet_get_midi_receiver (BSE_SNET (BSE_ITEM (self)->parent),
										   context_handle, &midi_channel));
      mvoice->voice = bse_midi_receiver_create_voice (mvoice->midi_receiver, midi_channel, trans);
      self->midi_voices = sfi_ring_prepend (self->midi_voices, mvoice);
    }
  else
    mvoice->ref_count++;
  *midi_receiver_p = mvoice->midi_receiver;
  *voice_p = mvoice->voice;
}

void
bse_midi_voice_input_unref_midi_voice (BseMidiVoiceInput *self,
				       guint              context_handle,
				       GslTrans		 *trans)
{
  SfiRing *ring;
  MidiVoice *mvoice;
  
  g_return_if_fail (BSE_IS_MIDI_VOICE_INPUT (self));
  g_return_if_fail (BSE_SOURCE_PREPARED (self));
  g_return_if_fail (trans != NULL);
  
  for (ring = self->midi_voices; ring; ring = sfi_ring_walk (ring, self->midi_voices))
    {
      mvoice = ring->data;
      if (mvoice->context_handle == context_handle)
	break;
    }
  if (!ring)
    g_warning ("module %s has no midi voice for context %u",
	       bse_object_debug_name (self),
	       context_handle);
  else
    {
      mvoice->ref_count--;
      if (!mvoice->ref_count)
	{
	  self->midi_voices = sfi_ring_remove_node (self->midi_voices, ring);
	  bse_midi_receiver_discard_voice (mvoice->midi_receiver, mvoice->voice, trans);
	  bse_midi_receiver_unref (mvoice->midi_receiver);
	  sfi_delete_struct (MidiVoice, mvoice);
	}
    }
}
