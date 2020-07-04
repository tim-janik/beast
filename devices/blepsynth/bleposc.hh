// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html

#ifndef __BSE_BLEPOSC_HH__
#define __BSE_BLEPOSC_HH__

#include <vector>
#include <array>

#include <sys/types.h>
#include <assert.h>
#include <math.h>
#include <glib.h>

#include <bse/bseblockutils.hh>
#include <bse/bsemathsignal.hh>

namespace Bse {
namespace BlepUtils {

using Bse::Block;
using std::max;

class OscImpl
{
  double rate_;
  double leaky_a;

  static const int WIDTH = 13;
  static const int WSHIFT = 6; // delay to align saw and impulse part of the signal
  static const int OVERSAMPLE = 64;

  static const float blep_table[WIDTH * OVERSAMPLE + 1];

public:
  double frequency_base   = 440;
  double frequency_factor = 1;

  double freq_mod_octaves = 0;

  double shape_base       = 0; // 0 = saw, range [-1:1]
  double shape_mod        = 1.0;

  double pulse_width_base = 0.5;
  double pulse_width_mod  = 0.0;

  double sync_base        = 0;
  double sync_mod         = 0;

  double sub_base         = 0;
  double sub_mod          = 0;

  double sub_width_base   = 0.5;
  double sub_width_mod    = 0.0;

  bool   need_reset_voice_state;

  enum class State {
    A,
    B,
    C,
    D
  };

  struct UnisonVoice
  {
    double freq_factor     = 1;
    double left_factor     = 1;
    double right_factor    = 0;

    double master_phase    = 0;
    double slave_phase     = 0;

    double last_value      = 0; /* leaky integrator state */
    double current_level   = 0; /* current position of the wave form (saw + jumps) */

    double last_dc         = 0; /* dc of previous parameters */
    double dc_delta        = 0;
    int    dc_steps        = 0;

    int    future_pos      = 0;


    State  state           = State::A;

    // could be shared under certain conditions
    std::array<float, WIDTH * 2> future;

    void
    init_future()
    {
      future.fill (0);
      future_pos = 0;
    }
    float
    pop_future()
    {
      const float f = future[future_pos++];
      if (future_pos == WIDTH)
        {
          // this loop was slightly faster than std::copy_n + std::fill_n
          for (int i = 0; i < WIDTH; i++)
            {
              future[i] = future[WIDTH + i];
              future[WIDTH + i] = 0;
            }
          future_pos = 0;
        }
      return f;
    }
  };

  std::vector<UnisonVoice> unison_voices;

