/* GXK - Gtk+ Extension Kit
 * Copyright (C) 1998-2002 Tim Janik
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
#ifndef __GXK_UTILS_H__
#define __GXK_UTILS_H__

#include        <gxk/gxkglobals.h>

G_BEGIN_DECLS

/* --- generated includes --- */
/* type IDs */
#include <gxk/gxkgentypes.h>
/* marshallers */
#include <gxk/gxkmarshal.h>


/* --- GObject convenience --- */
typedef struct {
  gchar        *type_name;
  GType         parent;
  GType        *type_id;
  gconstpointer type_data;	/* e.g. GEnumValue array */
} GxkTypeGenerated;
void    gxk_type_register_generated	(guint			 n_entries,
					 const GxkTypeGenerated	*entries);
void	g_object_set_long		(gpointer		 object,
					 const gchar		*name,
					 glong			 v_long);
glong	g_object_get_long		(gpointer		 object,
					 const gchar		*name);
#define	g_object_set_int		g_object_set_long	// FIXME
#define	g_object_get_int		g_object_get_long	// FIXME


/* --- Gtk convenience --- */
void	gxk_widget_make_insensitive	(GtkWidget	*widget);
void	gxk_widget_make_sensitive	(GtkWidget	*widget);
#define GTK_STYLE_THICKNESS(s,xy)	((s)-> xy##thickness)
void	gxk_toplevel_delete		(GtkWidget	*widget);


/* --- widget appearance --- */
void	gxk_widget_modify_as_title	(GtkWidget	*widget);
void	gxk_widget_modify_bg_as_base	(GtkWidget	*widget);
void	gxk_widget_modify_base_as_bg	(GtkWidget	*widget);
void	gxk_widget_force_bg_clear	(GtkWidget	*widget);


/* --- signal convenience --- */
gboolean	gxk_signal_handler_pending	(gpointer	 instance,
						 const gchar	*detailed_signal,
						 GCallback	 callback,
						 gpointer	 data);


G_END_DECLS

#endif /* __GXK_UTILS_H__ */
