// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_MIDILIB_HH__
#define __BSE_MIDILIB_HH__

#include <bse/processor.hh>

namespace Bse {

namespace MidiLib {

class MidiInputIface : public AudioSignal::Processor {
};

using MidiInputIfaceP = std::shared_ptr<MidiInputIface>;

} // MidiLib
} // Bse

#endif // __BSE_MIDILIB_HH__

