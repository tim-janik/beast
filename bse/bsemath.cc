// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsemath.hh"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define RING_BUFFER_LENGTH	(256) // FIXME: simlpy dup strings in the API
#define	PRINTF_DIGITS		"1270"
#define	FLOAT_STRING_SIZE	(2048)
/* --- functions --- */
static inline char*
pretty_print_double (char  *str,
		     double d)
{
  char *s= str;
  sprintf (s, "%."PRINTF_DIGITS"f", d);
  while (*s)
    s++;
  while (s[-1] == '0' && s[-2] != '.')
    s--;
  *s = 0;
  return s;
}
char*
bse_complex_list (uint         n_points,
		  BseComplex  *points,
		  const char  *indent)
{
  static uint rbi = 0;
  static char* rbuffer[RING_BUFFER_LENGTH] = { NULL, };
  char *s, *tbuffer = g_newa (char, (FLOAT_STRING_SIZE * 2 * n_points));
  uint i;
  rbi = (rbi + 1) % RING_BUFFER_LENGTH;
  if (rbuffer[rbi] != NULL)
    g_free (rbuffer[rbi]);
  s = tbuffer;
  for (i = 0; i < n_points; i++)
    {
      *s = 0;
      if (indent)
	strcat (s, indent);
      while (*s) s++;
      s = pretty_print_double (s, points[i].re);
      *s++ = ' ';
      s = pretty_print_double (s, points[i].im);
      *s++ = '\n';
    }
  *s++ = 0;
  rbuffer[rbi] = g_strdup (tbuffer);
  return rbuffer[rbi];
}
char*
bse_complex_str (BseComplex c)
{
  static uint rbi = 0;
  static char* rbuffer[RING_BUFFER_LENGTH] = { NULL, };
  char *s, tbuffer[FLOAT_STRING_SIZE * 2];
  rbi = (rbi + 1) % RING_BUFFER_LENGTH;
  if (rbuffer[rbi] != NULL)
    g_free (rbuffer[rbi]);
  s = tbuffer;
  *s++ = '{';
  s = pretty_print_double (s, c.re);
  *s++ = ',';
  *s++ = ' ';
  s = pretty_print_double (s, c.im);
  *s++ = '}';
  *s++ = 0;
  rbuffer[rbi] = g_strdup (tbuffer);
  return rbuffer[rbi];
}
char*
bse_poly_str (uint         degree,
	      double      *a,
	      const char  *var)
{
  static uint rbi = 0;
  static char* rbuffer[RING_BUFFER_LENGTH] = { NULL, };
  char *s, *tbuffer = g_newa (char, degree * FLOAT_STRING_SIZE);
  uint i;
  if (!var)
    var = "x";
  rbi = (rbi + 1) % RING_BUFFER_LENGTH;
  if (rbuffer[rbi] != NULL)
    g_free (rbuffer[rbi]);
  s = tbuffer;
  *s++ = '(';
  s = pretty_print_double (s, a[0]);
  for (i = 1; i <= degree; i++)
    {
      *s++ = '+';
      *s = 0; strcat (s, var); while (*s) s++;
      *s++ = '*';
      *s++ = '(';
      s = pretty_print_double (s, a[i]);
    }
  while (i--)
    *s++ = ')';
  *s++ = 0;
  rbuffer[rbi] = g_strdup (tbuffer);
  return rbuffer[rbi];
}
char*
bse_poly_str1 (uint         degree,
	       double      *a,
	       const char  *var)
{
  static uint rbi = 0;
  static char* rbuffer[RING_BUFFER_LENGTH] = { NULL, };
  char *s, *tbuffer = g_newa (char, degree * FLOAT_STRING_SIZE);
  uint i, need_plus = 0;
  if (!var)
    var = "x";
  rbi = (rbi + 1) % RING_BUFFER_LENGTH;
  if (rbuffer[rbi] != NULL)
    g_free (rbuffer[rbi]);
  s = tbuffer;
  *s++ = '(';
  if (a[0] != 0.0)
    {
      s = pretty_print_double (s, a[0]);
      need_plus = 1;
    }
  for (i = 1; i <= degree; i++)
    {
      if (a[i] == 0.0)
	continue;
      if (need_plus)
	{
	  *s++ = ' ';
	  *s++ = '+';
	  *s++ = ' ';
	}
      if (a[i] != 1.0)
	{
	  s = pretty_print_double (s, a[i]);
	  *s++ = '*';
	}
      *s = 0;
      strcat (s, var);
      while (*s) s++;
      if (i > 1)
	{
	  *s++ = '*';
	  *s++ = '*';
	  sprintf (s, "%u", i);
	  while (*s) s++;
	}
      need_plus = 1;
    }
  *s++ = ')';
  *s++ = 0;
  rbuffer[rbi] = g_strdup (tbuffer);
  return rbuffer[rbi];
}
void
bse_complex_gnuplot (const char  *file_name,
		     uint         n_points,
		     BseComplex  *points)
{
  FILE *fout = fopen (file_name, "w");
  fputs (bse_complex_list (n_points, points, ""), fout);
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
    {
      char xstr[FLOAT_STRING_SIZE], ystr[FLOAT_STRING_SIZE];
      pretty_print_double (xstr, xstart + i * xstep);
      pretty_print_double (ystr, ypoints[i]);
      fprintf (fout, "%s %s\n", xstr, ystr);
    }
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
