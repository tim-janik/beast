/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1996-1999, 2000-2003 Tim Janik
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
#include "bsemididecoder.h"
#include "bsemidireceiver.h"
#include "gslengine.h"
#include <string.h>

#define DEBUG   sfi_debug_keyfunc ("midi-decoder")
#define INFO    sfi_info_keyfunc ("midi-decoder")

#define BSE_MIDI_CHANNEL_VOICE_MESSAGE(s)       ((s) < 0xf0)
#define BSE_MIDI_SYSTEM_COMMON_MESSAGE(s)       (((s) & 0xf8) == 0xf0)
#define BSE_MIDI_SYSTEM_REALTIME_MESSAGE(s)     (((s) & 0xf8) == 0xf8)


/* --- prototypes --- */
static void     bse_midi_decoder_construct_event   (BseMidiDecoder *self,
                                                    guint64         tick_stamp);


/* --- function --- */
BseMidiDecoder*
bse_midi_decoder_new (gboolean     auto_queue)
{
  BseMidiDecoder *self;
  
  self = g_new0 (BseMidiDecoder, 1);
  self->events = NULL;
  self->event_type = 0;
  self->running_mode = 0;
  self->echannel = 0;
  self->n_bytes = 0;
  self->bytes = NULL;
  self->left_bytes = 0;
  self->auto_queue = auto_queue != FALSE;
  
  return self;
}

void
bse_midi_decoder_destroy (BseMidiDecoder *self)
{
  g_return_if_fail (self != NULL);
  
  while (self->events)
    {
      BseMidiEvent *event = sfi_ring_pop_head (&self->events);
      bse_midi_free_event (event);
    }
  g_free (self->bytes);
  g_free (self);
}

BseMidiEvent*
bse_midi_decoder_pop_event (BseMidiDecoder *self)
{
  g_return_val_if_fail (self != NULL, NULL);
  
  return sfi_ring_pop_head (&self->events);
}

