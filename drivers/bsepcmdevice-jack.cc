// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include <bse/bsecxxplugin.hh>
#include "bsepcmdevice-jack.hh"
#include <bse/bseblockutils.hh>
#include <bse/gsldatautils.hh>
#include <jack/jack.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <set>
#include <string>
#include <map>
#include <vector>
#include <chrono>

using std::vector;
using std::set;
using std::map;
using std::string;
using std::min;
using std::max;
using std::copy;
using Bse::Block;

namespace {

/**
 * This function uses std::copy to copy the n_values of data from ivalues
 * to ovalues. If a specialized version is available in bseblockutils,
 * then this - usually faster - version will be used.
 *
 * The data in ivalues and ovalues may not overlap.
 */
template<class Data> void
fast_copy (uint        n_values,
           Data       *ovalues,
	   const Data *ivalues)
{
  copy (ivalues, ivalues + n_values, ovalues);
}

template<> void
fast_copy (uint         n_values,
           float       *ovalues,
           const float *ivalues)
{
  Block::copy (n_values, ovalues, ivalues);
}

template<> void
fast_copy (uint	         n_values,
           uint32       *ovalues,
           const uint32 *ivalues)
{
  Block::copy (n_values, ovalues, ivalues);
}

/**
 * The FrameRingBuffer class implements a ringbuffer for the communication
 * between two threads. One thread - the producer thread - may only write
 * data to the ringbuffer. The other thread - the consumer thread - may
 * only read data from the ringbuffer.
 *
 * Given that these two threads only use the appropriate functions, no
 * other synchronization is required to ensure that the data gets safely
 * from the producer thread to the consumer thread. However, all operations
 * that are provided by the ringbuffer are non-blocking, so that you may
 * need a condition or other synchronization primitive if you want the
 * producer and/or consumer to block if the ringbuffer is full/empty.
 *
 * Implementation: the synchronization between the two threads is only
 * implemented by two index variables (read_frame_pos and write_frame_pos)
 * for which atomic integer reads and writes are required. Since the
 * producer thread only modifies the write_frame_pos and the consumer thread
 * only modifies the read_frame_pos, no compare-and-swap or similar
 * operations are needed to avoid concurrent writes.
 */
template<class T>
class FrameRingBuffer {
  //BIRNET_PRIVATE_COPY (FrameRingBuffer);
private:
  vector<vector<T>>   channel_buffer_;
  std::atomic<int>    atomic_read_frame_pos_;
  std::atomic<int>    atomic_write_frame_pos_;
  uint                channel_buffer_size_;       // = n_frames + 1; the extra frame allows us to
                                                  // see the difference between an empty/full ringbuffer
  uint                n_channels_;
public:
  FrameRingBuffer (uint n_frames = 0,
		   uint n_channels = 1)
  {
    resize (n_frames, n_channels);
  }
  /**
   * @returns the number of frames that are available for reading
   *
   * Check available read space in the ringbuffer.
   * This function may only be called from the consumer thread.
   */
  uint
  get_readable_frames()
  {
    int wpos = atomic_write_frame_pos_;
    int rpos = atomic_read_frame_pos_;

    if (wpos < rpos)		    /* wpos == rpos -> empty ringbuffer */
      wpos += channel_buffer_size_;

    return wpos - rpos;
  }
  /**
   * @returns the number of successfully read frames
   *
   * Read data from the ringbuffer; if there is not enough data
   * in the ringbuffer, the function will return the number of frames
   * that could be read without blocking.
   *
   * This function should be called from the consumer thread.
   */
  uint
  read (uint    n_frames,
        T     **frames)
  {
    int rpos = atomic_read_frame_pos_;
    uint can_read = min (get_readable_frames(), n_frames);

    uint read1 = min (can_read, channel_buffer_size_ - rpos);
    uint read2 = can_read - read1;

    for (uint ch = 0; ch < n_channels_; ch++)
      {
	fast_copy (read1, frames[ch], &channel_buffer_[ch][rpos]);
	fast_copy (read2, frames[ch] + read1, &channel_buffer_[ch][0]);
      }

    atomic_read_frame_pos_ = (rpos + can_read) % channel_buffer_size_;
    return can_read;
  }
  /**
   * @returns the number of frames that can be written
   *
   * Check available write space in the ringbuffer.
   * This function should be called from the producer thread.
   */
  uint
  get_writable_frames()
  {
    int wpos = atomic_write_frame_pos_;
    int rpos = atomic_read_frame_pos_;

    if (rpos <= wpos)		    /* wpos == rpos -> empty ringbuffer */
      rpos += channel_buffer_size_;

    // the extra frame allows us to see the difference between an empty/full ringbuffer
    return rpos - wpos - 1;
  }
  /**
   * @returns the number of successfully written frames
   *
   * Write data to the ringbuffer; if there is not enough free space
   * in the ringbuffer, the function will return the amount of frames
   * consumed by a partial write (without blocking).
   *
   * This function may only be called from the producer thread.
   */
  uint
  write (uint      n_frames,
         const T **frames)
  {
    int wpos = atomic_write_frame_pos_;
    uint can_write = min (get_writable_frames(), n_frames);

    uint write1 = min (can_write, channel_buffer_size_ - wpos);
    uint write2 = can_write - write1;

    for (uint ch = 0; ch < n_channels_; ch++)
      {
	fast_copy (write1, &channel_buffer_[ch][wpos], frames[ch]);
	fast_copy (write2, &channel_buffer_[ch][0], frames[ch] + write1);
      }
 
    // It is important that the data from the previous writes get written
    // to memory *before* the index variable is updated.
    //
    // Writing the C++ atomic variable (position) as last step should ensure
    // correct ordering (also across threads).

    atomic_write_frame_pos_ = (wpos + can_write) % channel_buffer_size_;
    return can_write;
  }
  /**
   * @returns the maximum number of frames that the ringbuffer can contain
   *
   * This function can be called from any thread.
   */
  uint
  get_total_n_frames() const
  {
    // the extra frame allows us to see the difference between an empty/full ringbuffer
    return channel_buffer_size_ - 1;
  }
  /**
   * @returns the number of elements that are part of one frame
   *
   * This function can be called from any thread.
   */
  uint
  get_n_channels() const
  {
    return n_channels_;
  }
  /**
   * Clear the ringbuffer.
   *
   * This function may not be used while either the producer thread or
   * the consumer thread are modifying the ringbuffer.
   */
  void
  clear()
  {
    atomic_read_frame_pos_ = 0;
    atomic_write_frame_pos_ = 0;
  }
  /**
   * Resize and clear the ringbuffer.
   *
   * This function may not be used while either the producer thread or
   * the consumer thread are modifying the ringbuffer.
   */
  void
  resize (uint n_frames,
          uint n_channels = 1)
  {
    n_channels_ = n_channels;
    channel_buffer_.resize (n_channels);

    // the extra frame allows us to see the difference between an empty/full ringbuffer
    channel_buffer_size_ = n_frames + 1;
    for (uint ch = 0; ch < n_channels_; ch++)
      channel_buffer_[ch].resize (channel_buffer_size_);

    clear();
  }
};


#define PDEBUG(...)     Bse::debug ("pcm-jack", __VA_ARGS__)
#define TEST_DROPOUT() if (unlink ("/tmp/drop") == 0) usleep (1.5 * 1000000. * jack->buffer_frames / handle->mix_freq); /* sleep 1.5 * buffer size */

/* --- JACK PCM handle --- */
struct JackPcmHandle
{
  BsePcmHandle  handle;

