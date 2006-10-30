/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2006 Tim Janik
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
#include <bse/bsedefs.h>
// #define TEST_VERBOSE
#include <sfi/sfitests.h>
#include <bse/bsefilter.h>
#include <bse/bsemain.h>
#include "topconfig.h"
#include <math.h>

using namespace Birnet;
using std::max;
using std::min;

static double
compare_coefficients (const BseIIRFilterDesign *fdes,
                      uint                      n_dncoeffs,
                      const double             *dncoeffs)
{
  TASSERT (n_dncoeffs / 2 == fdes->order + 1);
  double eps = 0;
  for (uint i = 0; i <= fdes->order; i++)
    {
      eps = max (eps, fabs (fdes->zn[i] - dncoeffs[i * 2 + 1]));
      eps = max (eps, fabs (fdes->zd[i] - dncoeffs[i * 2 + 0]));
    }
  return eps;
}

static double
to_db (double response)
{
  const double decibell20 = 8.6858896380650365530225783783321; /* 20.0 / ln (10.0) */
  return response <= 0.0 ? -999.99 : decibell20 * log (response);
}

static double
filter_ztrans_response (const BseIIRFilterDesign *fdes,
                        double                    freq)
{
  /* map freq onto unit circle, eval filter transfer func in z-plane */
  double omega = 2.0 * PI * freq / fdes->sampling_frequency;    // map freq onto unit circle circumference
  Complex z (cos (omega), sin (omega));                         // map freq onto unit circle (into z-plane): exp (j * omega)
  uint oi = fdes->order;
  Complex num = fdes->zn[oi], den = fdes->zd[oi];
  while (oi--)
    {
      num = num * z + fdes->zn[oi];
      den = den * z + fdes->zd[oi];
    }
  Complex r = num / den;
  double freq_magnitude = abs (r);
  double freq_phase = arg (r);
  (void) freq_phase;
  return freq_magnitude;
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
      num = num * (z - fdes->zz[i]);
      if (imag (fdes->zz[i]) != 0.0)
        num = num * (z - conj (fdes->zz[i]));
    }
  for (uint i = 0; i < fdes->n_poles; i++)
    {
      den = den * (z - fdes->zp[i]);
      if (imag (fdes->zp[i]) != 0.0)
        den = den * (z - conj (fdes->zp[i]));
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
             to_db (filter_ztrans_response (fdes, f)));

  gchar *nstr = bse_poly_str (fdes->order, (double*) fdes->zn, "z"); // FIXME
  gchar *dstr = bse_poly_str (fdes->order, (double*) fdes->zd, "z");
  fprintf (gf, "H(z)=%s/%s\n", nstr, dstr);
  g_free (nstr);
  g_free (dstr);

  fprintf (gf, "dB(x)=20*log(abs(x))/log(10)\n");
  fprintf (gf, "Z(x)=exp({0,-1}*x) # gnuplot variable x for H(z)\n");
  fprintf (gf, "set samples 10000  # increase accuracy\n");
  for (uint i = 0; i < n_arrows; i++)
    fprintf (gf, "set arrow %d from first %.17g+0,graph 0 to first %.17g+0,graph 1 nohead lt 0\n", 64 + i, arrows[i], arrows[i]);
  bool want_zero = false;
  for (uint i = 0; i < n_consts; i++)
    want_zero |= consts[i] == 0.0;
  fprintf (gf, "plot [][-150:3] %s '%s' using ($1):($3) with lines, '%s' using ($1):($2) with lines",
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
exit_with_iir_filter_gnuplot (const BseIIRFilterDesign *fdes,
                              const char               *fname,
                              double                    passband_ripple_db = NAN,
                              double                    passband_edge = NAN,
                              double                    passband_edge2 = NAN,
                              double                    stopband_db = NAN,
                              double                    stopband_edge = NAN,
                              double                    stopband_edge2 = NAN)
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
  bool success = bse_iir_filter_dump_gnuplot (fdes, fname, n_consts, consts, n_arrows, arrows, 10000);
  BIRNET_ASSERT (success == true);
  g_printerr ("GnuplotDump: wrote %s.gp and %s.dat use: gnuplot %s.gp\n", fname, fname, fname);
  exit (0);
}

