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
 * bswprivate.h: common functions in BSW which are not public API
 */
#ifndef __BSW_PRIVATE_H__
#define __BSW_PRIVATE_H__

#include        <bse/bswcommon.h>
#include        <bse/bseobject.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- BSW value iterators --- */
#define BSW_IS_VITER(vi)	((vi) && \
                                 ((vi)->type == BSW_TYPE_VITER_INT || \
                                  (vi)->type == BSW_TYPE_VITER_STRING || \
                                  (vi)->type == BSW_TYPE_VITER_BOXED || \
                                  (vi)->type == BSW_TYPE_VITER_PROXY))
#define	BSW_VALUE_WITH_VITER(v)	(G_VALUE_TYPE (v) == BSW_TYPE_VITER_INT || \
                                 G_VALUE_TYPE (v) == BSW_TYPE_VITER_STRING || \
                                 G_VALUE_TYPE (v) == BSW_TYPE_VITER_BOXED || \
                                 G_VALUE_TYPE (v) == BSW_TYPE_VITER_PROXY))
BswVIter*	bsw_viter_create			(GType		 type,
							 guint		 prealloc);
void		bsw_viter_append_int			(BswVIterInt	*iter,
							 gint		 v_int);
void		bsw_viter_append_string_take_ownership	(BswVIterString	*iter,
							 const gchar	*string);
#define	bsw_viter_append_string(vi,s)	bsw_viter_append_string_take_ownership ((vi), g_strdup (s))
void		bsw_viter_append_boxed_take_ownership	(BswVIterBoxed	*iter,
							 gpointer	 boxed);
void		bsw_viter_append_boxed			(BswVIterBoxed	*iter,
							 gconstpointer	 boxed);
void		bsw_viter_append_proxy			(BswVIterProxy	*iter,
							 BswProxy	 proxy);
#define bsw_viter_append_object(iter,obj)	G_STMT_START{ \
  gpointer __o = (obj); BswProxy __p = BSE_IS_OBJECT (__o) ? BSE_OBJECT_ID (__o) : 0; \
  bsw_viter_append_proxy ((iter), __p); \
}G_STMT_END


/* --- private boxed types --- */
BswPartNote*		bsw_part_note		(guint	tick,
						 guint	duration,
						 gfloat	freq,
						 gfloat	velocity);
BswNoteDescription*	bsw_note_description	(guint	note,
						 gint	fine_tune);


/* --- value type glue code --- */
static inline void
bsw_value_glue2bsw (const GValue *value,
		    GValue       *dest)
{
  if (G_VALUE_HOLDS_OBJECT (value))
    {
      GObject *object = g_value_get_object (value);

      if (value == dest)
	g_value_unset (dest);
      g_value_init (dest, BSW_TYPE_PROXY);
      g_value_set_pointer (dest, (gpointer) (BSE_IS_OBJECT (object) ? BSE_OBJECT_ID (object) : 0));
    }
  else if (value != dest)
    {
      g_value_init (dest, G_VALUE_TYPE (value));
      g_value_copy (value, dest);
    }
}

static inline void
bsw_value_glue2bse (const GValue *value,
		    GValue       *dest,
		    GType         object_type)
{
  if (G_VALUE_TYPE (value) == BSW_TYPE_PROXY)
    {
      guint object_id = (BswProxy) g_value_get_pointer (value);

      if (value == dest)
	g_value_unset (dest);
      g_value_init (dest, object_type);
      g_value_set_object (dest, bse_object_from_id (object_id));
    }
  else if (value != dest)
    {
      g_value_init (dest, G_VALUE_TYPE (value));
      g_value_copy (value, dest);
    }
}

static inline GType
bsw_property_type_from_bse (GType bse_type)
{
  if (G_TYPE_IS_OBJECT (bse_type))
    return BSW_TYPE_PROXY;
  else
    return bse_type;
}




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSW_PRIVATE_H__ */