  vector<jack_port_t *>	  input_ports;
  vector<jack_port_t *>	  output_ports;

  uint			  buffer_frames;	/* input/output ringbuffer size in frames */

  jack_client_t		 *jack_client;
  FrameRingBuffer<float>  input_ringbuffer;
  FrameRingBuffer<float>  output_ringbuffer;

  std::atomic<int>        atomic_active;
  std::atomic<int>        atomic_xruns;
  int                     printed_xruns;

  bool			  is_down;

  uint64                  device_read_counter;
  uint64                  device_write_counter;

  JackPcmHandle (jack_client_t *jack_client) :
    jack_client (jack_client),
    atomic_active (0),
    atomic_xruns (0),
    printed_xruns(0),
    is_down (false),
    device_read_counter (0),
    device_write_counter (0)
  {
    memset (&handle, 0, sizeof (handle));
  }
};

}

/* --- prototypes --- */
static void             bse_pcm_device_jack_class_init  (BsePcmDeviceJACKClass  *klass);
static void             bse_pcm_device_jack_init        (BsePcmDeviceJACK       *self);
static gsize            jack_device_read                (BsePcmHandle           *handle,
                                                         float                  *values);
static void             jack_device_write               (BsePcmHandle           *handle,
                                                         const float            *values);
static gboolean         jack_device_check_io            (BsePcmHandle           *handle,
                                                         glong                  *tiumeoutp);
