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
#include <bse/bsemathsignal.h>

#include <bse/gsldatautils.h>
#include <bse/gslloader.h>
#include <bse/gslfft.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include "topconfig.h"

#include <map>
#include <string>
#include <vector>
#include <utility>
#include <list>

using namespace std;

struct Options {
  string	      programName;
  guint               channel;
  bool                cut_zeros_head;
  bool                cut_zeros_tail;

  map<string, FILE*>  outputFiles;

  Options ();
  void parse (int *argc_p, char **argv_p[]);
  void printUsage ();

  FILE *openOutputFile (const char *filename);
} options;

class Signal
{
  mutable GslDataPeekBuffer peek_buffer;
  GslDataHandle	 *data_handle;
  guint		  signal_n_channels;
  GslLong	  signal_length;
  GslLong         signal_offset;

  /* check if the first sample is silent on all channels */
  bool head_is_silent()
  {
    for (guint i = 0; i < signal_n_channels; i++)
      if ((*this)[i] != 0)
	return false;

    return true;
  }

  /* check if the last sample is silent on all channels */
  bool tail_is_silent()
  {
    for (guint i = 0; i < signal_n_channels; i++)
      if ((*this)[signal_length - signal_n_channels + i] != 0)
	return false;

    return true;
  }

public:
  Signal (GslDataHandle *data_handle)
    : data_handle (data_handle)
  {
    signal_n_channels = gsl_data_handle_n_channels (data_handle);
    signal_length = gsl_data_handle_length (data_handle);
    signal_offset = 0;

    memset (&peek_buffer, 0, sizeof (peek_buffer));
    peek_buffer.dir = 1; /* incremental direction */;

    if (options.cut_zeros_head)
      {
	/* cut_zeros head */
	while (head_is_silent() && signal_length > signal_n_channels)
	  {
	    signal_offset += signal_n_channels;
	    signal_length -= signal_n_channels;
	  }
      }
    if (options.cut_zeros_tail)
      {
	/* cut_zeros tail */
	while (tail_is_silent() && signal_length > signal_n_channels)
	  {
	    signal_length -= signal_n_channels;
	  }
      }
  }

  GslLong length() const
  {
    return signal_length;
  }

  guint n_channels() const
  {
    return signal_n_channels;
  }

  double operator[] (GslLong k) const
  {
    return gsl_data_handle_peek_value (data_handle, k + signal_offset, &peek_buffer);
  }
  
  double mix_freq() const
  {
    return data_handle->setup.mix_freq;
  }

  double time_ms (GslLong k) const
  {
    GslLong n_frames = k / n_channels();
    return n_frames * 1000.0 / mix_freq();
  }
};

struct Feature;

list<Feature *> featureList;

struct Feature
{
  const char *option;
  const char *description;
  FILE *outputFile;

  void printValue (double data) const
  {
    fprintf (outputFile, "%f\n", data);
  }

  void printVector (const vector<double>& data) const
  {
    for (vector<double>::const_iterator di = data.begin(); di != data.end(); di++)
      fprintf (outputFile, (di == data.begin() ? "%f" : " %f"), *di);
    fprintf (outputFile, "\n");
  }

  Feature (const char *option, const char *description)
    : option (option), description (description), outputFile (NULL)
  {
  }

  virtual void compute (const Signal& signal) = 0;
  virtual void printResults() const = 0;
  virtual ~Feature()
  {
  }
};

struct StartTimeFeature : public Feature
{
  double startTime;
  StartTimeFeature() : Feature ("--start-time", "signal start time in ms (first non-zero sample)")
  {
    startTime = -1;
  }
  void compute (const Signal& signal)
  {
    for (GslLong l = options.channel; l < signal.length(); l += signal.n_channels())
      {
	if (signal[l] != 0)
	  {
	    startTime = signal.time_ms (l);
	    return;
	  }
      }
  }
  void printResults() const
  {
    printValue (startTime);
  }
};

