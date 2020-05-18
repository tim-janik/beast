// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsetool.hh"
#include <bse/bsemain.hh>
#include <bse/bseengine.hh>
#include <bse/bsemathsignal.hh>
#include <bse/gsldatautils.hh>
#include <bse/bseloader.hh>
#include <bse/gslfft.hh>
#include <bse/gslfilter.hh>
#include "bse/internal.hh"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <map>
#include <string>
#include <vector>
#include <utility>
#include <list>
#include <complex>

using namespace Bse;
using namespace BseTool;

// using namespace std;
using std::string;
using std::map;
using std::list;
using std::vector;
using std::min;
using std::max;

struct FExtractOptions {
  String        program_name;
  uint          channel;
  bool          cut_zeros_head;
  bool          cut_zeros_tail;
  bool          verbose;
  double        silence_threshold;
  double        base_freq_hint;
  double        focus_center;
  double        focus_width;
  uint          join_spectrum_slices;
  uint          timing_window_stepping_ms;
  uint          timing_window_size_ms;
  FILE         *output_file;

  FExtractOptions       ();
  void assign_options   (const ArgParser &ap);
  void validate_percent (const string& option, double value);
  void validate_int     (const string& option, int value, int vmin, int vmax);
};

namespace { // Anon
FExtractOptions options;
} // Anon

class FeAudioSignal
{
  vector<float>   m_samples;
  GslDataHandle	 *m_data_handle;
  uint		  m_n_channels;
  GslLong	  m_length;
  GslLong         m_offset;

  /* check if the first sample is silent on all channels */
  bool head_is_silent()
  {
    for (uint i = 0; i < m_n_channels; i++)
      if (fabs ((*this)[i]) > options.silence_threshold)
	return false;

    return true;
  }

  /* check if the last sample is silent on all channels */
  bool tail_is_silent()
  {
    for (uint i = 0; i < m_n_channels; i++)
      if (fabs ((*this)[m_length - m_n_channels + i]) > options.silence_threshold)
	return false;

    return true;
  }

  /* empty signal? */
  bool empty()
  {
    if (m_length < m_n_channels)
      {
        assert_return (m_length == 0, false);
        return true;
      }
    return false;
  }

public:
  FeAudioSignal (GslDataHandle *data_handle) :
    m_data_handle (data_handle)
  {
    m_n_channels = gsl_data_handle_n_channels (data_handle);
    m_length = gsl_data_handle_length (data_handle);
    m_offset = 0;

    m_samples.resize (m_length);
    ssize_t have_samples = 0;
    while (have_samples < m_length)
      {
        int64 r = gsl_data_handle_read (data_handle, have_samples, MIN (m_length - have_samples, 4096 * m_n_channels),
                                        &m_samples[have_samples]);
        if (r < 0)
          {
            printerr ("error reading sample data\n");
            _exit (1);
          }
        have_samples += r;
      }

    if (options.cut_zeros_head)
      {
	/* cut_zeros head */
	while (!empty() && head_is_silent())
	  {
	    m_offset += m_n_channels;
	    m_length -= m_n_channels;
	  }
      }
    if (options.cut_zeros_tail)
      {
	/* cut_zeros tail */
	while (!empty() && tail_is_silent())
	  {
	    m_length -= m_n_channels;
	  }
      }

    // calculate focus - first: make sure focus region is inside the signal
    if (options.focus_center - (options.focus_width / 2.0) < 0.0)
      options.focus_center = (options.focus_width / 2.0);

    if (options.focus_center + (options.focus_width / 2.0) > 100.0)
      options.focus_center = 100.0 - (options.focus_width / 2.0);

    // cut samples which are outside the focus region
    double start = options.focus_center - (options.focus_width / 2.0);
    double end = options.focus_center + (options.focus_width / 2.0);

    GslLong istart = GslLong (start / 100.0 * m_length + 0.5);
    GslLong iend = GslLong (end / 100.0 * m_length + 0.5);

    istart -= istart % m_n_channels;
    iend -= iend % m_n_channels;

    m_offset += istart;
    m_length = iend - istart;
  }

  GslLong length() const
  {
    return m_length;
  }

  uint n_channels() const
  {
    return m_n_channels;
  }

  double operator[] (GslLong k) const
  {
    return m_samples[k + m_offset];
  }

  double mix_freq() const
  {
    return gsl_data_handle_mix_freq (m_data_handle);
  }

  double time_ms (GslLong k) const
  {
    GslLong n_frames = k / n_channels();
    return n_frames * 1000.0 / mix_freq();
  }
};

struct Feature;

list<Feature *> feature_list;

struct Feature
{
  const char *option;
  const char *description;
  bool        extract_feature;      /* did the user enable this feature with --feature? */

  string
  double_to_string (double value,
                    bool   align = false) const
  {
    gchar numbuf[G_ASCII_DTOSTR_BUF_SIZE + 1] = "";
    g_ascii_formatd (numbuf, G_ASCII_DTOSTR_BUF_SIZE, align ? "%-15.9g" : "%.9g", value);
    return numbuf;
  }

  void
  print_value (const string &value_name,
               double        data) const
  {
    fprintf (options.output_file, "%s = %s;\n", value_name.c_str(), double_to_string (data).c_str());
  }

