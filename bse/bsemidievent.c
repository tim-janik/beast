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
#include "gslcommon.h"

#include <errno.h>


/* --- variables --- */
static GEnumClass      *bse_midi_signal_class = NULL;


/* --- functions --- */
/**
 * bse_midi_signal_default
 * @type: BseMidiSignalType type
 *
 * Get the initial default value for a midi signal.
 * This function is MT-safe and may be called from any thread.
 */
gfloat
bse_midi_signal_default (BseMidiSignalType type)
{
  switch (type)
    {
    case BSE_MIDI_SIGNAL_PITCH_BEND:	return 0.0;
    case BSE_MIDI_SIGNAL_CONTINUOUS_7:	return 0.8;	/* Volume */
    case BSE_MIDI_SIGNAL_CONTINUOUS_8:	return 0.5;	/* Balance */
    case BSE_MIDI_SIGNAL_CONTROL_7:	return 0.8;	/* Volume MSB */
    case BSE_MIDI_SIGNAL_CONTROL_8:	return 0.5;	/* Balance MSB */
    case BSE_MIDI_SIGNAL_CONTROL_120:	return 1.0;	/* All Sound Off ITrigger */
    case BSE_MIDI_SIGNAL_CONTROL_121:	return 1.0;	/* All Controllers Off ITrigger */
    case BSE_MIDI_SIGNAL_CONTROL_123:	return 1.0;	/* All Notes Off ITrigger */
    case BSE_MIDI_SIGNAL_CONTROL_124:	return 1.0;	/* Omni Mode Off ITrigger */
    case BSE_MIDI_SIGNAL_CONTROL_125:	return 1.0;	/* Omni Mode On ITrigger */
    case BSE_MIDI_SIGNAL_CONTROL_127:	return 1.0;	/* Polyphonic Mode On ITrigger */
    case BSE_MIDI_SIGNAL_CONSTANT_HIGH:			return 1.0;
    case BSE_MIDI_SIGNAL_CONSTANT_CENTER:		return 0.5;
    case BSE_MIDI_SIGNAL_CONSTANT_LOW:			return 0.0;
    case BSE_MIDI_SIGNAL_CONSTANT_NEGATIVE_CENTER:	return -0.5;
    case BSE_MIDI_SIGNAL_CONSTANT_NEGATIVE_HIGH:	return -1.0;
    default:				return 0.0;
    }
}

const gchar*
bse_midi_signal_name (BseMidiSignalType signal)
{
  GEnumValue *ev;

  if (!bse_midi_signal_class)
    bse_midi_signal_class = g_type_class_ref (BSE_TYPE_MIDI_SIGNAL_TYPE);

  ev = g_enum_get_value (bse_midi_signal_class, signal);
  return ev ? ev->value_name : NULL;
}

const gchar*
bse_midi_signal_nick (BseMidiSignalType signal)
{
  GEnumValue *ev;

  if (!bse_midi_signal_class)
    bse_midi_signal_class = g_type_class_ref (BSE_TYPE_MIDI_SIGNAL_TYPE);

  ev = g_enum_get_value (bse_midi_signal_class, signal);
  return ev ? ev->value_nick : NULL;
}


/* --- BseMidiEvents --- */
/**
 * bse_midi_free_event
 * @event: BseMidiEvent structure
 *
 * Free the @event and all data associated with it.
 * This function is MT-safe and may be called from any thread.
 */
void
bse_midi_free_event (BseMidiEvent *event)
{
  g_return_if_fail (event != NULL);
  g_return_if_fail (event->status != 0);

  if (event->status == BSE_MIDI_SYS_EX)
    g_free (event->data.sys_ex.bytes);
  gsl_delete_struct (BseMidiEvent, event);
}

BseMidiEvent*
bse_midi_event_note_on (guint   midi_channel,
			guint64 tick_stamp,
			gfloat  frequency,
			gfloat  velocity)
{
  BseMidiEvent *event;

  g_return_val_if_fail (midi_channel < BSE_MIDI_MAX_CHANNELS, NULL);
  g_return_val_if_fail (frequency > 0 && frequency < BSE_MAX_FREQUENCY_f, NULL);
  g_return_val_if_fail (velocity >= 0 && velocity <= 1, NULL);

  event = gsl_new_struct (BseMidiEvent, 1);
  event->status = BSE_MIDI_NOTE_ON;
  event->channel = midi_channel;
  event->tick_stamp = tick_stamp;
  event->data.note.frequency = frequency;
  event->data.note.velocity = velocity;

  return event;
}

