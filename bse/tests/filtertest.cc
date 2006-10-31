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
  const double decibel20 = 8.6858896380650365530225783783321; /* 20.0 / ln (10.0) */
  return response <= 0.0 ? -999.99 : max (decibel20 * log (response), -999.99);
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
  exit (0);
}

static double
max_band_damping_ztrans (const BseIIRFilterDesign *fdes,
                         double                    start_freq,
                         double                    end_freq)
{
  const double n_sample_points = 3333;
  const double n_random_points = 999;
  const double delta = max (1e-13, fabs (end_freq - start_freq) / n_sample_points);
  double eps = +INFINITY;
  for (double f = start_freq; f < end_freq; f += delta)
    eps = min (eps, filter_ztrans_response (fdes, f));
  eps = min (eps, filter_ztrans_response (fdes, end_freq));
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
  const double n_sample_points = 3333;
  const double n_random_points = 999;
  const double delta = max (1e-13, fabs (end_freq - start_freq) / n_sample_points);
  double eps = 0;
  for (double f = start_freq; f < end_freq; f += delta)
    eps = max (eps, filter_ztrans_response (fdes, f));
  eps = max (eps, filter_ztrans_response (fdes, end_freq));
  for (uint i = 0; i < n_random_points; i++)
    eps = max (eps, filter_ztrans_response (fdes, g_random_double_range (start_freq, end_freq)));
  return to_db (eps);
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
  double res1 = max_band_damping_ztrans (fdes, start_freq, end_freq);
  double res2 = max_band_damping_zp (fdes, start_freq, end_freq);
  return min (res1, res2);
}

static double
min_band_damping (const BseIIRFilterDesign *fdes,
                  double                    start_freq,
                  double                    end_freq)
{
  double res1 = min_band_damping_ztrans (fdes, start_freq, end_freq);
  double res2 = min_band_damping_zp (fdes, start_freq, end_freq);
  return max (res1, res2);
}

static void
butterwoth_tests ()
{
  TSTART ("Butterworth");
  bool success;
  double eps;
  const double ceps = 1e-13, gaineps = 1e-7;
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
    TASSERT (min_band_damping (&fdes, 0, 2000) < gaineps);
    TASSERT (max_band_damping (&fdes, 0, 2000) > -3.0103);
    TASSERT (min_band_damping (&fdes, 3500, 5000) < -68);
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
    TASSERT (min_band_damping (&fdes, 2000, 5000) < gaineps);
    TASSERT (max_band_damping (&fdes, 2000, 5000) > -3.0103);
    TASSERT (min_band_damping (&fdes, 0,     600) < -80);
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
    TASSERT (min_band_damping (&fdes, 1500, 3500) < gaineps);
    TASSERT (max_band_damping (&fdes, 1500, 3500) > -3.0103);
    TASSERT (min_band_damping (&fdes, 0,    1000) < -49.5);
    TASSERT (min_band_damping (&fdes, 4000, 5000) < -49.5);
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
    TASSERT (min_band_damping (&fdes, 0,    1000) < gaineps);
    TASSERT (max_band_damping (&fdes, 0,    1000) > -3.0103);
    TASSERT (min_band_damping (&fdes, 1500, 3500) < -77);
    TASSERT (max_band_damping (&fdes, 4000, 5000) > -3.0103);
    if (0)
      exit_with_iir_filter_gnuplot (&req, &fdes, "tmpfilter", -3.0103, 1000, 4000, -77, 1500, 3500);
  }

  TOK();
  TDONE();
}

