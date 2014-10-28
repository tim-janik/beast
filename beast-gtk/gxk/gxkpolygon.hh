// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GXK_POLYGON_H__
#define __GXK_POLYGON_H__

#include "gxkutils.hh"

G_BEGIN_DECLS

/* --- type macros --- */
#define GXK_TYPE_POLYGON              (gxk_polygon_get_type ())
#define GXK_POLYGON(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_POLYGON, GxkPolygon))
#define GXK_POLYGON_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_POLYGON, GxkPolygonClass))
#define GXK_IS_POLYGON(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_POLYGON))
#define GXK_IS_POLYGON_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_POLYGON))
#define GXK_POLYGON_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GXK_TYPE_POLYGON, GxkPolygonClass))


/* --- structures --- */
typedef struct {
  gfloat x1, y1, x2, y2;
  GtkShadowType draw_type;
} GxkPolygonLine;
typedef struct {
  gfloat xc, yc, xr, yr, sa, ea;
  GtkShadowType draw_type;
} GxkPolygonArc;
typedef struct {
  GtkWidget	 parent_instance;
  guint		 n_lines;
  GxkPolygonLine	*lines;
  guint		 n_arcs;
  GxkPolygonArc	*arcs;
  guint		 request_length;
} GxkPolygon;
typedef struct {
  GtkWidgetClass parent_class;
} GxkPolygonClass;
typedef struct {
  guint	          n_lines;
  GxkPolygonLine *lines;
  guint	          n_arcs;
  GxkPolygonArc  *arcs;
  guint		  length;
} GxkPolygonGraph;


/* --- prototypes --- */
GType		gxk_polygon_get_type		(void);
gpointer	gxk_polygon_new			(GxkPolygonGraph *polygon_graph);
void		gxk_polygon_set_lines		(GxkPolygon	 *self,
						 guint	 	  n_lines,
						 GxkPolygonLine	 *lines);
void		gxk_polygon_set_arcs		(GxkPolygon	 *self,
						 guint	 	  n_arcs,
						 GxkPolygonArc	 *arcs);
void		gxk_polygon_set_graph		(GxkPolygon	 *self,
						 GxkPolygonGraph *polygon_graph);
void		gxk_polygon_set_length		(GxkPolygon	 *self,
						 guint	 	  length);
extern GxkPolygonGraph gxk_polygon_power;
extern GxkPolygonGraph gxk_polygon_stop;
extern GxkPolygonGraph gxk_polygon_pause;
extern GxkPolygonGraph gxk_polygon_first;
extern GxkPolygonGraph gxk_polygon_previous;
extern GxkPolygonGraph gxk_polygon_rewind;
extern GxkPolygonGraph gxk_polygon_play;
extern GxkPolygonGraph gxk_polygon_forward;
extern GxkPolygonGraph gxk_polygon_next;
extern GxkPolygonGraph gxk_polygon_last;

G_END_DECLS

#endif  /* __GXK_POLYGON_H__ */