  void
  print_vector (const string         &vector_name,
                const vector<double> &data) const
  {
    fprintf (options.output_file, "%s[%zd] = {", vector_name.c_str(), data.size());
    for (vector<double>::const_iterator di = data.begin(); di != data.end(); di++)
      fprintf (options.output_file, " %s", double_to_string (*di, true).c_str());
    fprintf (options.output_file, " };\n");
  }

  void
  print_matrix (const string                   &matrix_name,
                const vector< vector<double> > &matrix) const
  {
    /* for a m x n matrix, we write
     * 
     * data[m,n] = {
     *   { x_11 x_12 ... x_1n }
     *   { x_21 x_22 ... x_2n }
     *   {  .    .   ...  .   }
     *   {  .    .   ...  .   }
     *   { x_m1 x_m2 ... x_mn }
     * };
     */
    fprintf (options.output_file, "%s[%zd,%zd] = {\n",
	     matrix_name.c_str(), matrix.size(), matrix.size() ? matrix[0].size() : 0);

    for (vector< vector<double> >::const_iterator mi = matrix.begin(); mi != matrix.end(); mi++)
      {
	fprintf (options.output_file, "  {");
	const vector<double>& line = *mi;

	for (vector<double>::const_iterator li = line.begin(); li != line.end(); li++)
	  fprintf (options.output_file, " %s", double_to_string (*li, true).c_str());
	fprintf (options.output_file, " }\n");
      }
    fprintf (options.output_file, "};\n");
  }

  Feature (const char *option,
           const char *description) :
    option (option),
    description (description),
    extract_feature (false)
  {
  }

  virtual void compute (const FeAudioSignal &signal) = 0;
  virtual void print_results() const = 0;
  virtual ~Feature()
  {
  }
};

struct StartTimeFeature : public Feature
{
  double start_time;
  StartTimeFeature() :
    Feature ("--start-time", "signal start time in ms (first non-zero sample)")
  {
    start_time = -1;
  }
  void
  compute (const FeAudioSignal &signal)
  {
    for (GslLong l = options.channel; l < signal.length(); l += signal.n_channels())
      {
	if (signal[l] != 0)
	  {
	    start_time = signal.time_ms (l);
	    return;
	  }
      }
  }
  void
  print_results() const
  {
    print_value ("start_time", start_time);
  }
};

struct EndTimeFeature : public Feature
{
  double end_time;
  EndTimeFeature() : Feature ("--end-time", "signal end time in ms (last non-zero sample)")
  {
    end_time = -1;
  }
  void compute (const FeAudioSignal &signal)
  {
    for (GslLong l = options.channel; l < signal.length(); l += signal.n_channels())
      {
	if (signal[l] != 0)
	  end_time = signal.time_ms (l);
      }
  }
  void print_results() const
  {
    print_value ("end_time", end_time);
  }
};

struct SpectrumFeature : public Feature
{
  vector< vector<double> > spectrum;
  vector< vector<double> > joined_spectrum;
  vector< double >         window;

  SpectrumFeature() :
    Feature ("--spectrum", "generate 30ms sliced frequency spectrums")
  {
  }

  void
  init_window (size_t size)
  {
    window.resize (size);

    for (size_t i = 0; i < size; i++)
      window[i] = bse_window_blackman (2.0 * i / size - 1.0); /* the bse blackman window is defined in range [-1, 1] */
  }

  vector<double>
  build_frequency_vector (double *samples)
  {
    const size_t size = window.size();
    assert_return (size > 0, vector<double>());

    vector<double> fvector;
    double in[size], c[size + 2], *im;

    for (size_t i = 0; i < size; i++)
      in[i] = window[i] * samples[i];

    gsl_power2_fftar (size, in, c);
    c[size] = c[1];
    c[size + 1] = 0;
    c[1] = 0;
    im = c + 1;

    for (size_t i = 0; i <= size >> 1; i++)
      {
	double abs = sqrt (c[i << 1] * c[i << 1] + im[i << 1] * im[i << 1]);
	/* FIXME: is this the correct normalization? */
	fvector.push_back (abs / size);
      }
    return fvector;
  }

  vector<double>
  collapse_frequency_vector (const vector<double> &fvector,
			     double                mix_freq,
			     double                first_freq,
			     double                factor)
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

  vector<double>
  static join_slices (vector< vector<double> >::const_iterator start,
		      vector< vector<double> >::const_iterator end,
		      double                                   normalize)
  {
    const vector < double > empty_return; // this can't be inlined as return-statement due to a bug in g++-3.3
    assert_return (end - start > 0, empty_return);

    vector<double> result (start->size());

    for (vector< vector<double> >::const_iterator spect_it = start; spect_it != end; spect_it++)
      {
	assert_return (spect_it->size() == result.size(), result);

	for (size_t i = 0; i < result.size(); i++)
	  result[i] += (*spect_it)[i];
      }
    for (size_t i = 0; i < result.size(); i++)
      result[i] /= normalize;

    return result;
  }

