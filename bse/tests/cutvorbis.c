/* GSL - Generic Sound Layer
 * Copyright (C) 2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <bse/gslvorbis-cutter.h>
#include <bse/bsemain.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

int
main (int   argc,
      char *argv[])
{
  const gchar *ifile, *ofile;
  GslVorbisCutter *cutter;
  GslLong cutpoint;
  gint ifd, ofd;

  /* initialization */
  g_thread_init (NULL);
  bse_init_intern (&argc, &argv, NULL);

  /* arguments */
  if (argc != 4)
    {
      g_printerr ("usage: cutvorbis infile.ogg <cutpoint> outfile.ogg\n");
      exit (1);
    }
  ifile = argv[1];
  cutpoint = atoi (argv[2]);
  ofile = argv[3];

  cutter = gsl_vorbis_cutter_new ();
  gsl_vorbis_cutter_set_cutpoint (cutter, cutpoint, GSL_VORBIS_CUTTER_PACKET_BOUNDARY);

  ifd = open (ifile, O_RDONLY);
  if (ifd < 0)
    {
      g_printerr ("Error: failed to open \"%s\": %s\n", ifile, g_strerror (errno));
      exit (1);
    }
  ofd = open (ofile, O_CREAT | O_TRUNC | O_WRONLY, 0666);
  if (ofd < 0)
    {
      g_printerr ("Error: failed to open \"%s\": %s\n", ofile, g_strerror (errno));
      exit (1);
    }

  while (!gsl_vorbis_cutter_ogg_eos (cutter))
    {
      guint blength = 8192, n, j;
      guint8 buffer[blength];
      do
        j = read (ifd, buffer, blength);
      while (j < 0 && errno == EINTR);
      if (j < 0)
        {
          g_printerr ("Error: failed to read from \"%s\": %s\n", ifile, g_strerror (errno));
          exit (1);
        }
      gsl_vorbis_cutter_write_ogg (cutter, j, buffer);
      n = gsl_vorbis_cutter_read_ogg (cutter, blength, buffer);
      do
        j = write (ofd, buffer, n);
      while (j < 0 && errno == EINTR);
      if (j < 0)
        {
          g_printerr ("Error: failed to write to \"%s\": %s\n", ofile, g_strerror (errno));
          exit (1);
        }
    }

  close (ifd);
  if (close (ofd) < 0)
    {
      g_printerr ("Error: failed to flush \"%s\": %s\n", ofile, g_strerror (errno));
      exit (1);
    }
  g_print ("done\n");

  return 0;
}
