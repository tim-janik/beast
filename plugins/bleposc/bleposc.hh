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

#if 0
struct OscImpl
{
  double rate;

  double frequency_base   = 440;
  double frequency_factor = 1;

  double freq_mod_octaves = 0;

  double shape_base       = 0; // 0 = saw, range [-1:1]
  double shape_mod        = 1.0;

  double sub_base         = 0;
  double sub_mod          = 0;

  double sync_base        = 0;
  double sync_mod         = 0;

  double pulse_width_base = 0.5;
  double pulse_width_mod  = 0.0;

  static const int WIDTH = 16;
  static const int OVERSAMPLE = 64;

  static const float blep_delta[WIDTH * OVERSAMPLE + 1];

  double blep_dc;

  enum class State {
    UP,
    DOWN
  };

  struct UnisonVoice
  {
    double phase = 0;
    double master_phase = 0;
    double freq_factor = 1;
    double left_factor = 1;
    double right_factor = 0;
    int future_pos;

    State state     = State::DOWN;
    State sub_state = State::DOWN;

    bool need_slave_reset = false;

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
    void
    reset_slave (double sync_factor, double pulse_width)
    {
      // unfortunately, the parameters required for determining a correct slave
      // phase for a given master phase are only available during block computation
      //
      // so we have to do this as first step of computing a block
      phase = master_phase * sync_factor;
      phase -= int (phase);
      state = phase > pulse_width ? State::UP : State::DOWN;
    }
    void
    reset_master (double new_master_phase, State new_sub_state)
    {
      master_phase     = new_master_phase;
      sub_state        = new_sub_state;
      need_slave_reset = true;
    }
  };
  std::vector<UnisonVoice> unison_voices;

  OscImpl()
  {
    /* compute average total blep dc offset (expected value for linear interpolation) */
    blep_dc = 0;
    for (size_t i = 0; i < WIDTH * OVERSAMPLE; i++)
      {
        blep_dc += (blep_delta[i] + blep_delta[i + 1]) / 2; // average for linear interpolation between values
      }
    blep_dc /= OVERSAMPLE;

    set_unison (1, 0, 0); // default
  }
  void
  reset()
  {
    const bool randomize_phase = unison_voices.size() > 1;

    for (auto& voice : unison_voices)
      {
        voice.init_future();

        if (randomize_phase) // randomize start phase for true unison
          {
            State sub_state = g_random_boolean() ? State::UP : State::DOWN;

            voice.reset_master (g_random_double_range (0, 1), sub_state);
          }
        else
          {
            voice.reset_master (0, State::DOWN);
          }
      }
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
        voice.future[i + voice.future_pos] += blep_delta[pos] * weight_left + blep_delta[pos + 1] * weight_right;

        pos += OVERSAMPLE;
      }
  }
  int
  auto_get_over (unsigned int n_values, const float *freq_in, const float *freq_mod_in, const float *sync_mod_in)
  {
    /* determine maximum freq */
    double max_freq = 0;
    if (freq_in)
      {
        for (unsigned int n = 0; n < n_values; n++)
          max_freq = max (max_freq, BSE_SIGNAL_TO_FREQ (freq_in[n]));
      }
    else
      {
        max_freq = frequency_base;
      }

    /* constant fine tune / transpose */
    max_freq *= frequency_factor;

    /* determine maximum frequency modulation */
    if (freq_mod_in)
      {
        double max_freq_mod = freq_mod_in[0] * freq_mod_octaves;

        for (unsigned int n = 0; n < n_values; n++)
          max_freq_mod = max (max_freq_mod, freq_mod_in[n] * freq_mod_octaves);

        max_freq *= bse_approx5_exp2 (max_freq_mod);
      }

    /* determine maximum sync_factor */
    double max_sync = 0;
    if (sync_mod_in)
      {
        for (unsigned int n = 0; n < n_values; n++)
          max_sync = max (max_sync, sync_mod * sync_mod_in[n]);
      }
    max_freq *= bse_approx5_exp2 (clamp (sync_base + max_sync, 0.0, 60.0) / 12);

    int over = 1;

    while (max_freq / (rate * over) > 0.4)
      over++;

    return over;
  }
  /* check if slave oscillator has reached target_phase
   *
   * when master oscillator sync occurs, only return true if this point in time is
   * before master oscillator sync
   */
  bool
  check_slave_before_master (UnisonVoice& voice, double target_phase, double sync_factor)
  {
    if (voice.phase > target_phase)
      {
        if (voice.master_phase > 1)
          {
            const double slave_frac = (voice.phase - target_phase) / sync_factor;
            const double master_frac = (voice.master_phase - 1);

            return master_frac < slave_frac;
          }
        else
          return true;
      }
    return false;
  }
  double
  clamp (double d, double min, double max)
  {
    return CLAMP (d, min, max);
  }

  static constexpr int FLAG_OVERSAMPLE = 1 << 0;
  static constexpr int FLAG_FREQ_IN    = 1 << 1;
  static constexpr int FLAG_FREQ_MOD   = 1 << 2;
  static constexpr int FLAG_SHAPE_MOD  = 1 << 3;
  static constexpr int FLAG_SUB_MOD    = 1 << 4;
  static constexpr int FLAG_SYNC_MOD   = 1 << 5;
  static constexpr int FLAG_PULSE_MOD  = 1 << 6;

