/* BSE - Better Sound Engine
 * Copyright (C) 2006 Tim Janik
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
#include <bse/bsedefs.h>
// #define TEST_VERBOSE
#include <sfi/sfitests.h>
#include <bse/bsefilter.h>
#include <bse/bsemain.h>
#include <bse/gslfilter.h> // FIXME
#include <bse/bseglobals.h> // FIXME
#include "topconfig.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <complex>

typedef std::complex<double> Complex;
using namespace Birnet;
using std::max;
using std::min;

static inline double
sqr (register double a)
{
  return a * a;
}

static inline uint
complex_find_nearest (const BseComplex *zp,
                      uint              n_zps,
                      const Complex    *zps)
{
  const Complex z = Complex (zp->re, zp->im);
  uint j = 0;
  double last = sqr (real (z) - real (zps[j])) + sqr (imag (z) - imag (zps[j]));
  if (last == 0.0)
    return j;
  for (uint i = 1; i < n_zps; i++)
    {
      double dist2 = sqr (real (z) - real (zps[i])) + sqr (imag (z) - imag (zps[i]));
      if (dist2 < last)
        {
          last = dist2;
          j = i;
          if (last == 0.0)
            return j;
        }
    }
  return j;
}

static double
compare_zeros (uint              n_zeros,
               const BseComplex *czeros,
               const double     *rizeros)
{
  Complex *z2 = (Complex*) alloca (sizeof (Complex) * n_zeros);
  for (uint i = 0; i < n_zeros; i++)
    z2[i] = Complex (rizeros[i * 2], rizeros[i * 2 + 1]);
  double max_eps = 0;
  for (uint i = 0; i < n_zeros; i++)
    {
      uint j = complex_find_nearest (&czeros[i], n_zeros - i, z2);
      double reps = fabs (real (z2[j]) - czeros[i].re);
      double ieps = fabs (imag (z2[j]) - czeros[i].im);
      z2[j] = z2[n_zeros - i - 1]; // optimization of: swap (z2[last], z2[j]);
      max_eps = max (max_eps, max (reps, ieps));
    }
  return max_eps;
}

static double
to_db (double response)
{
  return response <= 0.0 ? -999.99 : max (BSE_DECIBEL20_FACTOR * log (response), -999.99);
}

static double
filter_zp_response (const BseIIRFilterDesign *fdes,
                    double                    freq)
{
  /* map freq onto unit circle, eval filter zeros and poles in z-plane */
  double omega = 2.0 * PI * freq / fdes->sampling_frequency;    // map freq onto unit circle circumference
  Complex z (cos (omega), sin (omega));                         // map freq onto unit circle (into z-plane): exp (j * omega)
  Complex num (1, 0), den (1, 0);
  for (uint i = 0; i < fdes->n_zeros; i++)
    {
      num = num * (z - Complex (fdes->zz[i].re, fdes->zz[i].im));
      if (fdes->zz[i].im != 0.0)
        num = num * (z - std::conj (Complex (fdes->zz[i].re, fdes->zz[i].im)));
    }
  for (uint i = 0; i < fdes->n_poles; i++)
    {
      den = den * (z - Complex (fdes->zp[i].re, fdes->zp[i].im));
      if (fdes->zp[i].im != 0.0)
        den = den * (z - std::conj (Complex (fdes->zp[i].re, fdes->zp[i].im)));
    }
  Complex r = fdes->gain * num / den;
  double freq_magnitude = abs (r);
  double freq_phase = arg (r);
  (void) freq_phase;
  return freq_magnitude;
}

bool
bse_iir_filter_dump_gnuplot (const BseIIRFilterDesign *fdes,
                             const char               *fname,
                             uint                      n_consts,
                             const double             *consts,
                             uint                      n_arrows,
                             const double             *arrows,
                             uint                      scan_points = 10000)
{
  String dname = String (fname) + ".dat";
  String gname = String (fname) + ".gp";
  FILE *df = fopen (dname.c_str(), "w");
  if (!df)
    return false;
  FILE *gf = fopen (gname.c_str(), "w");
  if (!gf)
    {
      fclose (df);
      return false;
    }

  const double nyquist = 0.5 * fdes->sampling_frequency;
  const double delta = nyquist / scan_points;
  for (double f = 0; f < nyquist; f += delta)
    fprintf (df, "%.17g %.17g %.17g\n", f,
             to_db (filter_zp_response (fdes, f)),
             to_db (filter_zp_response (fdes, f)));

  //gchar *nstr = bse_poly_str (fdes->order, (double*) fdes->zn, "z");
  //gchar *dstr = bse_poly_str (fdes->order, (double*) fdes->zd, "z");
  //fprintf (gf, "H(z)=%s/%s\n", nstr, dstr);
  //g_free (nstr);
  //g_free (dstr);

  fprintf (gf, "dB(x)=20*log(abs(x))/log(10)\n");
  fprintf (gf, "Z(x)=exp({0,-1}*x) # gnuplot variable x for H(z)\n");
  fprintf (gf, "set samples 10000  # increase accuracy\n");
  for (uint i = 0; i < n_arrows; i++)
    fprintf (gf, "set arrow %d from first %.17g+0,graph 0 to first %.17g+0,graph 1 nohead lt 0\n", 64 + i, arrows[i], arrows[i]);
  bool want_zero = false;
  for (uint i = 0; i < n_consts; i++)
    want_zero |= consts[i] == 0.0;
  fprintf (gf, "plot [][:] %s '%s' using ($1):($3) with lines, '%s' using ($1):($2) with lines",
           want_zero ? "0, " : "",
           dname.c_str(), dname.c_str());
  for (uint i = 0; i < n_consts; i++)
    if (consts[i] != 0.0)
      fprintf (gf, ", %.17g", consts[i]);
  fprintf (gf, "\n");
  fprintf (gf, "pause -1\n");
  fclose (gf);
  fclose (df);
  return true;
}