  OscImpl()
  {
    set_unison (1, 0, 0); // default
  }
  void
  reset()
  {
    const bool randomize_phase = unison_voices.size() > 1;

    for (auto& voice : unison_voices)
      {
        voice.init_future();
        voice.dc_steps = 0;
        voice.dc_delta = 0;

        if (randomize_phase) // randomize start phase for true unison
          {
            reset_master (voice, g_random_double_range (0, 1));
          }
        else
          {
            reset_master (voice, 0);
          }
      }
  }
  void
  reset_master (UnisonVoice& voice, double master_phase)
  {
    voice.master_phase = master_phase;
    need_reset_voice_state = true;
  }
  void
  set_unison (size_t n_voices, float detune, float stereo)
  {
    const bool unison_voices_changed = unison_voices.size() != n_voices;

    unison_voices.resize (n_voices);

    bool left_channel = true; /* start spreading voices at the left channel */
    for (size_t i = 0; i < unison_voices.size(); i++)
      {
        if (n_voices == 1)
          unison_voices[i].freq_factor = 1;
        else
          {
            const float detune_cent = -detune / 2.0 + i / float (n_voices - 1) * detune;
            unison_voices[i].freq_factor = pow (2, detune_cent / 1200);
          }
        /* stereo spread factors */
        double left_factor, right_factor;
        bool odd_n_voices = unison_voices.size() & 1;
        if (odd_n_voices && i == unison_voices.size() / 2)  // odd number of voices: this voice is centered
          {
            left_factor  = (1 - stereo) + stereo * 0.5;
            right_factor = (1 - stereo) + stereo * 0.5;
          }
        else if (left_channel) // alternate beween left and right voices
          {
            left_factor  = 0.5 + stereo / 2;
            right_factor = 0.5 - stereo / 2;
            left_channel = false;
          }
        else
          {
            left_factor  = 0.5 - stereo / 2;
            right_factor = 0.5 + stereo / 2;
            left_channel = true;
          }
        /* ensure constant total energy of left + right channel combined
         *
         * also take into account the more unison voices we add up, the louder the result
         * will be, so compensate for this:
         *
         *   -> each time the number of voices is doubled, the signal level is increased by
         *      a factor of sqrt (2)
         */
        const double norm = sqrt (left_factor * left_factor + right_factor * right_factor) * sqrt (n_voices / 2.0);
        unison_voices[i].left_factor  = left_factor / norm;
        unison_voices[i].right_factor = right_factor / norm;
      }
    if (unison_voices_changed)
      reset();
  }
  void
  set_rate (double rate)
  {
    rate_ = rate;

    /* get leaky integrator constant for sample rate from ms (half-life time) */
    const double leaky_ms = 10;
    leaky_a = pow (2.0, -1000.0 / (rate_ * leaky_ms));
  }
  double
  rate()
  {
    return rate_;
  }
  double
  estimate_dc (double shape,
               double pulse_width,
               double sub,
               double sub_width,
               double sync_factor)
  {
    const double bound_a = sub_width * pulse_width;
    const double bound_b = 2 * sub_width * pulse_width + 1 - sub_width - pulse_width;
    const double bound_c = sub_width * pulse_width + (1 - sub_width);
    const double bound_d = 1.0;

    const double saw_slope = -4.0 * (shape + 1) * (1 - sub);

    const double a1 = 1;
    const double a2 = a1 + saw_slope * bound_a;

    const double b1 = a2 + 2.0 * (shape * (1 - sub) - sub);
    const double b2 = b1 + saw_slope * (bound_b - bound_a);

    const double c1 = b2 + 2 * (1 - sub);
    const double c2 = c1 + saw_slope * (bound_c - bound_b);

    const double d1 = c2 + 2.0 * (shape * (1 - sub) + sub);
    const double d2 = d1 + saw_slope * (bound_d - bound_c);

    /* dc without sync */
    const double dc_base = (a1 + a2) / 2 * bound_a
                         + (b1 + b2) / 2 * (bound_b - bound_a)
                         + (c1 + c2) / 2 * (bound_c - bound_b)
                         + (d1 + d2) / 2 * (bound_d - bound_c);

    /* quick path: no sync, no sync related dc computation */
    if (sync_factor < 1.01)
      return dc_base;

    /* dc offset introduced by sync */
    const double sync_phase = sync_factor - int (sync_factor);

    double a_avg = (a1 + a2) / 2;
    double b_avg = (b1 + b2) / 2;
    double c_avg = (c1 + c2) / 2;
    double d_avg = (d1 + d2) / 2;

    if (sync_phase < bound_a)
      {
        const double frac = (bound_a - sync_phase) / bound_a;
        const double sync_a2 = a1 * frac + a2 * (1 - frac);

        a_avg = (1 - frac) * (a1 + sync_a2) / 2;
        b_avg = c_avg = d_avg = 0;
      }
    else if (sync_phase < bound_b)
      {
        const double frac = (bound_b - sync_phase) / (bound_b - bound_a);
        const double sync_b2 = b1 * frac + b2 * (1 - frac);

        b_avg = (1 - frac) * (b1 + sync_b2) / 2;
        c_avg = d_avg = 0;
      }
    else if (sync_phase < bound_c)
      {
        const double frac = (bound_c - sync_phase) / (bound_c - bound_b);
        const double sync_c2 = c1 * frac + c2 * (1 - frac);

        c_avg = (1 - frac) * (c1 + sync_c2) / 2;
        d_avg = 0;
      }
    else
      {
        const double frac = (bound_d - sync_phase) / (bound_d - bound_c);
        const double sync_d2 = d1 * frac + d2 * (1 - frac);

        d_avg = (1 - frac) * (d1 + sync_d2) / 2;
      }

    /* dc sync part of the signal */
    const double dc_sync = a_avg * bound_a
                         + b_avg * (bound_b - bound_a)
                         + c_avg * (bound_c - bound_b)
                         + d_avg * (bound_d - bound_c);
    const double dc = (dc_base * (int) sync_factor + dc_sync) / sync_factor;

    return dc;
  }

