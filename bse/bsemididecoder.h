/* BSE - Better Sound Engine
 * Copyright (C) 1996-1999, 2000-2003 Tim Janik
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
#ifndef __BSE_MIDI_DECODER_H__
#define __BSE_MIDI_DECODER_H__

#include        <bse/bsemidievent.h>

G_BEGIN_DECLS


/* --- BSE MIDI structs --- */
typedef enum {
  BSE_MIDI_DECODER_ZERO = 0,
  /* read states as BSE_MIDI_DECODER_{needs_}...,
   * i.e. states indicate what's nect to be parsed
   */
  BSE_MIDI_DECODER_DELTA_TIME,
  BSE_MIDI_DECODER_EVENT,
  BSE_MIDI_DECODER_VLENGTH,
  BSE_MIDI_DECODER_DATA,        /* left_bytes != 0 */
  BSE_MIDI_DECODER_DONE,
} BseMidiDecoderState;
struct _BseMidiDecoder
{
  SfiRing         *events;      /* BseMidiEvent* */

  /* configuration */
  BseMusicalTuningType musical_tuning;
  uint                 auto_queue : 1;
  uint                 smf_support : 1;

  /*< private >*/
  uint                 state_changed : 1;
  BseMidiDecoderState  state;
  uint32               delta_time;     /* valid after BSE_MIDI_DECODER_DELTA_TIME_LOW */
  BseMidiEventType     event_type;     /* event after BSE_MIDI_DECODER_META_EVENT */
  BseMidiEventType     running_mode;
  uint                 zchannel;       /* current channel prefix (offset=-1) */
  uint32               left_bytes;     /* data to be read (BSE_MIDI_DECODER_DATA) */
  /* data accu */
  uint                 n_bytes;
  uint8               *bytes;
};


/* --- API --- */
BseMidiDecoder* bse_midi_decoder_new                      (gboolean              auto_queue,
                                                           gboolean              smf_support,
                                                           BseMusicalTuningType  musical_tuning);
void            bse_midi_decoder_destroy                  (BseMidiDecoder       *self);
void            bse_midi_decoder_push_data                (BseMidiDecoder       *self,
                                                           uint                  n_bytes,
                                                           uint8                *bytes,
                                                           uint64                usec_systime);
void            bse_midi_decoder_push_smf_data            (BseMidiDecoder       *self,
                                                           uint                  n_bytes,
                                                           uint8                *bytes);
BseMidiEvent*   bse_midi_decoder_pop_event                (BseMidiDecoder       *self);
SfiRing*        bse_midi_decoder_pop_event_list           (BseMidiDecoder       *self);

G_END_DECLS

#endif /* __BSE_MIDI_DECODER_H__ */
