// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include <bse/processor.hh>
#include <bse/signalmath.hh>
#include <bse/bsenote.hh>
#include "bse/internal.hh"

#define DDEBUG(...)     Bse::debug ("debugdsp", __VA_ARGS__)

namespace Bse {

/// Namespace used for DSP debugging definitiond.
using namespace AudioSignal;

// == BlepSynth ==
// subtractive synth based on band limited steps (MinBLEP):
// - aliasing-free square/saw and similar sounds including hard sync
class BlepSynth : public AudioSignal::Processor {
  OBusId stereout_;
  ParamId pid_c_, pid_d_, pid_e_, pid_f_, pid_g_;
  bool    old_c_, old_d_, old_e_, old_f_, old_g_;

  class Voice
  {
  public:
    enum State {
      IDLE,
      ON,
      RELEASE
      // TODO: SUSTAIN / pedal
    };
    // TODO : enum class MonoType

    State        state_       = IDLE;
    int          midi_note_   = -1;
    //int        channel_
    double       freq_        = 0;
    double       phase_       = 0;
  };
  std::vector<Voice>    voices_;
  std::vector<Voice *>  active_voices_;
  std::vector<Voice *>  idle_voices_;
  void
  query_info (ProcessorInfo &info) override
  {
    info.uri = "Bse.BlepSynth";
    // info.version = "0";
    info.label = "BlepSynth";
    info.category = "Synth";
  }
  void
  initialize () override
  {
    set_max_voices (64);

    start_param_group ("Keyboard Input");
    pid_c_ = add_param ("Main Input  1",  "C", "G:big", false);
    pid_d_ = add_param ("Main Input  2",  "D", "G:big", false);
    pid_e_ = add_param ("Main Input  3",  "E", "G:big", false);
    pid_f_ = add_param ("Main Input  4",  "F", "G:big", false);
    pid_g_ = add_param ("Main Input  5",  "G", "G:big", false);
    old_c_ = old_d_ = old_e_ = old_f_ = old_g_ = false;
  }
  void
  set_max_voices (uint n_voices)
  {
    voices_.clear();
    voices_.resize (n_voices);

    active_voices_.clear();
    active_voices_.reserve (n_voices);

    idle_voices_.clear();
    for (auto& v : voices_)
      idle_voices_.push_back (&v);
  }
  Voice *
  alloc_voice()
  {
    if (idle_voices_.empty()) // out of voices?
      return nullptr;

    Voice *voice = idle_voices_.back();
    assert_return (voice->state_ == Voice::IDLE, nullptr);   // every item in idle_voices should be idle

    // move voice from idle to active list
    idle_voices_.pop_back();
    active_voices_.push_back (voice);

    return voice;
  }
  void
  free_unused_voices()
  {
    size_t new_voice_count = 0;

    for (size_t i = 0; i < active_voices_.size(); i++)
      {
        Voice *voice = active_voices_[i];

        if (voice->state_ == Voice::IDLE)    // voice used?
          {
            idle_voices_.push_back (voice);
          }
        else
          {
            active_voices_[new_voice_count++] = voice;
          }
      }
    active_voices_.resize (new_voice_count);
  }
  void
  configure (uint n_ibusses, const SpeakerArrangement *ibusses, uint n_obusses, const SpeakerArrangement *obusses) override
  {
    remove_all_buses();
    stereout_ = add_output_bus ("Stereo Out", SpeakerArrangement::STEREO);
    assert_return (bus_info (stereout_).ident == "stereo-out");
  }
  void
  adjust_param (ParamId tag)
  {
  }
  void
  reset (const RenderSetup &rs) override
  {
  }
  void
  note_on (int midi_note, int vel)
  {
    Voice *voice = alloc_voice();
    if (voice)
      {
        voice->freq_ = bse_note_to_freq (Bse::MusicalTuning::OD_12_TET, midi_note);
        voice->phase_ = 0;
        voice->state_ = Voice::ON;
        voice->midi_note_ = midi_note;
      }
  }
  void
  note_off (int midi_note)
  {
    for (auto voice : active_voices_)
      {
        if (voice->state_ == Voice::ON && voice->midi_note_ == midi_note)
          {
            voice->state_ = Voice::IDLE;
          }
      }
    free_unused_voices();
  }
  void
  check_note (ParamId pid, bool& old_value, int note)
  {
    bool value = get_param (pid) > 0.0;
    if (value != old_value)
      {
        if (value)
          note_on (note, 100);
        else
          note_off (note);
        old_value = value;
      }
  }
  void
  render (const RenderSetup &rs, uint n_frames) override
  {
    /* TODO: replace this with true midi input */
    check_note (pid_c_, old_c_, 60);
    check_note (pid_d_, old_d_, 62);
    check_note (pid_e_, old_e_, 64);
    check_note (pid_f_, old_f_, 65);
    check_note (pid_g_, old_g_, 67);
    assert_return (n_ochannels (stereout_) == 2);
    float *left = oblock (stereout_, 0);
    float *right = oblock (stereout_, 1);
    floatfill (left, 0.f, n_frames);
    floatfill (right, 0.f, n_frames);
    constexpr float scale = 0.25;
    for (auto& voice : active_voices_)
      {
        for (uint i = 0; i < n_frames; i++)
          {
             left[i] +=  sin (voice->freq_ * voice->phase_ * 2 * M_PI / sample_rate()) * scale;
             right[i] += sin (voice->freq_ * voice->phase_ * 2 * M_PI / sample_rate()) * scale;
             voice->phase_++;
          }
      }
  }
};
static auto blepsynth = Bse::enroll_asp<BlepSynth>();

} // Bse
