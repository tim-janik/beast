/* GSL - Generic Sound Layer
 * Copyright (C) 2001 Stefan Westerfeld and Tim Janik
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
#include "gslmath.h"

#include <string.h>
#include <malloc.h>
#include <stdio.h>


#define RING_BUFFER_LENGTH	(16)
#define	PRINTF_DIGITS		"12" /* "1270" */
#define	FLOAT_STRING_SIZE	(2048)


/* factorization constants: 2^(1/12), ln(2^(1/12)) and 2^(1/(12*6))
 * retrived with:
 #include <stl.h>
 #include <complex.h>
 typedef long double ld;

 int main (void)
 {
 ld r, l;

 cout.precision(256);

 r = pow ((ld) 2, (ld) 1 / (ld) 12);
 cout << "2^(1/12) =\n";
 cout << "2^" << (ld) 1 / (ld) 12 << " =\n";
 cout << r << "\n";

 l = log (r);
 cout << "ln(2^(1/12)) =\n";
 cout << "ln(" << r << ") =\n";
 cout << l << "\n";

 r = pow ((ld) 2, (ld) 1 / (ld) 72);
 cout << "2^(1/72) =\n";
 cout << "2^" << (ld) 1 / (ld) 72 << " =\n";
 cout << r << "\n";

 return 0;
 }
*/


/* --- functions --- */
static inline char*
pretty_print_double (char  *str,
		     double d)
{
  char *s= str;

  sprintf (s, "%."PRINTF_DIGITS"f", d);
  while (*s) s++;
  while (s[-1] == '0' && s[-2] != '.')
    s--;
  *s = 0;
  return s;
}

char*
gsl_complex_list (unsigned int n_points,
		  GslComplex  *points,
		  const char  *indent)
{
  static unsigned int rbi = 0;
  static char* rbuffer[RING_BUFFER_LENGTH] = { NULL, };
  char *s, tbuffer[FLOAT_STRING_SIZE * 2 * n_points];
  unsigned int i;

  rbi++; if (rbi >= RING_BUFFER_LENGTH) rbi -= RING_BUFFER_LENGTH;
  if (rbuffer[rbi] != NULL)
    free (rbuffer[rbi]);
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
  rbuffer[rbi] = strdup (tbuffer);
  return rbuffer[rbi];
}

char*
gsl_complex_str (GslComplex c)
{
  static unsigned int rbi = 0;
  static char* rbuffer[RING_BUFFER_LENGTH] = { NULL, };
  char *s, tbuffer[FLOAT_STRING_SIZE * 2];

  rbi++; if (rbi >= RING_BUFFER_LENGTH) rbi -= RING_BUFFER_LENGTH;
  if (rbuffer[rbi] != NULL)
    free (rbuffer[rbi]);
  s = tbuffer;
  *s++ = '{';
  s = pretty_print_double (s, c.re);
  *s++ = ',';
  *s++ = ' ';
  s = pretty_print_double (s, c.im);
  *s++ = '}';
  *s++ = 0;
  rbuffer[rbi] = strdup (tbuffer);
  return rbuffer[rbi];
}

char*
gsl_poly_str (unsigned int degree,
	      double      *a,
	      const char  *var)
{
  static unsigned int rbi = 0;
  static char* rbuffer[RING_BUFFER_LENGTH] = { NULL, };
  char *s, tbuffer[degree * FLOAT_STRING_SIZE];
  unsigned int i;

  if (!var)
    var = "x";
  rbi++; if (rbi >= RING_BUFFER_LENGTH) rbi -= RING_BUFFER_LENGTH;
  if (rbuffer[rbi] != NULL)
    free (rbuffer[rbi]);
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
  rbuffer[rbi] = strdup (tbuffer);
  return rbuffer[rbi];
}

char*
gsl_poly_str1 (unsigned int degree,
	       double      *a,
	       const char  *var)
{
  static unsigned int rbi = 0;
  static char* rbuffer[RING_BUFFER_LENGTH] = { NULL, };
  char *s, tbuffer[degree * FLOAT_STRING_SIZE];
  unsigned int i, need_plus = 0;

  if (!var)
    var = "x";
  rbi++; if (rbi >= RING_BUFFER_LENGTH) rbi -= RING_BUFFER_LENGTH;
  if (rbuffer[rbi] != NULL)
    free (rbuffer[rbi]);
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
  rbuffer[rbi] = strdup (tbuffer);
  return rbuffer[rbi];
}

void
gsl_complex_gnuplot (const char  *file_name,
		     unsigned int n_points,
		     GslComplex  *points)
{
  FILE *fout = fopen (file_name, "w");

  fputs (gsl_complex_list (n_points, points, ""), fout);
  fclose (fout);
}

double
gsl_temp_freq (double kammer_freq,
	       int    halftone_delta)
{
  double factor;

  factor = pow (GSL_2_RAISED_TO_1_OVER_12_d, halftone_delta);

  return kammer_freq * factor;
}