static void
chebychev1_tests ()
{
  TSTART ("Chebyshev1");
  bool success;
  double eps;
  const double ceps = 3.5e-13, gaineps = 1e-7;
  BseIIRFilterDesign fdes;
  BseIIRFilterRequest req = { BseIIRFilterKind (0), };
  req.kind = BSE_IIR_FILTER_CHEBYSHEV1;
  TOK();

  {
    req.type = BSE_IIR_FILTER_LOW_PASS;
    req.order = 8;
    req.sampling_frequency = 20000;
    req.passband_ripple_db = 1.55;
    req.passband_edge = 3000;
    success = bse_iir_filter_design (&req, &fdes);
    TASSERT (success == true);
    const double dncoeffs[] = {
      +1.00000000000000000000E+00, +2.33723734080444880078E-05,
      -5.62943443819617250767E+00, +1.86978987264355904063E-04,
      +1.51476130288319801309E+01, +6.54426455425245637114E-04,
      -2.51115610547854366530E+01, +1.30885291085049127423E-03,
      +2.78744841823312583529E+01, +1.63606613856311414699E-03,
      -2.11496631561481791550E+01, +1.30885291085049127423E-03,
      +1.07046160257529265891E+01, +6.54426455425245637114E-04,
      -3.30978128782384617423E+00, +1.86978987264355904063E-04,
      +4.80878951603652515789E-01, +2.33723734080444880078E-05,
    };
    eps = compare_coefficients (&fdes, BIRNET_ARRAY_SIZE (dncoeffs), dncoeffs);
    TASSERT (eps <= ceps);
    TASSERT (min_band_damping (&fdes, 0, 3000) < gaineps);
    TASSERT (max_band_damping (&fdes, 0, 3000) > -1.55);
    TASSERT (min_band_damping (&fdes, 5000, 10000) < -80);
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
    const double dncoeffs[] = {
      +1.00000000000000000000E+00,    +3.56249822046611874793E-01,
      -4.96234086898923099085E+00,    -2.49374875432628329008E+00,
      +1.07914114429296752462E+01,    +7.48124626297884898207E+00,
      -1.32536434961142131073E+01,    -1.24687437716314164504E+01,
      +9.89084598843435891524E+00,    +1.24687437716314164504E+01,
      -4.46412696164509537056E+00,    -7.48124626297884898207E+00,
      +1.11977829190352773381E+00,    +2.49374875432628329008E+00,
      -1.17830171950224868449E-01,    -3.56249822046611874793E-01,
    };
    eps = compare_coefficients (&fdes, BIRNET_ARRAY_SIZE (dncoeffs), dncoeffs);
    TASSERT (eps <= ceps);
    TASSERT (min_band_damping (&fdes, 600, 5000) < gaineps);
    TASSERT (max_band_damping (&fdes, 600, 5000) >= -0.101);
    TASSERT (min_band_damping (&fdes, 0,    250) < -70);
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
    const double dncoeffs[] = {
      +1.00000000000000000000E+00, +2.31920385422163689425E-05,
      -3.98124955406681069192E+00, +0.00000000000000000000E+00,
      +1.27766889361197417685E+01, -2.31920385422163682649E-04,
      -2.89309897403600331245E+01, +0.00000000000000000000E+00,
      +5.77677582529077398021E+01, +1.04364173439973665324E-03,
      -9.61007933071947491044E+01, +0.00000000000000000000E+00,
      +1.45241576037504046326E+02, -2.78304462506596440863E-03,
      -1.92485913789037965671E+02, +0.00000000000000000000E+00,
      +2.34909345808716381043E+02, +4.87032809386543728142E-03,
      -2.56409506665396406788E+02, +0.00000000000000000000E+00,
      +2.59123417975454231055E+02, -5.84439371263852525812E-03,
      -2.35404468515497939052E+02, +0.00000000000000000000E+00,
      +1.97955507581144104279E+02, +4.87032809386543728142E-03,
      -1.48705798011436428396E+02, +0.00000000000000000000E+00,
      +1.02807044373303227758E+02, -2.78304462506596440863E-03,
      -6.21496429941032673128E+01, +0.00000000000000000000E+00,
      +3.41129408490583330149E+01, +1.04364173439973665324E-03,
      -1.55106342114784769848E+01, +0.00000000000000000000E+00,
      +6.22625879596567699537E+00, -2.31920385422163682649E-04,
      -1.74060521507014698273E+00, +0.00000000000000000000E+00,
      +4.01503497428800870672E-01, +2.31920385422163689425E-05,
    };
    eps = compare_coefficients (&fdes, BIRNET_ARRAY_SIZE (dncoeffs), dncoeffs);
    TASSERT (eps <= ceps);
    TASSERT (min_band_damping (&fdes,  3500,  9500) < gaineps);
    TASSERT (max_band_damping (&fdes,  3500,  9500) > -1.801);
    TASSERT (min_band_damping (&fdes,  0,     3000) < -55);
    TASSERT (min_band_damping (&fdes, 10200, 15000) < -55);
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
    const double dncoeffs[] = {
      +1.00000000000000000000E+00, +1.96880362536745248669E-02,
      -7.77156117237609578297E-16, -3.29561093801165653951E-17,
      +5.49776474725330643878E+00, +2.55944471297768816331E-01,
      -3.39805223889544372184E-15, -3.95473312561398710785E-16,
      +1.54467540799135534257E+01, +1.53566682778661300901E+00,
      -9.97986415729457121415E-15, -2.17510321908769261350E-15,
      +2.80123773311999961777E+01, +5.63077836855091451440E+00,
      -1.32697672294845858687E-14, -7.25034406362564125613E-15,
      +3.62362266146221472241E+01, +1.40769459213772858419E+01,
      -1.15797128830141815570E-14, -1.63132741431576928263E-14,
      +3.47984629108970082711E+01, +2.53385026584791148707E+01,
      -7.21081180876659288970E-15, -2.61012386290523097842E-14,
      +2.52251878087986405319E+01, +3.37846702113054817573E+01,
      -3.96666206825546652226E-15, -3.04514450672276958001E-14,
      +1.34131458439466229038E+01, +3.37846702113054817573E+01,
      -7.30101742951738685861E-16, -2.61012386290523097842E-14,
      +4.54116784190033140334E+00, +2.53385026584791148707E+01,
      +1.21108093172062059040E-15, -1.63132741431576928263E-14,
      -2.02162533208468282730E-02, +1.40769459213772858419E+01,
      +1.45269538585901525352E-15, -7.25034406362564125613E-15,
      -1.29024095627466262037E+00, +5.63077836855091451440E+00,
      +9.62500478624006561290E-16, -2.17510321908769221907E-15,
      -1.03382689326795040863E+00, +1.53566682778661300901E+00,
      +4.11237884023751831819E-16, -3.95473312561398612178E-16,
      -4.41285603877324350552E-01, +2.55944471297768816331E-01,
      +8.49641808731843584113E-17, -3.29561093801165592321E-17,
      -1.01124481689181242028E-01, +1.96880362536745248669E-02,
    };
    eps = compare_coefficients (&fdes, BIRNET_ARRAY_SIZE (dncoeffs), dncoeffs);
    TASSERT (eps <= ceps);
    TASSERT (min_band_damping (&fdes, 0,      8000) < gaineps);
    TASSERT (max_band_damping (&fdes, 0,      8000) > -1.001);
    TASSERT (min_band_damping (&fdes,  8500, 11500) < -78);
    TASSERT (max_band_damping (&fdes, 12000, 20000) > -1.001);
    if (0)
      exit_with_iir_filter_gnuplot (&req, &fdes, "tmpfilter", -1.001, 8000, 12000, -78, 8500, 11500);
  }

  TOK();
  TDONE();
}

