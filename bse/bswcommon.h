/* BSW - Bedevilled Sound Engine Wrapper
 * Copyright (C) 2002 Tim Janik
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
 * bswcommon.h: BSW Types used also by BSE
 */
#ifndef __BSW_COMMON_H__
#define __BSW_COMMON_H__

#include	<bse/glib-extra.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- type macros --- */
#define	BSW_TYPE_PROXY		 (bsw_proxy_get_type ())
#define BSW_TYPE_VITER_INT       (bsw_viter_int_get_type ())
#define BSW_TYPE_VITER_STRING    (bsw_viter_string_get_type ())
#define BSW_TYPE_VITER_PROXY     (bsw_viter_proxy_get_type ())
#define	BSW_VALUE_HOLDS_PROXY(v) (G_TYPE_CHECK_VALUE_TYPE ((v), BSW_TYPE_PROXY))


/* --- typedefs --- */
typedef gsize		 BswProxy;
typedef struct _BswVIter BswVIter;
typedef BswVIter         BswVIterInt;
typedef BswVIter         BswVIterString;
typedef BswVIter         BswVIterProxy;


/* --- BSW proxy --- */
GType		bsw_proxy_get_type		(void);
void		bsw_value_set_proxy		(GValue		*value,
						 BswProxy	 proxy);
BswProxy	bsw_value_get_proxy		(const GValue	*value);


/* --- BSW value iterators --- */
GType		bsw_viter_int_get_type		(void);
GType		bsw_viter_string_get_type	(void);
GType		bsw_viter_proxy_get_type	(void);
GType           bsw_viter_type                  (BswVIter       *iter);
void            bsw_viter_rewind                (BswVIter       *iter);
guint           bsw_viter_n_left                (BswVIter       *iter);
void            bsw_viter_next                  (BswVIter       *iter);
void            bsw_viter_prev                  (BswVIter       *iter);
void            bsw_viter_jump                  (BswVIter       *iter,
						 guint           nth);
BswVIter*       bsw_viter_copy                  (BswVIter       *iter);
void            bsw_viter_free                  (BswVIter       *iter);
gint            bsw_viter_get_int               (BswVIterInt    *iter);
gchar*          bsw_viter_get_string            (BswVIterString *iter);
BswProxy        bsw_viter_get_proxy             (BswVIterProxy  *iter);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSW_COMMON_H__ */
