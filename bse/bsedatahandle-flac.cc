// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "gsldatahandle.hh"
#include "bsedatahandle-flac.hh"
#include "gsldatautils.hh"
#include "gslfilter.hh"
#include "bseblockutils.hh"
#include <complex>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <FLAC/stream_decoder.h>

namespace Bse {

using std::vector;
using std::string;
using std::min;

namespace {
  enum FlacZOffset { NO_ZOFFSET, ADD_ZOFFSET };
}

class DataHandleFlac;

struct CDataHandleFlac : public GslDataHandle
{
  // back pointer to get casting right, even in presence of C++ vtable:
  DataHandleFlac* cxx_dh;
};

class DataHandleFlac {
private:
// ***************************** virtual file I/O ********************************
  GslLong   m_file_byte_offset;
  GslLong   m_file_byte_size;
  GslRFile *m_rfile;

  static FLAC__StreamDecoderReadStatus
  file_read_callback (const FLAC__StreamDecoder  *decoder,
                      FLAC__byte                  buffer[],
                      size_t                     *bytes,
                      void                       *client_data)
  {
    DataHandleFlac *dh = static_cast<DataHandleFlac *> (client_data);

    const size_t bytes_to_eof = dh->m_file_byte_size - (gsl_rfile_position (dh->m_rfile) - dh->m_file_byte_offset);
    GslLong l = gsl_rfile_read (dh->m_rfile, MIN (*bytes, bytes_to_eof), buffer);
    if (l < 0)
      return FLAC__STREAM_DECODER_READ_STATUS_ABORT;

    *bytes = l;
    if (*bytes == 0)
      return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
    else
      return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
  }

  static FLAC__StreamDecoderSeekStatus
  file_seek_callback (const FLAC__StreamDecoder *decoder,
                      FLAC__uint64               absolute_byte_offset,
                      void                      *client_data)
  {
    DataHandleFlac *dh = static_cast<DataHandleFlac *> (client_data);

    int64 l = dh->m_file_byte_offset + absolute_byte_offset;
    l = CLAMP (l, dh->m_file_byte_offset, dh->m_file_byte_offset + dh->m_file_byte_size);
    l = gsl_rfile_seek_set (dh->m_rfile, l);
    if (l >= 0)
      return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
    else
      return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
  }

  static FLAC__StreamDecoderTellStatus
  file_tell_callback (const FLAC__StreamDecoder *decoder,
                      FLAC__uint64              *absolute_byte_offset,
                      void                      *client_data)
  {
    DataHandleFlac *dh = static_cast<DataHandleFlac *> (client_data);

    *absolute_byte_offset = gsl_rfile_position (dh->m_rfile) - dh->m_file_byte_offset;
    return FLAC__STREAM_DECODER_TELL_STATUS_OK;
  }

  static FLAC__StreamDecoderLengthStatus
  file_length_callback (const FLAC__StreamDecoder *decoder,
                        FLAC__uint64              *stream_length,
                        void                      *client_data)
  {
    DataHandleFlac *dh = static_cast<DataHandleFlac *> (client_data);

    *stream_length = dh->m_file_byte_size;
    return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
  }

  static FLAC__bool
  file_eof_callback (const FLAC__StreamDecoder *decoder,
                     void *client_data)
  {
    DataHandleFlac *dh = static_cast<DataHandleFlac *> (client_data);

    return dh->m_file_byte_size == (gsl_rfile_position (dh->m_rfile) - dh->m_file_byte_offset);
  }

// *******************************************************************************

  static FLAC__StreamDecoderWriteStatus
  flac_write_callback (const FLAC__StreamDecoder  *decoder,
                       const FLAC__Frame          *frame,
                       const FLAC__int32          *const buffer[],
                       void                       *client_data)
  {
    DataHandleFlac *dh = static_cast<DataHandleFlac *> (client_data);
    dh->m_buffer_start = frame->header.number.sample_number * frame->header.channels;
    dh->m_buffer.clear();

    // scale with 1/32768 for 16 bit, 1/(2^23) for 24 bit
    double scale = 2.0 / (1 << frame->header.bits_per_sample);
    for (int i = 0; i < frame->header.blocksize; i++)
      {
        // interleave channels
        for (int ch = 0; ch < frame->header.channels; ch++)
          dh->m_buffer.push_back (buffer[ch][i] * scale);
      }
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
  }

  // pass error status from flac callback to caller
  bool                              m_error_occurred;
  FLAC__StreamDecoderErrorStatus    m_error_status;

