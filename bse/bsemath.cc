// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsemath.hh"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* --- functions --- */
std::string
bse_string_from_double (long double value)
{
  std::string str = Bse::string_format ("%.1270f", value);
  const char *s = &str[str.size()];
  while (s > &str[0] && s[-1] == '0' && s[-2] != '.')
    s--;
  return str.substr (0, s - &str[0]);
}

std::string
bse_complex_list (uint n_points, BseComplex *points, const std::string &indent)
{
  std::string string;
  for (uint i = 0; i < n_points; i++)
    {
      string += indent;
      string += bse_string_from_double (points[i].re);
      string += ' ';
      string += bse_string_from_double (points[i].im);
      string += '\n';
    }
  return string;
}

std::string
bse_complex_str (BseComplex c)
{
  std::string s;
  s += '{';
  s += bse_string_from_double (c.re);
  s += ", ";
  s += bse_string_from_double (c.im);
  s += '}';
  return s;
}

std::string
bse_poly_str (uint degree, double *a, const std::string &uvar)
{
  const std::string var = uvar.empty() ? "x" : uvar;
  std::string s;
  s += '(';
  s += bse_string_from_double (a[0]);
  uint i;
  for (i = 1; i <= degree; i++)
    {
      s += '+';
      s += var;
      s += "*(";
      s += bse_string_from_double (a[i]);
    }
  while (i--)
    s += ')';
  return s;
}

std::string
bse_poly_str1 (uint degree, double *a, const std::string &uvar)
{
  const std::string var = uvar.empty() ? "x" : uvar;
  std::string s;
  bool need_plus = 0;
  s += '(';
  if (a[0] != 0.0)
    {
      s += bse_string_from_double (a[0]);
      need_plus = true;
    }
  uint i;
  for (i = 1; i <= degree; i++)
    {
      if (a[i] == 0.0)
	continue;
      if (need_plus)
        s += " + ";
      if (a[i] != 1.0)
	{
          s += bse_string_from_double (a[i]);
	  s += '*';
	}
      s += var;
      if (i > 1)
        s += Bse::string_format ("**%u", i);
      need_plus = true;
    }
  s += ')';
  return s;
}

void
bse_complex_gnuplot (const char  *file_name,
		     uint         n_points,
		     BseComplex  *points)
{
  FILE *fout = fopen (file_name, "w");

  fputs (bse_complex_list (n_points, points, "").c_str(), fout);
  fclose (fout);
}

void
bse_float_gnuplot (const char    *file_name,
                   double         xstart,
                   double         xstep,
                   uint           n_ypoints,
                   const float   *ypoints)
{
  FILE *fout = fopen (file_name, "w");
  uint i;
  for (i = 0; i < n_ypoints; i++)
    fprintf (fout, "%s %s\n", bse_string_from_double (xstart + i * xstep).c_str(), bse_string_from_double (ypoints[i]).c_str());
  fclose (fout);
}

double
bse_temp_freq (double kammer_freq,
	       int    semitone_delta)
{
  double factor;

  factor = pow (BSE_2_POW_1_DIV_12, semitone_delta);

  return kammer_freq * factor;
}

void
bse_poly_from_re_roots (uint         degree,
			double      *a,
			BseComplex  *roots)
{
  uint i;

  /* initialize polynomial */
  a[1] = 1;
  a[0] = -roots[0].re;
  /* monomial factor multiplication */
  for (i = 1; i < degree; i++)
    {
      uint j;

      a[i + 1] = a[i];
      for (j = i; j >= 1; j--)
	a[j] = a[j - 1] - a[j] * roots[i].re;
      a[0] *= -roots[i].re;
    }
}

void
bse_cpoly_from_roots (uint         degree,
		      BseComplex  *c,
		      BseComplex  *roots)
{
  uint i;

  /* initialize polynomial */
  c[1].re = 1;
  c[1].im = 0;
  c[0].re = -roots[0].re;
  c[0].im = -roots[0].im;
  /* monomial factor multiplication */
  for (i = 1; i < degree; i++)
    {
      BseComplex r = bse_complex (-roots[i].re, -roots[i].im);
      uint j;

      c[i + 1] = c[i];
      for (j = i; j >= 1; j--)
	c[j] = bse_complex_add (c[j - 1], bse_complex_mul (c[j], r));
      c[0] = bse_complex_mul (c[0], r);
    }
}

gboolean
bse_poly2_droots (double roots[2],
		  double a,
		  double b,
		  double c)
{
  double square = b * b - 4.0 * a * c;
  double tmp;

  if (square < 0)
    return FALSE;

  if (b > 0)
    tmp = -b - sqrt (square);
  else
    tmp = -b + sqrt (square);

  roots[0] = tmp / (a + a);
  roots[1] = (c + c) / tmp;

  return TRUE;
}

double
bse_bit_depth_epsilon (uint n_bits)
{
  /* epsilon for various bit depths, based on significance of one bit,
   * minus fudge. created with:
   * { echo "scale=40"; for i in `seq 1 32` ; do echo "1/2^$i - 10^-($i+1)" ; done } | bc | sed 's/$/,/'
   */
  static const double bit_epsilons[] = {
    .4900000000000000000000000000000000000000,
    .2490000000000000000000000000000000000000,
    .1249000000000000000000000000000000000000,
    .0624900000000000000000000000000000000000,
    .0312490000000000000000000000000000000000,
    .0156249000000000000000000000000000000000,
    .0078124900000000000000000000000000000000,
    .0039062490000000000000000000000000000000,
    .0019531249000000000000000000000000000000,
    .0009765624900000000000000000000000000000,
    .0004882812490000000000000000000000000000,
    .0002441406249000000000000000000000000000,
    .0001220703124900000000000000000000000000,
    .0000610351562490000000000000000000000000,
    .0000305175781249000000000000000000000000,
    .0000152587890624900000000000000000000000,
    .0000076293945312490000000000000000000000,
    .0000038146972656249000000000000000000000,
    .0000019073486328124900000000000000000000,
    .0000009536743164062490000000000000000000,
    .0000004768371582031249000000000000000000,
    .0000002384185791015624900000000000000000,
    .0000001192092895507812490000000000000000,
    .0000000596046447753906249000000000000000,
    .0000000298023223876953124900000000000000,
    .0000000149011611938476562490000000000000,
    .0000000074505805969238281249000000000000,
    .0000000037252902984619140624900000000000,
    .0000000018626451492309570312490000000000,
    .0000000009313225746154785156249000000000,
    .0000000004656612873077392578124900000000,
    .0000000002328306436538696289062490000000,
  };

  return bit_epsilons[CLAMP (n_bits, 1, 32) - 1];
}

int
bse_rand_bool (void)
{
  return rand () & 1;	// FIXME
}
