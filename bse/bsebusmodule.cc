/* BSE - Better Sound Engine
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
#include "bsebusmodule.genidl.hh"

namespace Bse {

class BusModule : public BusModuleBase {
  /* bus-module module implementation */
  class Module : public SynthesisModule {
    /* configuration: */
    double level1, level2;
  public:
    void
    config (BusModuleProperties *params)
    {
      level1 = params->volume1;
      level2 = params->volume2;
    }
    void
    reset ()
    {
    }
    void
    process (uint n_values)
    {
      if (istream (ICHANNEL_AUDIO_IN1).connected && ostream (OCHANNEL_AUDIO_OUT1).connected)
        {
          const float *audio_in = istream (ICHANNEL_AUDIO_IN1).values;
          if (level1 == 1.0)
            ostream_set (OCHANNEL_AUDIO_OUT1, audio_in);
          else if (level1 == 0.0)
            ostream_set (OCHANNEL_AUDIO_OUT1, const_values (0.0));
          else
            {
              float *audio_out = ostream (OCHANNEL_AUDIO_OUT1).values;
              float *const audio_bound = audio_out + n_values;
              while (audio_out < audio_bound)
                *audio_out++ = level1 * *audio_in++;
            }
        }
      if (istream (ICHANNEL_AUDIO_IN2).connected && ostream (OCHANNEL_AUDIO_OUT2).connected)
        {
          const float *audio_in = istream (ICHANNEL_AUDIO_IN2).values;
          if (level2 == 1.0)
            ostream_set (OCHANNEL_AUDIO_OUT2, audio_in);
          else if (level2 == 0.0)
            ostream_set (OCHANNEL_AUDIO_OUT2, const_values (0.0));
          else
            {
              float *audio_out = ostream (OCHANNEL_AUDIO_OUT2).values;
              float *const audio_bound = audio_out + n_values;
              while (audio_out < audio_bound)
                *audio_out++ = level2 * *audio_in++;
            }
        }
    }
  };
public:
  /* implement creation and config methods for synthesis Module */
  BSE_EFFECT_INTEGRATE_MODULE (BusModule, Module, BusModuleProperties);
};

BSE_CXX_DEFINE_EXPORTS ();
BSE_CXX_REGISTER_EFFECT (BusModule);

} // Bse
