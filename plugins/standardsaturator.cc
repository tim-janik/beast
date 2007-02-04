/* Saturator - Saturation module                -*-mode: c++;-*-
 * Copyright (C) 2004 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "standardsaturator.genidl.hh"
#include <bse/bseengine.h>
#include <bse/bsemathsignal.h>

#define TEST_GNUPLOT    0

namespace Bse { namespace Standard {

class Saturator : public SaturatorBase {
  class Module: public SynthesisModule {
    SaturationType saturation;
    double         level;
    bool           auto_output;
    double         olevel;
  public:
    void
    config (SaturatorProperties *params)
    {
      saturation = params->saturation;
      level = params->level * 0.01;
      auto_output = params->auto_output;
      olevel = params->output_volume;
#if TEST_GNUPLOT
      if (params->test_dump)
        test_gnuplot();
#endif
    }
    void
    auto_update (SaturatorPropertyID prop_id,
                 double              value)
    {
      switch (prop_id)
        {
        case PROP_OUTPUT_VOLUME:
          if (!auto_output)
            olevel = value;
          break;
        default: ;
        }
    }
    void
    reset ()
    {
    }
#if TEST_GNUPLOT
    void
    test_gnuplot (void)
    {
      double lower = -25, upper = +25, step = 0.01;
      guint n = int ((upper - lower) / step + 1024), i = 0;
      float *block = g_new0 (float, n * 2), *out = block + n;
      for (double d = -25; d <= +25; d += 0.01)
        block[i++] = d;
      g_assert (i < n);
      double saved_olevel = olevel;
      olevel = 1 / 1.0;
      process_block (i, block, out, SaturateTanh (1.0));
      bse_float_gnuplot ("/tmp/tanh10", lower, step, i, out);
      process_block (i, block, out, SaturateAtan (1.0));
      bse_float_gnuplot ("/tmp/atan10", lower, step, i, out);
      process_block (i, block, out, SaturateQuadratic (1.0));
      bse_float_gnuplot ("/tmp/quad10", lower, step, i, out);
      process_block (i, block, out, SaturateSoftKnee (1.0));
      bse_float_gnuplot ("/tmp/knee10", lower, step, i, out);
      process_block (i, block, out, SaturateHard (1.0));
      bse_float_gnuplot ("/tmp/hard10", lower, step, i, out);
      /* plot "/tmp/tanh10" with lines, "/tmp/atan10" with lines, "/tmp/quad10" with lines, "/tmp/knee10" with lines, "/tmp/hard10" with lines */
      olevel = 1 / 0.5;
      process_block (i, block, out, SaturateTanh (0.5));
      bse_float_gnuplot ("/tmp/tanh05", lower, step, i, out);
      process_block (i, block, out, SaturateAtan (0.5));
      bse_float_gnuplot ("/tmp/atan05", lower, step, i, out);
      process_block (i, block, out, SaturateQuadratic (0.5));
      bse_float_gnuplot ("/tmp/quad05", lower, step, i, out);
      process_block (i, block, out, SaturateSoftKnee (0.5));
      bse_float_gnuplot ("/tmp/knee05", lower, step, i, out);
      process_block (i, block, out, SaturateHard (0.5));
      bse_float_gnuplot ("/tmp/hard05", lower, step, i, out);
      /* plot "/tmp/tanh05" with lines, "/tmp/atan05" with lines, "/tmp/quad05" with lines, "/tmp/knee05" with lines, "/tmp/hard05" with lines */
      olevel = saved_olevel;
    }
