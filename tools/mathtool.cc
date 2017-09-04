// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#define GSL_EXTENSIONS
#include <bse/bsemain.hh>
#include <bse/gslcommon.hh>
#include <bse/gslfilter.hh>
#include <bse/bseloader.hh>
#include <bse/bsemathsignal.hh>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PREC    "15"

using Rapicorn::printout;

static void	usage (void)	G_GNUC_NORETURN;

static void  ring_test (void);

static guint         shift_argc = 0;
static const gchar **shift_argv = NULL;

static const gchar*
shift (void)
{
  const gchar *arg;

  if (shift_argc > 1)
    {
      shift_argc--;
      arg = shift_argv++[1];
      if (!arg)
	arg = "";
    }
  else
    arg = NULL;
  return arg;
}
static const gchar*
pshift (void)
{
  const gchar *arg = shift ();

  return arg ? arg : "";
}

int
main (int   argc,
      char *argv[])
{
  const gchar *arg;
  /* iir filter parameters */
  enum { FILTER_GNUPLOT, FILTER_SCAN } filter_mode = FILTER_GNUPLOT;
  const gchar *filter_label = 0;
  gdouble *a, *b;
  guint i, order = 0;

  shift_argc = argc;
  shift_argv = (const gchar**) argv;

  /* init */
  bse_init_inprocess (&argc, argv, NULL,
                      Bse::cstrings_to_vector ("stand-alone=1", "wave-chunk-padding=1",
                                               "dcache-block-size=8192", "dcache-cache-memory=5242880", NULL));
  arg = shift ();
  if (!arg)
    usage ();
 restart:
  a = b = 0;
  if (strcmp (arg, "approx5-exp2-run") == 0)
    {
      gfloat f;
      gdouble r = 0;
      for (i = 0; i < 10; i++)
	for (f = -1; f <= 1.0; f += 1.0 / 10000000.0)
	  r += bse_approx5_exp2 (f);
      return (r > 0) & 8;
    }
  else if (strcmp (arg, "libc-exp-run") == 0)
    {
      gfloat f;
      gdouble r = 0;
      for (i = 0; i < 10; i++)
	for (f = -1; f <= 1.0; f += 1.0 / 10000000.0)
	  r += exp (f);
      return (r > 0) & 8;
    }
  else if (strcmp (arg, "wave-scan") == 0)
    {
      const gchar *file = pshift ();

      while (file)
	{
	  BseWaveFileInfo *fi;
	  Bse::Error error;

	  fi = bse_wave_file_info_load (file, &error);
	  if (fi)
	    {
	      guint i;

	      printout ("Loader \"%s\" found %u waves in \"%s\":\n", fi->loader->name, fi->n_waves, file);
	      for (i = 0; i < fi->n_waves; i++)
		printout ("%u) %s\n", i + 1, fi->waves[i].name);
	      bse_wave_file_info_unref (fi);
	    }
	  else
	    printout ("Failed to scan \"%s\": %s\n", file, bse_error_blurb (error));
	  file = pshift ();
	  if (!file[0])
	    break;
	}
    }
  else if (strcmp (arg, "file-test") == 0)
    {
      const gchar *file = pshift ();

      printout ("file test for \"%s\":\n", file);
      printout ("  is readable   : %s\n", bse_error_blurb (gsl_file_check (file, "r")));
      printout ("  is writable   : %s\n", bse_error_blurb (gsl_file_check (file, "w")));
      printout ("  is executable : %s\n", bse_error_blurb (gsl_file_check (file, "x")));
      printout ("  is file       : %s\n", bse_error_blurb (gsl_file_check (file, "f")));
      printout ("  is directory  : %s\n", bse_error_blurb (gsl_file_check (file, "d")));
      printout ("  is link       : %s\n", bse_error_blurb (gsl_file_check (file, "l")));
    }
  else if (strcmp (arg, "ring-test") == 0)
    {
      ring_test ();
    }
#if 0
  else if (strcmp (arg, "rf") == 0)
    {
      double x, y, z;
      x = atof (pshift ());
      y = atof (pshift ());
      z = atof (pshift ());

      printout ("rf(%f, %f, %f) = %."PREC"f\n", x, y, z, gsl_ellip_rf (x, y, z));
    }
  else if (strcmp (arg, "F") == 0)
    {
      double phi, ak;
      phi = atof (pshift ());
      ak = atof (pshift ());

      printout ("F(%f, %f) = %."PREC"f\n", phi, ak, gsl_ellip_F (phi, ak));
    }
  else if (strcmp (arg, "sn") == 0)
    {
      double u, emmc;
      u = atof (pshift ());
      emmc = atof (pshift ());

      printout ("sn(%f, %f) = %."PREC"f\n", u, emmc, gsl_ellip_sn (u, emmc));
    }
  else if (strcmp (arg, "snc") == 0)
    {
      BseComplex u, emmc;
      u.re = atof (pshift ());
      u.im = atof (pshift ());
      emmc.re = atof (pshift ());
      emmc.im = atof (pshift ());

      printout ("snc(%s, %s) = %s\n",
	       bse_complex_str (u),
	       bse_complex_str (emmc),
	       bse_complex_str (bse_complex_ellip_sn (u, emmc)));
    }
  else if (strcmp (arg, "sci_snc") == 0)
    {
      BseComplex u, k2;
      u.re = atof (pshift ());
      u.im = atof (pshift ());
      k2.re = atof (pshift ());
      k2.im = atof (pshift ());

      printout ("sci_snc(%s, %s) = %s\n",
                bse_complex_str (u),
                bse_complex_str (k2),
                bse_complex_str (bse_complex_ellip_sn (u, bse_complex_sub (bse_complex (1.0, 0), k2))));
    }
  else if (strcmp (arg, "asn") == 0)
    {
      double y, emmc;
      y = atof (pshift ());
      emmc = atof (pshift ());

      printout ("asn(%f, %f) = %."PREC"f\n", y, emmc, gsl_ellip_asn (y, emmc));
    }
  else if (strcmp (arg, "asnc") == 0)
    {
      BseComplex y, emmc;
      y.re = atof (pshift ());
      y.im = atof (pshift ());
      emmc.re = atof (pshift ());
      emmc.im = atof (pshift ());

      printout ("asnc(%s, %s) = %s\n",
                bse_complex_str (y), bse_complex_str (emmc),
                bse_complex_str (bse_complex_ellip_asn (y, emmc)));
      printout ("asn(%f, %f = %."PREC"f\n",
                y.re, emmc.re, gsl_ellip_asn (y.re, emmc.re));
    }
  else if (strcmp (arg, "sci_sn") == 0)
    {
      double u, k2;
      u = atof (pshift ());
      k2 = atof (pshift ());
      printout ("sci_sn(%f, %f) = %."PREC"f\n", u, k2, gsl_ellip_sn (u, 1.0 - k2));
    }
  else if (strcmp (arg, "sci_asn") == 0)
    {
      double y, k2;
      y = atof (pshift ());
      k2 = atof (pshift ());
      printout ("sci_asn(%f, %f) = %."PREC"f\n", y, k2, gsl_ellip_asn (y, 1.0 - k2));
    }
  else if (strcmp (arg, "sci_asnc") == 0)
    {
      BseComplex y, k2;
      y.re = atof (pshift ());
      y.im = atof (pshift ());
      k2.re = atof (pshift ());
      k2.im = atof (pshift ());
      printout ("sci_asnc(%s, %s) = %s\n",
                bse_complex_str (y), bse_complex_str (k2),
                bse_complex_str (bse_complex_ellip_asn (y, bse_complex_sub (bse_complex (1.0, 0), k2))));
      printout ("asn(%f, %f = %."PREC"f\n",
                y.re, k2.re, gsl_ellip_asn (y.re, 1.0 - k2.re));
    }
  else if (strncmp (arg, "poly", 4) == 0)
    {
      guint order;
      arg = arg + 4;
      order = 2;
      {
	double a[100] = { 1, 2, 1 }, b[100] = { 1, -3./2., 0.5 };
	printout ("# Test order=%u  norm=%f:\n",
                  order,
                  bse_poly_eval (order, a, 1) / bse_poly_eval (order, b, 1));
	printout ("H%u(z)=%s/%s\n", order,
                  bse_poly_str (order, a, "z"),
                  bse_poly_str (order, b, "z"));
	if (*arg)
	  {
	    BseComplex root, roots[100];
	    guint i;

	    if (*arg == 'r')
	      {
		printout ("#roots:\n");
		bse_poly_complex_roots (order, a, roots);
		for (i = 0; i < order; i++)
		  {
		    root = bse_complex_div (bse_complex (1, 0), roots[i]);
		    printout ("%+.14f %+.14f # %.14f\n", root.re, root.im, bse_complex_abs (root));
		  }
	      }
	    if (*arg == 'p')
	      {
		printout ("#poles:\n");
		bse_poly_complex_roots (order, b, roots);
		for (i = 0; i < order; i++)
		  {
		    root = bse_complex_div (bse_complex (1, 0), roots[i]);
		    printout ("%+.14f %+.14f # %.14f\n", root.re, root.im, bse_complex_abs (root));
		  }
	      }
	  }
      }
    }
#endif
  else if (strcmp (arg, "sin") == 0)
    {
      BseComplex phi;
      phi.re = atof (pshift ());
      phi.im = atof (pshift ());
      printout ("sin(%s) = %s\n",
                bse_complex_str (phi),
                bse_complex_str (bse_complex_sin (phi)));
    }
  else if (strcmp (arg, "cos") == 0)
    {
      BseComplex phi;
      phi.re = atof (pshift ());
      phi.im = atof (pshift ());
      printout ("cos(%s) = %s\n",
                bse_complex_str (phi),
                bse_complex_str (bse_complex_cos (phi)));
    }
  else if (strcmp (arg, "tan") == 0)
    {
      BseComplex phi;
      phi.re = atof (pshift ());
      phi.im = atof (pshift ());
      printout ("tan(%s) = %s\n",
                bse_complex_str (phi),
                bse_complex_str (bse_complex_tan (phi)));
    }
  else if (strcmp (arg, "sinh") == 0)
    {
      BseComplex phi;
      phi.re = atof (pshift ());
      phi.im = atof (pshift ());
      printout ("sinh(%s) = %s\n",
                bse_complex_str (phi),
                bse_complex_str (bse_complex_sinh (phi)));
    }
  else if (strcmp (arg, "cosh") == 0)
    {
      BseComplex phi;
      phi.re = atof (pshift ());
      phi.im = atof (pshift ());
      printout ("cosh(%s) = %s\n",
                bse_complex_str (phi),
                bse_complex_str (bse_complex_cosh (phi)));
    }
  else if (strcmp (arg, "tanh") == 0)
    {
      BseComplex phi;
      phi.re = atof (pshift ());
      phi.im = atof (pshift ());
      printout ("tanh(%s) = %s\n",
                bse_complex_str (phi),
                bse_complex_str (bse_complex_tanh (phi)));
    }
  else if (strcmp (arg, "midi2freq") == 0)
    {
      gint note;
      note = atol (pshift ());
      note = CLAMP (note, 0, 128);
      printout ("midi2freq(%u) = %f\n",
                note,
                bse_temp_freq (BSE_CONFIG (kammer_freq), note - BSE_CONFIG (midi_kammer_note)));
    }
  else if (strcmp (arg, "blp") == 0)
    {
      double f, e;
      order = atoi (pshift ()); order = MAX (order, 1);
      f = atof (pshift ());
      e = atof (pshift ());
      f *= PI / 2.;
      a = g_new (gdouble, order + 1);
      b = g_new (gdouble, order + 1);
      gsl_filter_butter_lp (order, f, e, a, b);
      printout ("# Lowpass Butterworth filter order=%u freq=%f epsilon(s^2)=%f norm0=%f:\n",
                order, f, e,
                bse_poly_eval (order, a, 1) / bse_poly_eval (order, b, 1));
      filter_label = "BL";
    }
  else if (strcmp (arg, "bhp") == 0)
    {
      double f, e;
      order = atoi (pshift ()); order = MAX (order, 1);
      f = atof (pshift ());
      e = atof (pshift ());
      f *= PI / 2.;
      a = g_new (gdouble, order + 1);
      b = g_new (gdouble, order + 1);

      gsl_filter_butter_hp (order, f, e, a, b);
      printout ("# Highpass Butterworth filter order=%u freq=%f epsilon(s^2)=%f norm0=%f:\n",
                order, f, e,
                bse_poly_eval (order, a, 1) / bse_poly_eval (order, b, 1));
      filter_label = "BH";
    }
  else if (strcmp (arg, "bbp") == 0)
    {
      double f1, f2, e;
      order = atoi (pshift ()); order = MAX (order, 1);
      f1 = atof (pshift ());
      f2 = atof (pshift ());
      e = atof (pshift ());
      f1 *= PI / 2.;
      f2 *= PI / 2.;
      a = g_new (gdouble, order + 1);
      b = g_new (gdouble, order + 1);

      gsl_filter_butter_bp (order, f1, f2, e, a, b);
      printout ("# Bandpass Butterworth filter order=%u freq1=%f freq2=%f epsilon(s^2)=%f norm0=%f:\n",
                order, f1, f2, e,
                bse_poly_eval (order, a, 1) / bse_poly_eval (order, b, 1));
      filter_label = "BP";
    }
  else if (strcmp (arg, "bbs") == 0)
    {
      double f1, f2, e;
      order = atoi (pshift ()); order = MAX (order, 1);
      f1 = atof (pshift ());
      f2 = atof (pshift ());
      e = atof (pshift ());
      f1 *= PI / 2.;
      f2 *= PI / 2.;
      a = g_new (gdouble, order + 1);
      b = g_new (gdouble, order + 1);

      gsl_filter_butter_bs (order, f1, f2, e, a, b);
      printout ("# Bandstop Butterworth filter order=%u freq1=%f freq2=%f epsilon(s^2)=%f norm0=%f:\n",
                order, f1, f2, e,
                bse_poly_eval (order, a, 1) / bse_poly_eval (order, b, 1));
      filter_label = "BS";
    }
  else if (strcmp (arg, "t1l") == 0)
    {
      double f, e;
      order = atoi (pshift ()); order = MAX (order, 1);
      f = atof (pshift ());
      e = atof (pshift ());
      f *= PI / 2.;
      a = g_new (gdouble, order + 1);
      b = g_new (gdouble, order + 1);

      gsl_filter_tscheb1_lp (order, f, e, a, b);
      printout ("# Lowpass Tschebyscheff Type1 order=%u freq=%f epsilon(s^2)=%f norm0=%f:\n",
                order, f, e,
                bse_poly_eval (order, a, 1) / bse_poly_eval (order, b, 1));
      filter_label = "T1L";
    }
  else if (strcmp (arg, "t1h") == 0)
    {
      double f, e;
      order = atoi (pshift ()); order = MAX (order, 1);
      f = atof (pshift ());
      e = atof (pshift ());
      f *= PI / 2.;

      a = g_new (gdouble, order + 1);
      b = g_new (gdouble, order + 1);

      gsl_filter_tscheb1_hp (order, f, e, a, b);
      printout ("# Highpass Tschebyscheff Type1 order=%u freq=%f epsilon(s^2)=%f norm0=%f:\n",
                order, f, e,
                bse_poly_eval (order, a, 1) / bse_poly_eval (order, b, 1));
      filter_label = "T1H";
    }
  else if (strcmp (arg, "t1s") == 0)
    {
      double fc, fr, e;
      order = atoi (pshift ()); order = MAX (order, 1);
      fc = atof (pshift ());
      fr = atof (pshift ());
      e = atof (pshift ());
      fc *= PI / 2.;
      fr *= PI / 2.;

      a = g_new (gdouble, order + 1);
      b = g_new (gdouble, order + 1);

      gsl_filter_tscheb1_bs (order, fc, fr, e, a, b);
      printout ("# Bandstop Tschebyscheff Type1 order=%u freq_c=%f freq_r=%f epsilon(s^2)=%f norm=%f:\n",
                order, fc, fr, e,
                bse_poly_eval (order, a, 1) / bse_poly_eval (order, b, 1));
      filter_label = "T1S";
    }
  else if (strcmp (arg, "t1p") == 0)
    {
      double fc, fr, e;
      order = atoi (pshift ()); order = MAX (order, 1);
      fc = atof (pshift ());
      fr = atof (pshift ());
      e = atof (pshift ());
      fc *= PI / 2.;
      fr *= PI / 2.;

      a = g_new (gdouble, order + 1);
      b = g_new (gdouble, order + 1);

      gsl_filter_tscheb1_bp (order, fc, fr, e, a, b);
      printout ("# Bandpass Tschebyscheff Type1 order=%u freq_c=%f freq_r=%f epsilon(s^2)=%f norm=%f:\n",
                order, fc, fr, e,
                bse_poly_eval (order, a, 1) / bse_poly_eval (order, b, 1));
      filter_label = "T1P";
    }
  else if (strcmp (arg, "t2l") == 0)
    {
      double f, st, e;
      order = atoi (pshift ()); order = MAX (order, 1);
      f = atof (pshift ());
      st = atof (pshift ());
      e = atof (pshift ());
      f *= PI / 2.;

      a = g_new (gdouble, order + 1);
      b = g_new (gdouble, order + 1);

      gsl_filter_tscheb2_lp (order, f, st, e, a, b);
      printout ("# Lowpass Tschebyscheff Type2 order=%u freq=%f steepness=%f (%f) epsilon(s^2)=%f norm=%f:\n",
                order, f, st, f * (1.+st), e,
                bse_poly_eval (order, a, 1) / bse_poly_eval (order, b, 1));
      filter_label = "T2L";
    }
  else if (strcmp (arg, "t2h") == 0)
    {
      double f, st, e;
      order = atoi (pshift ()); order = MAX (order, 1);
      f = atof (pshift ());
      st = atof (pshift ());
      e = atof (pshift ());
      f *= PI / 2.;

      a = g_new (gdouble, order + 1);
      b = g_new (gdouble, order + 1);

      gsl_filter_tscheb2_hp (order, f, st, e, a, b);
      printout ("# Highpass Tschebyscheff Type2 order=%u freq=%f steepness=%f (%f, %f) epsilon(s^2)=%f norm=%f:\n",
                order, f, st, PI - f, (PI - f) * (1.+st), e,
                bse_poly_eval (order, a, 1) / bse_poly_eval (order, b, 1));
      filter_label = "T2H";
    }
  else if (strcmp (arg, "t2p") == 0)
    {
      double f1, f2, st, e;
      order = atoi (pshift ()); order = MAX (order, 1);
      f1 = atof (pshift ());
      f2 = atof (pshift ());
      st = atof (pshift ());
      e = atof (pshift ());
      f1 *= PI / 2.;
      f2 *= PI / 2.;

      a = g_new (gdouble, order + 1);
      b = g_new (gdouble, order + 1);

      gsl_filter_tscheb2_bp (order, f1, f2, st, e, a, b);
      printout ("# Bandpass Tschebyscheff Type2 order=%u freq1=%f freq2=%f steepness=%f epsilon(s^2)=%f norm=%f:\n",
                order, f1, f2, st, e,
                bse_poly_eval (order, a, 1) / bse_poly_eval (order, b, 1));
      filter_label = "T2P";
    }
  else if (strcmp (arg, "t2s") == 0)
    {
      double f1, f2, st, e;
      order = atoi (pshift ()); order = MAX (order, 1);
      f1 = atof (pshift ());
      f2 = atof (pshift ());
      st = atof (pshift ());
      e = atof (pshift ());
      f1 *= PI / 2.;
      f2 *= PI / 2.;

      a = g_new (gdouble, order + 1);
      b = g_new (gdouble, order + 1);

      gsl_filter_tscheb2_bs (order, f1, f2, st, e, a, b);
      printout ("# Bandstop Tschebyscheff Type2 order=%u freq1=%f freq2=%f steepness=%f epsilon(s^2)=%f norm=%f:\n",
                order, f1, f2, st, e,
                bse_poly_eval (order, a, 1) / bse_poly_eval (order, b, 1));
      filter_label = "T2S";
    }
  else if (strcmp (arg, "scan") == 0)
    {
      filter_mode = FILTER_SCAN;
    }
  else if (strcmp (arg, "fir") == 0)
    {
      unsigned int iorder = atoi (pshift ());
      unsigned int n_points = 0;

      double *freq = g_newa (double, argc / 2 + 1);
      double *value = g_newa (double, argc / 2 + 1);
      double *a = g_newa (double, iorder);
      const char *f, *v;

      do
	{
	  f = pshift ();
	  v = pshift ();

	  if (f[0] && v[0])
	    {
	      freq[n_points] = atof (f) * PI;
	      value[n_points] = atof (v);
	      n_points++;
	    }
	}
      while (f[0] && v[0]);

      gsl_filter_fir_approx (iorder, a, n_points, freq, value, FALSE);
      printout ("FIR%u(z)=%s\n", iorder, bse_poly_str (iorder, a, "z"));
    }
  else
    usage ();

  if (a && b)
    {
      gdouble freq;

      if (filter_mode == FILTER_SCAN)
	{
	  freq = 0.001;
	  while (freq < 3.14)
	    {
	      printout ("%f %.20f\n", freq, gsl_filter_sine_scan (order, a, b, freq, 2 * M_PI));
	      freq = MIN (freq * 1.1, freq + 0.01);
	    }
	}
      else if (filter_mode == FILTER_GNUPLOT)
	{
	  printout ("%s%u(z)=%s/%s\n", filter_label, order,
                    bse_poly_str (order, a, "z"),
                    bse_poly_str (order, b, "z"));
	}
      else
        Bse::warning ("unknown filter_mode");
      g_free (a);
      g_free (b);
    }

  arg = shift ();
  if (arg)
    goto restart;

  return 0;
}

