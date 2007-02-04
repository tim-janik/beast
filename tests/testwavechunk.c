/* GSL - Generic Sound Layer
 * Copyright (C) 2001-2003 Tim Janik
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
#include <bse/gslwavechunk.h>
#include <bse/gsldatahandle.h>
#include <bse/bsemain.h>
#include <sfi/sfitests.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

enum {
  VERBOSITY_NONE,
  VERBOSITY_SETUP,
  VERBOSITY_BLOCKS,
  VERBOSITY_DATA,
  VERBOSITY_PADDING,
  VERBOSITY_CHECKS,
};
static guint verbosity = VERBOSITY_NONE;

static gfloat my_data[] = {
  0.555555555,1,2,3,4,5,6,7,8,9,
  10,11,12,13,14,15,16,17,18,19,
  20,21,22,23,24,25,26,27,28,29,
  30,31,32,33,34,35,36,
};
static guint  my_data_length = sizeof (my_data) / sizeof (my_data[0]);

static void	print_block (GslWaveChunk      *wchunk,
			     GslWaveChunkBlock *block);

#define DEBUG_SIZE	(1024 * 256)
#define	MINI_DEBUG_SIZE	(16)

static void
run_loop_test (GslWaveLoopType loop_type,
               gint            play_dir,
               gint	       loop_first,
               gint	       loop_last,
               gint	       loop_count)
{
  gfloat *tmpstorage = g_new (gfloat, DEBUG_SIZE);
  gfloat *cmpblock = tmpstorage + DEBUG_SIZE / 2;
  GslDataHandle *myhandle;
  GslDataCache *dcache;
  GslWaveChunkBlock block = { 0, };
  GslWaveChunk *wchunk;
  BseErrorType error;

  myhandle = gsl_data_handle_new_mem (1, 32, 44100, 440, my_data_length, my_data, NULL);
  dcache = gsl_data_cache_new (myhandle, 1);
  gsl_data_handle_unref (myhandle);
  wchunk = gsl_wave_chunk_new (dcache,
                               44100.0, 44.0,
			       loop_type, loop_first, loop_last, loop_count);
  error = gsl_wave_chunk_open (wchunk);
  if (error)
    g_error ("failed to open wave chunk: %s", bse_error_blurb (error));
  gsl_wave_chunk_unref (wchunk);
  if (verbosity >= VERBOSITY_SETUP)
    g_print ("SETUP: loop_type=%u loop_first=%lld loop_last=%lld loop_count=%d playdir=%+d\n",
	     wchunk->loop_type, wchunk->loop_first, wchunk->loop_last, wchunk->loop_count, play_dir);
  gsl_wave_chunk_debug_block (wchunk, - DEBUG_SIZE / 2, DEBUG_SIZE, cmpblock - DEBUG_SIZE / 2);

  block.play_dir = play_dir;

  block.offset = block.play_dir < 0 ? wchunk->wave_length + MINI_DEBUG_SIZE/2 : -MINI_DEBUG_SIZE/2;
  while (block.offset < wchunk->wave_length + MINI_DEBUG_SIZE &&
	 block.offset > -MINI_DEBUG_SIZE)
    {
      gint i, start, end, abort;

      gsl_wave_chunk_use_block (wchunk, &block);

      print_block (wchunk, &block);
      if (block.play_dir > 0)
	{
	  start = block.offset - wchunk->n_pad_values;
	  end = block.offset + block.length + wchunk->n_pad_values;
	}
      else
	{
	  start = block.offset + wchunk->n_pad_values;
	  end = block.offset - block.length - wchunk->n_pad_values;
	}
      abort = FALSE;
      for (i = start; i != end; i += block.play_dir)
	{
	  gfloat v = (block.play_dir < 0) ^ (block.dirstride > 0) ? block.start[i - block.offset] : block.start[block.offset - i];

	  if (fabs (cmpblock[i] - v) > 1e-15)
	    {
	      abort = TRUE;
	      verbosity = 99;
	    }
	  if (verbosity >= VERBOSITY_CHECKS)
	    g_print ("%s: offset=%d (block.offset=%lld) value=%.16f found=%.16f\n",
		     fabs (cmpblock[i] - v) > 1e-15 ? "MISMATCH" : "match",
		     i, (i - block.offset), cmpblock[i], v);
	}
      if (abort)
	{
	  g_error ("mismatches occoured, setup: loop_type=%u loop_first=%lld loop_last=%lld loop_count=%d (length=%lld)",
		   wchunk->loop_type, wchunk->loop_first, wchunk->loop_last, wchunk->loop_count,
		   gsl_data_handle_length (wchunk->dcache->dhandle));
	}

      gsl_wave_chunk_unuse_block (wchunk, &block);

      block.offset = block.next_offset;
      /* block.offset += block.play_dir; */
    }
  gsl_wave_chunk_close (wchunk);
  gsl_data_cache_unref (dcache);

  g_free (tmpstorage);
}

