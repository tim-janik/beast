/* StandardGusPatchEnvelope - Standard GUS Patch Envelope
 * Copyright (C) 2004-2005 Stefan Westerfeld
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <bse/bsecxxplugin.h>
#include <bse/bsewave.h>
#include <bse/gslwavechunk.h>
#include "standardguspatchenvelope.gen-idl.h"

using namespace std;
using namespace Sfi;

namespace Bse {
namespace Standard {

class GusPatchEnvelope : public GusPatchEnvelopeBase {
  /* properties (used to pass "global" envelope data into the modules) */
  struct Properties : public GusPatchEnvelopeProperties {
    BseWaveIndex *wave_index;

    Properties (GusPatchEnvelope *envelope)
      : GusPatchEnvelopeProperties (envelope),
        wave_index (envelope->wave_index)
    {
    }
  };
  /* actual computation */
  class Module : public SynthesisModule {
  private:
    gfloat envelope_value;
    BseWaveIndex *wave_index;
    GslWaveChunk *wave_chunk;
    gboolean retrigger;

  public:
    void
    config (Properties *properties)
    {
      wave_index = properties->wave_index;
    }
    void
    reset()
    {
      envelope_value = 0;
      retrigger = true;
      wave_chunk = 0;
    }
    void
    update_envelope (gfloat frequency)
    {
      const gchar *envelope = NULL;

      wave_chunk = bse_wave_index_lookup_best (wave_index, frequency);
      if (wave_chunk)
	envelope = bse_xinfos_get_value (wave_chunk->dcache->dhandle->setup.xinfos, "gus-patch-envelope");

      if (envelope)
	fprintf (stderr, "gus envelope from wave_chunk: %s\n", envelope);
    }
    void
    process (unsigned int n_values)
    {
      if (retrigger)
	{
	  const gfloat *frequency = istream (ICHANNEL_FREQUENCY).values;
	  update_envelope (frequency[0]);
	  retrigger = false;
	}

      /* optimize me: we need 4 cases */
      if (ostream (OCHANNEL_AUDIO_OUT1).connected || ostream (OCHANNEL_AUDIO_OUT2).connected)
        {
          if (istream (ICHANNEL_AUDIO_IN).connected)
            {
	      const gfloat *gate = istream (ICHANNEL_GATE_IN).values;
              const gfloat *in = istream (ICHANNEL_AUDIO_IN).values;
	      gfloat *out1 = ostream (OCHANNEL_AUDIO_OUT1).values, *bound = out1 + n_values;
	      gfloat *out2 = ostream (OCHANNEL_AUDIO_OUT2).values;
	      gfloat *done = ostream (OCHANNEL_DONE_OUT).values;

	      const gfloat envelope_incr = 0.01;
	      const gfloat epsilon       = envelope_incr / 2;

	      while (out1 < bound)
		{
		  bool gate_active = (*gate++ > 0.5);

		  if (gate_active)
		    envelope_value = min<gfloat> (envelope_value + envelope_incr, 1.0);
		  else
		    envelope_value = max<gfloat> (envelope_value - envelope_incr, 0.0);

		  gdouble output = *in++ * envelope_value;

		  // FIXME: panning
		  *out1++ = output;
		  *out2++ = output;

		  *done++ = (!gate_active && envelope_value < epsilon) ? 1.0 : 0.0;
		}
            }
	  else
	    {
	      ostream_set (OCHANNEL_AUDIO_OUT1, const_values (0));
	      ostream_set (OCHANNEL_AUDIO_OUT2, const_values (0));
	    }
        }
    }
  };

public:
  BseWaveIndex *wave_index;

  GusPatchEnvelope() : wave_index (NULL)
  {
  }

  bool
  property_changed (GusPatchEnvelopePropertyID prop_id)
  {
    switch (prop_id)
      {
      case PROP_WAVE:
	wave_index = wave ? bse_wave_get_index_for_modules (wave) : NULL;
      default: ;
      }
    return false;
  }

  /* implement creation and config methods for synthesis Module */
  BSE_EFFECT_INTEGRATE_MODULE (GusPatchEnvelope, Module, Properties);
};

BSE_CXX_DEFINE_EXPORTS();
BSE_CXX_REGISTER_EFFECT (GusPatchEnvelope);

} // Standard
} // Bse
