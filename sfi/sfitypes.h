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
typedef gchar*			SfiString;
typedef const gchar*		SfiChoice;
typedef struct _SfiFBlock	SfiFBlock;
typedef struct _SfiSeq		SfiSeq;
typedef struct _SfiRec		SfiRec;
typedef GType /* pointer */	SfiProxy;
typedef struct _SfiRing         SfiRing;


/* --- global --- */
void	sfi_init	(void);


/* FIXME: hacks! */
void	sfi_set_error	(GError       **errorp,	// do nothing if *errorp is set already
			 GQuark         domain,
			 gint           code,
			 const gchar   *format,
			 ...) G_GNUC_PRINTF (4, 5);
gchar*  sfi_note_to_string      (gint            note);
gint    sfi_note_from_string    (const gchar    *note_string,
				 GError        **errorp);
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
