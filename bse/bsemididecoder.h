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
  guint            auto_queue : 1;
  guint            smf_support : 1;

  /*< private >*/
  guint                 state_changed : 1;
  BseMidiDecoderState   state;
  guint32               delta_time;     /* valid after BSE_MIDI_DECODER_DELTA_TIME_LOW */
  BseMidiEventType      event_type;     /* event after BSE_MIDI_DECODER_META_EVENT */
  BseMidiEventType      running_mode;
  guint                 zchannel;       /* current channel prefix (offset=-1) */
  guint32               left_bytes;     /* data to be read (BSE_MIDI_DECODER_DATA) */
  /* data accu */
  guint                 n_bytes;
  guint8               *bytes;
};


/* --- API --- */
BseMidiDecoder* bse_midi_decoder_new                      (gboolean              auto_queue,
                                                           gboolean              smf_support);
void            bse_midi_decoder_destroy                  (BseMidiDecoder       *self);
void            bse_midi_decoder_push_data                (BseMidiDecoder       *self,
                                                           guint                 n_bytes,
                                                           guint8               *bytes,
                                                           guint64               usec_systime);
void            bse_midi_decoder_push_smf_data            (BseMidiDecoder       *self,
                                                           guint                 n_bytes,
                                                           guint8               *bytes);
BseMidiEvent*   bse_midi_decoder_pop_event                (BseMidiDecoder       *self);
SfiRing*        bse_midi_decoder_pop_event_list           (BseMidiDecoder       *self);

G_END_DECLS

#endif /* __BSE_MIDI_DECODER_H__ */