  static void
  flac_error_callback (const FLAC__StreamDecoder     *decoder,
                       FLAC__StreamDecoderErrorStatus status,
                       void                          *client_data)
  {
    DataHandleFlac *dh = static_cast<DataHandleFlac *> (client_data);
    dh->m_error_occurred = true;
    dh->m_error_status = status;
  }

protected:
  CDataHandleFlac	m_dhandle;
  int                   m_n_channels;
  bool			m_init_ok;
  string                m_file_name;
  FLAC__StreamDecoder  *m_decoder;
  int64                 m_buffer_start;
  vector<float>         m_buffer;
  float                 m_osc_freq;
  FlacZOffset           m_init_add_zoffset;
  GslLong               m_init_byte_offset;
  GslLong               m_init_byte_size;

public:
  DataHandleFlac (const string& file_name,
		  float         osc_freq,
                  FlacZOffset   add_zoffset,
                  GslLong       byte_offset = -1,
                  GslLong       byte_size = -1) :
    m_init_ok (false),
    m_decoder (NULL),
    m_init_add_zoffset (add_zoffset),
    m_init_byte_offset (byte_offset),
    m_init_byte_size (byte_size)
  {
    memset (&m_dhandle, 0, sizeof (m_dhandle));
    m_init_ok = gsl_data_handle_common_init (&m_dhandle, NULL);
    m_file_name = file_name;
    m_dhandle.name = g_strdup (m_file_name.c_str());
    m_osc_freq = osc_freq;
  }

  /* protected destructor: (use reference counting instead) */
  virtual
  ~DataHandleFlac()
  {
    if (m_init_ok)
      gsl_data_handle_common_free (&m_dhandle);
  }

  BseErrorType
  open (GslDataHandleSetup *setup)
  {
    m_decoder = FLAC__stream_decoder_new();
    if (!m_decoder)
      return BSE_ERROR_IO;

    m_rfile = gsl_rfile_open (m_file_name.c_str());
    if (!m_rfile)
      return gsl_error_from_errno (errno, BSE_ERROR_FILE_OPEN_FAILED);

    if (m_init_byte_offset >= 0 && m_init_byte_size >= 0)
      {
        m_file_byte_offset = m_init_byte_offset;
        m_file_byte_size = m_init_byte_size;
      }
    else
      {
        m_file_byte_offset = 0;
        m_file_byte_size = gsl_rfile_length (m_rfile);
      }

    if (m_init_add_zoffset == ADD_ZOFFSET)
      {
        m_file_byte_offset += gsl_hfile_zoffset (m_rfile->hfile) + 1;
      }

    /* seek to beginning of the virtual file */
    file_seek_callback (m_decoder, 0, this);

    m_error_occurred = false;
    int err = FLAC__stream_decoder_init_stream (m_decoder,
                                                file_read_callback,
                                                file_seek_callback,
                                                file_tell_callback,
                                                file_length_callback,
                                                file_eof_callback,
                                                flac_write_callback, NULL, flac_error_callback, this);
    if (err != 0)
      return BSE_ERROR_IO;

    /* decode enough to figure out number of channels */
    FLAC__bool mdok;
    do {
      mdok = FLAC__stream_decoder_process_single (m_decoder);
    } while (FLAC__stream_decoder_get_channels (m_decoder) == 0 && mdok);

    if (FLAC__stream_decoder_get_channels (m_decoder) == 0)
      return BSE_ERROR_IO;

    if (m_error_occurred)
      return BSE_ERROR_IO;

    m_n_channels = setup->n_channels = FLAC__stream_decoder_get_channels (m_decoder);
    setup->n_values = FLAC__stream_decoder_get_total_samples (m_decoder) * m_n_channels;
    setup->bit_depth = FLAC__stream_decoder_get_bits_per_sample (m_decoder);
    setup->mix_freq = FLAC__stream_decoder_get_sample_rate (m_decoder);
    setup->xinfos = bse_xinfos_add_float (setup->xinfos, "osc-freq", m_osc_freq);

    return BSE_ERROR_NONE;
  }

  void
  close()
  {
    m_dhandle.setup.xinfos = NULL;	/* cleanup pointer reference */
  }

  int64
  read_samples (int64  voffset,
	        int64  n_values,
	        float *values)
  {
    if (voffset >= m_buffer_start + m_buffer.size())
      {
        // try to read on, probably we'll have just the samples we need, then
        m_error_occurred = false;
        FLAC__bool decode_ok = FLAC__stream_decoder_process_single (m_decoder);

        if (!decode_ok || m_error_occurred)
          return -1;
      }

    if (voffset >= m_buffer_start && voffset < m_buffer_start + m_buffer.size())
      {
        int64 buffer_offset = voffset - m_buffer_start;
        n_values = MIN (n_values, m_buffer.size() - buffer_offset);
        std::copy (m_buffer.begin() + buffer_offset, m_buffer.begin() + buffer_offset + n_values, values);
        return n_values;
      }

    // need to seek to get to the right location
    m_error_occurred = false;
    FLAC__bool seek_ok = FLAC__stream_decoder_seek_absolute (m_decoder, voffset / m_n_channels);
    if (!seek_ok || m_error_occurred)
      return -1;

    if (voffset == m_buffer_start)
      return read_samples (voffset, n_values, values);    // will work this time, since we have the right samples now

    return 0;
  }

  static GslDataHandleFuncs dh_vtable;
  static GslDataHandle*
  dh_create (DataHandleFlac *cxx_dh)
  {
    if (cxx_dh->m_init_ok)
      {
	cxx_dh->m_dhandle.vtable = &dh_vtable;
	cxx_dh->m_dhandle.cxx_dh = cxx_dh;	/* make casts work, later on */
	return &cxx_dh->m_dhandle;
      }
    else
      {
	delete cxx_dh;
	return NULL;
      }
  }
  static DataHandleFlac*
  dh_cast (GslDataHandle *dhandle)
  {
    return static_cast<CDataHandleFlac *> (dhandle)->cxx_dh;
  }

