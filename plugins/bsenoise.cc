/* BseNoise - BSE Noise generator
 * Copyright (C) 1999,2000-2001 Tim Janik
 * Copyright (C) 2004 Stefan Westerfeld
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
#include "bsenoise.genidl.hh"
#include <vector>

using namespace std;
using namespace Sfi;

namespace Bse {

class Noise : public NoiseBase {
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

    void
    config (Properties *properties)
    {
      noise_data = properties->noise_data;
    }
    void
    reset()
    {
    }
    void
    process (unsigned int n_values)
    {
      g_return_if_fail (n_values <= block_size()); /* paranoid */

      ostream_set (OCHANNEL_NOISE_OUT, &(*noise_data)[rand() % (noise_data->size() - n_values)]);
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
      delete noise_data;
  }

  /* implement creation and config methods for synthesis Module */
  BSE_EFFECT_INTEGRATE_MODULE (Noise, Module, Properties);
};

vector<float> *Noise::noise_data;
uint           Noise::noise_data_ref_count = 0;


BSE_CXX_DEFINE_EXPORTS();
BSE_CXX_REGISTER_EFFECT (Noise);

} // Bse