static void
noexit_dump_iir_filter_gnuplot (const BseIIRFilterRequest *fireq,
                                const BseIIRFilterDesign  *fdes,
                                const char                *fname,
                                double                     passband_ripple_db = NAN,
                                double                     passband_edge = NAN,
                                double                     passband_edge2 = NAN,
                                double                     stopband_db = NAN,
                                double                     stopband_edge = NAN,
                                double                     stopband_edge2 = NAN)
{
  double consts[64];
  double arrows[64];
  uint n_consts = 0, n_arrows = 0;
  if (std::isfinite (passband_ripple_db))
    consts[n_consts++] = passband_ripple_db;
  if (std::isfinite (stopband_db))
    consts[n_consts++] = stopband_db;
  if (std::isfinite (passband_edge))
    arrows[n_arrows++] = passband_edge;
  if (std::isfinite (passband_edge2))
    arrows[n_arrows++] = passband_edge2;
  if (std::isfinite (stopband_edge))
    arrows[n_arrows++] = stopband_edge;
  if (std::isfinite (stopband_edge2))
    arrows[n_arrows++] = stopband_edge2;
  consts[n_consts++] = 0.0;
  bool success = bse_iir_filter_dump_gnuplot (fdes, fname, n_consts, consts, n_arrows, arrows, 55555);
  BIRNET_ASSERT (success == true);
  g_printerr ("Filter: %s\n", bse_iir_filter_request_string (fireq));
  g_printerr ("Design: %s\n", bse_iir_filter_design_string (fdes));
  g_printerr ("GnuplotDump: wrote %s.gp and %s.dat use: gnuplot %s.gp\n", fname, fname, fname);
}

static void
exit_with_iir_filter_gnuplot (const BseIIRFilterRequest *fireq,
                              const BseIIRFilterDesign  *fdes,
                              const char                *fname,
                              double                     passband_ripple_db = NAN,
                              double                     passband_edge = NAN,
                              double                     passband_edge2 = NAN,
                              double                     stopband_db = NAN,
                              double                     stopband_edge = NAN,
                              double                     stopband_edge2 = NAN)
{
  noexit_dump_iir_filter_gnuplot (fireq, fdes, fname, passband_ripple_db, passband_edge, passband_edge2, stopband_db, stopband_edge, stopband_edge2);
  exit (0);
}

static double
max_band_damping_zp (const BseIIRFilterDesign *fdes,
                     double                    start_freq,
                     double                    end_freq)
{
  const double n_sample_points = 3333;
  const double n_random_points = 999;
  const double delta = max (1e-13, fabs (end_freq - start_freq) / n_sample_points);
  double eps = +INFINITY;
  for (double f = start_freq; f < end_freq; f += delta)
    eps = min (eps, filter_zp_response (fdes, f));
  eps = min (eps, filter_zp_response (fdes, end_freq));
  for (uint i = 0; i < n_random_points; i++)
    eps = min (eps, filter_zp_response (fdes, g_random_double_range (start_freq, end_freq)));
  if (0)
    for (double f = start_freq; f < end_freq; f += delta * 30)
      g_printerr ("PassBandZPDB: %f: %f\n", f, to_db (filter_zp_response (fdes, f)));
  return to_db (eps);
}

static double
min_band_damping_zp (const BseIIRFilterDesign *fdes,
                     double                    start_freq,
                     double                    end_freq)
{
  const double n_sample_points = 3333;
  const double n_random_points = 999;
  const double delta = max (1e-13, fabs (end_freq - start_freq) / n_sample_points);
  double eps = 0;
  for (double f = start_freq; f < end_freq; f += delta)
    eps = max (eps, filter_zp_response (fdes, f));
  eps = max (eps, filter_zp_response (fdes, end_freq));
  for (uint i = 0; i < n_random_points; i++)
    eps = max (eps, filter_zp_response (fdes, g_random_double_range (start_freq, end_freq)));
  if (0)
    for (double f = start_freq; f < end_freq; f += delta * 30)
      g_printerr ("PassBandZPDB: %f: %f\n", f, to_db (filter_zp_response (fdes, f)));
  return to_db (eps);
}

