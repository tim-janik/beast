/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2002 Tim Janik
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
#include "bsesongthread.h"
#include "gslcommon.h"
#include "gslengine.h"
#include "bsetrack.h"
#include "bsepart.h"
#include "bsemidireceiver.h"
#include "bsemain.h"


/* --- prototypes --- */
static void	sequencer_thread	(gpointer		data);
static void	seq_init_SL		(BseSongSequencer      *seq,
					 guint64		cur_tick);
static void	seq_step_SL		(BseSongSequencer      *seq,
					 const guint64		start_tick);
static void	track_step_SL		(BseSongSequencer	*seq,
					 BseSongSequencerTrack *track,
					 const guint	        n_ticks,
					 gdouble		ticks2stamp);


/* --- variables --- */
static GslThread *seq_thread = NULL;
static GslRing   *seq_list = NULL;


/* --- functions --- */
BseSongSequencer*
bse_song_sequencer_setup (BseSong *song)
{
  BseSongSequencer *seq;

  if (!seq_thread)
    {
      seq_thread = gsl_thread_new (sequencer_thread, NULL);
      if (!seq_thread)
	g_error (G_STRLOC ": failed to create sequencer thread");
    }

  seq = g_new0 (BseSongSequencer, 1);
  seq->song = song;
  seq->next_stamp = 0;
  seq->n_tracks = 0;
  seq->tracks = NULL;

  BSE_SEQUENCER_LOCK ();
  seq_list = gsl_ring_prepend (seq_list, seq);
  gsl_thread_wakeup (seq_thread);
  BSE_SEQUENCER_UNLOCK ();

  return seq;
}

void
bse_song_sequencer_destroy (BseSongSequencer *seq)
{
  BSE_SEQUENCER_LOCK ();
  seq_list = gsl_ring_remove (seq_list, seq);
  BSE_SEQUENCER_UNLOCK ();

  g_free (seq->tracks);
  g_free (seq);

  if (!seq_list)
    {
      gsl_thread_abort (seq_thread);
      seq_thread = NULL;
    }
}

static void
sequencer_thread (gpointer data)
{
  g_printerr ("SST: start\n");
  do
    {
      const guint64 cur_stamp = gsl_tick_stamp ();
      guint seq_leap = gsl_engine_block_size () * 10;
      guint64 awake = G_MAXUINT64;
      GslRing *ring;
      
      BSE_SEQUENCER_LOCK ();
      for (ring = seq_list; ring; ring = gsl_ring_walk (seq_list, ring))
	{
	  BseSongSequencer *seq = ring->data;
	  guint64 future_stamp = cur_stamp + seq_leap * 2;
	  
	  if (!seq->next_stamp)	
	    seq_init_SL (seq, cur_stamp + seq_leap);

	  g_assert (seq->cur_stamp <= seq->next_stamp);

	  seq->next_stamp = MAX (seq->next_stamp, future_stamp);
	  
	  while (seq->cur_stamp < seq->next_stamp)
	    {
	      guint64 check_stamp = seq->cur_stamp;
	      
	      seq_step_SL (seq, seq->next_stamp - seq->cur_stamp);
	      if (seq->cur_stamp <= check_stamp)
		{
		  // g_printerr ("sequencer %p didn't advance tick count (%llu <= %llu)\n", seq, seq->cur_stamp, check_stamp);
		  break;
		}
	    }

	  awake = MIN (awake, seq->next_stamp - seq_leap);
	}
      BSE_SEQUENCER_UNLOCK ();

      if (awake != G_MAXUINT64)
	gsl_thread_awake_before (awake);
    }
  while (gsl_thread_sleep (-1));
  g_printerr ("SST: end\n");
}

static void
seq_init_SL (BseSongSequencer *seq,
	     const guint64     start_stamp)
{
  g_assert (seq->next_stamp == 0 && start_stamp > 0);

  seq->start_stamp = start_stamp;
  seq->cur_stamp = start_stamp;
  seq->next_stamp = start_stamp;
  seq->beats_per_second = seq->song->bpm;
  seq->beats_per_second /= 60.;
  seq->beat_tick = 0;
  if (seq->song->tracks)
    {
      BseTrack *track = seq->song->tracks->data;
      if (track->part_SL)
	{
	  seq->n_tracks = 1;
	  seq->tracks = g_new (BseSongSequencerTrack, seq->n_tracks);
	  seq->tracks[0].part = track->part_SL;
	  seq->tracks[0].midi_receiver = track->midi_receiver_SL;
	  seq->tracks[0].tick = 0;
	}
    }
}

static void
seq_step_SL (BseSongSequencer *seq,
	     const guint64     stamp_diff)
{
  gdouble pps, stamp_inc, ticks2stamp, ppqn = 384; // FIXME: track->ppqn
  guint beat_ticks;

  /* calc bpm tick increment */
  pps = ppqn * seq->beats_per_second;
  ticks2stamp = ((gfloat) gsl_engine_sample_freq()) / pps;
  beat_ticks = stamp_diff / ticks2stamp;

  /* process bpm ticks */
  if (seq->n_tracks)
    track_step_SL (seq, seq->tracks + 0, beat_ticks, ticks2stamp);

  /* advance bpm ticks */
  seq->beat_tick += beat_ticks;

  /* calc stamp increment */
  stamp_inc = beat_ticks;
  stamp_inc *= ticks2stamp;

  /* advance stamp */
  seq->cur_stamp += stamp_inc;
}

static void
track_step_SL (BseSongSequencer      *seq,
	       BseSongSequencerTrack *track,
	       const guint	      n_ticks,
	       gdouble		      ticks2stamp)
{
  BsePart *part = track->part;
  guint i, tick_bound = track->tick + n_ticks;
  gboolean need_debug;
  
  i = bse_part_node_lookup_SL (part, track->tick);
  while (i < part->n_nodes && part->nodes[i].tick < tick_bound)
    {
      BsePartEvent *ev = part->nodes[i].events;
      guint tick = part->nodes[i].tick;

      for (ev = part->nodes[i].events; ev; ev = ev->any.next)
	if (ev && ev->type == BSE_PART_EVENT_NOTE)
	  {
	    bse_midi_receiver_push_event (track->midi_receiver,
					  bse_midi_event_note_on (0,
								  seq->start_stamp + tick * ticks2stamp,
								  BSE_PART_FREQ (ev->note.ifreq), 1.0));
	    bse_midi_receiver_push_event (track->midi_receiver,
					  bse_midi_event_note_off (0,
								   seq->start_stamp + (tick + ev->note.duration) * ticks2stamp,
								   BSE_PART_FREQ (ev->note.ifreq)));
	    g_print ("note: %f till %f freq=%f\n",
		     seq->start_stamp + tick * ticks2stamp,
		     seq->start_stamp + (tick + ev->note.duration) * ticks2stamp,
		     BSE_PART_FREQ (ev->note.ifreq));
	  }
      i = bse_part_node_lookup_SL (part, tick + 1);
    }
  track->tick += n_ticks;
  need_debug=track->midi_receiver->events!= NULL;
  bse_midi_receiver_process_events (track->midi_receiver, seq->start_stamp + tick_bound * ticks2stamp);
if (need_debug)  g_print ("process until: %f (current=%llu next=%llu)\n",
	   seq->start_stamp + tick_bound * ticks2stamp,
	   gsl_tick_stamp (),
	   gsl_tick_stamp () + gsl_engine_block_size ());
}
