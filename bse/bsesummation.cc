/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2004 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bsesummation.genidl.hh"
using namespace std;
using namespace Sfi;

namespace Bse {

class Summation : public SummationBase {
  class Summer : public SynthesisModule {
  public:
    void
    config (SummationProperties *params)
    {
    }
    void
    reset ()
    {
    }
    void
    process (unsigned int n_values)
    {
      if (ostream (OCHANNEL_AUDIO_OUT1).connected)
        {
          if (jstream (JCHANNEL_AUDIO_IN1).n_connections > 1)
            {
              gfloat *ovalues = ostream (OCHANNEL_AUDIO_OUT1).values, *bound = ovalues + n_values;
              memcpy (ovalues, jstream (JCHANNEL_AUDIO_IN1).values[0], sizeof (ovalues[0]) * n_values);
              for (guint i = 1; i < jstream (JCHANNEL_AUDIO_IN1).n_connections; i++)
                {
                  const gfloat *s = jstream (JCHANNEL_AUDIO_IN1).values[i];
                  gfloat *d = ovalues;
                  while (d < bound)
                    *d++ += *s++;
                }
            }
          else if (jstream (JCHANNEL_AUDIO_IN1).n_connections < 1)
            ostream_set (OCHANNEL_AUDIO_OUT1, const_values (0));
          else /* 1 connection */
            ostream_set (OCHANNEL_AUDIO_OUT1, jstream (JCHANNEL_AUDIO_IN1).values[0]);
        }
      if (ostream (OCHANNEL_AUDIO_OUT2).connected)
        {
          if (jstream (JCHANNEL_AUDIO_IN2).n_connections > 1)
            {
              gfloat *ovalues = ostream (OCHANNEL_AUDIO_OUT2).values, *bound = ovalues + n_values;
              memcpy (ovalues, jstream (JCHANNEL_AUDIO_IN2).values[0], sizeof (ovalues[0]) * n_values);
              for (guint i = 1; i < jstream (JCHANNEL_AUDIO_IN2).n_connections; i++)
                {
                  const gfloat *s = jstream (JCHANNEL_AUDIO_IN2).values[i];
                  gfloat *d = ovalues;
                  while (d < bound)
                    *d++ += *s++;
                }
            }
          else if (jstream (JCHANNEL_AUDIO_IN2).n_connections < 1)
            ostream_set (OCHANNEL_AUDIO_OUT2, const_values (0));
          else /* 1 connection */
            ostream_set (OCHANNEL_AUDIO_OUT2, jstream (JCHANNEL_AUDIO_IN2).values[0]);
        }
    }
  };
public:
  /* implement creation and config methods for synthesis Module */
  BSE_EFFECT_INTEGRATE_MODULE (Summation, Summer, SummationProperties);
};

BSE_CXX_DEFINE_EXPORTS();
BSE_CXX_REGISTER_EFFECT (Summation);

} // Bse