  void
  compute (const FeAudioSignal &signal)
  {
    if (spectrum.size()) /* don't compute the same feature twice */
      return;

    init_window (4096);

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
	    vector<double> fvector = build_frequency_vector (samples);
	    spectrum.push_back (collapse_frequency_vector (fvector, signal.mix_freq(), 50, 1.6));
	  }
      }

    if (options.join_spectrum_slices > 1)
      {
	typedef vector< vector<double> >::const_iterator SpectrumConstIterator;
	const uint jslices = options.join_spectrum_slices; 

	/* for N-fold joining, we "truncate" the spectrum so that we only
	 * have complete sets of N 30ms spectrum buckets to join
	 */
	for (size_t i = 0; i + jslices <= spectrum.size(); i += jslices)
	  {
	    SpectrumConstIterator jstart_it = spectrum.begin();
	    SpectrumConstIterator jend_it = spectrum.begin() + jslices;
	    joined_spectrum.push_back (join_slices (jstart_it, jend_it, jslices));
	  }
      }
  }

  void print_results() const
  {
    if (options.join_spectrum_slices > 1)
      {
	fprintf (options.output_file,
	         "# this spectrum was computed with --join-spectrum-slices=%d\n",
	         options.join_spectrum_slices);
	print_matrix ("spectrum", joined_spectrum);
      }
    else
      print_matrix ("spectrum", spectrum);
  }
};

struct AvgSpectrumFeature : public Feature
{
  SpectrumFeature *spectrum_feature;
  vector<double>   avg_spectrum;

  AvgSpectrumFeature (SpectrumFeature *spectrum_feature) :
    Feature ("--avg-spectrum", "average frequency spectrum"),
    spectrum_feature (spectrum_feature)
  {
  }

  void compute (const FeAudioSignal &signal)
  {
    /*
     * dependancy: we need the spectrum to compute the average spectrum
     */
    spectrum_feature->compute (signal);

    for (vector< vector<double> >::const_iterator si = spectrum_feature->spectrum.begin(); si != spectrum_feature->spectrum.end(); si++)
    {
      avg_spectrum.resize (si->size());
      for (size_t j = 0; j < si->size(); j++)
	avg_spectrum[j] += (*si)[j] / spectrum_feature->spectrum.size();
    }
  }
  void print_results() const
  {
    print_vector ("avg_spectrum", avg_spectrum);
  }
};

struct AvgEnergyFeature : public Feature
{
  double avg_energy;

  AvgEnergyFeature() : Feature ("--avg-energy", "average signal energy in dB")
  {
    avg_energy = 0;
  }

  void compute (const FeAudioSignal &signal)
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

    avg_energy = bse_db_from_factor (sqrt (avg_energy), -200);
  }

  void print_results() const
  {
    print_value ("avg_energy", avg_energy);
  }
};

struct MinMaxPeakFeature : public Feature
{
  double min_peak;
  double max_peak;

  MinMaxPeakFeature() :
    Feature ("--min-max-peak", "minimum and maximum signal peak")
  {
    min_peak = 0;
    max_peak = 0;
  }

  void compute (const FeAudioSignal &signal)
  {
    for (GslLong l = options.channel; l < signal.length(); l += signal.n_channels())
      {
	min_peak = min (signal[l], min_peak);
	max_peak = max (signal[l], max_peak);
      }
  }

  void print_results() const
  {
    print_value ("min_peak", min_peak);
    print_value ("max_peak", max_peak);
  }
};

struct DCOffsetFeature : public Feature
{
  double dc_offset;

  DCOffsetFeature() :
    Feature ("--dc-offset-db", "computes the DC offset in dB")
  {
    dc_offset = 0;
  }

  void compute (const FeAudioSignal &signal)
  {
    double dc_offset_div = 0.0;

    for (GslLong l = options.channel; l < signal.length(); l += signal.n_channels())
      {
        dc_offset += signal[l];
        dc_offset_div += 1.0;
      }

    if (dc_offset_div > 0.5)
      dc_offset /= dc_offset_div;
  }

  void print_results() const
  {
    print_value ("dc_offset_db", bse_db_from_factor (dc_offset, -200));
  }
};

struct RawSignalFeature : public Feature
{
  vector<double> raw_signal;

  RawSignalFeature() : Feature ("--raw-signal", "extract raw signal")
  {
  }

  void compute (const FeAudioSignal &signal)
  {
    for (GslLong l = options.channel; l < signal.length(); l += signal.n_channels())
      raw_signal.push_back (signal[l]);
  }

  void print_results() const
  {
    /* This is more or less a debugging feature (for gnuplot or so),
     * so we don't print it using the usual print_vector or print_value
     * functions (because then gnuplot couldn't parse it any more).
     */
    for (uint i = 0; i < raw_signal.size(); i++)
      fprintf (options.output_file, "%s\n", double_to_string (raw_signal[i]).c_str());
  }
};

struct ComplexSignalFeature : public Feature
{
  static const int HSIZE = 256;

  vector< std::complex<double> > complex_signal;
  double                         hilbert[2*HSIZE+1];

  /*
   * Evaluates the FIR frequency response of the hilbert filter.
   *
   * freq = [0..pi] corresponds to [0..mix_freq/2]
   */
  std::complex<double>
  evaluate_hilbert_response (double freq)
  {
    std::complex<double> response = hilbert[HSIZE];

    for (int i = 1; i <= HSIZE; i++)
      {
	response += std::exp (std::complex<double> (0, -i * freq)) * hilbert[HSIZE-i];
	response += std::exp (std::complex<double> (0, i * freq)) * hilbert[HSIZE+i];
      }
    return response;
  }

