/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2001, 2003 Tim Janik and Stefan Westerfeld
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "gsldatahandle.hh"
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

class DataHandleFlac;

struct CDataHandleFlac : public GslDataHandle
{
  // back pointer to get casting right, even in presence of C++ vtable:
  DataHandleFlac* cxx_dh;
};

class DataHandleFlac {
private:
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
  static void
  flac_error_callback (const FLAC__StreamDecoder     *decoder,
                       FLAC__StreamDecoderErrorStatus status,
                       void                          *client_data)
  {
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

public:
  DataHandleFlac (const string& file_name,
		  float         osc_freq) :
    m_init_ok (false),
    m_decoder (NULL)
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

    int err = FLAC__stream_decoder_init_file (m_decoder, m_file_name.c_str(),
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

    m_n_channels = setup->n_channels = FLAC__stream_decoder_get_channels (m_decoder);
    setup->n_values = FLAC__stream_decoder_get_total_samples (m_decoder);
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
        FLAC__bool mdok = FLAC__stream_decoder_process_single (m_decoder);
      }

    if (voffset >= m_buffer_start && voffset < m_buffer_start + m_buffer.size())
      {
        int64 buffer_offset = voffset - m_buffer_start;
        n_values = MIN (n_values, m_buffer.size() - buffer_offset);
        std::copy (m_buffer.begin() + buffer_offset, m_buffer.begin() + buffer_offset + n_values, values);
        return n_values;
      }

    // need to seek to get to the right location
    FLAC__bool seek_ok = FLAC__stream_decoder_seek_absolute (m_decoder, voffset / m_n_channels);
    if (!seek_ok)
      return -1;

    if (voffset == m_buffer_start)
      return read_samples (voffset, n_values, values);    // will work this time, since we have the right samples now

    return 0;
  }

  static GslDataHandle*
  dh_create (DataHandleFlac *cxx_dh)
  {
    static GslDataHandleFuncs dh_vtable =
    {
      dh_open,
      dh_read,
      dh_close,
      NULL,
      NULL,
      dh_destroy,
    };

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

}

using namespace Bse;

extern "C" GslDataHandle*
bse_data_handle_new_flac (const char    *file_name,
                          gfloat         osc_freq)
{
  DataHandleFlac *cxx_dh = new DataHandleFlac (file_name, osc_freq);
  return DataHandleFlac::dh_create (cxx_dh);
}
