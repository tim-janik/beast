/* GSL - Generic Sound Layer
 * Copyright (C) 2001 Stefan Westerfeld and Tim Janik
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
#include <gsl/gslfilter.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define	PREC	"15"

static void
usage (char *s)
{
  printf ("usage: gslmathtest %s\n", s);
  exit (1);
}

int
main (int   argc,
      char *argv[])
{
  gchar *arg;

  if (argc < 2)
    goto abort;

  if (strcmp (argv[1], "rf") == 0)
    {
      double x, y, z;
      if (argc != 5)
	usage ("rf <x> <y> <z>");
      x = atof (argv[2]);
      y = atof (argv[3]);
      z = atof (argv[4]);
      
      printf ("rf(%f, %f, %f) = %."PREC"f\n", x, y, z, gsl_ellip_rf (x, y, z));
    }
  else if (strcmp (argv[1], "F") == 0)
    {
      double phi, ak;
      if (argc != 4)
	usage ("F <phi> <ak>");
      phi = atof (argv[2]);
      ak = atof (argv[3]);
      
      printf ("F(%f, %f) = %."PREC"f\n", phi, ak, gsl_ellip_F (phi, ak));
    }
  else if (strcmp (argv[1], "sn") == 0)
    {
      double u, emmc;
      if (argc != 4)
	usage ("sn <u> <emmc>");
      u = atof (argv[2]);
      emmc = atof (argv[3]);
      
      printf ("sn(%f, %f) = %."PREC"f\n", u, emmc, gsl_ellip_sn (u, emmc));
    }
  else if (strcmp (argv[1], "snc") == 0)
    {
      GslComplex u, emmc;
      if (argc != 6)
	usage ("sn <u.re> <u.im> <emmc.re> <emmc.im>");
      u.re = atof (argv[2]);
      u.im = atof (argv[3]);
      emmc.re = atof (argv[4]);
      emmc.im = atof (argv[5]);
      
      printf ("snc(%s, %s) = %s\n",
	      gsl_complex_str (u),
	      gsl_complex_str (emmc),
	      gsl_complex_str (gsl_complex_ellip_sn (u, emmc)));
    }
  else if (strcmp (argv[1], "sci_snc") == 0)
    {
      GslComplex u, k2;
      if (argc != 6)
	usage ("sci_sn <u.re> <u.im> <k2.re> <k2.im>");
      u.re = atof (argv[2]);
      u.im = atof (argv[3]);
      k2.re = atof (argv[4]);
      k2.im = atof (argv[5]);
      
      printf ("sci_snc(%s, %s) = %s\n",
	      gsl_complex_str (u),
	      gsl_complex_str (k2),
	      gsl_complex_str (gsl_complex_ellip_sn (u, gsl_complex_sub (gsl_complex (1.0, 0), k2))));
    }
  else if (strcmp (argv[1], "asn") == 0)
    {
      double y, emmc;
      if (argc != 4)
	usage ("asn <y> <emmc>");
      y = atof (argv[2]);
      emmc = atof (argv[3]);
      
      printf ("asn(%f, %f) = %."PREC"f\n", y, emmc, gsl_ellip_asn (y, emmc));
    }
  else if (strcmp (argv[1], "asnc") == 0)
    {
      GslComplex y, emmc;
      if (argc != 6)
	usage ("asnc <y.re> <y.im> <emmc.re> <emmc.im>");
      y.re = atof (argv[2]);
      y.im = atof (argv[3]);
      emmc.re = atof (argv[4]);
      emmc.im = atof (argv[5]);
      
      printf ("asnc(%s, %s) = %s\n",
	      gsl_complex_str (y), gsl_complex_str (emmc),
	      gsl_complex_str (gsl_complex_ellip_asn (y, emmc)));
      printf ("asn(%f, %f = %."PREC"f\n",
	      y.re, emmc.re, gsl_ellip_asn (y.re, emmc.re));
    }
  else if (strcmp (argv[1], "sci_sn") == 0)
    {
      double u, k2;
      if (argc != 4)
	usage ("sci_sn <u> <k2>");
      u = atof (argv[2]);
      k2 = atof (argv[3]);
      
      printf ("sci_sn(%f, %f) = %."PREC"f\n", u, k2, gsl_ellip_sn (u, 1.0 - k2));
    }
  else if (strcmp (argv[1], "sci_asn") == 0)
    {
      double y, k2;
      if (argc != 4)
	usage ("sci_asn <y> <k2>");
      y = atof (argv[2]);
      k2 = atof (argv[3]);
      
      printf ("sci_asn(%f, %f) = %."PREC"f\n", y, k2, gsl_ellip_asn (y, 1.0 - k2));
    }
  else if (strcmp (argv[1], "sci_asnc") == 0)
    {
      GslComplex y, k2;
      if (argc != 6)
	usage ("sci_asnc <y.re> <y.im> <k2.re> <k2.im>");
      y.re = atof (argv[2]);
      y.im = atof (argv[3]);
      k2.re = atof (argv[4]);
      k2.im = atof (argv[5]);
      
      printf ("sci_asnc(%s, %s) = %s\n",
	      gsl_complex_str (y), gsl_complex_str (k2),
	      gsl_complex_str (gsl_complex_ellip_asn (y, gsl_complex_sub (gsl_complex (1.0, 0), k2))));
      printf ("asn(%f, %f = %."PREC"f\n",
	      y.re, k2.re, gsl_ellip_asn (y.re, 1.0 - k2.re));
    }
  else if (strcmp (argv[1], "sin") == 0)
    {
      GslComplex phi;
      if (argc != 4)
	usage ("sin <phi.re> <phi.im>");
      phi.re = atof (argv[2]);
      phi.im = atof (argv[3]);
      
      printf ("sin(%s) = %s\n",
	      gsl_complex_str (phi),
	      gsl_complex_str (gsl_complex_sin (phi)));
    }
  else if (strcmp (argv[1], "cos") == 0)
    {
      GslComplex phi;
      if (argc != 4)
	usage ("cos <phi.re> <phi.im>");
      phi.re = atof (argv[2]);
      phi.im = atof (argv[3]);
      
      printf ("cos(%s) = %s\n",
	      gsl_complex_str (phi),
	      gsl_complex_str (gsl_complex_cos (phi)));
    }
  else if (strcmp (argv[1], "tan") == 0)
    {
      GslComplex phi;
      if (argc != 4)
	usage ("tan <phi.re> <phi.im>");
      phi.re = atof (argv[2]);
      phi.im = atof (argv[3]);
      
      printf ("tan(%s) = %s\n",
	      gsl_complex_str (phi),
	      gsl_complex_str (gsl_complex_tan (phi)));
    }
  else if (strcmp (argv[1], "sinh") == 0)
    {
      GslComplex phi;
      if (argc != 4)
	usage ("sinh <phi.re> <phi.im>");
      phi.re = atof (argv[2]);
      phi.im = atof (argv[3]);
      
      printf ("sinh(%s) = %s\n",
	      gsl_complex_str (phi),
	      gsl_complex_str (gsl_complex_sinh (phi)));
    }
  else if (strcmp (argv[1], "cosh") == 0)
    {
      GslComplex phi;
      if (argc != 4)
	usage ("cosh <phi.re> <phi.im>");
      phi.re = atof (argv[2]);
      phi.im = atof (argv[3]);
      
      printf ("cosh(%s) = %s\n",
	      gsl_complex_str (phi),
	      gsl_complex_str (gsl_complex_cosh (phi)));
    }
  else if (strcmp (argv[1], "tanh") == 0)
    {
      GslComplex phi;
      if (argc != 4)
	usage ("tanh <phi.re> <phi.im>");
      phi.re = atof (argv[2]);
      phi.im = atof (argv[3]);
      
      printf ("tanh(%s) = %s\n",
	      gsl_complex_str (phi),
	      gsl_complex_str (gsl_complex_tanh (phi)));
    }
  else if (strcmp (argv[1], "t1") == 0)
    {
      guint order;
      double f, e;
      if (argc != 5)
	usage ("t1 <order> <freq> <epsilon>");
      order = atoi (argv[2]);
      f = atof (argv[3]);
      e = atof (argv[4]);
      f *= GSL_PI / 2.;
      e = gsl_trans_zepsilon2ss (e);
      {
	double a[order + 1], b[order + 1];
	gsl_filter_tscheb1 (order, f, e, a, b);
	g_print ("# Tschebyscheff Type1 order=%u freq=%f s^2epsilon=%f norm0=%f:\n",
		 order, f, e,
		 gsl_poly_eval (order, a, 1) / gsl_poly_eval (order, b, 1));
	g_print ("H%u(z)=%s/%s\n", order,
		 gsl_poly_str (order, a, "z"),
		 gsl_poly_str (order, b, "z"));
      }
    }
  else if (strcmp (argv[1], "t2") == 0)
    {
      guint order;
      double fc, fr, e;
      if (argc != 6)
	usage ("t1 <order> <freqc> <freqr> <epsilon>");
      order = atoi (argv[2]);
      fc = atof (argv[3]);
      fr = atof (argv[4]);
      e = atof (argv[5]);
      fc *= GSL_PI / 2.;
      fr *= GSL_PI / 2.;
      e = gsl_trans_zepsilon2ss (e);
      {
	double a[order + 1], b[order + 1];
	gsl_filter_tscheb2 (order, fc, fr, e, a, b);
	g_print ("# Tschebyscheff Type2 order=%u freq_c=%f freq_r=%f s^2epsilon=%f norm=%f:\n",
		 order, fc, fr, e,
		 gsl_poly_eval (order, a, 1) / gsl_poly_eval (order, b, 1));
	g_print ("H%u(z)=%s/%s\n", order,
		 gsl_poly_str (order, a, "z"),
		 gsl_poly_str (order, b, "z"));
      }
    }
  else if (strncmp (argv[1], "test", 4) == 0)
    {
      guint order;
      arg = argv[1] + 4;
      if (argc != argc)
	usage ("test");
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
    {
    abort:
      usage ("{rf|F|sn|snc|sci_sn|sci_snc|asn|asnc|sci_asn|sci_asnc|sin(h)|cos(h)|tan(h)|t1|t2} ...");
    }
      
  return 0;
}