static void
print_block (GslWaveChunk      *wchunk,
	     GslWaveChunkBlock *block)
{
  gfloat *p = NULL;
  guint i;

  if (verbosity >= VERBOSITY_BLOCKS)
    {
      g_print ("BLOCK:");
      g_print (" offset=%lld", block->offset);
      g_print (" length=%lld", block->length);
      g_print (" dirstride=%d", block->dirstride);
    }

  if (verbosity >= VERBOSITY_PADDING)
    {
      g_print (" {prepad:");
      i = wchunk->n_pad_values;
      p = block->start - (block->dirstride > 0 ? i : -i);
      while (i--)
	{
	  g_print (" %.1f", *p);
	  p += block->dirstride;
	}
      g_print ("}");
    }

  if (verbosity >= VERBOSITY_DATA)
    {
      g_print (" {data:");
      p = block->start;
      while (p != block->end)
	{
	  g_print (" %.1f", *p);
	  p += block->dirstride;
	}
      g_print ("}");
    }

  if (verbosity >= VERBOSITY_PADDING)
    {
      i = wchunk->n_pad_values;
      g_print (" {postpad:");
      while (i--)
	{
	  g_print (" %.1f", *p);
	  p += block->dirstride;
	}
      g_print ("}");
    }

  if (verbosity >= VERBOSITY_BLOCKS)
    g_print ("\n");
}

static void
reversed_datahandle_test (void)
{
  GslDataHandle *myhandle;
  GslDataHandle *rhandle1, *rhandle2;
  GslLong o, l, i, e;
  BseErrorType error;
  
  TSTART ("reversed datahandle");
  
  myhandle = gsl_data_handle_new_mem (1, 32, 44100, 440, my_data_length, my_data, NULL);
  rhandle1 = gsl_data_handle_new_reverse (myhandle);
  gsl_data_handle_unref (myhandle);
  rhandle2 = gsl_data_handle_new_reverse (rhandle1);
  gsl_data_handle_unref (rhandle1);
  error = gsl_data_handle_open (rhandle2);
  if (error)
    g_error ("failed to open rhandle2: %s", bse_error_blurb (error));
  gsl_data_handle_unref (rhandle2);
  
  TASSERT (gsl_data_handle_length (rhandle2) == gsl_data_handle_length (myhandle));
  
  for (i = 1; i < 8; i++)
    {
      o = 0;
      l = gsl_data_handle_length (rhandle2);
      while (l)
        {
          gfloat d1[8], d2[8];
          
          e = gsl_data_handle_read (myhandle, o, MIN (i, l), d1);
          TCHECK (e == MIN (i, l));
          e = gsl_data_handle_read (rhandle2, o, MIN (i, l), d2);
          TCHECK (e == MIN (i, l));
          TCHECK (memcmp (d1, d2, sizeof (d1[0]) * e) == 0);
          l -= e;
          o += e;
        }
      TOK();
    }
  gsl_data_handle_close (rhandle2);

  TDONE();
}

static void
simple_loop_tests (void)
{
  TSTART ("simple loop");
  run_loop_test (GSL_WAVE_LOOP_NONE, -1, 0, 0, 0);
  TOK();
  run_loop_test (GSL_WAVE_LOOP_NONE, 1, 0, 0, 0);
  TOK();
  run_loop_test (GSL_WAVE_LOOP_NONE, -1, 0, 0, 0);
  TOK();
  run_loop_test (GSL_WAVE_LOOP_JUMP, 1, 0, 0, 0);
  TOK();
  run_loop_test (GSL_WAVE_LOOP_PINGPONG, 1, 0, 0, 0);
  TOK();
  run_loop_test (GSL_WAVE_LOOP_JUMP, -1, 0, 0, 0);
  TOK();
  run_loop_test (GSL_WAVE_LOOP_PINGPONG, -1, 0, 0, 0);
  TOK();
  TDONE();
}

static void
brute_force_loop_tests (void)
{
  gint i, j, k, count = 6;
  for (i = 1; i <= count; i++)
    {
      TSTART ("brute force loop test %d/%d", i, 6);
      for (j = 0; j < my_data_length - 1; j++)
        {
          for (k = j + 1; k < my_data_length; k++)
            {
              run_loop_test (GSL_WAVE_LOOP_JUMP, 1, j, k, i);
              run_loop_test (GSL_WAVE_LOOP_PINGPONG, 1, j, k, i);
              run_loop_test (GSL_WAVE_LOOP_JUMP, -1, j, k, i);
              run_loop_test (GSL_WAVE_LOOP_PINGPONG, -1, j, k, i);
            }
          TOK();
        }
      TDONE();
    }
}

int
main (gint   argc,
      gchar *argv[])
{
  /* init */
  SfiInitValue ivalues[] = {
    { "stand-alone",            "true" }, /* no rcfiles etc. */
    { "wave-chunk-padding",     NULL, 1, },
    { "wave_chunk_big_pad",     NULL, 2, },
    { "dcache_block_size",      NULL, 16, },
    { NULL }
  };
  bse_init_test (&argc, &argv, ivalues);

  reversed_datahandle_test();
  simple_loop_tests();
  if (sfi_init_settings().test_slow)
    brute_force_loop_tests();

  return 0;
}
