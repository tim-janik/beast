/* BSE Feature Extraction Tool
 * Copyright (C) 2004 Stefan Westerfeld
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#include <bse/bseengine.h>
#include <bse/gslsignal.h>

#include <bse/gsldatautils.h>
#include <bse/gslloader.h>
#include <bse/gslfft.h>
#include <bse/gslsignal.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include "topconfig.h"

#include <map>
#include <string>
#include <vector>
#include <utility>

using namespace std;

struct Options {
  string	      programName;
  FILE		     *extractStartTime;     /* NULL if start time feature should not be extracted */
  FILE		     *extractEndTime;	    /* NULL if end time feature should not be extracted */
  FILE		     *extractSpectrum;	    /* NULL if spectrum should not be extracted */
  FILE		     *extractAvgSpectrum;   /* NULL if average spectrum should not be extracted */
  FILE               *extractAvgEnergy;     /* NULL if average energy should not be extracted */
  guint               channel;

  map<string, FILE*>  outputFiles;

  Options ();
  void parse (int *argc_p, char **argv_p[]);
  void printUsage ();

  FILE *openOutputFile (const char *filename);
} options;

Options::Options ()
{
  programName = "bsefextract";
  extractStartTime = extractEndTime = NULL;
  channel = 0;
}

FILE *Options::openOutputFile (const char *filename)
{
  if (!filename || (strcmp (filename, "-") == 0))
    return stdout;
  
  FILE*& outfile = outputFiles[filename];
  if (!outfile)
    {
      outfile = fopen (filename, "w");
      if (!outfile)
	{
	  fprintf (stderr, "%s: can't open %s for writing: %s\n", programName.c_str(), filename, strerror (errno));
	  exit (1);
	}
    }
  return outfile;
}

void Options::parse (int *argc_p, char **argv_p[])
{
  unsigned int argc;
  char **argv;
  unsigned int i, e;

  g_return_if_fail (argc_p != NULL);
  g_return_if_fail (argv_p != NULL);
  g_return_if_fail (*argc_p >= 0);

  argc = *argc_p;
  argv = *argv_p;

  /*  I am tired of seeing .libs/lt-bsefextract all the time,
   *  but basically this should be done (to allow renaming the binary):
   *
  if (argc && argv[0])
    programName = argv[0];
  */

  for (i = 1; i < argc; i++)
    {
      const char *opt = strtok (argv[i], "=");
      const char *arg = opt ? strtok (NULL, "\n") : NULL;

      if (strcmp ("--help", argv[i]) == 0)
	{
	  printUsage();
	  exit (0);
	}
      else if (strcmp ("--version", argv[i]) == 0)
	{
	  printf ("%s %s\n", programName.c_str(), BST_VERSION);
	  exit (0);
	}
      else if (strcmp ("--start-time", opt) == 0)
	{
	  extractStartTime = openOutputFile (arg);
	  argv[i] = NULL;
	}
      else if (strcmp ("--end-time", argv[i]) == 0)
	{
	  extractEndTime = openOutputFile (arg);
	  argv[i] = NULL;
	}
      else if (strcmp ("--spectrum", argv[i]) == 0)
	{
	  extractSpectrum = openOutputFile (arg);
	  argv[i] = NULL;
	}
      else if (strcmp ("--avg-spectrum", argv[i]) == 0)
	{
	  extractAvgSpectrum = openOutputFile (arg);
	  argv[i] = NULL;
	}
      else if (strcmp ("--avg-energy", argv[i]) == 0)
	{
	  extractAvgEnergy = openOutputFile (arg);
	  argv[i] = NULL;
	}
      else if (strcmp ("--channel", argv[i]) == 0)
	{
	  if (!arg)
	    {
	      printUsage();
	      exit (1);
	    }
	  channel = atoi (arg);
	  argv[i] = NULL;
	}
    }

  /* resort argc/argv */
  e = 0;
  for (i = 1; i < argc; i++)
    {
      if (e)
	{
	  if (argv[i])
	    {
	      argv[e++] = argv[i];
	      argv[i] = NULL;
	    }
	}
      else if (!argv[i])
	e = i;
    }
  if (e)
    *argc_p = e;
}

