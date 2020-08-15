// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bse/processor.hh"
#include "bse/signalmath.hh"
#include "bse/internal.hh"

#include <liquidsfz.hh>

namespace Bse {

using namespace AudioSignal;
using LiquidSFZ::Synth;

// == LiquidSFZ ==
// SFZ sampler using liquidsfz library
class LiquidSFZ : public AudioSignal::Processor {
  OBusId stereo_out_;
  Synth synth_;

  void
  initialize () override
  {
    synth_.set_sample_rate (sample_rate());
    synth_.load ("/home/stefan/sfz/SalamanderGrandPianoV3_44.1khz16bit/SalamanderGrandPianoV3.sfz");
    // TODO: handle load error
  }
  void
  query_info (ProcessorInfo &info) override
  {
    info.uri = "Bse.LiquidSFZ";
    // info.version = "0";
    info.label = "LiquidSFZ";
    info.category = "Synth";
  }
  void
  configure (uint n_ibusses, const SpeakerArrangement *ibusses, uint n_obusses, const SpeakerArrangement *obusses) override
  {
    remove_all_buses();
    prepare_event_input();
    stereo_out_ = add_output_bus ("Stereo Out", SpeakerArrangement::STEREO);
    assert_return (bus_info (stereo_out_).ident == "stereo-out");
  }
  void
  reset () override
  {
    // TODO: not available in liquidsfz API at the moment
  }
  void
  render (uint n_frames) override
  {
    EventRange erange = get_event_input();
    for (const auto &ev : erange)
      {
        const int time_stamp = std::max<int> (ev.frame, 0);
        switch (ev.message())
          {
          case Message::NOTE_OFF:
            synth_.add_event_note_off (time_stamp, ev.channel, ev.pitch);
            break;
          case Message::NOTE_ON:
            synth_.add_event_note_on (time_stamp, ev.channel, ev.pitch, std::clamp (bse_ftoi (ev.velocity * 127), 0, 127));
            break;
          case Message::ALL_NOTES_OFF:
            // TODO: not available in liquidsfz API at the moment
            break;
          default: ;
          }
      }

    float *output[2] = {
      oblock (stereo_out_, 0),
      oblock (stereo_out_, 1)
    };
    synth_.process (output, n_frames);
  }
};

static auto liquidsfz = Bse::enroll_asp<LiquidSFZ>();

} // Bse
