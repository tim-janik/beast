/* BSE - Better Sound Engine
 * Copyright (C) 1996-2003 Tim Janik
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
#include "bsemidievent.hh"

#include "bseglue.hh"
#include "gslcommon.hh"
#include "bseieee754.hh"
#include "bsecxxplugin.hh"

#include <errno.h>


/* --- variables --- */
static GEnumClass      *bse_midi_signal_class = NULL;


/* --- functions --- */
/**
 * @param type BseMidiSignalType type
 *
 * Get the initial default value for a midi signal.
 * This function is MT-safe and may be called from any thread.
 */
float
bse_midi_signal_default (BseMidiSignalType type)
{
  switch (type)
    {
    case BSE_MIDI_SIGNAL_PITCH_BEND:	return 0.0;
    case BSE_MIDI_SIGNAL_CONTINUOUS_7:	return 100.0 / 127.0;   /* Volume */
    case BSE_MIDI_SIGNAL_CONTINUOUS_8:	return 0.5;	        /* Balance */
    case BSE_MIDI_SIGNAL_CONTINUOUS_10:	return 0.5;	        /* Panorama */
    case BSE_MIDI_SIGNAL_CONTINUOUS_11:	return 0x3f80 / (float) 0x3fff; /* Expression */
    case BSE_MIDI_SIGNAL_CONTROL_7:	return 100.0 / 127.0;	/* Volume MSB */
    case BSE_MIDI_SIGNAL_CONTROL_8:	return 0.5;	        /* Balance MSB */
    case BSE_MIDI_SIGNAL_CONTROL_10:	return 0.5;	        /* Panorama MSB */
    case BSE_MIDI_SIGNAL_CONTROL_11:	return 1.0;	        /* Expression MSB */
    case BSE_MIDI_SIGNAL_CONTROL_64:
      return BSE_GCONFIG (invert_sustain) ? 1.0 : 0.0;	        /* Damper Pedal Switch */
    case BSE_MIDI_SIGNAL_CONTROL_120:	return 1.0;	        /* All Sound Off ITrigger */
    case BSE_MIDI_SIGNAL_CONTROL_121:	return 1.0;	        /* All Controllers Off ITrigger */
    case BSE_MIDI_SIGNAL_CONTROL_123:	return 1.0;	        /* All Notes Off ITrigger */
    case BSE_MIDI_SIGNAL_CONTROL_124:	return 1.0;	        /* Omni Mode Off ITrigger */
    case BSE_MIDI_SIGNAL_CONTROL_125:	return 1.0;	        /* Omni Mode On ITrigger */
    case BSE_MIDI_SIGNAL_CONTROL_127:	return 1.0;	        /* Polyphonic Mode On ITrigger */
    case BSE_MIDI_SIGNAL_CONSTANT_HIGH:			return 1.0;
    case BSE_MIDI_SIGNAL_CONSTANT_CENTER:		return 0.5;
    case BSE_MIDI_SIGNAL_CONSTANT_LOW:			return 0.0;
    case BSE_MIDI_SIGNAL_CONSTANT_NEGATIVE_CENTER:	return -0.5;
    case BSE_MIDI_SIGNAL_CONSTANT_NEGATIVE_HIGH:	return -1.0;
    default:				return 0.0;
    }
}

const char*
bse_midi_signal_name (BseMidiSignalType signal)
{
  GEnumValue *ev;
  
  if (!bse_midi_signal_class)
    bse_midi_signal_class = (GEnumClass*) g_type_class_ref (BSE_TYPE_MIDI_SIGNAL_TYPE);
  
  ev = g_enum_get_value (bse_midi_signal_class, signal);
  return ev ? ev->value_name : NULL;
}

const char*
bse_midi_signal_nick (BseMidiSignalType signal)
{
  GEnumValue *ev;
  
  if (!bse_midi_signal_class)
    bse_midi_signal_class = (GEnumClass*) g_type_class_ref (BSE_TYPE_MIDI_SIGNAL_TYPE);
  
  ev = g_enum_get_value (bse_midi_signal_class, signal);
  return ev ? ev->value_nick : NULL;
}


/* --- BseMidiEvents --- */
/**
 * @param event BseMidiEvent structure
 *
 * Free the @a event and all data associated with it.
 * This function is MT-safe and may be called from any thread.
 */
void
bse_midi_free_event (BseMidiEvent *event)
{
  g_return_if_fail (event != NULL);
  g_return_if_fail (event->status != 0);
  
  switch (event->status)
    {
    case BSE_MIDI_MULTI_SYS_EX_START:
    case BSE_MIDI_MULTI_SYS_EX_NEXT:
    case BSE_MIDI_SYS_EX:
    case BSE_MIDI_SEQUENCER_SPECIFIC:
      g_free (event->data.sys_ex.bytes);
      break;
    case BSE_MIDI_TEXT_EVENT:
    case BSE_MIDI_COPYRIGHT_NOTICE:
    case BSE_MIDI_TRACK_NAME:
    case BSE_MIDI_INSTRUMENT_NAME:
    case BSE_MIDI_LYRIC:
    case BSE_MIDI_MARKER:
    case BSE_MIDI_CUE_POINT:
    case BSE_MIDI_TEXT_EVENT_08:
    case BSE_MIDI_TEXT_EVENT_09:
    case BSE_MIDI_TEXT_EVENT_0A:
    case BSE_MIDI_TEXT_EVENT_0B:
    case BSE_MIDI_TEXT_EVENT_0C:
    case BSE_MIDI_TEXT_EVENT_0D:
    case BSE_MIDI_TEXT_EVENT_0E:
    case BSE_MIDI_TEXT_EVENT_0F:
      g_free (event->data.text);
      break;
    default: ;
    }
  sfi_delete_struct (BseMidiEvent, event);
}

