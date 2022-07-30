// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "driver.hh"
#include "bseengine.hh"
#include "internal.hh"
#include "gsldatautils.hh"
#include "bseblockutils.hh"
#include "bsemididecoder.hh"

#include <unistd.h>

#define JDEBUG(...)     Bse::debug ("jack", __VA_ARGS__)

#if __has_include(<jack/jack.h>)
#include <jack/jack.h>
#include <jack/midiport.h>

#define MAX_JACK_STRING_SIZE    1024

using namespace Bse;

namespace { // Anon

/*------------------------------------------------------------------------
The jack driver as provided here should be usable. However, there are a few
things that could be improved, here is a short list.

Audio Engine start/stop
-----------------------
Currently the JACK driver registers a new BEAST client every time the device
is opened. This is problematic because connections between the BEAST jack
client and other applications will be disconnected on every close. So
redirecting output song playback through some other application would have
to be reconnected the next time the song plays. Also connecting BEAST to
other JACK clients while the device is closed - before actual playback -
would be impossible.

To fix this, there should be an explicit audio engine start/stop in BEAST.
Once the audio engine is started, the JACK client should remain registered
with JACK, even if no song is playing. This should fix the problems this
driver has due to JACK disconnect on device close.

Less buffering, better latency
------------------------------
Currently, the JACK driver has a ring buffer that holds some audio data.  This
introduces latency. This is not what JACK applications typically do.  So here
are some thoughts of how to avoid buffering completely.

To do so, we make the JACK callback block until BEAST has processed the audio
data.

(1) [JACK Thread] jack_process_callback gets called with N input samples
(2) [JACK Thread] wake up engine thread
(3) [JACK Thread] wait for engine thread
(4) [ENGINE Thread] engine thread processes N samples input -> N samples output
(5) [ENGINE Thread] engine thread wakes up jack thread
(6) [JACK Thread] jack_process_callback returns, supplying N output samples

So the idea is to receive input samples from jack (1) and then stall the jack
thread (2). The engine processes the samples (4) and wakes up the jack thread
(5) which then returns (6) the samples the engine created. The engine thread
can use helper threads to complete the job.

Note that this optimization works best (no buffering/latency) if the engine
block size and the jack block size are equal. It also works well if the jack
block size is an integer multiple fo the engine block size.

This avoids latency and buffering. However this may pose stricter RT
requirements onto BEAST. So whether it runs as dropout-free as the current
version would remain to be seen. A not so realtime version would buffer
M complete blocks of N samples, still avoiding partially filled buffers.
------------------------------------------------------------------------*/

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
  std::copy (ivalues, ivalues + n_values, ovalues);
}

template<> void
fast_copy (uint         n_values,
           float       *ovalues,
           const float *ivalues)
{
  Block::copy (n_values, ovalues, ivalues);
}

template<> BSE_UNUSED void
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
  vector<vector<T> >  channel_buffer_;
  std::atomic<int>    atomic_read_frame_pos_ {0};
  std::atomic<int>    atomic_write_frame_pos_ {0};
  uint                channel_buffer_size_ = 0;       // = n_frames + 1; the extra frame allows us to
                                                      // see the difference between an empty/full ringbuffer
  uint                n_channels_ = 0;
