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


/* --- BSW abstract iterator --- */
typedef		struct _BswIter			BswIter;
#define		BSW_IS_ITER(iter)		(bsw_iter_check (iter))
void		bsw_iter_rewind			(BswIter	*iter);
guint		bsw_iter_n_left			(BswIter	*iter);
void		bsw_iter_next			(BswIter	*iter);
void		bsw_iter_prev			(BswIter	*iter);
void		bsw_iter_jump			(BswIter	*iter,
						 guint		 nth);
BswIter*	bsw_iter_copy			(BswIter	*iter);
void		bsw_iter_free			(BswIter	*iter);
gboolean	bsw_iter_check			(const BswIter	*iter);
gboolean	bsw_iter_check_is_a		(const BswIter	*iter,
						 GType		 type);


/* --- BSW discrete iterators --- */
typedef		BswIter				BswIterInt;
#define		BSW_TYPE_ITER_INT		(bsw_iter_int_get_type ())
#define		BSW_IS_ITER_INT(iter)		(bsw_iter_check_is_a ((iter), BSW_TYPE_ITER_INT))
GType		bsw_iter_int_get_type		(void);
gint		bsw_iter_get_int		(BswIterInt	*iter);
typedef		BswIter				BswIterString;
#define		BSW_TYPE_ITER_STRING		(bsw_iter_string_get_type ())
#define		BSW_IS_ITER_STRING(iter)	(bsw_iter_check_is_a ((iter), BSW_TYPE_ITER_STRING))
GType		bsw_iter_string_get_type	(void);
const gchar*	bsw_iter_get_string		(BswIterString	*iter);


/* --- BSW Proxy (object wrapper) --- */
#define		BSW_TYPE_PROXY			(bsw_proxy_get_type ())
#define		BSW_VALUE_HOLDS_PROXY(v)	(G_TYPE_CHECK_VALUE_TYPE ((v), BSW_TYPE_PROXY))
typedef		gsize				 BswProxy;
GType		bsw_proxy_get_type		(void);
void		bsw_value_set_proxy		(GValue		*value,
						 BswProxy	 proxy);
BswProxy	bsw_value_get_proxy		(const GValue	*value);
GParamSpec*     bsw_param_spec_proxy		(const gchar    *name,
						 const gchar    *nick,
						 const gchar    *blurb,
						 GParamFlags     flags);

typedef		BswIter				BswIterProxy;
#define		BSW_TYPE_ITER_PROXY		(bsw_iter_proxy_get_type ())
#define		BSW_IS_ITER_PROXY(iter)		(bsw_iter_check_is_a ((iter), BSW_TYPE_ITER_PROXY))
GType		bsw_iter_proxy_get_type		(void);
BswProxy	bsw_iter_get_proxy		(BswIterProxy	*iter);


/* --- BSW Part Note --- */
#define		BSW_TYPE_PART_NOTE		(bsw_part_note_get_type ())
typedef struct
{
  guint	   id;
  guint    tick;
  guint	   duration;	/* in ticks */
  gint     note;
  gint     fine_tune;
  gfloat   velocity;	/* 0 .. 1 */
  gboolean selected;
} BswPartNote;
GType		bsw_part_note_get_type		(void);
void		bsw_part_note_free		(BswPartNote	*pnote);

typedef		BswIter				BswIterPartNote;
#define		BSW_TYPE_ITER_PART_NOTE		(bsw_iter_part_note_get_type ())
#define		BSW_IS_ITER_PART_NOTE(iter)	(bsw_iter_check_is_a ((iter), BSW_TYPE_ITER_PART_NOTE))
GType		bsw_iter_part_note_get_type	(void);
BswPartNote*	bsw_iter_get_part_note		(BswIterPartNote *iter);


/* -- BSW Note Description --- */
#define		BSW_TYPE_NOTE_DESCRIPTION	(bsw_note_description_get_type ())
typedef struct
{
  guint    note;
  gint     octave;
  gfloat   freq;
  gint     fine_tune;
  guint	   half_tone;	/* 0 .. 11 */
  gboolean upshift;
  gchar    letter;	/* "\0" if invalid */
  gchar	  *name;	/* NULL if invalid */
  /* constants */
  gint     max_fine_tune;
  guint    kammer_note;
} BswNoteDescription;
GType		bsw_note_description_get_type	(void);
void		bsw_note_description_free	(BswNoteDescription	*info);


/* --- BSW Note Sequence --- */
#define		BSW_TYPE_NOTE_SEQUENCE		(bsw_note_sequence_get_type ())
typedef struct
{
  guint  n_notes;
  gint   offset;	/* center/base note */
  struct {
    gint note;
  } notes[1];		/* flexible array */
} BswNoteSequence;
GType		 bsw_note_sequence_get_type	(void);
BswNoteSequence* bsw_note_sequence_new		(guint			 n_notes);
BswNoteSequence* bsw_note_sequence_copy		(const BswNoteSequence	*sequence);
void		 bsw_note_sequence_free		(BswNoteSequence	*sequence);
BswNoteSequence* bsw_note_sequence_resize	(BswNoteSequence	*sequence,
						 guint			 n_notes);


/* --- BSW value block --- */
#define		BSW_TYPE_VALUE_BLOCK		(bsw_value_block_get_type ())
typedef struct
{
  guint   ref_count;
  guint   n_values;
  gfloat  values[1];	/* flexible array */
} BswValueBlock;
GType		bsw_value_block_get_type	(void);
BswValueBlock*	bsw_value_block_new		(guint		 n_values);
BswValueBlock*	bsw_value_block_ref		(BswValueBlock	*vblock);
void		bsw_value_block_unref		(BswValueBlock	*vblock);


/* --- BSW Icon --- */
typedef struct
{
  guint   bytes_per_pixel; /* 3:RGB, 4:RGBA */
  guint   ref_count;       /* &(1<<31) indicates permanent ref counts */
  guint   width;
  guint   height;
  guint8 *pixels;
} BswIcon;
BswIcon*	bsw_icon_ref_static		(BswIcon	*icon);
BswIcon*	bsw_icon_ref			(BswIcon	*icon);
void		bsw_icon_unref			(BswIcon	*icon);


/* --- initialize scripts and plugins --- */
void	bsw_register_plugins	(const gchar	*path,
				 gboolean	 verbose,
				 gchar	       **messages);
void	bsw_register_scripts	(const gchar	*path,
				 gboolean	 verbose,
				 gchar	       **messages);


/* --- missing GLib --- */
gchar*  g_type_name_to_cname            (const gchar    *type_name);
gchar*  g_type_name_to_sname            (const gchar    *type_name);
gchar*  g_type_name_to_cupper           (const gchar    *type_name);
gchar*  g_type_name_to_type_macro       (const gchar    *type_name);
gchar*  bsw_type_name_to_sname          (const gchar    *type_name);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSW_COMMON_H__ */
