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
#include "bsemidievent.h"

#include <errno.h>


/* --- prototypes --- */
static void		decoder_read_event_data	(BseMidiDecoder	*decoder,
						 guint		*n_bytes_p,
						 guint8	       **bytes_p);
static BseMidiEvent*	decoder_extract_event	(BseMidiDecoder	*decoder);
static void		decoder_enqeue_event	(BseMidiDecoder	*decoder,
						 BseMidiEvent	*event);


/* --- variables --- */
static BseMidiEvent *decoder_event_cache = NULL;


/* --- functions --- */
BseMidiDecoder*
bse_midi_decoder_new (void)
{
  BseMidiDecoder *decoder = g_new0 (BseMidiDecoder, 1);

  decoder->n_channels = BSE_MIDI_MAX_CHANNELS;

  return decoder;
}

void
bse_midi_decoder_destroy (BseMidiDecoder *decoder)
{
  BseMidiEvent *event;
  guint i;

  g_return_if_fail (decoder != NULL);

  for (i = 0; i < decoder->n_channels; i++)
    {
      g_free (decoder->channels[i].control_values);
      g_free (decoder->channels[i].notes);
    }
  for (event = decoder->events; event; event = event->next)
    bse_midi_free_event (event);
  g_free (decoder->bytes);
  g_free (decoder);
}

void
_bse_midi_decoder_push_data (BseMidiDecoder *decoder,
			     guint           n_bytes,
			     guint8         *bytes)
{
  g_return_if_fail (decoder != NULL);
  if (n_bytes)
    g_return_if_fail (bytes != NULL);
  
  while (n_bytes)
    {
      /* start new event */
      if (!decoder->status)
	{
	  guint status = *bytes;

	  /* check for command/status byte */
	  if (status & 0x80)
	    {
	      if (BSE_MIDI_CHANNEL_VOICE_MESSAGE (status))
		{
		  decoder->status = status & 0xf0;
		  decoder->echannel = status & 0x0f;
		  decoder->last_status = status;	/* remember running mode */
		}
	      else /* system-realtime or system-common */
		{
		  decoder->status = status;
		  decoder->echannel = ~0;
		  if (BSE_MIDI_SYSTEM_COMMON_MESSAGE (status))
		    decoder->last_status = 0;		  /* reset running mode */
		}
	      n_bytes--;
	      bytes++;
	    }
	  else /* data byte, running mode command */
	    {
	      decoder->status = decoder->last_status & 0xF0;
	      decoder->echannel = decoder->last_status & 0x0F;
	    }
	  switch (decoder->status)
	    {
	    case 0:
	      /* ignore data byte as long as we don't know the runing mode */
	      n_bytes--;
	      bytes++;
              decoder->left_bytes = 0;
	      break;
	    case BSE_MIDI_NOTE_OFF:
	    case BSE_MIDI_NOTE_ON:
	    case BSE_MIDI_KEY_PRESSURE:		decoder->left_bytes = 2;	break;
	    case BSE_MIDI_CONTROL_CHANGE:	decoder->left_bytes = 2;	break;
	    case BSE_MIDI_PROGRAM_CHANGE:	decoder->left_bytes = 1;	break;
	    case BSE_MIDI_CHANNEL_PRESSURE:	decoder->left_bytes = 1;	break;
	    case BSE_MIDI_PITCH_BEND:		decoder->left_bytes = 2;	break;
	    case BSE_MIDI_SYS_EX:		decoder->left_bytes = ~0;	break;
	    case BSE_MIDI_SONG_POINTER:		decoder->left_bytes = 2;	break;
	    case BSE_MIDI_SONG_SELECT:		decoder->left_bytes = 1;	break;
	    case BSE_MIDI_TUNE:
	    case BSE_MIDI_TIMING_CLOCK:
	    case BSE_MIDI_SONG_START:
	    case BSE_MIDI_SONG_CONTINUE:
	    case BSE_MIDI_SONG_STOP:
	    case BSE_MIDI_ACTIVE_SENSING:
	    case BSE_MIDI_SYSTEM_RESET:
	      decoder->left_bytes = 0;
	      break;
	    case BSE_MIDI_END_EX:
	    default:
	      g_message (G_STRLOC ": unhandled midi %s byte 0x%02X\n",
			 decoder->status < 0x80 ? "data" : "command",
			 status);
	      decoder->status = 0;
              decoder->left_bytes = 0;
	      break;
	    }
	}
      /* read remaining data */
      if (n_bytes && decoder->status && decoder->left_bytes)
	decoder_read_event_data (decoder, &n_bytes, &bytes);
      /* extract event */
      if (decoder->status && decoder->left_bytes == 0)
	decoder_enqeue_event (decoder,
			      decoder_extract_event (decoder));
    }
}

