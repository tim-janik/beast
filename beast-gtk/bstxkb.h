/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999, 2000 Tim Janik and Red Hat, Inc.
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
#ifndef __BST_XKB_H__
#define __BST_XKB_H__

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#pragma }
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
#pragma {
}
#endif /* __cplusplus */

#endif /* __BST_XKB_H__ */
