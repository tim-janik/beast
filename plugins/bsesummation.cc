// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
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
      if (ostream (OCHANNEL_AUDIO_OUT1).connected || ostream (OCHANNEL_AUDIO_DIFF).connected)
        {
          if (jstream (JCHANNEL_AUDIO_IN1).n_connections > 1)
            {
              float *ovalues = ostream (OCHANNEL_AUDIO_OUT1).values, *bound = ovalues + n_values;
              memcpy (ovalues, jstream (JCHANNEL_AUDIO_IN1).values[0], sizeof (ovalues[0]) * n_values);
              for (uint i = 1; i < jstream (JCHANNEL_AUDIO_IN1).n_connections; i++)
                {
                  const float *s = jstream (JCHANNEL_AUDIO_IN1).values[i];
                  float *d = ovalues;
                  while (d < bound)
                    *d++ += *s++;
                }
            }
          else if (jstream (JCHANNEL_AUDIO_IN1).n_connections < 1)
            ostream_set (OCHANNEL_AUDIO_OUT1, const_values (0));
          else /* 1 connection */
            ostream_set (OCHANNEL_AUDIO_OUT1, jstream (JCHANNEL_AUDIO_IN1).values[0]);
        }
      if (ostream (OCHANNEL_AUDIO_OUT2).connected || ostream (OCHANNEL_AUDIO_DIFF).connected)
        {
          if (jstream (JCHANNEL_AUDIO_IN2).n_connections > 1)
            {
              float *ovalues = ostream (OCHANNEL_AUDIO_OUT2).values, *bound = ovalues + n_values;
              memcpy (ovalues, jstream (JCHANNEL_AUDIO_IN2).values[0], sizeof (ovalues[0]) * n_values);
              for (uint i = 1; i < jstream (JCHANNEL_AUDIO_IN2).n_connections; i++)
                {
                  const float *s = jstream (JCHANNEL_AUDIO_IN2).values[i];
                  float *d = ovalues;
                  while (d < bound)
                    *d++ += *s++;
                }
            }
          else if (jstream (JCHANNEL_AUDIO_IN2).n_connections < 1)
            ostream_set (OCHANNEL_AUDIO_OUT2, const_values (0));
          else /* 1 connection */
            ostream_set (OCHANNEL_AUDIO_OUT2, jstream (JCHANNEL_AUDIO_IN2).values[0]);
        }
      if (ostream (OCHANNEL_AUDIO_DIFF).connected)
        {
          const float *o1 = ostream (OCHANNEL_AUDIO_OUT1).values;
          const float *o2 = ostream (OCHANNEL_AUDIO_OUT2).values;
          float *df = ostream (OCHANNEL_AUDIO_DIFF).values;
          for (uint i = 0; i < n_values; i++)
            df[i] = o1[i] - o2[i];
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
