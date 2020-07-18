// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bse/processor.hh"
#include "bse/signalmath.hh"
#include "bse/bsenote.hh"
#include "devices/blepsynth/bleposc.hh"
#include "devices/blepsynth/laddervcf.hh"
#include "devices/blepsynth/linearsmooth.hh"
#include "bse/internal.hh"

namespace Bse {

using namespace AudioSignal;

// based on liquidsfz envelope.hh

namespace {

class Envelope
{
public:
  enum class Shape { EXPONENTIAL, LINEAR };
private:
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
  Shape shape_ = Shape::EXPONENTIAL;

  struct SlopeParams {
    int len;

    double factor;
    double delta;
    double end;
  } params_;

  double level_ = 0;

public:
  void
  set_shape (Shape shape)
  {
    shape_ = shape;
  }
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

    if (param_state == State::ATTACK || param_state == State::DELAY || param_state == State::HOLD || shape_ == Shape::LINEAR)
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

} // Anon

// == BlepSynth ==
// subtractive synth based on band limited steps (MinBLEP):
// - aliasing-free square/saw and similar sounds including hard sync
class BlepSynth : public AudioSignal::Processor {
  OBusId stereout_;
  ParamId pid_c_, pid_d_, pid_e_, pid_f_, pid_g_;
  bool    old_c_, old_d_, old_e_, old_f_, old_g_;

  struct OscParams {
    ParamId shape;
    ParamId pulse_width;
    ParamId sub;
    ParamId sub_width;
    ParamId sync;
    ParamId octave;
    ParamId pitch;

    ParamId unison_voices;
    ParamId unison_detune;
    ParamId unison_stereo;
  };
  OscParams osc_params[2];
  ParamId pid_mix_;

  ParamId pid_cutoff_;
  Logscale cutoff_logscale_;
  ParamId pid_resonance_;
  ParamId pid_drive_;
  ParamId pid_mode_;

  ParamId pid_attack_;
  ParamId pid_decay_;
  ParamId pid_sustain_;
  ParamId pid_release_;

  ParamId pid_fil_attack_;
  ParamId pid_fil_decay_;
  ParamId pid_fil_sustain_;
  ParamId pid_fil_release_;
  ParamId pid_fil_cut_mod_;

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
    Envelope     fil_envelope_;
    State        state_       = IDLE;
    int          midi_note_   = -1;
    int          channel_     = 0;
    double       freq_        = 0;

    LinearSmooth cutoff_smooth_;
    double       last_cutoff_;

    LinearSmooth cut_mod_smooth_;
    double       last_cut_mod_;

    BlepUtils::OscImpl osc1_;
    BlepUtils::OscImpl osc2_;
    LadderVCFNonLinear vcf_;
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
    set_max_voices (32);

    auto oscparams = [&] (int o) {
      start_param_group (string_format ("Oscillator %d", o + 1));
      osc_params[o].shape = add_param (string_format ("Osc %d Shape", o + 1), "Shape", -100, 100, 0, "%");
      osc_params[o].pulse_width = add_param (string_format ("Osc %d Pulse Width", o + 1), "P.W", 0, 100, 50, "%");
      osc_params[o].sub = add_param (string_format ("Osc %d Subharmonic", o + 1), "Sub", 0, 100, 0, "%");
      osc_params[o].sub_width = add_param (string_format ("Osc %d Subharmonic Width", o + 1), "Sub.W", 0, 100, 50, "%");
      osc_params[o].sync = add_param (string_format ("Osc %d Sync Slave", o + 1), "Sync", 0, 60, 0, "semitones");

      osc_params[o].pitch  = add_param (string_format ("Osc %d Pitch", o + 1), "Pitch", -7, 7, 0, "semitones");
      osc_params[o].octave = add_param (string_format ("Osc %d Octave", o + 1), "Octave", -2, 3, 0, "octaves");

      /* TODO: unison_voices property should have stepping set to 1 */
      osc_params[o].unison_voices = add_param (string_format ("Osc %d Unison Voices", o + 1), "Voices", 1, 16, 1, "voices");
      osc_params[o].unison_detune = add_param (string_format ("Osc %d Unison Detune", o + 1), "Detune", 0.5, 50, 6, "%");
      osc_params[o].unison_stereo = add_param (string_format ("Osc %d Unison Stereo", o + 1), "Stereo", 0, 100, 0, "%");
    };

    oscparams (0);

