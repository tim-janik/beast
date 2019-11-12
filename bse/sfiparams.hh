// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __SFI_PARAMS_H__
#define __SFI_PARAMS_H__

#include <bse/sfivalues.hh>
#include <bse/sfiprimitives.hh>


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
struct SfiChoiceValue {  // auxillary
  const gchar *choice_ident;
  const gchar *choice_label;
  const gchar *choice_blurb;
  SfiChoiceValue (const char *ident = NULL, const char *label = NULL, const char *blurb = NULL) :
    choice_ident (ident), choice_label (label), choice_blurb (blurb)
  {}
};
struct SfiChoiceValues { // auxillary
  guint                 n_values;
  const SfiChoiceValue *values;
  SfiChoiceValues (uint nv = 0, const SfiChoiceValue *vl = NULL) :
    n_values (nv), values (vl)
  {}
};
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
GParamSpec*	sfi_pspec_enum_choice	(const gchar    *name,
					 const gchar    *nick,
					 const gchar    *blurb,
					 const gchar	*default_value,
					 const std::string &enum_typename,
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
GParamSpec*	sfi_pspec_rec_generic	(const gchar    *name,
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
typedef SfiChoiceValues (*SfiChoiceValueGetter)     (GType                 enum_type);
GParamSpec*  sfi_pspec_to_serializable              (GParamSpec           *pspec);
GParamSpec*  sfi_pspec_choice_from_enum             (GParamSpec           *enum_pspec);
const char*  sfi_pspec_get_enum_typename            (GParamSpec           *pspec);
void         sfi_enum_type_set_choice_value_getter  (GType                 gtype,
                                                     SfiChoiceValueGetter  cvgetter);
void         sfi_boxed_type_set_rec_fields          (GType                 boxed_type,
                                                     const SfiRecFields    static_const_fields);
SfiRecFields sfi_boxed_type_get_rec_fields          (GType                 boxed_type);
void         sfi_boxed_type_set_seq_element         (GType                 boxed_type,
                                                     GParamSpec           *element);
GParamSpec*  sfi_boxed_type_get_seq_element         (GType                 boxed_type);


/* --- Sfi PSpec Options --- */
/* provide common option combinations: */
#define	SFI_PARAM_READWRITE       ":r:w:"
#define	SFI_PARAM_STORAGE	  ":r:w:S:"
#define	SFI_PARAM_STANDARD        ":r:w:S:G:"
#define SFI_PARAM_STANDARD_RDONLY ":r:w:S:G:ro:"
#define	SFI_PARAM_GUI		  ":r:w:G:"
#define	SFI_PARAM_GUI_RDONLY	  ":r:w:G:ro:"
#define	SFI_PARAM_GUI_READABLE	  ":r:G:"


/* --- serializable categories --- */
// Note, enum SfiSCategory is defined in bse/glib-extra.hh to be shared with sfidl
// typedef enum	{ ... } SfiSCategory;

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
GParamSpec*	sfi_pspec_set_group		(GParamSpec	*pspec,
						 const gchar	*group);
const gchar*	sfi_pspec_get_group		(GParamSpec	*pspec);
void		sfi_pspec_set_owner		(GParamSpec	*pspec,
						 const gchar	*owner);
const gchar*	sfi_pspec_get_owner		(GParamSpec	*pspec);
#define sfi_pspec_set_options(pspec, opts)      g_param_spec_set_options (pspec, opts)
#define sfi_pspec_add_option(pspec, opt, val)   g_param_spec_add_option (pspec, opt, val)
#define sfi_pspec_check_option(pspec, opt)      g_param_spec_check_option (pspec, opt)
#define sfi_pspec_provides_options(pspec, opts) g_param_spec_provides_options (pspec, opts)
#define sfi_pspec_get_options(pspec)            g_param_spec_get_options (pspec)
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
#define sfi_pspec_set_log_scale(p, c, b, n)     g_param_spec_set_log_scale (p, c, b, n)
#define sfi_pspec_get_log_scale(p, c, b, n)     g_param_spec_get_log_scale (p, c, b, n)
const gchar*	sfi_pspec_get_string_default	(GParamSpec	*pspec);
const gchar*	sfi_pspec_get_choice_default	(GParamSpec	*pspec);
SfiChoiceValues	sfi_pspec_get_choice_values	(GParamSpec	*pspec);
guint64         sfi_pspec_get_choice_hash       (GParamSpec     *pspec);
GParamSpec*	sfi_pspec_get_seq_element	(GParamSpec	*pspec);
SfiRecFields	sfi_pspec_get_rec_fields	(GParamSpec	*pspec);
GParamSpec*	sfi_pspec_get_rec_field		(GParamSpec	*pspec,
						 const gchar	*field_name);


/* --- internal --- */
void		_sfi_init_params	(void);
extern GType*	 sfi__param_spec_types;
SfiRec*		sfi_pspec_to_rec	(GParamSpec	*pspec);
GParamSpec*	sfi_pspec_from_rec	(SfiRec		*prec);

namespace Bse { // bsecore

SfiChoiceValues choice_values_from_enum (const String &enumname);

template<class EnumType> SfiChoiceValues
choice_values_from_enum ()
{
  return choice_values_from_enum (Aida::string_demangle_cxx (typeid (EnumType).name()));
}

GParamSpec*              pspec_from_key_value_list            (const std::string &name, const Aida::StringVector &introspection);

SfiChoiceValues          introspection_enum_to_choice_values  (const Aida::StringVector &introspection, const String &enumname);
GParamSpec*              introspection_field_to_param_spec    (const std::string &name, const Aida::StringVector &introspection, const String &subname);
Aida::StringVector       introspection_list_field_names       (const Aida::StringVector &introspection);
std::vector<GParamSpec*> introspection_fields_to_param_list   (const Aida::StringVector &introspection);

} // Bse


#endif /* __SFI_PARAMS_H__ */

/* vim:set ts=8 sts=2 sw=2: */
