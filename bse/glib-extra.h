/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2001 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * glib-extra.h: this file covers stuff that's missing from GLib 1.2.x
 */
#ifndef __GLIB_EXTRA_H__
#define __GLIB_EXTRA_H__

#include	<glib.h>
#include	<glib-object.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- saner aliases --- */
#define	g_string_printfa	g_string_append_printf
#define	g_scanner_add_symbol( scanner, symbol, value )	G_STMT_START { \
  g_scanner_scope_add_symbol ((scanner), 0, (symbol), (value)); \
} G_STMT_END
#define	g_scanner_remove_symbol( scanner, symbol )	G_STMT_START { \
  g_scanner_scope_remove_symbol ((scanner), 0, (symbol)); \
} G_STMT_END


/* --- string functions --- */
gchar*  g_strdup_quoted         (const gchar   *string);


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


/* --- bsearch impl --- */
#define G_BSEARCH_BEGIN(n_elements, elements, key_element, element_size)      \
{                                                                             \
  const gchar *G_BSEARCH_ELEMENT2 = NULL;                                     \
  const gchar *_glib__base = (elements);                                      \
  const gchar *G_BSEARCH_ELEMENT1 = (key_element);                            \
  guint _glib__n_elements = (n_elements);                                     \
  guint _glib__offset = 0, _glib__esize = (element_size);                     \
  gpointer *_glib__ret;                                                       \
  gboolean _glib__match = FALSE;                                              \
                                                                              \
  while (_glib__offset < _glib__n_elements)                                   \
    {                                                                         \
      guint _glib__index = (_glib__offset + _glib__n_elements) >> 1;          \
      gint _glib__diff;                                                       \
                                                                              \
      G_BSEARCH_ELEMENT2 = _glib__base + _glib__index * _glib__esize;       { \

/* difference = compare_func (G_BSEARCH_ELEMENT1, G_BSEARCH_ELEMENT2); */

#define G_BSEARCH_END(difference, match_nearest, return_loc)                } \
       _glib__diff = (difference);                                             \
      if (_glib__diff < 0)                                                    \
        _glib__n_elements = _glib__index;                                     \
      else if (_glib__diff > 0)                                               \
        _glib__offset = _glib__index + 1;                                     \
      else /* _glib__diff == 0 */                                             \
        {                                                                     \
          _glib__match = TRUE;                                                \
          break;                                                              \
        }                                                                     \
    }                                                                         \
  _glib__match = _glib__match || (match_nearest);                             \
  _glib__ret = (return_loc);                                                  \
  *_glib__ret = _glib__match ? (gpointer) G_BSEARCH_ELEMENT2 : NULL;          \
}


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




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GLIB_EXTRA_H__ */
