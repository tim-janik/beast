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
struct _BseMidiDecoder
{
  SfiRing         *events;      /* BseMidiEvent* */
  
  /*< private >*/
  BseMidiEventType event_type;  /* event currently being decoded */
  BseMidiEventType running_mode;
  guint            echannel;    /* channel of current event */
  guint            n_bytes;
  guint8          *bytes;
  guint            left_bytes;
  gboolean         auto_queue;
};


/* --- API --- */
BseMidiDecoder* bse_midi_decoder_new                      (gboolean              auto_queue);
void            bse_midi_decoder_destroy                  (BseMidiDecoder       *self);
void            bse_midi_decoder_push_data                (BseMidiDecoder       *self,
                                                           guint                 n_bytes,
                                                           guint8               *bytes,
                                                           guint64               usec_systime);
BseMidiEvent*   bse_midi_decoder_pop_event                (BseMidiDecoder       *self);


G_END_DECLS

#endif /* __BSE_MIDI_DECODER_H__ */
