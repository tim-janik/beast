// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsesequencer.hh"
#include "gslcommon.hh"
#include "bsemathsignal.hh"
#include "bseengine.hh"
#include "bsetrack.hh"
#include "bsepart.hh"
#include "bseproject.hh"
#include "bsemidireceiver.hh"
#include "bsemain.hh"
#include "bseieee754.hh"
#include <sys/poll.h>
#include <errno.h>
#include <string.h>
#include <vector>

#define SDEBUG(...)     BSE_KEY_DEBUG ("sequencer", __VA_ARGS__)

#define	BSE_SEQUENCER_FUTURE_BLOCKS    (7)

namespace Bse {

using Rapicorn::ThreadInfo; // FIXME

Sequencer              *Sequencer::singleton_ = NULL;
Mutex                   Sequencer::sequencer_mutex_;
static ThreadInfo      *sequencer_thread_self = NULL;

class Sequencer::PollPool {
public:
  struct IOWatch {
    BseIOWatch  watch_func;
    gpointer    watch_data;
    guint       index;
    guint       n_pfds;
    GPollFD    *notify_pfds; /* set during pol() */
  };
private:
  vector<IOWatch> watches;
  vector<GPollFD> watch_pfds;
public:
  guint
  get_n_pfds ()
  {
    return watch_pfds.size();
  }
  void
  fill_pfds (guint    n_pfds,
             GPollFD *pfds)
  {
    g_assert (n_pfds == watch_pfds.size());
    copy (watch_pfds.begin(), watch_pfds.end(), pfds);
    for (guint i = 0; i < watches.size(); i++)
      watches[i].notify_pfds = pfds + watches[i].index;
  }
  bool
  fetch_notify_watch (BseIOWatch &watch_func,
                      gpointer   &watch_data,
                      guint      &n_pfds,
                      GPollFD   *&pfds)
  {
    for (guint i = 0; i < watches.size(); i++)
      if (watches[i].notify_pfds)
        {
          for (guint j = 0; j < watches[i].n_pfds; j++)
            if (watches[i].notify_pfds[j].revents)
              {
                watch_func = watches[i].watch_func;
                watch_data = watches[i].watch_data;
                n_pfds = watches[i].n_pfds;
                pfds = watches[i].notify_pfds;
                watches[i].notify_pfds = NULL;
                return true;
              }
          /* no events found */
          watches[i].notify_pfds = NULL;
        }
    return false;
  }
  void
  add_watch (guint          n_pfds,
             const GPollFD *pfds,
             BseIOWatch     watch_func,
             gpointer       watch_data)
  {
    IOWatch iow = { 0, };
    iow.watch_func = watch_func;
    iow.watch_data = watch_data;
    iow.index = watch_pfds.size();
    iow.n_pfds = n_pfds;
    iow.notify_pfds = NULL;
    watches.push_back (iow);
    for (guint i = 0; i < n_pfds; i++)
      {
        GPollFD pfd = { 0, };
        pfd.fd = pfds[i].fd;
        pfd.events = pfds[i].events;
        watch_pfds.push_back (pfd);
      }
  }
  bool
  remove_watch (BseIOWatch     watch_func,
                gpointer       watch_data)
  {
    guint i;
    /* find watch */
    for (i = 0; i < watches.size(); i++)
      if (watches[i].watch_func == watch_func &&
          watches[i].watch_data == watch_data)
        break;
    if (i >= watches.size())
      return false;
    /* delete pfds */
    watch_pfds.erase (watch_pfds.begin() + watches[i].index, watch_pfds.begin() + watches[i].index + watches[i].n_pfds);
    /* adjust pfd indices of successors */
    for (guint j = i + 1; j < watches.size(); j++)
      watches[j].index -= watches[i].n_pfds;
    /* delete watch */
    watches.erase (watches.begin() + i);
    return true;
  }
  BIRNET_STATIC_ASSERT (sizeof (GPollFD) == sizeof (struct pollfd));
  BIRNET_STATIC_ASSERT (offsetof (GPollFD, fd) == offsetof (struct pollfd, fd));
  BIRNET_STATIC_ASSERT (sizeof (((GPollFD*) 0)->fd) == sizeof (((struct pollfd*) 0)->fd));
  BIRNET_STATIC_ASSERT (offsetof (GPollFD, events) == offsetof (struct pollfd, events));
  BIRNET_STATIC_ASSERT (sizeof (((GPollFD*) 0)->events) == sizeof (((struct pollfd*) 0)->events));
  BIRNET_STATIC_ASSERT (offsetof (GPollFD, revents) == offsetof (struct pollfd, revents));
  BIRNET_STATIC_ASSERT (sizeof (((GPollFD*) 0)->revents) == sizeof (((struct pollfd*) 0)->revents));
};

void
Sequencer::add_io_watch (uint n_pfds, const GPollFD *pfds, BseIOWatch watch_func, void *watch_data)
{
  g_return_if_fail (watch_func != NULL);
  BSE_SEQUENCER_LOCK();
  poll_pool_->add_watch (n_pfds, pfds, watch_func, watch_data);
  BSE_SEQUENCER_UNLOCK();
}

static BseIOWatch current_watch_func = NULL;    // global guards allow removal of watch function while it's in use
static gpointer   current_watch_data = NULL;
static bool       current_watch_needs_remove1 = false;
static bool       current_watch_needs_remove2 = false;

void
Sequencer::remove_io_watch (BseIOWatch watch_func, void *watch_data)
{
  g_return_if_fail (watch_func != NULL);
  /* removal requirements:
   * - any thread should be able to remove an io watch (once)
   * - a watch_func() should be able to remove its own io watch
   * - a watch_func() may not get called after remove_io_watch()
   *   finished in any thread
   * - concurrent removal of an io watch from within its watch_func() (executed
   *   within the sequencer thread) and from an external thread at the same
   *   time, should succeed in _both_ threads
   * - a warning should be issued if an io watch has already been removed (at
   *   least conceptually) or has never been installed
   */
  bool removal_success;
  BSE_SEQUENCER_LOCK();
  if (current_watch_func == watch_func && current_watch_data == watch_data)
    {  /* watch_func() to be removed is currently in call */
      if (&ThreadInfo::self() == sequencer_thread_self)
        {
          /* allow removal calls from within a watch_func() */
          removal_success = !current_watch_needs_remove1;       /* catch multiple calls */
          current_watch_needs_remove1 = true;
        }
      else /* not in sequencer thread */
        {
          /* allow removal from other threads during watch_func() call */
          removal_success = !current_watch_needs_remove2;       /* catch multiple calls */
          current_watch_needs_remove2 = true;
          /* wait until watch_func() call has finished */
          while (current_watch_func == watch_func && current_watch_data == watch_data)
            watch_cond_.wait (sequencer_mutex_);
        }
    }
  else /* can remove (watch_func(watch_data) not in call) */
    {
      removal_success = poll_pool_->remove_watch (watch_func, watch_data);
      /* wake up sequencer thread, so it stops polling on fds it doesn't own anymore */
      wakeup();
    }
  BSE_SEQUENCER_UNLOCK();
  if (!removal_success)
    g_warning ("%s: failed to remove %p(%p)", G_STRFUNC, watch_func, watch_data);
}

bool
Sequencer::pool_poll_Lm (gint timeout_ms)
{
  uint n_pfds = poll_pool_->get_n_pfds() + 1;   // one for the wake-up event_fd_
  GPollFD *pfds = g_newa (GPollFD, n_pfds);
  pfds[0].fd = event_fd_.inputfd();
  pfds[0].events = G_IO_IN;
  pfds[0].revents = 0;
  poll_pool_->fill_pfds (n_pfds - 1, pfds + 1); // rest used for io watch array
  BSE_SEQUENCER_UNLOCK();
  int result = poll ((struct pollfd*) pfds, n_pfds, timeout_ms);
  if (result < 0 && errno != EINTR)
    g_printerr ("%s: poll() error: %s\n", G_STRFUNC, g_strerror (errno));
  BSE_SEQUENCER_LOCK();
  if (result > 0 && pfds[0].revents)
    {
      event_fd_.flush();                        // eat wake up message
      result -= 1;
    }
  if (result > 0)
    {
      /* dispatch io watches */
      guint watch_n_pfds;
      GPollFD *watch_pfds;
      while (poll_pool_->fetch_notify_watch (current_watch_func, current_watch_data, watch_n_pfds, watch_pfds))
        {
          g_assert (!current_watch_needs_remove1 && !current_watch_needs_remove2);
          BSE_SEQUENCER_UNLOCK();
          bool current_watch_stays_alive = current_watch_func (current_watch_data, watch_n_pfds, watch_pfds);
          BSE_SEQUENCER_LOCK();
          if (current_watch_needs_remove1 ||            // removal queued from within io handler
              current_watch_needs_remove2 ||            // removal queued from other thread
              !current_watch_stays_alive)               // removal requested by io handler return value
            poll_pool_->remove_watch (current_watch_func, current_watch_data);
          current_watch_needs_remove1 = false;
          current_watch_needs_remove2 = false;
          current_watch_func = NULL;
          current_watch_data = NULL;
          watch_cond_.broadcast();      // wake up threads in remove_io_watch()
        }
    }
  return true;
}

void
Sequencer::start_song (BseSong *song, uint64 start_stamp)
{
  g_return_if_fail (BSE_IS_SONG (song));
  g_return_if_fail (BSE_SOURCE_PREPARED (song));
  g_return_if_fail (song->sequencer_start_request_SL == 0);
  g_assert (song->sequencer_owns_refcount_SL == false);
  start_stamp = MAX (start_stamp, 1);
  g_object_ref (song);
  BSE_SEQUENCER_LOCK();
  song->sequencer_owns_refcount_SL = true;
  song->sequencer_start_request_SL = start_stamp <= 1 ? stamp_ : start_stamp;
  song->sequencer_start_SL = 0;
  song->sequencer_done_SL = 0;
  song->delta_stamp_SL = 0;
  song->tick_SL = 0;
  SfiRing *ring;
  for (ring = song->tracks_SL; ring; ring = sfi_ring_walk (ring, song->tracks_SL))
    {
      BseTrack *track = (BseTrack*) ring->data;
      track->track_done_SL = FALSE;
    }
  songs_ = sfi_ring_append (songs_, song);
  BSE_SEQUENCER_UNLOCK();
  wakeup();
}

void
Sequencer::remove_song (BseSong *song)
{
  g_return_if_fail (BSE_IS_SONG (song));
  g_return_if_fail (BSE_SOURCE_PREPARED (song));
  if (song->sequencer_start_request_SL == 0)
    {
      g_assert (song->sequencer_owns_refcount_SL == false);
      return;   // uncontained
    }
  BSE_SEQUENCER_LOCK();
  SfiRing *ring = sfi_ring_find (songs_, song);
  songs_ = sfi_ring_remove_node (songs_, ring);
  song->sequencer_start_request_SL = 0;
  if (!song->sequencer_done_SL)
    song->sequencer_done_SL = stamp_;
  if (!song->sequencer_start_SL)
    song->sequencer_start_SL = song->sequencer_done_SL;
  bool need_unref = song->sequencer_owns_refcount_SL;
  song->sequencer_owns_refcount_SL = false;
  BSE_SEQUENCER_UNLOCK();
  if (!ring)
    g_warning ("%s: failed to find %s in sequencer", G_STRLOC, bse_object_debug_name (song));
  if (need_unref)
    g_object_unref (song);
}

static int
sequencer_remove_song_async (gpointer data) /* UserThread */
{
  BseSong *song = BSE_SONG (data);
  if (BSE_SOURCE_PREPARED (song) &&     // project might be deactivated already
      song->sequencer_done_SL)          // song might have been removed and re-added
    {
      Sequencer::instance().remove_song (song);
      BseProject *project = bse_item_get_project (BSE_ITEM (song));
      bse_project_check_auto_stop (project);
    }
  g_object_unref (song);                // sequencer_owns_refcount_SL = false; from sequencer_queue_remove_song_SL()
  return false;
}

static void
sequencer_queue_remove_song_SL (BseSong *song)
{
  g_return_if_fail (song->sequencer_owns_refcount_SL == true);
  song->sequencer_owns_refcount_SL = false;     // g_object_unref() in sequencer_remove_song_async()
  // queue a job into the BSE core for immediate execution
  bse_idle_now (sequencer_remove_song_async, song);
}

bool
Sequencer::thread_lagging (uint n_blocks)
{
  // return whether the sequencer lags for n_blocks future stamps
  const uint64 cur_stamp = Bse::TickStamp::current();
  uint64 next_stamp = cur_stamp + n_blocks * bse_engine_block_size();
  BSE_SEQUENCER_LOCK();
  bool lagging = stamp_ < next_stamp;
  BSE_SEQUENCER_UNLOCK();
  return lagging;
}

void
Sequencer::sequencer_thread ()
{
  Bse::TaskRegistry::add ("Sequencer", Rapicorn::ThisThread::process_pid(), Rapicorn::ThisThread::thread_pid());
  sequencer_thread_self = &ThreadInfo::self();
  SDEBUG ("thrdstrt: now=%llu", Bse::TickStamp::current());
  Bse::TickStampWakeupP wakeup = Bse::TickStamp::create_wakeup ([&]() { this->wakeup(); });
  BSE_SEQUENCER_LOCK();
  do
    {
      const guint64 cur_stamp = Bse::TickStamp::current();
      guint64 next_stamp = cur_stamp + BSE_SEQUENCER_FUTURE_BLOCKS * bse_engine_block_size();
      SfiRing *ring;
      for (ring = songs_; ring; ring = sfi_ring_walk (ring, songs_))
	{
          BseSong *song = BSE_SONG (ring->data);
          bool forced_ticks = 0;
          if (!song->sequencer_start_SL && song->sequencer_start_request_SL <= next_stamp + bse_engine_block_size())
            {
              song->sequencer_start_SL = next_stamp;
              forced_ticks = bse_engine_block_size();
            }
          if (song->sequencer_start_SL && !song->sequencer_done_SL)
            {
              gdouble stamp_diff = (next_stamp - song->sequencer_start_SL) - song->delta_stamp_SL;
              guint64 old_song_pos = bse_dtoll (song->sequencer_start_SL + song->delta_stamp_SL);
              gboolean song_starting = song->delta_stamp_SL == 0;
              if (UNLIKELY (forced_ticks))
                stamp_diff = MAX (stamp_diff, 1);
	      while (stamp_diff > 0)
		{
		  guint n_ticks = bse_dtoi (stamp_diff * song->tpsi_SL);
                  if (UNLIKELY (forced_ticks))
                    {
                      n_ticks = MAX (n_ticks, forced_ticks);
                      forced_ticks = 0;
                    }
		  if (n_ticks < 1)
		    break;
		  process_song_SL (song, n_ticks);
		  stamp_diff = (next_stamp - song->sequencer_start_SL) - song->delta_stamp_SL;
		}
              if (old_song_pos <= cur_stamp && !song_starting) /* detect underrun after song start */
                {
                  gchar *dh = bse_object_strdup_debug_handle (song);    /* thread safe */
                  /* if (!song->sequencer_underrun_detected_SL) */
                  g_printerr ("BseSequencer: underrun by %lld blocks for song: %s\n",
                              uint64 ((cur_stamp - old_song_pos) / bse_engine_block_size() + 1),
                              dh);
                  song->sequencer_underrun_detected_SL = TRUE;
                  g_free (dh);
                }
	    }
	}
      stamp_ = next_stamp;
      wakeup->awake_after (cur_stamp + bse_engine_block_size ());
    }
  while (pool_poll_Lm (-1));
  BSE_SEQUENCER_UNLOCK();
  SDEBUG ("thrdstop: now=%llu", Bse::TickStamp::current());
  Bse::TaskRegistry::remove (Rapicorn::ThisThread::thread_pid());
}

bool
Sequencer::process_song_unlooped_SL (BseSong *song, uint n_ticks, bool force_active_tracks)
{
  BseMidiReceiver *midi_receiver = song->midi_receiver_SL;
  gdouble current_stamp = song->sequencer_start_SL + song->delta_stamp_SL;
  gdouble stamps_per_tick = 1.0 / song->tpsi_SL;
  guint64 next_stamp = bse_dtoull (current_stamp + n_ticks * stamps_per_tick);
  guint tick_bound = song->tick_SL + n_ticks;
  guint n_done_tracks = 0, n_tracks = 0;
  SfiRing *ring;
  for (ring = song->tracks_SL; ring; ring = sfi_ring_walk (ring, song->tracks_SL))
    {
      BseTrack *track = (BseTrack*) ring->data;
      n_tracks++;
      if (!track->track_done_SL || force_active_tracks)
	{
	  track->track_done_SL = FALSE;
	  process_track_SL (track, current_stamp, song->tick_SL, tick_bound, stamps_per_tick, midi_receiver);
	}
      if (track->track_done_SL)
	n_done_tracks++;
    }
  bse_midi_receiver_process_events (midi_receiver, next_stamp);
  song->tick_SL += n_ticks;
  song->delta_stamp_SL += n_ticks * stamps_per_tick;
  return n_done_tracks != n_tracks;
}

void
Sequencer::process_song_SL (BseSong *song, uint n_ticks)
{
  gboolean tracks_active = TRUE;
  if (song->loop_enabled_SL && (gint64) song->tick_SL <= song->loop_right_SL)
    do
      {
	guint tdiff = song->loop_right_SL - song->tick_SL;
	tdiff = MIN (tdiff, n_ticks);
	if (tdiff)
	  process_song_unlooped_SL (song, tdiff, true);
	n_ticks -= tdiff;
	if ((gint64) song->tick_SL >= song->loop_right_SL)
	  {
	    song->tick_SL = song->loop_left_SL;
	  }
      }
    while (n_ticks);
  else
    tracks_active = process_song_unlooped_SL (song, n_ticks, false);
  if (!song->sequencer_done_SL && !tracks_active)
    {
      song->sequencer_done_SL = stamp_;
      sequencer_queue_remove_song_SL (song);
    }
}

void
Sequencer::process_track_SL (BseTrack *track, double start_stamp, uint start_tick,
                             uint bound, /* start_tick + n_ticks */
                             double stamps_per_tick, BseMidiReceiver *midi_receiver)
{
  uint start, next;
  BsePart *part = bse_track_get_part_SL (track, start_tick, &start, &next);
  /* advance to first part */
  if (!part && next)
    {
      part = bse_track_get_part_SL (track, next, &start, &next);
      SDEBUG ("trackjmp: tick=%u fast forward to first part part=%p now=%llu", start_tick, part, Bse::TickStamp::current());
    }
  if (!part || (next == 0 && start + part->last_tick_SL < start_tick))
    {
      track->track_done_SL = !bse_midi_receiver_voices_pending (midi_receiver, track->midi_channel_SL);
      SDEBUG ("trackchk: tick=%u next=%u part=%p done=%u now=%llu", // part==NULL || start + (part ? part->last_tick_SL : 0) < start_tick
              start_tick, next, part, track->track_done_SL, Bse::TickStamp::current());
      part = NULL;
    }
  while (part && start < bound)
    {
      guint part_start, part_bound;
      gdouble part_stamp;
      if (start_tick > start)
	part_start = start_tick - start;
      else
	part_start = 0;
      part_stamp = start_stamp + (start + part_start - start_tick) * stamps_per_tick;
      part_bound = next ? MIN (bound, next) : bound;
      part_bound -= start;
      if (!track->muted_SL)
	process_part_SL (part, part_stamp, part_start, part_bound, stamps_per_tick, midi_receiver, track->midi_channel_SL);
      part = next ? bse_track_get_part_SL (track, next, &start, &next) : NULL;
    }
}

void
Sequencer::process_part_SL (BsePart *part, double start_stamp, uint start_tick,
                            uint tick_bound, /* start_tick + n_ticks */
                            double stamps_per_tick, BseMidiReceiver *midi_receiver, uint midi_channel)
{
  BsePartTickNode *node, *last;
  uint channel;
  for (channel = 0; channel < part->n_channels; channel++)
    {
      BsePartEventNote *note = bse_part_note_channel_lookup_ge (&part->channels[channel], start_tick);
      BsePartEventNote *bound = note ? bse_part_note_channel_get_bound (&part->channels[channel]) : NULL;
      while (note < bound && note->tick < tick_bound)
        {
          BseMidiEvent *eon, *eoff;
          gfloat freq = BSE_PART_NOTE_FREQ (part, note);
          eon  = bse_midi_event_note_on (midi_channel,
                                         bse_dtoull (start_stamp + (note->tick - start_tick) * stamps_per_tick),
                                         freq, note->velocity);
          eoff = bse_midi_event_note_off (midi_channel,
                                          bse_dtoull (start_stamp + (note->tick - start_tick + note->duration) * stamps_per_tick),
                                          freq);
          bse_midi_receiver_push_event (midi_receiver, eon);
          bse_midi_receiver_push_event (midi_receiver, eoff);
          SDEBUG ("note-on:  tick=%llu midinote=%-3d velocity=%02x freq=% 10f now=%llu",
                  uint64 (eon->delta_time),  note->note, bse_ftoi (note->velocity * 128), freq, Bse::TickStamp::current());
          SDEBUG ("note-off: tick=%llu midinote=%-3d velocity=%02x freq=% 10f now=%llu",
                  uint64 (eoff->delta_time), note->note, bse_ftoi (note->velocity * 128), freq, Bse::TickStamp::current());
          note++;
        }
    }
  node = bse_part_controls_lookup_ge (&part->controls, start_tick);
  last = bse_part_controls_lookup_lt (&part->controls, tick_bound);
  if (node) while (node <= last)
    {
      BsePartEventControl *cev;
      for (cev = node->events; cev; cev = cev->next)
        {
          BseMidiEvent *event = bse_midi_event_signal (midi_channel,
                                                       bse_dtoull (start_stamp + (node->tick - start_tick) * stamps_per_tick),
                                                       BseMidiSignalType (cev->ctype), cev->value);
          bse_midi_receiver_push_event (midi_receiver, event);
          SDEBUG ("control:  tick=%llu midisignal=%-3d value=%f now=%llu",
                  uint64 (event->delta_time), cev->ctype, cev->value, Bse::TickStamp::current());
        }
      node++;
    }
}

Sequencer::Sequencer() :
  stamp_ (0), songs_ (NULL)
{
  stamp_ = Bse::TickStamp::current();
  assert (stamp_ > 0);

  poll_pool_ = new PollPool;

  if (event_fd_.open() != 0)
    g_error ("failed to create sequencer wake-up pipe: %s", strerror (errno));
}

void
Sequencer::_init_threaded ()
{
  assert (singleton_ == NULL);
  singleton_ = new Sequencer();
  singleton_->thread_ = std::thread (&Sequencer::sequencer_thread, singleton_); // FIXME: join on exit
}

} // Bse