static void
decoder_read_event_data (BseMidiDecoder *decoder,
			 guint          *n_bytes_p,
			 guint8        **bytes_p)
{
  guint n_bytes = *n_bytes_p;
  guint8 *bytes = *bytes_p;

  g_return_if_fail (decoder->status & 0x80);

  if (decoder->status != BSE_MIDI_SYS_EX)
    {
      guint i = MIN (decoder->left_bytes, n_bytes);
      guint n = decoder->n_bytes;
      
      decoder->n_bytes += i;
      decoder->bytes = g_renew (guint8, decoder->bytes, decoder->n_bytes);
      memcpy (decoder->bytes + n, bytes, i);
      decoder->left_bytes -= i;
      *n_bytes_p -= i;
      *bytes_p += i;
    }
  else /* search BSE_MIDI_END_EX */
    {
      guint i;

      /* search for end mark */
      for (i = 0; i < n_bytes; i++)
	if (bytes[i] == BSE_MIDI_END_EX)
	  break;
      /* append data bytes */
      if (i)
	{
	  guint n = decoder->n_bytes;

	  decoder->n_bytes += i - 1;
	  decoder->bytes = g_renew (guint8, decoder->bytes, decoder->n_bytes + 1);
	  memcpy (decoder->bytes + n, bytes, i - 1);
	}
      *n_bytes_p -= i;
      *bytes_p += i;
      /* did we find end mark? */
      if (i < n_bytes)
	{
	  decoder->status = BSE_MIDI_END_EX;
	  decoder->left_bytes = 0;
	}
    }
}

static BseMidiEvent*
decoder_extract_event (BseMidiDecoder *decoder)
{
  BseMidiEvent *event;

  g_return_val_if_fail (decoder->status & 0x80, NULL);
  g_return_val_if_fail (decoder->left_bytes == 0, NULL);

  /* special case completed SysEx */
  if (decoder->status == BSE_MIDI_END_EX)
    decoder->status = BSE_MIDI_SYS_EX;
  event = decoder_event_cache;
  if (event)
    decoder_event_cache = event->next;
  else
    event = g_new (BseMidiEvent, 1);
  event->status = decoder->status;
  event->channel = decoder->echannel;
  switch (event->status)
    {
      guint v;
    case BSE_MIDI_NOTE_OFF:
    case BSE_MIDI_NOTE_ON:
    case BSE_MIDI_KEY_PRESSURE:
      g_return_val_if_fail (decoder->n_bytes == 2, NULL);
      event->data.note.note = decoder->bytes[0] & 0x7F;
      event->data.note.velocity = decoder->bytes[1] & 0x7F;
      /* old keyboards send velocity=0 instead of note-off */
      if (event->status == BSE_MIDI_NOTE_ON && event->data.note.velocity == 0)
	event->status = BSE_MIDI_NOTE_OFF;
      /* some keyboards report junk velocity upon note-off */
      if (event->status == BSE_MIDI_NOTE_OFF)
	event->data.note.velocity = 0;
      break;
    case BSE_MIDI_CONTROL_CHANGE:
      g_return_val_if_fail (decoder->n_bytes == 2, NULL);
      event->data.control.control = decoder->bytes[0] & 0x7F;
      event->data.control.value = decoder->bytes[1] & 0x7F;
      break;
    case BSE_MIDI_PROGRAM_CHANGE:
      g_return_val_if_fail (decoder->n_bytes == 1, NULL);
      event->data.program = decoder->bytes[0] & 0x7F;
      break;
    case BSE_MIDI_CHANNEL_PRESSURE:
      g_return_val_if_fail (decoder->n_bytes == 1, NULL);
      event->data.intensity = decoder->bytes[0] & 0x7F;
      break;
    case BSE_MIDI_PITCH_BEND:
      g_return_val_if_fail (decoder->n_bytes == 2, NULL);
      event->data.pitch_bend = decoder->bytes[0] & 0x7F;
      v = decoder->bytes[1] & 0x7F;
      event->data.pitch_bend |= v << 7;
      break;
    case BSE_MIDI_SYS_EX:
      event->data.sys_ex.n_bytes = decoder->n_bytes;
      event->data.sys_ex.bytes = decoder->bytes;
      decoder->bytes = NULL;
      break;
    case BSE_MIDI_SONG_POINTER:
      g_return_val_if_fail (decoder->n_bytes == 2, NULL);
      event->data.song_pointer = decoder->bytes[0] & 0x7F;
      v = decoder->bytes[1] & 0x7F;
      event->data.song_pointer |= v << 7;
      break;
    case BSE_MIDI_SONG_SELECT:
      g_return_val_if_fail (decoder->n_bytes == 1, NULL);
      event->data.song_number = decoder->bytes[0] & 0x7F;
      break;
    default:
      event->data.sys_ex.n_bytes = 0;
      event->data.sys_ex.bytes = NULL;
      break;
    }
  decoder->n_bytes = 0;
  decoder->status = 0;

  event->next = NULL;

  return event;
}

