/* DavBassFilter - DAV Bass Filter
 * Copyright (c) 1999, 2000 David A. Bartold, 2003 Tim Janik
 *
 * This is a TB-303 filter clone based on the VCF303 portions of
 * gsyn v0.2.  Code in update_locals() is copyright (c) 1998 Andy Sloane.
 *
 * Filter algorithm in function recalc_a_b() is based on the one
 * described in Musical Applications of Microprocessors by Hal Chamberlin.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "davbassfilter.gen-idl.h"

namespace Bse {
namespace Dav {

class BassFilter : public BassFilterBase {
  class Module : public SynthesisModule {
    /* proeprties */
    double filt_cutoff, filt_reso, env_mod, env_decay;
    /* satte */
    double decay, resonance;
    double a, b, c0;
    double d1, d2;
    double e0, e1;
    float last_trigger;
    gint envbound; /* 64 at 44100 */
    gint envpos;
    inline void
    recalc_resonance ()
    {
      /* Update resonance. */
      resonance = exp (-1.20 + 3.455 * filt_reso);
    }
    inline void
    recalc_filter ()
    {
      /* Update vars given envmod, cutoff, and reso. */
      e0 = exp (5.613 - 0.8 * env_mod + 2.1553 * filt_cutoff - 0.7696 * (1.0 - filt_reso));
      e1 = exp (6.109 + 1.5876 * env_mod + 2.1553 * filt_cutoff - 1.2 * (1.0 - filt_reso));
      e0 *= PI / mix_freq();
      e1 *= PI / mix_freq();
      e1 -= e0;
    }
    inline void
    recalc_decay ()
    {
      /* Update decay given envdecay. */
      envbound = dtoi (0.001452 * mix_freq()); // 64 at 44100;
      envbound = MAX (envbound, 1);
      double d = env_decay;
      d = 0.2 + (2.3 * d);
      d *= mix_freq();
      decay = pow (0.1, envbound / d);
    }
    inline void
    recalc_a_b ()
    {
      double whopping = e0 + c0;
      double k = exp (-whopping / resonance);
      a = 2.0 * cos (2.0 * whopping) * k;
      b = -k * k;
    }
  public:
    void
    reset ()
    {
      last_trigger = 0;
      c0 = 0;
      d1 = d2 = 0;
      envpos = 0;
    }
    void
    config (BassFilterProperties *props)
    {
      filt_cutoff = props->cutoff_perc * 0.01;
      filt_reso = props->reso_perc * 0.01;
      env_mod = props->env_mod * 0.01;
      env_decay = props->env_decay * 0.01;

      recalc_resonance();
      recalc_filter();
      recalc_decay();
      recalc_a_b();

      if (props->trigger)
        {
          /* Reset filter delta freq. */
          c0 = e1;
          envpos = 0;
        }
    }
    void
    auto_update (BassFilterPropertyID prop_id,
                 double               value)
    {
      switch (prop_id)
        {
        case PROP_CUTOFF_PERC:
          filt_cutoff = value * 0.01;
          recalc_filter();
          recalc_a_b();
          break;
        case PROP_RESO_PERC:
          filt_reso = value * 0.01;
          recalc_resonance();
          recalc_filter();
          recalc_a_b();
          break;
        case PROP_ENV_MOD:
          env_mod = value * 0.01;
          recalc_filter();
          recalc_a_b();
          break;
        case PROP_ENV_DECAY:
          env_decay = value * 0.01;
          recalc_decay();
          break;
        default: ;
        }
    }
    void
    process (unsigned int n_values)
    {   /* this function runs in various synthesis threads */
      const float *in = istream (ICHANNEL_AUDIO_IN).values;
      const float *trigger = istream (ICHANNEL_TRIGGER_IN).values;
      float *out = ostream (OCHANNEL_AUDIO_OUT).values;
      float *bound = out + n_values;
      if (istream (ICHANNEL_TRIGGER_IN).connected)
        while (out < bound)
          {
            gfloat current_trigger = *trigger++;
            if (G_UNLIKELY (last_trigger < current_trigger))
              {
                c0 = e1;
                envpos = 0;
              }
            last_trigger = current_trigger;
            double c = (1.0 - a - b) * 0.2;
            double v = a * d1 + b * d2 + c * *in++;
            d2 = d1;
            d1 = v;
            *out++ = v;
            if (++envpos >= envbound)
              {
                envpos = 0;
                c0 *= decay;
                recalc_a_b ();
              }
          }
      else
        while (out < bound)
          {
            double c = (1.0 - a - b) * 0.2;
            double v = a * d1 + b * d2 + c * *in++;
            d2 = d1;
            d1 = v;
            *out++ = v;
            if (++envpos >= envbound)
              {
                envpos = 0;
                c0 *= decay;
                recalc_a_b ();
              }
          }
    }
  };
public:
  /* implement creation and config methods for synthesis Module */
  BSE_EFFECT_INTEGRATE_MODULE (BassFilter, Module, BassFilterProperties);
};

BSE_CXX_DEFINE_EXPORTS();
BSE_CXX_REGISTER_EFFECT (BassFilter);

} // Dav
} // Bse