public:
  FrameRingBuffer (uint n_frames = 0,
		   uint n_channels = 1)
  {
    resize (n_frames, n_channels);
  }
  /**
   * Check available read space in the ringbuffer.
   * This function may only be called from the consumer thread.
   *
   * @returns the number of frames that are available for reading
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
   * Read data from the ringbuffer; if there is not enough data
   * in the ringbuffer, the function will return the number of frames
   * that could be read without blocking.
   *
   * This function should be called from the consumer thread.
   *
   * @returns the number of successfully read frames
   */
  uint
  read (uint    n_frames,
        T     **frames)
  {
    int rpos = atomic_read_frame_pos_;
    uint can_read = std::min (get_readable_frames(), n_frames);

    uint read1 = std::min (can_read, channel_buffer_size_ - rpos);
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
   * Check available write space in the ringbuffer.
   * This function should be called from the producer thread.
   *
   * @returns the number of frames that can be written
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
   * Write data to the ringbuffer; if there is not enough free space
   * in the ringbuffer, the function will return the amount of frames
   * consumed by a partial write (without blocking).
   *
   * This function may only be called from the producer thread.
   *
   * @returns the number of successfully written frames
   */
  uint
  write (uint      n_frames,
         const T **frames)
  {
    int wpos = atomic_write_frame_pos_;
    uint can_write = std::min (get_writable_frames(), n_frames);

    uint write1 = std::min (can_write, channel_buffer_size_ - wpos);
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
   * Get total size of the ringbuffer.
   * This function can be called from any thread.
   *
   * @returns the maximum number of frames that the ringbuffer can contain
   */
  uint
  get_total_n_frames() const
  {
    // the extra frame allows us to see the difference between an empty/full ringbuffer
    return channel_buffer_size_ - 1;
  }
  /**
   * Get number of channels.
   * This function can be called from any thread.
   *
   * @returns the number of elements that are part of one frame
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

/* special case API in case FrameRingBuffer n_channels == 1 */
template<class T>
class RingBuffer
{
  FrameRingBuffer<T> rb;
public:
  RingBuffer (uint n_items = 0) :
    rb (n_items)
  {
  }
  uint
  get_readable_items()
  {
    return rb.get_readable_frames();
  }
  uint
  read (uint  n_items,
        T    *items)
  {
    T *i_ptr[1] = {items};
    return rb.read (n_items, i_ptr);
  }
  uint
  get_writable_items()
  {
    return rb.get_writable_frames();
  }
  uint
  write (uint     n_items,
         const T *items)
  {
    const T *i_ptr[1] = {items};
    return rb.write (n_items, i_ptr);
  }
  void
  clear()
  {
    rb.clear();
  }
  void
  resize (uint n_items)
  {
    rb.resize (n_items);
  }
};

static void
error_callback_silent (const char *msg)
{
  JDEBUG ("%s\n", msg);
}

static void
error_callback_show (const char *msg)
{
  Bse::printerr ("JACK: %s\n", msg);
}

static jack_client_t *
connect_jack()
{
  /* don't report errors during open: silently use the next available driver if JACK is not there */
  jack_set_error_function (error_callback_silent);

  jack_status_t status;
  jack_client_t *jack_client = jack_client_open ("Beast", JackNoStartServer, &status);

  jack_set_error_function (error_callback_show);

  JDEBUG ("attaching to server returned status: %d\n", status);
  return jack_client;
}

static void
disconnect_jack (jack_client_t *jack_client)
{
  assert_return (jack_client != NULL);

  jack_deactivate (jack_client);
  jack_client_close (jack_client);
}

struct DeviceDetails {
  uint ports = 0;
  uint input_ports = 0;
  uint output_ports = 0;
  uint physical_ports = 0;
  uint terminal_ports = 0;
  bool default_device = false;

  std::vector<std::string> input_port_names;
  std::vector<std::string> output_port_names;
  std::string input_port_alias;
};

static std::map<std::string, DeviceDetails>
query_jack_devices (jack_client_t *jack_client)
{
  std::map<std::string, DeviceDetails> devices;

  assert_return (jack_client, devices);
  assert_return (MAX_JACK_STRING_SIZE >= jack_port_name_size(), devices);

  const char **jack_ports = jack_get_ports (jack_client, NULL, NULL, 0);
  if (jack_ports)
    {
      bool have_default_device = false;

      for (uint i = 0; jack_ports[i]; i++)
	{
          jack_port_t *jack_port = jack_port_by_name (jack_client, jack_ports[i]);
	  const char *end = strchr (jack_ports[i], ':');
	  if (!jack_port || !end)
            continue;
          std::string device_name (jack_ports[i], end);

          const char *port_type = jack_port_type (jack_port);
          if (strcmp (port_type, JACK_DEFAULT_AUDIO_TYPE) == 0)
            {
              DeviceDetails &details = devices[device_name];
              details.ports++;

              const int flags = jack_port_flags (jack_port);
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
              if (flags & JackPortIsTerminal)
                details.terminal_ports++;
              if (flags & JackPortIsPhysical)
                {
                  details.physical_ports++;

                  if (!have_default_device && (flags & JackPortIsInput))
                    {
                      /* the first device that has physical ports is the default device */
                      details.default_device = true;
                      have_default_device = true;
                      char alias1[MAX_JACK_STRING_SIZE] = "", alias2[MAX_JACK_STRING_SIZE] = "";
                      char *aliases[2] = { alias1, alias2, };
                      const int cnt = jack_port_get_aliases (jack_port, aliases);
                      if (cnt >= 1 && alias1[0])
                        {
                          const char *acolon = strrchr (alias1, ':');
                          details.input_port_alias = acolon ? std::string (alias1, acolon - alias1) : alias1;
                        }
                    }
                }
	    }
	}
      free (jack_ports);
    }

  return devices;
}

// special audio / midi driver devid which doesn't auto connect to a physical device
static const String no_auto_connect_devid = "no-auto-connect";

static void
list_jack_drivers (Driver::EntryVec &entries)
{
  map<std::string, DeviceDetails> devices;
  jack_client_t *jack_client = connect_jack();
  if (jack_client)
    {
      devices = query_jack_devices (jack_client);
      disconnect_jack (jack_client);
    }

  for (std::map<std::string, DeviceDetails>::iterator di = devices.begin(); di != devices.end(); di++)
    {
      const std::string &devid = di->first;
      DeviceDetails &details = di->second;

      /* the default device is usually the hardware device, so things should work as expected
       * we could show try to show non-default devices as well, but this could be confusing
       */
      if (details.default_device && (details.input_ports || details.output_ports))
        {
          Driver::Entry entry;
          entry.devid = devid;
          entry.device_name = string_format ("JACK \"%s\" Audio Device", devid);
          const std::string phprefix = details.physical_ports == details.ports ? "Physical: " : "";
          if (!details.input_port_alias.empty())
            entry.device_name += " [" + phprefix + details.input_port_alias + "]";
          entry.capabilities = details.output_ports && details.input_ports ? "Full-Duplex Audio" : details.output_ports ? "Audio Input" : "Audio Output";
          entry.capabilities += string_format (", channels: %d*playback + %d*capture", details.input_ports, details.output_ports);
          entry.device_info = "Routing via the JACK Audio Connection Kit";
          if (details.physical_ports == details.ports)
            entry.notice = "Note: JACK driver adds latency compared to direct hardware access";
          entry.priority = Driver::JACK;
          entries.push_back (entry);

          // add device entry which doesn't auto connect to hardware device
          entry.devid = no_auto_connect_devid;
          entry.device_name = string_format ("JACK \"%s\" Audio Device", entry.devid);
          entry.capabilities = "Full-Duplex Audio";
          entry.priority = Driver::JACK + 1;
          entries.push_back (entry);
        }
    }
}

} // Anon

namespace Bse {

/* macro for jack dropout tests - see below */
#define TEST_DROPOUT() if (unlink ("/tmp/bse-dropout") == 0) usleep (1.5 * 1000000. * buffer_frames_ / mix_freq_); /* sleep 1.5 * buffer size */

class JackPcmDriver;
static JackPcmDriver *jack_pcm_driver = nullptr; // allow jack midi driver to communicate with jack audio driver

// == JackPcmDriver ==
class JackPcmDriver : public PcmDriver {
  jack_client_t                *jack_client_ = nullptr;
  uint                          n_channels_ = 0;
  uint                          mix_freq_ = 0;
  std::vector<jack_port_t *>    input_ports_;
  std::vector<jack_port_t *>    output_ports_;
  FrameRingBuffer<float>        input_ringbuffer_;
  FrameRingBuffer<float>        output_ringbuffer_;
  uint                          buffer_frames_ = 0;        /* input/output ringbuffer size in frames */
  uint                          block_length_ = 0;

  std::atomic<int>              atomic_active_ {0};
  std::atomic<int>              atomic_xruns_ {0};
  int                           printed_xruns_ = 0;

  bool                          is_down_ = false;
  bool                          printed_is_down_ = false;

  uint64                        device_read_counter_ = 0;
  uint64                        device_write_counter_ = 0;
  int                           device_open_counter_ = 0;

  /* midi stuff */
  using MidiDriverCallback = std::function<void(uint, uint8*)>;
  struct MidiLink {
    uint                        id = 0;
    jack_port_t                *jack_port = nullptr;
    RingBuffer<uint8>           ring_buffer { 8000 };
    MidiDriverCallback          callback;
  };
  vector<std::unique_ptr<MidiLink>> midi_links;

  int
  process_callback (jack_nframes_t n_frames)
  {
    /* setup port pointers */
    assert_return (input_ports_.size() == n_channels_, 0);
    assert_return (output_ports_.size() == n_channels_, 0);

    const float *in_values[n_channels_];
    float *out_values[n_channels_];
    for (uint ch = 0; ch < n_channels_; ch++)
      {
        in_values[ch] = (float *) jack_port_get_buffer (input_ports_[ch], n_frames);
        out_values[ch] = (float *) jack_port_get_buffer (output_ports_[ch], n_frames);
      }

    if (!atomic_active_)
      {
        for (auto values : out_values)
          Block::fill (n_frames, values, 0.0);
      }
    else if (input_ringbuffer_.get_writable_frames() >= n_frames && output_ringbuffer_.get_readable_frames() >= n_frames)
      {
        /* handle input ports */
        uint frames_written = input_ringbuffer_.write (n_frames, in_values);
        assert_return (frames_written == n_frames, 0); // we checked the available space before

        /* handle output ports */
        uint read_frames = output_ringbuffer_.read (n_frames, out_values);
        assert_return (read_frames == n_frames, 0); // we checked the available space before
      }
    else
      {
        /* underrun (less than n_frames available in input/output ringbuffer) -> write zeros */
        atomic_xruns_++;

        for (auto values : out_values)
          Block::fill (n_frames, values, 0.0);
      }

    if (atomic_active_)
      {
        for (auto& midi_link_ptr : midi_links)
          {
            /* handle midi input */
            void *port_buf = jack_port_get_buffer (midi_link_ptr->jack_port, n_frames);
            jack_nframes_t event_count = jack_midi_get_event_count (port_buf);
            for (jack_nframes_t event_index = 0; event_index < event_count; event_index++)
              {
                jack_midi_event_t    in_event;
                jack_midi_event_get (&in_event, port_buf, event_index);
                if (midi_link_ptr->ring_buffer.get_writable_items() >= in_event.size)  // write whole event or nothing
                  midi_link_ptr->ring_buffer.write (in_event.size, in_event.buffer);
              }
          }
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
            range.min = std::min (range.min, port_range.min);
            range.max = std::max (range.max, port_range.max);
          }
      }
    return range;
  }
  void
  latency_callback (jack_latency_callback_mode_t mode)
  {
    // the capture/playback latency added is the number of samples in the ringbuffer
    if (mode == JackCaptureLatency)
      {
        jack_latency_range_t range = get_latency_for_ports (input_ports_, mode);
        range.min += buffer_frames_;
        range.max += buffer_frames_;

        for (auto port : output_ports_)
          jack_port_set_latency_range (port, mode, &range);
      }
    else
      {
        jack_latency_range_t range = get_latency_for_ports (output_ports_, mode);
        range.min += buffer_frames_;
        range.max += buffer_frames_;

        for (auto port : input_ports_)
          jack_port_set_latency_range (port, mode, &range);
      }
  }
  void
  shutdown_callback()
  {
    is_down_ = true;
  }
public:
  explicit      JackPcmDriver (const String &devid) : PcmDriver (devid) {}
  static PcmDriverP
  create (const String &devid)
  {
    auto pdriverp = std::make_shared<JackPcmDriver> (devid);
    return pdriverp;
  }
  ~JackPcmDriver()
  {
    if (jack_client_)
      close();
  }
  virtual float
  pcm_frequency () const override
  {
    return mix_freq_;
  }
  virtual uint
  block_length () const override
  {
    return block_length_;
  }
  virtual void
  close () override
  {
    assert_return (opened());
    jack_pcm_driver = nullptr;
    disconnect_jack (jack_client_);
    jack_client_ = nullptr;
  }
  virtual Error
  open (IODir iodir, const PcmDriverConfig &config) override
  {
    assert_return (!opened(), Error::INTERNAL);
    assert_return (!jack_client_, Error::INTERNAL);
    assert_return (device_open_counter_++ == 0, Error::INTERNAL);    // calling open more than once is not supported

    jack_client_ = connect_jack();
    if (!jack_client_)
      return Bse::Error::FILE_OPEN_FAILED;

    // always use duplex mode for this device
    flags_ |= Flags::READABLE | Flags::WRITABLE;
    n_channels_ = config.n_channels;

    /* try setup */
    Bse::Error error = Bse::Error::NONE;

    mix_freq_ = jack_get_sample_rate (jack_client_);
    block_length_ = config.block_length;

    for (uint i = 0; i < n_channels_; i++)
      {
        const int port_name_size = jack_port_name_size();
        char port_name[port_name_size];
        jack_port_t *port;

        snprintf (port_name, port_name_size, "in_%u", i + 1);
        port = jack_port_register (jack_client_, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
        if (port)
          input_ports_.push_back (port);
        else
          error = Bse::Error::FILE_OPEN_FAILED;

        snprintf (port_name, port_name_size, "out_%u", i + 1);
        port = jack_port_register (jack_client_, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        if (port)
          output_ports_.push_back (port);
        else
          error = Bse::Error::FILE_OPEN_FAILED;
      }

    /* initialize ring buffers */
    if (error == 0)
      {
        // keep at least two jack callback sizes for dropout free audio
        uint min_buffer_frames = jack_get_buffer_size (jack_client_) * 2;

        // keep an extra engine buffer size (this compensates also for cases where the
        // engine buffer size is not a 2^N value, which would otherwise cause the
        // buffer never to be fully filled with 2 periods of data)
        min_buffer_frames += config.block_length;

        // honor the user defined latency specification
        //
        // the user defined latency is only used to adjust our local buffering
        // -> it doesn't take into account latencies outside beast, such as the buffering
        //    jack does, or latencies added by other clients)
        uint user_buffer_frames = config.latency_ms * config.mix_freq / 1000;
        uint buffer_frames = std::max (min_buffer_frames, user_buffer_frames);

        input_ringbuffer_.resize (buffer_frames, n_channels_);
        output_ringbuffer_.resize (buffer_frames, n_channels_);
        buffer_frames_  = output_ringbuffer_.get_writable_frames();

        // the ringbuffer should be exactly as big as requested
        if (buffer_frames_ != buffer_frames)
          {
            Bse::warning ("JACK driver: ring buffer size not correct: (buffer_frames_ != buffer_frames); (%u != %u)\n",
                          buffer_frames_, buffer_frames);
            error = Bse::Error::INTERNAL;
          }
        JDEBUG ("%s: ringbuffer size = %.3fms", devid_, buffer_frames_ / double (mix_freq_) * 1000);

        /* initialize output ringbuffer with silence
         * this will prevent dropouts at initialization, when no data is there at all
         */
        vector<float>	  silence (output_ringbuffer_.get_total_n_frames());
        vector<const float *> silence_buffers (output_ringbuffer_.get_n_channels());

        fill (silence_buffers.begin(), silence_buffers.end(), &silence[0]);

        uint frames_written = output_ringbuffer_.write (buffer_frames, &silence_buffers[0]);
        if (frames_written != buffer_frames)
          Bse::warning ("JACK driver: output silence init failed: (frames_written != jack->buffer_frames)\n");

      }

    /* activate */
    if (error == 0)
      {
        jack_set_process_callback (jack_client_,
          [] (jack_nframes_t n_frames, void *p) { return static_cast <JackPcmDriver *> (p)->process_callback (n_frames); }, this);

        jack_set_latency_callback (jack_client_,
          [] (jack_latency_callback_mode_t mode, void *p) { static_cast <JackPcmDriver *> (p)->latency_callback (mode); }, this);

        jack_on_shutdown (jack_client_,
          [] (void *p) { static_cast<JackPcmDriver *> (p)->shutdown_callback(); }, this);

        if (jack_activate (jack_client_) != 0)
          error = Bse::Error::FILE_OPEN_FAILED;
      }

    /* connect ports */
    const bool auto_connect = devid_ != no_auto_connect_devid;
    if (error == 0 && auto_connect)
      {
        std::map<std::string, DeviceDetails> devices = query_jack_devices (jack_client_);
        std::map<std::string, DeviceDetails>::const_iterator di;

        di = devices.find (devid_);
        if (di != devices.end())
          {
            const DeviceDetails &details = di->second;

            for (uint ch = 0; ch < n_channels_; ch++)
              {
                if (details.output_ports > ch)
                  jack_connect (jack_client_, details.output_port_names[ch].c_str(),
                                              jack_port_name (input_ports_[ch]));
                if (details.input_ports > ch)
                  jack_connect (jack_client_, jack_port_name (output_ports_[ch]),
                                              details.input_port_names[ch].c_str());
              }
          }
      }

    /* setup PCM handle or shutdown */
    if (error == 0)
      {
        flags_ |= Flags::OPENED;

        jack_pcm_driver = this;         // for jack midi driver
        uint dummy;
        pcm_latency (&dummy, &dummy);   // debugging only: print latency values
      }
    else
      {
        disconnect_jack (jack_client_);
        jack_client_ = nullptr;
      }
    JDEBUG ("%s: opening PCM: readable=%d writable=%d: %s", devid_, readable(), writable(), bse_error_blurb (error));
    return error;
  }
  virtual bool
  pcm_check_io (long *timeoutp) override
  {
    assert_return (jack_client_ != nullptr, false);

    if (0)
      {
        /* One of the things that we want to test is whether the JACK driver would
         * recover properly if a dropout should occur.
         *
         * Since dropouts occur rarely, we can use the TEST_DROPOUT macro. This
         * will check if /tmp/drop exists, and if so, sleep for some time to
         * ensure BSE can't write to/read from the ring buffer in time. Such an
         * artificial dropout can be created using
         *
         * $ touch /tmp/drop
         *
         * The file will be removed and a sleep will happen.
         *
         *  - for production builds, the macro should never be used
         *  - the macro invocation can be moved to other places in the source code
         *    to introduce a dropout there
         */
        TEST_DROPOUT();
      }

    /* enable processing in callback (if not already active) */
    atomic_active_ = 1;

    /* notify midi drivers if new events have arrived */
    notify_midi_drivers();

    /* report jack driver xruns */
    if (atomic_xruns_ != printed_xruns_)
      {
        printed_xruns_ = atomic_xruns_;
        Bse::printerr ("JACK: %s: %d beast driver xruns\n", devid_, printed_xruns_);
      }
    /* report jack shutdown */
    if (is_down_ && !printed_is_down_)
      {
        printed_is_down_ = true;
        Bse::printerr ("JACK: %s: connection to jack server lost\n", devid_);
        Bse::printerr ("JACK: %s:  -> to continue, manually stop playback and restart\n", devid_);
      }

    uint n_frames_avail = std::min (output_ringbuffer_.get_writable_frames(), input_ringbuffer_.get_readable_frames());

    /* check whether data can be processed */
    if (n_frames_avail >= block_length_)
      return true;        /* need processing */

    /* calculate timeout until processing is possible or needed */
    uint diff_frames = block_length_ - n_frames_avail;
    *timeoutp = diff_frames * 1000 / mix_freq_;

    /* wait at least 1ms, because caller may interpret (timeout == 0) as "process now" */
    *timeoutp = std::max<int> (*timeoutp, 1);
    return false;
  }
  virtual void
  pcm_latency (uint *rlatency, uint *wlatency) const override
  {
    assert_return (jack_client_ != NULL);

    jack_nframes_t jack_rlatency = 0;
    for (auto port : input_ports_)
      {
        jack_latency_range_t in_lrange;
        jack_port_get_latency_range (port, JackCaptureLatency, &in_lrange);

        jack_rlatency = std::max (jack_rlatency, in_lrange.max);
      }

    jack_nframes_t jack_wlatency = 0;
    for (auto port : output_ports_)
      {
        jack_latency_range_t out_lrange;
        jack_port_get_latency_range (port, JackPlaybackLatency, &out_lrange);

        jack_wlatency = std::max (jack_wlatency, out_lrange.max);
      }

    uint total_latency = buffer_frames_ + jack_rlatency + jack_wlatency;
    JDEBUG ("%s: jack_rlatency=%.3f ms jack_wlatency=%.3f ms ringbuffer=%.3f ms total_latency=%.3f ms",
            devid_,
            jack_rlatency / double (mix_freq_) * 1000,
            jack_wlatency / double (mix_freq_) * 1000,
            buffer_frames_ / double (mix_freq_) * 1000,
            total_latency / double (mix_freq_) * 1000);

    // ring buffer is normally completely filled
    //  -> the buffer latency counts as additional write latency

    *rlatency = jack_rlatency;
    *wlatency = jack_wlatency + buffer_frames_;
  }
  virtual size_t
  pcm_read (size_t n, float *values) override
  {
    assert_return (jack_client_ != nullptr, 0);
    assert_return (n == block_length_ * n_channels_, 0);

    device_read_counter_++;  // read must always gets called before write (see jack_device_write)

    float deinterleaved_frame_data[block_length_ * n_channels_];
    float *deinterleaved_frames[n_channels_];
    for (uint ch = 0; ch < n_channels_; ch++)
      deinterleaved_frames[ch] = &deinterleaved_frame_data[ch * block_length_];

    // in check_io, we already ensured that there is enough data in the input_ringbuffer

    uint frames_read = input_ringbuffer_.read (block_length_, deinterleaved_frames);
    assert_return (frames_read == block_length_, 0);

    for (uint ch = 0; ch < n_channels_; ch++)
      {
        const float *src = deinterleaved_frames[ch];
        float *dest = &values[ch];

        for (uint i = 0; i < frames_read; i++)
          {
            *dest = src[i];
            dest += n_channels_;
          }
      }
    return block_length_ * n_channels_;
  }
  virtual void
  pcm_write (size_t n, const float *values) override
  {
    assert_return (jack_client_ != nullptr);
    assert_return (n == block_length_ * n_channels_);

    /* our buffer management is based on the assumption that jack_device_read()
     * will always be performed before jack_device_write() - BEAST doesn't
     * always guarantee this (for instance when removing the pcm input module
     * from the snet while audio is playing), so we read and discard input
     * if BEAST didn't call jack_device_read() already
     */
    device_write_counter_++;
    if (device_read_counter_ < device_write_counter_)
      {
        float junk_frames[block_length_ * n_channels_];
        pcm_read (n, junk_frames);
        assert_return (device_read_counter_ == device_write_counter_);
      }

    // deinterleave
    float deinterleaved_frame_data[block_length_ * n_channels_];
    const float *deinterleaved_frames[n_channels_];
    for (uint ch = 0; ch < n_channels_; ch++)
      {
        float *channel_data = &deinterleaved_frame_data[ch * block_length_];
        for (uint i = 0; i < block_length_; i++)
          channel_data[i] = values[ch + i * n_channels_];
        deinterleaved_frames[ch] = channel_data;
      }

    // in check_io, we already ensured that there is enough space in the output_ringbuffer

    uint frames_written = output_ringbuffer_.write (block_length_, deinterleaved_frames);
    assert_return (frames_written == block_length_);
  }
  /*---- midi driver related ----*/
  uint
  attach_midi_driver (const std::string& from_port, MidiDriverCallback callback)
  {
    const uint id = midi_links.size() + 1;
    String port_name = string_format ("midi_in_%d", id);
    jack_port_t *midi_port = jack_port_register (jack_client_, port_name.c_str(), JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);

    const bool auto_connect = from_port != no_auto_connect_devid;
    if (auto_connect && jack_connect (jack_client_, from_port.c_str(), jack_port_name (midi_port)) != 0)
      {
        jack_port_unregister (jack_client_, midi_port);
        return 0;
      }

    auto& midi_link_ptr = midi_links.emplace_back (std::make_unique<MidiLink>());
    midi_link_ptr->callback = callback;
    midi_link_ptr->jack_port = midi_port;
    midi_link_ptr->id = id;
    return id;
  }
  void
  notify_midi_drivers()
  {
    for (auto& midi_link_ptr : midi_links)
      {
        uint n = midi_link_ptr->ring_buffer.get_readable_items();
        if (n > 0 && midi_link_ptr->callback)
          {
            uint8 midi_data[n];
            midi_link_ptr->ring_buffer.read (n, midi_data);
            midi_link_ptr->callback (n, midi_data); // process event data in JackMidiDriver
          }
      }
  }
  void
  detach_midi_driver (uint id)
  {
    for (auto& midi_link_ptr : midi_links)
      if (midi_link_ptr->id == id)
        midi_link_ptr->callback = nullptr;
  }
};

static const String jack_pcm_driverid = PcmDriver::register_driver ("jack", JackPcmDriver::create, list_jack_drivers);

/* JackMidiDriver and JackPcmDriver use the same jack_client which is owned by
 * the audio driver. The jack midi input port is really handled by the audio
 * driver in the JACK process_callback(), and the midi driver is notified by
 * the audio driver if new midi input is available.
 *
 * We assume that the API is used only by one thread at a time, and in the
 * following order:
 *
 * 1. open audio driver:
 *   - JackPcmDriver::open()
 * 2. open midi drivers:
 *   - JackMidiDriver::open() [can be called never, once or more than once]
 * 3. use audio/midi drivers:
 *   - JackPcmDriver::pcm_check_io(), ::pcm_write(), ... and related
 * 4. close drivers (in any order):
 *   - JackMidiDriver::close()
 *   - JackPcmDriver::close()
 *
 * After you have started using the audio/midi drivers, open() is no longer
 * permitted.
 */
class JackMidiDriver : public MidiDriver {
  BseMidiDecoder *midi_decoder_ = nullptr;
  uint            midi_driver_id_ = 0;
public:
  static MidiDriverP
  create (const String &devid)
  {
    auto mdriverp = std::make_shared<JackMidiDriver> (devid);
    return mdriverp;
  }
  explicit
  JackMidiDriver (const String &devid) :
    MidiDriver (devid)
  {
    midi_decoder_ = bse_midi_decoder_new (TRUE, FALSE, MusicalTuning::OD_12_TET);
  }
  ~JackMidiDriver()
  {
    bse_midi_decoder_destroy (midi_decoder_);
  }
  virtual void
  close () override
  {
    assert_return (opened());
    if (jack_pcm_driver)
      jack_pcm_driver->detach_midi_driver (midi_driver_id_);

    flags_ &= ~size_t (Flags::OPENED | Flags::READABLE | Flags::WRITABLE);
  }
  void
  process_midi (uint n, uint8 *midi_data)
  {
    const uint64 systime = sfi_time_system ();
    bse_midi_decoder_push_data (midi_decoder_, n, midi_data, systime);
  }
  virtual Error
  open (IODir iodir) override
  {
    assert_return (!opened(), Error::INTERNAL);

    if (!jack_pcm_driver)
      return Error::FILE_OPEN_FAILED;

    midi_driver_id_ = jack_pcm_driver->attach_midi_driver (devid_, [this] (uint n, uint8 *midi_data) { process_midi (n, midi_data); });
    if (!midi_driver_id_)
      return Error::FILE_OPEN_FAILED;

    flags_ |= Flags::READABLE | Flags::OPENED;
    return Error::NONE;
  }
  static void
  list (Driver::EntryVec &entries)
  {
    jack_client_t *jack_client = connect_jack();
    if (!jack_client)
      return;

    // ports which are automatically connected to hardware device
    uint priority = Driver::JACK;
    const char **ports = jack_get_ports (jack_client, NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsPhysical | JackPortIsOutput);
    if (ports)
      {
        for (uint i = 0; ports[i]; i++)
          {
            jack_port_t *jack_port = jack_port_by_name (jack_client, ports[i]);

            char alias1[MAX_JACK_STRING_SIZE] = "", alias2[MAX_JACK_STRING_SIZE] = "";
            char *aliases[2] = { alias1, alias2, };
            const int cnt = jack_port_get_aliases (jack_port, aliases);
            std::string input_port_alias = (cnt >= 1 && alias1[0]) ? alias1 : "";

            std::string devid = ports[i];
            Driver::Entry entry;
            entry.devid = devid;
            entry.device_name = string_format ("JACK Midi \"%s\"", devid);
            const std::string phprefix = "Physical: ";
            if (!input_port_alias.empty())
              entry.device_name += " [" + phprefix + input_port_alias + "]";

            entry.capabilities = "Hardware Midi Input";
            entry.device_info = "Routing via the JACK Audio Connection Kit";
            entry.priority = priority++;
            entry.readonly = true;
            entries.push_back (entry);
          }

        free (ports);
      }

    // port which isn't connected (let the user connect it to other apps)
    Driver::Entry entry;
    entry.devid = no_auto_connect_devid;
    entry.device_name = string_format ("JACK Midi \"%s\"", entry.devid);
    entry.capabilities = "Virtual Midi Input";
    entry.device_info = "Routing via the JACK Audio Connection Kit";
    entry.priority = priority++;
    entry.readonly = true;
    entries.push_back (entry);

    disconnect_jack (jack_client);
  }
};

static const String jack_midi_driverid = MidiDriver::register_driver ("jack", JackMidiDriver::create, JackMidiDriver::list);

} // Bse

#endif  // __has_include(<jack/jack.h>)
