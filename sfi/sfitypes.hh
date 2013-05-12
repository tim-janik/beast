// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __SFI_TYPES_H__
#define __SFI_TYPES_H__

#include <sfi/glib-extra.hh>
#include <sfi/sfiwrapper.hh>

G_BEGIN_DECLS

/* --- Sfi typedefs --- */
typedef bool			SfiBool;
typedef gint			SfiInt;
#define	SFI_MAXINT		(+2147483647)
#define	SFI_MININT		(-SFI_MAXINT - 1)
typedef long long int		SfiNum;
static_assert (sizeof (SfiNum) == 8, "SfiNum");
#define	SFI_MAXNUM		((SfiNum) +9223372036854775807LL)
#define	SFI_MINNUM		(-SFI_MAXNUM - 1)
typedef long long int		SfiTime;
static_assert (sizeof (SfiTime) == 8, "SfiTime");
typedef SfiInt			SfiNote;
typedef double			SfiReal;
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