static double
max_band_damping (const BseIIRFilterDesign *fdes,
                  double                    start_freq,
                  double                    end_freq)
{
  // double res1 = max_band_damping_ztrans (fdes, start_freq, end_freq);
  return max_band_damping_zp (fdes, MIN (start_freq, end_freq), MAX (start_freq, end_freq));
}

static double
min_band_damping (const BseIIRFilterDesign *fdes,
                  double                    start_freq,
                  double                    end_freq)
{
  // double res1 = min_band_damping_ztrans (fdes, start_freq, end_freq);
  return min_band_damping_zp (fdes, MIN (start_freq, end_freq), MAX (start_freq, end_freq));
}

static void
print_filter_on_abort (void *data)
{
  void **adata = (void**) data;
  const BseIIRFilterRequest *req = (BseIIRFilterRequest*) adata[0];
  const BseIIRFilterDesign *fdes = (BseIIRFilterDesign*) adata[1];
  noexit_dump_iir_filter_gnuplot (req, fdes, "tmpfilter",
                                  -fabs(req->passband_ripple_db), req->passband_edge, req->passband_edge2,
                                  req->stopband_db != 0 ? req->stopband_db : NAN, req->stopband_edge, NAN);
}

static void
butterwoth_tests ()
{
  TSTART ("Butterworth");
  bool success;
  const double gaineps = 1e-7;
  BseIIRFilterDesign fdes;
  BseIIRFilterRequest req = { BseIIRFilterKind (0), };
  req.kind = BSE_IIR_FILTER_BUTTERWORTH;
  void *abort_data[2];
  abort_data[0] = (void*) &req;
  abort_data[1] = &fdes;
  TABORT_HANDLER (print_filter_on_abort, abort_data);
  TOK();

  {
    req.type = BSE_IIR_FILTER_LOW_PASS;
    req.order = 8;
    req.sampling_frequency = 10000;
    req.passband_edge = 2000;
    success = bse_iir_filter_design (&req, &fdes);
    if (0)
      {
        g_printerr ("Filter: %s\n", bse_iir_filter_request_string (&req));
        g_printerr ("Design: %s\n", bse_iir_filter_design_string (&fdes));
      }
    TASSERT (success == true);
    TASSERT_CMP (min_band_damping (&fdes, 0, 2000), <, gaineps);
    TASSERT_CMP (max_band_damping (&fdes, 0, 2000), >, -3.0103);
    TASSERT_CMP (min_band_damping (&fdes, 3500, 5000), <, -68);
    if (0)
      exit_with_iir_filter_gnuplot (&req, &fdes, "tmpfilter", -3.0103, 2000, NAN, -68, 3500);
  }
  
  {
    req.type = BSE_IIR_FILTER_HIGH_PASS;
    req.order = 7;
    req.sampling_frequency = 10000;
    req.passband_edge = 2000;
    success = bse_iir_filter_design (&req, &fdes);
    TASSERT (success == true);
    TASSERT_CMP (min_band_damping (&fdes, 2000, 5000), <, gaineps);
    TASSERT_CMP (max_band_damping (&fdes, 2000, 5000), >, -3.0103);
    TASSERT_CMP (min_band_damping (&fdes, 0,     600), <, -80);
    if (0)
      exit_with_iir_filter_gnuplot (&req, &fdes, "tmpfilter", -3.0103, 2000, NAN, -80, 600);
  }

  {
    req.type = BSE_IIR_FILTER_BAND_PASS;
    req.order = 9;
    req.sampling_frequency = 10000;
    req.passband_edge = 1500;
    req.passband_edge2 = 3500;
    success = bse_iir_filter_design (&req, &fdes);
    TASSERT (success == true);
    TASSERT_CMP (min_band_damping (&fdes, 1500, 3500), <, gaineps);
    TASSERT_CMP (max_band_damping (&fdes, 1500, 3500), >, -3.0103);
    TASSERT_CMP (min_band_damping (&fdes, 0,    1000), <, -49.5);
    TASSERT_CMP (min_band_damping (&fdes, 4000, 5000), <, -49.5);
    if (0)
      exit_with_iir_filter_gnuplot (&req, &fdes, "tmpfilter", -3.0103, 1500, 3500, -49.5, 1000, 4000);
  }
  
  {
    req.type = BSE_IIR_FILTER_BAND_STOP;
    req.order = 14;
    req.sampling_frequency = 10000;
    req.passband_edge = 1000;
    req.passband_edge2 = 4000;
    success = bse_iir_filter_design (&req, &fdes);
    TASSERT (success == true);
    TASSERT_CMP (min_band_damping (&fdes, 0,    1000), <, gaineps);
    TASSERT_CMP (max_band_damping (&fdes, 0,    1000), >, -3.0103);
    TASSERT_CMP (min_band_damping (&fdes, 1500, 3500), <, -77);
    TASSERT_CMP (max_band_damping (&fdes, 4000, 5000), >, -3.0103);
    if (0)
      exit_with_iir_filter_gnuplot (&req, &fdes, "tmpfilter", -3.0103, 1000, 4000, -77, 1500, 3500);
  }

  TOK();
  TDONE();
}

