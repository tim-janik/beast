/* Splines - Spline creation and evaluation routines
 * Copyright (C) 2004 Tim Janik
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
#define _ISOC99_SOURCE  /* NAN, isfinite, isnan */
#include "gxkspline.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SQRT_3       (1.7320508075688772935274463415059)
static const double INF = 1e+999;


/* --- functions --- */
static inline double
segment_eval (const GxkSplineSegment *xseg,     /* must not be last segment */
              double                  x,
              double                 *yd1)
{
  const GxkSplineSegment *next = xseg + 1;
  /* check segment length */
  if (next->x == xseg->x)       /* points should always have distinct x positions */
    {
      if (yd1 && xseg->y == next->y)
        *yd1 = 0;
      else if (yd1)
        *yd1 = xseg->y < next->y ? +INF : -INF;
      return (xseg->y + next->y) * 0.5;
    }
  /* compute cubic spline polynomial */
  double v = xseg->x, w = next->x;
  double y = (w - x) * xseg->y / (w - v) +
             (w - x) * (x - v) * (xseg->yd2 * (x + v - 2. * w) -
                                  next->yd2 * (x + w - 2. * v)) / (6. * (w - v)) +
             (x - v) * next->y / (w - v);
  /* compute first derivative */
  if (yd1)
    *yd1 = (next->y - xseg->y) / (w - v) +
           xseg->yd2 * (v * v + 2 * w * (3 * x - w - v) - 3 * x * x) / (6. * (w - v)) -
           next->yd2 * (w * w + 2 * v * (3 * x - v - w) - 3 * x * x) / (6. * (w - v));
  return y;
}

/**
 * gxk_spline_new_natural
 * @n_points: number of fix points
 * @points:   fix points
 * Create a natural spline based on a given set of fix points.
 */
GxkSpline*
gxk_spline_new_natural (guint                   n_points,
                        const GxkSplinePoint   *points)
{
  return gxk_spline_new (n_points, points, NAN, NAN);
}

static int
spline_segment_cmp (const void *v1,
                    const void *v2)
{
  const GxkSplineSegment *s1 = v1;
  const GxkSplineSegment *s2 = v2;
  return s1->x < s2->x ? -1 : s1->x > s2->x;
}

/**
 * gxk_spline_new
 * @n_points: number of fix points
 * @points:   fix points
 * @dy_start: first derivatives at point[0]
 * @dy_end:   first derivatives at point[n_points - 1]
 * Create a not-a-knot spline based on a given set of fix points and the
 * first derivative of the first and last point of the interpolating function.
 */