    start_param_group ("Filter");
    /* TODO: cutoff property should have logarithmic scaling */
    const double FsharpHz = 440 * ::pow (2, 9 / 12.);
    const double freq_lo = FsharpHz / ::pow (2, 5);
    const double freq_hi = FsharpHz * ::pow (2, 5);
    pid_cutoff_ = add_param ("Cutoff", "Cutoff", freq_lo, freq_hi, FsharpHz, "Hz", STANDARD);
    cutoff_logscale_.setup (freq_lo, freq_hi);
    pid_resonance_ = add_param ("Resonance", "Reso", 0, 100, 25.0, "%");
    pid_drive_ = add_param ("Drive", "Drive", -24, 36, 0, "dB");
    ChoiceEntries centries;
    centries += { "None", "disable filter" };
    centries += { "L1", "1 pole lowpass, 6db/octave" };
    centries += { "L2", "2 pole lowpass, 12db/octave" };
    centries += { "L3", "3 pole lowpass, 18db/octave" };
    centries += { "L4", "4 pole lowpass, 24db/octave" };
    pid_mode_ = add_param ("Filter Mode", "Mode", std::move (centries), 2, "", "Ladder Filter Mode to be used");

    oscparams (1);

    start_param_group ("Volume Envelope");
    pid_attack_  = add_param ("Attack",  "A", 0, 8000, 110.0, "ms", STANDARD + "logcenter=1000");
    pid_decay_   = add_param ("Decay",   "D", 0, 8000, 200.0, "ms", STANDARD + "logcenter=1000");
    pid_sustain_ = add_param ("Sustain", "S", 0, 100, 50.0, "%");
    pid_release_ = add_param ("Release", "R", 0, 8000, 300.0, "ms", STANDARD + "logcenter=1000");

    start_param_group ("Filter Envelope");
    pid_fil_attack_   = add_param ("Attack",  "A", 0, 8000, 500.0, "ms", STANDARD + "logcenter=1000");
    pid_fil_decay_    = add_param ("Decay",   "D", 0, 8000, 1500.0, "ms", STANDARD + "logcenter=1000");
    pid_fil_sustain_  = add_param ("Sustain", "S", 0, 100,  30.0, "%");
    pid_fil_release_  = add_param ("Release", "R", 0, 8000, 300.0, "ms", STANDARD + "logcenter=1000");
    pid_fil_cut_mod_  = add_param ("Env Cutoff Modulation", "CutMod", -96, 96, 36, "semitones"); /* 8 octaves range */

    start_param_group ("Mix");
    pid_mix_ = add_param ("Mix", "Mix", 0, 100, 0, "%");

