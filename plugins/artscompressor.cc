/* ArtsCompressor - aRts Compressor Effect
 * Copyright (C) 2001 Matthias Kretz <kretz@kde.org>
 * Copyright (C) 2003 Stefan Westerfeld <stefan@space.twc.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
#include "artscompressor.gen-idl.h"

#include <math.h>
#include <string.h>

#define LN2 0.69314718

namespace Arts {
using namespace std;
using namespace Bse;

class Compressor : public CompressorBase
{
  class Module : public SynthesisModule {
    /* state */
    gfloat volume;
    /* params */
    gfloat threshold, ratio, output, attackfactor, releasefactor;
  public:
    void reset()
    {
      volume = 0;
    }
    
    void config(CompressorProperties *params)
    {
      threshold = params->threshold;
      ratio = params->ratio;
      output = params->output;
      
      if( params->attack == 0 )
        attackfactor = 1;
      else
	attackfactor = LN2 / ( params->attack / 1000 * mix_freq() );
      
      if( params->release == 0 )
        releasefactor = 1;
      else
	releasefactor = LN2 / ( params->release / 1000 * mix_freq() );
    }
    
    void process(unsigned int samples)
    {
      const float *invalue = istream (ICHANNEL_AUDIO_IN).values;
      float *outvalue = ostream (OCHANNEL_AUDIO_OUT).values;
      
      for( unsigned int i = 0; i < samples; i++ ) {
	float delta = fabs( invalue[i] ) - volume;
	if( delta > 0.0 )
	  volume += attackfactor * delta;
	else
	  volume += releasefactor * delta;
        
	if( volume > threshold ) {
	  outvalue[i] = ( ( volume - threshold ) * ratio + threshold ) / volume * invalue[i] * output;
	} else {
	  outvalue[i] = invalue[i] * output;
	}
      }
    }
  };
public:
  BSE_EFFECT_INTEGRATE_MODULE (Compressor, Module, CompressorProperties);
};

BSE_CXX_DEFINE_EXPORTS();
BSE_CXX_REGISTER_EFFECT (Compressor);

class StereoCompressor : public StereoCompressorBase
{
  class Module : public SynthesisModule {
    /* state */
    gfloat volume;
    /* params */
    gfloat threshold, ratio, output, attackfactor, releasefactor;
  public:
    void reset()
    {
      volume = 0;
    }
    
    void config(StereoCompressorProperties *params)
    {
      threshold = params->threshold;
      ratio = params->ratio;
      output = params->output;
      
      if( params->attack == 0 )
        attackfactor = 1;
      else
	attackfactor = LN2 / ( params->attack / 1000 * mix_freq() );
      
      if( params->release == 0 )
        releasefactor = 1;
      else
	releasefactor = LN2 / ( params->release / 1000 * mix_freq() );
    }
    
    void process(unsigned int samples)
    {
      const float *inleft = istream (ICHANNEL_LEFT_AUDIO_IN).values;
      const float *inright = istream (ICHANNEL_RIGHT_AUDIO_IN).values;
      float *outleft = ostream (OCHANNEL_LEFT_AUDIO_OUT).values;
      float *outright = ostream (OCHANNEL_RIGHT_AUDIO_OUT).values;
      
      for( unsigned int i = 0; i < samples; i++ ) {
	float delta = max(fabs(inleft[i]), fabs(inright[i])) - volume;
	if( delta > 0.0 )
	  volume += attackfactor * delta;
	else
	  volume += releasefactor * delta;
        
	if( volume > threshold ) {
	  outleft[i] = ( ( volume - threshold ) * ratio + threshold ) / volume * inleft[i] * output;
	  outright[i] = ( ( volume - threshold ) * ratio + threshold ) / volume * inright[i] * output;
	} else {
	  outleft[i] = inleft[i] * output;
	  outright[i] = inright[i] * output;
	}
      }
    }
  };
public:
  BSE_EFFECT_INTEGRATE_MODULE (StereoCompressor, Module, StereoCompressorProperties);
};

BSE_CXX_REGISTER_EFFECT (StereoCompressor);

} // Arts

/* vim:set ts=8 sw=2 sts=2: */