#define USE_TEMPLATE_FUNC 1
  void
  process_sample_stereo (float *left_out, float *right_out, unsigned int n_values,
                         const float *freq_in = nullptr,
                         const float *freq_mod_in = nullptr,
                         const float *shape_mod_in = nullptr,
                         const float *sub_mod_in = nullptr,
                         const float *sync_mod_in = nullptr,
                         const float *pulse_mod_in = nullptr)
  {
    int flags = 0;

    const int over = auto_get_over (n_values, freq_in, freq_mod_in, sync_mod_in);
    if (over > 1)
      flags |= FLAG_OVERSAMPLE;

    if (freq_in)
      flags |= FLAG_FREQ_IN;

    if (freq_mod_in)
      flags |= FLAG_FREQ_MOD;

    if (shape_mod_in)
      flags |= FLAG_SHAPE_MOD;

    if (sub_mod_in)
      flags |= FLAG_SUB_MOD;

    if (sync_mod_in)
      flags |= FLAG_SYNC_MOD;

    if (pulse_mod_in)
      flags |= FLAG_PULSE_MOD;

#if USE_TEMPLATE_FUNC
    switch (flags)
      {
#define BSE_INCLUDER_MATCH(n)   (n >= 0 && n < (1 << 7))
#define BSE_INCLUDER_FUNC(n)    process_block<n>
#define BSE_INCLUDER_ARGS(n)    (left_out, right_out, n_values, over, freq_in, freq_mod_in, shape_mod_in, sub_mod_in, sync_mod_in, pulse_mod_in)
#include <bse/bseincluder.hh>
      }
#else
    process_block (flags, left_out, right_out, n_values, over, freq_in, freq_mod_in, shape_mod_in, sub_mod_in, sync_mod_in, pulse_mod_in);
#endif
  }
#if USE_TEMPLATE_FUNC
  template<int FLAGS> void
  process_block (float *left_out, float *right_out, unsigned int n_values, int xover,
                 const float *freq_in,
                 const float *freq_mod_in,
                 const float *shape_mod_in,
                 const float *sub_mod_in,
                 const float *sync_mod_in,
                 const float *pulse_mod_in)
#else
  void
  process_block (int FLAGS,
                 float *left_out, float *right_out, unsigned int n_values, int xover,
                 const float *freq_in,
                 const float *freq_mod_in,
                 const float *shape_mod_in,
                 const float *sub_mod_in,
                 const float *sync_mod_in,
                 const float *pulse_mod_in)