void
gsl_poly_from_re_roots (unsigned int degree,
			double      *a,
			GslComplex  *roots)
{
  unsigned int i;

  /* initialize polynomial */
  a[1] = 1;
  a[0] = -roots[0].re;
  /* monomial factor multiplication */
  for (i = 1; i < degree; i++)
    {
      unsigned int j;

      a[i + 1] = a[i];
      for (j = i; j >= 1; j--)
	a[j] = a[j - 1] - a[j] * roots[i].re;
      a[0] *= -roots[i].re;
    }
}

void
gsl_cpoly_from_roots (unsigned int degree,
		      GslComplex  *c,
		      GslComplex  *roots)
{
  unsigned int i;

  /* initialize polynomial */
  c[1].re = 1;
  c[1].im = 0;
  c[0].re = -roots[0].re;
  c[0].im = -roots[0].im;
  /* monomial factor multiplication */
  for (i = 1; i < degree; i++)
    {
      GslComplex r = gsl_complex (-roots[i].re, -roots[i].im);
      unsigned int j;

      c[i + 1] = c[i];
      for (j = i; j >= 1; j--)
	c[j] = gsl_complex_add (c[j - 1], gsl_complex_mul (c[j], r));
      c[0] = gsl_complex_mul (c[0], r);
    }
}

void
gsl_poly_complex_roots (unsigned int degree,
			double      *a,		/* [0..degree] (degree+1 elements) */
			GslComplex  *roots)	/* [degree] */
{
  static void zrhqr (double a[], int m, double rtr[], double rti[]);
  double roots_re[1 + degree], roots_im[1 + degree];
  unsigned int i;

  zrhqr (a, degree, roots_re, roots_im);
  for (i = 0; i < degree; i++)
    {
      roots[i].re = roots_re[1 + i];
      roots[i].im = roots_im[1 + i];
    }
}

double
gsl_ellip_rf (double x,
	      double y,
	      double z)
{
  static double rf (double x, double y, double z);

  return rf (x, y, z);
}

double
gsl_ellip_F (double phi,
	     double ak)
{
  static double ellf (double phi, double ak);

  return ellf (phi, ak);
}

double
gsl_ellip_sn (double u,
	      double emmc)
{
  static void sncndn (double uu, double emmc, double *sn_p, double *cn_p, double *dn_p);
  double sn;

  sncndn (u, emmc, &sn, NULL, NULL);
  return sn;
}

double
gsl_ellip_asn (double y,
	       double emmc)
{
  static double rf (double x, double y, double z);

  return y * rf (1.0 - y * y, 1.0 - y * y * (1.0 - emmc), 1.0);
}

GslComplex
gsl_complex_ellip_asn (GslComplex y,
		       GslComplex emmc)
{
  static GslComplex rfC (GslComplex x, GslComplex y, GslComplex z);

  return gsl_complex_mul (y,
			  rfC (gsl_complex_sub (gsl_complex (1.0, 0),
						gsl_complex_mul (y, y)),
			       gsl_complex_sub (gsl_complex (1.0, 0),
						gsl_complex_mul3 (y, y, gsl_complex_sub (gsl_complex (1.0, 0),
											 emmc))),
			       gsl_complex (1.0, 0)));
}

GslComplex
gsl_complex_ellip_sn (GslComplex u,
		      GslComplex emmc)
{
  static void sncndnC (GslComplex uu, GslComplex emmc, GslComplex *sn_p, GslComplex *cn_p, GslComplex *dn_p);
  GslComplex sn;

  sncndnC (u, emmc, &sn, NULL, NULL);
  return sn;
}


/* --- Numerical Receipes --- */
#define	gsl_complex_rmul(scale, c)	gsl_complex_scale (c, scale)
#define	ONE				gsl_complex (1.0, 0)
#define	SIGN(a,b)	((b) >= 0.0 ? fabs (a) : -fabs(a))
static inline int IMAX (int i1, int i2) { return i1 > i2 ? i1 : i2; }
static inline double DMIN (double d1, double d2) { return d1 < d2 ? d1 : d2; }
static inline double DMAX (double d1, double d2) { return d1 > d2 ? d1 : d2; }
static inline double DSQR (double d) { return d == 0.0 ? 0.0 : d * d; }
#define nrerror(error)	g_error ("NR-ERROR: %s", (error))
static inline double* vector (long nl, long nh)
     /* allocate a vector with subscript range v[nl..nh] */
{
  double *v = malloc ((nh - nl + 1 + 1) * sizeof (*v));
  if (!v) nrerror ("vector allocation failed");
  return v - nl + 1;
}
static inline void free_vector (double *v, long nl, long nh)
{
  free (v + nl - 1);
}
static inline double** matrix (long nrl, long nrh, long ncl, long nch)
     /* allocate a matrix with subscript range m[nrl..nrh][ncl..nch] */
{
  long i, nrow = nrh - nrl + 1, ncol = nch - ncl + 1;
  double **m = malloc ((nrow + 1) * sizeof (*m));
  if (!m) nrerror ("matrix allocation failed");
  m += 1;
  m -= nrl;
  m[nrl] = malloc ((nrow * ncol + 1) * sizeof (**m));
  if (!m[nrl]) nrerror ("matrix allocation failed");
  m[nrl] += 1;
  m[nrl] -= ncl;
  for (i = nrl + 1; i <= nrh; i++)
    m[i] = m[i - 1] + ncol;
  return m;
}
static inline void free_matrix (double **m, long nrl, long nrh, long ncl, long nch)
{
  free (m[nrl] + ncl - 1);
  free (m + nrl - 1);
}