  void
  reset_voice_state (double shape,
                     double pulse_width,
                     double sub,
                     double sub_width,
                     double sync_factor)
  {
    const double bound_a = sub_width * pulse_width;
    const double bound_b = 2 * sub_width * pulse_width + 1 - sub_width - pulse_width;
    const double bound_c = sub_width * pulse_width + (1 - sub_width);
    const double bound_d = 1.0;

    const double saw_slope = -4.0 * (shape + 1) * (1 - sub);

    const double a1 = 1;
    const double a2 = a1 + saw_slope * bound_a;

    const double b1 = a2 + 2.0 * (shape * (1 - sub) - sub);
    const double b2 = b1 + saw_slope * (bound_b - bound_a);

    const double c1 = b2 + 2 * (1 - sub);
    const double c2 = c1 + saw_slope * (bound_c - bound_b);

    const double d1 = c2 + 2.0 * (shape * (1 - sub) + sub);
    const double d2 = d1 + saw_slope * (bound_d - bound_c);

    /* dc without sync */
    const double dc_base = (a1 + a2) / 2 * bound_a
                         + (b1 + b2) / 2 * (bound_b - bound_a)
                         + (c1 + c2) / 2 * (bound_c - bound_b)
                         + (d1 + d2) / 2 * (bound_d - bound_c);

    /* dc offset introduced by sync */
    const double sync_phase = sync_factor - int (sync_factor);

    double a_avg = (a1 + a2) / 2;
    double b_avg = (b1 + b2) / 2;
    double c_avg = (c1 + c2) / 2;
    double d_avg = (d1 + d2) / 2;

    if (sync_phase < bound_a)
      {
        const double frac = (bound_a - sync_phase) / bound_a;
        const double sync_a2 = a1 * frac + a2 * (1 - frac);

        a_avg = (1 - frac) * (a1 + sync_a2) / 2;
        b_avg = c_avg = d_avg = 0;
      }
    else if (sync_phase < bound_b)
      {
        const double frac = (bound_b - sync_phase) / (bound_b - bound_a);
        const double sync_b2 = b1 * frac + b2 * (1 - frac);

        b_avg = (1 - frac) * (b1 + sync_b2) / 2;
        c_avg = d_avg = 0;
      }
    else if (sync_phase < bound_c)
      {
        const double frac = (bound_c - sync_phase) / (bound_c - bound_b);
        const double sync_c2 = c1 * frac + c2 * (1 - frac);

        c_avg = (1 - frac) * (c1 + sync_c2) / 2;
        d_avg = 0;
      }
    else
      {
        const double frac = (bound_d - sync_phase) / (bound_d - bound_c);
        const double sync_d2 = d1 * frac + d2 * (1 - frac);

        d_avg = (1 - frac) * (d1 + sync_d2) / 2;
      }

    /* dc sync part of the signal */
    double dc_sync = a_avg * bound_a
                   + b_avg * (bound_b - bound_a)
                   + c_avg * (bound_c - bound_b)
                   + d_avg * (bound_d - bound_c);

    const double dc = (dc_base * (int) sync_factor + dc_sync) / sync_factor;

    for (auto& voice : unison_voices)
      {
        double dest_phase = voice.master_phase;

        double last_value; /* leaky integrator state */

        dest_phase *= sync_factor;
        dest_phase -= (int) dest_phase;

        voice.slave_phase = dest_phase;

        /* compute voice state and initial value without dc */
        if (dest_phase < bound_a)
          {
            double frac = (bound_a - dest_phase) / bound_a;
            last_value = a1 * frac + a2 * (1 - frac);

            voice.state = State::A;
          }
        else if (dest_phase < bound_b)
          {
            double frac = (bound_b - dest_phase) / (bound_b - bound_a);
            last_value = b1 * frac + b2 * (1 - frac);

            voice.state = State::B;
          }
        else if (dest_phase < bound_c)
          {
            double frac = (bound_c - dest_phase) / (bound_c - bound_b);
            last_value = c1 * frac + c2 * (1 - frac);

            voice.state = State::C;
          }
        else
          {
            double frac = (bound_d - dest_phase) / (bound_d - bound_c);
            last_value = d1 * frac + d2 * (1 - frac);

            voice.state = State::D;
          }
        voice.last_value    = last_value - dc;
        voice.last_dc       = dc;
        voice.current_level = last_value - 1;
      }
  }
  void
  insert_blep (UnisonVoice& voice, double frac, double weight)
  {
    int pos = frac * OVERSAMPLE;
    const float inter_frac = frac * OVERSAMPLE - pos;
    const float weight_left = (1 - inter_frac) * weight;
    const float weight_right = inter_frac * weight;

    pos = std::max (pos, 0);
    pos = std::min (pos, OVERSAMPLE - 1);

    for (int i = 0; i < WIDTH; i++)
      {
        voice.future[i + voice.future_pos] += blep_table[pos] * weight_left + blep_table[pos + 1] * weight_right;

        pos += OVERSAMPLE;
      }
  }
  void
  insert_future_delta (UnisonVoice& voice, double weight)
  {
    voice.future[voice.future_pos + WSHIFT] += weight;
  }

