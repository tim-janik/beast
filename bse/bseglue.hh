// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_GLUE_H__
#define __BSE_GLUE_H__

#include <bse/bsetype.hh>

SfiGlueContext*	bse_glue_context_intern	  (const gchar	*user);
GSource*        bse_glue_setup_dispatcher (SfiComPort   *port);

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

#endif /* __BSE_GLUE_H__ */

/* vim:set ts=8 sts=2 sw=2: */
