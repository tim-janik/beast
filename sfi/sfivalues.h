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
#ifndef __SFI_VALUES_H__
#define __SFI_VALUES_H__

#include <sfi/sfitypes.h>

G_BEGIN_DECLS


/* --- Sfi value type aliases --- */
#define	SFI_TYPE_BOOL		G_TYPE_BOOLEAN
#define	SFI_TYPE_INT		G_TYPE_INT
#define	SFI_TYPE_NUM		G_TYPE_INT64
#define	SFI_TYPE_REAL		G_TYPE_DOUBLE
#define	SFI_TYPE_STRING		G_TYPE_STRING
#define SFI_TYPE_PSPEC		G_TYPE_PARAM


/* --- Sfi value types --- */
#define SFI_TYPE_CHOICE		(sfi__value_types[0])
#define SFI_TYPE_BBLOCK		(sfi__value_types[1])
#define SFI_TYPE_FBLOCK		(sfi__value_types[2])
#define SFI_TYPE_SEQ		(sfi__value_types[3])
#define SFI_TYPE_REC		(sfi__value_types[4])
#define SFI_TYPE_PROXY		(sfi__value_types[5])


/* --- Sfi value macros --- */
#define SFI_IS_VALUE(value)		(sfi_check_value (value))
#define SFI_VALUE_HOLDS_BOOL(value)	(G_TYPE_CHECK_VALUE_TYPE ((value), SFI_TYPE_BOOL))
#define SFI_VALUE_HOLDS_INT(value)	(G_TYPE_CHECK_VALUE_TYPE ((value), SFI_TYPE_INT))
#define SFI_VALUE_HOLDS_NUM(value)	(G_TYPE_CHECK_VALUE_TYPE ((value), SFI_TYPE_NUM))
#define SFI_VALUE_HOLDS_REAL(value)	(G_TYPE_CHECK_VALUE_TYPE ((value), SFI_TYPE_REAL))
#define SFI_VALUE_HOLDS_STRING(value)	(G_TYPE_CHECK_VALUE_TYPE ((value), SFI_TYPE_STRING))
#define SFI_VALUE_HOLDS_CHOICE(value)	(G_TYPE_CHECK_VALUE_TYPE ((value), SFI_TYPE_CHOICE))
#define SFI_VALUE_HOLDS_BBLOCK(value)	(G_TYPE_CHECK_VALUE_TYPE ((value), SFI_TYPE_BBLOCK))
#define SFI_VALUE_HOLDS_FBLOCK(value)	(G_TYPE_CHECK_VALUE_TYPE ((value), SFI_TYPE_FBLOCK))
#define SFI_VALUE_HOLDS_PSPEC(value)	(G_TYPE_CHECK_VALUE_TYPE ((value), SFI_TYPE_PSPEC))
#define SFI_VALUE_HOLDS_SEQ(value)	(G_TYPE_CHECK_VALUE_TYPE ((value), SFI_TYPE_SEQ))
#define SFI_VALUE_HOLDS_REC(value)	(G_TYPE_CHECK_VALUE_TYPE ((value), SFI_TYPE_REC))
#define SFI_VALUE_HOLDS_PROXY(value)	(G_TYPE_CHECK_VALUE_TYPE ((value), SFI_TYPE_PROXY))


/* --- Sfi value accessor aliases --- */
#define	sfi_value_get_bool		g_value_get_boolean
#define	sfi_value_set_bool		g_value_set_boolean
#define	sfi_value_get_int		g_value_get_int
#define	sfi_value_set_int		g_value_set_int
#define	sfi_value_get_num		g_value_get_int64
#define	sfi_value_set_num		g_value_set_int64
#define	sfi_value_get_real		g_value_get_double
#define	sfi_value_set_real		g_value_set_double
#define	sfi_value_get_string		g_value_get_string
#define	sfi_value_set_string		g_value_set_string
#define	sfi_value_dup_string		g_value_dup_string
#define	sfi_value_set_static_string	g_value_set_static_string
#define	sfi_value_take_string		g_value_set_string_take_ownership


/* --- Sfi value accessors --- */
gchar*	    sfi_value_get_choice	(const GValue	*value);
void	    sfi_value_set_choice	(GValue		*value,
					 const gchar	*choice_value);
SfiBBlock*  sfi_value_get_bblock	(const GValue	*value);
SfiBBlock*  sfi_value_dup_bblock	(const GValue	*value);
void	    sfi_value_set_bblock	(GValue		*value,
					 SfiBBlock	*bblock);
void	    sfi_value_take_bblock	(GValue		*value,
					 SfiBBlock	*bblock);
SfiFBlock*  sfi_value_get_fblock	(const GValue	*value);
SfiFBlock*  sfi_value_dup_fblock	(const GValue	*value);
void	    sfi_value_set_fblock	(GValue		*value,
					 SfiFBlock	*fblock);
void	    sfi_value_take_fblock	(GValue		*value,
					 SfiFBlock	*fblock);