  /* returns a blackman window: x is supposed to be in the interval [0..1] */
  static float blackman_window (float x)
  {
    if(x < 0) return 0;
    if(x > 1) return 0;
    return 0.42-0.5*cos(M_PI*x*2)+0.08*cos(4*M_PI*x);
  }

  /* blackman window with x in [-1 .. 1] */
  static float bwindow (float x)
  {
    return blackman_window ((x + 1.0) / 2.0);
  }

  ComplexSignalFeature() :
    Feature ("--complex-signal", "extract complex signal (hilbert filtered)")
  {
    /* compute hilbert fir coefficients */
    for (int i = 0; i <= HSIZE; i++)
      {
	double x;
	if (i & 1)
	  x = 1./double(i) * bwindow (double(i) / double(HSIZE));
	else
	  x = 0.0;
	hilbert[HSIZE+i] = x;
	hilbert[HSIZE-i] = -x;
      }

    /* normalize the filter coefficients */
    double gain = std::abs (evaluate_hilbert_response (M_PI/2.0));
    for (int i = 0; i <= HSIZE; i++)
      {
	hilbert[HSIZE+i] /= gain;
	hilbert[HSIZE-i] /= gain;
      }
  }

  void compute (const FeAudioSignal &signal)
  {
    if (complex_signal.size()) /* already finished? */
      return;

    /*
     * performance: this loop could be rewritten to be faster, especially by
     *
     * (a) special casing head and tail computation, so that the if can be
     *     removed from the innermost loop
     * (b) taking into account that half of the hilbert filter coefficients
     *     are zero anyway
     */
    for (unsigned int i = options.channel; i < signal.length(); i += signal.n_channels())
      {
	double re = signal[i];
	double im = 0;

	int pos = i - HSIZE * signal.n_channels();
	for (int k = -HSIZE; k <= HSIZE; k++)
	  {
	    if (pos >= 0 && pos < signal.length())
	      im += signal[pos] * hilbert[k + HSIZE];

	    pos += signal.n_channels();
	  }
	complex_signal.push_back (std::complex<double> (re, im));
      }
  }

  void
  print_results() const
  {
    /* This is more or less a debugging feature (for gnuplot or so),
     * so we don't print it using the usual print_vector or print_value
     * functions (because then gnuplot couldn't parse it any more).
     */
    for (uint i = 0; i < complex_signal.size(); i++)
      fprintf (options.output_file, "%s %s\n", double_to_string (complex_signal[i].real()).c_str(),
                                               double_to_string (complex_signal[i].imag()).c_str());
  }
};

struct BaseFreqFeature : public Feature
{
  ComplexSignalFeature *complex_signal_feature;
  vector<double> freq;

  double base_freq;
  double base_freq_smear;
  double base_freq_wobble;

  BaseFreqFeature (ComplexSignalFeature *complex_signal_feature) :
    Feature ("--base-freq", "try to detect pitch of a signal"),
    complex_signal_feature (complex_signal_feature)
  {
    base_freq = 0;
    base_freq_smear = 0;
    base_freq_wobble = 0;
  }

  void
  compute (const FeAudioSignal &signal)
  {
    if (freq.size()) /* already finished? */
      return;

    /*
     * dependancy: we need the complex signal to compute the base frequency
     */
    complex_signal_feature->compute (signal);

    /*
     * if the user specified a base frequency hint, we search especially in
     * a +/- 10% range around that hint; to do so, we use a 2nd order
     * butterworth bandpass
     */
    const int BANDPASS_ORDER = 2;
    double a[BANDPASS_ORDER + 1] = { 0, };
    double b[BANDPASS_ORDER + 1] = { 0, };

    if (options.base_freq_hint > 0)
      {
	gsl_filter_butter_bp (BANDPASS_ORDER,
	    options.base_freq_hint / signal.mix_freq() * 2 * M_PI * 0.90, /* -10 % */
	    options.base_freq_hint / signal.mix_freq() * 2 * M_PI * 1.10, /* +10 % */
	    0.1, a, b);
      }

    std::complex<double> x0, x1, x2, y0, y1, y2;

    double last_phase = 0.0;
    double base_freq_div = 0.01; /* avoid division by zero */

#if 0 // test filter
    for (double i = 1.0; i < 10000; i = i * 11 / 10)
      {
	double vol = 0.0;
	int s;
	for (s = 0; s < 10000; s++)
	  {
	    std::complex<double> sig = std::complex<double> (sin (s * i / 44100.0 * 2 * M_PI), cos (s * i / 44100.0 * 2 * M_PI));

	    /* evaluate butterworth filter */
	    x0 = sig;
	    y0 = x0 * a[0] + x1 * a[1] + x2 * a[2] - y1 * b[1] - y2 * b[2];
	    x2 = x1; x1 = x0; y2 = y1; y1 = y0;

	    vol += std::abs (y0);
	  }
	printf ("%f %f\n", i, vol / s);
      }
    _exit (1);
#endif

    for (vector< std::complex<double> >::const_iterator si = complex_signal_feature->complex_signal.begin();
	                                                si != complex_signal_feature->complex_signal.end(); si++)
    {
      if (options.base_freq_hint > 0)
	{
	  x0 = *si;
	  y0 = x0 * a[0] + x1 * a[1] + x2 * a[2] - y1 * b[1] - y2 * b[2];
	  x2 = x1; x1 = x0; y2 = y1; y1 = y0;
	}
      else
	{
	  y0 = *si;
	}

      /* determine frequency value from phase difference */
      double phase = std::arg (y0);
      double phase_diff = last_phase - phase;

      if (phase_diff > M_PI)
	phase_diff -= 2.0*M_PI;
      else if(phase_diff < -M_PI)
	phase_diff += 2.0*M_PI;

      last_phase = phase;

      double current_freq = fabs (phase_diff / 2.0 / M_PI) * signal.mix_freq();
      freq.push_back (current_freq);

      /*
       * The following if-statement does something similar like --cut-zeros: (FIXME?)
       * 
       * It cuts away those parts of the signal where no sane frequency was detected.
       * I am not sure whether it is necessary, but I suppose it's safe to leave it here.
       */
      if (current_freq > 1.0)
	{
	  base_freq += current_freq;
	  base_freq_div += 1.0;
	}
    }

    base_freq /= base_freq_div;

    compute_smear_and_wobble (signal);
  }

