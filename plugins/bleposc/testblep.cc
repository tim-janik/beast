// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>
#include <bse/gslfft.hh>
#include <bse/bsemathsignal.hh>
#include <vector>
#include <complex>
#include "bleputils.hh"
#include "bleposc.hh"

using namespace Bse::BlepUtils;

using std::vector;
using std::string;
using std::complex;
using std::max;
using std::min;

static double *
complex_ptr (vector<complex<double>>& vec)
{
  return reinterpret_cast<double *> (&vec[0]);
}

static vector<double>
compute_fft_mag (Osc& o, size_t N)
{
  const bool IGNORE_DC = true;
  size_t OVER = 8;
  vector<complex<double>> in_ri (N * OVER), out_ri (N * OVER);

  if (IGNORE_DC)
    {
      for (size_t i = 0; i < 4 * N; i++)
        o.process_sample();
    }
  for (size_t i = 0; i < N; i++)
    {
      in_ri[i] = o.process_sample();
    }

  double win_norm = 0;
  for (size_t i = 0; i < N; i++)
    {
      const double win = window_kaiser ((i * 2.0 - N) / (N - 1), 4);

      in_ri[i] *= win;
      win_norm += win;
    }

  for (size_t i = 0; i < N; i++)
    {
      in_ri[i] *= 2.0 / win_norm;
    }

  gsl_power2_fftac (N * OVER, complex_ptr (in_ri), complex_ptr (out_ri));

  vector<double> out;
  for (auto ri : out_ri)
    out.push_back (std::abs (ri));

  return out;
}

struct SNR
{
  double snr;
  double freq;
};

static SNR fft_snr (Osc& o);

static void
print_fft (Osc& o, size_t N)
{
  vector<double> mag = compute_fft_mag (o, N);

  for (size_t i = 0; i <= mag.size() / 2; i++)
    {
      double d = i / double (mag.size()) * o.rate;
      printf ("%.17g %.17g\n", d, mag[i]);
    }
  SNR snr = fft_snr (o);
  printf ("#fft_snr: %f dB @ %f\n", snr.snr, snr.freq);
}

static SNR
fft_snr (Osc& o)
{
  vector<double> mag = compute_fft_mag (o, 8192);

  /* this is required because the width of the window is hardcoded to [-32:32] bins below */
  assert (mag.size() == 8192 * 8);

  double sig_max = 0;
  const double sub_freq = o.master_freq * 0.5;
  for (float freq = sub_freq; freq < o.rate; freq += sub_freq)
    {
      int pos = freq / o.rate * mag.size();
      for (int i = -32; i < 33; i++)
        {
          if (pos + i >= 0 && pos + i < int (mag.size()))
            {
              sig_max = std::max (mag[pos + i], sig_max);
              mag[pos + i] = 0;
            }
        }
    }

  SNR snr;
  double noise_max = 0;
  for (size_t i = 0; i <= mag.size() / 2; i++)
    {
      double freq = i / double (mag.size()) * o.rate;

      if (freq < 16000) // audible frequencies
        {
          double noise_mag = std::abs (mag[i]);
          if (noise_mag > noise_max)
            {
              noise_max = noise_mag;
              snr.freq = freq;
           }
        }
    }
  snr.snr = bse_db_from_factor (sig_max / noise_max, -200);
  return snr;
}

static void
auto_snr_test()
{
  GRand *rand = g_rand_new_with_seed (42);

  for (int i = 0; i < 1000; i++)
    {
      Osc o;
      o.rate = 48000;
      o.shape = g_rand_double_range (rand, -1, 1);
      o.master_freq = g_rand_double_range (rand, 200, 5000);
      o.freq = g_rand_double_range (rand, o.master_freq, 20000);
      o.pulse_width = g_rand_double_range (rand, 0.01, 0.99);
      o.sub_width = g_rand_double_range (rand, 0.01, 0.99);

      SNR snr = fft_snr (o);

      printf ("%f %f %f %f Y %f pw %f sw %f sfreq %f\n", snr.snr, o.shape, o.master_freq, o.freq, log2 (o.freq / o.master_freq) * 12, o.pulse_width, o.sub_width, snr.freq);
    }
}

static double
gettime()
{
  timeval tv;
  gettimeofday (&tv, 0);

  return tv.tv_sec + tv.tv_usec / 1000000.0;
}

double speed_test_x;

static void
speed_test (Osc& o)
{
  double time = 1e9;
  const int rate = 48000;
  const int len  = 32;

  for (int r = 0; r < 10; r++)
    {
      const double start = gettime();

      for (int i = 0; i < rate * len; i++)
        speed_test_x += o.process_sample();

      const double end = gettime();
      time = min (time, end - start);
    }

  printf ("%.7f voices   |  %.3f ns/sample\n", len / time, time * 1e9 / (rate * len));
}

