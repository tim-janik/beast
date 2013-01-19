// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "artscompressor.genidl.hh"
#include <bse/bsemath.hh>
#include <string.h>
namespace Bse {
namespace Arts {
using namespace std;
/*
 * constants
 */
#define LN2                        (BSE_LN2)    /* ln(2) */
static const double MUG_CORR_FACT = 0.4;	/* makeup gain correction factor (from jamin-0.9.0 source)
						 * dampens the makeup gain correction to stop it over correcting */
class Compressor : public CompressorBase
{
  /*
   * engine module
   */
  class Module : public SynthesisModule {
    /* state */
    double volume;
    /* params */
    double threshold, threshold_db;   /* threshold as normal and as dB value */
    double ratio;		      /* 0.5 for 2:1 compression */
    double output;		      /* linear factor */
    double attackfactor, releasefactor;
  public:
    void
    reset()
    {
      volume = 0;
    }
    void
    config (CompressorProperties *params)
    {
      threshold_db = params->threshold_db;
      threshold = comp_db2linear (threshold_db);
      ratio = 1 / params->ratio_to_one;
      output = comp_db2linear (params->output_db);
      /* compute half-life times: using max ensures that computing the attack- and releasefactor will
       *  (a) not result in division by zero
       *  (b) result in a value <= 1.0, where 1.0 means: adapt volume immediately, without half-life time
       */
      attackfactor = LN2 / max (params->attack / 1000 * mix_freq(), LN2);
      releasefactor = LN2 / max (params->release / 1000 * mix_freq(), LN2);
    }
    /* conversion doesn't test for linear == 0,
     * as the input (a volume) is guaranteed to be above threshold
     */
    double
    comp_linear2db (double linear)
    {
      // return 20 * log (linear) / log (10);
      return log (linear) * BSE_DECIBEL20_FACTOR;
    }
    double
    comp_db2linear (double db)
    {
      // return exp (db / 20 * log (10));
      return exp (db * BSE_1_DIV_DECIBEL20_FACTOR);
    }
    double
    compress (double input_signal)
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
    template<int channels> void
    process_loop (unsigned int samples)
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
    void
    process (unsigned int n_values)
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
  bool
  property_changed (CompressorPropertyID prop_id)
  {
    switch (prop_id)
      {
      /* implement special handling of GUI properties */
      case PROP_AUTO_OUTPUT:
      case PROP_RATIO_TO_ONE:
      case PROP_THRESHOLD_DB:
      case PROP_OUTPUT_DB:
	if (auto_output)
	  {
	    /* keep CLAMP range in sync with .idl file */
	    output_db = CLAMP ((threshold_db / ratio_to_one - threshold_db) * MUG_CORR_FACT, -20.0, 20.0);
	  }
        notify ("output_db");
        break;
      /* compat properties */
      case PROP_RATIO:
        if (ratio > 0)
	  set ("ratio_to_one", 1 / ratio, NULL);
	else
	  set ("ratio_to_one", 20.0, NULL); /* max ratio */
	break;
      case PROP_THRESHOLD:
	/* keep CLAMP range in sync with .idl file */
        set ("threshold_db", CLAMP (bse_db_from_factor (threshold, -100), -100.0, 0.0), NULL);
        break;
      case PROP_OUTPUT:
	/* keep CLAMP range in sync with .idl file */
        set ("output_db", CLAMP (bse_db_from_factor (output, -100), -20.0, 20.0), NULL);
        break;
      default: ;
      }
    return false;
  }
  bool
  editable_property (CompressorPropertyID prop_id,
                     GParamSpec          *)
  {
    if (prop_id == PROP_OUTPUT_DB && auto_output)
      return false;
    return true;
  }
  BSE_EFFECT_INTEGRATE_MODULE (Compressor, Module, CompressorProperties);
};
BSE_CXX_DEFINE_EXPORTS();
BSE_CXX_REGISTER_EFFECT (Compressor);
// printf ("input: %f dB (%f), output: %f dB (%f)\n", comp_linear2db (volume), volume, comp_linear2db (compress (volume)), compress (volume));
} // Arts
} // Bse
/* vim:set ts=8 sw=2 sts=2: */
