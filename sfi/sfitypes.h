/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Tim Janik
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
#ifndef __SFI_TYPES_H__
#define __SFI_TYPES_H__

#include <sfi/glib-extra.h>

G_BEGIN_DECLS


/* --- Sfi typedefs --- */
typedef gboolean		SfiBool;
typedef gint			SfiInt;
typedef gint64			SfiNum;
typedef guint64			SfiTime;
typedef gdouble			SfiReal;
#define SFI_MINREAL (2.2250738585072014e-308)	/* IEEE754 double */
#define SFI_MAXREAL (1.7976931348623157e+308)	/* IEEE754 double */
typedef gchar*			SfiString;
typedef const gchar*		SfiChoice;
typedef struct _SfiBBlock	SfiBBlock;
typedef struct _SfiFBlock	SfiFBlock;
typedef struct _SfiSeq		SfiSeq;
typedef struct _SfiRec		SfiRec;
typedef GType /* pointer */	SfiProxy;
typedef struct _SfiRing		SfiRing;
typedef struct {
  guint        n_fields;
  GParamSpec **fields;
} SfiRecFields;


/* --- global --- */
void	sfi_init	(void);


/* --- boxed types --- */
typedef SfiRec*           (*SfiBoxedToRec)              (gpointer                  boxed);
typedef gpointer          (*SfiBoxedFromRec)            (SfiRec                   *rec);
typedef struct {
  const gchar    *name;
  SfiRecFields    fields;
  SfiBoxedToRec   to_rec;
  SfiBoxedFromRec from_rec;
} SfiBoxedRecordInfo;
GType                       sfi_boxed_make_record       (const SfiBoxedRecordInfo *info,
							 GBoxedCopyFunc            copy,
							 GBoxedFreeFunc            free);
const SfiBoxedRecordInfo*   sfi_boxed_get_record_info   (GType                     boxed_type);
typedef SfiSeq*           (*SfiBoxedToSeq)              (gpointer                  boxed);
typedef gpointer          (*SfiBoxedFromSeq)            (SfiSeq                   *seq);
typedef struct {
  const gchar    *name;
  GParamSpec     *element;
  SfiBoxedToSeq   to_seq;
  SfiBoxedFromSeq from_seq;
} SfiBoxedSequenceInfo;
GType                       sfi_boxed_make_sequence     (const SfiBoxedSequenceInfo *info,
							 GBoxedCopyFunc              copy,
							 GBoxedFreeFunc              free);
const SfiBoxedSequenceInfo* sfi_boxed_get_sequence_info (GType                       boxed_type);


/* --- FIXME: hacks! --- */
typedef struct _GslGlueProc SfiGlueProc;
void	sfi_set_error	(GError       **errorp,	// do nothing if *errorp is set already
			 GQuark         domain,
			 gint           code,
			 const gchar   *format,
			 ...) G_GNUC_PRINTF (4, 5);
gboolean sfi_choice_match_detailed (const gchar *choice_val1,
				    const gchar *choice_val2,
				    gboolean     l1_ge_l2);
gboolean sfi_choice_match (const gchar *choice_val1,
			   const gchar *choice_val2);
gchar*	sfi_strdup_canon (const gchar *identifier);

typedef struct {
  const gchar *name;
  guint        name_length;
  guint        index;
} SfiConstants;

guint	     sfi_constants_get_index	(guint		     n_consts,
					 const SfiConstants *rsorted_consts,
					 const gchar	    *constant);
const gchar* sfi_constants_get_name	(guint		     n_consts,
					 const SfiConstants *consts,
					 guint		     index);
gint	     sfi_constants_rcmp		(const gchar	    *canon_identifier1,
					 const gchar	    *canon_identifier2);

G_END_DECLS

#endif /* __SFI_TYPES_H__ */

/* vim:set ts=8 sts=2 sw=2: */
