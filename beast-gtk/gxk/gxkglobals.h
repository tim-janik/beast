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
#ifndef __GXK_GLOBALS_H__
#define __GXK_GLOBALS_H__

#include        <gtk/gtk.h>

G_BEGIN_DECLS

/* --- structures --- */
typedef struct
{
  GtkTooltips *tooltips;
} GxkGlobals;


/* --- macros --- */
#define	GXK_TOOLTIPS	(gxk_globals->tooltips)


/* --- spacing/padding --- */
#define	GXK_SIBLING_PADDING	(3)	/* padding between neighboured buttons, etc. */
#define	GXK_INNER_PADDING	(3)	/* padding between property input fields */
#define	GXK_OUTER_PADDING	(4)	/* padding between functional blocks */


/* --- convenience --- */
#define gxk_nullify_on_destroy(object, location) \
  g_signal_connect_swapped ((object), "destroy", G_CALLBACK (g_nullify_pointer), (location))


/* --- variables --- */
extern const GxkGlobals* gxk_globals;


/* --- functions --- */
void	gxk_init	(void);


/* --- internal --- */
void	_gxk_init_utils		(void);
void	_gxk_init_stock		(void);


G_END_DECLS

#endif /* __GXK_GLOBALS_H__ */
