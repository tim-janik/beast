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
      GslRing *ring;
      guint64 pre_tick, cur_tick = gsl_tick_stamp ();

      pre_tick = gsl_engine_block_size ();
      pre_tick = cur_tick > pre_tick ? cur_tick - pre_tick : cur_tick;

      GSL_SPIN_LOCK (&seq_mutex);
      for (ring = seq_list; ring; ring = gsl_ring_walk (seq_list, ring))
	{
	  BseSongSequencer *seq = ring->data;

	  if (!seq->next_tick)	
	    seq_init (seq, cur_tick);

	  if (seq->next_tick <= pre_tick)
	    seq_step (seq, cur_tick);

	  gsl_thread_awake_after (seq->next_tick);
	}
      GSL_SPIN_UNLOCK (&seq_mutex);
    }
  while (gsl_thread_sleep (-1));
  g_printerr ("SST: end\n");
}

static void
seq_init (BseSongSequencer *seq,
	  guint64           cur_tick)
{
  g_assert (seq->next_tick == 0 && cur_tick > 0);

  seq->next_tick = cur_tick;
}

static void
seq_step (BseSongSequencer *seq,
	  guint64           cur_tick)
{
  BseSong *song = seq->song;
  BseSongNet *snet = &song->net;
  BsePattern *pattern;
  BsePatternNote *note;

  pattern = song->patterns->data;

  note = bse_pattern_peek_note (pattern, 0, seq->row);

  g_object_set (snet->voices[0].ofreq, "note_1", note->note, NULL);

  g_printerr ("SST: tick(%llu): note %u instr %p\n", cur_tick, note->note, note->instrument);

  /* next step setup */
  seq->row++;
  if (seq->row >= song->pattern_length)
    seq->row = 0;
  seq->next_tick += gsl_engine_sample_freq () * (60. / 4.) / (double) seq->song->bpm;
}
