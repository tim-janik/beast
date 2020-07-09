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
  void
  initialize () override
  {
    start_param_group ("Main Settings");
    add_param ("Main Input  1",  "M1", 0, 100, 11.0, "%");
    add_param ("Main Input  2",  "M2", true);
    add_param ("Main Input  3",  "M3", 0, 100, 11.0, "%");
    add_param ("Main Input  4",  "M4", false);
    add_param ("Main Input  5",  "M5", true);
    add_param ("Main Input  6",  "M6", 0, 100, 11.0, "%");
    add_param ("Main Input  7",  "M7", 0, 100, 11.0, "%");
    add_param ("Main Input  8",  "M8", 0, 100, 11.0, "%");
    add_param ("Main Input  9",  "M9", 0, 100, 11.0, "%");
    add_param ("Main Input 10", "M10", 0, 100, 11.0, "%");
    add_param ("Main Input 11", "M11", 0, 100, 11.0, "%");
    add_param ("Main Input 12", "M12", 0, 100, 11.0, "%");
    add_param ("Main Input 13", "M13", 0, 100, 11.0, "%");
    add_param ("Main Input 14", "M14", 0, 100, 11.0, "%");
    add_param ("Main Input 15", "M15", 0, 100, 11.0, "%");
    add_param ("Main Input 16", "M16", 0, 100, 11.0, "%");
    add_param ("Main Input 17", "M17", 0, 100, 11.0, "%");
    add_param ("Main Input 18", "M18", 0, 100, 11.0, "%");
    add_param ("Main Input 19", "M19", 0, 100, 11.0, "%");
    add_param ("Main Input 20", "M20", 0, 100, 11.0, "%");
    add_param ("Main Input 21", "M21", 0, 100, 11.0, "%");
    add_param ("Main Input 22", "M22", 0, 100, 11.0, "%");
    add_param ("Main Input 23", "M23", 0, 100, 11.0, "%");

    start_param_group ("Additional Knobs");
    add_param ("Additional Input 1",   "A1", 0, 100, 11.0, "%");
    add_param ("Additional Input 2",   "A2", 0, 100, 11.0, "%");
    add_param ("Additional Input 3",   "A3", 0, 100, 11.0, "%");
    add_param ("Additional Input 4",   "A4", 0, 100, 11.0, "%");
    add_param ("Additional Input 5",   "A5", 0, 100, 11.0, "%");
    add_param ("Additional Input 6",   "A6", 0, 100, 11.0, "%");
    add_param ("Additional Input 7",   "A7", 0, 100, 11.0, "%");
    add_param ("Additional Input 8"  , "A8", 0, 100, 11.0, "%");
    add_param ("Additional Input 9",   "A9", 0, 100, 11.0, "%");
    add_param ("Additional Input 10", "A10", 0, 100, 11.0, "%");
    add_param ("Additional Input 11", "A11", 0, 100, 11.0, "%");
    add_param ("Additional Input 12", "A12", 0, 100, 11.0, "%");
    add_param ("Additional Input 13", "A13", 0, 100, 11.0, "%");

    start_param_group ("Volume");
    add_param ("Volume1",  "V1", 0, 100, 33.0, "%");
    add_param ("Volume2",  "V2", 0, 100, 33.0, "%");
    add_param ("Volume3",  "V3", 0, 100, 33.0, "%");
    add_param ("Volume4",  "V4", 0, 100, 33.0, "%");
    add_param ("Volume5",  "V5", 0, 100, 33.0, "%");
    add_param ("Volume6",  "V6", 0, 100, 33.0, "%");
    add_param ("Volume7",  "V7", 0, 100, 33.0, "%");
    add_param ("Volume8",  "V8", 0, 100, 33.0, "%");
    add_param ("Volume9",  "V9", 0, 100, 33.0, "%");
    add_param ("Volume10", "V10", 0, 100, 33.0, "%");
    add_param ("Volume11", "V11", 0, 100, 33.0, "%");
    add_param ("Volume12", "V12", 0, 100, 33.0, "%");

    start_param_group ("Gain Envelope");
    add_param ("Env1 Attack", "A", 0, 100, 11.0, "%");
    add_param ("Env1 Decay", "D", 0, 100, 11.0, "%");
    add_param ("Env1 Sustain", "S", 0, 100, 11.0, "%");
    add_param ("Env1 Release", "R", 0, 100, 11.0, "%");

    add_param ("Gain", "G", 0, 100, 33.0, "%",
               "Amount of amplification for the input signal");
    add_param ("Gain Reduction", "GR", -96, 12, 0.0, "dB", "out:inversemeter",
               "Amount of gain reduction");
    ChoiceEntries centries;
    centries += { "Surround", "Usually 5.1 or 7.1 channel configuration" };
    centries += { "Stereo31", "Stereo with side and LFE channels" };
    centries += { "Stereo21", "Stereo with LFE channel" };
    centries += { "Stereo", "Left and Right speaker combination" }; // 20
    centries += { "Right" };
    centries += { "Left" };
    centries += { "Mono", "Sound configuration with a single speaker" };
    add_param ("Channel Selection", "Chan", std::move (centries), 0, "dropdown",
               "What channels are used for signal processing");

    start_param_group ("ADSR Envelope");
    add_param ("Env2 Attack", "A", 0, 100, 11.0, "%");
    add_param ("Env2 Decay", "D", 0, 100, 11.0, "%");
    add_param ("Env2 Sustain", "S", 0, 100, 11.0, "%");
    add_param ("Env2 Release", "R", 0, 100, 11.0, "%");
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
  adjust_param (ParamId tag)
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