static void
poldiv (double u[], int n, double v[], int nv, double q[], double r[])
     /* Given the n+1 coefficients of a polynomial of degree n in u[0..n], and the nv+1 coefficients
	of another polynomial of degree nv in v[0..nv], divide the polynomial u by the polynomial
	v ("u"/"v") giving a quotient polynomial whose coefficients are returned in q[0..n], and a
	remainder polynomial whose coefficients are returned in r[0..n]. The elements r[nv..n]
	and q[n-nv+1..n] are returned as zero. */
{
  int k,j;
  
  for (j=0;j<=n;j++) {
    r[j]=u[j];
    q[j]=0.0;
  }for (k=n-nv;k>=0;k--) {
    q[k]=r[nv+k]/v[nv];
    for (j=nv+k-1;j>=k;j--) r[j] -= q[k]*v[j-k];
  }for (j=nv;j<=n;j++) r[j]=0.0;
}

#define	MAX_ITER_BASE	9	/* TIMJ: was 3 */
#define	MAX_ITER_FAC	20	/* TIMJ: was 10 */
static void
hqr (double **a, int n, double wr[], double wi[])
     /* Finds all eigenvalues of an upper Hessenberg matrix a[1..n][1..n]. On input a can be
	exactly as output from elmhes §11.5; on output it is destroyed. The real and imaginary parts
	of the eigenvalues are returned in wr[1..n] and wi[1..n], respectively. */
{
  int nn,m,l,k,j,its,i,mmin;
  double z,y,x,w,v,u,t,s,r,q,p,anorm;
  r=q=p=0; /* TIMJ: silence compiler */
  
  anorm=0.0;                                  /* Compute matrix norm for possible use in lo- */
  for (i=1;i<=n;i++)                          /* cating single small subdiagonal element. */
    for (j=IMAX (i-1,1);j<=n;j++)
      anorm += fabs (a[i][j]);
  nn=n;
  t=0.0;                                      /* Gets changed only by an exceptional shift. */
  while (nn >= 1) {                           /* Begin search for next eigenvalue. */
    its=0;
    do {for (l=nn;l>=2;l--) {                 /* Begin iteration: look for single small subdi- */
      s=fabs (a[l-1][l-1])+fabs (a[l][l]);      /* agonal element. */
      if (s == 0.0) s=anorm;
      if ((double)(fabs (a[l][l-1]) + s) == s) break;
    }
    x=a[nn][nn];
    if (l == nn) {                    /* One root found. */
      wr[nn]=x+t;
      wi[nn--]=0.0;
    } else {
      y=a[nn-1][nn-1];
      w=a[nn][nn-1]*a[nn-1][nn];
      if (l == (nn-1)) {            /* Two roots found... */
	p=0.5*(y-x);
	q=p*p+w;
	z=sqrt (fabs (q));
	x += t;
	if (q >= 0.0) {           /* ...a real pair. */
	  z=p+SIGN (z,p);
	  wr[nn-1]=wr[nn]=x+z;
	  if (z) wr[nn]=x-w/z;
	  wi[nn-1]=wi[nn]=0.0;
	} else {                  /* ...a complex pair. */
	  wr[nn-1]=wr[nn]=x+p;
	  wi[nn-1]= -(wi[nn]=z);
	}
	nn -= 2;
      } else {                      /* No roots found. Continue iteration. */
	if (its == MAX_ITER_BASE * MAX_ITER_FAC)
	  nrerror ("Too many iterations in hqr");
	if (its && !(its%MAX_ITER_FAC)) {                /* Form exceptional shift. */
	  t += x;
	  for (i=1;i<=nn;i++) a[i][i] -= x;
	  s=fabs (a[nn][nn-1])+fabs (a[nn-1][nn-2]);
	  y=x=0.75*s;
	  w = -0.4375*s*s;
	}
	++its;
	for (m=(nn-2);m>=l;m--) {                    /* Form shift and then look for */
	  z=a[m][m];                                 /* 2 consecutive small sub- */
	  r=x-z;                                     /* diagonal elements. */
	  s=y-z;
	  p=(r*s-w)/a[m+1][m]+a[m][m+1];           /* Equation (11.6.23). */
	  q=a[m+1][m+1]-z-r-s;
	  r=a[m+2][m+1];
	  s=fabs (p)+fabs (q)+fabs (r);             /* Scale to prevent overflow or */
	  p /= s;                                    /* underflow. */
	  q /= s;
	  r /= s;
	  if (m == l) break;
	  u=fabs (a[m][m-1])*(fabs (q)+fabs (r));
	  v=fabs (p)*(fabs (a[m-1][m-1])+fabs (z)+fabs (a[m+1][m+1]));
	  if ((double)(u+v) == v)
	    break;          /* Equation (11.6.26). */
	}
	for (i=m+2;i<=nn;i++) {
	  a[i][i-2]=0.0;
	  if (i != (m+2))
	    a[i][i-3]=0.0;
	}
	for (k=m;k<=nn-1;k++) {
	  /* Double QR step on rows l to nn and columns m to nn. */
	  if (k != m) {
	    p=a[k][k-1];                      /* Begin setup of Householder */
	    q=a[k+1][k-1];                    /* vector. */
	    r=0.0;
	    if (k != (nn-1)) r=a[k+2][k-1];
	    if ((x=fabs (p)+fabs (q)+fabs (r)) != 0.0) {
	      p /= x;                     /* Scale to prevent overflow or */
	      q /= x;                     /* underflow. */
	      r /= x;
	    }
	  }
	  if ((s=SIGN (sqrt (p*p+q*q+r*r),p)) != 0.0) {
	    if (k == m) {
	      if (l != m)
		a[k][k-1] = -a[k][k-1];
	    } else
	      a[k][k-1] = -s*x;
	    p += s;                           /* Equations (11.6.24). */
	    x=p/s;
	    y=q/s;
	    z=r/s;
	    q /= p;
	    r /= p;
	    for (j=k;j<=nn;j++) {             /* Row modification. */
	      p=a[k][j]+q*a[k+1][j];
	      if (k != (nn-1)) {
		p += r*a[k+2][j];
		a[k+2][j] -= p*z;
	      }
	      a[k+1][j] -= p*y;
	      a[k][j] -= p*x;
	    }
	    mmin = nn<k+3 ? nn : k+3;
	    for (i=l;i<=mmin;i++) {           /* Column modification. */
	      p=x*a[i][k]+y*a[i][k+1];
	      if (k != (nn-1)) {
		p += z*a[i][k+2];
		a[i][k+2] -= p*r;
	      }a[i][k+1] -= p*q;
	      a[i][k] -= p;
	    }
	  }
	}
      }
    }
    } while (l < nn-1);
  }
}

