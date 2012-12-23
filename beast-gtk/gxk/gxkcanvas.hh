/* GXK - Gtk+ Extension Kit
 * Copyright (C) 2002-2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
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