static void
chebychev1_tests ()
{
  bool success;
  TSTART ("Chebyshev1");
  BseIIRFilterDesign fdes;
  BseIIRFilterRequest req = { BseIIRFilterKind (0), };
  req.kind = BSE_IIR_FILTER_CHEBYSHEV1;
  void *abort_data[2];
  abort_data[0] = (void*) &req;
  abort_data[1] = &fdes;
  TABORT_HANDLER (print_filter_on_abort, abort_data);
  const double gaineps = 1e-7;
  TOK();

  {
    req.type = BSE_IIR_FILTER_LOW_PASS;
    req.order = 8;
    req.sampling_frequency = 20000;
    req.passband_ripple_db = 1.5500;
    req.passband_edge = 3000;
    success = bse_iir_filter_design (&req, &fdes);
    TASSERT (success == true);
    TASSERT_CMP (min_band_damping (&fdes, 0, 3000), <, gaineps);
    TASSERT_CMP (max_band_damping (&fdes, 0, 3000), >, -1.5501);
    TASSERT_CMP (min_band_damping (&fdes, 5000, 10000), <, -80);
    if (0)
      exit_with_iir_filter_gnuplot (&req, &fdes, "tmpfilter", -1.55, 3000, NAN, -80, 5000);
  }

  {
    req.type = BSE_IIR_FILTER_HIGH_PASS;
    req.order = 7;
    req.sampling_frequency = 10000;
    req.passband_ripple_db = 0.1;
    req.passband_edge = 600;
    success = bse_iir_filter_design (&req, &fdes);
    TASSERT (success == true);
    TASSERT_CMP (min_band_damping (&fdes, 600, 5000), <, gaineps);
    TASSERT_CMP (max_band_damping (&fdes, 600, 5000), >, -0.101);
    TASSERT_CMP (min_band_damping (&fdes, 0,    250), <, -70);
    if (0)
      exit_with_iir_filter_gnuplot (&req, &fdes, "tmpfilter", -0.1, 600, NAN, -70, 250);
  }

  {
    req.type = BSE_IIR_FILTER_BAND_PASS;
    req.order = 10;
    req.sampling_frequency = 30000;
    req.passband_ripple_db = 1.8;
    req.passband_edge = 3500;
    req.passband_edge2 = 9500;
    success = bse_iir_filter_design (&req, &fdes);
    TASSERT (success == true);
    TASSERT_CMP (min_band_damping (&fdes,  3500,  9500), <, gaineps);
    TASSERT_CMP (max_band_damping (&fdes,  3500,  9500), >, -1.801);
    TASSERT_CMP (min_band_damping (&fdes,  0,     3000), <, -55);
    TASSERT_CMP (min_band_damping (&fdes, 10200, 15000), <, -55);
    if (0)
      exit_with_iir_filter_gnuplot (&req, &fdes, "tmpfilter", -1.801, 3500, 9500, -55, 3000, 10200);
  }
  
  {
    req.type = BSE_IIR_FILTER_BAND_STOP;
    req.order = 13;
    req.sampling_frequency = 40000;
    req.passband_ripple_db = 1;
    req.passband_edge = 8000;
    req.passband_edge2 = 12000;
    success = bse_iir_filter_design (&req, &fdes);
    TASSERT (success == true);
    TASSERT_CMP (min_band_damping (&fdes, 0,      8000), <, gaineps);
    TASSERT_CMP (max_band_damping (&fdes, 0,      8000), >, -1.001);
    TASSERT_CMP (min_band_damping (&fdes,  8500, 11500), <, -78);
    TASSERT_CMP (max_band_damping (&fdes, 12000, 20000), >, -1.001);
    if (0)
      exit_with_iir_filter_gnuplot (&req, &fdes, "tmpfilter", -1.001, 8000, 12000, -78, 8500, 11500);
  }

  TOK();
  TDONE();
}

typedef struct {
  const BseIIRFilterRequest *filter_request;
  double                     gain;
  uint                       n_zeros;
  const double              *zeros;
  uint                       n_poles;
  const double              *poles;
  const double              *filter_coefficients;
  uint                       n_filter_coefficients;
} FilterSetup;

