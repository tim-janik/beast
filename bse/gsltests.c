/* GSL - Generic Sound Layer
 * Copyright (C) 2001 Tim Janik and Stefan Westerfeld
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <gsl/gslmath.h>
#include <gsl/gslwavedsc.h>
#include <gsl/gslcommon.h>
#include <gsl/gslmath.h>
#include <gsl/gslfilter.h>
#include <stdlib.h>
#include <string.h>

#define PREC    "15"


static void	usage (void)	G_GNUC_NORETURN;


static guint   shift_argc = 0;
static gchar **shift_argv = NULL;

static gchar*
shift (void)
{
  gchar *arg;
  
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
static gchar*
pshift (void)
{
  gchar *arg = shift ();
  
  return arg ? arg : "";
}

int
main (int   argc,
      char *argv[])
{
  gchar *arg;
  
  shift_argc = argc;
  shift_argv = argv;

  g_thread_init (NULL);
  gsl_init (NULL);
  
  arg = shift ();
  if (!arg)
    usage ();
  
 restart:
  if (strcmp (arg, "gslwave-scan") == 0)
    {
      gchar *file = pshift ();
      GslRing *node, *ring = gsl_wave_file_scan (file);
      guint i = 0;
      
      g_print ("waves scanned from \"%s\": ", file);
      for (node = ring; node; node = gsl_ring_walk (ring, node), i++)
	g_print ("%s%s", i % 4 ? " " : "\n  ", (gchar*) node->data);
      g_print ("\n");
      gsl_wave_file_scan_free (ring);
    }
  else if (strcmp (arg, "file-test") == 0)
    {
      gchar *file = pshift ();
      
      g_print ("file test for \"%s\":\n", file);
      g_print ("  is_regular: %u\n", g_file_test (file, G_FILE_TEST_IS_REGULAR));
      g_print ("  is_symlink: %u\n", g_file_test (file, G_FILE_TEST_IS_SYMLINK));
      g_print ("  is_dir    : %u\n", g_file_test (file, G_FILE_TEST_IS_DIR));
      g_print ("  is_exec   : %u\n", g_file_test (file, G_FILE_TEST_IS_EXECUTABLE));
      g_print ("  exists    : %u\n", g_file_test (file, G_FILE_TEST_EXISTS));
    }
  else if (strcmp (arg, "rf") == 0)
    {
      double x, y, z;
      x = atof (pshift ());
      y = atof (pshift ());
      z = atof (pshift ());
      
      g_print ("rf(%f, %f, %f) = %."PREC"f\n", x, y, z, gsl_ellip_rf (x, y, z));
    }
  else if (strcmp (arg, "F") == 0)
    {
      double phi, ak;
      phi = atof (pshift ());
      ak = atof (pshift ());
      
      g_print ("F(%f, %f) = %."PREC"f\n", phi, ak, gsl_ellip_F (phi, ak));
    }
  else if (strcmp (arg, "sn") == 0)
    {
      double u, emmc;
      u = atof (pshift ());
      emmc = atof (pshift ());
      
      g_print ("sn(%f, %f) = %."PREC"f\n", u, emmc, gsl_ellip_sn (u, emmc));
    }
  else if (strcmp (arg, "snc") == 0)
    {
      GslComplex u, emmc;
      u.re = atof (pshift ());
      u.im = atof (pshift ());
      emmc.re = atof (pshift ());
      emmc.im = atof (pshift ());
      
      g_print ("snc(%s, %s) = %s\n",
	       gsl_complex_str (u),
	       gsl_complex_str (emmc),
	       gsl_complex_str (gsl_complex_ellip_sn (u, emmc)));
    }
  else if (strcmp (arg, "sci_snc") == 0)
    {
      GslComplex u, k2;
      u.re = atof (pshift ());
      u.im = atof (pshift ());
      k2.re = atof (pshift ());
      k2.im = atof (pshift ());
      
      g_print ("sci_snc(%s, %s) = %s\n",
	       gsl_complex_str (u),
	       gsl_complex_str (k2),
	       gsl_complex_str (gsl_complex_ellip_sn (u, gsl_complex_sub (gsl_complex (1.0, 0), k2))));
    }
  else if (strcmp (arg, "asn") == 0)
    {
      double y, emmc;
      y = atof (pshift ());
      emmc = atof (pshift ());
      
      g_print ("asn(%f, %f) = %."PREC"f\n", y, emmc, gsl_ellip_asn (y, emmc));
    }
  else if (strcmp (arg, "asnc") == 0)
    {
      GslComplex y, emmc;
      y.re = atof (pshift ());
      y.im = atof (pshift ());
      emmc.re = atof (pshift ());
      emmc.im = atof (pshift ());
      
      g_print ("asnc(%s, %s) = %s\n",
	       gsl_complex_str (y), gsl_complex_str (emmc),
	       gsl_complex_str (gsl_complex_ellip_asn (y, emmc)));
      g_print ("asn(%f, %f = %."PREC"f\n",
	       y.re, emmc.re, gsl_ellip_asn (y.re, emmc.re));
    }
  else if (strcmp (arg, "sci_sn") == 0)
    {
      double u, k2;
      u = atof (pshift ());
      k2 = atof (pshift ());
      g_print ("sci_sn(%f, %f) = %."PREC"f\n", u, k2, gsl_ellip_sn (u, 1.0 - k2));
    }
  else if (strcmp (arg, "sci_asn") == 0)
    {
      double y, k2;
      y = atof (pshift ());
      k2 = atof (pshift ());
      g_print ("sci_asn(%f, %f) = %."PREC"f\n", y, k2, gsl_ellip_asn (y, 1.0 - k2));
    }
  else if (strcmp (arg, "sci_asnc") == 0)
    {
      GslComplex y, k2;
      y.re = atof (pshift ());
      y.im = atof (pshift ());
      k2.re = atof (pshift ());
      k2.im = atof (pshift ());
      g_print ("sci_asnc(%s, %s) = %s\n",
	       gsl_complex_str (y), gsl_complex_str (k2),
	       gsl_complex_str (gsl_complex_ellip_asn (y, gsl_complex_sub (gsl_complex (1.0, 0), k2))));
      g_print ("asn(%f, %f = %."PREC"f\n",
	       y.re, k2.re, gsl_ellip_asn (y.re, 1.0 - k2.re));
    }
  else if (strcmp (arg, "sin") == 0)
    {
      GslComplex phi;
      phi.re = atof (pshift ());
      phi.im = atof (pshift ());
      g_print ("sin(%s) = %s\n",
	       gsl_complex_str (phi),
	       gsl_complex_str (gsl_complex_sin (phi)));
    }
  else if (strcmp (arg, "cos") == 0)
    {
      GslComplex phi;
      phi.re = atof (pshift ());
      phi.im = atof (pshift ());
      g_print ("cos(%s) = %s\n",
	       gsl_complex_str (phi),
	       gsl_complex_str (gsl_complex_cos (phi)));
    }
  else if (strcmp (arg, "tan") == 0)
    {
      GslComplex phi;
      phi.re = atof (pshift ());
      phi.im = atof (pshift ());
      g_print ("tan(%s) = %s\n",
	       gsl_complex_str (phi),
	       gsl_complex_str (gsl_complex_tan (phi)));
    }
  else if (strcmp (arg, "sinh") == 0)
    {
      GslComplex phi;
      phi.re = atof (pshift ());
      phi.im = atof (pshift ());
      g_print ("sinh(%s) = %s\n",
	       gsl_complex_str (phi),
	       gsl_complex_str (gsl_complex_sinh (phi)));
    }
  else if (strcmp (arg, "cosh") == 0)
    {
      GslComplex phi;
      phi.re = atof (pshift ());
      phi.im = atof (pshift ());
      g_print ("cosh(%s) = %s\n",
	       gsl_complex_str (phi),
	       gsl_complex_str (gsl_complex_cosh (phi)));
    }
  else if (strcmp (arg, "tanh") == 0)
    {
      GslComplex phi;
      phi.re = atof (pshift ());
      phi.im = atof (pshift ());
      g_print ("tanh(%s) = %s\n",
	       gsl_complex_str (phi),
	       gsl_complex_str (gsl_complex_tanh (phi)));
    }
  else if (strcmp (arg, "midi2freq") == 0)
    {
      gint note;
      note = atol (pshift ());
      note = CLAMP (note, 0, 128);
      g_print ("midi2freq(%u) = %f\n",
	       note,
	       gsl_temp_freq (gsl_get_config ()->kammer_freq,
			      note - gsl_get_config ()->midi_kammer_note));
    }
  else if (strcmp (arg, "butter") == 0)
    {
      guint order;
      double f, e;
      order = atoi (pshift ()); order = MAX (order, 1);
      f = atof (pshift ());
      e = atof (pshift ());
      f *= GSL_PI / 2.;
      e = gsl_trans_zepsilon2ss (e);
      {
        double a[order + 1], b[order + 1];
        gsl_filter_butter (order, f, e, a, b);
        g_print ("# Butterworth filter order=%u freq=%f epsilon(s^2)=%f norm0=%f:\n",
                 order, f, e,
                 gsl_poly_eval (order, a, 1) / gsl_poly_eval (order, b, 1));
        g_print ("H%u(z)=%s/%s\n", order,
                 gsl_poly_str (order, a, "z"),
                 gsl_poly_str (order, b, "z"));
      }
    }
  else if (strcmp (arg, "t1") == 0)
    {
      guint order;
      double f, e;
      order = atoi (pshift ()); order = MAX (order, 1);
      f = atof (pshift ());
      e = atof (pshift ());
      f *= GSL_PI / 2.;
      e = gsl_trans_zepsilon2ss (e);
      {
	double a[order + 1], b[order + 1];
	gsl_filter_tscheb1 (order, f, e, a, b);
	g_print ("# Tschebyscheff Type1 order=%u freq=%f epsilon(s^2)=%f norm0=%f:\n",
		 order, f, e,
		 gsl_poly_eval (order, a, 1) / gsl_poly_eval (order, b, 1));
	g_print ("H%u(z)=%s/%s\n", order,
		 gsl_poly_str (order, a, "z"),
		 gsl_poly_str (order, b, "z"));
      }
    }
  else if (strcmp (arg, "t2") == 0)
    {
      guint order;
      double fc, fr, e;
      order = atoi (pshift ()); order = MAX (order, 1);
      fc = atof (pshift ());
      fr = atof (pshift ());
      e = atof (pshift ());
      fc *= GSL_PI / 2.;
      fr *= GSL_PI / 2.;
      e = gsl_trans_zepsilon2ss (e);
      {
	double a[order + 1], b[order + 1];
	gsl_filter_tscheb2 (order, fc, fr, e, a, b);
	g_print ("# Tschebyscheff Type2 order=%u freq_c=%f freq_r=%f epsilon(s^2)=%f norm=%f:\n",
		 order, fc, fr, e,
		 gsl_poly_eval (order, a, 1) / gsl_poly_eval (order, b, 1));
	g_print ("H%u(z)=%s/%s\n", order,
		 gsl_poly_str (order, a, "z"),
		 gsl_poly_str (order, b, "z"));
      }
    }
  else if (strncmp (arg, "poly", 4) == 0)
    {
      guint order;
      arg = arg + 4;
      order = 2;
      {
	double a[100] = { 1, 2, 1 }, b[100] = { 1, -3./2., 0.5 };
	g_print ("# Test order=%u  norm=%f:\n",
		 order,
		 gsl_poly_eval (order, a, 1) / gsl_poly_eval (order, b, 1));
	g_print ("H%u(z)=%s/%s\n", order,
		 gsl_poly_str (order, a, "z"),
		 gsl_poly_str (order, b, "z"));
	if (*arg)
	  {
	    GslComplex root, roots[100];
	    guint i;
	    
	    if (*arg == 'r')
	      {
		g_print ("#roots:\n");
		gsl_poly_complex_roots (order, a, roots);
		for (i = 0; i < order; i++)
		  {
		    root = gsl_complex_div (gsl_complex (1, 0), roots[i]);
		    g_print ("%+.14f %+.14f # %.14f\n", root.re, root.im, gsl_complex_abs (root));
		  }
	      }
	    if (*arg == 'p')
	      {
		g_print ("#poles:\n");
		gsl_poly_complex_roots (order, b, roots);
		for (i = 0; i < order; i++)
		  {
		    root = gsl_complex_div (gsl_complex (1, 0), roots[i]);
		    g_print ("%+.14f %+.14f # %.14f\n", root.re, root.im, gsl_complex_abs (root));
		  }
	      }
	  }
      }
    }
  else
    usage ();
  
  arg = shift ();
  if (arg)
    goto restart;
  
  return 0;
}

static void
usage (void)
{
  g_print ("usage: gsltests {test} [args...]\n");
  g_print ("tests:\n");
  g_print ("  gslwave-scan <file>       scan a gslwave file for waves\n");
  g_print ("  file-test <file>          test file properties\n");
  g_print ("  rf <x> <y> <z>            Carlson's elliptic integral of the first kind\n");
  g_print ("  F <phi> <ak>              Legendre elliptic integral of the 1st kind\n");
  g_print ("  sn <u> <emmc>             Jacobian elliptic function sn()\n");
  g_print ("  asn <y> <emmc>            elliptic integral, inverse sn()\n");
  g_print ("  sin <phi.re> <phi.im>     complex sine\n");
  g_print ("  cos <phi.re> <phi.im>     complex cosine\n");
  g_print ("  tan <phi.re> <phi.im>     complex tangent\n");
  g_print ("  sinh <phi.re> <phi.im>    complex hyperbolic sine\n");
  g_print ("  cosh <phi.re> <phi.im>    complex hyperbolic cosine\n");
  g_print ("  tanh <phi.re> <phi.im>    complex hyperbolic tangent\n");
  g_print ("  midi2freq <midinote>      convert midinote into oscilaltor frequency\n");
  g_print ("  snc <u.re> <u.im> <emmc.re> <emmc.im>     sn() for complex numbers\n");
  g_print ("  asnc <y.re> <y.im> <emmc.re> <emmc.im>    asn() for complex numbers\n");
  g_print ("  sci_sn <u> <k2>                           scilab version of sn()\n");
  g_print ("  sci_asn <y> <k2>                          scilab version of asn()\n");
  g_print ("  sci_snc <u.re> <u.im> <k2.re> <k2.im>     scilab version of snc()\n");
  g_print ("  sci_asnc <y.re> <y.im> <k2.re> <k2.im>    scilab version of asnc()\n");
  g_print ("  butter <order> <freq> <epsilon>           butterworth filter\n");
  g_print ("  t1 <order> <freq> <epsilon>               type1 tschebyscheff filter\n");
  g_print ("  t2 <order> <freqc> <freqr> <epsilon>      type2 tschebyscheff filter\n");
  g_print ("  poly | polyr | polyp                      polynom test (+roots or +poles)\n");
  exit (1);
}
