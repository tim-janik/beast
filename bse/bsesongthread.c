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
#include "bsepattern.h"
#include "bseconstant.h"


/* --- prototypes --- */
static void	sequencer_thread	(gpointer		 data);
static void	seq_init		(BseSongSequencer	*seq,
					 guint64		 cur_tick);
static void	seq_step		(BseSongSequencer	*seq,
					 guint64		 cur_tick);


/* --- variables --- */
static GslThread *seq_thread = NULL;
static GslMutex   seq_mutex;
static GslRing   *seq_list = NULL;


/* --- functions --- */
BseSongSequencer*
bse_song_sequencer_setup (BseSong *song)
{
  BseSongSequencer *seq;

  if (!seq_thread)
    {
      gsl_mutex_init (&seq_mutex);
      seq_thread = gsl_thread_new (sequencer_thread, NULL);
      if (!seq_thread)
	g_error (G_STRLOC ": failed to create sequencer thread");
    }

  seq = g_new0 (BseSongSequencer, 1);
  seq->song = song;
  seq->next_tick = 0;
  
  GSL_SYNC_LOCK (&seq_mutex);
  seq_list = gsl_ring_prepend (seq_list, seq);
  gsl_thread_wakeup (seq_thread);
  GSL_SYNC_UNLOCK (&seq_mutex);

  return seq;
}

void
bse_song_sequencer_destroy (BseSongSequencer *seq)
{
  GSL_SYNC_LOCK (&seq_mutex);
  seq_list = gsl_ring_remove (seq_list, seq);
  GSL_SYNC_UNLOCK (&seq_mutex);

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
      const guint64 cur_tick = gsl_tick_stamp ();
      GslRing *ring;
      guint tick_latency = gsl_engine_block_size () * 10;
      guint64 future_tick = cur_tick + tick_latency;

      GSL_SPIN_LOCK (&seq_mutex);
      for (ring = seq_list; ring; ring = gsl_ring_walk (seq_list, ring))
	{
	  BseSongSequencer *seq = ring->data;

	  if (!seq->next_tick)	
	    seq_init (seq, cur_tick);

	  while (seq->next_tick < future_tick)
	    seq_step (seq, cur_tick);

	  gsl_thread_awake_before (MAX (seq->next_tick, tick_latency) - tick_latency);
	}
      GSL_SPIN_UNLOCK (&seq_mutex);
    }
  while (gsl_thread_sleep (-1));
  g_printerr ("SST: end\n");
}

static void
seq_init (BseSongSequencer *seq,
	  const guint64     cur_tick)
{
  g_assert (seq->next_tick == 0 && cur_tick > 0);

  seq->next_tick = cur_tick;
}

static void
seq_step (BseSongSequencer *seq,
	  const guint64     cur_tick)
{
  BseSong *song = seq->song;
  BseSongNet *snet = &song->net;
  BsePattern *pattern;
  BsePatternNote *note;
  guint64 next_tick = seq->next_tick;

  pattern = song->patterns->data;

  note = bse_pattern_peek_note (pattern, 0, seq->row);

  bse_constant_stamped_set_note (BSE_CONSTANT (snet->voices[0].ofreq), next_tick, 0, note->note);
  bse_constant_stamped_set_float (BSE_CONSTANT (snet->voices[0].ofreq), next_tick, 1, 1.0);
  bse_constant_stamped_set_float (BSE_CONSTANT (snet->voices[0].ofreq), next_tick + 1, 1, 0.0);

  g_printerr ("SST: tick(for:%llu at:%lld): note %u instr %p\n", next_tick, cur_tick, note->note, note->instrument);

  /* next step setup */
  seq->row++;
  if (seq->row >= song->pattern_length)
    seq->row = 0;
  seq->next_tick += gsl_engine_sample_freq () * (60. / 4.) / (double) seq->song->bpm;
}
