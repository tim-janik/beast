// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include <bse/processor.hh>
#include <bse/signalmath.hh>
#include <bse/bsenote.hh>
#include "bse/internal.hh"

#define DDEBUG(...)     Bse::debug ("debugdsp", __VA_ARGS__)

namespace Bse {

/// Namespace used for DSP debugging definitiond.
using namespace AudioSignal;

// based on liquidsfz envelope.hh

namespace
{

class Envelope
{
  /* values in seconds */
  float delay_ = 0;
  float attack_ = 0;
  float hold_ = 0;
  float decay_ = 0;
  float sustain_ = 0; /* <- percent */
  float release_ = 0;

  int delay_len_ = 0;
  int attack_len_ = 0;
  int hold_len_ = 0;
  int decay_len_ = 0;
  int release_len_ = 0;
  float sustain_level_ = 0;

  enum class State { DELAY, ATTACK, HOLD, DECAY, SUSTAIN, RELEASE, DONE };

  State state_ = State::DONE;

  struct SlopeParams {
    int len;

    double factor;
    double delta;
    double end;
  } params_;

  double level_ = 0;

public:
  void
  set_delay (float f)
  {
    delay_ = f;
  }
  void
  set_attack (float f)
  {
    attack_ = f;
  }
  void
  set_hold (float f)
  {
    hold_ = f;
  }
  void
  set_decay (float f)
  {
    decay_ = f;
  }
  void
  set_sustain (float f)
  {
    sustain_ = f;
  }
  void
  set_release (float f)
  {
    release_ = f;
  }
  void
  start (int sample_rate)
  {
    delay_len_ = std::max (int (sample_rate * delay_), 1);
    attack_len_ = std::max (int (sample_rate * attack_), 1);
    hold_len_ = std::max (int (sample_rate * hold_), 1);
    decay_len_ = std::max (int (sample_rate * decay_), 1);
    sustain_level_ = std::clamp<float> (sustain_ * 0.01, 0, 1); // percent->level
    release_len_ = std::max (int (sample_rate * release_), 1);

    level_ = 0;
    state_ = State::DELAY;

    compute_slope_params (delay_len_, 0, 0, State::DELAY);
  }
  void
  stop()
  {
    state_ = State::RELEASE;
    compute_slope_params (release_len_, level_, 0, State::RELEASE);
  }
  bool
  done()
  {
    return state_ == State::DONE;
  }
  void
  compute_slope_params (int len, float start_x, float end_x, State param_state)
  {
    params_.end = end_x;

    if (param_state == State::ATTACK || param_state == State::DELAY || param_state == State::HOLD)
      {
        // linear
        params_.len    = len;
        params_.delta  = (end_x - start_x) / params_.len;
        params_.factor = 1;
      }
    else
      {
        assert_return (param_state == State::DECAY || param_state == State::RELEASE);

        // exponential

        /* true exponential decay doesn't ever reach zero; therefore we need to
         * fade out early
         */
        const double RATIO = 0.001; // -60dB or 0.1% of the original height;

        /* compute iterative exponential decay parameters from inputs:
         *
         *   - len:           half life time
         *   - RATIO:         target ratio (when should we reach zero)
         *   - start_x/end_x: level at start/end of the decay slope
         *
         * iterative computation of next value (should be done params.len times):
         *
         *    value = value * params.factor + params.delta
         */
        const double f = -log ((RATIO + 1) / RATIO) / len;
        params_.len    = len;
        params_.factor = exp (f);
        params_.delta  = (end_x - RATIO * (start_x - end_x)) * (1 - params_.factor);
      }
  }

  float
  get_next()
  {
    if (state_ == State::SUSTAIN || state_ == State::DONE)
      return level_;

    level_ = level_ * params_.factor + params_.delta;
    params_.len--;
    if (!params_.len)
      {
        level_ = params_.end;

        if (state_ == State::DELAY)
          {
            compute_slope_params (attack_len_, 0, 1, State::ATTACK);
            state_ = State::ATTACK;
          }
        else if (state_ == State::ATTACK)
          {
            compute_slope_params (hold_len_, 1, 1, State::HOLD);
            state_ = State::HOLD;
          }
        else if (state_ == State::HOLD)
          {
            compute_slope_params (decay_len_, 1, sustain_level_, State::DECAY);
            state_ = State::DECAY;
          }
        else if (state_ == State::DECAY)
          {
            state_ = State::SUSTAIN;
          }
        else if (state_ == State::RELEASE)
          {
            state_ = State::DONE;
          }
      }
    return level_;
  }
};

}

// == BlepSynth ==
// subtractive synth based on band limited steps (MinBLEP):
// - aliasing-free square/saw and similar sounds including hard sync
class BlepSynth : public AudioSignal::Processor {
  OBusId stereout_;
  ParamId pid_c_, pid_d_, pid_e_, pid_f_, pid_g_;
  bool    old_c_, old_d_, old_e_, old_f_, old_g_;

  ParamId pid_attack_;
  ParamId pid_decay_;
  ParamId pid_sustain_;
  ParamId pid_release_;

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

    Envelope     envelope_;
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

    start_param_group ("Volume Envelope");
    pid_attack_ = add_param ("Attack", "A", 0, 100, "G:big", 11.0, "%");
    pid_decay_ = add_param ("Decay", "D", 0, 100, "G:big", 11.0, "%");
    pid_sustain_ = add_param ("Sustain", "S", 0, 100, "G:big", 11.0, "%");
    pid_release_ = add_param ("Release", "R", 0, 100, "G:big", 11.0, "%");
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

        // Volume Envelope
        /* TODO: we need non-linear translation between percent and time/level */
        voice->envelope_.set_delay (0);
        voice->envelope_.set_attack (get_param (pid_attack_) * 0.01);   /* time in seconds */
        voice->envelope_.set_hold (0);
        voice->envelope_.set_decay (get_param (pid_decay_) * 0.01);     /* time in seconds */
        voice->envelope_.set_sustain (get_param (pid_sustain_));        /* percent */
        voice->envelope_.set_release (get_param (pid_release_) * 0.01); /* time in seconds */
        voice->envelope_.start (sample_rate());
      }
  }
  void
  note_off (int midi_note)
  {
    for (auto voice : active_voices_)
      {
        if (voice->state_ == Voice::ON && voice->midi_note_ == midi_note)
          {
            voice->state_ = Voice::RELEASE;
            voice->envelope_.stop();
          }
      }
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
    bool   need_free = false;
    float *left = oblock (stereout_, 0);
    float *right = oblock (stereout_, 1);

    floatfill (left, 0.f, n_frames);
    floatfill (right, 0.f, n_frames);

    for (auto& voice : active_voices_)
      {
        for (uint i = 0; i < n_frames; i++)
          {
            float scale = 0.25 * voice->envelope_.get_next();
            left[i] +=  sin (voice->freq_ * voice->phase_ * 2 * M_PI / sample_rate()) * scale;
            right[i] += sin (voice->freq_ * voice->phase_ * 2 * M_PI / sample_rate()) * scale;
            voice->phase_++;
          }
        if (voice->envelope_.done())
          {
            voice->state_ = Voice::IDLE;
            need_free = true;
          }
      }
    if (need_free)
      free_unused_voices();
  }
};
static auto blepsynth = Bse::enroll_asp<BlepSynth>();

} // Bse
