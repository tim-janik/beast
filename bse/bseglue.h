/* BSE - Bedevilled Sound Engine
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
 */
#ifndef __BSE_GLUE_H__
#define __BSE_GLUE_H__

#include <bse/bsetype.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* FIXME: creation of a new context source should be done
 * by a janitor constructor
 */

SfiGlueContext*	bse_glue_context_intern	 (const gchar	*user);

typedef SfiRec*	(*BseGlueBoxedToRec)	 (gpointer	    boxed);
typedef SfiSeq*	(*BseGlueBoxedToSeq)	 (gpointer	    boxed);

GType			bse_glue_pspec_get_original_enum (GParamSpec *pspec);
guint			bse_glue_enum_index	 (GType		    enum_type,
						  gint		    enum_value);
GType			bse_glue_make_rorecord	 (const gchar	   *rec_name,
						  GBoxedCopyFunc    copy,
						  GBoxedFreeFunc    free,
						  BseGlueBoxedToRec to_record);
GType			bse_glue_make_rosequence (const gchar	   *seq_name,
						  GBoxedCopyFunc    copy,
						  GBoxedFreeFunc    free,
						  BseGlueBoxedToSeq to_sequence);
GValue*			bse_glue_boxed_to_value	 (GType		    boxed_type,
						  gpointer	    boxed);

/* convert value sto/from SFI serializable types */
GValue*	bse_value_to_sfi	(const GValue	*value);
GValue*	bse_value_from_sfi	(const GValue	*value,
				 GParamSpec	*pspec);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_GLUE_H__ */

/* vim:set ts=8 sts=2 sw=2: */
