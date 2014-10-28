// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef	__SFI_UTILS_H__
#define	__SFI_UTILS_H__

#include <math.h>
#include <bse/gsldefs.hh>
#include <sfi/sfi.hh>

G_BEGIN_DECLS

typedef struct
{
  guint   line_number;
  guint   n_fields;
  gchar **fields;
} SfiUtilFileEntry;

typedef struct
{
  guint             n_entries;
  SfiUtilFileEntry *entries;
  guint	            n_formats;
  gchar           **formats;
  gpointer          free1, free2;
} SfiUtilFileList;

/* value extraction from formats:
 * # <something>	-> value is <something>
 * <num>		-> value is <num> word of line
 * <num> n <nth>	-> <nth> number found in word <num>
 * <num> b <nth>	-> <nth> number found in basename(word <num>)
 */
SfiUtilFileList* sfi_util_file_list_read	(gint			 fd);
SfiUtilFileList* sfi_util_file_list_read_simple	(const gchar		*file_name,
						 guint			 n_formats,
						 const gchar		*formats);
void		 sfi_util_file_list_free	(SfiUtilFileList	*text);
const gchar*	 sfi_util_file_entry_get_field	(SfiUtilFileEntry	*entry,
						 const gchar	       **format_p);
gchar*		 sfi_util_file_entry_get_string	(SfiUtilFileEntry	*entry,
						 const gchar		*format,
						 const gchar		*dflt);
gdouble		 sfi_util_file_entry_get_num	(SfiUtilFileEntry	*line,
						 const gchar		*format,
						 gdouble	         dflt);


gchar*		 sfi_util_file_name_subst_ext	(const gchar		*file_name,
						 const gchar		*new_extension);

typedef struct {
  gchar	        short_opt;
  gchar        *long_opt;
  const gchar **value_p;
  guint         takes_arg : 1;
} SfiArgument;
void	    sfi_arguments_parse 	(gint            *argc_p,
					 gchar         ***argv_p,
					 guint            n_options,
					 const SfiArgument *options);
SfiRing*    sfi_arguments_parse_list	(gint            *argc_p,
					 gchar         ***argv_p,
					 guint            n_options,
					 const SfiArgument *options);
void	    sfi_arguments_collapse	(gint		 *argc_p,
					 gchar	       ***argv_p);

/* format for value extraction:
 * # <something>	-> string is <something>
 * n <nth>		-> <nth> number found in string
 * b <nth>		-> <nth> number found in basename(string)
 * c [*<num>]		-> counter (with optional multiplication)
 * if <nth>==0, number may not be preceeded by non-digit chars
 */
gdouble	    sfi_arguments_extract_num	(const gchar	*string,
                                         const gchar	*format,
                                         gdouble	*counter,
                                         gdouble	 dflt);
gboolean    sfi_arguments_read_num	(const gchar   **option,
					 gdouble	*num);
guint	    sfi_arguments_read_all_nums	(const gchar    *option,
					 gdouble	*first,
					 ...) G_GNUC_NULL_TERMINATED;

G_END_DECLS

#endif	/* __SFI_UTILS_H__ */
