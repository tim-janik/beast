/* BSE - Better Sound Engine
 * Copyright (C) 1997-1999, 2000-2004 Tim Janik
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
#include "bsesequencer.h"
#include "gslcommon.h"
#include "bsemathsignal.h"
#include "bseengine.h"
#include "bsetrack.h"
#include "bsepart.h"
#include "bseproject.h"
#include "bsemidireceiver.h"
#include "bsemain.h"
#include "bseieee754.h"
#include <sys/poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <vector>

/* due to a linker/compiler bug on SuSE 9.2, we need to
 * define extern "C" symbols outside of any C++ namespace,
 * in order for C code to link against it.
 */
extern "C" { BirnetThread *bse_sequencer_thread = NULL; }

using namespace std;

#define CHECKTRACE()    UNLIKELY (bse_trace_args.sequencer)
#define SEQTRACE(...)   do { if (CHECKTRACE()) sfi_debug_channel_printf (bse_trace_args.sequencer, NULL, __VA_ARGS__); } while (0)

#define	BSE_SEQUENCER_FUTURE_BLOCKS    (7)

/* --- prototypes --- */
static void	bse_sequencer_thread_main	(gpointer	 data);
static void	bse_sequencer_process_song_SL	(BseSong	*song,
						 guint		 n_ticks);


/* --- variables --- */
static BseSequencer    *global_sequencer = NULL;
static BirnetCond          current_watch_cond = { 0, };
static gint             sequencer_wake_up_pipe[2] = { -1, -1 };

/* --- functions --- */
extern "C" void
bse_sequencer_init_thread (void)
{
  g_assert (bse_sequencer_thread == NULL);

  sfi_cond_init (&current_watch_cond);

  if (pipe (sequencer_wake_up_pipe) < 0)
    g_error ("failed to create sequencer wake-up pipe: %s", strerror (errno));
  glong flags = fcntl (sequencer_wake_up_pipe[0], F_GETFL, 0);
  fcntl (sequencer_wake_up_pipe[0], F_SETFL, O_NONBLOCK | flags);
  flags = fcntl (sequencer_wake_up_pipe[1], F_GETFL, 0);
  fcntl (sequencer_wake_up_pipe[1], F_SETFL, O_NONBLOCK | flags);

  /* initialize BseSequencer */
  static BseSequencer sseq = { 0, };
  sseq.stamp = gsl_tick_stamp ();
  g_assert (sseq.stamp > 0);
  global_sequencer = &sseq;

  bse_sequencer_thread = sfi_thread_run ("Sequencer", bse_sequencer_thread_main, NULL);
  if (!bse_sequencer_thread)
    g_error ("failed to create sequencer thread");
}

static void
sequencer_wake_up (gpointer wake_up_data)
{
  guint8 wake_up_message = 'W';
  gint err;
  do
    err = write (sequencer_wake_up_pipe[1], &wake_up_message, 1);
  while (err < 0 && errno == EINTR);
}

namespace { // Anon
class PollPool {
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
} // Anon

static PollPool sequencer_poll_pool;

extern "C" void
bse_sequencer_add_io_watch (guint           n_pfds,
                            const GPollFD  *pfds,
                            BseIOWatch      watch_func,
                            gpointer        data)
{
  g_return_if_fail (watch_func != NULL);
  BSE_SEQUENCER_LOCK ();
  sequencer_poll_pool.add_watch (n_pfds, pfds, watch_func, data);
  BSE_SEQUENCER_UNLOCK ();
}

static BseIOWatch current_watch_func = NULL;
static gpointer   current_watch_data = NULL;
static bool       current_watch_needs_remove1 = false;
static bool       current_watch_needs_remove2 = false;

extern "C" void
bse_sequencer_remove_io_watch (BseIOWatch      watch_func,
                               gpointer        watch_data)
{
  g_return_if_fail (watch_func != NULL);
  /* removal requirements:
   * - any thread should be able to remove an io watch (once)
   * - a watch_func() should be able to remove its own io watch
   * - a watch_func() may not get called after bse_sequencer_remove_io_watch()
   *   finished in any thread
   * - concurrent removal of an io watch from within its watch_func() (executed
   *   within the sequencer thread) and from an external thread at the same
   *   time, should succeed in _both_ threads
   * - a warning should be issued if an io watch has already been removed (at
   *   least conceptually) or has never been installed
   */
  bool removal_success;
  BSE_SEQUENCER_LOCK ();
  if (current_watch_func == watch_func && current_watch_data == watch_data)
    {  /* watch_func() to be removed is currently in call */
      if (bse_sequencer_thread == sfi_thread_self())
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
            sfi_cond_wait (&current_watch_cond, &bse_main_sequencer_mutex);
        }
    }
  else /* can remove (watch_func(watch_data) not in call) */
    {
      removal_success = sequencer_poll_pool.remove_watch (watch_func, watch_data);
      /* wake up sequencer thread, so it stops polling on fds it doesn't own anymore */
      sfi_thread_wakeup (bse_sequencer_thread);
    }
  BSE_SEQUENCER_UNLOCK ();
  if (!removal_success)
    g_warning ("%s: failed to remove %p(%p)", G_STRFUNC, watch_func, watch_data);
}

