// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GSL_MAGIC_H__
#define __GSL_MAGIC_H__

#include <bse/gsldefs.hh>




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

namespace Bse {
class FileMagic {
  const String  extension_;
  const String  description_;
  const int     priority_ = 0;
  struct Matcher;
  std::vector<Matcher> matchers_;
  bool              parse_spec     (const String &magic_spec);
  explicit          FileMagic      (const String &fileextension, const String &magic_spec, const String &description, int priority);
  /*dtor*/         ~FileMagic      ();
  bool              match_header   (const String &header);
public:
  int               priority       () const { return priority_; }
  String            extension      () const { return extension_; }
  String            description    () const { return description_; }
  static FileMagic* register_magic (const String &fileextension, const String &magic_spec,
                                    const String &description, int priority = 0);
  static FileMagic* match_list     (const std::vector<FileMagic*> &magics, const String &filename, size_t skip_bytes = 0);
  static String     match_magic    (const String &filename, size_t skip_bytes = 0);
  static const int  MAGIC_HEADER_SIZE = 1024;
};
} // Bse

#endif /* __GSL_MAGIC_H__ */
