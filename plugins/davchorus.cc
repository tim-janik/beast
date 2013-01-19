// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "davchorus.genidl.hh"
namespace Bse {
namespace Dav {
class Chorus : public ChorusBase {
  /* synthesis module: */
  class Module : public SynthesisModule {
    /* delay line */
    gint   delay_length;
    float *delay;
    /* state: */
    gint   delay_pos;
    gfloat sine_pos;
    gfloat sine_delta;
    /* parameters */
    double wet_out;
  public:
    Module()
    {
      delay_length = mix_freq() / 40;
      delay = g_new0 (float, delay_length);
    }
    ~Module()
    {
      g_free (delay);
    }
    void
    reset ()
    {
      delay_pos = 0;
      sine_pos = 0.0;
      sine_delta = 0.08 * 2.0 * M_PI / mix_freq();        // FIXME: should be very-low frequency property
    }
    void
    config (ChorusProperties *params)
    {
      wet_out = params->wet_out / 100.0;
    }
    void
    process (unsigned int n_values)
    {
      const float *input = istream (ICHANNEL_AUDIO_IN).values;
      float *output = ostream (OCHANNEL_AUDIO_OUT).values;
      float *bound = output + n_values;
      double dry_out = 1.0 - wet_out;
      while (output < bound)      // FIXME: this loop should be much more optimized
        {
          delay[delay_pos] = *input++;
          int hi_pos = delay_pos;
          int lo_pos = dtoi ((sin (sine_pos) + 1.0) * (delay_length - 1) * 256.0 * 0.5);
          /* Normalize hi_pos and lo_pos counters. */
          hi_pos += lo_pos >> 8;
          lo_pos &= 0xff;
          /* Find hi_pos modulus delay_length. */
          while (hi_pos >= delay_length)
            hi_pos -= delay_length;
          /* Perform linear interpolation between hi_pos and hi_pos + 1. */
          double wet = delay[hi_pos] * (256 - lo_pos);
          hi_pos++;
          if (hi_pos >= delay_length)
            hi_pos -= delay_length;
          wet += delay[hi_pos] * lo_pos;
          wet = (wet / 256.0 + delay[delay_pos]) / 2;
          *output++ = wet * wet_out + delay[delay_pos] * dry_out;
          delay_pos++;
          if (delay_pos >= delay_length)
            delay_pos = 0;
          sine_pos += sine_delta;
          while (sine_pos >= 2.0 * M_PI)
            sine_pos -= 2.0 * M_PI;
        }
    }
  };
public:
  BSE_EFFECT_INTEGRATE_MODULE (Chorus, Module, ChorusProperties);
};
BSE_CXX_DEFINE_EXPORTS();
BSE_CXX_REGISTER_EFFECT (Chorus);
} // Dav
} // Bse
