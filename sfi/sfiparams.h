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
#ifndef __SFI_PARAMS_H__
#define __SFI_PARAMS_H__

#include <sfi/sfivalues.h>
#include <sfi/sfiprimitives.h>

G_BEGIN_DECLS


/* --- Sfi param spec macros --- */
#define SFI_TYPE_PARAM_BOOL		(G_TYPE_PARAM_BOOLEAN)
#define SFI_IS_PSPEC_BOOL(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), SFI_TYPE_PARAM_BOOL))
#define SFI_PSPEC_BOOL(pspec)		(G_TYPE_CHECK_INSTANCE_CAST ((pspec), SFI_TYPE_PARAM_BOOL, SfiParamSpecBool))
#define SFI_TYPE_PARAM_INT		(G_TYPE_PARAM_INT)
#define SFI_IS_PSPEC_INT(pspec)		(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), SFI_TYPE_PARAM_INT))
#define SFI_PSPEC_INT(pspec)		(G_TYPE_CHECK_INSTANCE_CAST ((pspec), SFI_TYPE_PARAM_INT, SfiParamSpecInt))
#define SFI_TYPE_PARAM_NUM		(G_TYPE_PARAM_INT64)
#define SFI_IS_PSPEC_NUM(pspec)		(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), SFI_TYPE_PARAM_NUM))
#define SFI_PSPEC_NUM(pspec)		(G_TYPE_CHECK_INSTANCE_CAST ((pspec), SFI_TYPE_PARAM_NUM, SfiParamSpecNum))
#define SFI_TYPE_PARAM_REAL		(G_TYPE_PARAM_DOUBLE)
#define SFI_IS_PSPEC_REAL(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), SFI_TYPE_PARAM_REAL))
#define SFI_PSPEC_REAL(pspec)		(G_TYPE_CHECK_INSTANCE_CAST ((pspec), SFI_TYPE_PARAM_REAL, SfiParamSpecReal))
#define SFI_TYPE_PARAM_STRING		(G_TYPE_PARAM_STRING)
#define SFI_IS_PSPEC_STRING(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), SFI_TYPE_PARAM_STRING))
#define SFI_PSPEC_STRING(pspec)		(G_TYPE_CHECK_INSTANCE_CAST ((pspec), SFI_TYPE_PARAM_STRING, SfiParamSpecString))
#define SFI_TYPE_PARAM_CHOICE		(sfi__param_spec_types[0])
#define SFI_IS_PSPEC_CHOICE(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), SFI_TYPE_PARAM_CHOICE))
#define SFI_PSPEC_CHOICE(pspec)		(G_TYPE_CHECK_INSTANCE_CAST ((pspec), SFI_TYPE_PARAM_CHOICE, SfiParamSpecChoice))
#define SFI_TYPE_PARAM_BBLOCK		(sfi__param_spec_types[1])
#define SFI_IS_PSPEC_BBLOCK(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), SFI_TYPE_PARAM_BBLOCK))
#define SFI_PSPEC_BBLOCK(pspec)		(G_TYPE_CHECK_INSTANCE_CAST ((pspec), SFI_TYPE_PARAM_BBLOCK, SfiParamSpecBBlock))
#define SFI_TYPE_PARAM_FBLOCK		(sfi__param_spec_types[2])
#define SFI_IS_PSPEC_FBLOCK(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), SFI_TYPE_PARAM_FBLOCK))
#define SFI_PSPEC_FBLOCK(pspec)		(G_TYPE_CHECK_INSTANCE_CAST ((pspec), SFI_TYPE_PARAM_FBLOCK, SfiParamSpecFBlock))
#define SFI_TYPE_PARAM_PSPEC		(G_TYPE_PARAM_PARAM)
#define SFI_IS_PSPEC_PSPEC(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), SFI_TYPE_PARAM_PSPEC))
#define SFI_PSPEC_PSPEC(pspec)		(G_TYPE_CHECK_INSTANCE_CAST ((pspec), SFI_TYPE_PARAM_PSPEC, SfiParamSpecPSpec))
#define SFI_TYPE_PARAM_SEQ		(sfi__param_spec_types[3])
#define SFI_IS_PSPEC_SEQ(pspec)		(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), SFI_TYPE_PARAM_SEQ))
#define SFI_PSPEC_SEQ(pspec)		(G_TYPE_CHECK_INSTANCE_CAST ((pspec), SFI_TYPE_PARAM_SEQ, SfiParamSpecSeq))
#define SFI_TYPE_PARAM_REC		(sfi__param_spec_types[4])
#define SFI_IS_PSPEC_REC(pspec)		(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), SFI_TYPE_PARAM_REC))
#define SFI_PSPEC_REC(pspec)		(G_TYPE_CHECK_INSTANCE_CAST ((pspec), SFI_TYPE_PARAM_REC, SfiParamSpecRec))
#define SFI_TYPE_PARAM_PROXY		(sfi__param_spec_types[5])
#define SFI_IS_PSPEC_PROXY(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), SFI_TYPE_PARAM_PROXY))
#define SFI_PSPEC_PROXY(pspec)		(G_TYPE_CHECK_INSTANCE_CAST ((pspec), SFI_TYPE_PARAM_PROXY, SfiParamSpecProxy))
#define SFI_TYPE_PARAM_NOTE		(sfi__param_spec_types[6])
#define SFI_IS_PSPEC_NOTE(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), SFI_TYPE_PARAM_NOTE))
#define SFI_PSPEC_NOTE(pspec)		(G_TYPE_CHECK_INSTANCE_CAST ((pspec), SFI_TYPE_PARAM_NOTE, SfiParamSpecNote))


