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
#include "gslengine.h"
#include "bsetrack.h"
#include "bsepart.h"
#include "bsemidireceiver.h"
#include "bsemain.h"


#define DEBUG	sfi_nodebug_with_key ("sequencer")


/* --- prototypes --- */
static void	bse_ssequencer_thread		(gpointer	 data);
static void	bse_ssequencer_process_song_SL	(BseSong	*song,
						 guint		 n_ticks);


/* --- variables --- */
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

  seq_thread = sfi_thread_run ("BseSSequencer", bse_ssequencer_thread, NULL);
  if (!seq_thread)
    g_error ("failed to create sequencer thread");
}

static BseSSequencerJob*
bse_ssequencer_add_super (BseSuper *super)
{
  BseSong *song = BSE_SONG (super);	// FIXME
  BseSSequencerJob *job;

  g_return_val_if_fail (song->sequencer_pending_SL == FALSE, NULL);

  job = sfi_new_struct0 (BseSSequencerJob, 1);
  job->type = BSE_SSEQUENCER_JOB_ADD;
  job->super = super;
  song->sequencer_pending_SL = TRUE;
  job->stamp = 0;
  return job;
}

void
bse_ssequencer_start_supers (SfiRing  *supers,
			     GslTrans *trans)
{
  SfiRing *ring, *jobs = NULL;
  for (ring = supers; ring; ring = sfi_ring_walk (ring, supers))
    {
      BseSuper *super = ring->data;
      if (BSE_IS_SONG (super))
	{
	  if (BSE_SONG (super)->sequencer_pending_SL)
	    g_warning ("%s: song (%p) already in sequencer", G_STRLOC, super);
	  else
	    jobs = sfi_ring_append (jobs, bse_ssequencer_add_super (super));
	}
    }
  if (jobs)
    {
      SfiTime start_stamp = bse_ssequencer_queue_jobs (jobs);
      /* make sure the transaction isn't processed prematurely */
      if (trans)
	gsl_trans_commit_delayed (trans, start_stamp);
    }
  else if (trans)
    gsl_trans_commit (trans);
}

BseSSequencerJob*
bse_ssequencer_job_stop_super (BseSuper *super)
{
  BseSSequencerJob *job = NULL;

  g_return_val_if_fail (BSE_IS_SUPER (super), NULL);

  job = sfi_new_struct0 (BseSSequencerJob, 1);
  job->type = BSE_SSEQUENCER_JOB_NOP;
  if (BSE_IS_SONG (super))
    {
      BseSong *song = BSE_SONG (super); // FIXME: use supers here
      if (song->sequencer_pending_SL == TRUE)
	{
	  job->type = BSE_SSEQUENCER_JOB_REMOVE;
	  job->super = super;
	  job->stamp = 0;
	}
    }
  return job;
}

static gint
jobs_cmp (gconstpointer a,
	  gconstpointer b)
{
  const BseSSequencerJob *job1 = a;
  const BseSSequencerJob *job2 = b;
  return job1->stamp < job2->stamp ? -1 : job1->stamp > job2->stamp;
}

void
bse_ssequencer_remove_super_SL (BseSuper *super)
{
  BseSong *song = BSE_SONG (super);	// FIXME
  self->supers = sfi_ring_remove (self->supers, super);
  song->sequencer_pending_SL = FALSE;
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
	  BseSong *song;
	  BseSuper *super;
	  SfiRing *ring;
	case BSE_SSEQUENCER_JOB_NOP:
	  break;
	case BSE_SSEQUENCER_JOB_ADD:
	  super = job->super;
	  song = BSE_SONG (super); // FIXME
	  song->start_SL = job->stamp;
	  song->delta_stamp_SL = 0;
	  song->tick_SL = 0;
	  song->song_done_SL = FALSE;
	  for (ring = song->tracks_SL; ring; ring = sfi_ring_walk (ring, song->tracks_SL))
	    {
	      BseTrack *track = ring->data;
	      track->track_done_SL = FALSE;
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
      jobs = sfi_ring_sort (jobs, jobs_cmp);
      BSE_SEQUENCER_LOCK ();
      self->jobs = sfi_ring_merge_sorted (self->jobs, jobs, jobs_cmp);
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
bse_ssequencer_thread (gpointer data)
{
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
	  BseSong *song = ring->data;
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
      BSE_SEQUENCER_UNLOCK ();

      sfi_thread_awake_after (cur_stamp + gsl_engine_block_size ());
    }
  while (sfi_thread_sleep (-1));
  DEBUG ("SST: end\n");
}

static void
bse_ssequencer_process_track_SL (BseTrack *track,
				 gdouble   start_stamp,
				 guint	   start_tick,
				 guint     n_ticks,
				 gdouble   stamps_per_tick);
static gboolean
bse_ssequencer_process_song_unlooped_SL (BseSong *song,
					 guint    n_ticks,
					 gboolean force_active_tracks)
{
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
      if (!track->midi_receiver_SL)
	track->track_done_SL = TRUE;
      else if (!track->track_done_SL || force_active_tracks)
	{
	  track->track_done_SL = FALSE;
	  bse_ssequencer_process_track_SL (track, current_stamp,
					   song->tick_SL, tick_bound,
					   stamps_per_tick);
	  bse_midi_receiver_process_events (track->midi_receiver_SL, next_stamp);
	}
      if (track->track_done_SL)
	n_done_tracks++;
    }
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
				BseMidiReceiver *midi_receiver);
static void
bse_ssequencer_process_track_SL (BseTrack *track,
				 gdouble   start_stamp,
				 guint	   start_tick,
				 guint     bound, /* start_tick + n_ticks */
				 gdouble   stamps_per_tick)
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
      track->track_done_SL = !bse_midi_receiver_voices_pending (track->midi_receiver_SL);
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
					track->midi_receiver_SL);
      part = next ? bse_track_get_part_SL (track, next, &start, &next) : NULL;
    }
}

static void
bse_ssequencer_process_part_SL (BsePart         *part,
				gdouble          start_stamp,
				guint	         start_tick,
				guint            bound, /* start_tick + n_ticks */
				gdouble          stamps_per_tick,
				BseMidiReceiver *midi_receiver)
{
  guint i = bse_part_node_lookup_SL (part, start_tick);
  while (i < part->n_nodes && part->nodes[i].tick < bound)
    {
      BsePartEvent *ev = part->nodes[i].events;
      guint etick = part->nodes[i].tick - start_tick;
      for (ev = part->nodes[i].events; ev; ev = ev->any.next)
	if (ev && ev->type == BSE_PART_EVENT_NOTE)
	  {
	    BseMidiEvent *eon, *eoff;
	    eon  = bse_midi_event_note_on (0, start_stamp + etick * stamps_per_tick,
					   BSE_PART_NOTE_EVENT_FREQ (&ev->note), 1.0);
	    eoff = bse_midi_event_note_off (0, start_stamp + (etick + ev->note.duration) * stamps_per_tick,
					    BSE_PART_NOTE_EVENT_FREQ (&ev->note));
	    bse_midi_receiver_push_event (midi_receiver, eon);
	    bse_midi_receiver_push_event (midi_receiver, eoff);
	    DEBUG ("note: %llu till %llu freq=%f (note=%d)",
		   eon->tick_stamp, eoff->tick_stamp, BSE_PART_NOTE_EVENT_FREQ (&ev->note), ev->note.note);
	  }
      i = bse_part_node_lookup_SL (part, part->nodes[i].tick + 1);
    }
}
