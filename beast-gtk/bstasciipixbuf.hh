/* BEAST - Better Audio System
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
#ifndef __BST_ASCII_PIXBUF_H__
#define __BST_ASCII_PIXBUF_H__

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


void		bst_ascii_pixbuf_ref	(void);
GdkPixbuf*	bst_ascii_pixbuf_new	(gchar character,
					 guint char_width,
					 guint char_height);
void		bst_ascii_pixbuf_unref	(void);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_ASCII_PIXBUF_H__ */