#endif
  {
    Block::fill (n_values, left_out, 0.0);
    Block::fill (n_values, right_out, 0.0);

    double master_freq = frequency_base;

    double shape = clamp (shape_base, -1.0, 1.0);
    double sub   = clamp (sub_base, 0.0, 1.0);

    double sync_factor = 0; // avoid uninitialized warning
    if ((FLAGS & FLAG_SYNC_MOD) == 0)
      sync_factor = bse_approx5_exp2 (clamp (sync_base, 0.0, 60.0) / 12);

    double pulse_width = 0; // avoid uninitialized warning
    if ((FLAGS & FLAG_PULSE_MOD) == 0)
      pulse_width = clamp (pulse_width_base, 0.01, 0.99);

    const int over = (FLAGS & FLAG_OVERSAMPLE) ? xover : 1;
    for (auto& voice : unison_voices)
      {
        for (unsigned int n = 0; n < n_values; n++)
          {
            if (FLAGS & FLAG_FREQ_IN)
              master_freq = frequency_factor * BSE_SIGNAL_TO_FREQ (freq_in[n]);

            double unison_master_freq = master_freq * voice.freq_factor;

            if (FLAGS & FLAG_FREQ_MOD)
              unison_master_freq *= bse_approx5_exp2 (freq_mod_in[n] * freq_mod_octaves);

            if (FLAGS & FLAG_SHAPE_MOD)
              shape = clamp (shape_base + shape_mod * shape_mod_in[n], -1.0, 1.0);

            if (FLAGS & FLAG_SUB_MOD)
              sub = clamp (sub_base + sub_mod * sub_mod_in[n], 0.0, 1.0);

            if (FLAGS & FLAG_SYNC_MOD)
              sync_factor = bse_approx5_exp2 (clamp (sync_base + sync_mod * sync_mod_in[n], 0.0, 60.0) / 12);

            const double unison_freq = unison_master_freq * sync_factor;

            if (FLAGS & FLAG_PULSE_MOD)
              pulse_width = clamp (pulse_width_base + pulse_width_mod * pulse_mod_in[n], 0.01, 0.99);

            if (voice.need_slave_reset)
              {
                voice.reset_slave (sync_factor, pulse_width);
                voice.need_slave_reset = false;
              }

            for (int i = 0; i < over; i++)
              {
                const double vsub_position = (over - i - 1.0) / over;
                const double vrate = rate * over;

                voice.phase += unison_freq / vrate;
                voice.master_phase += unison_master_freq / vrate;

                if (voice.state == State::DOWN && check_slave_before_master (voice, pulse_width, sync_factor))
                  {
                    voice.state = State::UP;
                    insert_blep (voice, vsub_position + (voice.phase - pulse_width) / (unison_freq / rate), -2.0 * shape * (1 - sub));
                  }
                if (check_slave_before_master (voice, 1, sync_factor))
                  {
                    voice.phase -= 1;

                    double blep_weight = 2.0 * (1 - shape); // sawish part
                    if (voice.state == State::UP)
                      {
                        voice.state = State::DOWN;
                        blep_weight += 2.0 * shape; // pulseish part
                      }

                    insert_blep (voice, vsub_position + voice.phase / (unison_freq / rate), blep_weight * (1 - sub));

                  if (check_slave_before_master (voice, pulse_width, sync_factor))
                      {
                        voice.state = State::UP;
                        insert_blep (voice, vsub_position + (voice.phase - pulse_width) / (unison_freq / rate), -2.0 * shape * (1 - sub));
                      }
                  }
                if (voice.master_phase > 1)
                  {
                    voice.master_phase -= 1;

                    double master_frac = voice.master_phase / (unison_master_freq / rate);

                    double new_phase = master_frac * unison_freq / rate;

                    // sawish part of the wave
                    double blep_weight = (voice.phase - new_phase) * 2.0 * (1 - shape);

                    // pulseish part of the wave
                    if (voice.state == State::UP)
                      {
                        voice.state = State::DOWN;
                        blep_weight += 2.0 * shape;
                      }
                    // sub part of the wave
                    double sub_blep_weight;
                    if (voice.sub_state == State::UP)
                      {
                        voice.sub_state = State::DOWN;
                        sub_blep_weight = 2.0;
                      }
                    else
                      {
                        voice.sub_state = State::UP;
                        sub_blep_weight = -2.0;
                      }

                    insert_blep (voice, vsub_position + master_frac, blep_weight * (1 - sub) + sub_blep_weight * sub);

                    voice.phase = new_phase;
                  }
                /*
                 * we need to check again (same code as before) if we need to switch to UP state,
                 * as voice.phase may be changed - so switching to DOWN and then UP may happen on
                 * the same sample
                 */
                if (voice.state == State::DOWN && voice.phase > pulse_width)
                  {
                    voice.state = State::UP;
                    insert_blep (voice, vsub_position + (voice.phase - pulse_width) / (unison_freq / rate), -2.0 * shape * (1 - sub));
                  }
              }

            double value = (voice.phase * 2 - 1) * (1 - shape);

            if (voice.state == State::DOWN)
              value -= 1 * shape;
            else
              value += 1 * shape;

            /* basic dc offset during saw/pulse production */
            double saw_dc = blep_dc * 2 * unison_freq / rate;

            /* dc offset introduced by sync */
            double sync_len = sync_factor;
            double sync_last_len = sync_len - int (sync_len);
            double sync_dc = -1 + sync_last_len;

            saw_dc += sync_dc * sync_last_len / sync_len;

            double pulse_dc = 1 - pulse_width * 2;
            if (pulse_width < sync_last_len)
              {
                double level = 1 - pulse_width / sync_last_len * 2;
                pulse_dc = (pulse_dc * (sync_len - sync_last_len) + level * sync_last_len) / sync_len;
              }
            else
              {
                double level = -1;
                pulse_dc = (pulse_dc * (sync_len - sync_last_len) + level * sync_last_len) / sync_len;
              }

            /* correct dc offset */
            value -= saw_dc * (1 - shape) + pulse_dc * shape;

            /* sub: introduces no extra dc offset */
            double sub_value = (voice.sub_state == State::DOWN) ? -1 : 1;

            double out = value * (1 - sub) + sub_value * sub + voice.pop_future();

            left_out[n] += out * voice.left_factor;
            right_out[n] += out * voice.right_factor;
          }
      }
  }
};
#endif

