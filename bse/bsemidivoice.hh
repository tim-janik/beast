// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_MIDI_VOICE_H__
#define __BSE_MIDI_VOICE_H__

#include <bse/bsesource.hh>
#include <bse/bsesnet.hh>

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

struct BseMidiVoiceSwitch : BseSource {
  guint		 midi_channel;
  GSList        *midi_voices;
};
struct BseMidiVoiceSwitchClass : BseSourceClass
{};
struct BseMidiVoiceInput : BseSource {
  BseMidiVoiceSwitch *voice_switch;
};
struct BseMidiVoiceInputClass : BseSourceClass
{};

void           bse_midi_voice_switch_set_midi_channel (BseMidiVoiceSwitch *self,
                                                       guint               midi_channel);
BseMidiContext bse_midi_voice_switch_ref_poly_voice   (BseMidiVoiceSwitch *self,
                                                       guint               context_handle,
                                                       BseTrans           *trans);
BseMidiContext bse_midi_voice_switch_peek_poly_voice  (BseMidiVoiceSwitch *self,
                                                       guint               context_handle);
void           bse_midi_voice_switch_unref_poly_voice (BseMidiVoiceSwitch *self,
                                                       guint               context_handle,
                                                       BseTrans           *trans);
void           bse_midi_voice_input_set_voice_switch  (BseMidiVoiceInput  *self,
                                                       BseMidiVoiceSwitch *voice_switch);


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

#endif /* __BSE_MIDI_VOICE_H__ */
