/* BSE - Better Sound Engine
 * Copyright (C) 2003 Tim Janik
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
#include "bsebalance.genidl.hh"

namespace Bse {

class Balance : public BalanceBase {
  /* balance module implementation */
  class Module : public SynthesisModule {
    /* coefficients: */
    double al1, al2;    // audio input levels
    double cl1, cl2;    // control input levels
    double ob, ocs;     // output balance and strength
    double lp;          // lowpass ascent
    /* state: */
    float xd1;
  public:
    void
    config (BalanceProperties *params)
    {
      al1 = params->alevel1 * 0.01;
      al2 = params->alevel2 * 0.01;
      cl1 = params->clevel1 * 0.01;
      cl2 = params->clevel2 * 0.01;
      ob = params->obalance * 0.5 * 0.01;
      ocs = params->ostrength * 0.5 * 0.01;
      lp = mix_freq() / params->lowpass;
    }
    void
    reset ()
    {
      xd1 = 0;
    }
    void
    process (unsigned int n_values)
    {
      /* this function runs in various synthesis threads */
      const float *a1 = istream (ICHANNEL_AUDIO_IN1).values;
      const float *a2 = istream (ICHANNEL_AUDIO_IN2).values;
      const float *c1 = istream (ICHANNEL_CTRL_IN1).values;
      const float *c2 = istream (ICHANNEL_CTRL_IN2).values;
      float *mix = ostream (OCHANNEL_MIX_OUT).values;
      float *left = ostream (OCHANNEL_LEFT_OUT).values;
      float *right = ostream (OCHANNEL_RIGHT_OUT).values;
      float *bound = left + n_values;
      double lpfrac = 1.0 / lp;
      double lpifrac = 1.0 - lpfrac;
      double w = xd1; // fetch state
      while (left < bound)
        {
          double c = *c1++ * cl1 + *c2++ * cl2;
          double a = *a1++ * al1 + *a2++ * al2;
          double b = ob + c * ocs;
          b = CLAMP (b, -0.5, 0.5);
          w = w * lpifrac + b * lpfrac;
          *mix++ = a;
          *left++ = a * (0.5 - w);
          *right++ = a * (0.5 + w);
        }
      xd1 = w; // store state
    }
  };
protected:
  bool
  property_changed (BalancePropertyID prop_id)
  {
    /* this is called after a property changed and _before_ the
     * modules are updated.
     */
    switch (prop_id)
      {
        /* implement special handling of GUI properties */
      case PROP_ALEVEL1:
      case PROP_ALEVEL2:
        abalance = bse_balance_get (alevel1, alevel2);
        notify ("abalance");
        break;
      case PROP_ABALANCE:
        bse_balance_set (abalance, &alevel1, &alevel2);
        notify ("alevel1");
        notify ("alevel2");
        break;
      case PROP_CLEVEL1:
      case PROP_CLEVEL2:
        cbalance = bse_balance_get (clevel1, clevel2);
        notify ("cbalance");
        break;
      case PROP_CBALANCE:
        bse_balance_set (cbalance, &clevel1, &clevel2);
        notify ("clevel1");
        notify ("clevel2");
        break;
      default: ;
      }
    return false;
  }
public:
  /* implement creation and config methods for synthesis Module */
  BSE_EFFECT_INTEGRATE_MODULE (Balance, Module, BalanceProperties);
};

BSE_CXX_DEFINE_EXPORTS();
BSE_CXX_REGISTER_EFFECT (Balance);

} // Bse