  GslLong
  file_byte_offset()
  {
    return m_file_byte_offset;
  }
  GslLong
  file_byte_size()
  {
    return m_file_byte_size;
  }
private:
/* for the "C" API (vtable) */
  static BseErrorType
  dh_open (GslDataHandle *dhandle, GslDataHandleSetup *setup)
  {
    return dh_cast (dhandle)->open (setup);
  }
  static void
  dh_close (GslDataHandle *dhandle)
  {
    dh_cast (dhandle)->close();
  }
  static void
  dh_destroy (GslDataHandle *dhandle)
  {
    delete dh_cast (dhandle);
  }
  static int64
  dh_read (GslDataHandle *dhandle,
	   int64          voffset,
	   int64          n_values,
	   gfloat        *values)
  {
    return dh_cast (dhandle)->read_samples (voffset, n_values, values);
  }
};

GslDataHandleFuncs DataHandleFlac::dh_vtable
{
  dh_open,
  dh_read,
  dh_close,
  NULL,
  NULL,
  dh_destroy,
};

}

using namespace Bse;

GslDataHandle*
bse_data_handle_new_flac (const char    *file_name,
                          gfloat         osc_freq)
{
  DataHandleFlac *cxx_dh = new DataHandleFlac (file_name, osc_freq, NO_ZOFFSET);
  return DataHandleFlac::dh_create (cxx_dh);
}

GslDataHandle*
bse_data_handle_new_flac_zoffset (const char *file_name,
                                  float       osc_freq,
                                  GslLong     byte_offset,
                                  GslLong     byte_size,
                                  uint       *n_channels_p,
                                  gfloat     *mix_freq_p)
{
  g_return_val_if_fail (file_name != NULL, NULL);
  g_return_val_if_fail (byte_offset >= 0, NULL);
  g_return_val_if_fail (byte_size > 0, NULL);

  DataHandleFlac *cxx_dh = new DataHandleFlac (file_name, osc_freq, ADD_ZOFFSET, byte_offset, byte_size);
  GslDataHandle *dhandle = DataHandleFlac::dh_create (cxx_dh);
  if (!dhandle)
    return NULL;

  /* figure out mix_freq, n_channels */
  BseErrorType error = gsl_data_handle_open (dhandle);
  if (!error)
    {
      if (n_channels_p)
        *n_channels_p = dhandle->setup.n_channels;
      if (mix_freq_p)
        *mix_freq_p = dhandle->setup.mix_freq;

      gsl_data_handle_close (dhandle);
      return dhandle;
    }
  else
    {
      gsl_data_handle_unref (dhandle);
      return NULL;
    }
}

/* flac storage */

Flac1Handle *
Flac1Handle::create (GslDataHandle *dhandle)
{
  if (dhandle->vtable == &DataHandleFlac::dh_vtable &&
      gsl_data_handle_open (dhandle) == BSE_ERROR_NONE)
    {
      return new Flac1Handle (dhandle);
    }
  else
    {
      return NULL;
    }
}

void
Flac1Handle::destroy_fn (gpointer data)
{
  delete (Flac1Handle *) data;
}

int
Flac1Handle::read_data_fn (void *data, void *buffer, uint blength)
{
  Flac1Handle *flac_handle = (Flac1Handle*) data;

  return flac_handle->read_data (buffer, blength);
}

Flac1Handle::Flac1Handle (GslDataHandle *dhandle) :
  rfile (nullptr),
  dhandle (dhandle)
{
  flac_handle = DataHandleFlac::dh_cast (dhandle);
}

Flac1Handle::~Flac1Handle()
{
  if (rfile)
    {
      gsl_rfile_close (rfile);
      rfile = NULL;
    }
  gsl_data_handle_close (dhandle);
  dhandle = NULL;
}

int
Flac1Handle::read_data (void *buffer, uint blength)
{
  if (!rfile)
    {
      rfile = gsl_rfile_open (dhandle->name);
      if (!rfile)
        return gsl_error_from_errno (errno, BSE_ERROR_FILE_OPEN_FAILED);
      byte_length = gsl_rfile_length (rfile);
      gsl_rfile_seek_set (rfile, flac_handle->file_byte_offset());
    }
  const uint bytes_to_eof = flac_handle->file_byte_size() - (gsl_rfile_position (rfile) - flac_handle->file_byte_offset());

  int n_bytes_read;
  do
    n_bytes_read = gsl_rfile_read (rfile, std::min (blength, bytes_to_eof), buffer);
  while (n_bytes_read < 0 && errno == EINTR);

  if (n_bytes_read <= 0)               /* bail on errors */
    return errno ? -errno : -EIO;

  return n_bytes_read;
}

void
Flac1Handle::put_wstore (SfiWStore *wstore)
{
  sfi_wstore_put_binary (wstore, read_data_fn, this, destroy_fn);
}
