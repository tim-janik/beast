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
#define SFI_IS_PARAM_SPEC_BOOL(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), SFI_TYPE_PARAM_BOOL))
#define SFI_PARAM_SPEC_BOOL(pspec)	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), SFI_TYPE_PARAM_BOOL, SfiParamSpecBool))
#define SFI_TYPE_PARAM_INT		(G_TYPE_PARAM_INT)
#define SFI_IS_PARAM_SPEC_INT(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), SFI_TYPE_PARAM_INT))
#define SFI_PARAM_SPEC_INT(pspec)	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), SFI_TYPE_PARAM_INT, SfiParamSpecInt))
#define SFI_TYPE_PARAM_NUM		(G_TYPE_PARAM_INT64)
#define SFI_IS_PARAM_SPEC_NUM(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), SFI_TYPE_PARAM_NUM))
#define SFI_PARAM_SPEC_NUM(pspec)	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), SFI_TYPE_PARAM_NUM, SfiParamSpecNum))
#define SFI_TYPE_PARAM_REAL		(G_TYPE_PARAM_DOUBLE)
#define SFI_IS_PARAM_SPEC_REAL(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), SFI_TYPE_PARAM_REAL))
#define SFI_PARAM_SPEC_REAL(pspec)	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), SFI_TYPE_PARAM_REAL, SfiParamSpecReal))
#define SFI_TYPE_PARAM_STRING		(G_TYPE_PARAM_STRING)
#define SFI_IS_PARAM_SPEC_STRING(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), SFI_TYPE_PARAM_STRING))
#define SFI_PARAM_SPEC_STRING(pspec)	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), SFI_TYPE_PARAM_STRING, SfiParamSpecString))
#define SFI_TYPE_PARAM_CHOICE		(sfi__param_spec_types[0])
#define SFI_IS_PARAM_SPEC_CHOICE(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), SFI_TYPE_PARAM_CHOICE))
#define SFI_PARAM_SPEC_CHOICE(pspec)	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), SFI_TYPE_PARAM_CHOICE, SfiParamSpecChoice))
#define SFI_TYPE_PARAM_BBLOCK		(sfi__param_spec_types[1])
#define SFI_IS_PARAM_SPEC_BBLOCK(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), SFI_TYPE_PARAM_BBLOCK))
#define SFI_PARAM_SPEC_BBLOCK(pspec)	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), SFI_TYPE_PARAM_BBLOCK, SfiParamSpecBBlock))
#define SFI_TYPE_PARAM_FBLOCK		(sfi__param_spec_types[2])
#define SFI_IS_PARAM_SPEC_FBLOCK(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), SFI_TYPE_PARAM_FBLOCK))
#define SFI_PARAM_SPEC_FBLOCK(pspec)	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), SFI_TYPE_PARAM_FBLOCK, SfiParamSpecFBlock))
#define SFI_TYPE_PARAM_SEQ		(sfi__param_spec_types[3])
#define SFI_IS_PARAM_SPEC_SEQ(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), SFI_TYPE_PARAM_SEQ))
#define SFI_PARAM_SPEC_SEQ(pspec)	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), SFI_TYPE_PARAM_SEQ, SfiParamSpecSeq))
#define SFI_TYPE_PARAM_REC		(sfi__param_spec_types[4])
#define SFI_IS_PARAM_SPEC_REC(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), SFI_TYPE_PARAM_REC))
#define SFI_PARAM_SPEC_REC(pspec)	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), SFI_TYPE_PARAM_REC, SfiParamSpecRec))
#define SFI_TYPE_PARAM_PROXY		(sfi__param_spec_types[5])
#define SFI_IS_PARAM_SPEC_PROXY(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), SFI_TYPE_PARAM_PROXY))
#define SFI_PARAM_SPEC_PROXY(pspec)	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), SFI_TYPE_PARAM_PROXY, SfiParamSpecProxy))


/* --- Sfi param spec aliases --- */
typedef GParamSpecBoolean SfiParamSpecBool;
typedef GParamSpecInt     SfiParamSpecInt;
typedef GParamSpecInt64   SfiParamSpecNum;
typedef GParamSpecDouble  SfiParamSpecReal;
typedef GParamSpecString  SfiParamSpecString;


/* --- Sfi param specs --- */
typedef struct {
  guint              n_values;
  const GEnumValue  *values;
} SfiChoiceValues;     // auxillary
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


/* --- Sfi GParamSpec  constructors --- */
GParamSpec*    sfi_param_spec_bool	(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 SfiBool	 default_value,
					 const gchar	*hints);