  void
  compute_smear_and_wobble (const FeAudioSignal &signal)
  {
    const int window_size = int (signal.mix_freq() / base_freq + 0.5);
    const int window_step = max (window_size / 3, 30);

    double last_avg_base_freq = 0.0;

    double smear_sum = 0.0, wobble_sum = 0.0;
    double window_count = 0.0;

    for (size_t offset = 0; (offset + 2 * window_step) < freq.size(); offset += window_step)
      {
	// compute average value of the base frequency of window_size samples
	double avg_base_freq = 0.0, avg_base_freq_div = 0.0;

	for (int i = 0; i < window_size; i++)
	  {
	    if (offset + i < freq.size())
	      {
		avg_base_freq += freq[offset + i];
		avg_base_freq_div += 1.0;
	      }
	  }
	if (avg_base_freq_div > 0.0)
	  avg_base_freq /= avg_base_freq_div;

	// --base-freq-smear computation
	smear_sum += fabs (avg_base_freq - base_freq);
	wobble_sum += fabs (last_avg_base_freq - avg_base_freq);

	window_count += 1.0;

	last_avg_base_freq = avg_base_freq;
      }

    if (window_count > 0.0)
      {
	base_freq_smear = smear_sum / window_count;
	base_freq_wobble = wobble_sum / window_count;
      }
    else
      {
	base_freq_smear = 0.0;
	base_freq_wobble = 0.0;
      }
  }

  void print_results() const
  {
    print_value ("base_freq", base_freq);
  }
};

struct BaseFreqSmear : public Feature
{
  BaseFreqFeature *base_freq_feature;

  BaseFreqSmear (BaseFreqFeature *base_freq_feature) :
    Feature ("--base-freq-smear", "inaccuracy of pitch detection"),
    base_freq_feature (base_freq_feature)
  {
  }

  void compute (const FeAudioSignal& signal)
  {
    /*
     * dependancy: we need the base frequency feature to compute the base frequency smear
     */
    base_freq_feature->compute (signal);
  }

  void print_results() const
  {
    print_value ("base_freq_smear", base_freq_feature->base_freq_smear);
  }
};

struct BaseFreqWobble : public Feature
{
  BaseFreqFeature *base_freq_feature;

  BaseFreqWobble (BaseFreqFeature *base_freq_feature)
    : Feature ("--base-freq-wobble", "rate of changes in the pitch over time"),
      base_freq_feature (base_freq_feature)
  {
  }

  void compute (const FeAudioSignal &signal)
  {
    /*
     * dependancy: we need the base frequency feature to compute the base frequency smear
     */
    base_freq_feature->compute (signal);
  }

  void print_results() const
  {
    print_value ("base_freq_wobble", base_freq_feature->base_freq_wobble);
  }
};

struct VolumeFeature : public Feature
{
  ComplexSignalFeature *complex_signal_feature;
  vector<double> vol;

  double volume;
  double volume_smear;
  double volume_wobble;

  VolumeFeature (ComplexSignalFeature *complex_signal_feature)
    : Feature ("--volume", "determine average signal volume"),
      complex_signal_feature (complex_signal_feature)
  {
    volume = 0;
    volume_smear = 0;
    volume_wobble = 0;
  }

  void compute (const FeAudioSignal &signal)
  {
    if (vol.size()) /* already finished? */
      return;

    /*
     * dependancy: we need the complex signal to compute the base frequency
     */
    complex_signal_feature->compute (signal);

    volume = 0.0;

    for (vector< std::complex<double> >::const_iterator si = complex_signal_feature->complex_signal.begin();
	                                                si != complex_signal_feature->complex_signal.end(); si++)
      {
	double v = std::abs (*si); //sqrt (si->real() * si->real() + si->imag() * si->imag()); //std::abs (*si);

	vol.push_back (v);
	volume += v;
      }

    if (vol.size())
      volume /= vol.size();

    compute_smear_and_wobble (signal);
  }

