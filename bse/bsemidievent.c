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

#include "bseglue.h"
#include "bsemidinotifier.h"
#include "gslcommon.h"

#include <errno.h>


/* --- prototypes --- */
static void		decoder_read_event_data_ASYNC	(BseMidiDecoder	*decoder,
							 guint		*n_bytes_p,
							 guint8	       **bytes_p);
static BseMidiEvent*	decoder_extract_event_ASYNC	(BseMidiDecoder	*decoder,
							 guint64	 usec_time);
static void		decoder_enqueue_event_ASYNC	(BseMidiDecoder	*decoder,
							 BseMidiEvent	*event);
static void		midi_event_process_ASYNC	(BseMidiEvent  *event,
							 BseMidiChannel channels[BSE_MIDI_MAX_CHANNELS]);


/* --- variables --- */
static GslMutex         notifier_event_mutex = { 0, };
static BseMidiEvent    *notifier_event_queue = NULL;
static BseMidiNotifier *midi_notifier = NULL;


/* --- functions --- */
BseMidiDecoder*
bse_midi_decoder_new (void)
{
  BseMidiDecoder *decoder = g_new0 (BseMidiDecoder, 1);
  guint i;
  static guint mutex_initialized = FALSE;

  if (!mutex_initialized)
    {
      mutex_initialized = TRUE;
      gsl_mutex_init (&notifier_event_mutex);
    }

  decoder->n_channels = BSE_MIDI_MAX_CHANNELS;
  for (i = 0; i < decoder->n_channels; i++)
    {
      decoder->channels[i].pitch_bend = 0x2000;
      decoder->channels[i].control_values[7 /* Volume */] = 102; /* 80% */
      decoder->channels[i].control_values[8 /* Balance */] = 64; /* 50% */
    }
  gsl_mutex_init (&decoder->mutex);

  return decoder;
}

void
bse_midi_decoder_destroy (BseMidiDecoder *decoder)
{
  guint i;

  g_return_if_fail (decoder != NULL);

  for (i = 0; i < decoder->n_channels; i++)
    g_free (decoder->channels[i].notes);
  while (decoder->events)
    {
      BseMidiEvent *event = decoder->events;
      decoder->events = event->next;
      event->next = NULL;
      _bse_midi_free_event_ASYNC (event);
    }
  g_free (decoder->bytes);
  gsl_mutex_destroy (&decoder->mutex);
  g_free (decoder);
}

void
_bse_midi_decoder_push_data_ASYNC (BseMidiDecoder *decoder,
				   guint           n_bytes,
				   guint8         *bytes,
				   guint64         usec_time)
{
  g_return_if_fail (decoder != NULL);
  if (n_bytes)
    g_return_if_fail (bytes != NULL);

  GSL_SPIN_LOCK (&decoder->mutex);

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
	decoder_read_event_data_ASYNC (decoder, &n_bytes, &bytes);
      /* extract event */
      if (decoder->status && decoder->left_bytes == 0)
	decoder_enqueue_event_ASYNC (decoder,
				     decoder_extract_event_ASYNC (decoder, usec_time));
    }

  GSL_SPIN_UNLOCK (&decoder->mutex);
}

