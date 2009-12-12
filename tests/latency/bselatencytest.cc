/* BseLatencyTest - BSE Latency Test
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
#include "bselatencytest.genidl.hh"
#include <sys/time.h>
#include <string>
#include <errno.h>
#include <stdio.h>

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
    void
    close_devices()
    {
      if (midi_output_file)
	{
	  fwrite (note_off, 3, 1, midi_output_file); /* don't leak note_ons */
	  fflush (midi_output_file);
	  fclose (midi_output_file);
          midi_output_file = NULL;
	}
      if (logfile && logfile != stdout)
        fclose (logfile);
      logfile = NULL;
    }
    Module()
    {
      midi_output_file = NULL;
      logfile = NULL;
    }
    ~Module()
    {
      close_devices();
    }
    string midi_output_name;
    string logfile_name;
    void
    reset()
    {
      close_devices();

      if (birnet_file_check (midi_output_name.c_str(), "pw")) /* writable pipe */
        midi_output_file = fopen (midi_output_name.c_str(), "w");
      if (!midi_output_file)
        sfi_error ("failed to open midi output \"%s\": %s\n", midi_output_name.c_str(), g_strerror (errno));
      if (logfile_name == "" || !midi_output_file)
        logfile = stdout;
      else
	{
	  logfile = fopen (logfile_name.c_str(), "w");
	  if (!logfile)
            sfi_error ("failed to open log file \"%s\": %s\n", logfile_name.c_str(), g_strerror (errno));
	}
    }
    void
    config (Properties *properties)
    {
      logfile_name = properties->logfile_name.c_str();
      midi_output_name = properties->midi_output.c_str();
      if (midi_output_name[0] == '$')
        midi_output_name = g_getenv (midi_output_name.c_str() + 1);

      /* send pending note-off events, close and reopen devices */
      reset();

      /* configure events */
      note_on[0] = 0x90 | (properties->midi_channel - 1);
      note_on[1] = properties->midi_note;
      note_on[2] = 100; /* velocity */

      note_off[0] = 0x80 | (properties->midi_channel - 1);
      note_off[1] = properties->midi_note;
      note_off[2] = 0; /* velocity */

      /* setup timing */
      bpm_samples = guint (mix_freq() * 60 / properties->bpm); /* quarter note duration */
      note_on_wait = bpm_samples / 2;
      note_off_wait = bpm_samples;
      threshold = bse_db_to_factor (properties->threshold_db);
      start_time = 0.0;
    }
    void
    process (unsigned int n_values)
    {
      const gfloat *auin = istream (ICHANNEL_AUDIO_IN).values;
      for (guint i = 0; i < n_values; i++)
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

	      if (logfile)
                fprintf (stdout, "%12.6f # latency in ms between note-on and signal-start; (block-offset=%u)\n",
                         1000.0 * (end_time + delta_time - start_time), i);
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