  void compute_smear_and_wobble (const FeAudioSignal &signal)
  {
    const double window_size_ms = 30; /* window size in milliseconds */
    const int window_size = int (signal.mix_freq() * window_size_ms / 1000.0 + 0.5);
    const int window_step = max (window_size / 3, 30);

    double last_avg_volume = 0.0;
    double window_count = 0.0;

    volume_smear = 0.0;
    volume_wobble = 0.0;

    for (size_t offset = 0; (offset + 2 * window_step) < vol.size(); offset += window_step)
      {
	// compute average value of the base frequency of window_size samples
	double avg_volume = 0.0, avg_volume_div = 0.0;

	for (int i = 0; i < window_size; i++)
	  {
	    if (offset + i < vol.size())
	      {
		avg_volume += vol[offset + i];
		avg_volume_div += 1.0;
	      }
	  }
	if (avg_volume_div > 0.0)
	  avg_volume /= avg_volume_div;

	volume_smear += fabs (avg_volume - volume);
	volume_wobble += fabs (last_avg_volume - avg_volume);

	window_count += 1.0;

	last_avg_volume = avg_volume;
      }

    if (window_count > 0.0)
      {
	volume_smear /= window_count;
	volume_wobble /= window_count;
      }
  }

  void print_results() const
  {
    print_value ("volume", volume);
  }
};

struct VolumeSmear : public Feature
{
  VolumeFeature *volume_feature;

  VolumeSmear (VolumeFeature *volume_feature) :
    Feature ("--volume-smear", "variation of signal volume"),
    volume_feature (volume_feature)
  {
  }

  void compute (const FeAudioSignal &signal)
  {
    // dependancy: we need the volume feature to compute the volume smear
    volume_feature->compute (signal);
  }

  void print_results() const
  {
    print_value ("volume_smear", volume_feature->volume_smear);
  }
};

struct VolumeWobble : public Feature
{
  VolumeFeature *volume_feature;


  VolumeWobble (VolumeFeature *volume_feature) :
    Feature ("--volume-wobble", "rate of changes in signal volume over time"),
    volume_feature (volume_feature)
  {
  }

  void compute (const FeAudioSignal &signal)
  {
    // dependancy: we need the volume feature to compute the volume wobble
    volume_feature->compute (signal);
  }

  void print_results() const
  {
    print_value ("volume_wobble", volume_feature->volume_wobble);
  }
};

struct TimingSlices
{
  enum SpectralFluxType
  {
    SPECTRAL_FLUX_POSITIVE,
    SPECTRAL_FLUX_NEGATIVE
  };
  vector< vector<double> > slices;

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

  void
  compute (const FeAudioSignal& signal)
  {
    if (slices.size()) /* don't compute the same feature twice */
      return;

    double file_size_ms = signal.time_ms (signal.length());
    uint fft_size_samples = 2;

    while (fft_size_samples / signal.mix_freq() * 1000 < options.timing_window_size_ms)
      fft_size_samples *= 2;

    if (options.verbose)
      {
	printerr ("timing window (for attack and release detection):\n");
	printerr ("  * size         %5.2f ms    %6u samples\n",
	  fft_size_samples / signal.mix_freq() * 1000,
	  fft_size_samples);
	printerr ("  * stepping     %5.2f ms    %6u samples\n",
	  float (options.timing_window_stepping_ms),
	  uint (options.timing_window_stepping_ms * signal.mix_freq() / 1000));
      }

    for (double offset_ms = 0; offset_ms < file_size_ms; offset_ms += options.timing_window_stepping_ms)
      {
	GslLong extract_frame = GslLong (offset_ms / file_size_ms * signal.length() / signal.n_channels());

	double samples[fft_size_samples];
	bool skip = false;
	GslLong k = extract_frame * signal.n_channels() + options.channel;

	for (uint j = 0; j < fft_size_samples; j++)
	  {
	    if (k < signal.length())
	      samples[j] = signal[k];
	    else
	      skip = true; /* alternative implementation: fill up with zeros;
			      however this results in click features being extracted at eof */
	    k += signal.n_channels();
	  }

	if (!skip)
	  slices.push_back (build_frequency_vector (fft_size_samples, samples));
      }
  }

  int
  n_slices()
  {
    return slices.size();
  }

  double
  spectral_flux (int             slice1,
                 int             slice2,
		 SpectralFluxType sf_type)
  {
    /* make s1 and s2 point at the desired slices, or, if they are out of range,
     * at a sufficiently large null slice (where sufficiently large may be 0,
     * if both slices were out of the range for which features have been extracted) */
    vector<double> null_slice;
    vector<double>& s1 = (slice1 >= 0 && slice1 < (ssize_t) slices.size()) ? slices[slice1] : null_slice;
    vector<double>& s2 = (slice2 >= 0 && slice2 < (ssize_t) slices.size()) ? slices[slice2] : null_slice;
    null_slice.resize (max (s1.size(), s2.size()));

    double sf = 0;
    for (size_t i = 0; i < s1.size(); i++)
      {
	if ((s1[i] < s2[i]) == (sf_type == SPECTRAL_FLUX_POSITIVE))
	  sf += fabs (s1[i] - s2[i]);
      }
    return sf;
  }
};

struct AttackTimes : public Feature
{
  TimingSlices   *timing_slices;
  vector<double>  attack_times;

  AttackTimes (TimingSlices *timing_slices) :
    Feature ("--attack-times", "compute timestamps for possible note attacks"),
    timing_slices (timing_slices)
  {
  }

  void
  compute (const FeAudioSignal &signal)
  {
    timing_slices->compute (signal);

    for (int i = 0; i < timing_slices->n_slices(); i++)
      attack_times.push_back (timing_slices->spectral_flux (i - 1, i, TimingSlices::SPECTRAL_FLUX_POSITIVE));
  }

