/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2004 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bsessequencer.h"
#include "gslcommon.h"
#include "bseengine.h"
#include "bsetrack.h"
#include "bsepart.h"
#include "bseproject.h"
#include "bsemidireceiver.h"
#include "bsemain.h"


#define DEBUG(...)      sfi_debug ("sequencer", __VA_ARGS__)


/* --- prototypes --- */
static void	bse_ssequencer_thread_main	(gpointer	 data);
static void	bse_ssequencer_process_song_SL	(BseSong	*song,
						 guint		 n_ticks);


/* --- variables --- */
SfiThread            *bse_ssequencer_thread = NULL;
static BseSSequencer *global_sequencer = NULL;


/* --- functions --- */
void
bse_ssequencer_init_thread (void)
{
  g_assert (bse_ssequencer_thread == NULL);

  /* initialize BseSSequencer */
  static BseSSequencer sseq = { 0, };
  sseq.stamp = gsl_tick_stamp ();
  g_assert (sseq.stamp > 0);
  global_sequencer = &sseq;

  bse_ssequencer_thread = sfi_thread_run ("Sequencer", bse_ssequencer_thread_main, NULL);
  if (!bse_ssequencer_thread)
    g_error ("failed to create sequencer thread");
}

void
bse_ssequencer_start_song (BseSong        *song,
                           guint64         start_stamp)
{
  g_assert (bse_ssequencer_thread != NULL);
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
      BseTrack *track = ring->data;
      track->track_done_SL = FALSE;
    }
  global_sequencer->songs = sfi_ring_append (global_sequencer->songs, song);
  BSE_SEQUENCER_UNLOCK ();
  sfi_thread_wakeup (bse_ssequencer_thread);
}

void
bse_ssequencer_remove_song (BseSong *song)
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
bse_ssequencer_remove_song_async (gpointer data)        /* UserThread */
{
  BseSong *song = BSE_SONG (data);
  if (BSE_SOURCE_PREPARED (song) &&     /* project might be deactivated already */
      song->sequencer_done_SL)          /* song might have been removed and re-added */
    {
      bse_ssequencer_remove_song (song);
      BseProject *project = bse_item_get_project (BSE_ITEM (song));
      bse_project_check_auto_stop (project);
    }
  g_object_unref (song);                        /* sequencer_owns_refcount_SL = FALSE from bse_ssequencer_queue_remove_song_SL() */
  return FALSE;
}

static void
bse_ssequencer_queue_remove_song_SL (BseSong *song)
{
  g_return_if_fail (song->sequencer_owns_refcount_SL == TRUE);
  song->sequencer_owns_refcount_SL = FALSE;     /* g_object_unref() in bse_ssequencer_remove_song_async() */
  bse_idle_now (bse_ssequencer_remove_song_async, song);
}

static void
bse_ssequencer_thread_main (gpointer data)
{
  DEBUG ("SST: start\n");
  do
    {
      const guint64 cur_stamp = gsl_tick_stamp ();
      guint64 next_stamp = cur_stamp + BSE_SSEQUENCER_PREPROCESS;
      SfiRing *ring;
      
      BSE_SEQUENCER_LOCK ();
      for (ring = global_sequencer->songs; ring; ring = sfi_ring_walk (ring, global_sequencer->songs))
	{
          BseSong *song = BSE_SONG (ring->data);
          if (!song->sequencer_start_SL && song->sequencer_start_request_SL <= cur_stamp)
            song->sequencer_start_SL = cur_stamp;
          if (song->sequencer_start_SL && !song->sequencer_done_SL)
            {
              gdouble stamp_diff = (next_stamp - song->sequencer_start_SL) - song->delta_stamp_SL;
	      while (stamp_diff > 0)
		{
		  guint n_ticks = stamp_diff * song->tpsi_SL;
		  if (n_ticks < 1)
		    break;
		  bse_ssequencer_process_song_SL (song, n_ticks);
		  stamp_diff = (next_stamp - song->sequencer_start_SL) - song->delta_stamp_SL;
		}
	    }
	}
      global_sequencer->stamp = next_stamp;
      BSE_SEQUENCER_UNLOCK ();

      sfi_thread_awake_after (cur_stamp + bse_engine_block_size ());
    }
  while (sfi_thread_sleep (-1));
  DEBUG ("SST: end\n");
}

gboolean
bse_sequencer_thread_lagging (void)
{
  const guint64 cur_stamp = gsl_tick_stamp ();
  guint64 next_stamp = cur_stamp + BSE_SSEQUENCER_PREPROCESS;
  BSE_SEQUENCER_LOCK ();
  gboolean lagging = global_sequencer->stamp < next_stamp;
  BSE_SEQUENCER_UNLOCK ();
  return lagging;
}

