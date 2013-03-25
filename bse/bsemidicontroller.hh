// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_MIDI_CONTROLLER_H__
#define __BSE_MIDI_CONTROLLER_H__
#include <bse/bsesource.hh>
#include <bse/bsemidievent.hh>
G_BEGIN_DECLS
/* --- object type macros --- */
#define BSE_TYPE_MIDI_CONTROLLER	      (BSE_TYPE_ID (BseMidiController))
#define BSE_MIDI_CONTROLLER(object)	      (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_MIDI_CONTROLLER, BseMidiController))
#define BSE_MIDI_CONTROLLER_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_MIDI_CONTROLLER, BseMidiControllerClass))
#define BSE_IS_CONTROLLER(object)	      (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_MIDI_CONTROLLER))
#define BSE_IS_CONTROLLER_CLASS(class)	      (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_MIDI_CONTROLLER))
#define BSE_MIDI_CONTROLLER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_MIDI_CONTROLLER, BseMidiControllerClassOut))

struct BseMidiController : BseSource {
  guint		     midi_channel;
  BseMidiSignalType  controls[4];
};
struct BseMidiControllerClass : BseSourceClass
{};

enum
{
  BSE_MIDI_CONTROLLER_OCHANNEL_CONTROL1,
  BSE_MIDI_CONTROLLER_OCHANNEL_CONTROL2,
  BSE_MIDI_CONTROLLER_OCHANNEL_CONTROL3,
  BSE_MIDI_CONTROLLER_OCHANNEL_CONTROL4,
  BSE_MIDI_CONTROLLER_N_OCHANNELS
};
G_END_DECLS
#endif /* __BSE_MIDI_CONTROLLER_H__ */