static double
max_band_damping_ztrans (const BseIIRFilterDesign *fdes,
                         double                    start_freq,
                         double                    end_freq)
{
  const double n_smaple_points = 3333;
  const double n_random_points = 999;
  const double delta = max (1e-13, fabs (end_freq - start_freq) / n_smaple_points);
  double eps = +INFINITY;
  for (double f = start_freq; f < end_freq; f += delta)
    eps = min (eps, filter_ztrans_response (fdes, f));
  for (uint i = 0; i < n_random_points; i++)
    eps = min (eps, filter_ztrans_response (fdes, g_random_double_range (start_freq, end_freq)));
  if (0)
    for (double f = start_freq; f < end_freq; f += delta)
      g_printerr ("PassBandZTransDB: %f: %f\n", f, to_db (filter_ztrans_response (fdes, f)));
  return to_db (eps);
}

static double
min_band_damping_ztrans (const BseIIRFilterDesign *fdes,
                         double                    start_freq,
                         double                    end_freq)
{
  const double n_smaple_points = 3333;
  const double n_random_points = 999;
  const double delta = max (1e-13, fabs (end_freq - start_freq) / n_smaple_points);
  double eps = 0;
  for (double f = start_freq; f < end_freq; f += delta)
    eps = max (eps, filter_ztrans_response (fdes, f));
  for (uint i = 0; i < n_random_points; i++)
    eps = max (eps, filter_ztrans_response (fdes, g_random_double_range (start_freq, end_freq)));
  return to_db (eps);
}

static double
max_band_damping_zp (const BseIIRFilterDesign *fdes,
                     double                    start_freq,
                     double                    end_freq)
{
  const double n_smaple_points = 3333;
  const double n_random_points = 999;
  const double delta = max (1e-13, fabs (end_freq - start_freq) / n_smaple_points);
  double eps = +INFINITY;
  for (double f = start_freq; f < end_freq; f += delta)
    eps = min (eps, filter_zp_response (fdes, f));
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
  const double n_smaple_points = 3333;
  const double n_random_points = 999;
  const double delta = max (1e-13, fabs (end_freq - start_freq) / n_smaple_points);
  double eps = 0;
  for (double f = start_freq; f < end_freq; f += delta)
    eps = max (eps, filter_zp_response (fdes, f));
  for (uint i = 0; i < n_random_points; i++)
    eps = max (eps, filter_zp_response (fdes, g_random_double_range (start_freq, end_freq)));
  if (0)
    for (double f = start_freq; f < end_freq; f += delta * 30)
      g_printerr ("PassBandZPDB: %f: %f\n", f, to_db (filter_zp_response (fdes, f)));
  return to_db (eps);
}