void
bse_midi_decoder_push_data (BseMidiDecoder *self,
                            guint           n_bytes,
                            guint8         *bytes,
                            guint64         usec_systime)
{
  guint64 tick_stamp;
  
  g_return_if_fail (self != NULL);
  if (n_bytes)
    g_return_if_fail (bytes != NULL);
  
  tick_stamp = gsl_engine_tick_stamp_from_systime (usec_systime);
  
  while (n_bytes)
    {
      if (!self->event_type)    /* decode next event from byte stream */
        {
          guint status = *bytes;
          
          /* check for command/status byte */
          if (status & 0x80)
            {
              if (BSE_MIDI_CHANNEL_VOICE_MESSAGE (status))
                {
                  self->event_type = status & 0xf0;
                  self->echannel = status & 0x0f;
                  self->running_mode = status;          /* remember MIDI running mode */
                }
              else /* system-realtime or system-common */
                {
                  self->event_type = status;
                  self->echannel = ~0;
                  if (BSE_MIDI_SYSTEM_COMMON_MESSAGE (status))
                    self->running_mode = 0;              /* reset MIDI running mode */
                }
              /* we read the command byte */
              n_bytes--;
              bytes++;
            }
          else /* data byte, MIDI running mode command */
            {
              self->event_type = self->running_mode & 0xF0;
              self->echannel = self->running_mode & 0x0F;
            }
          /* setup data byte counter */
          switch (self->event_type)
            {
            case 0:
              /* ignore data byte as long as we don't know the running mode */
              n_bytes--;
              bytes++;
              self->left_bytes = 0;
              break;
            case BSE_MIDI_NOTE_OFF:
            case BSE_MIDI_NOTE_ON:
            case BSE_MIDI_KEY_PRESSURE:         self->left_bytes = 2;   break;
            case BSE_MIDI_CONTROL_CHANGE:       self->left_bytes = 2;   break;
            case BSE_MIDI_PROGRAM_CHANGE:
            case BSE_MIDI_CHANNEL_PRESSURE:     self->left_bytes = 1;   break;
            case BSE_MIDI_PITCH_BEND:           self->left_bytes = 2;   break;
            case BSE_MIDI_SYS_EX:               self->left_bytes = ~0;  break;
            case BSE_MIDI_SONG_POINTER:         self->left_bytes = 2;   break;
            case BSE_MIDI_SONG_SELECT:          self->left_bytes = 1;   break;
            case BSE_MIDI_TUNE:
            case BSE_MIDI_TIMING_CLOCK:
            case BSE_MIDI_SONG_START:
            case BSE_MIDI_SONG_CONTINUE:
            case BSE_MIDI_SONG_STOP:
            case BSE_MIDI_ACTIVE_SENSING:
            case BSE_MIDI_SYSTEM_RESET:         self->left_bytes = 0;   break;
            case BSE_MIDI_END_EX:
            default: /* probably bogus, inform user for debugging purposes */
              INFO ("%s: unhandled midi %s byte 0x%02X\n", G_STRLOC,
                    status < 0x80 ? "data" : "command", status);
              self->event_type = 0;
              self->left_bytes = 0;
              break;
            }
        }
      else if (self->left_bytes)        /* self->event_type != 0; read remaining command data */
        {
          /* special casing SYS_EX since we need to read up until end mark */
          if (self->event_type == BSE_MIDI_SYS_EX)
            {
              guint i;
              
              /* search for end mark */
              for (i = 0; i < n_bytes; i++)
                if (bytes[i] == BSE_MIDI_END_EX)
                  break;
              /* append data bytes */
              if (i)
                {
                  guint n = self->n_bytes;
                  self->n_bytes += i - 1;
                  self->bytes = g_renew (guint8, self->bytes, self->n_bytes + 1);
                  memcpy (self->bytes + n, bytes, i - 1);
                }
              n_bytes -= i;
              bytes += i;
              /* did we find end mark? */
              if (i < n_bytes)
                {
                  self->event_type = BSE_MIDI_END_EX;
                  self->left_bytes = 0;
                }
            }
          else  /* read normal event data bytes */
            {
              guint i = MIN (self->left_bytes, n_bytes);
              guint n = self->n_bytes;
              self->n_bytes += i;
              self->bytes = g_renew (guint8, self->bytes, self->n_bytes);
              memcpy (self->bytes + n, bytes, i);
              self->left_bytes -= i;
              n_bytes -= i;
              bytes += i;
            }
        }
      if (self->event_type != 0 && self->left_bytes == 0)
        bse_midi_decoder_construct_event (self, tick_stamp);
      if (self->auto_queue)
        while (self->events)
          {
            BseMidiEvent *event = sfi_ring_pop_head (&self->events);
            bse_midi_receiver_farm_distribute_event (event);
            bse_midi_free_event (event);
          }
    }
  if (self->auto_queue)
    bse_midi_receiver_farm_process_events (tick_stamp);
}