#define RADIX 2.0
static void
balanc (double **a, int n)
     /* Given a matrix a[1..n][1..n], this routine replaces it by a balanced matrix with identical
	eigenvalues. A symmetric matrix is already balanced and is unaffected by this procedure. The
	parameter RADIX should be the machine's floating-point radix. */
{
  int last,j,i;
  double s,r,g,f,c,sqrdx;

  sqrdx=RADIX*RADIX;
  last=0;
  while (last == 0) {
    last=1;
    for (i=1;i<=n;i++) {        /* Calculate row and column norms. */
      r=c=0.0;
      for (j=1;j<=n;j++)
	if (j != i) {
	  c += fabs (a[j][i]);
	  r += fabs (a[i][j]);
	}
      if (c && r) {     /*  If both are nonzero, */
	g=r/RADIX;
	f=1.0;
	s=c+r;
	while (c<g) {   /* find the integer power of the machine radix that */
	  f *= RADIX;   /*  comes closest to balancing the matrix. */
	  c *= sqrdx;
	}
	g=r*RADIX;
	while (c>g) {
	  f /= RADIX;
	  c /= sqrdx;
	}
	if ((c+r)/f < 0.95*s) {
	  last=0;
	  g=1.0/f;
	  for (j=1;j<=n;j++)
	    a[i][j] *= g;  /*  Apply similarity transformation */
	  for (j=1;j<=n;j++) a[j][i] *= f;
	}
      }
    }
  }
}

#define MAX_DEGREE 50