static void generic_filter_tests   (const char                *test_name,
                                    const uint                 n_filters,
                                    const FilterSetup         *filters,
                                    uint                       tick_count = 1,
                                    uint                       skip_count = 0);

static void
test_problem_candidates ()
{
  FilterSetup filters[1000] = { { 0, } };
  uint index = 0;

  { // ellf gain is too high
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              20,
      /* sampling_frequency = */ 1073741824.0000,
      /* passband_ripple_db = */ 1.3719436899999999,
      /* passband_edge = */      483183820.79999995,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -26,
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.50971316315766546e-02;
    index++;
  }
  { // ellf gain is too high
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              19,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 0.01,
      /* passband_edge = */      1333.2,
      /* passband_edge2 = */     999.89999999999998,
      /* stopband_edge = */      0,
      /* stopband_db = */        -120,
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.21552945102360275e-05;
    index++;
  }
  { // ellf gain is too high
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              20,
      /* sampling_frequency = */ 8000.0000,
      /* passband_ripple_db = */ 0.01,
      /* passband_edge = */      3200,
      /* passband_edge2 = */     2400,
      /* stopband_edge = */      0,
      /* stopband_db = */        -26,
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.12649838606861938e-01;
    index++;
  }

  if (0) { // FIXME: bse gain = 4.3037825362077964
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              36,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 1.3719436899999999,
      /* passband_edge = */      943.23900000000003,
      /* passband_edge2 = */     499.94999999999999,
      /* stopband_edge = */      0,
      /* stopband_db = */        -26,
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = -3.83263619423376167e-01;
    index++;
  }

  /* and test them */
  generic_filter_tests ("Problem Filters", index, filters);
}

static void
random_filter_tests ()
{
  BseIIRFilterRequest frequest = { BseIIRFilterKind (0), };
  const double sampling_frequencies[/* prime! */] = {
    100, 1000, 2000, 3333, 4000, 8000, 12000, 16000, 20050, 32000, 44100, 48000, 56000, 64000,
    72000, 88000, 96000, 128000, 192000, 256000, 512000, 1024 * 1024, 1024 * 1024 * 1024,
  };
  const int n_sampling_frequencies = sizeof (sampling_frequencies) / sizeof (sampling_frequencies[0]);
  const int filter_orders[] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 19, 30, 31, 32,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* used for random orders */
  };
  int n_orders = sizeof (filter_orders) / sizeof (filter_orders[0]);
  int oix, max_order_index = 0;
  for (oix = 0; oix < n_orders; oix++)
    max_order_index = filter_orders[oix] ? oix : max_order_index;
  int rand_order_width = 64 - filter_orders[max_order_index] + 1;
  double pbe1;
  FilterSetup filters[100000] = { { 0, }, };
  uint filter_index, skip_count = 6;

  if (sfi_init_settings().test_quick)
    n_orders = 9;

