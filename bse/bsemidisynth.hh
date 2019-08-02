// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef	__BSE_MIDI_SYNTH_H__
#define	__BSE_MIDI_SYNTH_H__

#include	<bse/bsesnet.hh>
#include	<bse/bsesubsynth.hh>

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
  double	 volume_factor;         /* 1-based factor */
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

namespace Bse {

class MidiSynthImpl : public SNetImpl, public virtual MidiSynthIface {
protected:
  virtual        ~MidiSynthImpl  ();
  virtual void    post_init      () override;
public:
  explicit        MidiSynthImpl  (BseObject*);
  virtual int     midi_channel   () const override;
  virtual void    midi_channel   (int val) override;
  virtual int     n_voices       () const override;
  virtual void    n_voices       (int val) override;
  virtual double  volume_f       () const override;
  virtual void    volume_f       (double val) override;
};

} // Bse

#endif /* __BSE_MIDI_SYNTH_H__ */
