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

static void
spline_test (GxkSpline *spline,
             guint      interval_steps)
{
  guint i, k;
  for (i = 1; i < spline->n_segs; i++)
    for (k = 0; k < interval_steps + (i + 1 == spline->n_segs); k++)
      {
        double x = spline->segs[i - 1].x + k * (spline->segs[i].x - spline->segs[i - 1].x) / interval_steps;
        double y1 = gxk_spline_y (spline, x);
        double z = gxk_spline_findx (spline, y1);
        double y2 = gxk_spline_y (spline, z);
        g_print ("%-+24.18g %-+24.18g # findx=%-+24.18g dx=%-+24.18f (%u) dy=%-+24.18g (%u)\n", x, y1, z, x-z, x==z, y1 - y2, y1==y2);
      }
}

static void
print_spline (guint                   n_points,
              const GxkSplinePoint   *points,
              guint                   steps,
              double                  yds,
              double                  yde)
{
  GxkSpline *spline;
  if (yds >= G_MAXDOUBLE || yde >= G_MAXDOUBLE)
    spline = gxk_spline_new_natural (n_points, points);
  else
    spline = gxk_spline_new (n_points, points, yds, yde);
  gxk_spline_dump (spline);
  spline_test (spline, steps);
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

  guint variant = 0, steps = 10;
  if (argc > 1)
    variant = atoi (argv[1]);
  if (argc > 2)
    steps = atoi (argv[2]);

  if (variant == 1)
    {
      GxkSplinePoint points[] = {
        { -24,    30 },
        { -7,     53 },
        { -3,    -6 },
        { +3.3,  -10 },
        { +10.2,  5 },
        { +18,   -24 },
      };
      print_spline (G_N_ELEMENTS (points), points, steps, -7, -50);
    }
  else if (variant == 2)
    {
      GxkSplinePoint points[] = {
        { +12, -3 },
        { -3,  +4 },
        { +2,  -28 },
        { -10, +4 },
        { +5,  -23 },
        { -1,  +13 },
      };
      print_spline (G_N_ELEMENTS (points), points, steps, +5, -60);
    }
  else if (variant == 3)
    {
      GxkSplinePoint points[] = {
        { 0, 25 },
        { 1, 25 },
        { 2, 25 },
        { 3, 25 },
        { 4, 25 },
        { 5, 25 },
      };
      print_spline (G_N_ELEMENTS (points), points, steps, G_MAXDOUBLE, G_MAXDOUBLE);
    }

  return 0;
}
