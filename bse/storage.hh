// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_STORAGE_HH__
#define __BSE_STORAGE_HH__

#include <bse/bsesource.hh>

namespace Bse {

class Storage {
  class Impl;
  std::shared_ptr<Storage::Impl> impl_;
  bool     import_from_scm   (const String &filename);
public:
  explicit Storage           ();
  virtual ~Storage           ();
  // Writer API
  int      store_file_fd     (const String &filename);
  bool     store_file_buffer (const String &filename, const String &buffer, int64_t epoch_seconds = 0);
  bool     rm_file           (const String &filename);
  bool     set_mimetype_bse  ();
  bool     export_as         (const String &filename);
  // Reader API
  bool     import_as_scm     (const String &filename);
  bool     import_from       (const String &filename);
  String   fetch_file_buffer (const String &filename, ssize_t maxlength = -1);
  String   fetch_file        (const String &filename); // yields abspath
};

std::string beastbse_cachedir_create  ();
void        beastbse_cachedir_cleanup ();
std::string beastbse_cachedir_current ();

} // Bse

#endif // __BSE_STORAGE_HH__
