/* BSE - Better Sound Engine
 * Copyright (C) 1998-2002 Tim Janik
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
#ifndef __BSE_UTILS_H__
#define __BSE_UTILS_H__

#include <bse/bseenums.h>
#include <bse/bseglobals.h>
#include <bse/bsecompat.h>

G_BEGIN_DECLS

/* --- C++ helper declaration --- */
void    bse_cxx_init      (void);


/* --- record utils --- */
BseNoteDescription* bse_note_description             (BseMusicalTuningType   musical_tuning,
                                                      int                    note,
                                                      int                    fine_tune);
BsePartNote*        bse_part_note                    (guint                  id,
                                                      guint                  channel,
                                                      guint                  tick,
                                                      guint                  duration,
                                                      gint                   note,
                                                      gint                   fine_tune,
                                                      gfloat                 velocity,
                                                      gboolean               selected);
void                bse_part_note_seq_take_append    (BsePartNoteSeq        *seq,
                                                      BsePartNote           *element);
BsePartControl*     bse_part_control                 (guint                  id,
                                                      guint                  tick,
                                                      BseMidiSignalType      ctype,
                                                      gfloat                 value,
                                                      gboolean               selected);
void                bse_part_control_seq_take_append (BsePartControlSeq     *seq,
                                                      BsePartControl        *element);
void                bse_note_sequence_resize         (BseNoteSequence       *rec,
                                                      guint                  length);
guint               bse_note_sequence_length         (BseNoteSequence       *rec);
void                bse_property_candidate_relabel   (BsePropertyCandidates *pc,
                                                      const gchar           *label,
                                                      const gchar           *tooltip);
void                bse_item_seq_remove              (BseItemSeq            *iseq,
                                                      BseItem               *item);
SfiRing*            bse_item_seq_to_ring             (BseItemSeq            *iseq);
BseItemSeq*         bse_item_seq_from_ring           (SfiRing               *ring);


/* --- debugging --- */
void    bse_debug_dump_floats   (guint   debug_stream,
                                 guint   n_channels,
                                 guint   mix_freq,
                                 guint   n_values,
                                 gfloat *values);


/* --- balance calculation --- */
/* levels are 0..100, balance is -100..+100 */
double  bse_balance_get         (double  level1,
                                 double  level2);
void    bse_balance_set         (double  balance,
                                 double *level1,
                                 double *level2);


/* --- icons --- */
BseIcon* bse_icon_from_pixstream (const guint8     *pixstream);


/* --- ID allocator --- */
gulong	bse_id_alloc	(void);
void	bse_id_free	(gulong	id);


/* --- string array manipulation --- */
gchar**       bse_xinfos_add_value              (gchar          **xinfos,
                                                 const gchar     *key,
                                                 const gchar     *value);
gchar**       bse_xinfos_add_float              (gchar          **xinfos,
                                                 const gchar     *key,
                                                 gfloat           fvalue);
gchar**       bse_xinfos_add_num                (gchar          **xinfos,
                                                 const gchar     *key,
                                                 SfiNum           num);
gchar**       bse_xinfos_parse_assignment       (gchar          **xinfos,
                                                 const gchar     *assignment);
gchar**       bse_xinfos_del_value              (gchar          **xinfos,
                                                 const gchar     *key);
const gchar*  bse_xinfos_get_value              (gchar          **xinfos,
                                                 const gchar     *key);
gfloat        bse_xinfos_get_float              (gchar          **xinfos,
                                                 const gchar     *key);
SfiNum        bse_xinfos_get_num                (gchar          **xinfos,
                                                 const gchar     *key);
gchar**       bse_xinfos_dup_consolidated       (gchar          **xinfos,
                                                 gboolean         copy_interns);
gint          bse_xinfo_stub_compare            (const gchar     *xinfo1,  /* must contain '=' */
                                                 const gchar     *xinfo2); /* must contain '=' */


/* --- miscellaeous --- */
guint		bse_string_hash			(gconstpointer   string);
gint		bse_string_equals		(gconstpointer	 string1,
						 gconstpointer	 string2);
const gchar*    bse_intern_path_user_data       (const gchar    *dir);
const gchar*    bse_intern_default_author	(void);
const gchar*    bse_intern_default_license	(void);


/* --- bbuffer utils --- */
#define BSE_BBUFFER_SIZE        (128)
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

G_END_DECLS

#endif /* __BSE_UTILS_H__ */
