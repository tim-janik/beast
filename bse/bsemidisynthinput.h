/* BseMidiSynthInput - BSE Midi Synth glue module
 * Copyright (C) 1999, 2000-2001 Tim Janik
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
#ifndef __BSE_MIDI_SYNTH_INPUT_H__
#define __BSE_MIDI_SYNTH_INPUT_H__

#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_MIDI_SYNTH_INPUT	       (BSE_TYPE_ID (BseMidiSynthInput))
#define BSE_MIDI_SYNTH_INPUT(object)	       (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_MIDI_SYNTH_INPUT, BseMidiSynthInput))
#define BSE_MIDI_SYNTH_INPUT_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_MIDI_SYNTH_INPUT, BseMidiSynthInputClass))
#define BSE_IS_MIDI_SYNTH_INPUT(object)	       (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_MIDI_SYNTH_INPUT))
#define BSE_IS_MIDI_SYNTH_INPUT_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_MIDI_SYNTH_INPUT))
#define BSE_MIDI_SYNTH_INPUT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_MIDI_SYNTH_INPUT, BseMidiSynthInputClass))


/* --- BseMidiSynthInput source --- */
typedef struct _BseMidiSynthInput  BseMidiSynthInput;
typedef struct _BseSourceClass     BseMidiSynthInputClass;
struct _BseMidiSynthInput
{
  BseSource	parent_object;

  guint		midi_channel_id;
  guint		nth_note;

  /* PREPARED */
  GslModule   *midi_input_module;
};


/* --- channels --- */
enum
{
  BSE_MIDI_SYNTH_INPUT_OCHANNEL_FREQUENCY,
  BSE_MIDI_SYNTH_INPUT_OCHANNEL_GATE,
  BSE_MIDI_SYNTH_INPUT_OCHANNEL_VELOCITY,
  BSE_MIDI_SYNTH_INPUT_OCHANNEL_AFTERTOUCH
};


/* --- prototypes --- */
void	bse_midi_synth_input_set_params	(BseMidiSynthInput *msi,
					 guint              midi_channel_id,
					 guint		    nth_note);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_MIDI_SYNTH_INPUT_H__ */