static void
decoder_enqeue_event (BseMidiDecoder *decoder,
		      BseMidiEvent   *event)
{
  g_return_if_fail (event != NULL);
  g_return_if_fail (event->next == NULL);

  /* enqueing in opposite order */
  event->next = decoder->events;
  decoder->events = event;
}

void
bse_midi_free_event (BseMidiEvent *event)
{
  g_return_if_fail (event != NULL);
  g_return_if_fail (event->status != 0);

  if (event->status == BSE_MIDI_SYS_EX)
    g_free (event->data.sys_ex.bytes);
  event->status = 0;
  event->next = decoder_event_cache;
  decoder_event_cache = event;
}

void
bse_midi_event_process (BseMidiEvent  *event,
			BseMidiChannel channels[BSE_MIDI_MAX_CHANNELS])
{
  g_return_if_fail (event != NULL);
  g_return_if_fail (event->status >= 0x80);
  g_return_if_fail (channels != NULL);

  if (!BSE_MIDI_CHANNEL_VOICE_MESSAGE (event->status))
    switch (event->status)
      {
      case BSE_MIDI_SYS_EX:		g_message ("MIDI: ignoring SysEx command");		break;
      case BSE_MIDI_SONG_POINTER:	g_message ("MIDI: ignoring SongPointer command");	break;
      case BSE_MIDI_SONG_SELECT:
      case BSE_MIDI_TUNE:
      case BSE_MIDI_TIMING_CLOCK:
      case BSE_MIDI_SONG_START:
      case BSE_MIDI_SONG_CONTINUE:
      case BSE_MIDI_SONG_STOP:		/* ignore silently */					break;
      case BSE_MIDI_ACTIVE_SENSING: 	g_message ("MIDI: ignoring ActiveSensing command");	break;
      case BSE_MIDI_SYSTEM_RESET:	g_message ("MIDI: SystemReset");			break;
      default:			g_message ("MIDI: unknown event type 0x%02x", event->status);	break;
      }
  else
    {
      BseMidiChannel *ch = channels + event->channel;

      g_return_if_fail (event->channel < BSE_MIDI_MAX_CHANNELS);

      switch (event->status)
	{
	  guint i;
	case BSE_MIDI_NOTE_ON:
	  if (ch->use_count)
	    {
	      for (i = 0; i < ch->n_notes; i++)
		if (ch->notes[i].velocity == 0)
		  break;
	      if (i == ch->n_notes)
		ch->n_notes++;
	      if (ch->n_notes > ch->n_alloced_notes)
		{
		  ch->n_alloced_notes = ch->n_notes;
		  ch->notes = g_renew (BseMidiNote, ch->notes, ch->n_alloced_notes);
		}
	      ch->notes[i].note = event->data.note.note;
	      ch->notes[i].velocity = event->data.note.velocity;
	      ch->notes[i].aftertouch = event->data.note.velocity;
	      BSE_IF_DEBUG (MIDI)
		g_printerr ("MIDI: note-on: %u %+.2fHz %u %u (%u)\n", ch->notes[i].note, bse_note_to_freq (ch->notes[i].note), ch->notes[i].velocity, ch->notes[i].aftertouch, i);
	    }
	  else
	    {
	      BSE_IF_DEBUG (MIDI)
		g_printerr ("MIDI: note-on: %u %+.2fHz %u %u (unused midi channel %u)\n",
			    event->data.note.note, bse_note_to_freq (event->data.note.note),
			    event->data.note.velocity,
			    event->data.note.velocity,
			    event->channel);
	    }
	  break;
	case BSE_MIDI_NOTE_OFF:
	case BSE_MIDI_KEY_PRESSURE:
	  for (i = 0; i < ch->n_notes; i++)
	    if (ch->notes[i].note == event->data.note.note)
	      break;
	  if (i < ch->n_notes)
	    {
	      ch->notes[i].velocity = event->data.note.velocity;
	      if (event->status == BSE_MIDI_KEY_PRESSURE)
		ch->notes[i].aftertouch = event->data.note.velocity;
	      BSE_IF_DEBUG (MIDI)
		g_message ("MIDI: %s: %u %u %u (%u)",
			   event->status == BSE_MIDI_KEY_PRESSURE ? "key-pressure" : "note-off",
			   ch->notes[i].note, ch->notes[i].velocity, ch->notes[i].aftertouch, i);
	    }
	  else
	    {
	      g_message ("MIDI: %s: %u %u %u (unused midi channel %u)",
			 event->status == BSE_MIDI_KEY_PRESSURE ? "key-pressure" : "note-off",
			 event->data.note.note, event->data.note.velocity, event->data.note.velocity,
			 event->channel);
	    }
	  break;
	case BSE_MIDI_CONTROL_CHANGE:
	  if (ch->use_count)
	    {
	      ch->control_values[event->data.control.control] = event->data.control.value;
	      BSE_IF_DEBUG (MIDI)
		g_message ("MIDI: control_change[%u]: {%u} = %d\n",
			   event->channel + 1,
			   event->data.control.control,
			   event->data.control.value);
	    }
	  break;
	case BSE_MIDI_PROGRAM_CHANGE:
	  ch->program = event->data.program;
	  BSE_IF_DEBUG (MIDI)
	    g_message ("MIDI: program_change[%u]: %d\n", event->channel + 1, event->data.program);
	  break;
	case BSE_MIDI_CHANNEL_PRESSURE:
	  ch->pressure = event->data.intensity;
	  BSE_IF_DEBUG (MIDI)
	    g_message ("MIDI: pressure_change[%u]: %d\n", event->channel + 1, event->data.intensity);
	  break;
	case BSE_MIDI_PITCH_BEND:
	  ch->pitch_bend = event->data.pitch_bend;
	  BSE_IF_DEBUG (MIDI)
	    g_message ("MIDI: pitch_bend[%u]: 0x%04x\n", event->channel + 1, event->data.pitch_bend);
	  break;
	default:
	  BSE_IF_DEBUG (MIDI)
	    g_message ("MIDI: ignoring event 0x%02x", event->status);
	  break;
	}
    }
}