static bool
bse_sequencer_poll_Lm (gint timeout_ms)
{
  guint n_pfds = sequencer_poll_pool.get_n_pfds() + 1;  /* one for the wake-up pipe */
  GPollFD *pfds = g_newa (GPollFD, n_pfds);
  pfds[0].fd = sequencer_wake_up_pipe[0];
  pfds[0].events = G_IO_IN;
  pfds[0].revents = 0;
  sequencer_poll_pool.fill_pfds (n_pfds - 1, pfds + 1); /* rest used for io watch array */
  BSE_SEQUENCER_UNLOCK ();
  gint result = poll ((struct pollfd*) pfds, n_pfds, timeout_ms);
  if (result < 0 && errno != EINTR)
    g_printerr ("%s: poll() error: %s\n", G_STRFUNC, g_strerror (errno));
  BSE_SEQUENCER_LOCK ();
  if (result > 0 && pfds[0].revents)
    {
      guint8 buffer[256];
      read (sequencer_wake_up_pipe[0], buffer, 256);    /* eat wake up message */
      result -= 1;
    }
  if (result > 0)
    {
      /* dispatch io watches */
      guint watch_n_pfds;
      GPollFD *watch_pfds;
      while (sequencer_poll_pool.fetch_notify_watch (current_watch_func, current_watch_data, watch_n_pfds, watch_pfds))
        {
          g_assert (!current_watch_needs_remove1 && !current_watch_needs_remove2);
          BSE_SEQUENCER_UNLOCK ();
          bool current_watch_stays_alive = current_watch_func (current_watch_data, watch_n_pfds, watch_pfds);
          BSE_SEQUENCER_LOCK ();
          if (current_watch_needs_remove1 ||            /* removal queued from within io handler */
              current_watch_needs_remove2 ||            /* removal queued from other thread */
              !current_watch_stays_alive)               /* removal requested by io handler return value */
            sequencer_poll_pool.remove_watch (current_watch_func, current_watch_data);
          current_watch_needs_remove1 = false;
          current_watch_needs_remove2 = false;
          current_watch_func = NULL;
          current_watch_data = NULL;
          sfi_cond_broadcast (&current_watch_cond);     /* wake up threads in bse_sequencer_remove_io_watch() */
        }
    }
  return !sfi_thread_aborted();
}

extern "C" void
bse_sequencer_start_song (BseSong        *song,
                          guint64         start_stamp)
{
  g_assert (bse_sequencer_thread != NULL);
  g_return_if_fail (BSE_IS_SONG (song));
  g_return_if_fail (BSE_SOURCE_PREPARED (song));
  g_return_if_fail (song->sequencer_start_request_SL == 0);
  g_assert (song->sequencer_owns_refcount_SL == FALSE);
  start_stamp = MAX (start_stamp, 1);

  g_object_ref (song);
  BSE_SEQUENCER_LOCK ();
  song->sequencer_owns_refcount_SL = TRUE;
  song->sequencer_start_request_SL = start_stamp <= 1 ? global_sequencer->stamp : start_stamp;
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
  global_sequencer->songs = sfi_ring_append (global_sequencer->songs, song);
  BSE_SEQUENCER_UNLOCK ();
  sfi_thread_wakeup (bse_sequencer_thread);
}