static void
usage (void)
{
  printout ("usage: mathtests {test} [args...]\n");
  printout ("tests:\n");
  printout ("  wave-scan <file>          scan a wave file for waves\n");
  printout ("  file-test <file>          test file properties\n");
  printout ("  ring-test                 test ring implementation\n");
  printout ("  rf <x> <y> <z>            Carlson's elliptic integral of the first kind\n");
  printout ("  F <phi> <ak>              Legendre elliptic integral of the 1st kind\n");
  printout ("  sn <u> <emmc>             Jacobian elliptic function sn()\n");
  printout ("  asn <y> <emmc>            elliptic integral, inverse sn()\n");
  printout ("  sin <phi.re> <phi.im>     complex sine\n");
  printout ("  cos <phi.re> <phi.im>     complex cosine\n");
  printout ("  tan <phi.re> <phi.im>     complex tangent\n");
  printout ("  sinh <phi.re> <phi.im>    complex hyperbolic sine\n");
  printout ("  cosh <phi.re> <phi.im>    complex hyperbolic cosine\n");
  printout ("  tanh <phi.re> <phi.im>    complex hyperbolic tangent\n");
  printout ("  midi2freq <midinote>      convert midinote into oscilaltor frequency\n");
  printout ("  snc <u.re> <u.im> <emmc.re> <emmc.im>     sn() for complex numbers\n");
  printout ("  asnc <y.re> <y.im> <emmc.re> <emmc.im>    asn() for complex numbers\n");
  printout ("  sci_sn <u> <k2>                           scilab version of sn()\n");
  printout ("  sci_asn <y> <k2>                          scilab version of asn()\n");
  printout ("  sci_snc <u.re> <u.im> <k2.re> <k2.im>     scilab version of snc()\n");
  printout ("  sci_asnc <y.re> <y.im> <k2.re> <k2.im>    scilab version of asnc()\n");
  printout ("  blp <order> <freq> <epsilon>              butterworth lowpass filter\n");
  printout ("  bhp <order> <freq> <epsilon>              butterworth higpass filter\n");
  printout ("  bbp <order> <freqc> <freqr> <epsilon>     butterworth bandpass filter\n");
  printout ("  t1l <order> <freq> <epsilon>              type1 tschebyscheff lowpass filter\n");
  printout ("  t1h <order> <freq> <epsilon>              type1 tschebyscheff highpass filter\n");
  printout ("  t1s <order> <freqc> <freqr> <epsilon>     type1 tschebyscheff bandstop filter\n");
  printout ("  t1p <order> <freqc> <freqr> <epsilon>     type1 tschebyscheff bandpass filter\n");
  printout ("  t2l <order> <freqc> <steepn> <epsilon>    type2 tschebyscheff lowpass filter\n");
  printout ("  t2h <order> <freqc> <steepn> <epsilon>    type2 tschebyscheff highpass filter\n");
  printout ("  fir <order> <freq1> <value1> ...          fir approximation\n");
  printout ("  scan blp <order> <freq> <epsilon>         scan butterworth lowpass filter\n");
  printout ("  poly | polyr | polyp                      polynom test (+roots or +poles)\n");
  exit (1);
}

