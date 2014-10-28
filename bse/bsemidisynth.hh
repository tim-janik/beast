// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef	__BSE_MIDI_SYNTH_H__
#define	__BSE_MIDI_SYNTH_H__

#include	<bse/bsesnet.hh>
#include	<bse/bsesubsynth.hh>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_MIDI_SYNTH	         (BSE_TYPE_ID (BseMidiSynth))
#define BSE_MIDI_SYNTH(object)	         (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_MIDI_SYNTH, BseMidiSynth))
#define BSE_MIDI_SYNTH_CLASS(class)	 (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_MIDI_SYNTH, BseMidiSynthClass))
#define BSE_IS_MIDI_SYNTH(object)	 (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_MIDI_SYNTH))
#define BSE_IS_MIDI_SYNTH_CLASS(class)	 (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_MIDI_SYNTH))
#define BSE_MIDI_SYNTH_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_MIDI_SYNTH, BseMidiSynthClass))

struct BseMidiSynth : BseSNet {
  guint		 midi_channel_id;
  guint		 n_voices;
  gfloat	 volume_factor;         /* 1-based factor */
  BseSNet       *snet;
  BseSNet       *pnet;
  BseSource	*voice_input;
  BseSource	*voice_switch;
  BseSource	*context_merger;
  BseSource	*postprocess;
  BseSource	*output;
  BseSource	*sub_synth;
};
struct BseMidiSynthClass : BseSNetClass
{};

G_END_DECLS
#endif /* __BSE_MIDI_SYNTH_H__ */