static void
brute_coefficient_tests ()
{
  struct {
    const BseIIRFilterRequest *filter_request;
    const double              *filter_coefficients;
    uint                       n_filter_coefficients;
  } filters[1000000];
  uint index = 0;
  // #include "../../r+d-files/tmp.c"
  uint i;
  const double coefficients_epsilon = 1e-9;
  TSTART ("Bruteforce filter checks");
  for (i = 0; i < index; i++)
    {
      const BseIIRFilterRequest *req = filters[i].filter_request;
      const uint n_coefficients = filters[i].n_filter_coefficients;
      const double *coefficients = filters[i].filter_coefficients;
      BseIIRFilterDesign fdes;
      bool success = bse_iir_filter_design (req, &fdes);
      TASSERT (success == true);
      double eps = compare_coefficients (&fdes, n_coefficients, coefficients);
      if (eps > coefficients_epsilon)
        {
          g_printerr ("EPSILON FAILED: %.16g %.16g\n", eps, coefficients_epsilon);
          g_printerr ("EPSILON FAILED: %.16g %.16g\n", eps, coefficients_epsilon);
          g_printerr ("EPSILON FAILED: %.16g %.16g\n", eps, coefficients_epsilon);
          exit_with_iir_filter_gnuplot (req, &fdes, "tmpfilter",
                                        req->passband_ripple_db, req->passband_edge, req->passband_edge2,
                                        req->stopband_db != 0 ? req->stopband_db : NAN, req->stopband_edge, NAN);
          TASSERT (eps <= coefficients_epsilon);
        }
      else
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
  brute_coefficient_tests ();
  return 0;
}
