// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_MIDILIB_HH__
#define __BSE_MIDILIB_HH__

#include <bse/processor.hh>
#include <bse/clip.hh>

namespace Bse {

namespace MidiLib {

using ClipEventVectorP = ClipImpl::OrderedEventsP;

class MidiInputIface : public AudioSignal::Processor {
public:
  constexpr static ParamId BPM = ParamId (1);
  virtual void swap_event_vector (ClipEventVectorP &cev) = 0;
};

using MidiInputIfaceP = std::shared_ptr<MidiInputIface>;

} // MidiLib
} // Bse

#endif // __BSE_MIDILIB_HH__