BseMidiEvent*
bse_midi_event_note_off (guint   midi_channel,
			 guint64 tick_stamp,
			 gfloat  frequency)
{
  BseMidiEvent *event;

  g_return_val_if_fail (midi_channel < BSE_MIDI_MAX_CHANNELS, NULL);
  g_return_val_if_fail (frequency > 0 && frequency < BSE_MAX_FREQUENCY_f, NULL);

  event = gsl_new_struct (BseMidiEvent, 1);
  event->status = BSE_MIDI_NOTE_OFF;
  event->channel = midi_channel;
  event->tick_stamp = tick_stamp;
  event->data.note.frequency = frequency;
  event->data.note.velocity = 0.0;

  return event;
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
  return dest;
}

static void
boxed_free_midi_event (gpointer boxed)
{
  BseMidiEvent *event = boxed;

  bse_midi_free_event (event);
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
  gsl_glue_rec_take (rec, "status", &val);
  /* channel */
  val = gsl_glue_value_int (event->channel);
  gsl_glue_rec_take (rec, "channel", &val);
  /* msec_stamp */
  val = gsl_glue_value_int (event->tick_stamp / 1000); // FIXME: broken tick_stamp
  gsl_glue_rec_take (rec, "stamp", &val); // FIXME: "tick_stamp"
  switch (event->status)
    {
    case BSE_MIDI_NOTE_OFF:
    case BSE_MIDI_NOTE_ON:
    case BSE_MIDI_KEY_PRESSURE:
      val = gsl_glue_value_float (event->data.note.frequency);
      gsl_glue_rec_take (rec, "data1", &val);
      val = gsl_glue_value_float (event->data.note.velocity);
      gsl_glue_rec_take (rec, "data2", &val);
      break;
    case BSE_MIDI_CONTROL_CHANGE:
      val = gsl_glue_value_int (event->data.control.control);
      gsl_glue_rec_take (rec, "data1", &val);
      val = gsl_glue_value_float (event->data.control.value);
      gsl_glue_rec_take (rec, "data2", &val);
      break;
    case BSE_MIDI_PROGRAM_CHANGE:
      val = gsl_glue_value_int (event->data.program);
      gsl_glue_rec_take (rec, "data1", &val);
      val = gsl_glue_value_int (0);
      gsl_glue_rec_take (rec, "data2", &val);
      break;
    case BSE_MIDI_CHANNEL_PRESSURE:
      val = gsl_glue_value_float (event->data.intensity);
      gsl_glue_rec_take (rec, "data1", &val);
      val = gsl_glue_value_int (0);
      gsl_glue_rec_take (rec, "data2", &val);
      break;
    case BSE_MIDI_PITCH_BEND:
      val = gsl_glue_value_float (event->data.pitch_bend);
      gsl_glue_rec_take (rec, "data1", &val);
      val = gsl_glue_value_int (0);
      gsl_glue_rec_take (rec, "data2", &val);
      break;
    case BSE_MIDI_SONG_POINTER:
      val = gsl_glue_value_int (event->data.song_pointer);
      gsl_glue_rec_take (rec, "data1", &val);
      val = gsl_glue_value_int (0);
      gsl_glue_rec_take (rec, "data1", &val);
      break;
    case BSE_MIDI_SONG_SELECT:
      val = gsl_glue_value_int (event->data.song_number);
      gsl_glue_rec_take (rec, "data1", &val);
      val = gsl_glue_value_int (0);
      gsl_glue_rec_take (rec, "data2", &val);
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
      gsl_glue_rec_take (rec, "data1", &val);
      val = gsl_glue_value_int (0);
      gsl_glue_rec_take (rec, "data2", &val);
      break;
    }
  return rec;
}

GType
bse_midi_event_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = bse_glue_make_rorecord ("BseMidiEvent",
				   boxed_copy_midi_event,
				   boxed_free_midi_event,
				   midi_event_to_record);

  return type;
}
