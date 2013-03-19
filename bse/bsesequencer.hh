// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_SSEQUENCER_HH__
#define __BSE_SSEQUENCER_HH__
#include <bse/bsesong.hh>

namespace Bse {

using Rapicorn::Mutex;    // FIXME
using Rapicorn::Cond;    // FIXME
using Rapicorn::EventFd;    // FIXME

/** Note and MIDI sequencer.
 * The sequencer processes notes from parts and MIDI input and generates events for the synthesis engine.
 */
class Sequencer {
  static Sequencer *singleton_;
  static Mutex      sequencer_mutex_;
  struct PollPool;
  uint64     stamp_;            // sequencer time (ahead of real time)
  SfiRing   *songs_;
  Cond       watch_cond_;
  PollPool  *poll_pool_;
  EventFd    event_fd_;
  std::thread thread_;
private:
  void          sequencer_thread ();
  bool          pool_poll_Lm     (int timeout_ms);
  void          process_part_SL  (BsePart *part, double start_stamp, uint start_tick,
                                  uint tick_bound, /* start_tick + n_ticks */
                                  double stamps_per_tick, BseMidiReceiver *midi_receiver, uint midi_channel);
  void          process_track_SL (BseTrack *track, double start_stamp, uint start_tick,
                                  uint bound, /* start_tick + n_ticks */
                                  double stamps_per_tick, BseMidiReceiver *midi_receiver);
  void          process_song_SL  (BseSong *song, uint n_ticks);
  bool          process_song_unlooped_SL (BseSong *song, uint n_ticks, bool force_active_tracks);
  explicit      Sequencer       ();
protected:
  static void   _init_threaded  ();
public:
  void          add_io_watch    (uint n_pfds, const GPollFD *pfds, BseIOWatch watch_func, void *watch_data);
  void          remove_io_watch (BseIOWatch watch_func, void *watch_data);
  void          start_song	(BseSong *song, uint64 start_stamp);
  void          remove_song	(BseSong *song);
  bool          thread_lagging  (uint n_blocks);
  void          wakeup          ()      { event_fd_.wakeup(); }
  static Mutex& sequencer_mutex ()      { return sequencer_mutex_; }
  static Sequencer& instance    ()      { return *singleton_; }
};

#define BSE_SEQUENCER_LOCK()    (Bse::Sequencer::sequencer_mutex().lock())
#define BSE_SEQUENCER_UNLOCK()  (Bse::Sequencer::sequencer_mutex().unlock())

} // Bse

#endif // __BSE_SSEQUENCER_HH__
