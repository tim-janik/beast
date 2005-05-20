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
#include "bseengine.h"
#include <string.h>

static SFI_MSG_TYPE_DEFINE (debug_midi_decoder, "midi-decoder", SFI_MSG_DEBUG, NULL);
#define DEBUG(...)      sfi_debug (debug_midi_decoder, __VA_ARGS__)

/* --- prototypes --- */
static void     bse_midi_decoder_construct_event   (BseMidiDecoder *self);


/* --- function --- */
BseMidiDecoder*
bse_midi_decoder_new (gboolean auto_queue,
                      gboolean smf_support)
{
  BseMidiDecoder *self;
  
  self = g_new0 (BseMidiDecoder, 1);
  self->auto_queue = auto_queue != FALSE;
  self->smf_support = smf_support != FALSE;
  self->state_changed = FALSE;
  self->state = BSE_MIDI_DECODER_ZERO;
  self->delta_time = 0;
  self->event_type = 0;
  self->running_mode = 0;
  self->zchannel = 0;
  self->left_bytes = 0;
  self->n_bytes = 0;
  self->bytes = NULL;
  
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

SfiRing*
bse_midi_decoder_pop_event_list (BseMidiDecoder *self)
{
  SfiRing *events;
  g_return_val_if_fail (self != NULL, NULL);
  events = self->events;
  self->events = NULL;
  return events;
}

static inline const char*
decoder_state_to_string (BseMidiDecoderState state)
{
  switch (state)
    {
    case BSE_MIDI_DECODER_ZERO:         return "zero";
    case BSE_MIDI_DECODER_DELTA_TIME:   return "delta-time";
    case BSE_MIDI_DECODER_EVENT:        return "event";
    case BSE_MIDI_DECODER_VLENGTH:      return "vlength";
    case BSE_MIDI_DECODER_DATA:         return "data";
    case BSE_MIDI_DECODER_DONE:         return "done";
    }
  return "unknown";
}

static void
midi_decoder_advance_state (BseMidiDecoder *self)
{
  BseMidiDecoderState next_state = self->state + 1;
  next_state = next_state <= BSE_MIDI_DECODER_DONE ? next_state : BSE_MIDI_DECODER_ZERO;
  if (next_state == BSE_MIDI_DECODER_ZERO)
    {
      /* do the usual initialization */
      self->delta_time = 0;
      self->event_type = 0;
      /* keep running_mode and zchannel */
      g_assert (self->left_bytes == 0);
      if (self->n_bytes)
        g_warning ("leaking %d bytes of midi data", self->n_bytes);
      self->n_bytes = 0;
    }
  self->state = next_state;
  self->state_changed = TRUE;
}

static inline void
midi_decoder_next_state (BseMidiDecoder     *self,
                         BseMidiDecoderState next_state)
{
  next_state = next_state <= BSE_MIDI_DECODER_DONE ? next_state : BSE_MIDI_DECODER_ZERO;
  while (self->state != next_state)
    midi_decoder_advance_state (self);
}

typedef struct {
  guint8 *bytes;
  guint8 *bound;
  guint64 delta_time;
} Data;

static inline void
midi_decoder_parse_data (BseMidiDecoder *self,
                         Data           *d)
{
  switch (self->state)
    {
      BseMidiDecoderState next_state;
      guint v;
    case BSE_MIDI_DECODER_ZERO:
      if (d->bytes < d->bound)
        midi_decoder_advance_state (self);
      break;
    case BSE_MIDI_DECODER_DELTA_TIME:
      if (d->bytes >= d->bound)
        break;
      next_state = BSE_MIDI_DECODER_EVENT;
      if (self->smf_support)
        {
          v = *d->bytes++;
          self->delta_time = (self->delta_time << 7) + (v & 0x7f);
          if (v & 0x80) /* need more bytes */
            next_state = BSE_MIDI_DECODER_DELTA_TIME;
        }
      else
        {
          self->delta_time = d->delta_time;
        }
      midi_decoder_next_state (self, next_state);
      break;
    case BSE_MIDI_DECODER_EVENT:
      if (d->bytes >= d->bound)
        break;
      v = *d->bytes++;
      next_state = BSE_MIDI_DECODER_VLENGTH;
      /* check status byte (data/command) */
      if (self->event_type == 0xFF)             /* special case, second half of meta event in smf_support */
        {
          self->event_type = BSE_MIDI_SEQUENCE_NUMBER + v;
        }
      else if (!(v & 0x80))                                     /* data, MIDI running mode command */
        {
          self->event_type = self->running_mode;
          /* self->zchannel also used by running mode */
          if (self->event_type)
            d->bytes--; /* push back data byte */
          else  /* unknown running mode */
            {
              /* skip data byte and start over */
              next_state = BSE_MIDI_DECODER_ZERO;
            }
        }
      else if (BSE_MIDI_CHANNEL_VOICE_MESSAGE (v))              /* ordinary MIDI command */
        {
          self->event_type = v & 0xf0;
          self->zchannel = v & 0x0f;
          self->running_mode = self->event_type;
          /* self->zchannel also used by running mode */
        }
      else if (self->smf_support && v == 0xF0)                  /* SMF Sys-Ex */
        {
          self->event_type = BSE_MIDI_MULTI_SYS_EX_START;
          self->running_mode = 0;
          /* keep self->zchannel */
        }
      else if (self->smf_support && v == 0xF7)                  /* SMF Sys-Ex Escape */
        {
          self->event_type = BSE_MIDI_MULTI_SYS_EX_NEXT;
          self->running_mode = 0;
          /* keep self->zchannel */
        }
      else if (self->smf_support && v == 0xFF)                  /* SMF Meta-Event, first half */
        {
          /* need second byte */
          next_state = BSE_MIDI_DECODER_EVENT;
          self->event_type = 0xFF;
          self->running_mode = 0;
          /* keep self->zchannel */
        }
      else if (BSE_MIDI_SYSTEM_COMMON_MESSAGE (v))              /* system-common */
        {
          self->event_type = v;
          self->running_mode = 0;
          /* keep self->zchannel */
        }
      else /* BSE_MIDI_SYSTEM_REALTIME_MESSAGE (v) */           /* system-realtime */
        {
          self->event_type = v;
          /* keep running mode */
        }
      midi_decoder_next_state (self, next_state);
      break;
    case BSE_MIDI_DECODER_VLENGTH:
      next_state = BSE_MIDI_DECODER_DATA;
      /* setup data byte counter */
      if (self->event_type >= 0x100)    /* all meta events */
        {
          if (d->bytes >= d->bound)
            break;
          v = *d->bytes++;
          self->left_bytes = (self->left_bytes << 7) + (v & 0x7f);
          if (v & 0x80) /* need more bytes */
            next_state = BSE_MIDI_DECODER_VLENGTH;
        }
      else
        switch (self->event_type)
          {
          case BSE_MIDI_NOTE_OFF:     case BSE_MIDI_NOTE_ON:
          case BSE_MIDI_KEY_PRESSURE: case BSE_MIDI_CONTROL_CHANGE:
          case BSE_MIDI_PITCH_BEND:   case BSE_MIDI_SONG_POINTER:
            self->left_bytes = 2;
            break;
          case BSE_MIDI_SONG_SELECT:  case BSE_MIDI_PROGRAM_CHANGE:
          case BSE_MIDI_CHANNEL_PRESSURE:
            self->left_bytes = 1;
            break;
          case BSE_MIDI_TUNE:         case BSE_MIDI_TIMING_CLOCK:
          case BSE_MIDI_SONG_START:   case BSE_MIDI_SONG_CONTINUE:
          case BSE_MIDI_SONG_STOP:    case BSE_MIDI_ACTIVE_SENSING:
          case BSE_MIDI_SYSTEM_RESET:
            self->left_bytes = 0;
            break;
          case BSE_MIDI_SYS_EX:
            self->left_bytes = ~0;      /* search for BSE_MIDI_END_EX */
            break;
          case BSE_MIDI_END_EX:
          default: /* probably bogus, inform user for debugging purposes */
            sfi_diag ("BseMidiDecoder: unhandled midi %s byte 0x%02X\n",
                      self->event_type < 0x80 ? "data" : "command", self->event_type);
            self->event_type = 0;             /* start over */
            next_state = BSE_MIDI_DECODER_ZERO;
            break;
          }
      midi_decoder_next_state (self, next_state);
      break;
    case BSE_MIDI_DECODER_DATA:
      if (self->event_type == BSE_MIDI_SYS_EX)
        {       /* special casing SYS_EX since we need to read up until end mark */
          guint8 *p = memchr (d->bytes, BSE_MIDI_END_EX, d->bound - d->bytes);
          p = p ? p : d->bound;
          if (p > d->bytes)     /* append data bytes */
            {
              guint n = self->n_bytes, l = p - d->bytes;
              self->n_bytes += l;
              self->bytes = g_renew (guint8, self->bytes, self->n_bytes);
              memcpy (self->bytes + n, d->bytes, l);
            }
          d->bytes = p;
          if (p < d->bound)     /* check end mark */
            self->left_bytes = 0;
        }
      else      /* read normal event data bytes */
        {
          guint n = self->n_bytes, l = MIN (self->left_bytes, d->bound - d->bytes);
          self->n_bytes += l;
          self->bytes = g_renew (guint8, self->bytes, self->n_bytes);
          memcpy (self->bytes + n, d->bytes, l);
          d->bytes += l;
          self->left_bytes -= l;
        }
      if (!self->left_bytes)
        midi_decoder_advance_state (self);
      break;
    case BSE_MIDI_DECODER_DONE:
      if (self->event_type)
        bse_midi_decoder_construct_event (self);
      midi_decoder_advance_state (self);
      break;
    }
}

void
bse_midi_decoder_push_data (BseMidiDecoder *self,
                            guint           n_bytes,
                            guint8         *bytes,
                            guint64         usec_systime)
{
  Data data;
  
  g_return_if_fail (self != NULL);
  if (n_bytes)
    g_return_if_fail (bytes != NULL);
  
  data.delta_time = bse_engine_tick_stamp_from_systime (usec_systime);
  data.bytes = bytes;
  data.bound = bytes + n_bytes;
  while (data.bytes < data.bound || self->state_changed)
    {
      self->state_changed = FALSE;
      midi_decoder_parse_data (self, &data);
    }
  
  if (self->auto_queue)
    {
      while (self->events)
        {
          BseMidiEvent *event = sfi_ring_pop_head (&self->events);
          bse_midi_receiver_farm_distribute_event (event);
          bse_midi_free_event (event);
        }
      bse_midi_receiver_farm_process_events (data.delta_time);
    }
}

void
bse_midi_decoder_push_smf_data (BseMidiDecoder       *self,
                                guint                 n_bytes,
                                guint8               *bytes)
{
  g_return_if_fail (self != NULL);
  if (n_bytes)
    g_return_if_fail (bytes != NULL);
  g_return_if_fail (self->smf_support == TRUE);
  bse_midi_decoder_push_data (self, n_bytes, bytes, 0);
}

static inline gboolean
midi_decoder_extract_specific (BseMidiDecoder *self,
                               BseMidiEvent   *event)
{
  const double DR7F = 1.0 / (gdouble) 0x7f;
  const double DR2000 = 1.0 / (gdouble) 0x2000;
  /* command specific event portions */
  switch (event->status)
    {
      guint v;
      gint ival;
    case BSE_MIDI_NOTE_OFF:     /* 7bit note, 7bit velocity */
    case BSE_MIDI_NOTE_ON:      /* 7bit note, 7bit velocity */
    case BSE_MIDI_KEY_PRESSURE: /* 7bit note, 7bit intensity */
      if (self->n_bytes < 2)
        return FALSE;
      event->data.note.frequency = bse_note_to_freq (self->bytes[0] & 0x7f);
      ival = self->bytes[1] & 0x7f;
      /* in running-mode, 0 velocity indicates note-off */
      if (event->status == BSE_MIDI_NOTE_ON && ival == 0)
        event->status = BSE_MIDI_NOTE_OFF;
      /* some MIDI devices report junk velocity upon note-off */
      if (event->status == BSE_MIDI_NOTE_OFF)
        event->data.note.velocity = 0;
      else /* note-on or key-pressure */
        event->data.note.velocity = ival * DR7F;
      DEBUG ("ch-%02x: note-%s: freq=%.3f velocity=%.4f (%02x %02x)", event->channel,
             event->status == BSE_MIDI_NOTE_ON ? "on " :
             event->status == BSE_MIDI_NOTE_OFF ? "off" : "pressure",
             event->data.note.frequency, event->data.note.velocity,
             self->bytes[0], self->bytes[1]);
      break;
    case BSE_MIDI_CONTROL_CHANGE:       /* 7bit ctl-nr, 7bit value */
      if (self->n_bytes < 2)
        return FALSE;
      event->data.control.control = self->bytes[0] & 0x7f;
      ival = self->bytes[1] & 0x7f;
      event->data.control.value = ival * DR7F;
      DEBUG ("ch-%02x: control[%02u]: %.4f (0x%02x)", event->channel,
             event->data.control.control,
             event->data.control.value, ival);
      break;
    case BSE_MIDI_PROGRAM_CHANGE:       /* 7bit prg-nr */
      if (self->n_bytes < 1)
        return FALSE;
      event->data.program = self->bytes[0] & 0x7f;
      DEBUG ("ch-%02x: program-change: 0x%02x", event->channel, event->data.program);
      break;
    case BSE_MIDI_CHANNEL_PRESSURE:     /* 7bit intensity */
      if (self->n_bytes < 1)
        return FALSE;
      ival = self->bytes[0] & 0x7f;
      event->data.intensity = ival * DR7F;
      DEBUG ("ch-%02x: channel-pressure: %.4f (0x%02x)", event->channel, event->data.intensity, ival);
      break;
    case BSE_MIDI_PITCH_BEND:           /* 14bit signed: 7lsb, 7msb */
      if (self->n_bytes < 2)
        return FALSE;
      ival = self->bytes[0] & 0x7f;
      v = self->bytes[1] & 0x7f;
      ival |= v << 7;
      ival -= 0x2000;   /* range=0..0x3fff, center=0x2000 */
      event->data.pitch_bend = ival * DR2000;
      DEBUG ("ch-%02x: pitch-bend: %.4f (0x%04x)", event->channel, event->data.pitch_bend, ival);
      break;
    case BSE_MIDI_MULTI_SYS_EX_START:   /* BSE_MIDI_SYS_EX split across multiple events */
    case BSE_MIDI_MULTI_SYS_EX_NEXT:    /* continuation, last data byte of final packet is 0xF7 */
      /* for the above only, the final data byte is 0x7F */
    case BSE_MIDI_SYS_EX:               /* data... (without final 0x7F) */
      event->data.sys_ex.n_bytes = self->n_bytes;
      event->data.sys_ex.bytes = self->bytes;
      DEBUG ("ch-%02x: sys-ex: %u bytes", event->channel, self->n_bytes);
      self->n_bytes = 0;
      self->bytes = NULL;
      break;
    case BSE_MIDI_SONG_POINTER:         /* 14bit pointer: 7lsb, 7msb */
      if (self->n_bytes < 2)
        return FALSE;
      event->data.song_pointer = self->bytes[0] & 0x7f;
      v = self->bytes[1] & 0x7f;
      event->data.song_pointer |= v << 7;
      DEBUG ("ch-%02x: song-pointer: 0x%04x", event->channel, event->data.song_pointer);
      break;
    case BSE_MIDI_SONG_SELECT:          /* 7bit song-nr */
      if (self->n_bytes < 1)
        return FALSE;
      event->data.song_number = self->bytes[0] & 0x7f;
      DEBUG ("ch-%02x: song-select: 0x%02x", event->channel, event->data.song_number);
      break;
    case BSE_MIDI_SEQUENCE_NUMBER:      /* 16bit sequence number (msb, lsb) */
      if (self->n_bytes < 2)
        return FALSE;
      event->data.sequence_number = self->bytes[0] << 8;
      event->data.sequence_number += self->bytes[1];
      DEBUG ("ch-%02x: sequence-number: 0x%04x", event->channel, event->data.sequence_number);
      break;
    case BSE_MIDI_TEXT_EVENT:           /* 8bit text */
    case BSE_MIDI_COPYRIGHT_NOTICE:     /* 8bit text */
    case BSE_MIDI_TRACK_NAME:           /* 8bit text */
    case BSE_MIDI_INSTRUMENT_NAME:      /* 8bit text */
    case BSE_MIDI_LYRIC:                /* 8bit text */
    case BSE_MIDI_MARKER:               /* 8bit text */
    case BSE_MIDI_CUE_POINT:            /* 8bit text */
    case BSE_MIDI_TEXT_EVENT_08:        /* 8bit text */
    case BSE_MIDI_TEXT_EVENT_09:        /* 8bit text */
    case BSE_MIDI_TEXT_EVENT_0A:        /* 8bit text */
    case BSE_MIDI_TEXT_EVENT_0B:        /* 8bit text */
    case BSE_MIDI_TEXT_EVENT_0C:        /* 8bit text */
    case BSE_MIDI_TEXT_EVENT_0D:        /* 8bit text */
    case BSE_MIDI_TEXT_EVENT_0E:        /* 8bit text */
    case BSE_MIDI_TEXT_EVENT_0F:        /* 8bit text */
      event->data.text = g_strndup (self->bytes, self->n_bytes);
      DEBUG ("ch-%02x: text event (0x%02X): %s", event->channel, event->status, event->data.text);
      break;
    case BSE_MIDI_CHANNEL_PREFIX:       /* 8bit channel number (0..15) */
      if (self->n_bytes < 1)
        return FALSE;
      event->data.zprefix = self->bytes[0];
      DEBUG ("ch-XX: channel zprefix: %u", event->data.zprefix);
      break;
    case BSE_MIDI_SET_TEMPO:            /* 24bit usecs-per-quarter-note (msb first) */
      if (self->n_bytes < 3)
        return FALSE;
      event->data.usecs_pqn = self->bytes[0] << 16;
      event->data.usecs_pqn += self->bytes[1] << 8;
      event->data.usecs_pqn += self->bytes[2];
      DEBUG ("ch-%02x: set-tempo: usecs-per-quarter-note=%u", event->channel, event->data.usecs_pqn);
      break;
    case BSE_MIDI_SMPTE_OFFSET:         /* 8bit hour, minute, second, frame, 100th-frame-fraction */
      if (self->n_bytes < 5)
        return FALSE;
      event->data.smpte_offset.hour = self->bytes[0];
      event->data.smpte_offset.minute = self->bytes[1];
      event->data.smpte_offset.second = self->bytes[2];
      event->data.smpte_offset.frame = self->bytes[3];
      event->data.smpte_offset.fraction = self->bytes[4];
      DEBUG ("ch-%02x: smpte signature: hour=%u minute=%u second=%u frame=%u fraction=%u", event->channel,
             event->data.smpte_offset.hour, event->data.smpte_offset.minute, event->data.smpte_offset.second,
             event->data.smpte_offset.frame, event->data.smpte_offset.fraction);
      break;
    case BSE_MIDI_TIME_SIGNATURE:       /* 8bit numerator, -ld(1/denominator), metro-clocks, 32nd-npq */
      if (self->n_bytes < 4)
        return FALSE;
      event->data.time_signature.numerator = self->bytes[0];
      event->data.time_signature.denominator = 1 << self->bytes[1];
      event->data.time_signature.metro_clocks = self->bytes[2];
      event->data.time_signature.notated_32nd = self->bytes[3];
      DEBUG ("ch-%02x: time signature: %u/%u metro=%u 32/4=%u", event->channel,
             event->data.time_signature.numerator, event->data.time_signature.denominator,
             event->data.time_signature.metro_clocks, event->data.time_signature.notated_32nd);
      break;
    case BSE_MIDI_KEY_SIGNATURE:        /* 8bit sharpsflats, majorminor */
      if (self->n_bytes < 2)
        return FALSE;
      v = self->bytes[0];
      if (v & 0x40)
        event->data.key_signature.n_flats = v & 0x3f;
      else
        event->data.key_signature.n_sharps = v & 0x3f;
      event->data.key_signature.major_key = self->bytes[1] == 0;
      event->data.key_signature.minor_key = self->bytes[1] != 0;
      DEBUG ("ch-%02x: key signature: flats=%u sharps=%u major-key=%u", event->channel,
             event->data.key_signature.n_flats, event->data.key_signature.n_sharps, event->data.key_signature.major_key);
      break;
    case BSE_MIDI_SEQUENCER_SPECIFIC:   /* manufacturer specific sequencing data */
      event->data.sys_ex.n_bytes = self->n_bytes;
      event->data.sys_ex.bytes = self->bytes;
      DEBUG ("ch-%02x: sequencer specific: %u bytes", event->channel, self->n_bytes);
      self->n_bytes = 0;
      self->bytes = NULL;
      break;
    default: ;
    }
  return TRUE;
}

static void
bse_midi_decoder_construct_event (BseMidiDecoder *self)
{
  BseMidiEvent *event = bse_midi_alloc_event ();
  g_return_if_fail (self->event_type >= 0x080);
  g_return_if_fail (self->left_bytes == 0);
  
  /* try to collapse multi packet sys-ex to normal sys-ex */
  if (self->event_type == BSE_MIDI_MULTI_SYS_EX_START &&
      self->n_bytes > 0 &&
      self->bytes[self->n_bytes - 1] == 0xF7)
    {
      self->n_bytes--;
      self->event_type = BSE_MIDI_SYS_EX;
    }
  /* common event portion */
  event->status = self->event_type;
  event->channel = 1 + self->zchannel;
  event->delta_time = self->delta_time;
  /* specific event portion */
  if (midi_decoder_extract_specific (self, event))
    {
      if (event->status == BSE_MIDI_CHANNEL_PREFIX)
        self->zchannel = event->data.zprefix;
      self->events = sfi_ring_append (self->events, event);
    }
  else
    {
      if (event->status)
        sfi_diag ("BseMidiDecoder: discarding midi event (0x%02X): data invalid\n", event->status);
      bse_midi_free_event (event);
    }
  self->n_bytes = 0;
}