class OscImpl
{
public:
  double rate;

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

  double master_phase     = 0;
  double slave_phase      = 0;
  double last_value       = 0;
  double sync_jump_level  = 0;

  static const int WIDTH = 12;
  static const int WSHIFT = 4; // delay to align saw and impulse part of the signal
  static const int OVERSAMPLE = 64;

  static const float impulse_table[WIDTH * OVERSAMPLE + 1];

  enum class State {
    A,
    B,
    C,
    D
  } state = State::A;

  struct UnisonVoice /* FIXME: not implemented yet */
  {
    void
    reset_master (double new_master_phase, State new_sub_state)
    {
      /* FIXME: not implemented yet */
    }
  };

  std::vector<UnisonVoice> unison_voices;

  OscImpl()
  {
    init_future();
    last_value = 1;
    sync_jump_level = 1;
  }
  void
  reset()
  {
    /* FIXME: not implemented yet */
  }
  void
  set_unison (size_t n_voices, float detune, float stereo)
  {
    /* FIXME: not implemented yet */
  }

  int future_pos;

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

  void
  insert_impulse (double frac, double weight)
  {
    int pos = frac * OVERSAMPLE;
    const float inter_frac = frac * OVERSAMPLE - pos;
    const float weight_left = (1 - inter_frac) * weight;
    const float weight_right = inter_frac * weight;

    pos = std::max (pos, 0);
    pos = std::min (pos, OVERSAMPLE - 1);

    for (int i = 0; i < WIDTH; i++)
      {
        future[i + future_pos] += impulse_table[pos] * weight_left + impulse_table[pos + 1] * weight_right;

        pos += OVERSAMPLE;
      }
  }
  void
  insert_future_delta (double weight)
  {
    future[future_pos + WSHIFT] += weight;
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
  check_slave_before_master (double target_phase, double sync_factor)
  {
    if (slave_phase > target_phase)
      {
        if (master_phase > 1)
          {
            const double slave_frac = (slave_phase - target_phase) / sync_factor; // FIXME: ?
            const double master_frac = master_phase - 1;                          // FIXME: ?

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
    double master_freq = frequency_base;
    double pulse_width = clamp (pulse_width_base, 0.01, 0.99);
    double sub         = clamp (sub_base, 0.0, 1.0);
    double sub_width   = clamp (sub_width_base, 0.01, 0.99);
    double shape       = clamp (shape_base, -1.0, 1.0);
    double sync_factor = bse_approx5_exp2 (clamp (sync_base, 0.0, 60.0) / 12);

    const double slave_freq = master_freq * 0.5 * sync_factor;

    /* get leaky integrator constant for sample rate from ms (half-life time) */
    const double leaky_ms = 10;
    const double leaky_a = pow (2.0, -1000.0 / (rate * leaky_ms));

    for (unsigned int n = 0; n < n_values; n++)
      {
        master_phase += master_freq * 0.5 / rate;
        slave_phase += slave_freq / rate;

        bool state_changed;
        do
          {
            state_changed = false;

            if (state == State::A)
              {
                const double bound_a = sub_width * pulse_width;

                if (check_slave_before_master (bound_a, sync_factor)) //slave_phase > bound_a)
                  {
                    double slave_frac = (slave_phase - bound_a) / (slave_freq / rate);

                    insert_impulse (slave_frac, 2.0 * (shape * (1 - sub) - sub));
                    sync_jump_level += 2.0 * (shape * (1 - sub) - sub);
                    state = State::B;
                    state_changed = true;
                  }
              }
            if (state == State::B)
              {
                const double bound_b = 2 * sub_width * pulse_width + 1 - sub_width - pulse_width;

                if (check_slave_before_master (bound_b, sync_factor)) //slave_phase > bound_b)
                  {
                    double slave_frac = (slave_phase - bound_b) / (slave_freq / rate);

                    insert_impulse (slave_frac, 2.0 * (1 - sub));
                    sync_jump_level += 2 * (1 - sub);
                    state = State::C;
                    state_changed = true;
                  }
              }
            if (state == State::C)
              {
                const double bound_c = sub_width * pulse_width + (1 - sub_width);

                if (check_slave_before_master (bound_c, sync_factor)) //slave_phase > bound_c)
                  {
                    double slave_frac = (slave_phase - bound_c) / (slave_freq / rate);

                    insert_impulse (slave_frac, 2.0 * (shape * (1 - sub) + sub));
                    sync_jump_level += 2.0 * (shape * (1 - sub) + sub);
                    state = State::D;
                    state_changed = true;
                  }
              }
            if (state == State::D)
              {
                if (check_slave_before_master (1, sync_factor)) //slave_phase > 1)
                  {
                    slave_phase -= 1;

                    double slave_frac = slave_phase / (slave_freq / rate);

                    insert_impulse (slave_frac, 2.0 * (1 - sub));
                    sync_jump_level += 2.0 * (1 - sub);
                    state = State::A;
                    state_changed = true;
                  }
              }
            // FIXME: we may want to recheck state == State::A (and so on)
            if (master_phase > 1)
              {
                master_phase -= 1;

                double master_frac = master_phase / (master_freq * 0.5 / rate);

                insert_impulse (master_frac, (1 - sync_jump_level) + 4.0 * (shape + 1) * (1 - sub) * sync_factor);

                slave_phase = master_phase * sync_factor;
                sync_jump_level = 1;

                state = State::A;
                state_changed = true;
              }
          }
        while (state_changed); // rerun all state checks if state was modified

        double saw_delta = -4.0 * slave_freq / rate * (shape + 1) * (1 - sub);
        insert_future_delta (saw_delta); // align with the impulses

        /* leaky integration */
        double value = leaky_a * last_value + pop_future();
        last_value = value;

        left_out[n] = right_out[n] = value;
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
  process_sample()
  {
    float left, right;

    process_sample_stereo (&left, &right);

    return (left + right) / 2;
  }
  void
  process_sample_stereo (float *left_out, float *right_out)
  {
    osc_impl.rate = rate;
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
