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

namespace Bse
{

namespace Resampler
{

}

using std::vector;
using Resampler::Resampler2;

class DataHandleUpsample2 : public GslDataHandle,
			    public Sfi::GNewable /* 0 initialization */
{
  GslDataHandle	       *src_handle;
  int                   precision_bits;

public:
  gboolean		init_ok;
  vector<Resampler2 *>  upsamplers;
  int64			pcm_frame;
  vector<float>		pcm_data;
  int64			frame_size;
  int64                 filter_delay;
  int64                 filter_order;

  DataHandleUpsample2 (GslDataHandle *src_handle, int precision_bits)
    : src_handle (src_handle),
      precision_bits (precision_bits),
      init_ok (false)
  {
    g_return_if_fail (src_handle != NULL);
  
    init_ok = gsl_data_handle_common_init (this, NULL);
    if (init_ok)
      {
	name = g_strconcat (src_handle->name, "// #upsample2 /", NULL);
	src_handle = gsl_data_handle_ref (src_handle);
      }
  }

  ~DataHandleUpsample2()
  {
    gsl_data_handle_unref (src_handle);
    gsl_data_handle_common_free (this);
  }

  BseErrorType
  open (GslDataHandleSetup *setup)
  {
    BseErrorType error = gsl_data_handle_open (src_handle);
    if (error != BSE_ERROR_NONE)
      return error;

    *setup = src_handle->setup; /* copies setup.xinfos by pointer */
    setup->mix_freq *= 2.0;
    setup->n_values *= 2;

    frame_size = 1024 * setup->n_channels;
    pcm_frame = -2;
    pcm_data.resize (frame_size);

    for (guint i = 0; i < setup->n_channels; i++)
      {
	BseResampler2Precision precision = static_cast<BseResampler2Precision> (precision_bits);
	Resampler2 *resampler = Resampler2::create (BSE_RESAMPLER2_MODE_UPSAMPLE, precision);
	g_assert (resampler); /* FIXME: better error handling */

	upsamplers.push_back (resampler);
	filter_order = resampler->order();

	g_assert (filter_order % 2 == 0);
	filter_delay = filter_order / 2 + 1;
      }
    return BSE_ERROR_NONE;
  }

  void
  close()
  {
    for (guint i = 0; i < setup.n_channels; i++)
      delete upsamplers[i];

    upsamplers.clear();
    pcm_data.clear();
    
    setup.xinfos = NULL;	/* cleanup pointer reference */
    gsl_data_handle_close (src_handle);
  }

  int64
  src_read (int64   voffset,
	    int64   n_values,
	    gfloat *values)
  {
    voffset += filter_delay * setup.n_channels;	/* compensate filter delay */

    int64 left = n_values;
    do
      {
	int64 l;
	if (voffset >= 0 && voffset < src_handle->setup.n_values)
	  {
	    l = gsl_data_handle_read (src_handle, voffset, std::min (left, src_handle->setup.n_values - voffset), values);
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
  deinterleave (float* src, float *dest, int64 n_values)
  {
    const int64 n_channels = setup.n_channels;

    for (int64 ch = 0; ch < n_channels; ch++)
      for (int64 v = ch; v < n_values; v += n_channels)
	*dest++ = src[v];
  }

  void
  interleave (float* src, float *dest, int64 n_values)
  {
    const int64 n_channels = setup.n_channels;

    for (int64 ch = 0; ch < n_channels; ch++)
      for (int64 v = ch; v < n_values; v += n_channels)
	dest[v] = *src++;
  }

  int64
  prepare_filter_history (int64 frame)
  {
    const int64 n_channels = setup.n_channels;
    const int64 n_input_samples = filter_order;

    float input_interleaved[n_input_samples * n_channels];
    float input[n_input_samples * n_channels];

    int64 l = src_read (frame * frame_size / 2 - n_input_samples * n_channels,
	                n_input_samples * n_channels, input_interleaved);
    if (l < 0)
      return l; /* pass on errors */

    deinterleave (input_interleaved, input, n_input_samples * setup.n_channels);

    for (guint ch = 0; ch < setup.n_channels; ch++)
      {
	/* we don't need the output, this is just for filling the filter history */
	float output[n_input_samples * 2];

	upsamplers[ch]->process_block (input + ch * n_input_samples, n_input_samples, output);
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
    if (frame != pcm_frame + 1)
      {
	int64 l = prepare_filter_history (frame);
	if (l < 0)
	  return l; /* pass on errors */
      }

    float input_interleaved[frame_size / 2];
    float input[frame_size / 2];
    float output[frame_size];

    int64 l = src_read (frame * frame_size / 2, frame_size / 2, input_interleaved);
    if (l < 0)
      return l; /* pass on errors */

    deinterleave (input_interleaved, input, frame_size / 2);
    for (guint ch = 0; ch < setup.n_channels; ch++)
      {
	const int64 output_per_channel = frame_size / setup.n_channels;
	const int64 input_per_channel = output_per_channel / 2;

	upsamplers[ch]->process_block (input + ch * input_per_channel, input_per_channel, output + ch * output_per_channel);
      }
    interleave (output, &pcm_data[0], frame_size);

    pcm_frame = frame;
    return 1;
  }

  int64
  read (int64          voffset,
	int64          n_values,
	gfloat        *values)
  {
    int64 frame = voffset / pcm_data.size();
    if (frame != pcm_frame)
      {
	int64 l = read_frame (frame);
	if (l < 0)
	  return l;
      }
    g_assert (pcm_frame == frame);

    voffset -= pcm_frame * frame_size;
    g_assert (voffset >= 0);

    n_values = std::min (n_values, frame_size - voffset);
    for (int64 i = 0; i < n_values; i++)
      values[i] = pcm_data[voffset + i];

    return n_values;
  }
};

} // namespace Bse

using namespace Bse;

static BseErrorType
dh_upsample2_open (GslDataHandle *dhandle, GslDataHandleSetup *setup)
{
  return reinterpret_cast<DataHandleUpsample2 *> (dhandle)->open (setup);
}

static void
dh_upsample2_close (GslDataHandle *dhandle)
{
  reinterpret_cast<DataHandleUpsample2 *> (dhandle)->close();
}


static void
dh_upsample2_destroy (GslDataHandle *dhandle)
{
  delete reinterpret_cast<DataHandleUpsample2 *> (dhandle);
}

static int64
dh_upsample2_read (GslDataHandle *dhandle,
		   int64          voffset,
		   int64          n_values,
		   gfloat        *values)
{
  return reinterpret_cast<DataHandleUpsample2 *> (dhandle)->read (voffset, n_values, values);
}

GslDataHandle*
bse_data_handle_new_upsample2 (GslDataHandle *src_handle, int precision_bits)
{
  static GslDataHandleFuncs dh_upsample2_vtable = {
    dh_upsample2_open,
    dh_upsample2_read,
    dh_upsample2_close,
    NULL,
    dh_upsample2_destroy,
  };
  DataHandleUpsample2 *dhandle = new DataHandleUpsample2 (src_handle, precision_bits);
  if (dhandle->init_ok)
    {
      dhandle->vtable = &dh_upsample2_vtable;
      return dhandle;
    }
  else
    {
      delete dhandle;
      return NULL;
    }
}