  void
  print_results() const
  {
    print_vector ("attack_times", attack_times);
    // for (uint i = 0; i < attack_times.size(); i++) // debugging (gnuplot output)
    //   printf ("%g\n", attack_times[i]);
  }
};

struct ReleaseTimes : public Feature
{
  TimingSlices   *timing_slices;
  vector<double>  release_times;

  ReleaseTimes (TimingSlices *timing_slices) :
    Feature ("--release-times", "compute timestamps for possible note releases"),
    timing_slices (timing_slices)
  {
  }

  void
  compute (const FeAudioSignal &signal)
  {
    timing_slices->compute (signal);

    for (int i = 0; i < timing_slices->n_slices(); i++)
      release_times.push_back (timing_slices->spectral_flux (i - 1, i, TimingSlices::SPECTRAL_FLUX_NEGATIVE));
  }

  void
  print_results() const
  {
    print_vector ("release_times", release_times);
    // for (uint i = 0; i < release_times.size(); i++) // debugging (gnuplot output)
    //  printf ("%g\n", release_times[i]);
  }
};

FExtractOptions::FExtractOptions () :
  join_spectrum_slices (1)
{
  program_name = "bsetool fextract";
  channel = 0;
  verbose = false;
  cut_zeros_head = false;
  cut_zeros_tail = false;
  silence_threshold = 0.0;
  base_freq_hint = 0.0;
  focus_center = 50.0;
  focus_width = 100.0;
  timing_window_stepping_ms = 30;
  timing_window_size_ms = 50;
  output_file = stdout;
}

void
FExtractOptions::validate_percent (const string &option, double value)
{
  if (value < 0.0 || value > 100.0)
    {
      printerr ("%s: invalid argument `%f' for `%s'\n\n", program_name, value, option.c_str());
      printerr ("Valid arguments are percent values (between 0 and 100).\n");
      printerr ("Try `%s --help' for more information.\n", program_name);
      _exit (1);
    }
}

void
FExtractOptions::validate_int (const string &option, int value, int vmin, int vmax)
{
  if (value < vmin || value > vmax)
    {
      printerr ("%s: invalid argument `%d' for `%s'\n\n", program_name, value, option.c_str());
      printerr ("Valid arguments are between %d and %d.\n", vmin, vmax);
      printerr ("Try `%s --help' for more information.\n", program_name);
      _exit (1);
    }
}

static void
print_header (const String &src)
{
  fprintf (options.output_file, "# this output was generated by %s %s from channel %d in file %s\n",
           options.program_name.c_str(), Bse::version().c_str(), options.channel, src.c_str());
  fprintf (options.output_file, "#\n");
}

static ArgDescription fextract_options[32] = {
  { "<audiofile>", "",                  "Audio file to extract features from", "", },
  { "--verbose", "",                    "Verbose feature extraction", "" },
  { "--channel", "<channel>",           "select channel (0: left, 1: right)", "" },
  { "--cut-zeros", "",                  "cut zero samples at start/end of the signal", "" },
  { "--cut-zeros-head", "",             "cut zero samples at start of the signal", "" },
  { "--cut-zeros-tail", "",             "cut zero samples at end of the signal", "" },
  { "--silence-threshold", "",          "threshold for zero cutting (as 16bit sample value)", "0" },
  { "--focus-center", "<X>",            "center focus region around X%% [50]", "50" },
  { "--focus-width", "<Y>",             "width of focus region in %% [100]", "100" },
  { "--base-freq-hint", "",             "expected base frequency (for the pitch detection)", "0" },
  { "--join-spectrum-slices", "<N>",    "when extracting a spectrum, join N 30ms slices", "1" },
  { "--timing-window-size", "<N>",      "attack/release detector window size in ms [50] (actual window size may be larger, use --verbose)", "50" },
  { "--timing-window-stepping", "<N>",  "attack/release detector stepping in ms [30]", "30" },
  { "-o", "<output_file>",              "set the name of a file to write the features to", "-" },
};

static const char*
fextract_blurb (const char *blurb)
{ // slight hack to finish up fextract_options before calling CommandRegistry::CommandRegistry()
  const int NOPTS = 14;
  assert_return (fextract_options[NOPTS - 1].arg_name != NULL, NULL);
  assert_return (fextract_options[NOPTS].arg_name == NULL, NULL); // call *once* only
  /* supported features */
  SpectrumFeature *spectrum_feature = new SpectrumFeature;
  ComplexSignalFeature *complex_signal_feature = new ComplexSignalFeature;
  BaseFreqFeature *base_freq_feature = new BaseFreqFeature (complex_signal_feature);
  VolumeFeature *volume_feature = new VolumeFeature (complex_signal_feature);
  TimingSlices *timing_slices = new TimingSlices;  // not user visible
  feature_list.push_back (new StartTimeFeature());
  feature_list.push_back (new EndTimeFeature());
  feature_list.push_back (spectrum_feature);
  feature_list.push_back (new AvgSpectrumFeature (spectrum_feature));
  feature_list.push_back (new AvgEnergyFeature());
  feature_list.push_back (new MinMaxPeakFeature());
  feature_list.push_back (new DCOffsetFeature());
  feature_list.push_back (new RawSignalFeature());
  feature_list.push_back (complex_signal_feature);
  feature_list.push_back (base_freq_feature);
  feature_list.push_back (new BaseFreqSmear (base_freq_feature));
  feature_list.push_back (new BaseFreqWobble (base_freq_feature));
  feature_list.push_back (volume_feature);
  feature_list.push_back (new VolumeSmear (volume_feature));
  feature_list.push_back (new VolumeWobble (volume_feature));
  feature_list.push_back (new AttackTimes (timing_slices));
  feature_list.push_back (new ReleaseTimes (timing_slices));
  // add feature options
  size_t i = NOPTS;
  for (const auto &feat : feature_list)
    {
      fextract_options[i].arg_name = feat->option;
      fextract_options[i].value_name = "";
      fextract_options[i].arg_blurb = feat->description;
      i++;
    }
  return blurb;
}