GParamSpec*    sfi_param_spec_int	(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 SfiInt		 default_value,
					 SfiInt		 minimum_value,
					 SfiInt		 maximum_value,
					 SfiInt		 stepping,
					 const gchar	*hints);
GParamSpec*    sfi_param_spec_num	(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 SfiNum		 default_value,
					 SfiNum		 minimum_value,
					 SfiNum		 maximum_value,
					 SfiNum		 stepping,
					 const gchar	*hints);
GParamSpec*    sfi_param_spec_real	(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 SfiReal	 default_value,
					 SfiReal	 minimum_value,
					 SfiReal	 maximum_value,
					 SfiReal	 stepping,
					 const gchar	*hints);
GParamSpec*    sfi_param_spec_log_scale (const gchar    *name,
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
GParamSpec*	sfi_param_spec_string	(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 const gchar	*default_value,
					 const gchar	*hints);
GParamSpec*	sfi_param_spec_choice	(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 const gchar	*default_value,
					 SfiChoiceValues static_const_evalues,
					 const gchar	*hints);
GParamSpec*	sfi_param_spec_bblock	(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 const gchar	*hints);
GParamSpec*	sfi_param_spec_fblock	(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 const gchar	*hints);
GParamSpec*	sfi_param_spec_seq	(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 GParamSpec     *element_spec,
					 const gchar	*hints);
GParamSpec*	sfi_param_spec_rec	(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 SfiRecFields    static_const_fields,
					 const gchar	*hints);
GParamSpec*	sfi_param_spec_proxy	(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 const gchar	*hints);


/* --- conversion --- */
GParamSpec* 	sfi_param_spec_choice_from_enum	 (GParamSpec *enum_pspec);
GParamSpec* 	sfi_param_spec_proxy_from_object (GParamSpec *object_pspec);
GParamSpec*     sfi_param_spec_to_serializable   (GParamSpec *pspec);


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
/* readable and writable */
#define	SFI_PARAM_DEFAULT	  SFI_PARAM_READWRITE SFI_PARAM_GUI SFI_PARAM_STORAGE
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
  SFI_SCAT_CHOICE	= 'c',
  SFI_SCAT_BBLOCK	= 'B',
  SFI_SCAT_FBLOCK	= 'F',
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
#define     SFI_IS_PARAM_SPEC_NOTE	 SFI_IS_PARAM_SPEC_INT
GParamSpec* sfi_param_spec_note		(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 SfiInt          default_value,
					 const gchar    *hints);
#define     SFI_IS_PARAM_SPEC_TIME	 SFI_IS_PARAM_SPEC_TIME
GParamSpec* sfi_param_spec_time		(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 const gchar    *hints);


/* --- pspec accessors --- */
void		sfi_pspec_set_group		(GParamSpec	*pspec,
						 const gchar	*group);
const gchar*	sfi_pspec_get_group		(GParamSpec	*pspec);
void		sfi_pspec_set_hints		(GParamSpec	*pspec,
						 const gchar	*hints);
void		sfi_pspec_set_static_hints	(GParamSpec	*pspec,
						 const gchar	*hints);
gboolean	sfi_pspec_test_hint		(GParamSpec	*pspec,
						 const gchar	*hint);
const gchar*	sfi_pspec_get_hints		(GParamSpec	*pspec);
SfiBool		sfi_pspec_get_bool_default	(GParamSpec	*pspec);
SfiInt		sfi_pspec_get_int_default	(GParamSpec	*pspec);
void		sfi_pspec_get_int_range		(GParamSpec	*pspec,
						 SfiInt         *minimum_value,
						 SfiInt         *maximum_value,
						 SfiInt         *stepping);
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
void		sfi_pspec_get_log_scale		(GParamSpec	*pspec,
						 SfiReal        *center,
						 SfiReal        *base,
						 SfiReal        *n_steps);
const gchar*	sfi_pspec_get_string_default	(GParamSpec	*pspec);
const gchar*	sfi_pspec_get_choice_default	(GParamSpec	*pspec);
SfiChoiceValues	sfi_pspec_get_choice_values	(GParamSpec	*pspec);
GEnumValue*	sfi_pspec_get_choice_value_list	(GParamSpec	*pspec);
GParamSpec*	sfi_pspec_get_seq_element	(GParamSpec	*pspec);
SfiRecFields	sfi_pspec_get_rec_fields	(GParamSpec	*pspec);
GParamSpec*	sfi_pspec_get_rec_field		(GParamSpec	*pspec,
						 const gchar	*field_name);


/* --- internal --- */
void		_sfi_init_params	(void);
extern GType*	 sfi__param_spec_types;

G_END_DECLS

#endif /* __SFI_PARAMS_H__ */

/* vim:set ts=8 sts=2 sw=2: */