struct EndTimeFeature : public Feature
{
  double endTime;
  EndTimeFeature() : Feature ("--end-time", "signal end time in ms (last non-zero sample)")
  {
    endTime = -1;
  }
  void compute (const Signal& signal)
  {
    for (GslLong l = options.channel; l < signal.length(); l += signal.n_channels())
      {
	if (signal[l] != 0)
	  endTime = signal.time_ms (l);
      }
  }
  void printResults() const
  {
    printValue (endTime);
  }
};

struct SpectrumFeature : public Feature
{
  vector< vector<double> > spectrum;

  SpectrumFeature() : Feature ("--spectrum", "frequency spectrum")
  {
  }

  vector<double>
  build_frequency_vector (GslLong size,
			  double *samples)
  {
    vector<double> fvector;
    double in[size], c[size + 2], *im;
    gint i;

    for (i = 0; i < size; i++)
      in[i] = bse_window_blackman (2.0 * i / size - 1.0) * samples[i]; /* the bse blackman window is defined in range [-1, 1] */

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

  vector<double>
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

  void compute (const Signal& signal)
  {
    if (spectrum.size()) /* don't compute the same feature twice */
      return;

    double file_size_ms = signal.time_ms (signal.length());

    for (double offset_ms = 0; offset_ms < file_size_ms; offset_ms += 30) /* extract a feature vector every 30 ms */
      {
	GslLong extract_frame = GslLong (offset_ms / file_size_ms * signal.length() / signal.n_channels());

	double samples[4096];
	bool skip = false;
	GslLong k = extract_frame * signal.n_channels() + options.channel;

	for (int j = 0; j < 4096; j++)
	  {
	    if (k < signal.length())
	      samples[j] = signal[k];
	    else
	      skip = true; /* alternative implementation: fill up with zeros;
			      however this results in click features being extracted at eof */
	    k += signal.n_channels();
	  }

	if (!skip)
	  {
	    vector<double> fvector = build_frequency_vector (4096, samples);
	    spectrum.push_back (collapse_frequency_vector (fvector, signal.mix_freq(), 50, 1.6));
	  }
      }
  }

  void printResults() const
  {
    for (vector< vector<double> >::const_iterator si = spectrum.begin(); si != spectrum.end(); si++)
      printVector (*si);
  }
};

struct AvgSpectrumFeature : public Feature
{
  SpectrumFeature *spectrumFeature;
  vector<double> avg_spectrum;

  AvgSpectrumFeature (SpectrumFeature *spectrumFeature) : Feature ("--avg-spectrum", "average frequency spectrum"),
							  spectrumFeature (spectrumFeature)
  {
  }

  void compute (const Signal& signal)
  {
    /*
     * dependancy: we need the spectrum to compute the average spectrum
     */
    spectrumFeature->compute (signal);

    for (vector< vector<double> >::const_iterator si = spectrumFeature->spectrum.begin(); si != spectrumFeature->spectrum.end(); si++)
    {
      avg_spectrum.resize (si->size());
      for (size_t j = 0; j < si->size(); j++)
	avg_spectrum[j] += (*si)[j] / spectrumFeature->spectrum.size();
    }
  }
  void printResults() const
  {
    printVector (avg_spectrum);
  }
};

struct AvgEnergyFeature : public Feature
{
  double avg_energy;

  AvgEnergyFeature() : Feature ("--avg-energy", "average signal energy in dB")
  {
    avg_energy = 0;
  }

  void compute (const Signal& signal)
  {
    GslLong avg_energy_count = 0;
    for (GslLong l = options.channel; l < signal.length(); l += signal.n_channels())
      {
	double sample = signal[l];

	avg_energy += sample * sample;
	avg_energy_count++;
      }

    if (avg_energy_count)
      avg_energy /= avg_energy_count;

    avg_energy = 10 * log (avg_energy) / log (10);
  }

  void printResults() const
  {
    fprintf (outputFile, "%f\n", avg_energy);
  }
};

struct MinMaxPeakFeature : public Feature
{
  double min_peak;
  double max_peak;

  MinMaxPeakFeature() : Feature ("--min-max-peak", "minimum and maximum signal peak")
  {
    min_peak = 0;
    max_peak = 0;
  }

