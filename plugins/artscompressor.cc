/* ArtsCompressor - aRts Compressor Effect
 * Copyright (C) 2001 Matthias Kretz <kretz@kde.org>
 * Copyright (C) 2003-2004 Stefan Westerfeld <stefan@space.twc.de>
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

namespace Bse {
namespace Arts {
using namespace std;

class Compressor : public CompressorBase
{
  class Module : public SynthesisModule {
    /* state */
    double volume;
    /* params */
    double threshold, threshold_db;   /* threshold as normal and as dB value */
    double ratio;		      /* 0.5 for 2:1 compression */
    double output;		      /* linear factor */
    double attackfactor, releasefactor;
  public:
    void reset()
    {
      volume = 0;
    }
    
    void config(CompressorProperties *params)
    {
      threshold_db = params->threshold_db;
      threshold = comp_db2linear (threshold_db);
      ratio = 1 / params->ratio_to_one;
      output = comp_db2linear (params->output_db);
      
      if( params->attack == 0 )
        attackfactor = 1;
      else
	attackfactor = LN2 / ( params->attack / 1000 * mix_freq() );
      
      if( params->release == 0 )
        releasefactor = 1;
      else
	releasefactor = LN2 / ( params->release / 1000 * mix_freq() );
    }

    /* conversion doesn't test for linear == 0,
     * as the input (a volume) is guaranteed to be above threshold
     */
    double comp_linear2db (double linear)
    {
      return 20 * log (linear) / log (10);
    }
    double comp_db2linear (double db)
    {
      return exp (db / 20 * log (10));
    }
    double compress (double input_signal)
    {
      double volume_db = comp_linear2db (volume);
      double output_signal = comp_db2linear ((volume_db - threshold_db) * ratio + threshold_db) / volume * input_signal * output;

      return output_signal;
    }

    static const int CHANNEL_A1 = 1;
    static const int CHANNEL_A2 = 2;
    static const int CHANNELS_A1n_A2n = 0;
    static const int CHANNELS_A1y_A2n = CHANNEL_A1;
    static const int CHANNELS_A1n_A2y = CHANNEL_A2;
    static const int CHANNELS_A1y_A2y = CHANNEL_A1 + CHANNEL_A2;

    template<int channels>
    void process_loop (unsigned int samples)
    {
      const float *invalue1 = istream (ICHANNEL_AUDIO_IN1).values;
      const float *invalue2 = istream (ICHANNEL_AUDIO_IN2).values;
      float *outvalue1 = ostream (OCHANNEL_AUDIO_OUT1).values;
      float *outvalue2 = ostream (OCHANNEL_AUDIO_OUT2).values;
      
      for( unsigned int i = 0; i < samples; i++ ) {
	double delta = 0.0;
	switch (channels)
	  {
	  case CHANNELS_A1n_A2n: delta = -volume;
				 break;
	  case CHANNELS_A1y_A2n: delta = fabs (invalue1[i]) - volume;
				 break;
	  case CHANNELS_A1n_A2y: delta = fabs (invalue2[i]) - volume;
				 break;
	  case CHANNELS_A1y_A2y: delta = max (fabs (invalue1[i]), fabs (invalue2[i])) - volume;
				 break;
	  }

	if( delta > 0.0 )
	  volume += attackfactor * delta;
	else
	  volume += releasefactor * delta;
        
	if (volume > threshold)
	  {
	    if (channels & CHANNEL_A1)
	      outvalue1[i] = compress (invalue1[i]);
	    if (channels & CHANNEL_A2)
	      outvalue2[i] = compress (invalue2[i]);
	  }
	else
	  {
	    if (channels & CHANNEL_A1)
	      outvalue1[i] = invalue1[i] * output;
	    if (channels & CHANNEL_A2)
	      outvalue2[i] = invalue2[i] * output;
	  }
      }
    }
    void process (unsigned int n_values)
    {
      if (istream (ICHANNEL_AUDIO_IN1).connected)
	{
	  if (istream (ICHANNEL_AUDIO_IN2).connected)
	    {
	      process_loop <CHANNELS_A1y_A2y> (n_values);
	    }
	  else
	    {
	      process_loop <CHANNELS_A1y_A2n> (n_values);
	      ostream_set (OCHANNEL_AUDIO_OUT2, const_values (0));
	    }
	}
      else
	{
	  if (istream (ICHANNEL_AUDIO_IN2).connected)
	    {
	      process_loop <CHANNELS_A1n_A2y> (n_values);
	      ostream_set (OCHANNEL_AUDIO_OUT1, const_values (0));
	    }
	  else
	    {
	      process_loop <CHANNELS_A1n_A2n> (n_values);
	      ostream_set (OCHANNEL_AUDIO_OUT1, const_values (0));
	      ostream_set (OCHANNEL_AUDIO_OUT2, const_values (0));
	    }
	}
    }
  };
public:
  void property_changed (CompressorPropertyID prop_id)
  {
    switch (prop_id)
      {
      /* implement special handling of GUI properties */
      case PROP_AUTO_OUTPUT:
      case PROP_RATIO_TO_ONE:
      case PROP_THRESHOLD_DB:
	if (auto_output)
	  output_db = threshold_db / ratio_to_one - threshold_db;
        notify ("output_db");
        break;
      /* compat properties */
      case PROP_RATIO:
        if (ratio > 0)
	  set ("ratio_to_one", 1 / ratio, NULL);
	else
	  set ("ratio_to_one", 10000, NULL); /* max ratio */
	break;
      case PROP_THRESHOLD:
        set ("threshold_db", bse_db_from_factor (threshold, -100), NULL);
        break;
      case PROP_OUTPUT:
        set ("output_db", bse_db_from_factor (output, -100), NULL);
        break;
      default: ;
      }
  }
  BSE_EFFECT_INTEGRATE_MODULE (Compressor, Module, CompressorProperties);
};

BSE_CXX_DEFINE_EXPORTS();
BSE_CXX_REGISTER_EFFECT (Compressor);

// printf ("input: %f dB (%f), output: %f dB (%f)\n", comp_linear2db (volume), volume, comp_linear2db (compress (volume)), compress (volume));

} // Arts
} // Bse

/* vim:set ts=8 sw=2 sts=2: */