GxkSpline*
gxk_spline_new (guint                   n_points,
                const GxkSplinePoint   *points,
                double                  dy_start,
                double                  dy_end)
{
  g_return_val_if_fail (n_points >= 2, NULL);
  GxkSpline *spline = g_malloc (sizeof (spline[0]) + n_points * sizeof (spline->segs[0]));
  /* initialize segments */
  spline->n_segs = n_points;
  gint i;
  for (i = 0; i < spline->n_segs; i++)
    {
      GxkSplineSegment *seg = spline->segs + i;
      seg->x = points[i].x;
      seg->y = points[i].y;
      seg->ex1 = seg->ex2 = NAN;
    }
  /* ensure the segments are sorted in ascending order */
  qsort (spline->segs, spline->n_segs, sizeof (spline->segs[0]), spline_segment_cmp);
  /* check the first derivatives for not-a-knot vs. natural spline */
  double *dyx = g_alloca (sizeof (dyx[0]) * spline->n_segs);
  GxkSplineSegment *ss = spline->segs;
  /* curvature of first point */
  if (isnan (dy_start))
    ss[0].yd2 = dyx[0] = 0.0;
  else
    {
      ss[0].yd2 = -0.5;
      double deltax = ss[1].x - ss[0].x;
      double deltay = ss[1].y - ss[0].y;
      dyx[0]= (3. / deltax) * (deltay / deltax - dy_start);
    }
  gint last = spline->n_segs - 1;
  /* decomposition loop of tridiagonal algorithm */
  for (i = 1; i < last; i++)
    {
      double delta0x = ss[i].x - ss[i - 1].x;
      double delta1x = ss[i + 1].x - ss[i].x;
      double delta2x = ss[i + 1].x - ss[i - 1].x;
      double denom = delta0x * ss[i - 1].yd2 + 2. * delta2x;
      ss[i].yd2 = (delta0x - delta2x) / denom;
      double delta0y = ss[i].y - ss[i - 1].y;
      double delta1y = ss[i + 1].y - ss[i].y;
      double sld = delta1y / delta1x - delta0y / delta0x;
      dyx[i] = (6. * sld - delta0x * dyx[i - 1]) / denom;
    }
  /* curvature of last point */
  if (isnan (dy_end))
    ss[last].yd2 = 0;
  else
    {
      double deltax = ss[last].x - ss[last - 1].x;
      double deltay = ss[last].y - ss[last - 1].y;
      double t = 6. / deltax * (dy_end - deltay / deltax);
      ss[last].yd2 = (t - dyx[last - 1]) / (ss[last - 1].yd2 + 2.);
    }
  /* backsubstitution loop of tridiagonal algorithm */
  for (i = last; i > 0; i--)
    ss[i - 1].yd2 = ss[i - 1].yd2 * ss[i].yd2 + dyx[i - 1];
  /* compute segment extrema and setup ymin/ymax */
  for (i = 0; i < spline->n_segs - 1; i++)
    {
      GxkSplineSegment *seg = spline->segs + i, *next = seg + 1;
      seg->ymin = MIN (seg->y, seg[1].y);
      seg->ymax = MAX (seg->y, seg[1].y);
      double v = seg->x, w = next->x, g = next->y, h = seg->y, s = seg->yd2, t = next->yd2;
      double rsq = 6 * (s * (g - h) + t * (h - g)) + (s * (t + s) + t*t) * (v * (v - 2 * w) + w*w);
      if (rsq >= 0 && t - s)
        {
          seg->ex1 = (SQRT_3 * t * v - SQRT_3 * s * w - sqrt (rsq)) / (SQRT_3 * t - SQRT_3 * s);
          seg->ex2 = (SQRT_3 * t * v - SQRT_3 * s * w + sqrt (rsq)) / (SQRT_3 * t - SQRT_3 * s);
          if (seg->ex1 >= seg->x && seg->ex1 <= next->x)
            {
              double fx = segment_eval (seg, seg->ex1, NULL);
              seg->ymin = MIN (seg->ymin, fx);
              seg->ymax = MAX (seg->ymax, fx);
            }
          if (seg->ex2 >= seg->x && seg->ex2 <= next->x)
            {
              double fx = segment_eval (seg, seg->ex2, NULL);
              seg->ymin = MIN (seg->ymin, fx);
              seg->ymax = MAX (seg->ymax, fx);
            }
        }
    }
  /* fixup last segment (which has an x extend of 0) */
  i = spline->n_segs - 1;
  spline->segs[i].ymin = spline->segs[i].ymax = spline->segs[i].y;
  return spline;
}

/**
 * gxk_spline_eval
 * @spline:  correctly setup #GxkSpline
 * @x:       x position for evaluation
 * @dy1:     location to store first derivative of y
 * @RETURNS: y of @spline at position x
 *
 * Evaluate the @spline polynomial at position @x and
 * return the interpolated value y, as well as its first derivative.
 */
double
gxk_spline_eval (const GxkSpline *spline,
                 double           x,
                 double          *yd1)
{
  /* find segment via bisection */
  guint first = 0, last = spline->n_segs - 1;
  while (first + 1 < last)
    {
      guint i = (first + last) >> 1;
      if (spline->segs[i].x > x)
        last = i;
      else
        first = i;
    }
  g_assert (first + 1 == last);
  /* eval polynomials */
  double y = segment_eval (spline->segs + first, x, yd1);
  return y;
}