static void
bse_midi_decoder_construct_event (BseMidiDecoder *self,
                                  guint64         tick_stamp)
{
  BseMidiEvent *event;
  
  g_return_if_fail (self->event_type & 0x80);
  g_return_if_fail (self->left_bytes == 0);
  
  /* special case completed SysEx */
  if (self->event_type == BSE_MIDI_END_EX)
    self->event_type = BSE_MIDI_SYS_EX;
  event = bse_midi_alloc_event ();
  event->status = self->event_type;
  event->channel = self->echannel + 1;
  event->tick_stamp = tick_stamp;
  switch (event->status)
    {
      guint v;
      gint ival;
    case BSE_MIDI_NOTE_OFF:
    case BSE_MIDI_NOTE_ON:
    case BSE_MIDI_KEY_PRESSURE:
      g_return_if_fail (self->n_bytes == 2);
      event->data.note.frequency = bse_note_to_freq (self->bytes[0] & 0x7f);
      ival = self->bytes[1] & 0x7f;
      /* old MIDI devices send velocity as 0 instead of note-off */
      if (event->status == BSE_MIDI_NOTE_ON && ival == 0)
        event->status = BSE_MIDI_NOTE_OFF;
      /* some MIDI devices report junk velocity upon note-off */
      if (event->status == BSE_MIDI_NOTE_OFF)
        event->data.note.velocity = 0;
      else /* note-on or key-pressure */
        event->data.note.velocity = ival / (gfloat) 0x7f;
      DEBUG ("ch-%02x: note-%s: freq=%.3f velocity=%.4f", event->channel,
             event->status == BSE_MIDI_NOTE_ON ? "on" :
             event->status == BSE_MIDI_NOTE_OFF ? "off" : "pressure",
             event->data.note.frequency, event->data.note.velocity);
      break;
    case BSE_MIDI_CONTROL_CHANGE:
      g_return_if_fail (self->n_bytes == 2);
      event->data.control.control = self->bytes[0] & 0x7f;
      ival = self->bytes[1] & 0x7f;
      event->data.control.value = ival / (gfloat) 0x7f;
      DEBUG ("ch-%02x: control[%02u]: %.4f (0x%02x)", event->channel,
             event->data.control.control,
             event->data.control.value, ival);
      break;
    case BSE_MIDI_PROGRAM_CHANGE:
      g_return_if_fail (self->n_bytes == 1);
      event->data.program = self->bytes[0] & 0x7f;
      DEBUG ("ch-%02x: program-change: 0x%02x", event->channel, event->data.program);
      break;
    case BSE_MIDI_CHANNEL_PRESSURE:     /* 0..0x7f */
      g_return_if_fail (self->n_bytes == 1);
      ival = self->bytes[0] & 0x7f;
      event->data.intensity = ival / (gfloat) 0x7f;
      DEBUG ("ch-%02x: channel-pressure: %.4f (0x%02x)", event->channel, event->data.intensity, ival);
      break;
    case BSE_MIDI_PITCH_BEND:   /* 0..0x3fff; center: 0x2000 */
      g_return_if_fail (self->n_bytes == 2);
      ival = self->bytes[0] & 0x7f;
      v = self->bytes[1] & 0x7f;
      ival |= v << 7;
      ival -= 0x2000;   /* pitch bend center */
      event->data.pitch_bend = ival / (gfloat) 0x2000;
      DEBUG ("ch-%02x: pitch-bend: %.4f (0x%04x)", event->channel, event->data.pitch_bend, ival);
      break;
    case BSE_MIDI_SYS_EX:
      event->data.sys_ex.n_bytes = self->n_bytes;
      event->data.sys_ex.bytes = self->bytes;
      self->bytes = NULL;
      DEBUG ("ch-%02x: sys-ex: %u bytes", event->channel, self->n_bytes);
      break;
    case BSE_MIDI_SONG_POINTER:
      g_return_if_fail (self->n_bytes == 2);
      event->data.song_pointer = self->bytes[0] & 0x7f;
      v = self->bytes[1] & 0x7f;
      event->data.song_pointer |= v << 7;
      DEBUG ("ch-%02x: song-pointer: 0x%04x", event->channel, event->data.song_pointer);
      break;
    case BSE_MIDI_SONG_SELECT:
      g_return_if_fail (self->n_bytes == 1);
      event->data.song_number = self->bytes[0] & 0x7f;
      DEBUG ("ch-%02x: song-select: 0x%02x", event->channel, event->data.song_number);
      break;
    default:
      event->data.sys_ex.n_bytes = 0;
      event->data.sys_ex.bytes = NULL;
      break;
    }
  self->n_bytes = 0;
  self->event_type = 0;
  
  self->events = sfi_ring_append (self->events, event);
}