static void
speed2_test()
{
  const int len  = 64;

  OscImpl o;

  float random_buffer[len];
  for (int n = 0; n < len; n++)
    random_buffer[n] = g_random_double_range (-1, 1);

  for (int subtest = 0; subtest < 7; subtest++)
    {
      o.rate = 48000;
      o.frequency_base = 440;
      o.freq_mod_octaves = 0.00001;
      o.shape_base = 0; // saw
      o.shape_mod = 0.00001;
      o.sync_base = 20;
      o.sync_mod = 0.00001;
      o.pulse_width_base = 0.5;
      o.pulse_width_mod  = 0.00001;

      float freq_buffer[len];
      for (int n = 0; n < len; n++)
        freq_buffer[n] = BSE_SIGNAL_FROM_FREQ (o.frequency_base);

      float *freq_in = nullptr;
      float *freq_mod = nullptr;
      float *shape_mod = nullptr;
      float *sub_mod = nullptr;
      float *sync_mod = nullptr;
      float *pulse_width_mod = nullptr;
      const char *label = nullptr;
      int unison = 1;

      switch (subtest)
        {
          case 0: label = "440y3";
                  break;
          case 1: label = "440y3+pw";
                  pulse_width_mod = random_buffer;
                  break;
          case 2: label = "440y3+y";
                  pulse_width_mod = random_buffer;
                  sync_mod = random_buffer;
                  break;
          case 3: label = "440y3+f";
                  freq_in = freq_buffer;
                  break;
          case 4: label = "440y3+all";
                  freq_in = freq_buffer;
                  freq_mod = random_buffer;
                  shape_mod = random_buffer;
                  sub_mod = random_buffer;
                  sync_mod = random_buffer;
                  pulse_width_mod = random_buffer;
                  break;
          case 5: label = "440";
                  o.sync_base = 0;
                  break;
          case 6: label = "440y3+u7";
                  unison = 7;
                  break;
        }
      if (unison > 1)
        o.set_unison (unison, 10.0, 1);

      double time = 1e9;

      const int blocks = 10000 / unison;

      for (int r = 0; r < 10; r++)
        {
          const double start = gettime();

          for (int i = 0; i < blocks; i++)
            {
              float lbuffer[len];
              float rbuffer[len];
              o.process_sample_stereo (lbuffer, rbuffer, len,
                                       freq_in,
                                       freq_mod,
                                       shape_mod,
                                       sub_mod,
                                       sync_mod,
                                       pulse_width_mod);

              for (int n = 0; n < len; n++)
                speed_test_x += lbuffer[n] + rbuffer[n];
            }

          const double end = gettime();
          time = min (time, end - start);
        }

      printf ("%d  %8.3f voices   |  %7.3f ns/sample    | %s\n", subtest, (len * blocks * unison) / o.rate / time, time * 1e9 / (len * blocks * unison), label);
    }
}

template<int FN> double
exp2_func (double d)
{
  switch (FN)
  {
    case 0: return pow (2, d);
    case 1: return exp (d * 0.693147180559945);
    case 2: return bse_approx2_exp2 (d);
    case 3: return bse_approx3_exp2 (d);
    case 4: return bse_approx4_exp2 (d);
    case 5: return bse_approx5_exp2 (d);
    case 6: return bse_approx6_exp2 (d);
    case 7: return bse_approx7_exp2 (d);
    case 8: return bse_approx8_exp2 (d);
    case 9: return bse_approx9_exp2 (d);
  }
}
template<int FN> std::string
exp2_label()
{
  switch (FN)
  {
    case 0: return "pow";
    case 1: return "exp";
    default: return Bse::string_format ("approx%d_exp2", FN);
  }
}

template<int FN> void
exp2_subtest()
{
  double time = 1e9;
  const int len  = 1'000'000;

  // speed
  for (int runs = 0; runs < 32; runs++)
    {
      const double start = gettime();
      for (int i = 0; i < len; i++)
        {
          double d = double (i) / len;
          speed_test_x += exp2_func<FN> (d);
        }
      const double end = gettime();
      time = min (time, end - start);
    }

  // accuracy
  double err = 0;
  for (double d = -5; d < 5; d += 0.0001)
    {
      err = max (err, fabs (exp2_func<FN> (d) - pow (2, d)) / pow (2, d));
    }
  printf ("%15s  %6.3f ns   err=%5.2g\n", exp2_label<FN>().c_str(), time * 1e9 / len, err);
}

static void
exp2_test()
{
  exp2_subtest<0>();
  exp2_subtest<1>();
  exp2_subtest<2>();
  exp2_subtest<3>();
  exp2_subtest<4>();
  exp2_subtest<5>();
  exp2_subtest<6>();
  exp2_subtest<7>();
  exp2_subtest<8>();
  exp2_subtest<9>();
}