static void             jack_device_latency             (BsePcmHandle           *handle,
                                                         uint                   *rlatency,
                                                         uint                   *wlatency);

/* --- define object type and export to BSE --- */

BSE_RESIDENT_TYPE_DEF (BsePcmDeviceJACK, bse_pcm_device_jack, BSE_TYPE_PCM_DEVICE, NULL,
                       "PCM driver implementation for JACK Audio Connection Kit "
                                  "(http://jackaudio.org)", NULL);

/* --- variables --- */
static gpointer parent_class = NULL;

/* --- functions --- */

static void
error_callback_silent (const char *msg)
{
  PDEBUG ("JACK: %s\n", msg);
}

static void
error_callback_show (const char *msg)
{
  Bse::printerr ("JACK: %s\n", msg);
}

static void
bse_pcm_device_jack_init (BsePcmDeviceJACK *self)
{
  self->jack_client = nullptr;

  jack_set_error_function (error_callback_show);
}

namespace {

bool
connect_jack (BsePcmDeviceJACK *self, jack_status_t &status)
{
  /* we should only be called if jack_client is not already registered */
  assert_return (!self->jack_client, false);

  /* don't report errors during open: silently use the next available driver if JACK is not there */
  jack_set_error_function (error_callback_silent);

  self->jack_client = jack_client_open ("beast", JackNoStartServer, &status);

  jack_set_error_function (error_callback_show);

  PDEBUG ("attaching to JACK server returned status: %d\n", status);
  return (self->jack_client != nullptr);
}

string
jack_status_to_error_msg (jack_status_t status)
{
  if (status & JackInvalidOption)
    return _("The operation contained an invalid or unsupported option.");
  if (status & JackNameNotUnique)
    return _("The desired client name was not unique.");
  if (status & JackServerFailed)
    return _("Unable to connect to the JACK server.");
  if (status & JackServerError)
    return _("Communication error with the JACK server.");
  if (status & JackNoSuchClient)
    return _("Requested client does not exist.");
  if (status & JackInitFailure)
    return _("Unable to initialize client.");
  if (status & JackShmFailure)
    return _("Unable to access shared memory.");
  if (status & JackVersionError)
    return _("Client's protocol version does not match.");
  if (status & JackBackendError)
    return _("Backent error.");
  if (status & JackClientZombie)
    return _("Client zombified failure.");

  return Rapicorn::string_format (_("Unknown JACK status: 0x%03x"), status);
}

void
disconnect_jack (BsePcmDeviceJACK *self)
{
  assert_return (self->jack_client != NULL);

  jack_deactivate (self->jack_client);
  jack_client_close (self->jack_client);
  self->jack_client = NULL;
}

struct DeviceDetails {
  uint ports, input_ports, output_ports, physical_ports, terminal_ports;
  bool default_device;
  vector<string> input_port_names;
  vector<string> output_port_names;
  DeviceDetails() :
    ports (0),
    input_ports (0),
    output_ports (0),
    physical_ports (0),
    terminal_ports (0),
    default_device (false)
  {
  }
};
}