extern "C" void
bse_sequencer_remove_song (BseSong *song)
{
  g_return_if_fail (BSE_IS_SONG (song));
  g_return_if_fail (BSE_SOURCE_PREPARED (song));
  if (song->sequencer_start_request_SL == 0)
    {
      g_assert (song->sequencer_owns_refcount_SL == FALSE);
      return;   /* uncontained */
    }

  BSE_SEQUENCER_LOCK ();
  SfiRing *ring = sfi_ring_find (global_sequencer->songs, song);
  global_sequencer->songs = sfi_ring_remove_node (global_sequencer->songs, ring);
  song->sequencer_start_request_SL = 0;
  if (!song->sequencer_done_SL)
    song->sequencer_done_SL = global_sequencer->stamp;
  if (!song->sequencer_start_SL)
    song->sequencer_start_SL = song->sequencer_done_SL;
  gboolean need_unref = song->sequencer_owns_refcount_SL;
  song->sequencer_owns_refcount_SL = FALSE;
  BSE_SEQUENCER_UNLOCK ();
  if (!ring)
    g_warning ("%s: failed to find %s in sequencer", G_STRLOC, bse_object_debug_name (song));
  if (need_unref)
    g_object_unref (song);
}

static gboolean
bse_sequencer_remove_song_async (gpointer data) /* UserThread */
{
  BseSong *song = BSE_SONG (data);
  if (BSE_SOURCE_PREPARED (song) &&     /* project might be deactivated already */
      song->sequencer_done_SL)          /* song might have been removed and re-added */
    {
      bse_sequencer_remove_song (song);
      BseProject *project = bse_item_get_project (BSE_ITEM (song));
      bse_project_check_auto_stop (project);
    }
  g_object_unref (song);                        /* sequencer_owns_refcount_SL = FALSE from bse_sequencer_queue_remove_song_SL() */
  return FALSE;
}

static void
bse_sequencer_queue_remove_song_SL (BseSong *song)
{
  g_return_if_fail (song->sequencer_owns_refcount_SL == TRUE);
  song->sequencer_owns_refcount_SL = FALSE;     /* g_object_unref() in bse_sequencer_remove_song_async() */
  /* queue a job into the BSE core for immediate execution */
  bse_idle_now (bse_sequencer_remove_song_async, song);
}

extern "C" gboolean
bse_sequencer_thread_lagging (guint n_blocks)
{
  /* return whether the sequencer lags for n_blocks future stamps */
  const guint64 cur_stamp = gsl_tick_stamp ();
  guint64 next_stamp = cur_stamp + n_blocks * bse_engine_block_size();
  BSE_SEQUENCER_LOCK ();
  gboolean lagging = global_sequencer->stamp < next_stamp;
  BSE_SEQUENCER_UNLOCK ();
  return lagging;
}

static void
bse_sequencer_thread_main (gpointer data)
{
  SEQTRACE ("SEQ:thrdstrt: now=%llu", gsl_tick_stamp());
  sfi_thread_set_wakeup (sequencer_wake_up, NULL, NULL);
  bse_message_setup_thread_handler();
  BSE_SEQUENCER_LOCK ();
  do
    {
      const guint64 cur_stamp = gsl_tick_stamp ();
      guint64 next_stamp = cur_stamp + BSE_SEQUENCER_FUTURE_BLOCKS * bse_engine_block_size();
      SfiRing *ring;
      
      for (ring = global_sequencer->songs; ring; ring = sfi_ring_walk (ring, global_sequencer->songs))
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
		  bse_sequencer_process_song_SL (song, n_ticks);
		  stamp_diff = (next_stamp - song->sequencer_start_SL) - song->delta_stamp_SL;
		}
              if (old_song_pos <= cur_stamp && !song_starting) /* detect underrun after song start */
                {
                  gchar *dh = bse_object_strdup_debug_handle (song);    /* thread safe */
                  /* if (!song->sequencer_underrun_detected_SL) */
                  g_printerr ("BseSequencer: underrun by %lld blocks for song: %s\n",
                              (cur_stamp - old_song_pos) / bse_engine_block_size() + 1,
                              dh);
                  song->sequencer_underrun_detected_SL = TRUE;
                  g_free (dh);
                }
	    }
	}
      global_sequencer->stamp = next_stamp;
      
      sfi_thread_awake_after (cur_stamp + bse_engine_block_size ());
    }
  while (bse_sequencer_poll_Lm (-1));
  BSE_SEQUENCER_UNLOCK ();
  SEQTRACE ("SEQ:thrdstop: now=%llu", gsl_tick_stamp());
}

static void
bse_sequencer_process_track_SL (BseTrack        *track,
                                gdouble          start_stamp,
                                guint            start_tick,
                                guint            n_ticks,
                                gdouble          stamps_per_tick,
                                BseMidiReceiver *midi_receiver);

