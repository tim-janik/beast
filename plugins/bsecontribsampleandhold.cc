/* BseContribSampleAndHold                              -*-mode: c++;-*-
 * Copyright (C) 2004 Artem Popov <tfwo@mail.ru>
 *
 * This plugin is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This plugin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bsecontribsampleandhold.genidl.hh"
#include <bse/bseengine.h>

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
