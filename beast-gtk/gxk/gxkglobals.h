/* GXK - Gtk+ Extension Kit
 * Copyright (C) 2002 Tim Janik
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
#ifndef __GXK_GLOBALS_H__
#define __GXK_GLOBALS_H__

#include <sfi/glib-extra.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

/* --- i18n and gettext helpers --- */
#ifdef GXK_COMPILATION
#  define GXK_I18N_DOMAIN NULL
#  define _(str)        dgettext (GXK_I18N_DOMAIN, str)
#  define T_(str)       dgettext (GXK_I18N_DOMAIN, str)
#  define N_(str)       (str)
#endif


/* --- macros --- */
#define	GXK_TOOLTIPS	(gxk_globals->tooltips)


/* --- typedefs & structures --- */
typedef void (*GxkFreeFunc) (gpointer data);
typedef struct
{
  GtkTooltips *tooltips;
} GxkGlobals;


/* --- spacing/padding --- */
#define	GXK_OUTER_BORDER	(5)	/* outer dialog border-width */
#define	GXK_INNER_SPACING	(3)	/* spacing/padding between h/v boxes */
#define	GXK_BUTTON_PADDING	(3)	/* padding between adjacent buttons */


/* --- convenience --- */
gulong  gxk_nullify_in_object (gpointer object,
                               gpointer location);


/* --- variables --- */
extern const GxkGlobals* gxk_globals;


/* --- functions --- */
void	gxk_init	(void);


/* --- internal --- */
void	gxk_init_utils		(void);
void	gxk_init_params	        (void);
void	gxk_init_stock		(void);
void	gxk_init_actions	(void);
void	gxk_init_assortments	(void);
void	gxk_init_radget_types	(void);


G_END_DECLS

#endif /* __GXK_GLOBALS_H__ */
