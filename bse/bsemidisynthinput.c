/* BseMidiSynthInput - BSE Midi Synth glue module
 * Copyright (C) 1999, 2000-2001 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bsemidisynthinput.h"

#include "bseserver.h"
#include "gslengine.h"


/* --- prototypes --- */
static void	 bse_midi_synth_input_init		(BseMidiSynthInput		*scard);
static void	 bse_midi_synth_input_class_init	(BseMidiSynthInputClass	*class);
static void	 bse_midi_synth_input_do_destroy	(BseObject		*object);
static void	 bse_midi_synth_input_prepare		(BseSource		*source);
static void	 bse_midi_synth_input_context_create	(BseSource		*source,
							 guint			 instance_id,
							 GslTrans		*trans);
static void	 bse_midi_synth_input_context_dismiss	(BseSource		*source,
							 guint			 instance_id,
							 GslTrans		*trans);
static void	 bse_midi_synth_input_reset		(BseSource		*source);


/* --- variables --- */
static gpointer		 parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseMidiSynthInput)
{
  static const GTypeInfo midi_synth_input_info = {
    sizeof (BseMidiSynthInputClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_midi_synth_input_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseMidiSynthInput),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_midi_synth_input_init,
  };
  guint midi_synth_input_type_id;
  
  midi_synth_input_type_id = bse_type_register_static (BSE_TYPE_SOURCE,
						       "BseMidiSynthInput",
						       "Internal MIDI Synth glue object",
						       &midi_synth_input_info);
  return midi_synth_input_type_id;
}

static void
bse_midi_synth_input_class_init (BseMidiSynthInputClass *class)
{
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ochannel_id;
  
  parent_class = g_type_class_peek_parent (class);
  
  object_class->destroy = bse_midi_synth_input_do_destroy;
  
  source_class->prepare = bse_midi_synth_input_prepare;
  source_class->context_create = bse_midi_synth_input_context_create;
  source_class->context_dismiss = bse_midi_synth_input_context_dismiss;
  source_class->reset = bse_midi_synth_input_reset;
  
  ochannel_id = bse_source_class_add_ochannel (source_class, "Frequency", NULL);
  g_assert (ochannel_id == BSE_MIDI_SYNTH_INPUT_OCHANNEL_FREQUENCY);
  ochannel_id = bse_source_class_add_ochannel (source_class, "Gate", NULL);
  g_assert (ochannel_id == BSE_MIDI_SYNTH_INPUT_OCHANNEL_GATE);
  ochannel_id = bse_source_class_add_ochannel (source_class, "Velocity", NULL);
  g_assert (ochannel_id == BSE_MIDI_SYNTH_INPUT_OCHANNEL_VELOCITY);
  ochannel_id = bse_source_class_add_ochannel (source_class, "Aftertouch", NULL);
  g_assert (ochannel_id == BSE_MIDI_SYNTH_INPUT_OCHANNEL_AFTERTOUCH);
}

static void
bse_midi_synth_input_init (BseMidiSynthInput *msi)
{
  msi->midi_channel_id = 1;
  msi->nth_note = 0;
  msi->midi_input_module = NULL;
}

static void
bse_midi_synth_input_do_destroy (BseObject *object)
{
  BseMidiSynthInput *msi = BSE_MIDI_SYNTH_INPUT (object);

  g_assert (msi->midi_input_module == NULL);
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

void
bse_midi_synth_input_set_params (BseMidiSynthInput *msi,
				 guint              midi_channel_id,
				 guint              nth_note)
{
  g_return_if_fail (BSE_IS_MIDI_SYNTH_INPUT (msi));
  g_return_if_fail (midi_channel_id - 1 < BSE_MIDI_MAX_CHANNELS);
  g_return_if_fail (!BSE_SOURCE_PREPARED (msi));
  g_return_if_fail (msi->midi_input_module == NULL);	/* paranoid */

  msi->midi_channel_id = midi_channel_id;
  msi->nth_note = nth_note;
}

static void
bse_midi_synth_input_prepare (BseSource *source)
{
  BseMidiSynthInput *msi = BSE_MIDI_SYNTH_INPUT (source);
  guint signals[4] = {
    BSE_MIDI_SIGNAL_FREQUENCY,	/* BSE_MIDI_SYNTH_INPUT_OCHANNEL_FREQUENCY */
    BSE_MIDI_SIGNAL_GATE,	/* BSE_MIDI_SYNTH_INPUT_OCHANNEL_GATE */
    BSE_MIDI_SIGNAL_VELOCITY,	/* BSE_MIDI_SYNTH_INPUT_OCHANNEL_VELOCITY */
    BSE_MIDI_SIGNAL_AFTERTOUCH,	/* BSE_MIDI_SYNTH_INPUT_OCHANNEL_AFTERTOUCH */
  };

  msi->midi_input_module = bse_server_retrive_midi_input_module (bse_server_get (),
								 "MidiIn",
								 msi->midi_channel_id,
								 msi->nth_note,
								 signals);
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

static void
bse_midi_synth_input_context_create (BseSource *source,
				     guint      context_handle,
				     GslTrans  *trans)
{
  BseMidiSynthInput *msi = BSE_MIDI_SYNTH_INPUT (source);

  /* we simply borrow the midi module from the BseServer for
   * all contexts, so it's already committed into the engine.
   *
   * the only thing we need to do is to setup module i/o streams
   * with BseSource i/o channels to enable connections
   */
  bse_source_set_context_module (source, context_handle, msi->midi_input_module);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

static void
bse_midi_synth_input_context_dismiss (BseSource *source,
				      guint      context_handle,
				      GslTrans  *trans)
{
  // BseMidiSynthInput *msi = BSE_MIDI_SYNTH_INPUT (source);
  // GslModule *module = msi->midi_input_module;
  
  /* keep this function in sync with bse_source_real_context_dismiss() and
   * bse_source_real_context_connect() as we override the disconnecting/
   * discarding behaviour completely.
   * thing is, the GslModule isn't ours, so theoretically we would just need
   * to disconnect and not discard it.
   * but since connecting/disconnecting src to dest modules is handled by
   * the dest modules, we actually have to do nothing.
   */

  /* don't chain parent class' handler
   * BSE_SOURCE_CLASS (parent_class)->context_dismiss (source, context_handle, trans);
   */
}

static void
bse_midi_synth_input_reset (BseSource *source)
{
  BseMidiSynthInput *msi = BSE_MIDI_SYNTH_INPUT (source);
  
  bse_server_discard_midi_input_module (bse_server_get (), msi->midi_input_module);
  msi->midi_input_module = NULL;
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}
