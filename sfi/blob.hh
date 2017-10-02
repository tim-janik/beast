// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __BSE_BLOB_HH__
#define __BSE_BLOB_HH__

#include <sfi/cxxaux.hh>

namespace Bse {

class BlobImpl;

/// Binary large object storage container.
class Blob {
  std::shared_ptr<BlobImpl>  implp_;
  static Blob  from_res      (const char *resource);
  static Blob  from_string   (const String &name, const String &data);
  explicit     Blob          (std::shared_ptr<BlobImpl> blobimpl);
public:
  String       name          ();                        ///< Retrieve the Blob's filename or url.
  const char*  data          ();                        ///< Retrieve the Blob's data.
  const uint8* bytes         ();                        ///< Retrieve the Blob's data as uint8 buffer.
  size_t       size          ();                        ///< Retrieve the Blob's data size in bytes.
  String       string        ();                        ///< Copy Blob data into a zero terminated string.
  explicit     Blob          ();                        ///< Construct an empty Blob.
  explicit     Blob          (const String &auto_url);  ///< Construct Blob from url or filename (auto detected).
  static Blob  from_file     (const String &filename);  ///< Create Blob by loading from @a filename.
  static Blob  from_url      (const String &url);       ///< Create Blob by opening a @a url.
  explicit     operator bool () const;                  ///< Checks if the Blob contains accessible data.
};

// == Helpers ==
uint8*  zintern_decompress (unsigned int decompressed_size, const unsigned char *cdata, unsigned int cdata_size);
void    zintern_free       (uint8 *dc_data);

} // Bse

#endif // __BSE_BLOB_HH__
