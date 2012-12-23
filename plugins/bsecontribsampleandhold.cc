/* BseContribSampleAndHold                              -*-mode: c++;-*-
 * Copyright (C) 2004 Artem Popov <tfwo@mail.ru>
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
