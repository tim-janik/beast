// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_DATA_HANDLE_FLAC_HH__
#define __BSE_DATA_HANDLE_FLAC_HH__

#include <bse/gslfilehash.hh>

// == flac datahandle C API ==
GslDataHandle*    bse_data_handle_new_flac          (const char* file_name, float osc_freq);
GslDataHandle*    bse_data_handle_new_flac_zoffset  (const char *file_name, float osc_freq,
                                                     int64 byte_offset, int64 byte_size,
                                                     uint *n_channelsp, float *mix_freq_p);

namespace Bse {

class DataHandleFlac;

/// Flac1Handle supports storing flac files as binary appendix to BSE files.
class Flac1Handle
{
  GslRFile       *rfile;
  GslDataHandle  *dhandle;
  DataHandleFlac *flac_handle;
  uint            byte_length;
  static void destroy_fn (void *handle);
  static int read_data_fn (void *handle, void *buffer, uint blength);
  Flac1Handle (GslDataHandle *dhandle);
public:
  ~Flac1Handle();
  int read_data (void *buffer, uint blength);   ///< Returns -errno || length
  /// This function deletes the flac1handle object when sfi_wstore_destroy (wstore) is executed.
  void put_wstore (SfiWStore *wstore);
  /// Return a valid Flac1Handle if @a dhandle is not flac, and a Flac1Handle otherwise
  static Flac1Handle *create (GslDataHandle *dhandle);
};

} // Bse

#endif // __BSE_DATA_HANDLE_FLAC_HH