/* --- Sfi param spec aliases --- */
typedef GParamSpecBoolean SfiParamSpecBool;
typedef GParamSpecInt     SfiParamSpecInt;
typedef GParamSpecInt64   SfiParamSpecNum;
typedef GParamSpecDouble  SfiParamSpecReal;
typedef GParamSpecString  SfiParamSpecString;
typedef GParamSpecParam   SfiParamSpecPSpec;


/* --- Sfi param specs --- */
typedef struct {
  gchar *choice_name;
  gchar *choice_blurb;
} SfiChoiceValue;	// auxillary
typedef struct {
  guint                 n_values;
  const SfiChoiceValue *values;
} SfiChoiceValues;	// auxillary
typedef struct {
  GParamSpecString   pspec;
  SfiChoiceValues    cvalues;
} SfiParamSpecChoice;
typedef struct {
  GParamSpecBoxed    pspec;
} SfiParamSpecBBlock;
typedef struct {
  GParamSpecBoxed    pspec;
} SfiParamSpecFBlock;
typedef struct {
  GParamSpecBoxed    pspec;
  GParamSpec        *element;
} SfiParamSpecSeq;
typedef struct {
  GParamSpecBoxed    pspec;
  SfiRecFields       fields;
} SfiParamSpecRec;
typedef struct {
  GParamSpecPointer  pspec;
} SfiParamSpecProxy;
typedef struct {
  GParamSpecInt      pspec;
  gboolean           allow_void;
} SfiParamSpecNote;


/* --- Sfi GParamSpec  constructors --- */
GParamSpec*    sfi_pspec_bool		(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 SfiBool	 default_value,
					 const gchar	*hints);
GParamSpec*    sfi_pspec_int		(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 SfiInt		 default_value,
					 SfiInt		 minimum_value,
					 SfiInt		 maximum_value,
					 SfiInt		 stepping,
					 const gchar	*hints);
GParamSpec*    sfi_pspec_num		(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 SfiNum		 default_value,
					 SfiNum		 minimum_value,
					 SfiNum		 maximum_value,
					 SfiNum		 stepping,
					 const gchar	*hints);
GParamSpec*    sfi_pspec_real		(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 SfiReal	 default_value,
					 SfiReal	 minimum_value,
					 SfiReal	 maximum_value,
					 SfiReal	 stepping,
					 const gchar	*hints);
GParamSpec*    sfi_pspec_log_scale	(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 SfiReal	 default_value,
					 SfiReal	 minimum_value,
					 SfiReal	 maximum_value,
					 SfiReal	 stepping,
					 SfiReal	 center,
					 SfiReal	 base,
					 SfiReal	 n_steps,
					 const gchar	*hints);
GParamSpec*	sfi_pspec_string	(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 const gchar	*default_value,
					 const gchar	*hints);
GParamSpec*	sfi_pspec_choice	(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 const gchar	*default_value,
					 SfiChoiceValues static_const_evalues,
					 const gchar	*hints);
