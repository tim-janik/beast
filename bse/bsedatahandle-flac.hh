// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_DATA_HANDLE_FLAC_HH__
#define __BSE_DATA_HANDLE_FLAC_HH__

#include <bse/gslfilehash.hh>

/* --- flac datahandle C API--- */
GslDataHandle*    bse_data_handle_new_flac          (const gchar*   file_name,
                                                     gfloat         osc_freq);
GslDataHandle*    bse_data_handle_new_flac_zoffset  (const gchar   *file_name,
                                                     float          osc_freq,
                                                     GslLong        byte_offset,
                                                     GslLong        byte_size,
                                                     uint          *n_channelsp,
                                                     float         *mix_freq_p);

namespace Bse
{

/* Flac1Handle supports storing flac files as binary appendix */
class DataHandleFlac;
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

  /* returns -errno || length */
  int read_data (void *buffer, uint blength);

  // put_wstore() deletes flac1handle object when sfi_wstore_destroy (wstore) is executed.
  void put_wstore (SfiWStore *wstore);

  /* returns valid Flac1Handle if dhandle is not flac, Flac1Handle otherwise */
  static Flac1Handle *create (GslDataHandle *dhandle);
};

};

#endif /* __BSE_DATA_HANDLE_FLAC_HH */