  double
  clamp (double d, double min, double max)
  {
    return CLAMP (d, min, max);
  }

  /* check if slave oscillator has reached target_phase
   *
   * when master oscillator sync occurs, only return true if this point in time is
   * before master oscillator sync
   */
  bool
  check_slave_before_master (UnisonVoice& voice, double target_phase, double sync_factor)
  {
    if (voice.slave_phase > target_phase)
      {
        if (voice.master_phase > 1)
          {
            const double slave_frac = (voice.slave_phase - target_phase) / sync_factor;
            const double master_frac = voice.master_phase - 1;

            return master_frac < slave_frac;
          }
        else
          return true;
      }
    return false;
  }
  void
  process_sample_stereo (float *left_out, float *right_out, unsigned int n_values,
                         const float *freq_in = nullptr,
                         const float *freq_mod_in = nullptr,
                         const float *shape_mod_in = nullptr,
                         const float *sub_mod_in = nullptr,
                         const float *sync_mod_in = nullptr,
                         const float *pulse_mod_in = nullptr,
                         const float *sub_width_mod_in = nullptr)
  {
    Block::fill (n_values, left_out, 0.0);
    Block::fill (n_values, right_out, 0.0);

    double master_freq = frequency_factor * frequency_base;
    double pulse_width = clamp (pulse_width_base, 0.01, 0.99);
    double sub         = clamp (sub_base, 0.0, 1.0);
    double sub_width   = clamp (sub_width_base, 0.01, 0.99);
    double shape       = clamp (shape_base, -1.0, 1.0);
    double sync_factor = fast_exp2 (clamp (sync_base, 0.0, 60.0) / 12);

    /* dc substampling according to control frequency (cpu/quality trade off) */
    const int dc_steps = max (bse_ftoi (rate_ / 4000), 1);

    for (auto& voice : unison_voices)
      {
        const double master_freq2inc = 0.5 / rate_ * voice.freq_factor;

        for (unsigned int n = 0; n < n_values; n++)
          {
            if (freq_in)
              master_freq = frequency_factor * BSE_SIGNAL_TO_FREQ (freq_in[n]);

            double master_inc = master_freq * master_freq2inc;
            if (freq_mod_in)
              master_inc *= fast_exp2 (freq_mod_in[n] * freq_mod_octaves);

            if (shape_mod_in)
              shape = clamp (shape_base + shape_mod * shape_mod_in[n], -1.0, 1.0);

            if (sub_mod_in)
              sub = clamp (sub_base + sub_mod * sub_mod_in[n], 0.0, 1.0);

            if (sync_mod_in)
              sync_factor = fast_exp2 (clamp (sync_base + sync_mod * sync_mod_in[n], 0.0, 60.0) / 12);

            if (pulse_mod_in)
              pulse_width = clamp (pulse_width_base + pulse_width_mod * pulse_mod_in[n], 0.01, 0.99);

            if (sub_width_mod_in)
              sub_width = clamp (sub_width_base + sub_width_mod * sub_width_mod_in[n], 0.01, 0.99);

            /* reset needs parameters, so we need to do it here */
            if (need_reset_voice_state)
              {
                reset_voice_state (shape, pulse_width, sub, sub_width, sync_factor);
                need_reset_voice_state = false;
              }

            const double slave_inc = master_inc * sync_factor;
            const double saw_delta = -4.0 * slave_inc * (shape + 1) * (1 - sub);

            voice.master_phase += master_inc;
            voice.slave_phase  += slave_inc;

            bool state_changed;
            do
              {
                state_changed = false;

                if (voice.state == State::A)
                  {
                    const double bound_a = sub_width * pulse_width;

                    if (check_slave_before_master (voice, bound_a, sync_factor))
                      {
                        const double slave_frac = (voice.slave_phase - bound_a) / slave_inc;

                        const double jump_a = 2.0 * (shape * (1 - sub) - sub);
                        const double saw = -4.0 * (shape + 1) * (1 - sub) * bound_a;
                        const double blep_height = jump_a + saw - (voice.current_level + (1 - slave_frac) * saw_delta);

                        insert_blep (voice, slave_frac, blep_height);
                        voice.current_level += blep_height;
                        voice.state = State::B;
                        state_changed = true;
                      }
                  }
                if (voice.state == State::B)
                  {
                    const double bound_b = 2 * sub_width * pulse_width + 1 - sub_width - pulse_width;

                    if (check_slave_before_master (voice, bound_b, sync_factor))
                      {
                        const double slave_frac = (voice.slave_phase - bound_b) / slave_inc;

                        const double jump_ab = 2.0 * ((shape + 1) * (1 - sub) - sub);
                        const double saw = -4.0 * (shape + 1) * (1 - sub) * bound_b;
                        const double blep_height = jump_ab + saw - (voice.current_level + (1 - slave_frac) * saw_delta);

                        insert_blep (voice, slave_frac, blep_height);
                        voice.current_level += blep_height;
                        voice.state = State::C;
                        state_changed = true;
                      }
                  }
                if (voice.state == State::C)
                  {
                    const double bound_c = sub_width * pulse_width + (1 - sub_width);

                    if (check_slave_before_master (voice, bound_c, sync_factor))
                      {
                        const double slave_frac = (voice.slave_phase - bound_c) / slave_inc;

                        const double jump_abc = 2.0 * (2 * shape + 1) * (1 - sub);
                        const double saw = -4.0 * (shape + 1) * (1 - sub) * bound_c;
                        const double blep_height = jump_abc + saw - (voice.current_level + (1 - slave_frac) * saw_delta);

                        insert_blep (voice, slave_frac, blep_height);
                        voice.current_level += blep_height;
                        voice.state = State::D;
                        state_changed = true;
                      }
                  }
                if (voice.state == State::D)
                  {
                    if (check_slave_before_master (voice, 1, sync_factor))
                      {
                        voice.slave_phase -= 1;

                        const double slave_frac = voice.slave_phase / slave_inc;

                        voice.current_level += (1 - slave_frac) * saw_delta;

                        insert_blep (voice, slave_frac, -voice.current_level);

                        voice.current_level = saw_delta * slave_frac - saw_delta;
                        voice.state = State::A;
                        state_changed = true;
                      }
                  }
                if (!state_changed && voice.master_phase > 1)
                  {
                    voice.master_phase -= 1;

                    const double master_frac = voice.master_phase / master_inc;

                    const double new_slave_phase = voice.master_phase * sync_factor;

                    voice.current_level += (1 - master_frac) * saw_delta;

                    insert_blep (voice, master_frac, -voice.current_level);

                    voice.current_level = saw_delta * master_frac - saw_delta;
                    voice.slave_phase = new_slave_phase;

                    voice.state = State::A;
                    state_changed = true;
                  }
              }
            while (state_changed); // rerun all state checks if state was modified

            if (voice.dc_steps > 0)
              {
                voice.dc_steps--;
              }
            else
              {
                const double dc = estimate_dc (shape, pulse_width, sub, sub_width, sync_factor);

                voice.dc_steps = dc_steps - 1;
                voice.dc_delta = (voice.last_dc - dc) / dc_steps;
                voice.last_dc = dc;
              }

            voice.current_level += saw_delta;
            insert_future_delta (voice, saw_delta + voice.dc_delta); // align with the impulses

            /* leaky integration */
            double value = leaky_a * voice.last_value + voice.pop_future();
            voice.last_value = value;

            left_out[n] += value * voice.left_factor;
            right_out[n] += value * voice.right_factor;
          }
      }
  }
};

class Osc /* simple interface to OscImpl */
{
  double
  to_sync (double sync_factor)
  {
    return 12 * log (sync_factor) / log (2);
  }
public:
  OscImpl osc_impl;