GParamSpec* sfi_value_get_pspec		(const GValue	*value);
GParamSpec* sfi_value_dup_pspec		(const GValue	*value);
void	    sfi_value_set_pspec		(GValue		*value,
					 GParamSpec	*pspec);
void	    sfi_value_take_pspec	(GValue		*value,
					 GParamSpec	*pspec);
SfiSeq*	    sfi_value_get_seq		(const GValue	*value);
void	    sfi_value_set_seq		(GValue		*value,
					 SfiSeq		*seq);
void	    sfi_value_take_seq		(GValue		*value,
					 SfiSeq		*seq);
SfiRec*	    sfi_value_get_rec		(const GValue	*value);
SfiRec*     sfi_value_dup_rec           (const GValue   *value);
void	    sfi_value_set_rec		(GValue		*value,
					 SfiRec         *rec);
void	    sfi_value_take_rec		(GValue         *value,
					 SfiRec         *rec);
SfiProxy    sfi_value_get_proxy		(const GValue	*value);
void	    sfi_value_set_proxy		(GValue		*value,
					 SfiProxy	 proxy);
void	    sfi_value_copy_deep		(const GValue	*src_value,
					 GValue		*dest_value);
#define	    sfi_value_copy_shallow	g_value_copy


/* --- Sfi value constructors --- */
GValue*	sfi_value_empty		(void);
GValue*	sfi_value_clone_deep	(const GValue	*value);
GValue*	sfi_value_clone_shallow	(const GValue	*value);
GValue*	sfi_value_bool		(SfiBool	 vbool);
GValue*	sfi_value_int		(SfiInt		 vint);
GValue*	sfi_value_num		(SfiNum		 vnum);
GValue*	sfi_value_real		(SfiReal	 vreal);
GValue*	sfi_value_string	(const gchar	*vstring);
GValue*	sfi_value_lstring	(const gchar	*vstring,
				 guint		 length);
GValue*	sfi_value_choice	(const gchar	*vchoice);
GValue*	sfi_value_lchoice	(const gchar	*vchoice,
				 guint           length);
GValue*	sfi_value_choice_enum	(const GValue	*enum_value);
GValue*	sfi_value_choice_genum	(gint            enum_value,
                                 GType           enum_type);
GValue*	sfi_value_bblock	(SfiBBlock	*vfblock);
GValue*	sfi_value_fblock	(SfiFBlock	*vfblock);
GValue*	sfi_value_pspec		(GParamSpec	*pspec);
GValue*	sfi_value_seq		(SfiSeq		*vseq);
GValue*	sfi_value_new_take_seq	(SfiSeq		*vseq);
GValue*	sfi_value_rec		(SfiRec		*vrec);
GValue*	sfi_value_new_take_rec	(SfiRec		*vrec);
GValue*	sfi_value_proxy		(SfiProxy	 vproxy);
void	sfi_value_free		(GValue		*value);


/* --- convenience aliases --- */
#define SFI_VALUE_HOLDS_NOTE(value)	SFI_VALUE_HOLDS_INT(value)
#define	sfi_value_get_note		sfi_value_get_int
#define	sfi_value_set_note		sfi_value_set_int
#define	sfi_value_note			sfi_value_int
#define SFI_VALUE_HOLDS_TIME(value)	SFI_VALUE_HOLDS_NUM(value)
#define	sfi_value_get_time		sfi_value_get_num
#define	sfi_value_set_time		sfi_value_set_num
#define	sfi_value_time			sfi_value_num


/* --- transformation --- */
void         sfi_value_choice2enum        (const GValue *choice_value,
                                           GValue       *enum_value,
                                           GParamSpec   *fallback_param);
void         sfi_value_choice2enum_simple (const GValue *choice_value,
                                           GValue       *enum_value);
void         sfi_value_enum2choice        (const GValue *enum_value,
                                           GValue       *choice_value);
gint         sfi_choice2enum              (const gchar  *choice_value,
                                           GType         enum_type);
const gchar* sfi_enum2choice              (gint          enum_value,
                                           GType         enum_type);
gint         sfi_value_get_enum_auto      (GType         enum_type,
                                           const GValue *value);
void         sfi_value_set_enum_auto      (GType         enum_type,
                                           GValue       *value,
                                           gint          enum_value);
/* transform functions to work around glib bugs */
gboolean     sfi_value_transform          (const GValue *src_value,
                                           GValue       *dest_value);
gboolean     sfi_value_type_compatible    (GType         src_type,
                                           GType         dest_type);
gboolean     sfi_value_type_transformable (GType         src_type,
                                           GType         dest_type);


/* --- internal --- */
void	     _sfi_init_values	(void);
extern GType *sfi__value_types;
gboolean      sfi_check_value	(const GValue	*value);

G_END_DECLS

#endif /* __SFI_VALUES_H__ */

/* vim:set ts=8 sts=2 sw=2: */