static void
butterwoth_tests ()
{
  TSTART ("Butterworth");
  bool success;
  double eps;
  const double ceps = 1e-13;
  BseIIRFilterDesign fdes;
  BseIIRFilterRequest req = { BseIIRFilterKind (0), };
  req.kind = BSE_IIR_FILTER_BUTTERWORTH;
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
    const double dncoeffs[] = {
      +1.00000000000000000000E+00,    +2.27184001245132058747E-03,
      -1.59056649578489484043E+00,    +1.81747200996105646997E-02,
      +2.08381330026821487422E+00,    +6.36115203486369712449E-02,
      -1.53262556329449450843E+00,    +1.27223040697273942490E-01,
      +8.69440915484917642431E-01,    +1.59028800871592435051E-01,
      -3.19175943252755334179E-01,    +1.27223040697273942490E-01,
      +8.20901315715001494988E-02,    +6.36115203486369712449E-02,
      -1.22466701861471700258E-02,    +1.81747200996105646997E-02,
      +8.61368381197359644919E-04,    +2.27184001245132058747E-03,
    };
    eps = compare_coefficients (&fdes, BIRNET_ARRAY_SIZE (dncoeffs), dncoeffs);
    TASSERT (eps <= ceps);
    TASSERT (max_band_damping_ztrans (&fdes, 0, 2000) > -3.0103);
    TASSERT (max_band_damping_zp     (&fdes, 0, 2000) > -3.0103);
    TASSERT (min_band_damping_ztrans (&fdes, 3500, 5000) < -68);
    TASSERT (min_band_damping_zp     (&fdes, 3500, 5000) < -68);
    if (0)
      exit_with_iir_filter_gnuplot (&fdes, "tmpfilter", -3.0103, 2000, NAN, -68, 3500);
  }
  
  {
    req.type = BSE_IIR_FILTER_HIGH_PASS;
    req.order = 7;
    req.sampling_frequency = 10000;
    req.passband_edge = 2000;
    success = bse_iir_filter_design (&req, &fdes);
    if (0)
      {
        g_printerr ("Filter: %s\n", bse_iir_filter_request_string (&req));
        g_printerr ("Design: %s\n", bse_iir_filter_design_string (&fdes));
      }
    TASSERT (success == true);
    const double dncoeffs[] = {
      +1.00000000000000000000E+00, +4.53114370687427228668E-02,
      -1.38928014715713654681E+00, -3.17180059481199039251E-01,
      +1.67502361752882622525E+00, +9.51540178443597173263E-01,
      -1.05389731637545636111E+00, -1.58590029740599525176E+00,
      +5.08551510935083994625E-01, +1.58590029740599525176E+00,
      -1.44829453299511690112E-01, -9.51540178443597173263E-01,
      +2.62522192538094598091E-02, +3.17180059481199039251E-01,
      -2.02968024924351847157E-03, -4.53114370687427228668E-02,
    };
    eps = compare_coefficients (&fdes, BIRNET_ARRAY_SIZE (dncoeffs), dncoeffs);
    TASSERT (eps <= ceps);
    TASSERT (max_band_damping_ztrans (&fdes, 2000, 5000) > -3.0103);
    TASSERT (max_band_damping_zp     (&fdes, 2000, 5000) > -3.0103);
    TASSERT (min_band_damping_ztrans (&fdes, 0,     600) < -80);
    TASSERT (min_band_damping_zp     (&fdes, 0,     600) < -80);
    if (0)
      exit_with_iir_filter_gnuplot (&fdes, "tmpfilter", -3.0103, 2000, NAN, -80, 600);
  }

  {
    req.type = BSE_IIR_FILTER_BAND_PASS;
    req.order = 9;
    req.sampling_frequency = 10000;
    req.passband_edge = 1500;
    req.passband_edge2 = 3500;
    success = bse_iir_filter_design (&req, &fdes);
    TASSERT (success == true);
    const double dncoeffs[] = {
      +1.00000000000000000000E+00,    +1.06539452359780476010E-03,	
      -8.88178419700125232339E-16,    +0.00000000000000000000E+00,	
      +1.79158135278859775852E+00,    -9.58855071238024284086E-03,	
      -1.88737914186276611872E-15,    +0.00000000000000000000E+00,	
      +2.53189988089812256788E+00,    +3.83542028495209713634E-02,	
      -2.77555756156289135106E-15,    +0.00000000000000000000E+00,	
      +2.11822942034193495431E+00,    -8.94931399822155998480E-02,	
      -1.66533453693773481064E-15,    +0.00000000000000000000E+00,	
      +1.37075629439323409819E+00,    +1.34239709973323406711E-01,	
      -8.88178419700125232339E-16,    +0.00000000000000000000E+00,	
      +6.09038913076474175412E-01,    -1.34239709973323406711E-01,	
      -3.88578058618804789148E-16,    +0.00000000000000000000E+00,	
      +1.99331556962956374379E-01,    +8.94931399822155998480E-02,	
      -1.04083408558608425665E-16,    +0.00000000000000000000E+00,	
      +4.31047310152814222572E-02,    -3.83542028495209713634E-02,	
      -1.30104260698260532081E-17,    +0.00000000000000000000E+00,	
      +5.80426165430881872698E-03,    +9.58855071238024284086E-03,	
      -9.75781955236953990607E-19,    +0.00000000000000000000E+00,	
      +3.55580604257624494427E-04,    -1.06539452359780476010E-03,	
    };
    eps = compare_coefficients (&fdes, BIRNET_ARRAY_SIZE (dncoeffs), dncoeffs);
    TASSERT (eps <= ceps);
    TASSERT (max_band_damping_ztrans (&fdes, 1500, 3500) > -3.0103);
    TASSERT (max_band_damping_zp     (&fdes, 1500, 3500) > -3.0103);
    TASSERT (min_band_damping_ztrans (&fdes, 0,    1000) < -49.5);
    TASSERT (min_band_damping_zp     (&fdes, 0,    1000) < -49.5);
    TASSERT (min_band_damping_ztrans (&fdes, 4000, 5000) < -49.5);
    TASSERT (min_band_damping_zp     (&fdes, 4000, 5000) < -49.5);
    if (0)
      exit_with_iir_filter_gnuplot (&fdes, "tmpfilter", -3.0103, 1500, 3500, -49.5, 1000, 4000);
  }
  
  {
    req.type = BSE_IIR_FILTER_BAND_STOP;
    req.order = 14;
    req.sampling_frequency = 10000;
    req.passband_edge = 1000;
    req.passband_edge2 = 4000;
    success = bse_iir_filter_design (&req, &fdes);
    TASSERT (success == true);
    const double dncoeffs[] = {
      +1.00000000000000000000E+00, +2.40762197986991808164E-05,
      -1.77635683940025046468E-15, -7.02253897611853178699E-20,
      -2.79453948353326975251E+00, +3.37067077181788551758E-04,
      +3.37268190805928291809E-15, -9.12930066895409541568E-19,
      +5.36686283457015544940E+00, +2.19093600168162542380E-03,
      -4.55093861900790486175E-15, -5.47758040137245763460E-18,
      -6.92455413586185386521E+00, +8.76374400672650169519E-03,
      +6.21312896964543170952E-15, -2.00844614716990087589E-17,
      +6.95774054370916861245E+00, +2.41002960184978805291E-02,
      -3.95343480175114336816E-15, -5.02111536792475234381E-17,
      -5.41985806690049898293E+00, +4.82005920369957610583E-02,
      +3.27678422590294005090E-15, -9.03800766226455286300E-17,
      +3.40006470141212968628E+00, +7.23008880554936450569E-02,
      -1.66685241997921451684E-15, -1.20506768830194021739E-16,
      -1.70520248824675912935E+00, +8.26295863491355864205E-02,
      +7.54794447430096049345E-16, -1.20506768830194021739E-16,
      +6.87886899574442156613E-01, +7.23008880554936450569E-02,
      -1.98944322387512029238E-16, -9.03800766226455286300E-17,
      -2.19574234020385783417E-01, +4.82005920369957610583E-02,
      +5.71577832807201868803E-17, -5.02111536792475172751E-17,
      +5.45755818762698116653E-02, +2.41002960184978805291E-02,
      -6.05840315523638317519E-18, -2.00844614716990087589E-17,
      -1.01733051095814052561E-02, +8.76374400672650169519E-03,
      +1.40316301668595190544E-18, -5.47758040137245686422E-18,
      +1.34351148707345332579E-03, +2.19093600168162542380E-03,
      -1.07480540072690203962E-19, -9.12930066895409541568E-19,
      -1.12019310595550133057E-04, +3.37067077181788551758E-04,
      +1.59976730467755552320E-21, -7.02253897611853539810E-20,
      +4.44553559045252372504E-06, +2.40762197986991808164E-05,
    };
    eps = compare_coefficients (&fdes, BIRNET_ARRAY_SIZE (dncoeffs), dncoeffs);
    TASSERT (eps <= ceps);
    TASSERT (max_band_damping_ztrans (&fdes, 0,    1000) > -3.0103);
    TASSERT (max_band_damping_zp     (&fdes, 0,    1000) > -3.0103);
    TASSERT (min_band_damping_ztrans (&fdes, 1500, 3500) < -77);
    TASSERT (min_band_damping_zp     (&fdes, 1500, 3500) < -77);
    TASSERT (max_band_damping_ztrans (&fdes, 4000, 5000) > -3.0103);
    TASSERT (max_band_damping_zp     (&fdes, 4000, 5000) > -3.0103);
    if (0)
      exit_with_iir_filter_gnuplot (&fdes, "tmpfilter", -3.0103, 1000, 4000, -77, 1500, 3500);
  }

  TOK();
  TDONE();
}

int
main (int    argc,
      char **argv)
{
  bse_init_test (&argc, &argv, NULL);
  butterwoth_tests ();
  return 0;
}
