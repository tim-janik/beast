// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseresampler.hh"
#include "gsldatahandle.hh"
#include <sfi/sficxx.hh>
#include <vector>

namespace Bse {
using Resampler::Resampler2;
using std::vector;

class DataHandleResample2;

struct CDataHandleResample2 : public GslDataHandle
{
  // back pointer to get casting right, even in presence of C++ vtable:
  DataHandleResample2* cxx_dh;
};

class DataHandleResample2
{
protected:
  CDataHandleResample2	m_dhandle;
  GslDataHandle	       *m_src_handle;
  int                   m_precision_bits;
  vector<Resampler2 *>  m_resamplers;
  int64			m_pcm_frame;
  vector<float>		m_pcm_data;
  int64			m_frame_size;
  int64                 m_filter_delay;
  int64                 m_filter_delay_input;
  int64                 m_filter_order;
  bool			m_init_ok;

  DataHandleResample2 (GslDataHandle *src_handle,
                       int            precision_bits) :
    m_src_handle (src_handle),
    m_precision_bits (precision_bits),
    m_pcm_frame (0),			  // unnecessary, but makes debugging easier - just in case
    m_frame_size (0),			  // unnecessary, but makes debugging easier - just in case
    m_filter_delay (0),			  // unnecessary, but makes debugging easier - just in case
    m_filter_order (0),			  // unnecessary, but makes debugging easier - just in case
    m_init_ok (false)
  {
    assert_return (src_handle != NULL);

    memset (&m_dhandle, 0, sizeof (m_dhandle));
    m_init_ok = gsl_data_handle_common_init (&m_dhandle, NULL);
    if (m_init_ok)
      gsl_data_handle_ref (m_src_handle);
  }

