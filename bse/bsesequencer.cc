/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2003 Tim Janik
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
#include "bsemidireceiver.h"
#include "bsemain.h"


#define DEBUG(...)      sfi_debug ("sequencer", __VA_ARGS__)


/* --- prototypes --- */
static void	bse_ssequencer_thread_main	(gpointer	 data);
static void	bse_ssequencer_process_song_SL	(BseSong	*song,
						 guint		 n_ticks);


/* --- variables --- */
SfiThread            *bse_ssequencer_thread = NULL;
static BseSSequencer *self = NULL;
static SfiThread     *seq_thread = NULL;


/* --- functions --- */
void
bse_ssequencer_init_thread (void)
{
  static BseSSequencer sseq = { 0, };

  g_assert (self == NULL);

  self = &sseq;
  self->stamp = gsl_tick_stamp ();
  g_assert (self->stamp > 0);

  seq_thread = sfi_thread_run ("Sequencer", bse_ssequencer_thread_main, NULL);
  if (!seq_thread)
    g_error ("failed to create sequencer thread");
}

static BseSSequencerJob*
bse_ssequencer_add_super (BseSuper *super)
{
  BseSSequencerJob *job;

  g_return_val_if_fail (super->sequencer_pending_SL == FALSE, NULL);

  job = sfi_new_struct0 (BseSSequencerJob, 1);
  job->type = BSE_SSEQUENCER_JOB_ADD;
  job->super = super;
  super->sequencer_pending_SL = TRUE;
  job->stamp = 0;
  return job;
}

void
bse_ssequencer_start_supers (SfiRing  *supers,
			     BseTrans *trans)
{
  SfiRing *ring, *jobs = NULL;
  for (ring = supers; ring; ring = sfi_ring_walk (ring, supers))
    {
      BseSuper *super = ring->data;
      g_return_if_fail (BSE_IS_SUPER (super));
      if (super->sequencer_pending_SL)
	g_warning ("%s: module %s already in sequencer", G_STRLOC, bse_object_debug_name (super));
      else
	jobs = sfi_ring_append (jobs, bse_ssequencer_add_super (super));
    }
  if (jobs)
    {
      SfiTime start_stamp = bse_ssequencer_queue_jobs (jobs);
      /* make sure the transaction isn't processed prematurely */
      if (trans)
	bse_trans_commit_delayed (trans, start_stamp);
    }
  else if (trans)
    bse_trans_commit (trans);
}

BseSSequencerJob*
bse_ssequencer_job_stop_super (BseSuper *super)
{
  BseSSequencerJob *job = NULL;

  g_return_val_if_fail (BSE_IS_SUPER (super), NULL);

  job = sfi_new_struct0 (BseSSequencerJob, 1);
  job->type = BSE_SSEQUENCER_JOB_NOP;
  if (super->sequencer_pending_SL == TRUE)
    {
      job->type = BSE_SSEQUENCER_JOB_REMOVE;
      job->super = super;
      job->stamp = 0;
    }
  return job;
}

static gint
jobs_cmp (gconstpointer a,
	  gconstpointer b,
          gpointer      data)
{
  const BseSSequencerJob *job1 = a;
  const BseSSequencerJob *job2 = b;
  return job1->stamp < job2->stamp ? -1 : job1->stamp > job2->stamp;
}

void
bse_ssequencer_remove_super_SL (BseSuper *super)
{
  g_return_if_fail (BSE_IS_SUPER (super));
  self->supers = sfi_ring_remove (self->supers, super);
  super->sequencer_pending_SL = FALSE;
}

static void
bse_ssequencer_handle_jobs_SL (SfiTime next_stamp)
{
  BseSSequencerJob *job = self->jobs ? self->jobs->data : NULL;
  while (job)
    {
      if (job->stamp > next_stamp)
	return;
      job = sfi_ring_pop_head (&self->jobs); /* same job, but remove from list */
      switch (job->type)
	{
	  BseSuper *super;
	  SfiRing *ring;
	case BSE_SSEQUENCER_JOB_NOP:
	  break;
	case BSE_SSEQUENCER_JOB_ADD:
	  super = job->super;
	  if (BSE_IS_SONG (super))	// FIXME
	    {
	      BseSong *song = BSE_SONG (super);
	      song->start_SL = job->stamp;
	      song->delta_stamp_SL = 0;
	      song->tick_SL = 0;
	      song->song_done_SL = FALSE;
	      for (ring = song->tracks_SL; ring; ring = sfi_ring_walk (ring, song->tracks_SL))
		{
		  BseTrack *track = ring->data;
		  track->track_done_SL = FALSE;
		}
	    }
	  self->supers = sfi_ring_push_tail (self->supers, super);
	  break;
	case BSE_SSEQUENCER_JOB_REMOVE:
	  bse_ssequencer_remove_super_SL (job->super);
	  break;
	default:
	  g_warning ("%s: unhandled job type: %u", G_STRLOC, job->type);
	}
      sfi_delete_struct (BseSSequencerJob, job);
      job = self->jobs ? self->jobs->data : NULL;
    }
}

