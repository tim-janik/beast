/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-2002 Tim Janik
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
 */
#ifndef __BSE_UTILS_H__
#define __BSE_UTILS_H__

#include	<bse/bseenums.h>
#include	<bse/bseglobals.h>
#include	<bse/bsecompat.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- dates and times --- */
#define BSE_MAX_DATE_ERRORS		(10)
BseTime		bse_time_current	(void);
BseTime		bse_time_to_gmt		(BseTime	time_val);
BseTime		bse_time_from_gmt	(BseTime	time_val);
gchar*		bse_time_to_str		(BseTime	time_val);
gchar*		bse_time_to_bbuffer	(BseTime	time_val,
					 gchar		bbuffer[BSE_BBUFFER_SIZE]);
/* bse_time_from_string() returns a time > BSE_MIN_TIME, or 0 if
 * the parsing errors were really fundamental.
 */
BseTime		bse_time_from_string	(const gchar   *time_string,
					 BseErrorType	errors[BSE_MAX_DATE_ERRORS]);


/* --- record utils --- */
BseNoteDescription*	bse_note_description	(SfiInt note,
						 gint   fine_tune);
BsePartNote*		bse_part_note		(guint    id,
						 guint    tick,
						 guint    duration,
						 gint     note,
						 gint     fine_tune,
						 gfloat   velocity,
						 gboolean selected);
void			bse_part_note_seq_take_append	(BsePartNoteSeq *seq,
							 BsePartNote    *element);
BseNoteSequence* bse_note_sequence_copy_deep	(BseNoteSequence *rec);
void             bse_note_sequence_resize	(BseNoteSequence *rec,
						 guint            length);
guint            bse_note_sequence_length	(BseNoteSequence *rec);


/* --- notes & frequencies --- */
gint	bse_note_from_freq	(gdouble	 freq);
gint    bse_note_fine_tune_from_note_freq (gint    note,
					   gdouble freq);
gdouble	bse_note_to_freq	(gint		 note);
gdouble	bse_note_to_tuned_freq	(gint		 note,
				 gint            fine_tune);

/* find match_freq in inclusive_set (NULL acts as wildcard) and don't
 * find match_freq in exclusive_set (NULL acts as empty set). the sets
 * have to contain GValues of type G_TYPE_FLOAT.
 */
gboolean        bse_value_arrays_match_freq     (gfloat          match_freq,
						 GValueArray    *inclusive_set,
						 GValueArray    *exclusive_set);
gboolean        bse_darrays_match_freq	        (gfloat          match_freq,
						 GDArray        *inclusive_set,
						 GDArray        *exclusive_set);


/* --- icons --- */
BseIcon* bse_icon_from_pixdata (const BsePixdata *pixdata);


/* --- miscellaeous --- */
/* these functions mutate special characters to conform to
 * internal rules about sample names. operation is performed
 * on the source string.
 */
gchar*		bse_sample_name_make_valid	(gchar		*string);
gchar*		bse_song_name_make_valid	(gchar		*string);

guint		bse_string_hash			(gconstpointer   string);
gint		bse_string_equals		(gconstpointer	 string1,
						 gconstpointer	 string2);

void		bse_nullify			(gpointer	*location);


/* --- file utils --- */
void		bse_str_slist_free		(GSList		*slist);
GSList*		bse_path_pattern_list_matches	(const gchar	*file_pattern,
						 const gchar	*cwd,
						 GFileTest	 file_test);
GSList*		bse_search_path_list_entries	(const gchar	*search_path);
GSList*		bse_search_path_list_matches	(const gchar	*search_path,
						 const gchar	*cwd);
GSList*		bse_search_path_list_files	(const gchar	*search_path,
						 const gchar	*file_pattern,
						 const gchar	*cwd,
						 GFileTest	 file_test);


/* --- bbuffer utils --- */
void	bse_bbuffer_puts	(gchar       	 bbuffer[BSE_BBUFFER_SIZE],
				 const gchar	*string);
guint	bse_bbuffer_printf	(gchar		 bbuffer[BSE_BBUFFER_SIZE],
				 const gchar    *format,
				 ...) G_GNUC_PRINTF (2, 3);
static inline void
bse_bbuffer_putc (gchar bbuffer[BSE_BBUFFER_SIZE],
		  gchar character)
{
  bbuffer[0] = character;
  bbuffer[1] = 0;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_UTILS_H__ */
