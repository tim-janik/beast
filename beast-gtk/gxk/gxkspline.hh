// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GXK_SPLINE_H__
#define __GXK_SPLINE_H__

#include        <gxk/gxkglobals.hh>

G_BEGIN_DECLS

typedef struct _GxkSpline GxkSpline;
typedef struct {
  double x, y;
} GxkSplinePoint;
typedef struct {
  double x, y, yd2;
  double ymin, ymax;
  double ex1, ex2;
} GxkSplineSegment;
struct _GxkSpline {
  guint             n_segs;     /* == n_points */
  GxkSplineSegment  segs[0];    /* last segment always has NAN extrema */
};
GxkSpline*      gxk_spline_new_natural  (guint                   n_points,
                                         const GxkSplinePoint   *points);
GxkSpline*      gxk_spline_new          (guint                   n_points,
                                         const GxkSplinePoint   *points,
                                         double                  dy_start,
                                         double                  dy_end);
double          gxk_spline_eval         (const GxkSpline        *spline,
                                         double                  x,
                                         double                 *yd1);
double          gxk_spline_y            (const GxkSpline        *spline,
                                         double                  x);
double          gxk_spline_findx        (const GxkSpline        *spline,
                                         double                  y);
void            gxk_spline_dump         (GxkSpline              *spline);
GxkSpline*      gxk_spline_copy         (GxkSpline              *spline);
void            gxk_spline_free         (GxkSpline              *spline);



G_END_DECLS

#endif /* __GXK_SPLINE_H__ */
