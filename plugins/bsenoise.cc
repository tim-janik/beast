// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsenoise.genidl.hh"
#include <bse/bsemain.hh>
#include <vector>

using namespace std;
using namespace Sfi;

namespace Bse {

class Noise : public NoiseBase {
  /* mini random number generator (adapted from rapicorn), to generate
   * deterministic random numbers when --bse-disable-randomization was used
   */
  class DetRandomGenerator
  {
    uint32  seed;
  public:
    DetRandomGenerator()
    {
      reset();
    }
    void
    reset()
    {
      seed = 2147483563;
    }
    /* range: [-1.0,1.0) */
    double
    rand_sample()
    {
      const double scale = 4.656612873077392578125e-10; /* 1 / 2^31 */
      seed = 1664525 * seed + 1013904223;
      return int32 (seed) * scale;    /* the cast adds the sign bit */
    }
  };
  /* properties (used to pass "global" noise data into the modules) */
  struct Properties : public NoiseProperties {
    const vector<float> *noise_data;

    Properties (Noise *noise) : NoiseProperties (noise), noise_data (noise->noise_data)
    {
    }
  };
  /* actual computation */
  class Module : public SynthesisModule {
  public:
    const vector<float> *noise_data;
    DetRandomGenerator   det_random_generator;
    bool                 allow_randomization;

    void
    config (Properties *properties)
    {
      noise_data = properties->noise_data;
      allow_randomization = bse_main_args->allow_randomization;
    }
    void
    reset()
    {
      det_random_generator.reset();
    }
    void
    process (unsigned int n_values)
    {
      g_return_if_fail (n_values <= block_size()); /* paranoid */

      if (allow_randomization) /* fast */
      {
	ostream_set (OCHANNEL_NOISE_OUT, &(*noise_data)[rand() % (noise_data->size() - n_values)]);
      }
      else /* slow, but deterministic */
      {
	float *outvalue = ostream (OCHANNEL_NOISE_OUT).values;
	for (unsigned int i = 0; i < n_values; i++)
	  outvalue[i] = det_random_generator.rand_sample();
      }
    }
  };
public:
  /* preparation of a long block of random data */
  static vector<float> *noise_data;
  static uint		noise_data_ref_count;

  void
  prepare1()
  {
    if (!noise_data_ref_count)
      { 
	const int N_NOISE_BLOCKS = 20;
	noise_data = new vector<float> (max_block_size() * N_NOISE_BLOCKS);

	for (vector<float>::iterator ni = noise_data->begin(); ni != noise_data->end(); ni++)
	  *ni = 1.0 - rand() / (0.5 * RAND_MAX);
      }
    noise_data_ref_count++;
  }
  void
  reset1()
  {
    g_return_if_fail (noise_data_ref_count > 0);

    noise_data_ref_count--;
    if (noise_data_ref_count == 0)
      {
	delete noise_data;
	noise_data = 0;
      }
  }

  /* implement creation and config methods for synthesis Module */
  BSE_EFFECT_INTEGRATE_MODULE (Noise, Module, Properties);
};

vector<float> *Noise::noise_data = 0;
uint           Noise::noise_data_ref_count = 0;


BSE_CXX_DEFINE_EXPORTS();
BSE_CXX_REGISTER_EFFECT (Noise);

} // Bse