static void
zrhqr (double a[], int m, double rtr[], double rti[])
     /* Find all the roots of a polynomial with real coefficients, E(i=0..m) a(i)x^i, given the degree m
	and the coefficients a[0..m]. The method is to construct an upper Hessenberg matrix whose
	eigenvalues are the desired roots, and then use the routines balanc and hqr. The real and
	imaginary parts of the roots are returned in rtr[1..m] and rti[1..m], respectively. */
{
  int j,k;
  double **hess,xr,xi;
  
  hess=matrix (1,MAX_DEGREE,1,MAX_DEGREE);
  if (m > MAX_DEGREE || a[m] == 0.0 || /* TIMJ: */ fabs (a[m]) < 1e-15 )
    nrerror ("bad args in zrhqr");
  for (k=1;k<=m;k++)	            /* Construct the matrix. */
    {
      hess[1][k] = -a[m-k]/a[m];
      for (j=2;j<=m;j++)
	hess[j][k]=0.0;
      if (k != m)
	hess[k+1][k]=1.0;
    }
  balanc (hess,m);                  /* Find its eigenvalues. */
  hqr (hess,m,rtr,rti);
  if (0)		/* TIMJ: don't need sorting */
    for (j=2;j<=m;j++)
      {				    /* Sort roots by their real parts by straight insertion. */
	xr=rtr[j];
	xi=rti[j];
	for (k=j-1;k>=1;k--)
	  {
	    if (rtr[k] <= xr)
	      break;
	    rtr[k+1]=rtr[k];
	    rti[k+1]=rti[k];
	  }
	rtr[k+1]=xr;
	rti[k+1]=xi;
      }
  free_matrix (hess,1,MAX_DEGREE,1,MAX_DEGREE);
}


#define EPSS 2.0e-16	/* TIMJ, was(float): 1.0e-7 */
#define MR 8
#define MT 100		/* TIMJ: was: 10 */
#define MAXIT (MT*MR)
/* Here EPSS is the estimated fractional roundoff error. We try to break (rare) limit cycles with
   MR different fractional values, once every MT steps, for MAXIT total allowed iterations. */

static void
laguer (GslComplex a[], int m, GslComplex *x, int *its)
     /* Given the degree m and the m+1 complex coefficients a[0..m] of the polynomial         mi=0 a[i]xi,
	and given a complex value x, this routine improves x by Laguerre's method until it converges,
	within the achievable roundoff limit, to a root of the given polynomial. The number of iterations
	taken is returned as its. */
{
  int iter,j;
  double abx,abp,abm,err;
  GslComplex dx,x1,b,d,f,g,h,sq,gp,gm,g2;
  static double frac[MR+1] = {0.0,0.5,0.25,0.75,0.13,0.38,0.62,0.88,1.0};
  /* Fractions used to break a limit cycle. */
  
  for (iter=1;iter<=MAXIT;iter++)
    {        /* Loop over iterations up to allowed maximum. */
      *its=iter;
      b=a[m];
      err=gsl_complex_abs (b);
      d=f=gsl_complex (0.0,0.0);
      abx=gsl_complex_abs (*x);
      for (j=m-1;j>=0;j--)
	{            /* Efficient computation of the polynomial and */
	  f=gsl_complex_add (gsl_complex_mul (*x,f),d);         /* its first two derivatives. */
	  d=gsl_complex_add (gsl_complex_mul (*x,d),b);
	  b=gsl_complex_add (gsl_complex_mul (*x,b),a[j]);
	  err=gsl_complex_abs (b)+abx*err;
	}
      err *= EPSS;
      /* Estimate of roundoff error in evaluating polynomial. */
      if (gsl_complex_abs (b) <= err)
	return;               /* We are on the root. */
      g=gsl_complex_div (d,b);                              /* The generic case: use Laguerre's formula. */
      g2=gsl_complex_mul (g,g);
      h=gsl_complex_sub (g2,gsl_complex_rmul (2.0,gsl_complex_div (f,b)));
      sq=gsl_complex_sqrt (gsl_complex_rmul ((double) (m-1),gsl_complex_sub (gsl_complex_rmul ((double) m,h),g2)));
      gp=gsl_complex_add (g,sq);
      gm=gsl_complex_sub (g,sq);
      abp=gsl_complex_abs (gp);
      abm=gsl_complex_abs (gm);
      if (abp < abm)
	gp=gm;
      dx=((DMAX (abp,abm) > 0.0 ? gsl_complex_div (gsl_complex ((double) m,0.0),gp)
	   : gsl_complex_rmul (1+abx,gsl_complex (cos ((double)iter),sin ((double)iter)))));
      x1=gsl_complex_sub (*x,dx);
      if (x->re == x1.re && x->im == x1.im)
	return;                 /* Converged. */
      if (iter % MT) *x=x1;
      else *x=gsl_complex_sub (*x,gsl_complex_rmul (frac[iter/MT],dx));
      /* Every so often we take a fractional step, to break any limit cycle (itself a rare occurrence). */
    }
  nrerror ("too many iterations in laguer");
  /* Very unusual - can occur only for complex roots. Try a different starting guess for the root. */
}

/* Here is a driver routine that calls laguer in succession for each root, performs
   the deflation, optionally polishes the roots by the same Laguerre method - if you
   are not going to polish in some other way - and finally sorts the roots by their real
   parts. (We will use this routine in Chapter 13.) */

#define EPS	4.0e-15		/* TIMJ, was(float): 2.0e-6 */
#define MAXM 100
/* A small number, and maximum anticipated value of m. */

