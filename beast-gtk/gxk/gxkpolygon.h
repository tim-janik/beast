/* GXK - Gtk+ Extension Kit
 * Copyright (C) 2003 Tim Janik
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
#ifndef __GXK_POLYGON_H__
#define __GXK_POLYGON_H__

#include "gxkutils.h"

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
} GxkPolygonLine;
typedef struct {
  gfloat xc, yc, xr, yr, sa, ea;
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