  /* protected destructor: (use reference counting instead) */
  virtual
  ~DataHandleResample2()
  {
    if (m_init_ok)
      {
	gsl_data_handle_unref (m_src_handle);
	gsl_data_handle_common_free (&m_dhandle);
      }
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
  deinterleave (float  *src,
                float  *dest,
                size_t  n_values)
  {
    const guint n_channels = m_dhandle.setup.n_channels;

    for (guint ch = 0; ch < n_channels; ch++)
      for (size_t v = ch; v < n_values; v += n_channels)
	*dest++ = src[v];
  }
  void
  interleave (float  *src,
              float  *dest,
              size_t  n_values)
  {
    const guint n_channels = m_dhandle.setup.n_channels;

    for (guint ch = 0; ch < n_channels; ch++)
      for (size_t v = ch; v < n_values; v += n_channels)
	dest[v] = *src++;
  }

  /* implemented by upsampling and downsampling datahandle */
  virtual BseResampler2Mode mode	() const = 0;
  virtual int64		    read_frame  (int64 frame) = 0;

public:
  Bse::Error
  open (GslDataHandleSetup *setup)
  {
    Bse::Error error = gsl_data_handle_open (m_src_handle);
    if (error != Bse::Error::NONE)
      return error;

    /* !not! m_dhandle.setup; the framework magically ensures that *m_dhandle.setup
     * is initialized by whatever we write into *setup if open is successful
     */
    *setup = m_src_handle->setup; /* copies setup.xinfos by pointer */
    switch (mode())
      {
      case BSE_RESAMPLER2_MODE_UPSAMPLE:    setup->mix_freq *= 2.0;
					    setup->n_values *= 2;
					    break;
      case BSE_RESAMPLER2_MODE_DOWNSAMPLE:  setup->mix_freq /= 2.0;
					    setup->n_values = (setup->n_values + 1) / 2;
					    break;
      default:				    assert_return_unreached (Bse::Error::INTERNAL);
      }

    m_frame_size = 1024 * setup->n_channels;
    m_pcm_frame = -2;
    m_pcm_data.resize (m_frame_size);

    BseResampler2Precision precision = Resampler2::find_precision_for_bits (m_precision_bits);
    for (guint i = 0; i < setup->n_channels; i++)
      {
	Resampler2 *resampler = Resampler2::create (mode(), precision);
	assert_return (resampler, Bse::Error::INTERNAL);

	m_resamplers.push_back (resampler);
      }
    assert_return (!m_resamplers.empty(), Bse::Error::INTERNAL); /* n_channels is always > 0 */
    m_filter_order = m_resamplers[0]->order();

    /* Resampler2::delay() is defined in output samples, but we need to
     * compensate by shifting the input samples to enable seeking, thus the
     * factor 2
     */
    if (mode() == BSE_RESAMPLER2_MODE_UPSAMPLE)
      {
	m_filter_delay = (int) round (m_resamplers[0]->delay());

	// dividing this value may erase half a sample delay (if m_filter_delay is odd)
	// this half sample delay is compensated on the input
	m_filter_delay_input = m_filter_delay % 2;
	m_filter_delay /= 2;
      }
    else
      {
	m_filter_delay = (int) round (m_resamplers[0]->delay() * 2);
	m_filter_delay_input = 0;
      }
    return Bse::Error::NONE;
  }
  void
  close()
  {
    for (guint i = 0; i < m_dhandle.setup.n_channels; i++)
      delete m_resamplers[i];

    m_resamplers.clear();
    m_pcm_data.clear();

    m_dhandle.setup.xinfos = NULL;	/* cleanup pointer reference */
    gsl_data_handle_close (m_src_handle);
  }
  int64
  read (int64  voffset,
	int64  n_values,
	float *values)
  {
    /* for odd upsampler delays, shift the read request 1 sample (in addition
     * to the delay compensation performed in src_read())
     */
    voffset += m_filter_delay_input * m_dhandle.setup.n_channels;

    int64 frame = voffset / m_pcm_data.size();
    if (frame != m_pcm_frame)
      {
	int64 l = read_frame (frame);
	if (l < 0)
	  return l;
      }
    assert_return (m_pcm_frame == frame, 0);

    voffset -= m_pcm_frame * m_frame_size;
    assert_return (voffset >= 0, 0);

    n_values = std::min (n_values, m_frame_size - voffset);
    for (int64 i = 0; i < n_values; i++)
      values[i] = m_pcm_data[voffset + i];

    return n_values;
  }
  int64
  get_state_length() const
  {
    int64 source_state_length = gsl_data_handle_get_state_length (m_src_handle);
    // m_src_handle must be opened and have valid state size
    assert_return (source_state_length >= 0, 0);  

    if (mode() == BSE_RESAMPLER2_MODE_UPSAMPLE)
      source_state_length *= 2;
    else
      source_state_length = (source_state_length + 1) / 2;

    // we must be opened => n_channels > 0, 1 Resampler per Channel
    assert_return (!m_resamplers.empty(), 0);

    /* For fractional delays, a delay of 10.5 for instance means that input[0]
     * affects samples 10 and 11, and thus the state length we assume for
     * that case is 11.
     */
    int64 per_channel_state = ceil (m_resamplers[0]->delay());
    return source_state_length + per_channel_state * m_dhandle.setup.n_channels;
  }
  static GslDataHandle*
  dh_create (DataHandleResample2 *cxx_dh)
  {
    static GslDataHandleFuncs dh_vtable =
    {
      dh_open,
      dh_read,
      dh_close,
      NULL,
      dh_get_state_length,
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
private:
/* for the "C" API (vtable) */
  static DataHandleResample2*
  dh_cast (GslDataHandle *dhandle)
  {
    return static_cast<CDataHandleResample2 *> (dhandle)->cxx_dh;
    //return reinterpret_cast<DataHandleResample2 *> (dhandle);
  }
  static Bse::Error
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
  static int64
  dh_get_state_length (GslDataHandle *dhandle)
  {
    return dh_cast (dhandle)->get_state_length();
  }
};

class DataHandleUpsample2 : public DataHandleResample2
{
public:
  DataHandleUpsample2 (GslDataHandle *src_handle,
                       int            precision_bits) :
    DataHandleResample2 (src_handle, precision_bits)
  {
    if (m_init_ok)
      m_dhandle.name = g_strconcat (m_src_handle->name, "// #upsample2 /", NULL);
  }
  BseResampler2Mode
  mode() const
  {
    return BSE_RESAMPLER2_MODE_UPSAMPLE;
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

	m_resamplers[ch]->process_block (input + ch * n_input_samples, n_input_samples, output);
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

	m_resamplers[ch]->process_block (input + ch * input_per_channel, input_per_channel, output + ch * output_per_channel);
      }
    interleave (output, &m_pcm_data[0], m_frame_size);

    m_pcm_frame = frame;
    return 1;
  }
};

class DataHandleDownsample2 : public DataHandleResample2
{
public:
  DataHandleDownsample2 (GslDataHandle *src_handle,
			 int            precision_bits) :
    DataHandleResample2 (src_handle, precision_bits)
  {
  }
  BseResampler2Mode
  mode() const
  {
    return BSE_RESAMPLER2_MODE_DOWNSAMPLE;
  }
  int64
  prepare_filter_history (int64 frame)
  {
    const int64 n_channels = m_dhandle.setup.n_channels;
    const int64 n_input_samples = m_filter_order * 2;

    float input_interleaved[n_input_samples * n_channels];
    float input[n_input_samples * n_channels];

    int64 l = src_read (frame * m_frame_size * 2 - n_input_samples * n_channels,
	                n_input_samples * n_channels, input_interleaved);
    if (l < 0)
      return l; /* pass on errors */

    deinterleave (input_interleaved, input, n_input_samples * m_dhandle.setup.n_channels);

    for (guint ch = 0; ch < m_dhandle.setup.n_channels; ch++)
      {
	/* we don't need the output, this is just for filling the filter history */
	float output[n_input_samples / 2];

	m_resamplers[ch]->process_block (input + ch * n_input_samples, n_input_samples, output);
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

    float input_interleaved[m_frame_size * 2];
    float input[m_frame_size * 2];
    float output[m_frame_size];

    int64 l = src_read (frame * m_frame_size * 2, m_frame_size * 2, input_interleaved);
    if (l < 0)
      return l; /* pass on errors */

    deinterleave (input_interleaved, input, m_frame_size * 2);
    for (guint ch = 0; ch < m_dhandle.setup.n_channels; ch++)
      {
	const int64 output_per_channel = m_frame_size / m_dhandle.setup.n_channels;
	const int64 input_per_channel = output_per_channel * 2;

	m_resamplers[ch]->process_block (input + ch * input_per_channel, input_per_channel, output + ch * output_per_channel);
      }
    interleave (output, &m_pcm_data[0], m_frame_size);

    m_pcm_frame = frame;
    return 1;
  }
};

} // Bse

using namespace Bse;

GslDataHandle*
bse_data_handle_new_upsample2 (GslDataHandle *src_handle,
                               int            precision_bits)
{
  DataHandleResample2 *cxx_dh = new DataHandleUpsample2 (src_handle, precision_bits);
  return DataHandleResample2::dh_create (cxx_dh);
}

GslDataHandle*
bse_data_handle_new_downsample2 (GslDataHandle *src_handle,
                                 int            precision_bits)
{
  DataHandleResample2 *cxx_dh = new DataHandleDownsample2 (src_handle, precision_bits);
  return DataHandleResample2::dh_create (cxx_dh);
}