static void
zroots (GslComplex a[], int m, GslComplex roots[], int polish)
     /* Given the degree m and the m+1 complex coefficients a[0..m] of the polynomial mi=0 a (i)xi,
	this routine successively calls laguer and finds all m complex roots in roots[1..m]. The
	boolean variable polish should be input as true (1) if polishing (also by Laguerre's method)
	is desired, false (0) if the roots will be subsequently polished by other means. */
{
  int i,its,j,jj;
  GslComplex x,b,c,ad[MAXM];
  
  for (j=0;j<=m;j++) ad[j]=a[j];              /* Copy of coefficients for successive deflation. */
  for (j=m;j>=1;j--)                          /* Loop over each root to be found. */
    {
      x=gsl_complex (0.0,0.0);                  /* Start at zero to favor convergence to small- */
      laguer (ad,j,&x,&its);                     /* est remaining root, and find the root. */
      if (fabs (x.im) <= 2.0*EPS*fabs (x.re))
	x.im=0.0;
      roots[j]=x;
      b=ad[j];                                  /* Forward deflation. */
      for (jj=j-1;jj>=0;jj--)
	{
	  c=ad[jj];
	  ad[jj]=b;
	  b=gsl_complex_add (gsl_complex_mul (x,b),c);
	}
    }
  if (polish)
    for (j=1;j<=m;j++)                        /* Polish the roots using the undeflated coeffi- */
      laguer (a,m,&roots[j],&its);            /* cients. */
  for (j=2;j<=m;j++)                          /* Sort roots by their real parts by straight insertion */
    {
      x=roots[j];
      for (i=j-1;i>=1;i--) {
	if (roots[i].re <= x.re)
	  break;
	roots[i+1]=roots[i];
      }
      roots[i+1]=x;
    }
}

#define ITMAX 20	/* At most ITMAX iterations. */
#define TINY 2.0-15	/* TIMJ, was (float): 1.0e-6 */

static void
qroot (double p[], int n, double *b, double *c, double eps)
     /* Given n+1 coefficients p[0..n] of a polynomial of degree n, and trial values for the coefficients
	of a quadratic factor x*x+b*x+c, improve the solution until the coefficients b,c change by less
	than eps. The routine poldiv §5.3 is used. */
{
  int iter;
  double sc,sb,s,rc,rb,r,dv,delc,delb;
  double *q,*qq,*rem;
  double d[3];
  
  q=vector (0,n);
  qq=vector (0,n);
  rem=vector (0,n);
  d[2]=1.0;
  for (iter=1;iter<=ITMAX;iter++)
    {
      d[1]=(*b);
      d[0]=(*c);
      poldiv (p,n,d,2,q,rem);
      s=rem[0];                                        /* First division r,s. */
      r=rem[1];
      poldiv (q,(n-1),d,2,qq,rem);
      sb = -(*c)*(rc = -rem[1]);                       /* Second division partial r,s with respect to */
      rb = -(*b)*rc+(sc = -rem[0]);                    /* c. */
      dv=1.0/(sb*rc-sc*rb);                            /* Solve 2x2 equation. */
      delb=(r*sc-s*rc)*dv;
      delc=(-r*sb+s*rb)*dv;
      *b += (delb=(r*sc-s*rc)*dv);
      *c += (delc=(-r*sb+s*rb)*dv);
      if ((fabs (delb) <= eps*fabs (*b) || fabs (*b) < TINY)
	  && (fabs (delc) <= eps*fabs (*c) || fabs (*c) < TINY))
	{
	  free_vector (rem,0,n);                      /* Coefficients converged. */
	  free_vector (qq,0,n);
	  free_vector (q,0,n);
	  return;
	}
    }
  nrerror ("Too many iterations in routine qroot");
}

#define SNCNDN_CA 0.0003                      /* The accuracy is the square of SNCNDN_CA. */
static void
sncndn (double uu, double emmc, double *sn_p, double *cn_p, double *dn_p)
     /* Returns the Jacobian elliptic functions sn(u, kc), cn(u, kc), and dn(u, kc). Here uu = u, while
	emmc = k2c. */
{
  double a,b,c,d,emc,u,sn,cn,dn;
  double em[14],en[14];
  int i,ii,l,bo;
  d=0; /* TIMJ: shutup compiler */
  
  emc=emmc;
  u=uu;
  if (emc) {
    bo=(emc < 0.0);
    if (bo) {
      d=1.0-emc;
      emc /= -1.0/d;
      u *= (d=sqrt(d));
    }a=1.0;
    dn=1.0;
    for (i=1;i<=13;i++) {
      l=i;
      em[i]=a;
      en[i]=(emc=sqrt(emc));
      c=0.5*(a+emc);
      if (fabs(a-emc) <= SNCNDN_CA*a) break;
      emc *= a;
      a=c;
    }u *= c;
    sn=sin(u);
    cn=cos(u);
    if (sn) {
      a=cn/sn;
      c *= a;
      for (ii=l;ii>=1;ii--) {
	b=em[ii];
	a *= c;
	c *= dn;
	dn=(en[ii]+a)/(b+a);
	a=c/b;
      }a=1.0/sqrt(c*c+1.0);
      sn=(sn >= 0.0 ? a : -a);
      cn=c*sn;
    }if (bo) {
      a=dn;
      dn=cn;
      cn=a;
      sn /= d;
    }
  } else {
    cn=1.0/cosh(u);
    dn=cn;
    sn=tanh(u);
  }
  if (sn_p)
    *sn_p = sn;
  if (cn_p)
    *cn_p = cn;
  if (dn_p)
    *dn_p = dn;
}

