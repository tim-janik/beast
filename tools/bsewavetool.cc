/* BseWaveTool - BSE Wave creation tool
 * Copyright (C) 2001-2004 Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bsewavetool.h"
#include "topconfig.h"
#include "sfiutils.h"
#include "bwtwave.h"
#include <bse/bsemain.h>	/* for bse_init_intern() */
#include <bse/gslloader.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>
//#include <fcntl.h>
//#include <errno.h>
//#include <stdio.h>

namespace BseWaveTool {


#define PRG_NAME        ("bsewavetool")
#define	IROUND(dbl)	((int) (floor (dbl + 0.5)))

/* --- prototypes --- */
static void     wavetool_parse_args     (int    *argc_p,
                                         char ***argv_p);
static void     wavetool_print_blurb    (void);

/* --- variables --- */
static const gchar *command_name = NULL;
static const gchar *input_file = NULL;
static const gchar *output_file = NULL;

/* --- main program --- */
extern "C" int
main (int   argc,
      char *argv[])
{
  /* initialization */
  g_thread_init (NULL);
  bse_init_intern (&argc, &argv, NULL);
  sfi_debug_allow ("all"); // FIXME
  
  /* pre-parse argument list to decide command */
  wavetool_parse_args (&argc, &argv);

  /* check args */
  if (!command_name)
    {
      if (argc > 1)
        g_printerr ("%s: missing command\n", PRG_NAME);
      wavetool_print_blurb();
      exit (1);
    }
  if (!input_file)
    {
      g_printerr ("%s: missing input file name\n", PRG_NAME);
      wavetool_print_blurb();
      exit (1);
    }

  BseErrorType error;
  Wave *wave = NULL;

  /* load wave file */
  sfi_debug ("main", "LOAD: %s", input_file);
  GslWaveFileInfo *winfo = gsl_wave_file_info_load (input_file, &error);
  if (winfo && winfo->n_waves == 1)
    {
      GslWaveDsc *wdsc = gsl_wave_dsc_load (winfo, 0, &error);
      if (wdsc && wdsc->n_chunks >= 1)
        {
          wave = new Wave (wdsc->name, wdsc->n_channels);
          guint i;
          for (i = 0; i < wdsc->n_chunks; i++)
            {
              GslDataHandle *dhandle = gsl_wave_handle_create (wdsc, i, &error);
              if (!dhandle)
                sfi_warning ("failed to load wave chunk from wave \"%s\" in file \"%s\": %s (loader: %s)",
                             wdsc->name, input_file, bse_error_blurb (error), gsl_wave_file_info_loader (winfo));
              else
                {
                  wave->add_chunk (dhandle);
                  gsl_data_handle_unref (dhandle);
                }
            }
          gsl_wave_dsc_free (wdsc);
        }
      else
        sfi_warning ("failed to load wave description from file \"%s\": %s (loader: %s)",
                     input_file, bse_error_blurb (error), gsl_wave_file_info_loader (winfo));
      gsl_wave_file_info_unref (winfo);
      if (wave && wave->chunks.empty())
        {
          delete wave;
          wave = NULL;
        }
    }
  else
    sfi_warning ("failed to load wave file \"%s\": %s", input_file, bse_error_blurb (error));
  if (!wave)
    {
      sfi_error ("no wave to process, aborting...");
      exit (1);
    }

  /* process */
  sfi_debug ("main", "EXEC: %s", command_name);
  
  /* save */
  sfi_debug ("main", "SAVE: %s", output_file);
  wave->sort();
  error = wave->store (output_file);
  if (error)
    {
      sfi_error ("failed to store wave \"%s\" to file \"%s\": %s", wave->name.c_str(), output_file, bse_error_blurb (error));
      exit (1);
    }

  /* FIXME: junk
    bsewavetool <samplefile>   store                 # auto-appends .bsewave to samplefile
    bsewavetool <file.bsewave> create [wavefiles...] # besonders, da file.bsewave nicht existieren/geladen werden muss
    bsewavetool <file.bsewave> merge <second.bsewave>
    bsewavetool <file.bsewave> ogg-pack [-c compressionlevel]
    bsewavetool <file.bsewave> add-wave <wavefile> [-n note] ...
    bsewavetool <file.bsewave> del-wave {<note>|-f <freq>} ...
    bsewavetool <file.bsewave> loop [-a loop-algorithm] ...
    bsewavetool <file.bsewave> clip [-c clip-config] ...
    bsewavetool <file.bsewave> omit [-a remove-algorithm] ...
    bsewavetool.1 # need manual page
   */

  return 0;
}

static void
wavetool_print_version (void)
{
  g_print ("bsewavetool version %s (%s)\n", BST_VERSION, BST_VERSION_HINT);
  g_print ("Refer to beast --version for more version information.\n");
  g_print ("bsewavetool comes with ABSOLUTELY NO WARRANTY.\n");
  g_print ("You may redistribute copies of bsewavetool under the terms\n");
  g_print ("of the GNU General Public License which can be found in\n");
  g_print ("the BEAST source package. Sources, examples and contact\n");
  g_print ("information are available at http://beast.gtk.org/.\n");
}

static void
wavetool_print_blurb (void)
{
  g_print ("Usage: bsewavetool [options] command <file.bsewave> [command-options]\n");
  g_print ("Options:\n");
  g_print ("  -o <output.bsewave>   name of the destination file (default: file.bsewave)\n");
  g_print ("  -h, --help            show this help message\n");
  g_print ("  -v, --version         print version information\n");
  g_print ("Commands:\n");
  g_print ("  foo\n");
  g_print ("    --clip <cfg>        clipping configuration, consisting of ':'-seperated values:\n");
  g_print ("                        minimum signal threshold (0..32767) [16]\n");
  g_print ("                        n_samples of silence to verify at head [0]\n");
  g_print ("                        n_samples of silence to verify at tail [0]\n");
  g_print ("                        n_samples of fade-in before signal starts [16]\n");
  g_print ("                        n_samples of padding after signal ends [16]\n");
  g_print ("                        n_samples of tail silence to detect at minimum to\n");
  g_print ("                                  allow tail clipping [0]\n");
  /*       "**********1*********2*********3*********4*********5*********6*********7*********" */
}

static void
wavetool_parse_args (int    *argc_p,
                     char ***argv_p)
{
  guint argc = *argc_p;
  gchar **argv = *argv_p;
  gchar *envar;
  guint i;

  envar = getenv ("BSEWAVETOOL_DEBUG");
  if (envar)
    sfi_debug_allow (envar);
  envar = getenv ("BSEWAVETOOL_NO_DEBUG");
  if (envar)
    sfi_debug_deny (envar);

  for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "--") == 0)
        {
          argv[i] = NULL;
          break;
        }
      else if (strcmp ("--debug-list", argv[i]) == 0)
        {
          g_print ("debug keys: all");
          g_print ("\n");
          exit (0);
          argv[i] = NULL;
        }
      else if (strcmp ("--debug", argv[i]) == 0 ||
               strncmp ("--debug=", argv[i], 8) == 0)
        {
          gchar *equal = argv[i] + 7;
          if (*equal == '=')
            sfi_debug_allow (equal + 1);
          else if (i + 1 < argc)
            {
              argv[i++] = NULL;
              sfi_debug_allow (argv[i]);
            }
          argv[i] = NULL;
        }
      else if (strcmp ("--no-debug", argv[i]) == 0 ||
               strncmp ("--no-debug=", argv[i], 8) == 0)
        {
          gchar *equal = argv[i] + 7;
          if (*equal == '=')
            sfi_debug_deny (equal + 1);
          else if (i + 1 < argc)
            {
              argv[i++] = NULL;
              sfi_debug_deny (argv[i]);
            }
          argv[i] = NULL;
        }
      else if (strcmp ("-h", argv[i]) == 0 ||
               strcmp ("--help", argv[i]) == 0)
        {
          wavetool_print_blurb ();
          argv[i] = NULL;
          exit (0);
        }
      else if (strcmp ("-v", argv[i]) == 0 ||
               strcmp ("--version", argv[i]) == 0)
        {
          wavetool_print_version ();
          argv[i] = NULL;
          exit (0);
        }
      else if (strcmp ("-o", argv[i]) == 0 ||
               strncmp ("-o", argv[i], 2) == 0)
        {
          gchar *equal = argv[i] + 2;
          if (*equal == '=')            /* -o=Foo */
            output_file = equal + 1;
          else if (*equal)              /* -oFoo */
            output_file = equal;
          else if (i + 1 < argc)        /* -o Foo */
            {
              argv[i++] = NULL;
              output_file = argv[i];
            }
          argv[i] = NULL;
        }
      else /* non-options */
        {
          if (!command_name)
            {
              command_name = argv[i];
              argv[i] = NULL;
            }
          else if (!input_file)
            {
              input_file = argv[i];
              argv[i] = NULL;
              if (!output_file)
                output_file = input_file;
            }
          else
            ; /* ignore further non-options */
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

} // BseWaveTool