void
FExtractOptions::assign_options (const ArgParser &ap)
{
  // assign options
  verbose = ap["verbose"] == "1";
  if (ap["cut-zeros"] == "1")
    cut_zeros_head = cut_zeros_tail = true;
  if (ap["cut-zeros-head"] == "1")
    cut_zeros_head = true;
  if (ap["cut-zeros-tail"] == "1")
    cut_zeros_tail = true;
  silence_threshold = g_ascii_strtod (ap["silence-threshold"].c_str(), NULL) / 32767.0;
  focus_width = g_ascii_strtod (ap["focus-width"].c_str(), NULL);
  validate_percent ("--focus-width", focus_width);
  focus_center = g_ascii_strtod (ap["focus-center"].c_str(), NULL);
  validate_percent ("--focus-center", focus_center);
  base_freq_hint = g_ascii_strtod (ap["base-freq-hint"].c_str(), NULL);
  timing_window_size_ms = g_ascii_strtod (ap["timing-window-size"].c_str(), NULL);
  timing_window_stepping_ms = g_ascii_strtod (ap["timing-window-stepping"].c_str(), NULL);
  channel = atoi (ap["channel"].c_str());
  join_spectrum_slices = atoi (ap["join-spectrum-slices"].c_str());
  validate_int ("--join-spectrum-slices", join_spectrum_slices, 1, 100000);
  const String outputfile = ap["o"];
  if (outputfile == "-")
    output_file = stdout;
  else
    {
      output_file = fopen (outputfile.c_str(), "w");
      if (!output_file)
        {
          printerr ("%s: failed to open '%s' for writing: %s\n", program_name, outputfile, strerror (errno));
          _exit (3);
        }
    }
  for (list<Feature*>::const_iterator fi = feature_list.begin(); fi != feature_list.end(); fi++)
    {
      const char *const fname = (*fi)->option;
      assert_return (fname[0] == '-' && fname[1] == '-');
      if (ap[fname + 2] == "1")
        (*fi)->extract_feature = true;
    }
}

static String
fextract_run (const ArgParser &ap)
{
  // INIT-NEEDS: "stand-alone=1", "wave-chunk-padding=1", "dcache-block-size=8192", "dcache-cache-memory=5242880"
  // FExtractOptions options;
  options.assign_options (ap);

  /* open input */
  Bse::Error error;
  const String audiofile = ap["audiofile"];
  BseWaveFileInfo *wave_file_info = bse_wave_file_info_load (audiofile.c_str(), &error);
  if (!wave_file_info)
    {
      printerr ("%s: failed to open the input file %s: %s\n", options.program_name, audiofile, bse_error_blurb (error));
      _exit (1);
    }

  BseWaveDsc *waveDsc = bse_wave_dsc_load (wave_file_info, 0, FALSE, &error);
  if (!waveDsc)
    {
      printerr ("%s: failed to open the input file %s: %s\n", options.program_name, audiofile, bse_error_blurb (error));
      _exit (1);
    }

  GslDataHandle *dhandle = bse_wave_handle_create (waveDsc, 0, &error);
  if (!dhandle)
    {
      printerr ("%s: failed to open the input file %s: %s\n", options.program_name, audiofile, bse_error_blurb (error));
      _exit (1);
    }

  error = gsl_data_handle_open (dhandle);
  if (error != 0)
    {
      printerr ("%s: failed to open the input file %s: %s\n", options.program_name, audiofile, bse_error_blurb (error));
      _exit (1);
    }

  /* extract features */
  FeAudioSignal signal (dhandle);

  if (options.channel >= signal.n_channels())
    {
      printerr ("%s: bad channel %d, input file %s has %d channels\n",
                options.program_name, options.channel, audiofile, signal.n_channels());
      _exit (1);
    }

  for (list<Feature*>::const_iterator fi = feature_list.begin(); fi != feature_list.end(); fi++)
    if ((*fi)->extract_feature)
      (*fi)->compute (signal);

  /* print results */
  print_header (audiofile);
  for (list<Feature*>::const_iterator fi = feature_list.begin(); fi != feature_list.end(); fi++)
    {
      const Feature &feature = *(*fi);
      if (feature.extract_feature)
	{
	  fprintf (options.output_file, "# %s: %s\n", feature.option, feature.description);
	  feature.print_results();
	}
    }

  return ""; // no error
}

static CommandRegistry fextract_cmd (fextract_options, fextract_run, "fextract",
                                     fextract_blurb ("Audio feature extraction"));