  void compute (const Signal& signal)
  {
    for (GslLong l = options.channel; l < signal.length(); l += signal.n_channels())
      {
	min_peak = min (signal[l], min_peak);
	max_peak = max (signal[l], max_peak);
      }
  }

  void printResults() const
  {
    fprintf (outputFile, "%f\n", min_peak);
    fprintf (outputFile, "%f\n", max_peak);
  }
};

Options::Options ()
{
  programName = "bsefextract";
  channel = 0;
  cut_zeros_head = false;
  cut_zeros_tail = false;
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
      char *argv_copy = g_strdup (argv[i]);
      const char *opt = strtok (argv_copy, "=");
      const char *arg = opt ? strtok (NULL, "\n") : NULL;

      if (strcmp ("--help", opt) == 0)
	{
	  printUsage();
	  exit (0);
	}
      else if (strcmp ("--version", opt) == 0)
	{
	  printf ("%s %s\n", programName.c_str(), BST_VERSION);
	  exit (0);
	}
      else if (strcmp ("--cut-zeros", opt) == 0)
	{
	  cut_zeros_head = cut_zeros_tail = true;
	  argv[i] = NULL;
	}
      else if (strcmp ("--cut-zeros-head", opt) == 0)
	{
	  cut_zeros_head = true;
	  argv[i] = NULL;
	}
      else if (strcmp ("--cut-zeros-tail", opt) == 0)
	{
	  cut_zeros_tail = true;
	  argv[i] = NULL;
	}
      else if (strcmp ("--channel", opt) == 0)
	{
	  if (!arg)
	    {
	      printUsage();
	      exit (1);
	    }
	  channel = atoi (arg);
	  argv[i] = NULL;
	}
      else
	{
	  for (list<Feature*>::const_iterator fi = featureList.begin(); fi != featureList.end(); fi++)
	    {
	      if (strcmp ((*fi)->option, opt) == 0)
		{
		  (*fi)->outputFile = openOutputFile (arg);
		  argv[i] = NULL;
		}
	    }
	}
      g_free (argv_copy);
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
  for (list<Feature*>::const_iterator fi = featureList.begin(); fi != featureList.end(); fi++)
    printf (" %-28s%s\n", (*fi)->option, (*fi)->description);
  fprintf (stderr, "\n");
  fprintf (stderr, "other options:\n");
  fprintf (stderr, " --channel=<channel>         select channel (0: left, 1: right)\n");
  fprintf (stderr, " --help                      help for %s\n", programName.c_str());
  fprintf (stderr, " --version                   print version\n");
  fprintf (stderr, " --cut-zeros                 cut zero samples at start/end of the signal\n");
  fprintf (stderr, " --cut-zeros-head            cut zero samples at start of the signal\n");
  fprintf (stderr, " --cut-zeros-tail            cut zero samples at end of the signal\n");
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

  /* supported features */
  SpectrumFeature *spectrumFeature = new SpectrumFeature;

  featureList.push_back (new StartTimeFeature());
  featureList.push_back (new EndTimeFeature());
  featureList.push_back (spectrumFeature);
  featureList.push_back (new AvgSpectrumFeature (spectrumFeature));
  featureList.push_back (new AvgEnergyFeature());
  featureList.push_back (new MinMaxPeakFeature());

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
  Signal signal (dhandle);

  if (options.channel >= signal.n_channels())
    {
      fprintf (stderr, "%s: bad channel %d, input file %s has %d channels\n",
	       options.programName.c_str(), options.channel, argv[1], signal.n_channels());
      exit (1);
    }

  for (list<Feature*>::const_iterator fi = featureList.begin(); fi != featureList.end(); fi++)
    if ((*fi)->outputFile)
      (*fi)->compute (signal);

  /* print results */
  for (list<Feature*>::const_iterator fi = featureList.begin(); fi != featureList.end(); fi++)
    {
      const Feature& feature = *(*fi);
      if (feature.outputFile)
	{
	  printHeader (feature.outputFile, argv[1]);
	  fprintf (feature.outputFile, "# %s: %s\n", feature.option, feature.description);
	  feature.printResults();
	}
    }
}

/* vim:set ts=8 sts=2 sw=2: */