static void
sncndnC (GslComplex uu, GslComplex emmc, GslComplex *sn_p, GslComplex *cn_p, GslComplex *dn_p)
{
  GslComplex a,b,c,d,emc,u,sn,cn,dn;
  GslComplex em[14],en[14];
  int i,ii,l,bo;
  
  emc=emmc;
  u=uu;
  if (emc.re || emc.im) /* gsl_complex_abs (emc)) */
    {
      /* bo=gsl_complex_abs (emc) < 0.0; */
      bo=emc.re < 0.0;
      if (bo) {
	d=gsl_complex_sub (ONE, emc);
	emc = gsl_complex_div (emc, gsl_complex_div (gsl_complex (-1.0, 0), d));
	d = gsl_complex_sqrt (d);
	u = gsl_complex_mul (u, d);
      }
      a=ONE; dn=ONE;
      for (i=1;i<=13;i++) {
	l=i;
	em[i]=a;
	emc = gsl_complex_sqrt (emc);
	en[i]=emc;
	c = gsl_complex_mul (gsl_complex (0.5, 0), gsl_complex_add (a, emc));
	if (gsl_complex_abs (gsl_complex_sub (a, emc)) <=
	    gsl_complex_abs (gsl_complex_mul (gsl_complex (SNCNDN_CA, 0), a)))
	  break;
	emc = gsl_complex_mul (emc, a);
	a=c;
      }
      u = gsl_complex_mul (u, c);
      sn = gsl_complex_sin (u);
      cn = gsl_complex_cos (u);
      if (sn.re) /* gsl_complex_abs (sn)) */
	{
	  a= gsl_complex_div (cn, sn);
	  c = gsl_complex_mul (c, a);
	  for (ii=l;ii>=1;ii--) {
	    b = em[ii];
	    a = gsl_complex_mul (a, c);
	    c = gsl_complex_mul (c, dn);
	    dn = gsl_complex_div (gsl_complex_add (en[ii], a), gsl_complex_add (b, a));
	    a = gsl_complex_div (c, b);
	  }
	  a = gsl_complex_div (ONE, gsl_complex_sqrt (gsl_complex_add (ONE, gsl_complex_mul (c, c))));
	  if (sn.re >= 0.0) /* gsl_complex_arg (sn) >= 0.0) */
	    sn = a;
	  else
	    {
	      sn.re = -a.re;
	      sn.im = a.im;
	    }
	  cn = gsl_complex_mul (c, sn);
	}
      if (bo) {
	a=dn;
	dn=cn;
	cn=a;
	sn = gsl_complex_div (sn, d);
      }
    } else {
      cn=gsl_complex_div (ONE, gsl_complex_cosh (u));
      dn=cn;
      sn=gsl_complex_tanh (u);
    }
  if (sn_p)
    *sn_p = sn;
  if (cn_p)
    *cn_p = cn;
  if (dn_p)
    *dn_p = dn;
}

#define RF_ERRTOL	0.0025		/* TIMJ, was(float): 0.08 */
#define RF_TINY		2.2e-307	/* TIMJ, was(float): 1.5e-38 */
#define RF_BIG		1.5e+307	/* TIMJ, was(float):  3.0e37 */
#define RF_THIRD	(1.0/3.0)
#define RF_C1		(1.0/24.0)
#define RF_C2		0.1
#define RF_C3		(3.0/44.0)
#define RF_C4		(1.0/14.0)

