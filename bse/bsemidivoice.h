/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1999, 2000-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __BSE_MIDI_VOICE_H__
#define __BSE_MIDI_VOICE_H__

#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_MIDI_VOICE_INPUT	        (BSE_TYPE_ID (BseMidiVoiceInput))
#define BSE_MIDI_VOICE_INPUT(object)	        (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_MIDI_VOICE_INPUT, BseMidiVoiceInput))
#define BSE_MIDI_VOICE_INPUT_CLASS(class)       (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_MIDI_VOICE_INPUT, BseMidiVoiceInputClass))
#define BSE_IS_MIDI_VOICE_INPUT(object)	        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_MIDI_VOICE_INPUT))
#define BSE_IS_MIDI_VOICE_INPUT_CLASS(class)    (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_MIDI_VOICE_INPUT))
#define BSE_MIDI_VOICE_INPUT_GET_CLASS(object)  (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_MIDI_VOICE_INPUT, BseMidiVoiceInputClass))
#define BSE_TYPE_MIDI_VOICE_SWITCH	        (BSE_TYPE_ID (BseMidiVoiceSwitch))
#define BSE_MIDI_VOICE_SWITCH(object)	        (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_MIDI_VOICE_SWITCH, BseMidiVoiceSwitch))
#define BSE_MIDI_VOICE_SWITCH_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_MIDI_VOICE_SWITCH, BseMidiVoiceSwitchClass))
#define BSE_IS_MIDI_VOICE_SWITCH(object)	(G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_MIDI_VOICE_SWITCH))
#define BSE_IS_MIDI_VOICE_SWITCH_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_MIDI_VOICE_SWITCH))
#define BSE_MIDI_VOICE_SWITCH_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_MIDI_VOICE_SWITCH, BseMidiVoiceSwitchClass))


/* --- object structures --- */
typedef struct _BseMidiVoiceInput  BseMidiVoiceInput;
typedef struct _BseMidiVoiceSwitch BseMidiVoiceSwitch;
typedef struct _BseSourceClass     BseMidiVoiceInputClass;
typedef struct _BseSourceClass     BseMidiVoiceSwitchClass;
struct _BseMidiVoiceInput
{
  BseSource	parent_object;

  BseMidiReceiver *midi_receiver;
  guint		   midi_channel;

  SfiRing	  *midi_voices;
};
struct _BseMidiVoiceSwitch
{
  BseSource	parent_object;

  BseMidiVoiceInput *voice_input;
};


/* --- prototypes --- */
void	bse_midi_voice_input_set_midi_receiver	(BseMidiVoiceInput	*self,
						 BseMidiReceiver	*midi_receiver,
						 guint			 midi_channel);
void	bse_midi_voice_switch_set_voice_input	(BseMidiVoiceSwitch	*self,
						 BseMidiVoiceInput	*voice_input);


/* --- channels --- */
enum
{
  BSE_MIDI_VOICE_INPUT_OCHANNEL_FREQUENCY,
  BSE_MIDI_VOICE_INPUT_OCHANNEL_GATE,
  BSE_MIDI_VOICE_INPUT_OCHANNEL_VELOCITY,
  BSE_MIDI_VOICE_INPUT_OCHANNEL_AFTERTOUCH
};
enum
{
  BSE_MIDI_VOICE_SWITCH_ICHANNEL_LEFT,
  BSE_MIDI_VOICE_SWITCH_ICHANNEL_RIGHT,
  BSE_MIDI_VOICE_SWITCH_ICHANNEL_DISCONNECT
};
enum
{
  BSE_MIDI_VOICE_SWITCH_OCHANNEL_LEFT,
  BSE_MIDI_VOICE_SWITCH_OCHANNEL_RIGHT,
  BSE_MIDI_VOICE_SWITCH_OCHANNEL_DISCONNECT
};



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_MIDI_VOICE_H__ */
