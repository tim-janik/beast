// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GXK_CANVAS_H__
#define __GXK_CANVAS_H__

#include        <gxk/gxkglobals.hh>
#include        <libgnomecanvas/libgnomecanvas.h>

G_BEGIN_DECLS

/* --- functions --- */
GnomeCanvasPoints*	gnome_canvas_points_new0	(guint			 num_points);
GnomeCanvasPoints*	gnome_canvas_points_newv	(guint			 num_points,
							 ...);
GnomeCanvasItem*	gnome_canvas_typed_item_at	(GnomeCanvas		*canvas,
							 GtkType		 item_type,
							 gdouble		 world_x,
							 gdouble		 world_y);
gboolean        gnome_canvas_item_check_undisposed      (GnomeCanvasItem        *item);
guint		gnome_canvas_item_get_stacking		(GnomeCanvasItem	*item);
void		gnome_canvas_item_keep_between		(GnomeCanvasItem	*between,
							 GnomeCanvasItem	*item1,
							 GnomeCanvasItem	*item2);
void		gnome_canvas_item_keep_above		(GnomeCanvasItem	*above,
							 GnomeCanvasItem	*item1,
							 GnomeCanvasItem	*item2);
void		gnome_canvas_text_set_zoom_size		(GnomeCanvasText	*item,
							 gdouble		 pixels);
void		gnome_canvas_set_zoom			(GnomeCanvas		*canvas,
							 gdouble		 pixels_per_unit);
void		gnome_canvas_FIXME_hard_update		(GnomeCanvas		*canvas);


G_END_DECLS

#endif /* __GXK_CANVAS_H__ */
