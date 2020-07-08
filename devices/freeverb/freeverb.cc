// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bse/processor.hh"
#include "bse/bseieee754.hh"
#include "bse/internal.hh"
#include "revmodel.hpp"

#include "revmodel.cpp"
#include "allpass.cpp"
#include "comb.cpp"

namespace {

using namespace Bse;
using namespace AudioSignal;

class Freeverb : public AudioSignal::Processor {
  IBusId stereoin;
  OBusId stereout;
  revmodel model;
  void
  query_info (ProcessorInfo &info) override
  {
    info.uri = "Bse.VST2.JzR3.Freeverb3";
    info.version = "0";
    info.label = "Freeverb3";
    info.category = "Reverb";
    info.website_url = "https://beast.testbit.eu";
    info.creator_name = "Jezar at Dreampoint";
  }
  enum ParamIds { DRY = 1, WET, ROOMSIZE, WIDTH, DAMPING, MODE };
  void
  initialize () override
  {
    ChoiceEntries centries;
    ParamId tag;

    start_param_group ("Reverb Settings");
    tag = add_param ("Dry level",  "Dry", 0, scaledry, "G:big", scaledry * initialdry, "dB");
    assert_return (ParamIds (tag) == DRY);

    tag = add_param ("Wet level",  "Wet", 0, scalewet, "G:big", scalewet * initialwet, "dB");
    assert_return (ParamIds (tag) == WET);

    start_param_group ("Room Settings");
    tag = add_param ("Room size",  "RS", offsetroom, offsetroom + scaleroom, "G:big", offsetroom + scaleroom * initialroom, "size");
    assert_return (ParamIds (tag) == ROOMSIZE);

    tag = add_param ("Width",  "W", 0, 100, "G:big", 100 * initialwidth, "%");
    assert_return (ParamIds (tag) == WIDTH);

    tag = add_param ("Damping",  "D", 0, 100, "G:big", 100 * initialdamp, "%");
    assert_return (ParamIds (tag) == DAMPING);

    centries += { "Normal Damping", "Damping with sign correction as implemented in STK Freeverb" };
    centries += { "VLC Damping",    "The VLC Freeverb version disables one damping feedback chain" };
    centries += { "Signflip 2000",  "Preserve May 2000 Freeverb damping sign flip" };
    tag = add_param ("Mode",  "M", std::move (centries), "G:dropdown", -1, "Damping mode found in different Freeverb variants");
    assert_return (ParamIds (tag) == MODE);
  }
  void
  configure (uint n_ibusses, const SpeakerArrangement *ibusses, uint n_obusses, const SpeakerArrangement *obusses) override
  {
    remove_all_buses();
    stereoin = add_input_bus  ("Stereo In",  SpeakerArrangement::STEREO);
    stereout = add_output_bus ("Stereo Out", SpeakerArrangement::STEREO);
  }
  void
  adjust_param (ParamId tag) override
  {
    switch (ParamIds (tag))
      {
      case WET:         return model.setwet (get_param (tag) / scalewet);
      case DRY:         return model.setdry (get_param (tag) / scaledry);
      case ROOMSIZE:    return model.setroomsize ((get_param (tag) - offsetroom) / scaleroom);
      case WIDTH:       return model.setwidth (0.01 * get_param (tag));
      case MODE:
      case DAMPING:     return model.setdamp (0.01 * get_param (ParamId (DAMPING)),
                                              bse_ftoi (get_param (ParamId (MODE))));
      }
  }
  void
  reset() override
  {
    model.setmode (0); // no-freeze, allow mute
    model.mute(); // silence internal buffers
    adjust_params (true);
  }
  void
  render (uint n_frames) override
  {
    adjust_params (false);
    float *input0 = const_cast<float*> (ifloats (stereoin, 0));
    float *input1 = const_cast<float*> (ifloats (stereoin, 1));
    float *output0 = oblock (stereout, 0);
    float *output1 = oblock (stereout, 1);
    model.processreplace (input0, input1, output0, output1, n_frames, 1);
  }
};
static auto freeverb = Bse::enroll_asp<Freeverb>();

} // Anon