/**
 * gxk_spline_y
 * @spline:  correctly setup #GxkSpline
 * @x:       x position for evaluation
 * @RETURNS: y of @spline at position x
 *
 * Evaluate the @spline polynomial at position @x and
 * return the interpolated value y.
 */
double
gxk_spline_y (const GxkSpline        *spline,
              double                  x)
{
  return gxk_spline_eval (spline, x, NULL);
}

static double
round_to_double (double vin)
{
  volatile double rounded = vin;
  return rounded;
}

/**
 * gxk_spline_findx
 * @spline:  correctly setup #GxkSpline
 * @y:       interpolated y value
 * @RETURNS: x position to yield y or NAN
 *
 * Find an x position for which spline evaluation yields y.
 * Due to round off, calling gxk_spline_y() on the result may
 * produce a number equal to y only within a certain epsilon.
 * If multiple x positions will yield y upon evaluation, any
 * of them may be returned. If no x position can yield y,
 * NAN is returned. Evaluation of this function may take
 * about 10 times as long as calling its counterpart
 * gxk_spline_y(), some times much longer.
 */
double
gxk_spline_findx (const GxkSpline *spline,
                  double           y)
{
  /* find closest segment */
  guint i, best = spline->n_segs;
  double besty = G_MAXDOUBLE;
  for (i = 0; i < spline->n_segs; i++)
    if (y >= spline->segs[i].ymin && y <= spline->segs[i].ymax)
      {
        double dist = fabs (y - spline->segs[i].y);
        if (dist <= besty)
          {
            besty = dist;
            best = i;
          }
      }
  if (best >= spline->n_segs)
    return NAN; /* no match */
  const GxkSplineSegment *xseg = spline->segs + best, *next = xseg + 1;
  if (best + 1 == spline->n_segs)       /* matched the final segment */
    {
      /* in the final segment, y == xseg->ymin = xseg->ymax = xseg->y */
      return xseg->x;
    }
  if (besty == 0.0)     /* honour exact match */
    return xseg->x;
  /* figure left and right bounds of x(y) */
  double xmin = xseg->x, xmax = xseg->x;
  double ymin = xseg->y, ymax = xseg->y;
  if (next->y <= ymin)
    {
      xmin = next->x;
      ymin = next->y;
    }
  if (next->y >= ymax)
    {
      xmax = next->x;
      ymax = next->y;
    }
  if (isfinite (xseg->ex1) && xseg->ex1 >= xseg->x && xseg->ex1 <= next->x)
    {
      double exy = segment_eval (xseg, xseg->ex1, NULL);
      if (exy <= ymin)
        {
          xmin = xseg->ex1;
          ymin = exy;
        }
      if (exy >= ymax)
        {
          xmax = xseg->ex1;
          ymax = exy;
        }
    }
  if (isfinite (xseg->ex2) && xseg->ex2 >= xseg->x && xseg->ex2 <= next->x)
    {
      double exy = segment_eval (xseg, xseg->ex2, NULL);
      if (exy <= ymin)
        {
          xmin = xseg->ex2;
          ymin = exy;
        }
      if (exy >= ymax)
        {
          xmax = xseg->ex2;
          ymax = exy;
        }
    }
  /* sanity check boundaries */
  if (y <= ymin)
    return y < ymin ? NAN : xmin;
  if (y >= ymax)
    return y > ymax ? NAN : xmax;
  /* ensured: ymin < y < ymax */
  guint iteration_counter = 0;
#if 0   /* bisection */
  double prevx, z, x = xmin;
  do
    {
      prevx = x;
      x = (xmin + xmax) * 0.5;
      z = segment_eval (xseg, x, NULL);
      if (z < y)
        xmin = x;
      else if (z > y)
        xmax = x;
      else
        return x;       /* y==0 */
      iteration_counter++;
    }
  while (prevx != round_to_double (x));
#else   /* newton-raphson + bisection (improves on pure bisection by a factor of 8-10) */
  double prevx2, prevx1 = 0, x = xmin, dz = 0, z = ymin, check;
  do
    {
      prevx2 = prevx1;
      prevx1 = x;
      /* newton-raphson step */
      if (dz)
        {
          double lastdx = prevx1 - prevx2, ndx = (z - y) / dz;
          x -= ndx;
          if ((xmin - x) * (xmax - x) >= 0 ||           /* check boundaries */
              fabs (lastdx * dz) < 2.5 * fabs (z - y))  /* and convergence ratio */
            dz = 0;     /* force bisection */
        }
      /* bisection step */
      if (!dz)
        x = (xmin + xmax) * 0.5;
      z = segment_eval (xseg, x, &dz);
      if (z < y)
        xmin = x;
      else if (z > y)
        xmax = x;
      else
        break;          /* z == y */
      iteration_counter++;
      check = round_to_double (x);
    }
  while (prevx1 != check &&     /* catch delta x approaching 0 */
         prevx2 != check);      /* and boundary ping-pong (pathological case) */
#endif
  if (0)
    {
      static guint caller_sum, caller_times;
      caller_sum += iteration_counter;
      caller_times++;
      g_printerr ("spline_findx: iters=%u (avg=%f) x=%.17g y=%.17g approx=%.17g dx=%.17g dy=%.17g\n",
                  iteration_counter, caller_sum / (double) caller_times, x, y, z, xmax-xmin, z-y);
    }
  return x;
}

