/* GLib Extra - Tentative GLib code and GLib supplements
 * Copyright (C) 1997-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __SFI_GLIB_EXTRA_H__
#define __SFI_GLIB_EXTRA_H__

#include	<glib.h>
#include	<glib-object.h>


G_BEGIN_DECLS


/* --- provide (historic) aliases --- */
#define	g_string_printfa	g_string_append_printf
#define	g_scanner_add_symbol( scanner, symbol, value )	G_STMT_START { \
  g_scanner_scope_add_symbol ((scanner), 0, (symbol), (value)); \
} G_STMT_END
#define	g_scanner_remove_symbol( scanner, symbol )	G_STMT_START { \
  g_scanner_scope_remove_symbol ((scanner), 0, (symbol)); \
} G_STMT_END


/* --- abandon typesafety for some frequently used functions --- */
#define g_object_notify(o,s)	g_object_notify ((gpointer) (o), (s))


/* --- string functions --- */
gchar**		g_straddv	 (gchar	       **str_array,
				  const gchar	*new_str);
gchar**		g_strslistv	(GSList		*slist);
guint		g_strlenv	(gchar	       **str_array);


/* --- double array --- */
typedef struct {
  guint    n_values;
  gdouble *values;
  guint    n_prealloced;
} GDArray;

GDArray*	g_darray_new	(guint		 prealloc);
void		g_darray_free	(GDArray	*darray);
gdouble		g_darray_get	(GDArray	*darray,
				 guint		 index);
void		g_darray_insert	(GDArray        *darray,
				 guint           index,
				 gdouble	 value);
void		g_darray_append	(GDArray        *darray,
				 gdouble	 value);
void		g_darray_set	(GDArray        *darray,
				 guint           index,
				 gdouble	 value);
#define	g_darray_prepend(a,v)	g_darray_insert ((a), 0, (v))



/* --- signal queue --- */
#if 0
typedef gboolean (*GUSignalFunc) (gint8          usignal,
			 	  gpointer       data);
guint   g_usignal_add            (gint8          usignal,
				  GUSignalFunc   function,
				  gpointer       data);
guint   g_usignal_add_full       (gint           priority,
				  gint8          usignal,
				  GUSignalFunc   function,
				  gpointer       data,
				  GDestroyNotify destroy);
void    g_usignal_notify         (gint8          usignal);
#endif


G_END_DECLS

#endif /* __SFI_GLIB_EXTRA_H__ */
