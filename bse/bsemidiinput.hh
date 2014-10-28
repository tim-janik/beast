// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_MIDI_INPUT_H__
#define __BSE_MIDI_INPUT_H__

#include <bse/bsesource.hh>
#include <bse/bsemidievent.hh>


G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_MIDI_INPUT	         (BSE_TYPE_ID (BseMidiInput))
#define BSE_MIDI_INPUT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_MIDI_INPUT, BseMidiInput))
#define BSE_MIDI_INPUT_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_MIDI_INPUT, BseMidiInputClass))
#define BSE_IS_INPUT(object)	         (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_MIDI_INPUT))
#define BSE_IS_INPUT_CLASS(class)	 (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_MIDI_INPUT))
#define BSE_MIDI_INPUT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_MIDI_INPUT, BseMidiInputClassOut))

struct BseMidiInput : BseSource {
  guint		     midi_channel;
};
struct BseMidiInputClass : BseSourceClass
{};

enum
{
  BSE_MIDI_INPUT_OCHANNEL_FREQUENCY,
  BSE_MIDI_INPUT_OCHANNEL_GATE,
  BSE_MIDI_INPUT_OCHANNEL_VELOCITY,
  BSE_MIDI_INPUT_OCHANNEL_AFTERTOUCH,
  BSE_MIDI_INPUT_N_OCHANNELS
};


G_END_DECLS

#endif /* __BSE_MIDI_INPUT_H__ */
