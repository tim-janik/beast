/* BseMIDIModule - BSE MIDI Signal Output
 * Copyright (C) 2001 Tim Janik
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
#include "bsemidimodule.h"

#include "gslengine.h"
#include <sys/time.h>	/* struct timeval */
#include <unistd.h>	/* select() */



/* --- functions --- */
static gfloat
pick_signal (BseMidiChannel *channel,
	     guint           signal,
	     guint	     nth_note)
{
  gfloat value = 0;

  if (signal >= BSE_MIDI_SIGNAL_FREQUENCY)
    {
      if (nth_note < channel->n_notes)
	{
	  BseMidiNote *note = channel->notes + nth_note;

	  switch (signal)
	    {
	    case BSE_MIDI_SIGNAL_FREQUENCY:	value = BSE_VALUE_FROM_FREQ (bse_note_to_freq (note->note));	break;
	    case BSE_MIDI_SIGNAL_GATE:		value = note->velocity > 0 ? 1.0 : 0.0;		break;
	    case BSE_MIDI_SIGNAL_VELOCITY:	value = BSE_VALUE_FROM_BOUNDED_UINT (note->velocity, BSE_MIDI_MAX_VELOCITY);		break;
	    case BSE_MIDI_SIGNAL_AFTERTOUCH:	value = BSE_VALUE_FROM_BOUNDED_UINT (note->aftertouch, BSE_MIDI_MAX_AFTERTOUCH);	break;
	    }
	}
    }
  else if (signal >= BSE_MIDI_SIGNAL_PROGRAM)
    {
      switch (signal)
	{
	case BSE_MIDI_SIGNAL_PROGRAM:		value = BSE_VALUE_FROM_BOUNDED_UINT (channel->program, BSE_MIDI_MAX_PROGRAM);	break;
	case BSE_MIDI_SIGNAL_PRESSURE:		value = BSE_VALUE_FROM_BOUNDED_UINT (channel->pressure, BSE_MIDI_MAX_PRESSURE);	break;
	case BSE_MIDI_SIGNAL_PITCH_BEND:	value = BSE_VALUE_FROM_BOUNDED_UINT (channel->pitch_bend, BSE_MIDI_MAX_PITCH_BEND + 1) * 2.0 - 1.0;	break;
	}
    }
  else /* signal = BSE_MIDI_SIGNAL_CONTROL + nth */
    {
      if (channel->use_count)	/* avoid removal race */
	value = BSE_VALUE_FROM_BOUNDED_UINT (channel->control_values[signal], BSE_MIDI_MAX_CONTROL_VALUE);
    }

  return value;
}

static void
bse_midi_module_process (GslModule *module,
			 guint      n_values)
{
  BseMidiModuleData *mdata = module->user_data;
  guint *signals = mdata->signals;
  guint nth_note = mdata->nth_note;
  BseMidiChannel *channel;
  gfloat values[BSE_MIDI_MODULE_N_CHANNELS];
  guint i;

  channel = _bse_midi_decoder_lock_channel (mdata->decoder, mdata->midi_channel_id - 1);
  for (i = 0; i < BSE_MIDI_MODULE_N_CHANNELS; i++)
    if (module->ostreams[i].connected)
      values[i] = pick_signal (channel, signals[i], nth_note);
    else
      values[i] = 0.0;
  _bse_midi_decoder_unlock_channel (mdata->decoder, channel);

  /* wait with gsl_const_values() untill here to keep lock latencies small */
  for (i = 0; i < BSE_MIDI_MODULE_N_CHANNELS; i++)
    if (module->ostreams[i].connected)
      module->ostreams[i].values = gsl_engine_const_values (values[i]);
}

GslModule*
bse_midi_module_insert (BseMidiDecoder *decoder,
			guint           midi_channel_id,
			guint           nth_note, /* voice */
			guint           signals[BSE_MIDI_MODULE_N_CHANNELS],
			GslTrans       *trans)
{
  static const GslClass midi_module_class = {
    0,				/* n_istreams */
    0,				/* n_jstreams */
    BSE_MIDI_MODULE_N_CHANNELS,	/* n_ostreams */
    bse_midi_module_process,	/* process */
    (GslModuleFreeFunc) g_free,	/* free */
    GSL_COST_CHEAP
  };
  BseMidiModuleData *mdata;
  GslModule *module;
  guint i;

  g_return_val_if_fail (decoder != NULL, NULL);
  g_return_val_if_fail (midi_channel_id - 1 < BSE_MIDI_MAX_CHANNELS, NULL);
  g_return_val_if_fail (signals != NULL, NULL);
  g_return_val_if_fail (trans != NULL, NULL);
  
  mdata = g_new0 (BseMidiModuleData, 1);
  mdata->decoder = decoder;
  mdata->midi_channel_id = midi_channel_id;
  mdata->nth_note = nth_note;
  for (i = 0; i < BSE_MIDI_MODULE_N_CHANNELS; i++)
    mdata->signals[i] = signals[i];
  module = gsl_module_new (&midi_module_class, mdata);
  gsl_trans_add (trans, gsl_job_integrate (module));
  gsl_trans_add (trans, gsl_job_set_consumer (module, TRUE));

  _bse_midi_decoder_use_channel (mdata->decoder, mdata->midi_channel_id - 1);
  
  return module;
}

void
bse_midi_module_remove (GslModule *midi_module,
			GslTrans  *trans)
{
  BseMidiModuleData *mdata;

  g_return_if_fail (midi_module != NULL);
  g_return_if_fail (trans != NULL);

  mdata = midi_module->user_data;
  _bse_midi_decoder_unuse_channel (mdata->decoder, mdata->midi_channel_id - 1);

  gsl_trans_add (trans, gsl_job_discard (midi_module));
}

gboolean
bse_midi_module_matches (GslModule *midi_module,
			 guint      midi_channel_id,
			 guint      nth_note,
			 guint      signals[BSE_MIDI_MODULE_N_CHANNELS])
{
  BseMidiModuleData *mdata;
  gboolean match = TRUE;
  guint i;

  g_return_val_if_fail (midi_module != NULL, FALSE);
  g_return_val_if_fail (signals != NULL, FALSE);
  
  mdata = midi_module->user_data;
  for (i = 0; i < BSE_MIDI_MODULE_N_CHANNELS; i++)
    match &= mdata->signals[i] == signals[i];
  match &= mdata->nth_note == nth_note;
  match &= mdata->midi_channel_id == midi_channel_id;

  return match;
}