GParamSpec*	sfi_pspec_bblock	(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 const gchar	*hints);
GParamSpec*	sfi_pspec_fblock	(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 const gchar	*hints);
GParamSpec*	sfi_pspec_pspec		(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 const gchar	*hints);
GParamSpec*	sfi_pspec_seq		(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 GParamSpec     *element_spec,
					 const gchar	*hints);
GParamSpec*	sfi_pspec_rec		(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 SfiRecFields    static_const_fields,
					 const gchar	*hints);
GParamSpec*	sfi_pspec_proxy		(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 const gchar	*hints);


/* --- pspec wrappers --- */
#define	sfi_pspec_ref			g_param_spec_ref
#define	sfi_pspec_unref			g_param_spec_unref
#define	sfi_pspec_sink			g_param_spec_sink
#define	sfi_pspec_get_qdata		g_param_spec_get_qdata
#define	sfi_pspec_set_qdata		g_param_spec_set_qdata
#define	sfi_pspec_set_qdata_full	g_param_spec_set_qdata_full
#define	sfi_pspec_steal_qdata		g_param_spec_steal_qdata
#define	sfi_pspec_get_name		g_param_spec_get_name
#define	sfi_pspec_get_nick		g_param_spec_get_nick
#define	sfi_pspec_get_blurb		g_param_spec_get_blurb


/* --- conversion --- */
GParamSpec* 	 sfi_pspec_choice_from_enum  (GParamSpec *enum_pspec);
GParamSpec* 	 sfi_pspec_proxy_from_object (GParamSpec *object_pspec);
GParamSpec*      sfi_pspec_to_serializable   (GParamSpec *pspec);


/* --- Sfi param hints --- */
#define	SFI_PARAM_READABLE	  "r:"
#define	SFI_PARAM_WRITABLE	  "w:"
#define	SFI_PARAM_READWRITE	  SFI_PARAM_READABLE SFI_PARAM_WRITABLE
#define	SFI_PARAM_LAX_VALIDATION  "lxv:"
#define	SFI_PARAM_SERVE_GUI	  "gui:"	/* GUI representation */
#define	SFI_PARAM_SERVE_STORAGE	  "storage:"	/* gets serialized */
/* storage flags */
#define	SFI_PARAM_FORCE_DIRTY	  "dirty:"
/* GUI hints */
#define	SFI_PARAM_HINT_RDONLY	  "rdonly:"
#define	SFI_PARAM_HINT_RADIO	  "radio:"
#define	SFI_PARAM_HINT_DIAL	  "dial:"
#define	SFI_PARAM_HINT_SCALE	  "scale:"
#define	SFI_PARAM_HINT_LOG_SCALE  "log-scale:"
/* readable and writable */
#define	SFI_PARAM_DEFAULT	  SFI_PARAM_READWRITE SFI_PARAM_SERVE_GUI SFI_PARAM_SERVE_STORAGE
#define	SFI_PARAM_GUI		  SFI_PARAM_READWRITE SFI_PARAM_SERVE_GUI
#define	SFI_PARAM_STORAGE	  SFI_PARAM_READWRITE SFI_PARAM_SERVE_STORAGE
/* readable and for non-GUI writable */
#define SFI_PARAM_DEFAULT_RDONLY  SFI_PARAM_DEFAULT SFI_PARAM_HINT_RDONLY
#define	SFI_PARAM_GUI_RDONLY	  SFI_PARAM_GUI SFI_PARAM_HINT_RDONLY


/* --- serializable categories --- */
typedef enum	/*< skip >*/
{
  SFI_SCAT_INVAL	= 0,
  SFI_SCAT_BOOL		= 'b',
  SFI_SCAT_INT		= 'i',
  SFI_SCAT_NUM		= 'n',
  SFI_SCAT_REAL		= 'r',
  SFI_SCAT_STRING	= 's',
  SFI_SCAT_CHOICE	= 'C',
  SFI_SCAT_BBLOCK	= 'B',
  SFI_SCAT_FBLOCK	= 'F',
  SFI_SCAT_PSPEC	= 'P',
  SFI_SCAT_SEQ		= 'Q',
  SFI_SCAT_REC		= 'R',
  SFI_SCAT_PROXY	= 'p',
  SFI_SCAT_TYPE_MASK	= 0x00ff,
  SFI_SCAT_NOTE		= 0x0100 | SFI_SCAT_INT,
  SFI_SCAT_TIME		= 0x0200 | SFI_SCAT_NUM,
} SfiSCategory;