static gboolean
bse_sequencer_process_song_unlooped_SL (BseSong *song,
                                        guint    n_ticks,
                                        gboolean force_active_tracks)
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
	  bse_sequencer_process_track_SL (track, current_stamp,
                                          song->tick_SL, tick_bound,
                                          stamps_per_tick,
                                          midi_receiver);
	}
      if (track->track_done_SL)
	n_done_tracks++;
    }
  bse_midi_receiver_process_events (midi_receiver, next_stamp);
  song->tick_SL += n_ticks;
  song->delta_stamp_SL += n_ticks * stamps_per_tick;
  return n_done_tracks != n_tracks;
}

static void
bse_sequencer_process_song_SL (BseSong *song,
                               guint    n_ticks)
{
  gboolean tracks_active = TRUE;
  if (song->loop_enabled_SL && (gint64) song->tick_SL <= song->loop_right_SL)
    do
      {
	guint tdiff = song->loop_right_SL - song->tick_SL;
	tdiff = MIN (tdiff, n_ticks);
	if (tdiff)
	  bse_sequencer_process_song_unlooped_SL (song, tdiff, TRUE);
	n_ticks -= tdiff;
	if ((gint64) song->tick_SL >= song->loop_right_SL)
	  {
	    song->tick_SL = song->loop_left_SL;
	  }
      }
    while (n_ticks);
  else
    tracks_active = bse_sequencer_process_song_unlooped_SL (song, n_ticks, FALSE);
  if (!song->sequencer_done_SL && !tracks_active)
    {
      song->sequencer_done_SL = global_sequencer->stamp;
      bse_sequencer_queue_remove_song_SL (song);
    }
}

static void
bse_sequencer_process_part_SL (BsePart         *part,
                               gdouble          start_stamp,
                               guint	        start_tick,
                               guint            bound, /* start_tick + n_ticks */
                               gdouble          stamps_per_tick,
                               BseMidiReceiver *midi_receiver,
                               guint            midi_channel);
static void
bse_sequencer_process_track_SL (BseTrack        *track,
                                gdouble          start_stamp,
                                guint	         start_tick,
                                guint            bound, /* start_tick + n_ticks */
                                gdouble          stamps_per_tick,
                                BseMidiReceiver *midi_receiver)
{
  guint start, next;
  BsePart *part = bse_track_get_part_SL (track, start_tick, &start, &next);
  /* advance to first part */
  if (!part && next)
    {
      part = bse_track_get_part_SL (track, next, &start, &next);
      SEQTRACE ("SEQ:trackjmp: tick=%u fast forward to first part part=%p now=%llu", start_tick, part, gsl_tick_stamp());
    }
  if (!part || (next == 0 && start + part->last_tick_SL < start_tick))
    {
      track->track_done_SL = !bse_midi_receiver_voices_pending (midi_receiver, track->midi_channel_SL);
      SEQTRACE ("SEQ:trackchk: tick=%u next=%u part=%p done=%u now=%llu", // part==NULL || start + (part ? part->last_tick_SL : 0) < start_tick
                start_tick, next, part, track->track_done_SL, gsl_tick_stamp());
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
	bse_sequencer_process_part_SL (part, part_stamp,
                                       part_start, part_bound, stamps_per_tick,
                                       midi_receiver, track->midi_channel_SL);
      part = next ? bse_track_get_part_SL (track, next, &start, &next) : NULL;
    }
}

static void
bse_sequencer_process_part_SL (BsePart         *part,
                               gdouble          start_stamp,
                               guint	        start_tick,
                               guint            tick_bound, /* start_tick + n_ticks */
                               gdouble          stamps_per_tick,
                               BseMidiReceiver *midi_receiver,
                               guint            midi_channel)
{
  BsePartTickNode *node, *last;
  guint channel;
  
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
          if (CHECKTRACE())
            {
              SEQTRACE ("SEQ:note-on:  tick=%llu midinote=%-3d velocity=%02x freq=% 10f now=%llu",
                        eon->delta_time,  note->note, bse_ftoi (note->velocity * 128), freq, gsl_tick_stamp());
              SEQTRACE ("SEQ:note-off: tick=%llu midinote=%-3d velocity=%02x freq=% 10f now=%llu",
                        eoff->delta_time, note->note, bse_ftoi (note->velocity * 128), freq, gsl_tick_stamp());
            }
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
          SEQTRACE ("SEQ:control:  tick=%llu midisignal=%-3d value=%f now=%llu",
                    event->delta_time, cev->ctype, cev->value, gsl_tick_stamp());
        }
      node++;
    }
}
