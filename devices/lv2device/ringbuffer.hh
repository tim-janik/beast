// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html

#include "bse/bseblockutils.hh"

namespace {

using namespace Bse;
using namespace std;

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

template<class T>
class RingBuffer
{
  FrameRingBuffer<T> buffer_;
public:
  RingBuffer (uint n_values = 0) :
    buffer_ (n_values)
  {
  }
  uint
  write (uint     n_values,
         const T *values)
  {
    const T *t[1] = { values };
    return buffer_.write (n_values, t);
  }
  uint
  get_writable_values()
  {
    return buffer_.get_writable_frames();
  }
  uint
  read (uint n_values,
        T   *values)
  {
    T *t[1] = { values };
    return buffer_.read (n_values, t);
  }
  uint
  get_readable_values()
  {
    return buffer_.get_readable_frames();
  }
};

};
