// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsecontribsampleandhold.genidl.hh"
#include <bse/bseengine.hh>

namespace Bse { namespace Contrib {

class SampleAndHold: public SampleAndHoldBase {
  class Module: public SynthesisModule {
    gfloat value;
    gboolean negative;

  public:
    void
    config (SampleAndHoldProperties *params)
    {
    }

    void
    reset ()
    {
      value = 0;
      negative = true;
    }

    void
    process (unsigned int n_values)
    {
      const float *input = istream (ICHANNEL_AUDIO_IN).values;
      const float *trigger = istream (ICHANNEL_TRIGGER_IN).values;
      float *output = ostream (OCHANNEL_AUDIO_OUT).values;
      float *bound = output + n_values;

      while (output < bound)
        {
	  float t = *trigger++;
	  float i = *input++;
	  if (t <= 0 && (!negative))
	    {
	      negative = true;
	    }
          if (t > 0 && negative)
	    {
	      negative = false;
	      value = i;
	    }
          *output++ = value;
        }
    }
  };

public:
  BSE_EFFECT_INTEGRATE_MODULE (SampleAndHold, Module, SampleAndHoldProperties);
};

BSE_CXX_DEFINE_EXPORTS ();
BSE_CXX_REGISTER_EFFECT (SampleAndHold);

} } // Bse::Contrib