static map<string, DeviceDetails>
query_jack_devices (BsePcmDeviceJACK *self)
{
  map<string, DeviceDetails> devices;

  assert_return (self->jack_client, devices);

  const char **jack_ports = jack_get_ports (self->jack_client, NULL, NULL, 0);
  if (jack_ports)
    {
      bool have_default_device = false;

      for (uint i = 0; jack_ports[i]; i++)
	{
	  const char *end = strchr (jack_ports[i], ':');
	  if (end)
	    {
	      string device_name (jack_ports[i], end);

	      jack_port_t *jack_port = jack_port_by_name (self->jack_client, jack_ports[i]);
	      if (jack_port)
		{
                  const char *port_type = jack_port_type (jack_port);
                  if (strcmp (port_type, JACK_DEFAULT_AUDIO_TYPE) == 0)
                    {
	              DeviceDetails &details = devices[device_name];
	              details.ports++;

                      int flags = jack_port_flags (jack_port);
                      if (flags & JackPortIsInput)
                        {
                          details.input_ports++;
                          details.input_port_names.push_back (jack_ports[i]);
                        }
                      if (flags & JackPortIsOutput)
                        {
                          details.output_ports++;
                          details.output_port_names.push_back (jack_ports[i]);
                        }
                      if (flags & JackPortIsPhysical)
                        {
                          details.physical_ports++;

                          if (!have_default_device)
                            {
                              /* the first device that has physical ports is the default device */
                              details.default_device = true;
                              have_default_device = true;
                            }
                        }
                      if (flags & JackPortIsTerminal)
                        details.terminal_ports++;
                    }
		}
	    }
	}
      free (jack_ports);
    }

  return devices;
}

static SfiRing*
bse_pcm_device_jack_list_devices (BseDevice *device)
{
  BsePcmDeviceJACK *self = BSE_PCM_DEVICE_JACK (device);

  jack_status_t status;
  char         *jack_error_msg = nullptr;

  map<string, DeviceDetails> devices;
  if (connect_jack (self, status))
    {
      devices = query_jack_devices (self);
      disconnect_jack (self);
    }
  else
    {
      jack_error_msg = g_strdup (jack_status_to_error_msg (status).c_str());
    }

  SfiRing *ring = NULL;
  for (map<string, DeviceDetails>::iterator di = devices.begin(); di != devices.end(); di++)
    {
      const string &device_name = di->first;
      DeviceDetails &details = di->second;
      BseDeviceEntry *entry;
      entry = bse_device_group_entry_new (device, g_strdup (device_name.c_str()),
                                                  g_strdup ("JACK Devices"),
                                                  g_strdup_printf ("%s (%d*input %d*output)%s",
								   device_name.c_str(),
								   details.input_ports,
								   details.output_ports,
								   details.physical_ports == details.ports ?
								     " [ hardware ]" : ""));

      /* ensure that the default device always is the first item listed */
      if (details.default_device)
	ring = sfi_ring_prepend (ring, entry);
      else
	ring = sfi_ring_append (ring, entry);
    }

  if (!ring)
    ring = sfi_ring_append (ring, bse_device_error_new (device, jack_error_msg ? jack_error_msg : g_strdup_printf ("No devices found")));
  return ring;
}

static void
jack_shutdown_callback (void *jack_handle_ptr)
{
  JackPcmHandle *jack = static_cast <JackPcmHandle *> (jack_handle_ptr);

  jack->is_down = true;
}