  double rate;
  double freq;
  double pulse_width = 0.5;
  double shape = 0; // 0 = saw, range [-1:1]
  double sub = 0;
  double sub_width = 0.5;

  double master_freq;

  void
  set_unison (size_t n_voices, float detune, float stereo)
  {
    osc_impl.set_unison (n_voices, detune, stereo);
  }
  double
  test_seek_to (double phase)
  {
    assert (osc_impl.unison_voices.size() > 0);
    osc_impl.reset();
    osc_impl.reset_master (osc_impl.unison_voices[0], phase);  // jump to phase

    process_sample(); // propagate parameters & perform reset
    return osc_impl.unison_voices[0].last_value;
  }
  double
  process_sample()
  {
    float left, right;

    process_sample_stereo (&left, &right);

    return (left + right) / 2;
  }
  void
  process_sample_stereo (float *left_out, float *right_out)
  {
    osc_impl.set_rate (rate);
    osc_impl.sync_base = to_sync (freq / master_freq);
    osc_impl.pulse_width_base = pulse_width;
    osc_impl.shape_base = shape;
    osc_impl.sub_base = sub;
    osc_impl.sub_width_base = sub_width;
    osc_impl.frequency_base = master_freq;

    return osc_impl.process_sample_stereo (left_out, right_out, 1);
  }
};

}
}

#endif