/**
 * gxk_spline_free
 * @spline: correctly setup #GxkSpline
 *
 * Free a @spline structure.
 */
void
gxk_spline_free (GxkSpline *spline)
{
  g_return_if_fail (spline != NULL);
  g_free (spline);
}

/**
 * gxk_spline_dump
 * @spline: correctly setup #GxkSpline
 *
 * Produce a debugging printout of @spline on stderr.
 */
void
gxk_spline_dump (GxkSpline *spline)
{
  g_printerr ("GxkSpline[%u] = {\n", spline->n_segs);
  g_printerr ("  // x, y, yd2, ymin, ymax, ex1, ex2\n");
  guint i;
  for (i = 0; i < spline->n_segs; i++)
    {
      GxkSplineSegment *seg = spline->segs + i;
      g_printerr ("  { %-+.17g, %-+.17g, %-+.17g, %-+.17g, %-+.17g, %-+.17g, %-+.17g },",
                  seg->x, seg->y, seg->yd2, seg->ymin, seg->ymax, seg->ex1, seg->ex2);
      const double test_epsilon = 0.0000001;
      if (isfinite (seg->ex1))
        {
          g_printerr ("\n    ");
          double s1 = gxk_spline_y (spline, seg->ex1 - test_epsilon);
          double s2 = gxk_spline_y (spline, seg->ex1);
          double s3 = gxk_spline_y (spline, seg->ex1 - test_epsilon);
          const char *judge = (s2 - s1) * (s2 - s3) < 0 ? "FAIL" : "OK";
          if (s2 - s1 == 0 || s2 - s3 == 0)
            judge = "BROKEN";   /* test_epsilon too small */
          g_printerr ("// extremum%u check: %s", 1, judge);
        }
      if (isfinite (seg->ex2))
        {
          g_printerr ("\n    ");
          double s1 = gxk_spline_y (spline, seg->ex2 - test_epsilon);
          double s2 = gxk_spline_y (spline, seg->ex2);
          double s3 = gxk_spline_y (spline, seg->ex2 - test_epsilon);
          const char *judge = (s2 - s1) * (s2 - s3) < 0 ? "FAIL" : "OK";
          if (s2 - s1 == 0 || s2 - s3 == 0)
            judge = "BROKEN";   /* test_epsilon too small */
          g_printerr ("// extremum%u check: %s", 2, judge);
        }
      g_printerr ("\n");
    }
  g_printerr ("};\n");
}