static void
decoder_process_events (BseMidiDecoder *decoder)
{
  BseMidiEvent *event, *next = NULL, *last = NULL;

  /* events were enqueued in opposite order so need to reverse */
  for (event = decoder->events; event; event = next)
    {
      next = event->next;
      event->next = last;
      last = event;
    }
  decoder->events = last;

  /* process events */
  for (event = decoder->events; event; event = next)
    {
      next = event->next;
      bse_midi_event_process (event, decoder->channels);
      bse_midi_free_event (event);
    }
  decoder->events = NULL;
}

BseMidiChannel*
_bse_midi_decoder_lock_channel (BseMidiDecoder *decoder,
				guint           channel)
{
  g_return_val_if_fail (decoder != NULL, NULL);
  g_return_val_if_fail (channel < BSE_MIDI_MAX_CHANNELS, NULL);

  // FIXME: need mutex locking here
  // FIXME: processing events as a hack here
  decoder_process_events (decoder);

  return decoder->channels + channel;
}

void
_bse_midi_decoder_unlock_channel (BseMidiDecoder *decoder,
				  BseMidiChannel *channel)
{
  g_return_if_fail (decoder != NULL);
  g_return_if_fail (channel != NULL);

  // FIXME: need mutex locking here
}

void
_bse_midi_decoder_use_channel (BseMidiDecoder *decoder,
			       guint           channel_indx)
{
  BseMidiChannel *channel;

  g_return_if_fail (decoder != NULL);
  g_return_if_fail (channel_indx < BSE_MIDI_MAX_CHANNELS);

  channel = _bse_midi_decoder_lock_channel (decoder, channel_indx);
  if (!channel->use_count)
    {
      channel->n_alloced_notes = 16;
      channel->notes = g_new0 (BseMidiNote, channel->n_alloced_notes);
      channel->control_values = g_new0 (guint, 128);
    }
  channel->use_count++;
  _bse_midi_decoder_unlock_channel (decoder, channel);
}

void
_bse_midi_decoder_unuse_channel (BseMidiDecoder *decoder,
				 guint           channel_indx)
{
  BseMidiChannel *channel;

  g_return_if_fail (decoder != NULL);
  g_return_if_fail (channel_indx < BSE_MIDI_MAX_CHANNELS);

  channel = _bse_midi_decoder_lock_channel (decoder, channel_indx);
  if (channel->use_count < 1)
    g_warning ("%s: use_count==0 for channel %u", G_STRLOC, channel_indx);
  else
    {
      channel->use_count--;
      if (!channel->use_count)
	{
	  g_free (channel->control_values);
	  channel->control_values = NULL;
	  g_free (channel->notes);
	  channel->notes = NULL;
	  channel->n_alloced_notes = 0;
	  channel->n_notes = 0;
	}
    }
  _bse_midi_decoder_unlock_channel (decoder, channel);
}