#define MAKE_FILTER(frequest, filter_type) do                                   \
  {                                                                             \
    if (!filters[filter_index].filter_request)                                  \
      filters[filter_index].filter_request = g_newa (BseIIRFilterRequest, 1);   \
    BseIIRFilterRequest *rqcopy;                                                \
    rqcopy = (BseIIRFilterRequest*) filters[filter_index].filter_request;       \
    *rqcopy = frequest;                                                         \
    rqcopy->type = filter_type;                                                 \
    filter_index++;                                                             \
  } while (0)

  /* generate filter requirements */
  filter_index = 0;
  frequest.kind = BSE_IIR_FILTER_BUTTERWORTH;
  for (oix = 0; oix < n_orders; oix++)
    for (pbe1 = 0.15; pbe1 <= 0.46; pbe1 += 0.15)
      {
        /* low/high */
        frequest.order = oix <= max_order_index ? filter_orders[oix] : filter_orders[max_order_index] + g_random_int() % rand_order_width;
        frequest.sampling_frequency = sampling_frequencies[g_random_int() % n_sampling_frequencies];
        frequest.passband_edge = pbe1 * frequest.sampling_frequency;
        MAKE_FILTER (frequest, BSE_IIR_FILTER_LOW_PASS);
        MAKE_FILTER (frequest, BSE_IIR_FILTER_HIGH_PASS);
        /* band filters */
        if (pbe1 + 0.1 < 0.48)
          {
            frequest.order = (frequest.order + 1) / 2;
            frequest.passband_edge2 = g_random_double_range (pbe1 + 0.1, 0.48) * frequest.sampling_frequency;
            MAKE_FILTER (frequest, BSE_IIR_FILTER_BAND_PASS);
            MAKE_FILTER (frequest, BSE_IIR_FILTER_BAND_STOP);
          }
      }
  /* design and test filters */
  generic_filter_tests ("Random Butterworth", filter_index, filters, skip_count);

  /* generate filter requirements */
  filter_index = 0;
  frequest.kind = BSE_IIR_FILTER_CHEBYSHEV1;
  for (oix = 0; oix < n_orders; oix++)
    for (pbe1 = 0.15; pbe1 <= 0.46; pbe1 += 0.15)
      {
        /* low/high */
        frequest.order = oix <= max_order_index ? filter_orders[oix] : filter_orders[max_order_index] + g_random_int() % rand_order_width;
        frequest.sampling_frequency = sampling_frequencies[g_random_int() % n_sampling_frequencies];
        frequest.passband_edge = pbe1 * frequest.sampling_frequency;
        frequest.passband_ripple_db = g_random_double_range (0.0001, 0.001) * pow (9, g_random_int() % 5); /* 0.0001 .. 6.6 */
        MAKE_FILTER (frequest, BSE_IIR_FILTER_LOW_PASS);
        frequest.passband_ripple_db = g_random_double_range (0.0001, 0.001) * pow (9, g_random_int() % 5); /* 0.0001 .. 6.6 */
        MAKE_FILTER (frequest, BSE_IIR_FILTER_HIGH_PASS);
        /* band filters */
        if (pbe1 + 0.1 < 0.48)
          {
            frequest.order = (frequest.order + 1) / 2;
            frequest.passband_edge2 = g_random_double_range (pbe1 + 0.1, 0.48) * frequest.sampling_frequency;
            frequest.passband_ripple_db = g_random_double_range (0.0001, 0.001) * pow (9, g_random_int() % 5); /* 0.0001 .. 6.6 */
            MAKE_FILTER (frequest, BSE_IIR_FILTER_BAND_PASS);
            frequest.passband_ripple_db = g_random_double_range (0.0001, 0.001) * pow (9, g_random_int() % 5); /* 0.0001 .. 6.6 */
            MAKE_FILTER (frequest, BSE_IIR_FILTER_BAND_STOP);
          }
      }
  /* design and test filters */
  generic_filter_tests ("Random Chebyshev1", filter_index, filters, skip_count);

  /* generate filter requirements */
  filter_index = 0;
  frequest.kind = BSE_IIR_FILTER_ELLIPTIC;
  frequest.stopband_edge = 0;
  for (oix = 0; oix < n_orders; oix++)
    for (pbe1 = 0.15; pbe1 <= 0.46; pbe1 += 0.15)
      {
        /* low/high */
        frequest.order = oix <= max_order_index ? filter_orders[oix] : filter_orders[max_order_index] + g_random_int() % rand_order_width;
        frequest.sampling_frequency = sampling_frequencies[g_random_int() % n_sampling_frequencies];
        frequest.passband_edge = pbe1 * frequest.sampling_frequency;
        frequest.passband_ripple_db = g_random_double_range (0.0001, 0.001) * pow (9, g_random_int() % 5); /* 0.0001 .. 6.6 */
        frequest.stopband_db = g_random_double_range (-9, -150) - 2 * frequest.order;
        MAKE_FILTER (frequest, BSE_IIR_FILTER_LOW_PASS);
        frequest.passband_ripple_db = g_random_double_range (0.0001, 0.001) * pow (9, g_random_int() % 5); /* 0.0001 .. 6.6 */
        frequest.stopband_db = g_random_double_range (-9, -150) - 2 * frequest.order;
        MAKE_FILTER (frequest, BSE_IIR_FILTER_HIGH_PASS);
        /* band filters */
        if (pbe1 + 0.1 < 0.48)
          {
            frequest.order = (frequest.order + 1) / 2;
            frequest.passband_edge2 = g_random_double_range (pbe1 + 0.1, 0.48) * frequest.sampling_frequency;
            frequest.passband_ripple_db = g_random_double_range (0.0001, 0.001) * pow (9, g_random_int() % 5); /* 0.0001 .. 6.6 */
            frequest.stopband_db = g_random_double_range (-9, -150) - 2 * frequest.order;
            MAKE_FILTER (frequest, BSE_IIR_FILTER_BAND_PASS);
            frequest.passband_ripple_db = g_random_double_range (0.0001, 0.001) * pow (9, g_random_int() % 5); /* 0.0001 .. 6.6 */
            frequest.stopband_db = g_random_double_range (-9, -150) - 2 * frequest.order;
            MAKE_FILTER (frequest, BSE_IIR_FILTER_BAND_STOP);
          }
      }
  /* design and test filters */
  generic_filter_tests ("Random Elliptic (dB)", filter_index, filters, skip_count);
  
  /* generate filter requirements */
  filter_index = 0;
  frequest.kind = BSE_IIR_FILTER_ELLIPTIC;
  frequest.stopband_db = 0;
  for (oix = 0; oix < n_orders; oix++)
    for (pbe1 = 0.1; pbe1 <= 0.46; pbe1 += 0.15)
      {
        /* low/high */
        frequest.order = oix <= max_order_index ? filter_orders[oix] : filter_orders[max_order_index] + g_random_int() % rand_order_width;
        frequest.sampling_frequency = sampling_frequencies[g_random_int() % n_sampling_frequencies];
        frequest.passband_edge = pbe1 * frequest.sampling_frequency;
        frequest.passband_ripple_db = g_random_double_range (0.0001, 0.001) * pow (9, g_random_int() % 5); /* 0.0001 .. 6.6 */
        frequest.stopband_edge = g_random_double_range (pbe1 + 0.01, 0.49) * frequest.sampling_frequency;
        MAKE_FILTER (frequest, BSE_IIR_FILTER_LOW_PASS);
        frequest.passband_ripple_db = g_random_double_range (0.0001, 0.001) * pow (9, g_random_int() % 5); /* 0.0001 .. 6.6 */
        frequest.stopband_edge = g_random_double_range (0.01, pbe1 - 0.01) * frequest.sampling_frequency;
        MAKE_FILTER (frequest, BSE_IIR_FILTER_HIGH_PASS);
        /* band filters */
        if (pbe1 + 0.1 < 0.48)
          {
            frequest.order = (frequest.order + 1) / 2;
            frequest.passband_edge2 = g_random_double_range (pbe1 + 0.1, 0.48) * frequest.sampling_frequency;
            frequest.passband_ripple_db = g_random_double_range (0.0001, 0.001) * pow (9, g_random_int() % 5); /* 0.0001 .. 6.6 */
            frequest.stopband_edge = g_random_double_range (MAX (frequest.passband_edge, frequest.passband_edge2) + 0.01, 0.491 * frequest.sampling_frequency);
            MAKE_FILTER (frequest, BSE_IIR_FILTER_BAND_PASS);
            frequest.passband_ripple_db = g_random_double_range (0.0001, 0.001) * pow (9, g_random_int() % 5); /* 0.0001 .. 6.6 */
            double pwidth = fabs (frequest.passband_edge2 - frequest.passband_edge);
            frequest.stopband_edge = g_random_double_range (MIN (frequest.passband_edge, frequest.passband_edge2) + pwidth * 0.01,
                                                            MAX (frequest.passband_edge, frequest.passband_edge2) - pwidth * 0.01);
            MAKE_FILTER (frequest, BSE_IIR_FILTER_BAND_STOP);
          }
      }
  /* design and test filters */
  generic_filter_tests ("Random Elliptic (Hz)", filter_index, filters, skip_count);