static double
rf (double x, double y, double z)
     /* Computes Carlson's elliptic integral of the first kind, RF (x, y, z). x, y, and z must be nonneg-
	ative, and at most one can be zero. RF_TINY must be at least 5 times the machine underflow limit,
	RF_BIG at most one fifth the machine overflow limit. */
{
  double alamb,ave,delx,dely,delz,e2,e3,sqrtx,sqrty,sqrtz,xt,yt,zt;

  if (1 /* TIMJ: add verbose checks */)
    {
      if (DMIN (DMIN (x, y), z) < 0.0)
	nrerror ("rf: x,y,z have to be positive");
      if (DMIN (DMIN (x + y, x + z), y + z) < RF_TINY)
	nrerror ("rf: only one of x,y,z may be 0");
      if (DMAX (DMAX (x, y), z) > RF_BIG)
	nrerror ("rf: at least one of x,y,z is too big");
    }
  if (DMIN(DMIN(x,y),z) < 0.0 || DMIN(DMIN(x+y,x+z),y+z) < RF_TINY ||
      DMAX(DMAX(x,y),z) > RF_BIG)
    nrerror("invalid arguments in rf");
  xt=x;
  yt=y;
  zt=z;
  do {
    sqrtx=sqrt(xt);
    sqrty=sqrt(yt);
    sqrtz=sqrt(zt);
    alamb=sqrtx*(sqrty+sqrtz)+sqrty*sqrtz;
    xt=0.25*(xt+alamb);
    yt=0.25*(yt+alamb);
    zt=0.25*(zt+alamb);
    ave=RF_THIRD*(xt+yt+zt);
    delx=(ave-xt)/ave;
    dely=(ave-yt)/ave;
    delz=(ave-zt)/ave;
  } while (DMAX(DMAX(fabs(delx),fabs(dely)),fabs(delz)) > RF_ERRTOL);
  e2=delx*dely-delz*delz;
  e3=delx*dely*delz;
  return (1.0+(RF_C1*e2-RF_C2-RF_C3*e3)*e2+RF_C4*e3)/sqrt(ave);
}

static GslComplex
rfC (GslComplex x, GslComplex y, GslComplex z)
{
  GslComplex alamb,ave,delx,dely,delz,e2,e3,sqrtx,sqrty,sqrtz,xt,yt,zt;
  GslComplex RFC_C1 = {1.0/24.0, 0}, RFC_C2 = {0.1, 0}, RFC_C3 = {3.0/44.0, 0}, RFC_C4 = {1.0/14.0, 0};
  
  if (DMIN (DMIN (gsl_complex_abs (x), gsl_complex_abs (y)), gsl_complex_abs (z)) < 0.0)
    nrerror ("rf: x,y,z have to be positive");
  if (DMIN (DMIN (gsl_complex_abs (x) + gsl_complex_abs (y), gsl_complex_abs (x) + gsl_complex_abs (z)),
	    gsl_complex_abs (y) + gsl_complex_abs (z)) < RF_TINY)
    nrerror ("rf: only one of x,y,z may be 0");
  if (DMAX (DMAX (gsl_complex_abs (x), gsl_complex_abs (y)), gsl_complex_abs (z)) > RF_BIG)
    nrerror ("rf: at least one of x,y,z is too big");
  xt=x;
  yt=y;
  zt=z;
  do {
    sqrtx = gsl_complex_sqrt (xt);
    sqrty = gsl_complex_sqrt (yt);
    sqrtz = gsl_complex_sqrt (zt);
    alamb = gsl_complex_add (gsl_complex_mul (sqrtx, gsl_complex_add (sqrty, sqrtz)), gsl_complex_mul (sqrty, sqrtz));
    xt =    gsl_complex_mul (gsl_complex (0.25, 0), gsl_complex_add (xt, alamb));
    yt =    gsl_complex_mul (gsl_complex (0.25, 0), gsl_complex_add (yt, alamb));
    zt =    gsl_complex_mul (gsl_complex (0.25, 0), gsl_complex_add (zt, alamb));
    ave =   gsl_complex_mul (gsl_complex (RF_THIRD, 0), gsl_complex_add3 (xt, yt, zt));
    delx =  gsl_complex_div (gsl_complex_sub (ave, xt), ave);
    dely =  gsl_complex_div (gsl_complex_sub (ave, yt), ave);
    delz =  gsl_complex_div (gsl_complex_sub (ave, zt), ave);
    /* } while (DMAX (DMAX (fabs (delx.re), fabs (dely.re)), fabs (delz.re)) > RF_ERRTOL); */
  } while (DMAX (DMAX (gsl_complex_abs (delx), gsl_complex_abs (dely)), gsl_complex_abs (delz)) > RF_ERRTOL);
  e2 = gsl_complex_sub (gsl_complex_mul (delx, dely), gsl_complex_mul (delz, delz));
  e3 = gsl_complex_mul3 (delx, dely, delz);
  return gsl_complex_div (gsl_complex_add3 (gsl_complex (1.0, 0),
					    gsl_complex_mul (e2,
							     gsl_complex_sub3 (gsl_complex_mul (RFC_C1, e2),
									       RFC_C2,
									       gsl_complex_mul (RFC_C3, e3))),
					    gsl_complex_mul (RFC_C4, e3)),
			  gsl_complex_sqrt (ave));
}


static double
ellf (double phi, double ak)
     /* Legendre elliptic integral of the 1st kind F(phi, k), evaluated using Carlson's function RF.
	The argument ranges are 0 <= phi <= pi/2, 0 <=  k*sin(phi) <= 1. */
{
  double s=sin(phi);
  return s*rf(DSQR(cos(phi)),(1.0-s*ak)*(1.0+s*ak),1.0);
}
