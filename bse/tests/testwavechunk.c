/* GSL - Generic Sound Layer
 * Copyright (C) 2001 Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include        "gslwavechunk.h"
#include        "gsldatahandle.h"
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<math.h>

enum {
  VERBOSITY_NONE,
  VERBOSITY_SETUP,
  VERBOSITY_BLOCKS,
  VERBOSITY_DATA,
  VERBOSITY_PADDING,
  VERBOSITY_CHECKS,
};

typedef struct {
  GslDataHandle handle;
  GslLong       offset;
} MyHandle;

static gfloat my_data[] = {
  .5,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,
  20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,45,
};
static guint  my_data_length = sizeof (my_data) / sizeof (my_data[0]);
		     
static guint verbosity = 0;

static void
my_handle_destroy (GslDataHandle *handle)
{
  gsl_data_handle_common_free (handle);
}
static GslErrorType
my_handle_open (GslDataHandle *data_handle)
{
  MyHandle *handle = (MyHandle*) data_handle;

  handle->offset = 0;
  return GSL_ERROR_NONE;
}
static GslLong
my_handle_read (GslDataHandle *data_handle,
		GslLong        voffset,
		GslLong        n_values,
		gfloat        *values)
{
  MyHandle *handle = (MyHandle*) data_handle;
  guint i;
  
  if (handle->offset >= my_data_length)
    return 0;
  
  n_values = MIN (n_values, my_data_length - voffset);
  for (i = 0; i < n_values; i++)
    values[i] = my_data[voffset + i];
  return n_values;
}
static void
my_handle_close (GslDataHandle *data_handle)
{
  // MyHandle *handle = (MyHandle*) data_handle;
}
static GslDataHandleFuncs my_handle_vtable = {
  my_handle_open,
  my_handle_read,
  my_handle_close,
  my_handle_destroy,
};

static void
print_block (GslWaveChunk      *wchunk,
	     GslWaveChunkBlock *block)
{
  gfloat *p = NULL;
  guint i;

  if (verbosity >= VERBOSITY_BLOCKS)
    {
      g_print ("BLOCK:");
      g_print (" offset=%ld", block->offset);
      g_print (" length=%ld", block->length);
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

#define DEBUG_SIZE	(1024 * 1024)
#define	MINI_DEBUG_SIZE	(16)

static void
run_tests (GslWaveLoopType loop_type,
	   gint            play_dir,
	   gint		   loop_start,
	   gint		   loop_end,
	   gint		   loop_count)
{
  gfloat tmpstorage[DEBUG_SIZE], *cmpblock = tmpstorage + DEBUG_SIZE / 2;
  MyHandle myhandle = { { 0, }, };
  GslDataCache *dcache;
  GslWaveChunkBlock block = { 0, };
  GslWaveChunk *wchunk;

  gsl_data_handle_common_init (&myhandle.handle, NULL);
  myhandle.handle.vtable = &my_handle_vtable;
  myhandle.handle.name = g_strdup ("inlined-test-sample");
  myhandle.handle.n_values = my_data_length;
  dcache = gsl_data_cache_new ((GslDataHandle*) &myhandle, 1);
  wchunk = _gsl_wave_chunk_create (dcache, 0, my_data_length,
				   1, 44.0, 44100.0,
				   loop_type, loop_start, loop_end, loop_count);
  if (verbosity >= VERBOSITY_SETUP)
    g_print ("SETUP: loop_type=%u loop_start=%ld loop_end=%ld loop_count=%d playdir=%d\n",
	     wchunk->loop_type, wchunk->loop_start, wchunk->loop_end, wchunk->loop_count, play_dir);
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
	    g_print ("%s: offset=%d (block.offset=%ld) value=%.16f found=%.16f\n",
		     fabs (cmpblock[i] - v) > 1e-15 ? "MISMATCH" : "match",
		     i, (i - block.offset), cmpblock[i], v);
	}
      if (abort)
	{
	  g_error ("mismatches occoured, setup: loop_type=%u loop_start=%ld loop_end=%ld",
		   wchunk->loop_type, wchunk->loop_start, wchunk->loop_end);
	}

      gsl_wave_chunk_unuse_block (wchunk, &block);

      block.offset = block.next_offset;
      // block.offset += block.play_dir;
    }
  _gsl_wave_chunk_destroy (wchunk);
  gsl_data_cache_unref (dcache);
}

int
main (gint   argc,
      gchar *argv[])
{
  GslConfigValue gslconfig[] = {
    { "wave_chunk_padding",     1, },
    { "wave_chunk_big_pad",     2, },
    { "dcache_block_size",      8, },
    { NULL, },
  };
  gint i, j, k;

  verbosity = VERBOSITY_SETUP;
  
  g_thread_init (NULL);
  gsl_init (gslconfig);
  
  run_tests (GSL_WAVE_LOOP_NONE, -1, 0, 0, 0);

  run_tests (GSL_WAVE_LOOP_NONE, 1, 0, 0, 0);
  run_tests (GSL_WAVE_LOOP_NONE, -1, 0, 0, 0);
  run_tests (GSL_WAVE_LOOP_JUMP, 1, 0, 0, 0);
  run_tests (GSL_WAVE_LOOP_PINGPONG, 1, 0, 0, 0);
  run_tests (GSL_WAVE_LOOP_JUMP, -1, 0, 0, 0);
  run_tests (GSL_WAVE_LOOP_PINGPONG, -1, 0, 0, 0);
  for (i = 1; i < 7; i++)
    for (j = 0; j < my_data_length - 1; j++)
      for (k = j + 1; k < my_data_length; k++)
	{
	  run_tests (GSL_WAVE_LOOP_JUMP, 1, j, k, i);
	  run_tests (GSL_WAVE_LOOP_PINGPONG, 1, j, k + 1, i);
	  run_tests (GSL_WAVE_LOOP_JUMP, -1, j, k, i);
	  run_tests (GSL_WAVE_LOOP_PINGPONG, -1, j, k + 1, i);
	}
  
  return 0;
}
