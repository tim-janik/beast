/* Spline test routines
 * Copyright (C) 2004 Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#define _ISOC99_SOURCE
#include <math.h>
#include "gxk/gxkspline.h"
#include <stdlib.h>

static GxkSpline*
gxk_spline_new_swapped (guint                   n_points,
                        const GxkSplinePoint   *points,
                        double                  dy_start,
                        double                  dy_end)
{
  g_error ("don't use this");
  GxkSplinePoint *spoints = g_alloca (sizeof (spoints[0]) * n_points);
  guint i;
  for (i = 0; i < n_points; i++)
    {
      spoints[i].x = points[i].y;
      spoints[i].y = points[i].x;
    }
  return gxk_spline_new (n_points, spoints, dy_start, dy_end);
}          

static void
spline_test (GxkSpline *spline,
             guint      interval_steps,
             gboolean   swap)
{
  guint i, k;
  for (i = 1; i < spline->n_segs; i++)
    for (k = 0; k < interval_steps + (i + 1 == spline->n_segs); k++)
      {
        double x = spline->segs[i - 1].x + k * (spline->segs[i].x - spline->segs[i - 1].x) / interval_steps;
        double y1 = gxk_spline_y (spline, x);
        if (swap)
          g_print ("%-+24.18g %-+24.18g\n", y1, x);
        else if (1)
          g_print ("%-+24.18g %-+24.18g\n", x, y1);
        else
          {
            double z = gxk_spline_findx (spline, y1);
            double y2 = gxk_spline_y (spline, z);
            g_print ("%-+24.18g %-+24.18g # findx=%-+24.18g dx=%-+24.18f (%u) dy=%-+24.18g (%u)\n", x, y1, z, x-z, x==z, y1 - y2, y1==y2);
          }
      }
}

static void
print_spline (guint                   n_points,
              const GxkSplinePoint   *points,
              guint                   steps,
              double                  yds,
              double                  yde,
              gboolean                swap)
{
  GxkSpline *spline;
  spline = gxk_spline_new (n_points, points, yds, yde);
  gxk_spline_dump (spline);
  spline_test (spline, steps, swap);
  gxk_spline_free (spline);
}

int
main (int   argc,
      char *argv[])
{
  /* GLib's thread and object systems */
  g_thread_init (NULL);
  g_type_init ();

  /* initialize Gtk+ and go into threading mode */
  gtk_init (&argc, &argv);
  g_set_prgname ("splinetest");	/* overriding Gdk's program name */
  GDK_THREADS_ENTER ();
  
  /* initialize Gtk+ Extension Kit */
  gxk_init ();

  /* usage: splinetest <variant123> <nsteps> */

  gint variant = 0, steps = 10;
  if (argc > 1)
    variant = atoi (argv[1]);
  if (argc > 2)
    steps = atoi (argv[2]);

  if (ABS (variant) == 1)
    {
      GxkSplinePoint points[] = {
        { -24,    30 },
        { -7,     53 },
        { -3,    -6 },
        { +3.3,  -10 },
        { +10.2,  5 },
        { +18,   -24 },
      };
      print_spline (G_N_ELEMENTS (points), points, steps, -7, -50, variant < 0);
    }
  else if (ABS (variant) == 2)
    {
      GxkSplinePoint points[] = {
        { +12, -3 },
        { -3,  +4 },
        { +2,  -28 },
        { -10, +4 },
        { +5,  -23 },
        { -1,  +13 },
      };
      print_spline (G_N_ELEMENTS (points), points, steps, +5, -60, variant < 0);
    }
  else if (ABS (variant) == 3)
    {
      GxkSplinePoint points[] = {
        { 0, 25 },
        { 1, 25 },
        { 2, 25 },
        { 3, 25 },
        { 4, 25 },
        { 5, 25 },
      };
      print_spline (G_N_ELEMENTS (points), points, steps, NAN, NAN, variant < 0);
    }
  else if (ABS (variant) == 4)
    {
      GxkSplinePoint points[] = {
        { -20, -25 },
        { -16, -24.5 },
        { -12, -24.1 },
        {  -8, -20 },
        {  -4, -15 },
        {  +0,  -2.1 },
        {  +4,  -2 },
        {  +8,  +3 },
        { +12,  +3.3 },
        { +16,  +6 },
        { +20,  +9 },
      };
      print_spline (G_N_ELEMENTS (points), points, steps, 1, 1, variant < 0);
    }
  else if (ABS (variant) == 5)
    {
      GxkSplinePoint points[] = {
#if 1
        { 3.47, -6 },
        { 3.79, -3 },
        { 4.05, +0 },
        { 4.46, +6 },
        { 4.75, +12 },
        { 4.95, +18 },
        { 5.19, +30 },
        // { 5.43, +88 },
#elif 0
        { 4.29, -6 },
        { 4.77, -4 },
        { 5.17, -2 },
        // { 5.45,  0 },
        { 6.01,  2 },
        { 6.65,  5 },
        { 7.49, 10 },
        { 8.61, 20 },
        { 9.29, 30 },
        { 9.93, 50 },
        // { 10.21, 58.75 },
        { 10.71, 96 },
#elif 1
        {  0, -6  },
        {  1, -4  },
        {  2, -2  },
        {  3, +0  },
        {  4, +2  },
        {  5, +5  },
        {  6, +6  },
        {  7, 10  },
        {  8, 20  },
        {  9, 30  },
        { 10, 50  },
        { 11, 144 },
#endif
      };
      print_spline (G_N_ELEMENTS (points), points, steps, NAN, NAN, variant < 0);
    }

  return 0;
}