void Options::printUsage ()
{
  fprintf (stderr, "usage: %s [ <options> ] <audiofile>\n", programName.c_str());
  fprintf (stderr, "\n");
  fprintf (stderr, "features that can be extracted:\n");
  fprintf (stderr, " --start-time                signal start time in ms (first non-zero sample)\n");
  fprintf (stderr, " --end-time                  signal end time in ms (last non-zero sample)\n");
  fprintf (stderr, " --spectrum                  frequency spectrum\n");
  fprintf (stderr, " --avg-spectrum              average frequency spectrum\n");
  fprintf (stderr, " --avg-energy                average signal energy in dB\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "other options:\n");
  fprintf (stderr, " --channel=<channel>         select channel (0: left, 1: right)\n");
  fprintf (stderr, " --help                      help for %s\n", programName.c_str());
  fprintf (stderr, " --version                   print version\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "If you want to write an extracted feature to a seperate files, you can\n");
  fprintf (stderr, "append =<filename> to a feature (example: %s --start-time=t.start t.wav).\n", programName.c_str());
}

void printHeader (FILE *file, const char *src)
{
  static map<FILE *, bool> done;

  if (!done[file])
    {
      fprintf (file, "# this output was generated by %s %s from channel %d in file %s\n", options.programName.c_str(), BST_VERSION, options.channel, src);
      fprintf (file, "#\n");
      done[file] = true;
    }
}

static vector<double>
build_frequency_vector (GslLong size,
                        double *samples)
{
  vector<double> fvector;
  double in[size], c[size + 2], *im;
  gint i;

  for (i = 0; i < size; i++)
    in[i] = gsl_window_blackman (2.0 * i / size - 1.0) * samples[i]; /* gsl blackman window is defined in range [-1, 1] */

  gsl_power2_fftar (size, in, c);
  c[size] = c[1];
  c[size + 1] = 0;
  c[1] = 0;
  im = c + 1;

  for (i = 0; i <= size >> 1; i++)
    {
      double abs = sqrt (c[i << 1] * c[i << 1] + im[i << 1] * im[i << 1]);
      /* FIXME: is this the correct normalization? */
      fvector.push_back (abs / size);
    }
  return fvector;
}

static vector<double>
collapse_frequency_vector (const vector<double>& fvector,
			   double mix_freq,
			   double first_freq,
			   double factor)
{
  vector<double> result;
  double value = 0;
  int count = 0;

  for (size_t j = 0; j < fvector.size(); j++)
    {
      double freq = (j * mix_freq) / (fvector.size() - 1) / 2;
      while (freq > first_freq)
	{
	  if (count)
	    result.push_back (value);
	  count = 0;
	  value = 0;
	  first_freq *= factor;
	}

      value += fvector[j];
      count++;
    }

  if (count)
    result.push_back (value);

  return result;
}

void fput_vector (FILE *file, const vector<double>& data)
{
  for (vector<double>::const_iterator di = data.begin(); di != data.end(); di++)
    fprintf (file, (di == data.begin() ? "%f" : " %f"), *di);
  fprintf (file, "\n");
}

int main (int argc, char **argv)
{
  /* init */
  GslConfigValue gslconfig[] = {
    { "wave_chunk_padding",     1, },
    { "dcache_block_size",      8192, },
    { "dcache_cache_memory",	5 * 1024 * 1024, },
    { NULL, },
  };
  
  g_thread_init (NULL);
  g_type_init ();
  gsl_init (gslconfig);
  /*bse_init_intern (&argc, &argv, NULL);*/

  /* parse options */
  options.parse (&argc, &argv);

  if (argc != 2)
    {
      options.printUsage ();
      return 1;
    }

  /* open input */
  GslErrorType error;

  GslWaveFileInfo *waveFileInfo = gsl_wave_file_info_load (argv[1], &error);
  if (!waveFileInfo)
    {
      fprintf (stderr, "%s: can't open the input file %s: %s\n", options.programName.c_str(), argv[1], gsl_strerror (error));
      exit (1);
    }

  GslWaveDsc *waveDsc = gsl_wave_dsc_load (waveFileInfo, 0, &error);
  if (!waveDsc)
    {
      fprintf (stderr, "%s: can't open the input file %s: %s\n", options.programName.c_str(), argv[1], gsl_strerror (error));
      exit (1);
    }

  GslDataHandle *dhandle = gsl_wave_handle_create (waveDsc, 0, &error);
  if (!dhandle)
    {
      fprintf (stderr, "%s: can't open the input file %s: %s\n", options.programName.c_str(), argv[1], gsl_strerror (error));
      exit (1);
    }

  error = gsl_data_handle_open (dhandle);
  if (error)
    {
      fprintf (stderr, "%s: can't open the input file %s: %s\n", options.programName.c_str(), argv[1], gsl_strerror (error));
      exit (1);
    }

  /* extract features */
  double startTime = -1;
  double endTime = -1;

  guint	  n_channels = gsl_data_handle_n_channels (dhandle);
  GslLong length = gsl_data_handle_length (dhandle);
  GslLong offset = 0;

  if (options.channel >= n_channels)
    {
      fprintf (stderr, "%s: bad channel %d, input file %s has %d channels\n",
	       options.programName.c_str(), options.channel, argv[1], n_channels);
      exit (1);
    }
  vector<double> sample_data (n_channels);
  gfloat values[512];
  while (offset < length)
    {
      GslLong n_values = gsl_data_handle_read (dhandle, offset, sizeof (values) / sizeof (gfloat), values);

      if (n_values > 0)
	{
	  if (options.extractStartTime || options.extractEndTime)
	    {
	      for (GslLong i = 0; i < n_values; i++)
		{
		  if (values[i] != 0)
		    {
		      GslLong n_frames = (offset + i) / n_channels;
		      gfloat time = n_frames * 1000.0 / dhandle->setup.mix_freq;
		      if (startTime < 0)
			startTime = time;
		      endTime = time;
		    }
		}
	    }
	  offset += n_values;
	}
      else
	{
	  fprintf (stderr, "%s: read error on input file %s\n", options.programName.c_str(), argv[1]);
	  exit (1);
	}
    }

  vector< vector<double> > spectrum;
  vector<double> avg_spectrum;

  if (options.extractSpectrum || options.extractAvgSpectrum)
    {
      double file_size_ms = 1000.0 * length / n_channels / dhandle->setup.mix_freq;
      GslDataPeekBuffer peek_buffer = { +1 /* incremental direction */, 0, };

      for (double offset_ms = 0; offset_ms < file_size_ms; offset_ms += 30) /* extract a feature vector every 30 ms */
	{
	  GslLong extract_frame = GslLong (offset_ms / file_size_ms * length / n_channels);

	  double samples[4096];
	  bool skip = false;
	  GslLong k = extract_frame * n_channels + options.channel;

	  for (int j = 0; j < 4096; j++)
	    {
	      if (k < length)
		samples[j] = gsl_data_handle_peek_value (dhandle, k, &peek_buffer);
	      else
		skip = true; /* alternative implementation: fill up with zeros;
				however this results in click features being extracted at eof */
	      k += n_channels;
	    }

	  if (!skip)
	    {
	      vector<double> fvector = build_frequency_vector (4096, samples);
	      spectrum.push_back (collapse_frequency_vector (fvector, dhandle->setup.mix_freq, 50, 1.6));
	    }
	}

      for (vector< vector<double> >::const_iterator si = spectrum.begin(); si != spectrum.end(); si++)
	{
	  avg_spectrum.resize (si->size());
	  for (size_t j = 0; j < si->size(); j++)
	    avg_spectrum[j] += (*si)[j] / spectrum.size();
	}
    }

  double avg_energy = 0;
  if (options.extractAvgEnergy)
    {
      GslDataPeekBuffer peek_buffer = { +1 /* incremental direction */, 0, };
      GslLong avg_energy_count = 0;

      for (GslLong k = options.channel; k < length; k++)
	{
	  double sample = gsl_data_handle_peek_value (dhandle, k, &peek_buffer);
	  avg_energy += sample * sample;
	  avg_energy_count++;
	}

      if (avg_energy_count)
	avg_energy /= avg_energy_count;

      avg_energy = 10 * log (avg_energy) / log (10);
    }

  /* print results */
  if (options.extractStartTime)
    {
      printHeader (options.extractStartTime, argv[1]);
      fprintf (options.extractStartTime, "# --start-time: signal start time in ms (first non-zero sample)\n");
      fprintf (options.extractStartTime, "%f\n", startTime);
    }

  if (options.extractEndTime)
    {
      printHeader (options.extractEndTime, argv[1]);
      fprintf (options.extractEndTime, "# --end-time: signal end time in ms (last non-zero sample)\n");
      fprintf (options.extractEndTime, "%f\n", endTime);
    }

  if (options.extractSpectrum)
    {
      printHeader (options.extractSpectrum, argv[1]);
      fprintf (options.extractSpectrum, "# --spectrum: frequency spectrum\n");

      for (vector< vector<double> >::const_iterator si = spectrum.begin(); si != spectrum.end(); si++)
	fput_vector (options.extractSpectrum, *si);
    }
  if (options.extractAvgSpectrum)
    {
      printHeader (options.extractAvgSpectrum, argv[1]);
      fprintf (options.extractAvgSpectrum, "# --avg-spectrum: average frequency spectrum\n");
      fput_vector (options.extractAvgSpectrum, avg_spectrum);
    }
  if (options.extractAvgEnergy)
    {
      printHeader (options.extractAvgEnergy, argv[1]);
      fprintf (options.extractAvgEnergy, "# --avg-energy: average signal energy in dB\n");
      fprintf (options.extractAvgEnergy, "%f\n", avg_energy);
    }
}

/* vim:set ts=8 sts=2 sw=2: */
