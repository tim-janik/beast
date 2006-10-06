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

#include <birnet/birnetthread.h>
#include <sfi/glib-extra.h>

G_BEGIN_DECLS


/* --- Sfi typedefs --- */

/* 64 bit types - FIXME: assert in configure script that sizeof (long long) == 8 */
typedef long long               sfi_int64;
typedef unsigned long long      sfi_uint64;

#if (1) /* internal: do not make these available when compiling third party code */
#define int64                   sfi_int64
#define uint64                  sfi_uint64
#endif

typedef gboolean		SfiBool;
typedef gint			SfiInt;
#define	SFI_MAXINT		(+2147483647)
#define	SFI_MININT		(-SFI_MAXINT - 1)
typedef gint64			SfiNum;
#define	SFI_MAXNUM		((SfiNum) +9223372036854775807LL)
#define	SFI_MINNUM		(-SFI_MAXNUM - 1)
typedef gint64			SfiTime;
typedef SfiInt			SfiNote;
typedef gdouble			SfiReal;
#define SFI_MINREAL		(2.2250738585072014e-308)	/* IEEE754 double */
#define SFI_MAXREAL		(1.7976931348623157e+308)	/* IEEE754 double */
typedef const gchar*		SfiChoice;
typedef gchar*  		SfiString;                      /* convenience for code generators */
typedef struct _SfiBBlock	SfiBBlock;
typedef struct _SfiFBlock	SfiFBlock;
typedef struct _SfiSeq		SfiSeq;
typedef struct _SfiRec		SfiRec;
typedef GType /* pointer */	SfiProxy;
typedef struct {
  guint        n_fields;
  GParamSpec **fields;
} SfiRecFields;
typedef struct _SfiUStore	SfiUStore;
typedef struct _SfiUPool	SfiUPool;
typedef struct _SfiPPool	SfiPPool;

/* --- FIXME: hacks! --- */
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
const char*  sfi_category_concat        (const char         *prefix,
                                         const char         *trunk);

/* --- idl macro magic --- */
#define SFI_START_ARGS()     (
#define SFI_END_ARGS()       )
#define SFI_END_ARGS1(a)     a)
#define SFI_END_ARGS2(a,b)   a , b)
#define SFI_END_ARGS3(a,b,c) a , b , c)


G_END_DECLS

#endif /* __SFI_TYPES_H__ */

/* vim:set ts=8 sts=2 sw=2: */