static int
jack_process_callback (jack_nframes_t n_frames,
                       void          *jack_handle_ptr)
{
  JackPcmHandle *jack = static_cast <JackPcmHandle *> (jack_handle_ptr);

  /* setup port pointers */
  uint n_channels = jack->handle.n_channels;

  assert_return (jack->input_ports.size() == n_channels, 0);
  assert_return (jack->output_ports.size() == n_channels, 0);

  const float *in_values[n_channels];
  float *out_values[n_channels];
  for (uint ch = 0; ch < n_channels; ch++)
    {
      in_values[ch] = (float *) jack_port_get_buffer (jack->input_ports[ch], n_frames);
      out_values[ch] = (float *) jack_port_get_buffer (jack->output_ports[ch], n_frames);
    }

  if (!jack->atomic_active)
    {
      for (auto values : out_values)
        Block::fill (n_frames, values, 0.0);
    }
  else if (jack->input_ringbuffer.get_writable_frames() >= n_frames && jack->output_ringbuffer.get_readable_frames() >= n_frames)
    {
      /* handle input ports */
      uint frames_written = jack->input_ringbuffer.write (n_frames, in_values);
      assert_return (frames_written == n_frames, 0); // we checked the available space before

      /* handle output ports */
      uint read_frames = jack->output_ringbuffer.read (n_frames, out_values);
      assert_return (read_frames == n_frames, 0); // we checked the available space before
    }
  else
    {
      /* underrun (less than n_frames available in input/output ringbuffer) -> write zeros */
      jack->atomic_xruns++;

      for (auto values : out_values)
        Block::fill (n_frames, values, 0.0);
    }
  return 0;
}

static jack_latency_range_t
get_latency_for_ports (const vector<jack_port_t *>& ports,
                       jack_latency_callback_mode_t mode)
{
  jack_latency_range_t range;

  // compute minimum possible and maximum possible latency over all ports
  for (size_t p = 0; p < ports.size(); p++)
    {
      jack_latency_range_t port_range;

      jack_port_get_latency_range (ports[p], mode, &port_range);

      if (!p) // first port
        range = port_range;
      else
        {
          range.min = min (range.min, port_range.min);
          range.max = max (range.max, port_range.max);
        }
    }
  return range;
}

static void
jack_latency_callback (jack_latency_callback_mode_t  mode,
                       void                         *jack_handle_ptr)
{
  JackPcmHandle *jack = static_cast <JackPcmHandle *> (jack_handle_ptr);

  // the capture/playback latency added is the number of samples in the ringbuffer
  if (mode == JackCaptureLatency)
    {
      jack_latency_range_t range = get_latency_for_ports (jack->input_ports, mode);
      range.min += jack->buffer_frames;
      range.max += jack->buffer_frames;

      for (auto port : jack->output_ports)
        jack_port_set_latency_range (port, mode, &range);
    }
  else
    {
      jack_latency_range_t range = get_latency_for_ports (jack->output_ports, mode);
      range.min += jack->buffer_frames;
      range.max += jack->buffer_frames;

      for (auto port : jack->input_ports)
        jack_port_set_latency_range (port, mode, &range);
    }
}