#undef MAKE_FILTER
}

static void
test_filter_catalog ()
{
  FilterSetup filters[100000] = { { 0, } };
  uint index = 0;

  /* include predesigned filters */
#include "filtercatalog.cc"

  uint skip_count = 0, tick_count = 3;
  if (sfi_init_settings().test_quick)
    {
      tick_count = 1;
      skip_count = 17;
    }

  /* test predesigned filters */
  generic_filter_tests ("Filter Catalog", index, filters, tick_count, skip_count);
}

static void
generic_filter_tests (const char        *test_name,
                      const uint         n_filters,
                      const FilterSetup *filters,
                      const uint         tick_count,
                      uint               skip_count)
{
  uint i;
  const double coefficients_epsilon = 1e-7;
  const double max_gain = 0.01; // maximum gain in passband
  TSTART ("%s", test_name);
  void *abort_data[2];
  TABORT_HANDLER (print_filter_on_abort, abort_data);
  skip_count = MAX (1, skip_count);
  for (i = 0; i < n_filters; i += 1 + g_random_int() % skip_count)
    {
      const BseIIRFilterRequest *req = filters[i].filter_request;
      BseIIRFilterDesign fdes;
      abort_data[0] = (void*) req;
      abort_data[1] = &fdes;
      bool success = bse_iir_filter_design (req, &fdes);
      TCHECK (success == true);
      if (filters[i].zeros)
        TCHECK_CMP (filters[i].n_zeros, ==, fdes.n_zeros);
      if (filters[i].poles)
        TCHECK_CMP (filters[i].n_poles, ==, fdes.n_poles);
      double zeps = filters[i].zeros ? compare_zeros (fdes.n_zeros, fdes.zz, filters[i].zeros) : 0;
      double peps = filters[i].poles ? compare_zeros (fdes.n_poles, fdes.zp, filters[i].poles) : 0;
      TCHECK_CMP (zeps, <, coefficients_epsilon);
      TCHECK_CMP (peps, <, coefficients_epsilon);
      /* broad gain check */
      TCHECK_CMP (min_band_damping (&fdes, 0, 0.5 * req->sampling_frequency), <, max_gain);
      /* finer gain checks */
      double passband_ripple_db = req->kind == BSE_IIR_FILTER_BUTTERWORTH ? -3.010299956639811952 : -req->passband_ripple_db;
      double stopband_db = req->kind == BSE_IIR_FILTER_ELLIPTIC && req->stopband_db < 0.0 ? req->stopband_db : 2.02 * passband_ripple_db;
      stopband_db = MAX (stopband_db, passband_ripple_db * (req->order - 1)); /* constrain insane stopband_db requirements */
      if (req->type == BSE_IIR_FILTER_LOW_PASS || req->type == BSE_IIR_FILTER_BAND_STOP)
        {
          double min_pass_damping = min_band_damping (&fdes, 0, req->passband_edge);
          TCHECK_CMP (min_pass_damping, <, max_gain);
          TCHECK_CMP (min_pass_damping, >, -0.01);
          double max_pass_damping = max_band_damping (&fdes, 0, req->passband_edge);
          TCHECK_CMP (max_pass_damping, >, passband_ripple_db - 0.01);
          TCHECK_CMP (max_pass_damping, <, passband_ripple_db * 0.9);
        }
      if (req->type == BSE_IIR_FILTER_LOW_PASS)
        {
          double max_stop_damping = max_band_damping (&fdes, 0.5 * req->sampling_frequency, 0.5 * req->sampling_frequency);
          TCHECK_CMP (max_stop_damping, <, stopband_db * 0.98);
        }
      if (req->type == BSE_IIR_FILTER_HIGH_PASS)
        {
          double min_pass_damping = min_band_damping (&fdes, req->passband_edge, 0.5 * req->sampling_frequency);
          TCHECK_CMP (min_pass_damping, <, max_gain);
          TCHECK_CMP (min_pass_damping, >, -0.01);
          double max_pass_damping = max_band_damping (&fdes, req->passband_edge, 0.5 * req->sampling_frequency);
          TCHECK_CMP (max_pass_damping, >, passband_ripple_db - 0.01);
          TCHECK_CMP (max_pass_damping, <, passband_ripple_db * 0.9);
          double max_stop_damping = max_band_damping (&fdes, 0, 0);
          TCHECK_CMP (max_stop_damping, <, stopband_db * 0.98);
        }
      if (req->type == BSE_IIR_FILTER_BAND_PASS)
        {
          double min_pass_damping = min_band_damping (&fdes, req->passband_edge, req->passband_edge2);
          TCHECK_CMP (min_pass_damping, <, max_gain);
          TCHECK_CMP (min_pass_damping, >, -0.01);
          double max_pass_damping = max_band_damping (&fdes, req->passband_edge, req->passband_edge2);
          TCHECK_CMP (max_pass_damping, >, passband_ripple_db - 0.01);
          TCHECK_CMP (max_pass_damping, <, passband_ripple_db * 0.9);
          double max_stop_damping0 = max_band_damping (&fdes, 0, 0);
          TCHECK_CMP (max_stop_damping0, <, stopband_db * 0.98);
          double max_stop_damping2 = max_band_damping (&fdes, 0.5 * req->sampling_frequency, 0.5 * req->sampling_frequency);
          TCHECK_CMP (max_stop_damping2, <, stopband_db * 0.98);
        }
      if (req->type == BSE_IIR_FILTER_BAND_STOP)
        {
          double min_pass_damping0 = min_band_damping (&fdes, 0, req->passband_edge);
          TCHECK_CMP (min_pass_damping0, <, max_gain);
          TCHECK_CMP (min_pass_damping0, >, -0.01);
          double max_pass_damping0 = max_band_damping (&fdes, 0, req->passband_edge);
          TCHECK_CMP (max_pass_damping0, >, passband_ripple_db - 0.01);
          TCHECK_CMP (max_pass_damping0, <, passband_ripple_db * 0.9);
          double min_pass_damping2 = min_band_damping (&fdes, req->passband_edge2, 0.5 * req->sampling_frequency);
          TCHECK_CMP (min_pass_damping2, <, max_gain);
          TCHECK_CMP (min_pass_damping2, >, -0.01);
          double max_pass_damping2 = max_band_damping (&fdes, req->passband_edge2, 0.5 * req->sampling_frequency);
          TCHECK_CMP (max_pass_damping2, >, passband_ripple_db - 0.01);
          TCHECK_CMP (max_pass_damping2, <, passband_ripple_db * 0.9);
          double max_stop_damping = max_band_damping (&fdes, fdes.center_frequency, fdes.center_frequency);
          TCHECK_CMP (max_stop_damping, <, stopband_db * 0.98);
        }
      if (i % tick_count == 0)
        TOK();
    }
  TDONE();
}

int
main (int    argc,
      char **argv)
{
  bse_init_test (&argc, &argv, NULL);
  butterwoth_tests ();
  chebychev1_tests ();
  test_problem_candidates ();
  test_filter_catalog();
  random_filter_tests ();
  return 0;
}
