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
#define	BSW_TYPE_PROXY		  (bsw_proxy_get_type ())
#define BSW_TYPE_VITER_INT        (bsw_viter_int_get_type ())
#define BSW_TYPE_VITER_STRING     (bsw_viter_string_get_type ())
#define BSW_TYPE_VITER_BOXED	  (bsw_viter_boxed_get_type ())
#define BSW_TYPE_VITER_PROXY      (bsw_viter_proxy_get_type ())
#define	BSW_TYPE_PART_NOTE	  (bsw_part_note_get_type ())
#define	BSW_TYPE_NOTE_DESCRIPTION (bsw_note_description_get_type ())
#define	BSW_TYPE_VALUE_BLOCK	  (bsw_value_block_get_type ())
#define	BSW_VALUE_HOLDS_PROXY(v)  (G_TYPE_CHECK_VALUE_TYPE ((v), BSW_TYPE_PROXY))


/* --- typedefs & structures --- */
typedef gsize		      BswProxy;
typedef struct _BswVIter      BswVIter;
typedef BswVIter              BswVIterInt;
typedef BswVIter              BswVIterString;
typedef BswVIter              BswVIterBoxed;
typedef BswVIter              BswVIterProxy;
typedef struct _BswValueBlock BswValueBlock;
typedef struct _BswIcon	      BswIcon;
struct _BswIcon
{
  guint   bytes_per_pixel; /* 3:RGB, 4:RGBA */
  guint   ref_count;       /* &(1<<31) indicates permanent ref counts */
  guint   width;
  guint   height;
  guint8 *pixels;
};
struct _BswValueBlock
{
  guint   ref_count;
  guint   n_values;
  gfloat  values[1];	/* flexible array */
};


/* --- BSW proxy --- */
GType		bsw_proxy_get_type		(void);
void		bsw_value_set_proxy		(GValue		*value,
						 BswProxy	 proxy);
BswProxy	bsw_value_get_proxy		(const GValue	*value);
GParamSpec*     bsw_param_spec_proxy            (const gchar    *name,
						 const gchar    *nick,
						 const gchar    *blurb,
						 GParamFlags     flags);


/* --- BSW value iterators --- */
GType		bsw_viter_int_get_type		(void);
GType		bsw_viter_string_get_type	(void);
GType		bsw_viter_boxed_get_type	(void);
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
gpointer        bsw_viter_get_boxed             (BswVIterBoxed  *iter);
BswProxy        bsw_viter_get_proxy             (BswVIterProxy  *iter);


/* -- BSW Notes --- */
typedef struct
{
  guint  tick;
  guint	 duration;	/* in ticks */
  gfloat freq;
  gfloat velocity;	/* 0 .. 1 */
} BswPartNote;
GType		bsw_part_note_get_type		(void);
void		bsw_part_note_free		(BswPartNote	*pnote);


typedef struct
{
  guint    note;
  gint     octave;
  guint	   half_tone;	/* 0 .. 11 */
  gboolean upshift;
  gfloat   freq;
  gint     fine_tune;
  gchar    letter;	/* "\0" if invalid */
  gchar	  *name;	/* NULL if invalid */
  /* constants */
  gint     max_fine_tune;
  guint    kammer_note;
} BswNoteDescription;
GType		bsw_note_description_get_type	(void);
void		bsw_note_description_free	(BswNoteDescription	*info);


/* --- BSW value block --- */
GType		bsw_value_block_get_type	(void);
BswValueBlock*	bsw_value_block_new		(guint		 n_values);
BswValueBlock*	bsw_value_block_ref		(BswValueBlock	*vblock);
void		bsw_value_block_unref		(BswValueBlock	*vblock);


/* --- BSW icons --- */
BswIcon*	bsw_icon_ref_static		(BswIcon	*icon);
BswIcon*	bsw_icon_ref			(BswIcon	*icon);
void		bsw_icon_unref			(BswIcon	*icon);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSW_COMMON_H__ */
