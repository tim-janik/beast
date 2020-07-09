// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include <bse/processor.hh>
#include <bse/signalmath.hh>
#include "bse/internal.hh"

#define DDEBUG(...)     Bse::debug ("debugdsp", __VA_ARGS__)

namespace Bse {

/// Namespace used for DSP debugging definitiond.
namespace DebugDsp {
using namespace AudioSignal;

// == DbgParameterizer ==
// Debug module to test parameter handling
class DbgParameterizer : public AudioSignal::Processor {
  IBusId stereoin, auxin;
  OBusId stereout;
  void
  query_info (ProcessorInfo &info) override
  {
    info.uri = "Bse.DebugDsp.DbgParameterizer";
    // info.version = "0";
    info.label = "DbgParameterizer";
    info.category = "Shaping";
  }
  enum Params {
    C1 = 1,
    G1, G2,
    E1, E2, E3, E4, E5, E6, E7, E8,
    V00, V01, V02, V03, V04, V05, V06, V07, V08, V09, V10, V11, V12,
    A00, A01, A02, A03, A04, A05, A06, A07, A08, A09, A10, A11, A12, A13,
    M00, M01, M02, M03, M04, M05, M06, M07, M08, M09, M10, M11, M12, M13, M14, M15, M16, M17, M18, M19, M20, M21, M22, M23,
  };
  void
  initialize () override
  {
    start_param_group ("Main Settings");
    add_param (M01, "Main Input  1",  "M1", 0, 100, 11.0, "%");
    add_param (M02, "Main Input  2",  "M2", true);
    add_param (M03, "Main Input  3",  "M3", 0, 100, 11.0, "%");
    add_param (M04, "Main Input  4",  "M4", false);
    add_param (M05, "Main Input  5",  "M5", true);
    add_param (M06, "Main Input  6",  "M6", 0, 100, 11.0, "%");
    add_param (M07, "Main Input  7",  "M7", 0, 100, 11.0, "%");
    add_param (M08, "Main Input  8",  "M8", 0, 100, 11.0, "%");
    add_param (M09, "Main Input  9",  "M9", 0, 100, 11.0, "%");
    add_param (M10, "Main Input 10", "M10", 0, 100, 11.0, "%");
    add_param (M11, "Main Input 11", "M11", 0, 100, 11.0, "%");
    add_param (M12, "Main Input 12", "M12", 0, 100, 11.0, "%");
    add_param (M13, "Main Input 13", "M13", 0, 100, 11.0, "%");
    add_param (M14, "Main Input 14", "M14", 0, 100, 11.0, "%");
    add_param (M15, "Main Input 15", "M15", 0, 100, 11.0, "%");
    add_param (M16, "Main Input 16", "M16", 0, 100, 11.0, "%");
    add_param (M17, "Main Input 17", "M17", 0, 100, 11.0, "%");
    add_param (M18, "Main Input 18", "M18", 0, 100, 11.0, "%");
    add_param (M19, "Main Input 19", "M19", 0, 100, 11.0, "%");
    add_param (M20, "Main Input 20", "M20", 0, 100, 11.0, "%");
    add_param (M21, "Main Input 21", "M21", 0, 100, 11.0, "%");
    add_param (M22, "Main Input 22", "M22", 0, 100, 11.0, "%");
    add_param (M23, "Main Input 23", "M23", 0, 100, 11.0, "%");

    start_param_group ("Additional Knobs");
    add_param (A01, "Additional Input 1",   "A1", 0, 100, 11.0, "%");
    add_param (A02, "Additional Input 2",   "A2", 0, 100, 11.0, "%");
    add_param (A03, "Additional Input 3",   "A3", 0, 100, 11.0, "%");
    add_param (A04, "Additional Input 4",   "A4", 0, 100, 11.0, "%");
    add_param (A05, "Additional Input 5",   "A5", 0, 100, 11.0, "%");
    add_param (A06, "Additional Input 6",   "A6", 0, 100, 11.0, "%");
    add_param (A07, "Additional Input 7",   "A7", 0, 100, 11.0, "%");
    add_param (A08, "Additional Input 8"  , "A8", 0, 100, 11.0, "%");
    add_param (A09, "Additional Input 9",   "A9", 0, 100, 11.0, "%");
    add_param (A10, "Additional Input 10", "A10", 0, 100, 11.0, "%");
    add_param (A11, "Additional Input 11", "A11", 0, 100, 11.0, "%");
    add_param (A12, "Additional Input 12", "A12", 0, 100, 11.0, "%");
    add_param (A13, "Additional Input 13", "A13", 0, 100, 11.0, "%");

    start_param_group ("Volume");
    add_param (V01, "Volume1",  "V1", 0, 100, 33.0, "%");
    add_param (V02, "Volume2",  "V2", 0, 100, 33.0, "%");
    add_param (V03, "Volume3",  "V3", 0, 100, 33.0, "%");
    add_param (V04, "Volume4",  "V4", 0, 100, 33.0, "%");
    add_param (V05, "Volume5",  "V5", 0, 100, 33.0, "%");
    add_param (V06, "Volume6",  "V6", 0, 100, 33.0, "%");
    add_param (V07, "Volume7",  "V7", 0, 100, 33.0, "%");
    add_param (V08, "Volume8",  "V8", 0, 100, 33.0, "%");
    add_param (V09, "Volume9",  "V9", 0, 100, 33.0, "%");
    add_param (V10, "Volume10", "V10", 0, 100, 33.0, "%");
    add_param (V11, "Volume11", "V11", 0, 100, 33.0, "%");
    add_param (V12, "Volume12", "V12", 0, 100, 33.0, "%");

    start_param_group ("Gain Envelope");
    add_param (E1, "Env1 Attack", "A", 0, 100, 11.0, "%");
    add_param (E2, "Env1 Decay", "D", 0, 100, 11.0, "%");
    add_param (E3, "Env1 Sustain", "S", 0, 100, 11.0, "%");
    add_param (E4, "Env1 Release", "R", 0, 100, 11.0, "%");

    add_param (G1, "Gain", "G", 0, 100, 33.0, "%",
               "Amount of amplification for the input signal");
    add_param (G2, "Gain Reduction", "GR", -96, 12, 0.0, "dB", "out:inversemeter",
               "Amount of gain reduction");
    ChoiceEntries centries;
    centries += { "Surround", "Usually 5.1 or 7.1 channel configuration" };
    centries += { "Stereo31", "Stereo with side and LFE channels" };
    centries += { "Stereo21", "Stereo with LFE channel" };
    centries += { "Stereo", "Left and Right speaker combination" }; // 20
    centries += { "Right" };
    centries += { "Left" };
    centries += { "Mono", "Sound configuration with a single speaker" };
    add_param (C1, "Channel Selection", "Chan", std::move (centries), 0, "dropdown",
               "What channels are used for signal processing");

    start_param_group ("ADSR Envelope");
    add_param (E5, "Env2 Attack", "A", 0, 100, 11.0, "%");
    add_param (E6, "Env2 Decay", "D", 0, 100, 11.0, "%");
    add_param (E7, "Env2 Sustain", "S", 0, 100, 11.0, "%");
    add_param (E8, "Env2 Release", "R", 0, 100, 11.0, "%");
  }
  void
  configure (uint n_ibusses, const SpeakerArrangement *ibusses, uint n_obusses, const SpeakerArrangement *obusses) override
  {
    remove_all_buses();
    prepare_event_input();
    stereoin = add_input_bus  ("Stereo In",  SpeakerArrangement::STEREO);
    auxin    = add_input_bus  ("Aux In",     SpeakerArrangement::STEREO);
    stereout = add_output_bus ("Stereo Out", SpeakerArrangement::STEREO);
    assert_return (bus_info (stereoin).ident == "stereo-in");
    assert_return (bus_info (auxin   ).ident == "aux-in");
    assert_return (bus_info (stereout).ident == "stereo-out");
  }
  void
  adjust_param (Id32 tag)
  {}
  void
  reset() override
  {}
  void
  render (uint n_frames) override
  {
    assert_return (n_ichannels (stereoin) == 2);
    assert_return (n_ichannels (auxin) == 2);
    assert_return (n_ochannels (stereout) == 2);

    EventRange erange = get_event_input();
    for (const auto &ev : erange)
      printerr ("DbgParameterizer: %s\n", ev.to_string());

    for (uint ch = 0; ch < 2; ch++)
      {
        const float *stinf = ifloats (stereoin, ch);
        const float *auxf = ifloats (auxin, ch);
        if ((stinf[0] == 0.0 && iconst (stereoin, ch, n_frames)) ||
            (auxf[0]  == 0.0 && iconst (auxin, ch, n_frames)))
          assign_oblock (stereout, ch, 0.0);
        else if (connected (stereout))
          {
            float *soutf = oblock (stereout, ch);
            if (auxf[0]  == 1.0 && iconst (auxin, ch, n_frames))
              redirect_oblock (stereout, ch, stinf);
            else if (stinf[0]  == 1.0 && iconst (stereoin, ch, n_frames))
              redirect_oblock (stereout, ch, auxf);
            else // stinf.connected && auxf.connected
              floatmul (soutf, stinf, auxf, n_frames);
          }
      }
  }
};
static auto dbgparameterizer = Bse::enroll_asp<DbgParameterizer>();

} // DebugDsp
} // Bse