static Bse::Error
bse_pcm_device_jack_open (BseDevice     *device,
                          gboolean       require_readable,
                          gboolean       require_writable,
                          uint           n_args,
                          const char   **args)
{
  BsePcmDeviceJACK *self = BSE_PCM_DEVICE_JACK (device);

  jack_status_t status;
  if (!connect_jack (self, status))
    return Bse::Error::FILE_OPEN_FAILED;

  JackPcmHandle *jack = new JackPcmHandle (self->jack_client);
  BsePcmHandle *handle = &jack->handle;

  // always use duplex mode for this device
  handle->readable = TRUE;
  handle->writable = TRUE;
  handle->n_channels = BSE_PCM_DEVICE (device)->req_n_channels;

  /* try setup */
  Bse::Error error = Bse::Error::NONE;

  const char *dname = (n_args == 1) ? args[0] : "alsa_pcm";
  handle->mix_freq = jack_get_sample_rate (self->jack_client);

  for (uint i = 0; i < handle->n_channels; i++)
    {
      const int port_name_size = jack_port_name_size();
      char port_name[port_name_size];
      jack_port_t *port;

      snprintf (port_name, port_name_size, "in_%u", i);
      port = jack_port_register (self->jack_client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
      if (port)
	jack->input_ports.push_back (port);
      else
	error = Bse::Error::FILE_OPEN_FAILED;

      snprintf (port_name, port_name_size, "out_%u", i);
      port = jack_port_register (self->jack_client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
      if (port)
	jack->output_ports.push_back (port);
      else
	error = Bse::Error::FILE_OPEN_FAILED;
    }

  /* initialize ring buffers */
  if (error == 0)
    {
      // keep at least two jack callback sizes for dropout free audio
      uint min_buffer_frames = jack_get_buffer_size (self->jack_client) * 2;

      // keep an extra engine buffer size (this compensates also for cases where the
      // engine buffer size is not a 2^N value, which would otherwise cause the
      // buffer never to be fully filled with 2 periods of data)
      min_buffer_frames += BSE_PCM_DEVICE (device)->req_block_length;

      // honor the user defined latency specification
      //
      // the user defined latency is only used to adjust our local buffering
      // -> it doesn't take into account latencies outside beast, such as the buffering
      //    jack does, or latencies added by other clients)
      uint user_buffer_frames = BSE_PCM_DEVICE (device)->req_latency_ms * handle->mix_freq / 1000;
      uint buffer_frames = max (min_buffer_frames, user_buffer_frames);

      jack->input_ringbuffer.resize (buffer_frames, handle->n_channels);
      jack->output_ringbuffer.resize (buffer_frames, handle->n_channels);
      jack->buffer_frames  = jack->output_ringbuffer.get_writable_frames();

      // the ringbuffer should be exactly as big as requested
      if (jack->buffer_frames != buffer_frames)
        {
          Bse::warning ("JACK driver: ring buffer size not correct: (jack->buffer_frames != buffer_frames)\n");
          error = Bse::Error::INTERNAL;
        }
      PDEBUG ("ringbuffer size = %.3fms", jack->buffer_frames / double (handle->mix_freq) * 1000);

      /* initialize output ringbuffer with silence
       * this will prevent dropouts at initialization, when no data is there at all
       */
      vector<float>	  silence (jack->output_ringbuffer.get_total_n_frames());
      vector<const float *> silence_buffers (jack->output_ringbuffer.get_n_channels());

      fill (silence_buffers.begin(), silence_buffers.end(), &silence[0]);

      uint frames_written = jack->output_ringbuffer.write (jack->buffer_frames, &silence_buffers[0]);
      if (frames_written != jack->buffer_frames)
        Bse::warning ("JACK driver: output silence init failed: (frames_written != jack->buffer_frames)\n");

    }

  /* activate */
  if (error == 0)
    {
      jack_set_process_callback (self->jack_client, jack_process_callback, jack);
      jack_set_latency_callback (self->jack_client, jack_latency_callback, jack);
      jack_on_shutdown (self->jack_client, jack_shutdown_callback, jack);

      if (jack_activate (self->jack_client) != 0)
	error = Bse::Error::FILE_OPEN_FAILED;
    }

  /* connect ports (only if autoconnect is enabled) */
  if (error == 0 && BSE_GCONFIG (jack_autoconnect))
    {
      map<string, DeviceDetails> devices = query_jack_devices (self);
      map<string, DeviceDetails>::const_iterator di;

      di = devices.find (dname);
      if (di != devices.end())
	{
	  const DeviceDetails &details = di->second;

	  for (uint ch = 0; ch < handle->n_channels; ch++)
	    {
	      if (details.output_ports > ch)
		jack_connect (self->jack_client, details.output_port_names[ch].c_str(),
		                                 jack_port_name (jack->input_ports[ch]));
	      if (details.input_ports > ch)
		jack_connect (self->jack_client, jack_port_name (jack->output_ports[ch]),
		                                 details.input_port_names[ch].c_str());
	    }
	}
    }

  /* setup PCM handle or shutdown */
  if (error == 0)
    {
      bse_device_set_opened (device, dname, handle->readable, handle->writable);
      if (handle->readable)
        handle->read = jack_device_read;
      if (handle->writable)
        handle->write = jack_device_write;
      handle->check_io = jack_device_check_io;
      handle->latency = jack_device_latency;
      BSE_PCM_DEVICE (device)->handle = handle;

      guint dummy;
      jack_device_latency (handle, &dummy, &dummy);   // debugging only: print latency values
    }
  else
    {
      disconnect_jack (self);
      delete jack;
    }
  PDEBUG ("JACK: opening PCM \"%s\" readupble=%d writable=%d: %s", dname, handle->readable, handle->writable, bse_error_blurb (error));

  return error;
}

static void
bse_pcm_device_jack_close (BseDevice *device)
{
  BsePcmDeviceJACK *self = BSE_PCM_DEVICE_JACK (device);

  JackPcmHandle *jack = (JackPcmHandle*) BSE_PCM_DEVICE (device)->handle;
  BSE_PCM_DEVICE (device)->handle = NULL;

  disconnect_jack (self);
  delete jack;
}

static void
bse_pcm_device_jack_finalize (GObject *object)
{
  // BsePcmDeviceJACK *self = BSE_PCM_DEVICE_JACK (object);

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
jack_device_check_io (BsePcmHandle *handle,
                      glong        *timeoutp)
{
  JackPcmHandle *jack = (JackPcmHandle*) handle;
  assert_return (jack->jack_client != NULL, false);

  /* enable processing in callback (if not already active) */
  jack->atomic_active = 1;

  /* report jack driver xruns */
  if (jack->atomic_xruns != jack->printed_xruns)
    {
      jack->printed_xruns = jack->atomic_xruns;
      Bse::printerr ("%d beast jack driver xruns\n", jack->printed_xruns);
    }

  uint n_frames_avail = min (jack->output_ringbuffer.get_writable_frames(), jack->input_ringbuffer.get_readable_frames());

  /* check whether data can be processed */
  if (n_frames_avail >= handle->block_length)
    return TRUE;        /* need processing */

  /* calculate timeout until processing is possible or needed */
  uint diff_frames = handle->block_length - n_frames_avail;
  *timeoutp = diff_frames * 1000 / handle->mix_freq;

  /* wait at least 1ms, because caller may interpret (timeout == 0) as "process now" */
  *timeoutp = max<int> (*timeoutp, 1);
  return FALSE;
}

static void
jack_device_latency (BsePcmHandle *handle,
                     uint        *rlatency,
                     uint        *wlatency)
{
  JackPcmHandle *jack = (JackPcmHandle*) handle;
  jack_nframes_t jack_rlatency = 0, jack_wlatency = 0;

  assert_return (jack->jack_client != NULL);

  for (uint i = 0; i < jack->input_ports.size(); i++)
    {
      jack_latency_range_t in_lrange;
      jack_port_get_latency_range (jack->input_ports[i], JackCaptureLatency, &in_lrange);

      jack_rlatency = max (jack_rlatency, in_lrange.max);
    }

  for (uint i = 0; i < jack->output_ports.size(); i++)
    {
      jack_latency_range_t out_lrange;
      jack_port_get_latency_range (jack->output_ports[i], JackPlaybackLatency, &out_lrange);

      jack_wlatency = max (jack_wlatency, out_lrange.max);
    }
  
  uint total_latency = jack->buffer_frames + jack_rlatency + jack_wlatency;
  PDEBUG ("jack_rlatency=%.3f ms jack_wlatency=%.3f ms ringbuffer=%.3f ms total_latency=%.3f ms",
          jack_rlatency / double (handle->mix_freq) * 1000,
          jack_wlatency / double (handle->mix_freq) * 1000,
          jack->buffer_frames / double (handle->mix_freq) * 1000,
          total_latency / double (handle->mix_freq) * 1000);

  // ring buffer is normally completely filled
  //  -> the buffer latency counts as additional write latency

  *rlatency = jack_rlatency;
  *wlatency = jack_wlatency + jack->buffer_frames;
}

static gsize
jack_device_read (BsePcmHandle *handle,
                  float        *values)
{
  JackPcmHandle *jack = (JackPcmHandle*) handle;
  assert_return (jack->jack_client != NULL, 0);

  jack->device_read_counter++;  // read must always gets called before write (see jack_device_write)

  float deinterleaved_frame_data[handle->block_length * handle->n_channels];
  float *deinterleaved_frames[handle->n_channels];
  for (uint ch = 0; ch < handle->n_channels; ch++)
    deinterleaved_frames[ch] = &deinterleaved_frame_data[ch * handle->block_length];

  // in check_io, we already ensured that there is enough data in the input_ringbuffer

  uint frames_read = jack->input_ringbuffer.read (handle->block_length, deinterleaved_frames);
  assert_return (frames_read == handle->block_length, 0);

  for (uint ch = 0; ch < handle->n_channels; ch++)
    {
      const float *src = deinterleaved_frames[ch];
      float *dest = &values[ch];

      for (uint i = 0; i < frames_read; i++)
        {
          *dest = src[i];
          dest += handle->n_channels;
        }
    }
  return handle->block_length * handle->n_channels;
}

static void
jack_device_write (BsePcmHandle *handle,
                   const float  *values)
{
  JackPcmHandle *jack = (JackPcmHandle*) handle;
  assert_return (jack->jack_client != NULL);

  /* our buffer management is based on the assumption that jack_device_read()
   * will always be performed before jack_device_write() - BEAST doesn't
   * always guarantee this (for instance when removing the pcm input module
   * from the snet while audio is playing), so we read and discard input
   * if BEAST didn't call jack_device_read() already
   */
  jack->device_write_counter++;
  if (jack->device_read_counter < jack->device_write_counter)
    {
      float junk_frames[handle->block_length * handle->n_channels];
      jack_device_read (handle, junk_frames);
      assert_return (jack->device_read_counter == jack->device_write_counter);
    }

  // deinterleave
  float deinterleaved_frame_data[handle->block_length * handle->n_channels];
  const float *deinterleaved_frames[handle->n_channels];
  for (uint ch = 0; ch < handle->n_channels; ch++)
    {
      float *channel_data = &deinterleaved_frame_data[ch * handle->block_length];
      for (uint i = 0; i < handle->block_length; i++)
        channel_data[i] = values[ch + i * handle->n_channels];
      deinterleaved_frames[ch] = channel_data;
    }

  // in check_io, we already ensured that there is enough space in the output_ringbuffer

  uint frames_written = jack->output_ringbuffer.write (handle->block_length, deinterleaved_frames);
  assert_return (frames_written == handle->block_length);
}

static void
bse_pcm_device_jack_class_init (BsePcmDeviceJACKClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseDeviceClass *device_class = BSE_DEVICE_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);
  
  gobject_class->finalize = bse_pcm_device_jack_finalize;
  
  device_class->list_devices = bse_pcm_device_jack_list_devices;
  const char *name = "jack";
  const char *syntax = _("device");
  const char *info = /* TRANSLATORS: keep this text to 70 chars in width */
                     _("JACK Audio Connection Kit PCM driver (http://jackaudio.org)\n");

  /* Being BSE_RATING_PREFFERED + 1, should do the right thing: if jackd is running,
   * use it, if jackd isn't running (we'll get an error upon initialization,
   * then), use the next best driver.
   *
   * Right now we will never start the jackd process ourselves.
   */
  bse_device_class_setup (klass, BSE_RATING_PREFERRED + 1, name, syntax, info);
  device_class->open = bse_pcm_device_jack_open;
  device_class->close = bse_pcm_device_jack_close;
}
