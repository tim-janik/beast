// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseportamento.genidl.hh"
#include <bse/bsemathsignal.hh>

using std::max;

namespace Bse {

class Portamento : public PortamentoBase {
  /* actual computation */
  class Module : public SynthesisModule {
    double current_frequency;
    double dest_frequency;
    double pfactor;
    int    pfactor_steps;
    double time_sec;
    bool   skip;
  public:
    void
    config (PortamentoProperties *properties)
    {
      time_sec = properties->time / 1000;
    }
    void
    reset()
    {
      skip = true;
      current_frequency = 0;
      pfactor_steps = 0;
    }
    Module()
    {
    }
    ~Module()
    {
    }
    void
    process (unsigned int n_values)
    {
      const gfloat *freq_in = istream (ICHANNEL_FREQ_IN).values;
      const gfloat *gate = istream (ICHANNEL_GATE_IN).values;
      gfloat *freq_out = ostream (OCHANNEL_FREQ_OUT).values;

      for (unsigned int i = 0; i < n_values; i++)
        {
          double input_frequency = BSE_SIGNAL_TO_FREQ (freq_in[i]);
          if (skip && gate[i] > 0.9)
            {
              current_frequency = input_frequency;
            }
          else
            {
              if (input_frequency != dest_frequency)
                {
                  // multiplication with pfactor mix_freq()*time_sec times should make
                  // current_frequency == input_frequency

                  pfactor_steps = max<int> (mix_freq() * time_sec, 1);
                  pfactor = exp (log (input_frequency / current_frequency) / pfactor_steps);

                  dest_frequency = input_frequency;
                }
              if (pfactor_steps > 0)
                {
                  pfactor_steps--;
                  current_frequency *= pfactor;
                }
            }
          freq_out[i] = BSE_SIGNAL_FROM_FREQ (current_frequency);
          skip = (gate[i] < 0.1);
        }
    }
  };
public:
  /* implement creation and config methods for synthesis Module */
  BSE_EFFECT_INTEGRATE_MODULE (Portamento, Module, PortamentoProperties);
};

BSE_CXX_DEFINE_EXPORTS();
BSE_CXX_REGISTER_EFFECT (Portamento);

} // Bse
