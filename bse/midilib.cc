// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "midilib.hh"
#include <bse/signalmath.hh>
#include "bse/internal.hh"

#define MDEBUG(...)     Bse::debug ("midilib", __VA_ARGS__)

namespace Bse {

/// Namespace used for Midi Processor implementations.
namespace MidiLib {
using namespace AudioSignal;

// == MidiInputImpl ==
class MidiInputImpl : public MidiInputIface {
  constexpr static int64_t I63MAX = 9223372036854775807;
  int64_t start_frame = I63MAX, current_tick = 0;
  int64_t frame_ticks_sl16 = 0;
  ClipEventVectorP cevp;
  void assign_events (ClipEventVectorP cev) override { cevp = cev; }
  void
  query_info (ProcessorInfo &info) override
  {
    info.uri = "Bse.MidiLib.MidiInput";
    info.version = "0";
    info.label = "MIDI-Input";
    info.category = "Input & Output";
  }
  void
  initialize () override
  {
    add_param (BPM, "BPM",  "", 20, 500, 110);
  }
  void
  configure (uint n_ibusses, const SpeakerArrangement *ibusses, uint n_obusses, const SpeakerArrangement *obusses) override
  {
    remove_all_buses();
    prepare_event_input();
    prepare_event_output();
  }
  void
  reset() override
  {}
  void
  render (uint n_frames) override
  {
    // fetch IO streams
    EventStream &evout = get_event_output(); // needs prepare_event_output()
    EventRange evinp = get_event_input();
    const int64_t frame0 = engine().frame_counter();
    const int64_t frameb = frame0 + n_frames; // boundary, last + 1
    // MIDI through for input events // FIXME: copy only while frame<=0
    for (const auto ev : evinp)
      evout.append (ev.frame, ev);
    // add Clip events
    if (frameb <= start_frame)
      return;
  }
};
static auto midilib_midiinputimpl = Bse::enroll_asp<MidiInputImpl>();

} // MidiLib
} // Bse
