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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __BSE_MIDI_MODULE_H__
#define __BSE_MIDI_MODULE_H__

#include        <bse/bsemidievent.h>
#include        <gsl/gsldefs.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define	BSE_MIDI_MODULE_N_CHANNELS	(4)


/* --- structures --- */
typedef struct
{
  BseMidiDecoder *decoder;
  guint		  midi_channel_id;
  guint		  nth_note;
  guint		  signals[BSE_MIDI_MODULE_N_CHANNELS];
} BseMidiModuleData;


/* --- prototypes --- */
GslModule*	bse_midi_module_insert	(BseMidiDecoder	*decoder,
					 guint		 midi_channel_id,
					 guint		 nth_note, /* voice */
					 guint		 signals[BSE_MIDI_MODULE_N_CHANNELS],
					 GslTrans	*trans);
void		bse_midi_module_remove	(GslModule	*midi_module,
					 GslTrans	*trans);
gboolean	bse_midi_module_matches	(GslModule	*midi_module,
					 guint		 midid_channel_id,
					 guint		 nth_note,
					 guint		 signals[BSE_MIDI_MODULE_N_CHANNELS]);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_MIDI_MODULE_H__ */
