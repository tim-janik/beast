// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include <bse/gslvorbis-cutter.hh>
#include <bse/bsemain.hh>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
static GslVorbisCutterMode cutmode = GSL_VORBIS_CUTTER_NONE;
static SfiNum              cutpoint = 0;
static guint               filtered_serialno = 0;
static gboolean            filter_serialno = FALSE;
static void
parse_args (int    *argc_p,
            char ***argv_p)
{
  guint argc = *argc_p;
  gchar **argv = *argv_p;
  guint i;
  for (i = 1; i < argc; i++)
    {
      if (strcmp ("-s", argv[i]) == 0 && i + 1 < argc)
        {
          cutmode = GSL_VORBIS_CUTTER_SAMPLE_BOUNDARY;
          argv[i++] = NULL;
          cutpoint = g_ascii_strtoull (argv[i], NULL, 10);
          argv[i] = NULL;
        }
      else if (strcmp ("-k", argv[i]) == 0 && i + 1 < argc)
        {
          cutmode = GSL_VORBIS_CUTTER_PACKET_BOUNDARY;
          argv[i++] = NULL;
          cutpoint = g_ascii_strtoull (argv[i], NULL, 10);
          argv[i] = NULL;
        }
      else if (strcmp ("-p", argv[i]) == 0 && i + 1 < argc)
        {
          cutmode = GSL_VORBIS_CUTTER_PAGE_BOUNDARY;
          argv[i++] = NULL;
          cutpoint = g_ascii_strtoull (argv[i], NULL, 10);
          argv[i] = NULL;
        }
      else if (strcmp ("-S", argv[i]) == 0 && i + 1 < argc)
        {
          filter_serialno = TRUE;
          argv[i++] = NULL;
          filtered_serialno = g_ascii_strtoull (argv[i], NULL, 10);
          argv[i] = NULL;
        }
    }
  guint e = 1;
  for (i = 1; i < argc; i++)
    if (argv[i])
      {
        argv[e++] = argv[i];
        if (i >= e)
          argv[i] = NULL;
      }
  *argc_p = e;
}
int
main (int   argc,
      char *argv[])
{
  const gchar *ifile, *ofile;
  GslVorbisCutter *cutter;
  gint ifd, ofd;
  /* initialization */
  bse_init_inprocess (&argc, argv, "BseCutVorbis",
                      Bse::cstrings_to_vector ("stand-alone=1", "wave-chunk-padding=1",
                                               "dcache-block-size=8192", "dcache-cache-memory=5242880", NULL));
  /* arguments */
  parse_args (&argc, &argv);
  if (argc != 3)
    {
      g_printerr ("usage: cutvorbis infile.ogg [{-s|-k|-p} <cutpoint>] [-S <serialno>] outfile.ogg\n");
      g_printerr ("  -S <serialno>  only process data from the Ogg/Vorbis stream with <serialno>\n");
      g_printerr ("  -s <cutpoint>  cut the Ogg/Vorbis stream at sample <cutpoint>\n");
      g_printerr ("  -k <cutpoint>  same as -s, but cut at vorbis packet boundary\n");
      g_printerr ("  -p <cutpoint>  same as -s, but cut at ogg page boundary\n");
      exit (1);
    }
  ifile = argv[1];
  ofile = argv[2];
  cutter = gsl_vorbis_cutter_new ();
  gsl_vorbis_cutter_set_cutpoint (cutter, cutmode, cutpoint);
  if (filter_serialno)
    gsl_vorbis_cutter_filter_serialno (cutter, filtered_serialno);
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
      if (j <= 0)
        {
          const char *errstr = g_strerror (errno);
          if (!errno && j == 0)
            errstr = "End of File";
          g_printerr ("Error: failed to read from \"%s\": %s\n", ifile, errstr);
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
