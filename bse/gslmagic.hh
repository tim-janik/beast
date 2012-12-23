// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GSL_MAGIC_H__
#define __GSL_MAGIC_H__

#include <bse/gsldefs.hh>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- structures --- */
typedef struct _GslRealMagic GslRealMagic;
struct _GslMagic
{
  gpointer data;
  gchar   *extension;

  /*< private >*/
  gint          priority;
  GslRealMagic *match_list;
};


/* match entity with:
 * prefix,
 * extension,
 * magic_spec
 *
 * where prefix has absolute preference, and extension is just
 * a _hint_ for magic_spec match order, unless magic_spec==NULL
 *
 * no prefix for save handlers. (?) just extension matches.
 *
 * need pre-parse functionality, to figure name and type of a
 * file's contents.
 */


/* --- prototypes --- */
GslMagic*	gsl_magic_create		(gpointer	 data,
						 gint		 priority,
						 const gchar	*extension,
						 const gchar	*magic_spec);
GslMagic*	gsl_magic_list_match_file	(SfiRing	*magic_list,
						 const gchar    *file_name);
GslMagic*	gsl_magic_list_match_file_skip	(SfiRing	*magic_list,
						 const gchar    *file_name,
						 guint           skip_bytes);
void		gsl_magic_list_brute_match	(SfiRing	*magic_list,
						 const gchar	*file_name,
						 guint		 skip_bytes,
						 GslMagic	*skip_magic,
						 SfiRing       **ext_matches,
						 SfiRing       **other_matches);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_MAGIC_H__ */