static void
bse_ssequencer_process_track_SL (BseTrack        *track,
                                 gdouble          start_stamp,
                                 guint            start_tick,
                                 guint            n_ticks,
                                 gdouble          stamps_per_tick,
                                 BseMidiReceiver *midi_receiver);

static gboolean
bse_ssequencer_process_song_unlooped_SL (BseSong *song,
					 guint    n_ticks,
					 gboolean force_active_tracks)
{
  BseMidiReceiver *midi_receiver = song->midi_receiver_SL;
  gdouble current_stamp = song->sequencer_start_SL + song->delta_stamp_SL;
  gdouble stamps_per_tick = 1.0 / song->tpsi_SL;
  guint64 next_stamp = current_stamp + n_ticks * stamps_per_tick;
  guint tick_bound = song->tick_SL + n_ticks;
  guint n_done_tracks = 0, n_tracks = 0;
  SfiRing *ring;
  for (ring = song->tracks_SL; ring; ring = sfi_ring_walk (ring, song->tracks_SL))
    {
      BseTrack *track = ring->data;
      n_tracks++;
      if (!track->track_done_SL || force_active_tracks)
	{
	  track->track_done_SL = FALSE;
	  bse_ssequencer_process_track_SL (track, current_stamp,
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
bse_ssequencer_process_song_SL (BseSong *song,
				guint    n_ticks)
{
  gboolean tracks_active = TRUE;
  if (song->loop_enabled_SL && song->tick_SL <= song->loop_right_SL)
    do
      {
	guint tdiff = song->loop_right_SL - song->tick_SL;
	tdiff = MIN (tdiff, n_ticks);
	if (tdiff)
	  bse_ssequencer_process_song_unlooped_SL (song, tdiff, TRUE);
	n_ticks -= tdiff;
	if (song->tick_SL >= song->loop_right_SL)
	  {
	    song->tick_SL = song->loop_left_SL;
	  }
      }
    while (n_ticks);
  else
    tracks_active = bse_ssequencer_process_song_unlooped_SL (song, n_ticks, FALSE);
  if (!song->sequencer_done_SL && !tracks_active)
    {
      song->sequencer_done_SL = global_sequencer->stamp;
      bse_ssequencer_queue_remove_song_SL (song);
    }
}

static void
bse_ssequencer_process_part_SL (BsePart         *part,
				gdouble          start_stamp,
				guint	         start_tick,
				guint            bound, /* start_tick + n_ticks */
				gdouble          stamps_per_tick,
				BseMidiReceiver *midi_receiver,
                                guint            midi_channel);
static void
bse_ssequencer_process_track_SL (BseTrack        *track,
				 gdouble          start_stamp,
				 guint	          start_tick,
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
      DEBUG ("track[%u]: advancing to first part: %p", start_tick, part);
    }
  if (!part || (next == 0 && start + part->last_tick_SL < start_tick))
    {
      DEBUG ("track[%u]: could be done: %p==NULL || %u < %u (next=%u)", start_tick, part, start + (part ? part->last_tick_SL : 0), start_tick, next);
      track->track_done_SL = !bse_midi_receiver_voices_pending (midi_receiver, track->midi_channel_SL);
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
	bse_ssequencer_process_part_SL (part, part_stamp,
					part_start, part_bound, stamps_per_tick,
					midi_receiver, track->midi_channel_SL);
      part = next ? bse_track_get_part_SL (track, next, &start, &next) : NULL;
    }
}

static void
bse_ssequencer_process_part_SL (BsePart         *part,
				gdouble          start_stamp,
				guint	         start_tick,
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
          gfloat freq = BSE_PART_NOTE_FREQ (note);
          eon  = bse_midi_event_note_on (midi_channel,
                                         start_stamp + (note->tick - start_tick) * stamps_per_tick,
                                         freq, note->velocity);
          eoff = bse_midi_event_note_off (midi_channel,
                                          start_stamp + (note->tick - start_tick + note->duration) * stamps_per_tick,
                                          freq);
          bse_midi_receiver_push_event (midi_receiver, eon);
          bse_midi_receiver_push_event (midi_receiver, eoff);
          DEBUG ("note: %llu till %llu freq=%f (note=%d velocity=%f)",
                 eon->delta_time, eoff->delta_time, freq,
                 note->note, note->velocity);
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
                                                       start_stamp + (node->tick - start_tick) * stamps_per_tick,
                                                       cev->ctype, cev->value);
          bse_midi_receiver_push_event (midi_receiver, event);
          DEBUG ("control: %llu signal=%d (value=%f)",
                 event->delta_time, cev->ctype, cev->value);
        }
      node++;
    }
}
