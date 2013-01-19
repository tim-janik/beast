// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#define _ISOC99_SOURCE
#include <math.h>
#include "gxk/gxkspline.hh"
#include <stdlib.h>
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
        if (!swap)
          g_print ("%-+24.18g %-+24.18g\n", x, y1);
        else if (1)
          g_print ("%-+24.18g %-+24.18g\n", y1, x);
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
  else if (ABS (variant) == 7)
    {
      GxkSplinePoint points[] = {
        { +12, 0 },
        {  +6, 1 },
        {  +3, 2 },
        {  +0, 3 },
        {  -3, 4 },
        {  -6, 5 },
        { -12, 6 },
        { -24, 7 },
        { -48, 8 },
        { -96, 9 },
      };
      print_spline (G_N_ELEMENTS (points), points, steps, NAN, NAN, variant < 0);
    }
  else if (ABS (variant) == 8)
    {
      GxkSplinePoint points[] = {
#if 1
        { +12, 0 },
        {  +6, 1 },
        {  +3, 2 },
        {  +0, 3 },
        {  -3, 4 },
        {  -6, 5 },
        { -12, 6 },
        { -24, 7 },
        { -48, 8 },
        { -96, 9 },
        { -1000, 10 },
#else
        { 0, +12,  },
        { 1, +6,  },
        { 2, +3,  },
        { 3, +0,  },
        { 4, -3,  },
        { 5, -6,  },
        { 6,-12,  },
        { 7,-24,  },
        { 8,-48,  },
        { 9,-96, },
        { 10,-1000,  },
#endif
      };
      print_spline (G_N_ELEMENTS (points), points, steps, NAN, NAN, variant < 0);
    }
  return 0;
}
