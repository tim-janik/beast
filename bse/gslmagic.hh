// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GSL_MAGIC_HH__
#define __GSL_MAGIC_HH__

#include <bse/gsldefs.hh>

namespace Bse {

class FileMagic {
  const String  extension_;
  const String  description_;
  std::shared_ptr<void> sdata_;
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
  void              sdata          (std::shared_ptr<void> data) { sdata_ = data; }
  std::shared_ptr<void> sdata      () const                     { return sdata_; }
  static FileMagic* register_magic (const String &fileextension, const String &magic_spec,
                                    const String &description, int priority = 0);
  static FileMagic* match_list     (const std::vector<FileMagic*> &magics, const String &filename, size_t skip_bytes = 0);
  static String     match_magic    (const String &filename, size_t skip_bytes = 0);
  static const int  MAGIC_HEADER_SIZE = 1024;
};

} // Bse

#endif // __GSL_MAGIC_HH__