static void
vnorm_test (Osc& o)
{
  const int rate = 48000;
  const int len  = 32;

  for (int n_voices = 1; n_voices < 16; n_voices++)
    {
      for (int stereo = 0; stereo < 5; stereo++)
        {
          o.set_unison (n_voices, 15, stereo / 4.0);

          double eleft = 0, eright = 0;
          for (int i = 0; i < rate * len; i++)
            {
              float vleft, vright;

              o.process_sample_stereo (&vleft, &vright);

              eleft  += vleft * vleft;
              eright += vright * vright;
            }
          printf ("%d %d %f %f\n", stereo, n_voices, 10 * log10 (eleft / (rate * len)), 10 * log10 (eright / (rate * len)));
        }
    }
}

static void
reset_test (Osc& o)
{
  for (int rpos = 0; rpos <= 10; rpos++)
    {
      double r = rpos / 10.;

      o.osc_impl.reset(); // clear future
      o.osc_impl.unison_voices[0].reset_master (r, OscImpl::State::A);

      double t = r * o.rate / o.master_freq;
      for (int l = 0; l < 4000; l++)
        printf ("%.17g %.17g #%d\n", t++, o.process_sample(), rpos);
    }
}

static void
plot_blep (float shape, float sync, float sub, float pulse, float sub_width)
{
  Osc o;
  o.rate = 48000;
  o.pulse_width = pulse / 100;
  o.shape = shape / 100;
  o.sub = sub / 100;
  o.sub_width = sub_width / 100;
  o.master_freq = 48;
  o.freq = pow (2, sync / 12) * o.master_freq; /* sync param: semitones */

  for (int i = 0; i < 4800; i++)
    {
      printf ("%.17g\n", o.process_sample());
    }
}

static void
dc_test (Osc& o)
{
  for (int sec = 0; sec < 4; sec++)
    {
      double dc = 0;
      for (int i = 0; i < o.rate; i++)
        dc += o.process_sample();
      dc /= o.rate;
      printf ("%d %.6f %.17g #dB\n", sec, dc, bse_db_from_factor (fabs (dc), -200));
    }
}

int
main (int argc, char **argv)
{
  Osc o;
  o.rate = 48000;
  o.pulse_width = 0.5;
  o.shape = 0; // saw
  o.freq = 440 * 3.1;
  o.master_freq = 440;

#if 0
  /* dc problem */
// 36.782045 0.563029 4174.116241 16051.472457 Y 23.317956 pw 0.793546 sw 0.818630 sfreq 5.859375
  o.rate = 48000;
  o.shape = 0.563029;
  o.freq = 16051.472457;
  o.master_freq = 4174.116241;
  o.pulse_width = 0.793546;
  o.sub_width = 0.818630;
#endif
#if 0
  /* sync bug */
  o.shape = 0;
  o.freq = 1150 * 1.97;
  o.master_freq = 1150;
#endif
#if 0
  /* sync bug */
  o.shape = 1;
  o.freq = 1150 * 1.48;
  o.master_freq = 1150;
#endif
#if 0
  /* pulse bug */
  o.shape = 1;
  o.freq = 17540;
  o.master_freq = 2440;
  o.pulse_width = 0.01;
#endif
#if 0
  // to get better speed test results:
  //
  // g++ -O2 -o tb testblep.cc bleposcdata.cc $(pkg-config --cflags --libs bse) -std=c++11
  //
  // and: run only speed test
  speed_test (o);
  return 0;
#endif

  if (argc == 7 && strcmp (argv[1], "plotblep") == 0)
    {
      plot_blep (atof (argv[2]), atof (argv[3]), atof (argv[4]), atof (argv[5]), atof (argv[6]));
      return 0;
    }

  if (argc == 2)
    {
      string test_name = argv[1];

      if (test_name == "vnorm")
        vnorm_test (o);
      else if (test_name == "fft")
        print_fft (o, 8192);
      else if (test_name == "speed")
        speed_test (o);
      else if (test_name == "speed2")
        speed2_test();
      else if (test_name == "snr")
        auto_snr_test();
      else if (test_name == "exp2")
        exp2_test();
      else if (test_name == "reset")
        reset_test (o);
      else if (test_name == "dc")
        dc_test (o);
      else
        {
          Bse::printerr ("%s: unsupported test type '%s', try vnorm, fft, speed, speed2, reset, exp2 or snr\n", argv[0], test_name.c_str());
          return 1;
        }
      return 0;
    }

  //for (o.freq = 20; o.freq < 20000; o.freq *= 1.00003)
  for (int i = 0; i < 48000; i++)
    {
      double sample = o.process_sample();

      printf ("%.17g\n", sample * 0.5);
    }
}
