/* BSE - Better Sound Engine
 * Copyright (C) 2002, 2003 Tim Janik
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
#ifndef __BSE_GLUE_H__
#define __BSE_GLUE_H__

#include <bse/bsetype.hh>

G_BEGIN_DECLS

/* FIXME: creation of a new context source should be done
 * by a janitor constructor
 */

SfiGlueContext*	bse_glue_context_intern	 (const gchar	*user);

/* Construct a new #SfiRec from a boxed value. */
typedef SfiRec*	(*BseGlueBoxedToRec)	 (gpointer	    boxed);
/* Construct a new #SfiSeq from a boxed value. */
typedef SfiSeq*	(*BseGlueBoxedToSeq)	 (gpointer	    boxed);

GType			bse_glue_pspec_get_original_enum (GParamSpec *pspec);
guint			bse_glue_enum_index	 (GType		    enum_type,
						  gint		    enum_value);
GValue*			bse_glue_boxed_to_value	 (GType		    boxed_type,
						  gpointer	    boxed);

/* convert value sto/from SFI serializable types */
GValue*	bse_value_to_sfi	(const GValue	*value);
GValue*	bse_value_from_sfi	(const GValue	*value,
				 GParamSpec	*pspec);

G_END_DECLS

#endif /* __BSE_GLUE_H__ */

/* vim:set ts=8 sts=2 sw=2: */
