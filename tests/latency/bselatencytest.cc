/* BseLatencyTest - BSE Latency Test
 * Copyright (C) 2004 Stefan Westerfeld
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
#include "bselatencytest.gen-idl.h"
#include <sys/time.h>
#include <vector>

using namespace std;
using namespace Sfi;

namespace Bse {

class LatencyTest : public LatencyTestBase {
  /* properties */
  struct Properties : public LatencyTestProperties {
    Properties (LatencyTest *noise) : LatencyTestProperties (noise)
    {
    }
  };
  /* actual computation */
  class Module : public SynthesisModule {
  public:
    FILE *midi_output_file;
    FILE *logfile;

    char note_on[3];
    char note_off[3];

    guint note_on_wait;
    guint note_off_wait;
    guint bpm_samples;
    gfloat threshold;

    double start_time;
    double end_time;

    double
    gettime ()
    {
      timeval tv;
      gettimeofday (&tv, 0);

      return double(tv.tv_sec) + double(tv.tv_usec) * (1.0 / 1000000.0);
    }

    Module()
    {
      midi_output_file = NULL;
      logfile = NULL;
    }
    void
    config (Properties *properties)
    {
      reset();

      midi_output_file = fopen (properties->midi_output.c_str(), "w");
      if (!midi_output_file)
	fprintf (stderr, "can't open midi output: %s\n", properties->midi_output.c_str());

      if (properties->logfile_name != "")
	{
	  logfile = fopen (properties->logfile_name.c_str(), "w");
	  if (!logfile)
	    fprintf (stderr, "can't open log file: %s\n");
	}

      note_on[0] = 0x90 | (properties->midi_channel - 1);
      note_on[1] = properties->midi_note;
      note_on[2] = 100; /* velocity */

      note_off[0] = 0x80 | (properties->midi_channel - 1);
      note_off[1] = properties->midi_note;
      note_off[2] = 0; /* velocity */

      bpm_samples = mix_freq() * 60 / properties->bpm; /* quarter note duration */
      note_on_wait = bpm_samples / 2;
      note_off_wait = bpm_samples;
      threshold = bse_db_to_factor (properties->threshold_db);
      start_time = 0.0;
    }
    void
    reset()
    {
      if (midi_output_file)
	{
	  fwrite (note_off, 3, 1, midi_output_file); /* don't leak note_ons */
	  fflush (midi_output_file);
	  fclose (midi_output_file);
	  midi_output_file = NULL;
	}
      if (logfile)
	{
	  fclose (logfile);
	  logfile = NULL;
	}
    }
    void
    process (unsigned int n_values)
    {
      if (!midi_output_file)
	return;

      const gfloat *auin = istream (ICHANNEL_AUDIO_IN).values;
      for (int i = 0; i < n_values; i++)
	{
	  if (!note_on_wait)
	    {
	      start_time = gettime();

	      fwrite (note_on, 3, 1, midi_output_file);
	      fflush (midi_output_file);
	      note_on_wait += bpm_samples;
	    }
	  else
	    note_on_wait--;

	  if (!note_off_wait)
	    {
	      fwrite (note_off, 3, 1, midi_output_file);
	      fflush (midi_output_file);
	      note_off_wait += bpm_samples;
	    }
	  else
	    note_off_wait--;

	  if (auin[i] > threshold && start_time > 0)
	    {
	      double delta_time = double (i) / mix_freq();
	      double end_time = gettime();

	      fprintf (logfile ? logfile : stdout, "%f # start=%f end=%f in_block_delta=%f\n", end_time + delta_time - start_time,
		                                                                               start_time, end_time, delta_time);
	      start_time = 0.0;
	    }
	}
    }
  };
public:
  /* implement creation and config methods for synthesis Module */
  BSE_EFFECT_INTEGRATE_MODULE (LatencyTest, Module, Properties);
};

BSE_CXX_DEFINE_EXPORTS();
BSE_CXX_REGISTER_EFFECT (LatencyTest);

} // Bse
