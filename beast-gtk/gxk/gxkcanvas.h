/* GXK - Gtk+ Extension Kit
 * Copyright (C) 2002 Tim Janik
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
#ifndef __GXK_CANVAS_H__
#define __GXK_CANVAS_H__

#include        <gxk/gxkglobals.h>
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
