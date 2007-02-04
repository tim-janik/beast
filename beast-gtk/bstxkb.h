/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2002 Tim Janik and Red Hat, Inc.
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
#ifndef __BST_XKB_H__
#define __BST_XKB_H__

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- prototypes --- */
gboolean	bst_xkb_open		(const gchar	*display,
					 gboolean	 sync);
void		bst_xkb_close		(void);
void		bst_xkb_dump		(void);
const gchar*	bst_xkb_get_symbol 	(gboolean	 physical);
void		bst_xkb_parse_symbol	(const gchar	*symbol,
					 gchar         **encoding_p,
					 gchar         **model_p,
					 gchar         **layout_p,
					 gchar         **variant_p);






#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_XKB_H__ */
