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
#include        "gslcommon.h"

#include        "gslwavedsc.h"
#include        "gsldatacache.h"
#include        "gslwavechunk.h"
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<sys/stat.h>
#include	<fcntl.h>

enum {
  VERBOSITY_NONE,
  VERBOSITY_SETUP,
  VERBOSITY_BLOCKS,
  VERBOSITY_DATA,
  VERBOSITY_PADDING,
  VERBOSITY_CHECKS,
};
static guint verbosity = VERBOSITY_SETUP;

int
main (gint   argc,
      gchar *argv[])
{
  GslWaveDsc *wave;
  GslWaveChunk *wchunk;

  verbosity = VERBOSITY_SETUP;
  
  g_thread_init (NULL);
  gsl_init (NULL);

  if (argc != 2)
    g_error ("need *.gslwave file");

  wave = gsl_wave_dsc_read (argv[1]);
  if (!wave || !wave->n_chunks)
    g_error ("failed to read wave file with chunks from \"%s\"",
	     argv[1]);

  wchunk = gsl_wave_chunk_from_dsc (wave, 0);
  if (!wchunk)
    g_error ("failed to create wchunk from \"%s\"", wave->chunks[0].file_name);
					   

  g_print ("wavechunk=%p\n", wchunk);
  
  return 0;
}