BseMidiEvent*
bse_midi_copy_event (const BseMidiEvent *src)
{
  BseMidiEvent *event;

  g_return_val_if_fail (src != NULL, NULL);

  event = bse_midi_alloc_event ();
  *event = *src;
  if (src->status == BSE_MIDI_SYS_EX)
    event->data.sys_ex.bytes = (uint8*) g_memdup (src->data.sys_ex.bytes, src->data.sys_ex.n_bytes);
  return event;
}

BseMidiEvent*
bse_midi_alloc_event (void)
{
  return sfi_new_struct0 (BseMidiEvent, 1);
}

BseMidiEvent*
bse_midi_event_note_on (uint   midi_channel,
			uint64 delta_time,
			float  frequency,
			float  velocity)
{
  BseMidiEvent *event;
  
  g_return_val_if_fail (frequency > 0 && frequency < BSE_MAX_FREQUENCY, NULL);
  g_return_val_if_fail (velocity >= 0 && velocity <= 1, NULL);
  g_return_val_if_fail (midi_channel > 0, NULL);
  
  event = bse_midi_alloc_event ();
  event->status = BSE_MIDI_NOTE_ON;
  event->channel = midi_channel;
  event->delta_time = delta_time;
  event->data.note.frequency = frequency;
  event->data.note.velocity = velocity;
  
  return event;
}

BseMidiEvent*
bse_midi_event_note_off (uint   midi_channel,
			 uint64 delta_time,
			 float  frequency)
{
  BseMidiEvent *event;
  
  g_return_val_if_fail (frequency > 0 && frequency < BSE_MAX_FREQUENCY, NULL);
  g_return_val_if_fail (midi_channel > 0, NULL);
  
  event = bse_midi_alloc_event ();
  event->status = BSE_MIDI_NOTE_OFF;
  event->channel = midi_channel;
  event->delta_time = delta_time;
  event->data.note.frequency = frequency;
  event->data.note.velocity = 0.0;
  
  return event;
}

BseMidiEvent*
bse_midi_event_signal (uint              midi_channel,
                       uint64            delta_time,
                       BseMidiSignalType signal_type,
                       float             value)
{
  BseMidiEvent *event;

  g_return_val_if_fail (value >= -1 && value <= +1, NULL);
  g_return_val_if_fail (midi_channel > 0, NULL);

  event = bse_midi_alloc_event ();
  switch (signal_type)
    {
    case BSE_MIDI_SIGNAL_PROGRAM:
      event->status = BSE_MIDI_PROGRAM_CHANGE;
      event->data.program = bse_ftoi (CLAMP (value, 0, 1) * 0x7f);
      break;
    case BSE_MIDI_SIGNAL_PRESSURE:
      event->status = BSE_MIDI_CHANNEL_PRESSURE;
      event->data.intensity = MAX (value, 0);
      break;
    case BSE_MIDI_SIGNAL_PITCH_BEND:
      event->status = BSE_MIDI_PITCH_BEND;
      event->data.pitch_bend = value;
      break;
    case BSE_MIDI_SIGNAL_VELOCITY:
    case BSE_MIDI_SIGNAL_FINE_TUNE:
    case BSE_MIDI_SIGNAL_CONSTANT_HIGH:
    case BSE_MIDI_SIGNAL_CONSTANT_CENTER:
    case BSE_MIDI_SIGNAL_CONSTANT_LOW:
    case BSE_MIDI_SIGNAL_CONSTANT_NEGATIVE_CENTER:
    case BSE_MIDI_SIGNAL_CONSTANT_NEGATIVE_HIGH:
      /* these are special signals that don't map to MIDI events */
      sfi_delete_struct (BseMidiEvent, event);
      return NULL;
    default:
      if (signal_type >= 128)   /* literal controls */
        {
          event->status = BSE_MIDI_CONTROL_CHANGE;
          event->data.control.control = signal_type - 128;
          event->data.control.value = value;
        }
      else /* continuous controls */
        {
          event->status = BSE_MIDI_X_CONTINUOUS_CHANGE;
          event->data.control.control = signal_type - 64;
          event->data.control.value = value;
        }
      break;
    }
  event->channel = midi_channel;
  event->delta_time = delta_time;
  return event;
}

static void *
boxed_copy_midi_event (void *boxed)
{
  BseMidiEvent *src = (BseMidiEvent*) boxed;
  BseMidiEvent *dest = bse_midi_copy_event (src);
  return dest;
}

static void
boxed_free_midi_event (void *boxed)
{
  BseMidiEvent *event = (BseMidiEvent*) boxed;
  bse_midi_free_event (event);
}

GType
bse_midi_event_get_type (void)
{
  static GType type = 0;
  if (!type)
    type = g_boxed_type_register_static ("BseMidiEvent", boxed_copy_midi_event, boxed_free_midi_event);
  return type;
}
