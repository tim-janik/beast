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


/* --- prototypes --- */
static void	bse_ssequencer_thread		(gpointer	 data);
static void	bse_ssequencer_process_song_SL	(BseSong	*song,
						 guint		 n_ticks);


/* --- variables --- */
static BseSSequencer *self = NULL;
static SfiThread     *seq_thread = NULL;


/* --- functions --- */
void
bse_ssequencer_start (void)
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

BseSSequencerJob*
bse_ssequencer_add_song (BseSong         *song)
{
  BseSSequencerJob *job;

  g_return_val_if_fail (BSE_IS_SONG (song), NULL);

  job = sfi_new_struct0 (BseSSequencerJob, 1);
  job->type = BSE_SSEQUENCER_JOB_ADD;
  job->song = song;
  job->stamp = 0;
  return job;
}

BseSSequencerJob*
bse_ssequencer_remove_song (BseSong *song)
{
  BseSSequencerJob *job;

  g_return_val_if_fail (BSE_IS_SONG (song), NULL);

  job = sfi_new_struct0 (BseSSequencerJob, 1);
  job->type = BSE_SSEQUENCER_JOB_REMOVE;
  job->song = song;
  job->stamp = 0;
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
	case BSE_SSEQUENCER_JOB_ADD:
	  song = job->song;
	  song->start_SL = job->stamp;
	  song->delta_stamp_SL = 0;
	  song->tick_SL = 0;
	  self->songs = sfi_ring_push_tail (self->songs, song);
	  break;
	case BSE_SSEQUENCER_JOB_REMOVE:
	  song = job->song;
	  self->songs = sfi_ring_remove (self->songs, song);
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
  g_printerr ("SST: start\n");
  do
    {
      const SfiTime cur_stamp = gsl_tick_stamp ();
      SfiTime next_stamp = cur_stamp + BSE_SSEQUENCER_PREPROCESS;
      SfiRing *ring;

      BSE_SEQUENCER_LOCK ();
      bse_ssequencer_handle_jobs_SL (next_stamp);
      for (ring = self->songs; ring; ring = sfi_ring_walk (ring, self->songs))
	{
	  BseSong *song = ring->data;
	  gdouble stamp_diff = (next_stamp - song->start_SL) - song->delta_stamp_SL;
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
  g_printerr ("SST: end\n");
}

static void
bse_ssequencer_process_song_SL (BseSong *song,
				guint    n_ticks)
{
  guint current_tick = song->tick_SL;
  guint tick_bound = current_tick + n_ticks;
  gdouble stamp_delta = n_ticks / song->tpsi_SL;
  SfiTime next_stamp = song->start_SL + song->delta_stamp_SL + stamp_delta;
  SfiRing *ring;

  for (ring = song->tracks_SL; ring; ring = sfi_ring_walk (ring, song->tracks_SL))
    {
      BseTrack *track = ring->data;
      BsePart *part = track->part_SL;
      BseMidiReceiver *midi_receiver = track->midi_receiver_SL;
      guint i;
      if (!part || !midi_receiver)
	continue;
      i = bse_part_node_lookup_SL (part, current_tick);
      while (i < part->n_nodes && part->nodes[i].tick < tick_bound)
	{
	  BsePartEvent *ev = part->nodes[i].events;
	  guint tick = part->nodes[i].tick;
	  for (ev = part->nodes[i].events; ev; ev = ev->any.next)
	    if (ev && ev->type == BSE_PART_EVENT_NOTE)
	      {
		BseMidiEvent *eon, *eoff;
		eon  = bse_midi_event_note_on (0,
					       song->start_SL +
					       song->delta_stamp_SL +
					       (tick - current_tick) / song->tpsi_SL,
					       BSE_PART_NOTE_EVENT_FREQ (&ev->note),
					       1.0);
		eoff = bse_midi_event_note_off (0,
						song->start_SL +
						song->delta_stamp_SL +
						(tick - current_tick + ev->note.duration) / song->tpsi_SL,
						BSE_PART_NOTE_EVENT_FREQ (&ev->note));
		bse_midi_receiver_push_event (midi_receiver, eon);
		bse_midi_receiver_push_event (midi_receiver, eoff);
		g_print ("note: %llu till %llu freq=%f (note=%d)\n",
			 eon->tick_stamp,
			 eoff->tick_stamp,
			 BSE_PART_NOTE_EVENT_FREQ (&ev->note),
			 ev->note.note);
	      }
	  i = bse_part_node_lookup_SL (part, tick + 1);
	}
      bse_midi_receiver_process_events (midi_receiver, next_stamp);
    }
  song->tick_SL += n_ticks;
  song->delta_stamp_SL += stamp_delta;
}