static void
print_int_ring (SfiRing *ring)
{
  SfiRing *node;
  printout ("{");
  for (node = ring; node; node = sfi_ring_walk (node, ring))
    printout ("%c", char (size_t (node->data)));
  printout ("}");
}

static gint
ints_cmp (gconstpointer d1,
	  gconstpointer d2,
          gpointer      data)
{
  size_t i1 = size_t (d1);
  size_t i2 = size_t (d2);
  return i1 < i2 ? -1 : i1 > i2;
}

static void
ring_test (void)
{
  gint data_array[][64] = {
    { 0, },
    { 1, 'a', },
    { 2, 'a', 'a', },
    { 2, 'a', 'b', },
    { 2, 'z', 'a', },
    { 3, 'a', 'c', 'z' },
    { 3, 'a', 'z', 'c' },
    { 3, 'c', 'a', 'z' },
    { 3, 'z', 'c', 'a' },
    { 3, 'a', 'a', 'a' },
    { 3, 'a', 'a', 'z' },
    { 3, 'a', 'z', 'a' },
    { 3, 'z', 'a', 'a' },
    { 10, 'g', 's', 't', 'y', 'x', 'q', 'i', 'n', 'j', 'a' },
    { 15, 'w', 'k', 't', 'o', 'c', 's', 'j', 'd', 'd', 'q', 'p', 'v', 'q', 'r', 'a' },
    { 26, 'z', 'y', 'x', 'w', 'v', 'u', 't', 's', 'r', 'q', 'p', 'o', 'n', 'm'
      ,   'l', 'k', 'j', 'i', 'h', 'g', 'f', 'e', 'd', 'c', 'b', 'a', },
  };

  for (uint n = 0; n < G_N_ELEMENTS (data_array); n++)
    {
      uint l = data_array[n][0];
      SfiRing *ring = NULL;
      for (uint i = 1; i <= l; i++)
	ring = sfi_ring_append (ring, (void*) size_t (data_array[n][i]));
      printout ("source: ");
      print_int_ring (ring);
      ring = sfi_ring_sort (ring, ints_cmp, NULL);
      printout (" sorted: ");
      print_int_ring (ring);
      printout ("\n");
      sfi_ring_free (ring);
    }
}


/* vim:set ts=8 sts=2 sw=2: */
