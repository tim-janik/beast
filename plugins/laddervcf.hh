// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
//
#ifndef __BSE_LADDER_VCF_HH__
#define __BSE_LADDER_VCF_HH__

#include <bse/bseresampler.hh>

namespace Bse {

enum class LadderVCFMode { LP1, LP2, LP3, LP4 };

template<bool OVERSAMPLE, bool NON_LINEAR>
class LadderVCF
{
  struct Channel {
    double x1, x2, x3, x4;
    double y1, y2, y3, y4;

    // NOTE: Bse currently doesn't enforce SSE alignment so we force FPU resampling
    Resampler2 res_up   { Resampler2::UP,   Resampler2::PREC_48DB, false };
    Resampler2 res_down { Resampler2::DOWN, Resampler2::PREC_48DB, false };
  };
  std::array<Channel, 2> channels;
  LadderVCFMode mode;
  double pre_scale, post_scale;
  double rate;
  double freq_mod_octaves;
  double key_tracking;
  double resonance_mod;

  double last_key_freq;
  double last_key_tracking_factor;

public:
  LadderVCF()
  {
    reset();
    set_mode (LadderVCFMode::LP4);
    set_drive (0);
    set_rate (48000);
    set_freq_mod_octaves (5);
    set_key_tracking (0.5);
    set_resonance_mod (0.1);
  }
  void
  set_mode (LadderVCFMode new_mode)
  {
    mode = new_mode;
  }
  void
  set_drive (double drive_db)
  {
    const double drive_delta_db = 36;

    pre_scale = bse_db_to_factor (drive_db - drive_delta_db);
    post_scale = std::max (1 / pre_scale, 1.0);
  }
  void
  set_rate (double r)
  {
    rate = r;
  }
  void
  set_freq_mod_octaves (double octaves)
  {
    freq_mod_octaves = octaves;
  }
  void
  set_key_tracking (double amount)
  {
    key_tracking = amount;

    // force recomputing key tracking factor
    last_key_freq = -1;
    last_key_tracking_factor = 0;
  }
  void
  set_resonance_mod (double amount)
  {
    resonance_mod = amount;
  }
  void
  reset()
  {
    for (auto& c : channels)
      {
        c.x1 = c.x2 = c.x3 = c.x4 = 0;
        c.y1 = c.y2 = c.y3 = c.y4 = 0;

        c.res_up.reset();
        c.res_down.reset();
      }
    last_key_freq = -1;
    last_key_tracking_factor = 0;
  }
  double
  distort (double x)
  {
    if (NON_LINEAR)
      {
        /* shaped somewhat similar to tanh() and others, but faster */
        x = std::clamp (x, -1.0, 1.0);

        return x - x * x * x * (1.0 / 3);
      }
    else
      {
        return x;
      }
  }
private:
  template<int CHANNEL_MASK> inline bool
  need_channel (uint ch)
  {
    return (1 << ch) & CHANNEL_MASK;
  }
  /*
   * This ladder filter implementation is mainly based on
   *
   * Välimäki, Vesa & Huovilainen, Antti. (2006).
   * Oscillator and Filter Algorithms for Virtual Analog Synthesis.
   * Computer Music Journal. 30. 19-31. 10.1162/comj.2006.30.2.19.
   */
  template<LadderVCFMode MODE, int CHANNEL_MASK> inline void
  run (double *values, double fc, double res)
  {
    fc = M_PI * fc;
    const double g = 0.9892 * fc - 0.4342 * fc * fc + 0.1381 * fc * fc * fc - 0.0202 * fc * fc * fc * fc;
    const double gg = g * g;

    res *= 1.0029 + 0.0526 * fc - 0.0926 * fc * fc + 0.0218 * fc * fc * fc;

    constexpr uint oversample_count = OVERSAMPLE ? 2 : 1;
    for (uint os = 0; os < oversample_count; os++)
      {
        for (uint i = 0; i < channels.size(); i++)
          {
            if (need_channel<CHANNEL_MASK> (i))
              {
                Channel& c = channels[i];
                const double x = values[i] * pre_scale;
                const double g_comp = 0.5; // passband gain correction
                const double x0 = distort (x - (c.y4 - g_comp * x) * res * 4) * gg * gg * (1.0 / 1.3 / 1.3 / 1.3 / 1.3);

                c.y1 = x0 + c.x1 * 0.3 + c.y1 * (1 - g);
                c.x1 = x0;

                c.y2 = c.y1 + c.x2 * 0.3 + c.y2 * (1 - g);
                c.x2 = c.y1;

                c.y3 = c.y2 + c.x3 * 0.3 + c.y3 * (1 - g);
                c.x3 = c.y2;

                c.y4 = c.y3 + c.x4 * 0.3 + c.y4 * (1 - g);
                c.x4 = c.y3;

                switch (MODE)
                  {
                    case LadderVCFMode::LP1:
                      values[i] = c.y1 / (gg * g * (1.0 / (1.3 * 1.3 * 1.3))) * post_scale;
                      break;
                    case LadderVCFMode::LP2:
                      values[i] = c.y2 / (gg * (1.0 / (1.3 * 1.3))) * post_scale;
                      break;
                    case LadderVCFMode::LP3:
                      values[i] = c.y3 / (g * (1.0 / 1.3)) * post_scale;
                      break;
                    case LadderVCFMode::LP4:
                      values[i] = c.y4 * post_scale;
                      break;
                    default:
                      BSE_ASSERT_RETURN_UNREACHED();
                  }
              }
          }
        values += channels.size();
      }
  }
  template<LadderVCFMode MODE, int CHANNEL_MASK> inline void
  do_run_block (uint          n_samples,
                double        fc,
                double        res,
                const float **inputs,
                float       **outputs,
                const float  *freq_in,
                const float  *freq_mod_in,
                const float  *key_freq_in,
                const float  *reso_mod_in)
  {
    float over_samples[2][2 * n_samples];
    float freq_scale = OVERSAMPLE ? 0.5 : 1.0;
    float nyquist    = rate * 0.5;

    if (OVERSAMPLE)
      {
        for (size_t i = 0; i < channels.size(); i++)
          if (need_channel<CHANNEL_MASK> (i))
            channels[i].res_up.process_block (inputs[i], n_samples, over_samples[i]);
      }

    fc *= freq_scale;

    for (uint i = 0; i < n_samples; i++)
      {
        double mod_fc = fc;
        double mod_res = res;

        if (freq_in)
          mod_fc = BSE_SIGNAL_TO_FREQ (freq_in[i]) * freq_scale / nyquist;

        if (freq_mod_in)
          mod_fc *= bse_approx3_exp2 (freq_mod_in[i] * freq_mod_octaves);

        if (key_freq_in)
          {
            if (BSE_UNLIKELY (BSE_SIGNAL_FREQ_CHANGED (key_freq_in[i], last_key_freq)))
              {
                /* key tracking from frequency is expensive to compute; with this optimization
                 * it should be fast, because the key frequency usually doesn't change
                 */
                last_key_freq = key_freq_in[i];
                last_key_tracking_factor = exp (key_tracking * log (BSE_SIGNAL_TO_FREQ (key_freq_in[i]) / 261.63));
              }
            mod_fc *= last_key_tracking_factor;
          }
        if (reso_mod_in)
          mod_res = std::clamp (mod_res + resonance_mod * reso_mod_in[i], 0.0, 1.0);

        mod_fc = std::clamp (mod_fc, 0.0, 1.0);

        if (OVERSAMPLE)
          {
            const uint over_pos = i * 2;
            double values[4] = {
              over_samples[0][over_pos],
              over_samples[1][over_pos],
              over_samples[0][over_pos + 1],
              over_samples[1][over_pos + 1],
            };

            run<MODE, CHANNEL_MASK> (values, mod_fc, mod_res);

            over_samples[0][over_pos] = values[0];
            over_samples[1][over_pos] = values[1];
            over_samples[0][over_pos + 1] = values[2];
            over_samples[1][over_pos + 1] = values[3];
          }
        else
          {
            double values[2] = { inputs[0][i], inputs[1][i] };

            run<MODE, CHANNEL_MASK> (values, mod_fc, mod_res);

            outputs[0][i] = values[0];
            outputs[1][i] = values[1];
          }
      }
    if (OVERSAMPLE)
      {
        for (size_t i = 0; i < channels.size(); i++)
          if (need_channel<CHANNEL_MASK> (i))
            channels[i].res_down.process_block (over_samples[i], 2 * n_samples, outputs[i]);
      }
  }
  template<LadderVCFMode MODE> inline void
  run_block_mode (uint          n_samples,
                  double        fc,
                  double        res,
                  const float **inputs,
                  float       **outputs,
                  int           channel_mask,
                  const float  *freq_in,
                  const float  *freq_mod_in,
                  const float  *key_freq_in,
                  const float  *reso_mod_in)
  {
    switch (channel_mask)
      {
        case 0: do_run_block<MODE, 0> (n_samples, fc, res, inputs, outputs, freq_in, freq_mod_in, key_freq_in, reso_mod_in);
                break;
        case 1: do_run_block<MODE, 1> (n_samples, fc, res, inputs, outputs, freq_in, freq_mod_in, key_freq_in, reso_mod_in);
                break;
        case 2: do_run_block<MODE, 2> (n_samples, fc, res, inputs, outputs, freq_in, freq_mod_in, key_freq_in, reso_mod_in);
                break;
        case 3: do_run_block<MODE, 3> (n_samples, fc, res, inputs, outputs, freq_in, freq_mod_in, key_freq_in, reso_mod_in);
                break;
        default: BSE_ASSERT_RETURN_UNREACHED();
      }
  }
public:
  void
  run_block (uint           n_samples,
             double         fc,
             double         res,
             const float  **inputs,
             float        **outputs,
             bool           need_left,
             bool           need_right,
             const float   *freq_in,
             const float   *freq_mod_in,
             const float   *key_freq_in,
             const float   *reso_mod_in)
  {
    int channel_mask = 0;
    if (need_left)
      channel_mask |= 1;
    if (need_right)
      channel_mask |= 2;
    switch (mode)
    {
      case LadderVCFMode::LP4: run_block_mode<LadderVCFMode::LP4> (n_samples, fc, res, inputs, outputs, channel_mask,
                                                                   freq_in, freq_mod_in, key_freq_in, reso_mod_in);
                               break;
      case LadderVCFMode::LP3: run_block_mode<LadderVCFMode::LP3> (n_samples, fc, res, inputs, outputs, channel_mask,
                                                                   freq_in, freq_mod_in, key_freq_in, reso_mod_in);
                               break;
      case LadderVCFMode::LP2: run_block_mode<LadderVCFMode::LP2> (n_samples, fc, res, inputs, outputs, channel_mask,
                                                                   freq_in, freq_mod_in, key_freq_in, reso_mod_in);
                               break;
      case LadderVCFMode::LP1: run_block_mode<LadderVCFMode::LP1> (n_samples, fc, res, inputs, outputs, channel_mask,
                                                                   freq_in, freq_mod_in, key_freq_in, reso_mod_in);
                               break;
    }
  }
};

// fast linear model of the filter
typedef LadderVCF<false, false> LadderVCFLinear;

// slow but accurate non-linear model of the filter (uses oversampling)
typedef LadderVCF<true,  true>  LadderVCFNonLinear;

// fast non-linear version (no oversampling), may have aliasing
typedef LadderVCF<false, true>  LadderVCFNonLinearCheap;

}

#endif
