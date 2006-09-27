/* BSE Resampling Datahandles
 * Copyright (C) 2001-2003 Tim Janik
 * Copyright (C) 2006 Stefan Westerfeld
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bseresampler.hh"
#include "gsldatahandle.h"
#include <sfi/sficxx.hh>
#include <vector>

namespace Bse {
using Resampler::Resampler2;
using std::vector;

class DataHandleUpsample2
{
  GslDataHandle         m_dhandle;
  GslDataHandle	       *m_src_handle;
  int                   m_precision_bits;
  vector<Resampler2 *>  m_upsamplers;
  int64			m_pcm_frame;
  vector<float>		m_pcm_data;
  int64			m_frame_size;
  int64                 m_filter_delay;
  int64                 m_filter_order;
  bool   		m_init_ok;

  /* private destructor: (use reference counting instead) */
  ~DataHandleUpsample2()
  {
    if (m_init_ok)
      {
	gsl_data_handle_unref (m_src_handle);
	gsl_data_handle_common_free (&m_dhandle);
      }
  }
public:
  DataHandleUpsample2 (GslDataHandle *src_handle,
                       int            precision_bits) :
    m_src_handle (src_handle),
    m_precision_bits (precision_bits),
    m_pcm_frame (0),			  // unnecessary, but makes debugging easier - just in case
    m_frame_size (0),			  // unnecessary, but makes debugging easier - just in case
    m_filter_delay (0),			  // unnecessary, but makes debugging easier - just in case
    m_filter_order (0),			  // unnecessary, but makes debugging easier - just in case
    m_init_ok (false)
  {
    g_return_if_fail (src_handle != NULL);
 
    memset (&m_dhandle, 0, sizeof (m_dhandle));

    m_init_ok = gsl_data_handle_common_init (&m_dhandle, NULL);
    if (m_init_ok)
      {
	m_dhandle.name = g_strconcat (m_src_handle->name, "// #upsample2 /", NULL);
	gsl_data_handle_ref (m_src_handle);
      }
  }
  BseErrorType
  open (GslDataHandleSetup *setup)
  {
    BseErrorType error = gsl_data_handle_open (m_src_handle);
    if (error != BSE_ERROR_NONE)
      return error;

    /* !not! m_dhandle.setup; the framework magically ensures that *m_dhandle.setup
     * is initialized by whatever we write into *setup if open is successful
     */
    *setup = m_src_handle->setup; /* copies setup.xinfos by pointer */
    setup->mix_freq *= 2.0;
    setup->n_values *= 2;

    m_frame_size = 1024 * setup->n_channels;
    m_pcm_frame = -2;
    m_pcm_data.resize (m_frame_size);

    for (guint i = 0; i < setup->n_channels; i++)
      {
	BseResampler2Precision precision = static_cast<BseResampler2Precision> (m_precision_bits);
	Resampler2 *resampler = Resampler2::create (BSE_RESAMPLER2_MODE_UPSAMPLE, precision);
	g_assert (resampler); /* FIXME: better error handling */

	m_upsamplers.push_back (resampler);
	m_filter_order = resampler->order();

	g_assert (m_filter_order % 2 == 0);
	m_filter_delay = m_filter_order / 2 + 1;
      }
    return BSE_ERROR_NONE;
  }
  void
  close()
  {
    for (guint i = 0; i < m_dhandle.setup.n_channels; i++)
      delete m_upsamplers[i];

    m_upsamplers.clear();
    m_pcm_data.clear();
    
    m_dhandle.setup.xinfos = NULL;	/* cleanup pointer reference */
    gsl_data_handle_close (m_src_handle);
  }
  int64
  src_read (int64   voffset,
	    int64   n_values,
	    gfloat *values)
  {
    voffset += m_filter_delay * m_dhandle.setup.n_channels;	/* compensate filter delay */

    int64 left = n_values;
    do
      {
	int64 l;
	if (voffset >= 0 && voffset < m_src_handle->setup.n_values)
	  {
	    l = gsl_data_handle_read (m_src_handle, voffset, std::min (left, m_src_handle->setup.n_values - voffset), values);
	    if (l < 0)
	      return l;	/* pass on errors */
	  }
	else
	  {
	    /*
	     * the resampler needs data before and after the data provided
	     * by src_handle; so we offer zero values here
	     */
	    *values = 0;
	    l = 1;
	  }

	voffset += l;
	left -= l;
	values += l;
      }
    while (left > 0);

    return n_values;
  }
  void
  deinterleave (float *src,
                float *dest,
                int64  n_values)
  {
    const int64 n_channels = m_dhandle.setup.n_channels;

    for (int64 ch = 0; ch < n_channels; ch++)
      for (int64 v = ch; v < n_values; v += n_channels)
	*dest++ = src[v];
  }
  void
  interleave (float *src,
              float *dest,
              int64  n_values)
  {
    const int64 n_channels = m_dhandle.setup.n_channels;

    for (int64 ch = 0; ch < n_channels; ch++)
      for (int64 v = ch; v < n_values; v += n_channels)
	dest[v] = *src++;
  }
  int64
  prepare_filter_history (int64 frame)
  {
    const int64 n_channels = m_dhandle.setup.n_channels;
    const int64 n_input_samples = m_filter_order;

    float input_interleaved[n_input_samples * n_channels];
    float input[n_input_samples * n_channels];

    int64 l = src_read (frame * m_frame_size / 2 - n_input_samples * n_channels,
	                n_input_samples * n_channels, input_interleaved);
    if (l < 0)
      return l; /* pass on errors */

    deinterleave (input_interleaved, input, n_input_samples * m_dhandle.setup.n_channels);

    for (guint ch = 0; ch < m_dhandle.setup.n_channels; ch++)
      {
	/* we don't need the output, this is just for filling the filter history */
	float output[n_input_samples * 2];

	m_upsamplers[ch]->process_block (input + ch * n_input_samples, n_input_samples, output);
      }
    return 1;
  }
  int64
  read_frame (int64 frame)
  {
    /*
     * if we're seeking (not reading data linearily), we need to reinitialize
     * the filter history with new values
     */
    if (frame != m_pcm_frame + 1)
      {
	int64 l = prepare_filter_history (frame);
	if (l < 0)
	  return l; /* pass on errors */
      }

    float input_interleaved[m_frame_size / 2];
    float input[m_frame_size / 2];
    float output[m_frame_size];

    int64 l = src_read (frame * m_frame_size / 2, m_frame_size / 2, input_interleaved);
    if (l < 0)
      return l; /* pass on errors */

    deinterleave (input_interleaved, input, m_frame_size / 2);
    for (guint ch = 0; ch < m_dhandle.setup.n_channels; ch++)
      {
	const int64 output_per_channel = m_frame_size / m_dhandle.setup.n_channels;
	const int64 input_per_channel = output_per_channel / 2;

	m_upsamplers[ch]->process_block (input + ch * input_per_channel, input_per_channel, output + ch * output_per_channel);
      }
    interleave (output, &m_pcm_data[0], m_frame_size);

    m_pcm_frame = frame;
    return 1;
  }
  int64
  read (int64  voffset,
	int64  n_values,
	float *values)
  {
    int64 frame = voffset / m_pcm_data.size();
    if (frame != m_pcm_frame)
      {
	int64 l = read_frame (frame);
	if (l < 0)
	  return l;
      }
    g_assert (m_pcm_frame == frame);

    voffset -= m_pcm_frame * m_frame_size;
    g_assert (voffset >= 0);

    n_values = std::min (n_values, m_frame_size - voffset);
    for (int64 i = 0; i < n_values; i++)
      values[i] = m_pcm_data[voffset + i];

    return n_values;
  }

private:
/* for the "C" API (vtable) */
  static DataHandleUpsample2*
  dh_cast (GslDataHandle *dhandle)
  {
    return reinterpret_cast<DataHandleUpsample2 *> (dhandle);
  }
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
    return dh_cast (dhandle)->read (voffset, n_values, values);
  }

public:
  static GslDataHandle*
  dh_create (GslDataHandle *src_handle,
	     int            precision_bits)
  {
    static GslDataHandleFuncs dh_vtable =
    {
      dh_open,
      dh_read,
      dh_close,
      NULL,
      dh_destroy,
    };
    DataHandleUpsample2 *dhandle = new DataHandleUpsample2 (src_handle, precision_bits);
    if (dhandle->m_init_ok)
      {
	dhandle->m_dhandle.vtable = &dh_vtable;
	return &dhandle->m_dhandle;
      }
    else
      {
	delete dhandle;
	return NULL;
      }
  }
};

} // Bse

extern "C" GslDataHandle*
bse_data_handle_new_upsample2 (GslDataHandle *src_handle,
                               int            precision_bits)
{
  return Bse::DataHandleUpsample2::dh_create (src_handle, precision_bits);
}