static void
decoder_read_event_data_ASYNC (BseMidiDecoder *decoder,
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
decoder_extract_event_ASYNC (BseMidiDecoder *decoder,
			     guint64         usec_time)
{
  BseMidiEvent *event;

  g_return_val_if_fail (decoder->status & 0x80, NULL);
  g_return_val_if_fail (decoder->left_bytes == 0, NULL);

  /* special case completed SysEx */
  if (decoder->status == BSE_MIDI_END_EX)
    decoder->status = BSE_MIDI_SYS_EX;
  event = gsl_new_struct (BseMidiEvent, 1);
  event->status = decoder->status;
  event->channel = decoder->echannel;
  event->usec_stamp = usec_time;
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
decoder_enqueue_event_ASYNC (BseMidiDecoder *decoder,
			     BseMidiEvent   *event)
{
  g_return_if_fail (event != NULL);
  g_return_if_fail (event->next == NULL);

  /* enqueing in opposite order */
  event->next = decoder->events;
  decoder->events = event;
}

void
_bse_midi_free_event_ASYNC (BseMidiEvent *event)
{
  g_return_if_fail (event != NULL);
  g_return_if_fail (event->next == NULL);
  g_return_if_fail (event->status != 0);

  if (event->status == BSE_MIDI_SYS_EX)
    g_free (event->data.sys_ex.bytes);
  event->status = 0;
  gsl_delete_struct (BseMidiEvent, event);
}

static void
midi_event_process_ASYNC (BseMidiEvent  *event,
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

void
_bse_midi_set_notifier (BseMidiNotifier *notifier)
{
  midi_notifier = notifier;
}

BseMidiNotifier*
_bse_midi_get_notifier (void)
{
  return midi_notifier;
}

static void
decoder_process_events_ASYNC (BseMidiDecoder *decoder)
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
  while (decoder->events)
    {
      event = decoder->events;
      decoder->events = event->next;
      event->next = NULL;
      midi_event_process_ASYNC (event, decoder->channels);
      if (midi_notifier)
	{
	  GSL_SPIN_LOCK (&notifier_event_mutex);
	  event->next = notifier_event_queue;
	  notifier_event_queue = event;
	  GSL_SPIN_UNLOCK (&notifier_event_mutex);
	}
      else
	_bse_midi_free_event_ASYNC (event);
    }
}

BseMidiChannel*
bse_midi_decoder_lock_channel_ASYNC (BseMidiDecoder *decoder,
				     guint           channel)
{
  g_return_val_if_fail (decoder != NULL, NULL);
  g_return_val_if_fail (channel < BSE_MIDI_MAX_CHANNELS, NULL);

  GSL_SPIN_LOCK (&decoder->mutex);

  /* make sure all events so far are processed */
  decoder_process_events_ASYNC (decoder);

  return decoder->channels + channel;
}

void
bse_midi_decoder_unlock_channel_ASYNC (BseMidiDecoder *decoder,
				       BseMidiChannel *channel)
{
  g_return_if_fail (decoder != NULL);
  g_return_if_fail (channel != NULL);

  GSL_SPIN_UNLOCK (&decoder->mutex);

  /* wake up midi notifer if necessary */
  if (notifier_event_queue)
    gsl_thread_wakeup (gsl_thread_main ());
}

BseMidiEvent*
_bse_midi_fetch_notify_events_ASYNC (void)
{
  BseMidiEvent *events;

  GSL_SPIN_LOCK (&notifier_event_mutex);
  events = notifier_event_queue;
  notifier_event_queue = NULL;
  GSL_SPIN_UNLOCK (&notifier_event_mutex);

  return events;
}

gboolean
_bse_midi_has_notify_events_ASYNC (void)
{
  /* prolly don't need a lock */
  return notifier_event_queue != NULL;
}

void
bse_midi_decoder_use_channel (BseMidiDecoder *decoder,
			      guint           channel_indx)
{
  BseMidiChannel *channel;

  g_return_if_fail (decoder != NULL);
  g_return_if_fail (channel_indx < BSE_MIDI_MAX_CHANNELS);

  channel = bse_midi_decoder_lock_channel_ASYNC (decoder, channel_indx);
  if (!channel->use_count)
    {
      channel->n_alloced_notes = 16;
      channel->notes = g_new0 (BseMidiNote, channel->n_alloced_notes);
    }
  channel->use_count++;
  bse_midi_decoder_unlock_channel_ASYNC (decoder, channel);
}

void
bse_midi_decoder_unuse_channel (BseMidiDecoder *decoder,
				guint           channel_indx)
{
  BseMidiChannel *channel;

  g_return_if_fail (decoder != NULL);
  g_return_if_fail (channel_indx < BSE_MIDI_MAX_CHANNELS);

  channel = bse_midi_decoder_lock_channel_ASYNC (decoder, channel_indx);
  if (channel->use_count < 1)
    g_warning ("%s: use_count==0 for channel %u", G_STRLOC, channel_indx);
  else
    {
      channel->use_count--;
      if (!channel->use_count)
	{
	  g_free (channel->notes);
	  channel->notes = NULL;
	  channel->n_alloced_notes = 0;
	  channel->n_notes = 0;
	}
    }
  bse_midi_decoder_unlock_channel_ASYNC (decoder, channel);
}

static gpointer
boxed_copy_midi_event (gpointer boxed)
{
  BseMidiEvent *src = boxed;
  BseMidiEvent *dest = g_new (BseMidiEvent, 1);

  *dest = *src;
  if (dest->status == BSE_MIDI_SYS_EX)
    {
      dest->data.sys_ex.n_bytes = 0;
      dest->data.sys_ex.bytes = NULL;
    }
  dest->next = NULL;
  return dest;
}

static void
boxed_free_midi_event (gpointer boxed)
{
  BseMidiEvent *event = boxed;
  
  g_free (event);
}

static GslGlueRec*
midi_event_to_record (gpointer crecord)
{
  BseMidiEvent *event = crecord;
  GslGlueValue val;
  GslGlueRec *rec;

  rec = gsl_glue_rec ();
  /* status */
  val = gsl_glue_value_enum (g_type_name (BSE_TYPE_MIDI_EVENT_TYPE),
			     bse_glue_enum_index (BSE_TYPE_MIDI_EVENT_TYPE, event->status));
  gsl_glue_rec_take_append (rec, &val);
  /* channel */
  val = gsl_glue_value_int (event->channel);
  gsl_glue_rec_take_append (rec, &val);
  /* msec_stamp */
  val = gsl_glue_value_int (event->usec_stamp / 1000);
  gsl_glue_rec_take_append (rec, &val);
  switch (event->status)
    {
    case BSE_MIDI_NOTE_OFF:
    case BSE_MIDI_NOTE_ON:
    case BSE_MIDI_KEY_PRESSURE:
      val = gsl_glue_value_int (event->data.note.note);
      gsl_glue_rec_take_append (rec, &val);
      val = gsl_glue_value_int (event->data.note.velocity);
      gsl_glue_rec_take_append (rec, &val);
      break;
    case BSE_MIDI_CONTROL_CHANGE:
      val = gsl_glue_value_int (event->data.control.control);
      gsl_glue_rec_take_append (rec, &val);
      val = gsl_glue_value_int (event->data.control.value);
      gsl_glue_rec_take_append (rec, &val);
      break;
    case BSE_MIDI_PROGRAM_CHANGE:
      val = gsl_glue_value_int (event->data.program);
      gsl_glue_rec_take_append (rec, &val);
      val = gsl_glue_value_int (0);
      gsl_glue_rec_take_append (rec, &val);
      break;
    case BSE_MIDI_CHANNEL_PRESSURE:
      val = gsl_glue_value_int (event->data.intensity);
      gsl_glue_rec_take_append (rec, &val);
      val = gsl_glue_value_int (0);
      gsl_glue_rec_take_append (rec, &val);
      break;
    case BSE_MIDI_PITCH_BEND:
      val = gsl_glue_value_int (event->data.pitch_bend);
      gsl_glue_rec_take_append (rec, &val);
      val = gsl_glue_value_int (0);
      gsl_glue_rec_take_append (rec, &val);
      break;
    case BSE_MIDI_SONG_POINTER:
      val = gsl_glue_value_int (event->data.song_pointer);
      gsl_glue_rec_take_append (rec, &val);
      val = gsl_glue_value_int (0);
      gsl_glue_rec_take_append (rec, &val);
      break;
    case BSE_MIDI_SONG_SELECT:
      val = gsl_glue_value_int (event->data.song_number);
      gsl_glue_rec_take_append (rec, &val);
      val = gsl_glue_value_int (0);
      gsl_glue_rec_take_append (rec, &val);
      break;
    case BSE_MIDI_SYS_EX:
    case BSE_MIDI_TUNE:
    case BSE_MIDI_TIMING_CLOCK:
    case BSE_MIDI_SONG_START:
    case BSE_MIDI_SONG_CONTINUE:
    case BSE_MIDI_SONG_STOP:
    case BSE_MIDI_ACTIVE_SENSING:
    case BSE_MIDI_SYSTEM_RESET:
    case BSE_MIDI_END_EX:
    default:
      val = gsl_glue_value_int (0);
      gsl_glue_rec_take_append (rec, &val);
      val = gsl_glue_value_int (0);
      gsl_glue_rec_take_append (rec, &val);
      break;
    }
  return rec;
}

GType
bse_midi_event_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = bse_glue_make_rorecord ("BseMidiEvent", boxed_copy_midi_event, boxed_free_midi_event, midi_event_to_record);

  return type;
}