static SfiTime
bse_ssequencer_queue_jobs_internal (SfiRing *jobs,
				    gboolean process_now)
{
  SfiTime stamp = gsl_tick_stamp () + BSE_SSEQUENCER_PREPROCESS * 1.5;
  if (jobs)
    {
      SfiRing *tmp;
      /* shift start stamps to earliest possible stamp */
      for (tmp = jobs; tmp; tmp = sfi_ring_walk (tmp, jobs))
	{
	  BseSSequencerJob *job = tmp->data;
	  if (job->type == BSE_SSEQUENCER_JOB_ADD)
	    job->stamp = MAX (job->stamp, stamp);
	}
      jobs = sfi_ring_sort (jobs, jobs_cmp, NULL);
      BSE_SEQUENCER_LOCK ();
      self->jobs = sfi_ring_merge_sorted (self->jobs, jobs, jobs_cmp, NULL);
      if (process_now)
	bse_ssequencer_handle_jobs_SL (gsl_tick_stamp ());
      BSE_SEQUENCER_UNLOCK ();
    }
  return stamp;
}

SfiTime
bse_ssequencer_queue_jobs (SfiRing *jobs)
{
  SfiTime stamp;
  /* returns the earliest possible playback stamp */
  stamp = bse_ssequencer_queue_jobs_internal (jobs, FALSE);
  sfi_thread_wakeup (seq_thread);
  return stamp;
}

void
bse_ssequencer_handle_jobs (SfiRing *jobs)
{
  /* we basically do the same as bse_ssequencer_queue_jobs(),
   * except that removal jobs are processed immediately
   */
  bse_ssequencer_queue_jobs_internal (jobs, TRUE);
}

static void
bse_ssequencer_thread_main (gpointer data)
{
  bse_ssequencer_thread = sfi_thread_self ();
  DEBUG ("SST: start\n");
  do
    {
      const SfiTime cur_stamp = gsl_tick_stamp ();
      SfiTime next_stamp = cur_stamp + BSE_SSEQUENCER_PREPROCESS;
      SfiRing *ring;

      BSE_SEQUENCER_LOCK ();
      bse_ssequencer_handle_jobs_SL (next_stamp);
      for (ring = self->supers; ring; )
	{
	  BseSuper *super = ring->data;
	  if (BSE_IS_SONG (super))
	    {
	      BseSong *song = BSE_SONG (super);
	      gdouble stamp_diff = (next_stamp - song->start_SL) - song->delta_stamp_SL;
	      ring = sfi_ring_walk (ring, self->supers);	/* list may be modified */
	      while (stamp_diff > 0)
		{
		  guint n_ticks = stamp_diff * song->tpsi_SL;
		  if (n_ticks < 1)
		    break;
		  bse_ssequencer_process_song_SL (song, n_ticks);
		  stamp_diff = (next_stamp - song->start_SL) - song->delta_stamp_SL;
		}
	    }
	  else
	    ring = sfi_ring_walk (ring, self->supers);
	}
      BSE_SEQUENCER_UNLOCK ();

      sfi_thread_awake_after (cur_stamp + bse_engine_block_size ());
    }
  while (sfi_thread_sleep (-1));
  DEBUG ("SST: end\n");
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
  gdouble current_stamp = song->start_SL + song->delta_stamp_SL;
  gdouble stamps_per_tick = 1.0 / song->tpsi_SL;
  SfiTime next_stamp = current_stamp + n_ticks * stamps_per_tick;
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
  if (!song->song_done_SL)
    {
      song->song_done_SL = !tracks_active;
      if (song->song_done_SL)
	bse_song_stop_sequencing_SL (song);
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