SfiSCategory	sfi_categorize_type	(GType		 value_type);
SfiSCategory	sfi_categorize_pspec	(GParamSpec	*pspec);
GType		sfi_category_type	(SfiSCategory	 pspec_cat);
GType		sfi_category_param_type	(SfiSCategory	 pspec_cat);


/* --- convenience aliases --- */
GParamSpec* sfi_pspec_note		(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 SfiInt          default_value,
					 SfiInt		 min_note,
					 SfiInt		 max_note,
					 gboolean	 allow_void,
					 const gchar    *hints);
#define     SFI_IS_PSPEC_TIME		 SFI_IS_PSPEC_TIME
GParamSpec* sfi_pspec_time		(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 const gchar    *hints);


/* --- pspec accessors --- */
void		sfi_pspec_set_group		(GParamSpec	*pspec,
						 const gchar	*group);
const gchar*	sfi_pspec_get_group		(GParamSpec	*pspec);
void		sfi_pspec_set_owner		(GParamSpec	*pspec,
						 const gchar	*owner);
const gchar*	sfi_pspec_get_owner		(GParamSpec	*pspec);
void		sfi_pspec_set_hints		(GParamSpec	*pspec,
						 const gchar	*hints);
void		sfi_pspec_add_hint		(GParamSpec	*pspec,
						 const gchar	*hint);
void		sfi_pspec_remove_hint		(GParamSpec	*pspec,
						 const gchar	*hint);
gboolean	sfi_pspec_test_hint		(GParamSpec	*pspec,
						 const gchar	*hint);
gboolean	sfi_pspec_test_all_hints	(GParamSpec	*pspec,
						 const gchar	*hints);
const gchar*	sfi_pspec_get_hints		(GParamSpec	*pspec);
SfiBool		sfi_pspec_get_bool_default	(GParamSpec	*pspec);
SfiInt		sfi_pspec_get_int_default	(GParamSpec	*pspec);
void		sfi_pspec_get_int_range		(GParamSpec	*pspec,
						 SfiInt         *minimum_value,
						 SfiInt         *maximum_value,
						 SfiInt         *stepping);
gboolean	sfi_pspec_allows_void_note	(GParamSpec	*pspec);
#define		sfi_pspec_get_note_default	 sfi_pspec_get_int_default
#define		sfi_pspec_get_note_range	 sfi_pspec_get_int_range
SfiNum		sfi_pspec_get_num_default	(GParamSpec	*pspec);
void		sfi_pspec_get_num_range		(GParamSpec	*pspec,
						 SfiNum         *minimum_value,
						 SfiNum         *maximum_value,
						 SfiNum         *stepping);
SfiReal		sfi_pspec_get_real_default	(GParamSpec	*pspec);
void		sfi_pspec_get_real_range	(GParamSpec	*pspec,
						 SfiReal        *minimum_value,
						 SfiReal        *maximum_value,
						 SfiReal        *stepping);
void		sfi_pspec_set_log_scale		(GParamSpec	*pspec,
						 SfiReal         center,
						 SfiReal         base,
						 SfiReal         n_steps);
gboolean	sfi_pspec_get_log_scale		(GParamSpec	*pspec,
						 SfiReal        *center,
						 SfiReal        *base,
						 SfiReal        *n_steps);
const gchar*	sfi_pspec_get_string_default	(GParamSpec	*pspec);
const gchar*	sfi_pspec_get_choice_default	(GParamSpec	*pspec);
SfiChoiceValues	sfi_pspec_get_choice_values	(GParamSpec	*pspec);
GParamSpec*	sfi_pspec_get_seq_element	(GParamSpec	*pspec);
SfiRecFields	sfi_pspec_get_rec_fields	(GParamSpec	*pspec);
GParamSpec*	sfi_pspec_get_rec_field		(GParamSpec	*pspec,
						 const gchar	*field_name);


/* --- internal --- */
void		_sfi_init_params	(void);
extern GType*	 sfi__param_spec_types;
SfiRec*		sfi_pspec_to_rec	(GParamSpec	*pspec);
GParamSpec*	sfi_pspec_from_rec	(SfiRec		*prec);


G_END_DECLS

#endif /* __SFI_PARAMS_H__ */

/* vim:set ts=8 sts=2 sw=2: */