#endif
    template<typename Saturate> inline void
    process_block (guint        n_values,
                   const float *in,
                   float       *out,
                   Saturate     saturate)
    {
      if (olevel == 1)
        for (guint i = 0; i < n_values; i++)
          out[i] = saturate (in[i]);
      else
        for (guint i = 0; i < n_values; i++)
          out[i] = saturate (in[i]) * olevel;
    }
    struct SaturateTanh {
      double prescale;
      SaturateTanh (double level) :
        prescale (atanh (MIN (0.999, level)))
      {}
      inline float
      operator() (float input)
      {
        return bse_approx4_tanh (input * prescale);
      }
    };
    struct SaturateAtan {
      double prescale;
      SaturateAtan (double level) :
        prescale (tan (MIN (0.99, level) * BSE_PI_DIV_2))
      {}
      inline float
      operator() (float input)
      {
        return bse_approx_atan1 (input * prescale);
      }
    };
    struct SaturateQuadratic {
      double limit;
      SaturateQuadratic (double h) :
        limit (h)
      {}
      inline float
      operator() (float x)
      {
        if (G_UNLIKELY (x > 1.0))
          return limit;
        else if (G_UNLIKELY (x < -1.0))
          return -limit;
        return limit * (2 * x - x * fabs (x));
        /* satq(x,h)=x>1 ? h : x<-1 ? -h : (2*x - x*abs(x))*h */
      }
    };
    struct SaturateSoftKnee {
      double a, am1, inva12;
      SaturateSoftKnee (double threshold) :
        a (threshold), am1 (a - 1), inva12 (2.0 * a / (a + 1))
      {}
      /* this soft knee saturation type is based on a wave shaper by Bram De Jong */
      inline float
      piece_a1 (float x)
      {
        double xma = x - a, xaf = xma / am1;
        return (a + xma / (1 + xaf * xaf)) * inva12;
      }
      inline float
      operator() (float x)
      {
        if (G_UNLIKELY (x > 1.0))
          return a;
        else if (G_UNLIKELY (x < -1.0))
          return -a;
        if (x >= 0)
          return x <= a ? x * inva12 : piece_a1 (x);
        else
          return x >= -a ? x * inva12 : -piece_a1 (-x);
        /* bdj_a1(x,a) = (a + (x-a) / (1 + ((x-a) / (a-1))**2)) * (2.0 * a / (a + 1))
         * bdj(x,a)=x>1 ? a : x<-1 ? -a : x >= 0 ? (x<=a ? x * (2*a/(a+1)) : bdj_a1 (x,a)) : (x>=-a ? x * (2*a/(a+1)) : -bdj_a1 (-x,a))
         */
      }
    };
    struct SaturateHard {
      double limit;
      SaturateHard (double l) :
        limit (l)
      {}
      inline float
      operator() (float input)
      {
        return bse_saturate_hard (input, limit);
      }
    };
    inline void
    process_channel (unsigned int n_values,
                     const float *in,
                     float       *out)
    {
      switch (saturation)
        {
        case SATURATE_TANH:
          process_block (n_values, in, out, SaturateTanh (level));
          break;
        case SATURATE_ATAN:
          process_block (n_values, in, out, SaturateAtan (level));
          break;
        case SATURATE_QUADRATIC:
          process_block (n_values, in, out, SaturateQuadratic (level));
          break;
        case SATURATE_SOFT_KNEE:
          process_block (n_values, in, out, SaturateSoftKnee (level));
          break;
        case SATURATE_HARD:
          process_block (n_values, in, out, SaturateHard (level));
          break;
        }
    }
    void
    process (unsigned int n_values)
    {
      const float *in1  = istream (ICHANNEL_AUDIO_IN1).values;
      float       *out1 = ostream (OCHANNEL_AUDIO_OUT1).values;
      const float *in2  = istream (ICHANNEL_AUDIO_IN2).values;
      float       *out2 = ostream (OCHANNEL_AUDIO_OUT2).values;
      if (ostream (Saturator::OCHANNEL_AUDIO_OUT1).connected)
        process_channel (n_values, in1, out1);
      if (ostream (Saturator::OCHANNEL_AUDIO_OUT2).connected)
        process_channel (n_values, in2, out2);
    }
  };
  bool
  property_changed (SaturatorPropertyID prop_id)
  {
    switch (prop_id)
      {
      case PROP_OUTPUT_VOLUME: /* triggered by automation */
      case PROP_LEVEL:
      case PROP_AUTO_OUTPUT:
        if (auto_output)
          {
            double maxol = bse_db_to_factor (42);
            output_volume = 1 / MAX (level * 0.01, 0.000001);
            output_volume = MIN (output_volume, maxol);
          }
        notify ("output_volume");
        break;
      default: ;
      }
    return FALSE;
  }
  bool
  editable_property (SaturatorPropertyID prop_id,
                     GParamSpec         *)
  {
    if (prop_id == PROP_OUTPUT_VOLUME && auto_output)
      return false;
    return true;
  }
public:
  /* implement creation and config methods for synthesis Module */
  BSE_EFFECT_INTEGRATE_MODULE (Saturator, Module, SaturatorProperties);
};

BSE_CXX_DEFINE_EXPORTS();
BSE_CXX_REGISTER_ALL_TYPES_FROM_STANDARDSATURATOR_IDL();

} } // Bse::Standard