    start_param_group ("Keyboard Input");
    pid_c_ = add_param ("Main Input  1",  "C", false);
    pid_d_ = add_param ("Main Input  2",  "D", false);
    pid_e_ = add_param ("Main Input  3",  "E", false);
    pid_f_ = add_param ("Main Input  4",  "F", false);
    pid_g_ = add_param ("Main Input  5",  "G", false);
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
    prepare_event_input();
    stereout_ = add_output_bus ("Stereo Out", SpeakerArrangement::STEREO);
    assert_return (bus_info (stereout_).ident == "stereo-out");
  }
  void
  reset () override
  {
    set_max_voices (0);
    set_max_voices (32);
  }
  void
  init_osc (BlepUtils::OscImpl& osc, float freq)
  {
    osc.frequency_base = freq;
    osc.set_rate (sample_rate());
#if 0
    osc.freq_mod_octaves  = properties->freq_mod_octaves;
#endif
  }
  void
  update_osc (BlepUtils::OscImpl& osc, const OscParams& params)
  {
    osc.shape_base          = get_param (params.shape) * 0.01;
    osc.pulse_width_base    = get_param (params.pulse_width) * 0.01;
    osc.sub_base            = get_param (params.sub) * 0.01;
    osc.sub_width_base      = get_param (params.sub_width) * 0.01;
    osc.sync_base           = get_param (params.sync);

    int octave = bse_ftoi (get_param (params.octave));
    octave = CLAMP (octave, -2, 3);
    osc.frequency_factor = fast_exp2 (octave + get_param (params.pitch) / 12.);

    int unison_voices = bse_ftoi (get_param (params.unison_voices));
    unison_voices = CLAMP (unison_voices, 1, 16);
    osc.set_unison (unison_voices, get_param (params.unison_detune), get_param (params.unison_stereo) * 0.01);
  }
  void
  note_on (int channel, int midi_note, int vel)
  {
    Voice *voice = alloc_voice();
    if (voice)
      {
        voice->freq_ = bse_note_to_freq (Bse::MusicalTuning::OD_12_TET, midi_note);
        voice->state_ = Voice::ON;
        voice->channel_ = channel;
        voice->midi_note_ = midi_note;

        // Volume Envelope
        /* TODO: we need non-linear translation between percent and time/level */
        voice->envelope_.set_delay (0);
        voice->envelope_.set_attack (get_param (pid_attack_) * 0.001);   /* time in milliseconds */
        voice->envelope_.set_hold (0);
        voice->envelope_.set_decay (get_param (pid_decay_) * 0.001);     /* time in milliseconds */
        voice->envelope_.set_sustain (get_param (pid_sustain_));         /* percent */
        voice->envelope_.set_release (get_param (pid_release_) * 0.001); /* time in milliseconds */
        voice->envelope_.start (sample_rate());

        // Filter Envelope
        voice->fil_envelope_.set_delay (0);
        voice->fil_envelope_.set_attack (get_param (pid_fil_attack_) * 0.001);   /* time in milliseconds */
        voice->fil_envelope_.set_hold (0);
        voice->fil_envelope_.set_decay (get_param (pid_fil_decay_) * 0.001);     /* time in milliseconds */
        voice->fil_envelope_.set_sustain (get_param (pid_fil_sustain_));         /* percent */
        voice->fil_envelope_.set_release (get_param (pid_fil_release_) * 0.001); /* time in milliseconds */
        voice->fil_envelope_.set_shape (Envelope::Shape::LINEAR);
        voice->fil_envelope_.start (sample_rate());

        init_osc (voice->osc1_, voice->freq_);
        init_osc (voice->osc2_, voice->freq_);

        voice->osc1_.reset();
        voice->osc2_.reset();
        voice->vcf_.reset();

        voice->cutoff_smooth_.reset (sample_rate(), 0.020);
        voice->last_cutoff_ = -5000; // force reset

        voice->cut_mod_smooth_.reset (sample_rate(), 0.020);
        voice->last_cut_mod_ = -5000; // force reset
      }
  }
  void
  note_off (int channel, int midi_note)
  {
    for (auto voice : active_voices_)
      {
        if (voice->state_ == Voice::ON && voice->midi_note_ == midi_note && voice->channel_ == channel)
          {
            voice->state_ = Voice::RELEASE;
            voice->envelope_.stop();
          }
      }
  }
  void
  check_note (ParamId pid, bool& old_value, int note)
  {
    const bool value = get_param (pid) > 0.5;
    if (value != old_value)
      {
        constexpr int channel = 0;
        if (value)
          note_on (channel, note, 100);
        else
          note_off (channel, note);
        old_value = value;
      }
  }
  void
  render (uint n_frames) override
  {
    /* TODO: replace this with true midi input */
    check_note (pid_c_, old_c_, 60);
    check_note (pid_d_, old_d_, 62);
    check_note (pid_e_, old_e_, 64);
    check_note (pid_f_, old_f_, 65);
    check_note (pid_g_, old_g_, 67);

    EventRange erange = get_event_input();
    for (const auto &ev : erange)
      switch (ev.message())
        {
        case Message::NOTE_OFF:
          note_off (ev.channel, ev.pitch);
          break;
        case Message::NOTE_ON:
          note_on (ev.channel, ev.pitch, ev.velocity);
          break;
        case Message::ALL_NOTES_OFF:
          for (auto voice : active_voices_)
            if (voice->state_ == Voice::ON && voice->channel_ == ev.channel)
              note_off (voice->channel_, voice->midi_note_);
          break;
        default: ;
        }

    assert_return (n_ochannels (stereout_) == 2);
    bool   need_free = false;
    float *left_out = oblock (stereout_, 0);
    float *right_out = oblock (stereout_, 1);

    floatfill (left_out, 0.f, n_frames);
    floatfill (right_out, 0.f, n_frames);

    for (auto& voice : active_voices_)
      {
        float osc1_left_out[n_frames];
        float osc1_right_out[n_frames];
        float osc2_left_out[n_frames];
        float osc2_right_out[n_frames];

        update_osc (voice->osc1_, osc_params[0]);
        update_osc (voice->osc2_, osc_params[1]);
        voice->osc1_.process_sample_stereo (osc1_left_out, osc1_right_out, n_frames);
        voice->osc2_.process_sample_stereo (osc2_left_out, osc2_right_out, n_frames);

        // apply volume envelope & mix
        float mix_left_out[n_frames];
        float mix_right_out[n_frames];
        const float mix_norm = get_param (pid_mix_) * 0.01;
        const float v1 = 1 - mix_norm;
        const float v2 = mix_norm;
        for (uint i = 0; i < n_frames; i++)
          {
            mix_left_out[i]  = osc1_left_out[i] * v1 + osc2_left_out[i] * v2;
            mix_right_out[i] = osc1_right_out[i] * v1 + osc2_right_out[i] * v2;
          }
        bool run_filter = true;
        switch (bse_ftoi (get_param (pid_mode_)))
          {
          case 4: voice->vcf_.set_mode (LadderVCFMode::LP4);
            break;
          case 3: voice->vcf_.set_mode (LadderVCFMode::LP3);
            break;
          case 2: voice->vcf_.set_mode (LadderVCFMode::LP2);
            break;
          case 1: voice->vcf_.set_mode (LadderVCFMode::LP1);
            break;
          default: run_filter = false;
            break;
          }
        /* --------- run ladder filter - processing in place is ok --------- */

        /* TODO: under some conditions we could enable SSE in LadderVCF (alignment and block_size) */
        const float *inputs[2]  = { mix_left_out, mix_right_out };
        float       *outputs[2] = { mix_left_out, mix_right_out };
        double cutoff = get_param (pid_cutoff_) * inyquist();
        double resonance = get_param (pid_resonance_) * 0.01;

        if (fabs (voice->last_cutoff_ - cutoff) > 1e-7)
          {
            const bool reset = voice->last_cutoff_ < -1000;

            voice->cutoff_smooth_.set (fast_log2 (cutoff), reset);
            voice->last_cutoff_ = cutoff;
          }
        double cut_mod = get_param (pid_fil_cut_mod_) / 12.; /* convert semitones to octaves */
        if (fabs (voice->last_cut_mod_ - cut_mod) > 1e-7)
          {
            const bool reset = voice->last_cut_mod_ < -1000;

            voice->cut_mod_smooth_.set (cut_mod, reset);
            voice->last_cut_mod_ = cut_mod;
          }
        /* TODO: possible improvements:
         *  - exponential smoothing (get rid of exp2f)
         *  - don't do anything if cutoff_smooth_->steps_ == 0 (add accessor)
         */
        float freq_in[n_frames];
        for (uint i = 0; i < n_frames; i++)
          freq_in[i] = fast_exp2 (voice->cutoff_smooth_.get_next() + voice->fil_envelope_.get_next() * voice->cut_mod_smooth_.get_next());
        voice->vcf_.set_drive (get_param (pid_drive_));

        float no_out[n_frames];
        if (!run_filter)
          {
            // we keep running the filter even if it is disabled in order to have
            // sane filter signal to switch to when the filter is enabled again
            outputs[0] = no_out;
            outputs[1] = no_out;
          }
        voice->vcf_.run_block (n_frames, cutoff, resonance, inputs, outputs, true, true, freq_in, nullptr, nullptr, nullptr);

        // apply volume envelope
        for (uint i = 0; i < n_frames; i++)
          {
            float amp = 0.25 * voice->envelope_.get_next();
            left_out[i] += mix_left_out[i] * amp;
            right_out[i] += mix_right_out[i] * amp;
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
  std::string
  param_value_to_text (Id32 paramid, double value) const
  {
    /* fake step=1 */
    for (int o = 0; o < 2; o++)
      {
        if (paramid == osc_params[o].unison_voices)
          return string_format ("%d voices", bse_ftoi (value));
        if (paramid == osc_params[o].octave)
          return string_format ("%d octaves", bse_ftoi (value));
      }

    return Processor::param_value_to_text (paramid, value);
  }
  double
  value_to_normalized (Id32 paramid, double value) const override
  {
    if (paramid == pid_cutoff_)
      return cutoff_logscale_.iscale (value);
    return Processor::value_to_normalized (paramid, value);
  }
  double
  value_from_normalized (Id32 paramid, double normalized) const override
  {
    if (paramid == pid_cutoff_)
      return cutoff_logscale_.scale (normalized);
    return Processor::value_from_normalized (paramid, normalized);
  }
};
static auto blepsynth = Bse::enroll_asp<BlepSynth>();

} // Bse
