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
#include <errno.h>
#include <string.h>
#include <sys/time.h>


namespace BseWaveTool {


#define PRG_NAME        ("bsewavetool")
#define	IROUND(dbl)	((int) (floor (dbl + 0.5)))

/* --- prototypes --- */
static void     wavetool_parse_args     (int    *argc_p,
                                         char ***argv_p);
static void     wavetool_print_blurb    (void);

/* --- variables --- */
static string command_name;
static string input_file;
static string output_file;
list<Command*> Command::registry;

/* --- main program --- */
extern "C" int
main (int   argc,
      char *argv[])
{
  /* initialize random numbers */
  struct timeval tv;
  gettimeofday (&tv, NULL);
  srand (tv.tv_usec + (tv.tv_sec << 16));

  /* initialization */
  g_thread_init (NULL);
  int orig_argc = argc;
  bse_init_intern (&argc, &argv, NULL);
  sfi_debug_allow ("main"); // FIXME
  
  /* pre-parse argument list to decide command */
  wavetool_parse_args (&argc, &argv);

  /* check args */
  if (command_name == "")
    {
      if (orig_argc > 1)
        g_printerr ("%s: missing command\n", PRG_NAME);
      else
        wavetool_print_blurb();
      exit (1);
    }
  Command *command = NULL;
  for (list<Command*>::iterator it = Command::registry.begin(); it != Command::registry.end(); it++)
    if ((*it)->name == command_name)
      {
        command = *it;
        break;
      }
  if (!command)
    {
      g_printerr ("%s: unknown command: %s\n", PRG_NAME, command_name.c_str());
      exit (1);
    }
  if (input_file == "")
    {
      g_printerr ("%s: missing input file name\n", PRG_NAME);
      exit (1);
    }

  /* parse and check command args */
  guint missing = command->parse_args (argc, argv);
  int e = 1; /* collapse NULL args */
  for (int i = 1; i < argc; i++)
    if (argv[i])
      {
        argv[e++] = argv[i];
        if (i >= e)
          argv[i] = NULL;
      }
  argc = e;
  if (missing)
    {
      g_printerr ("%s: missing %u arguments to command \"%s\"\n", PRG_NAME, missing, command->name.c_str());
      exit (1);
    }
  if (argc > 1)
    {
      g_printerr ("%s: extra argument given to command \"%s\": %s\n", PRG_NAME, command->name.c_str(), argv[1]);
      exit (1);
    }
  
  /* load wave file */
  sfi_debug ("main", "LOAD: %s", input_file.c_str());
  Wave *wave = command->create ();
  BseErrorType error = BSE_ERROR_NONE;
  if (!wave)
    {
      GslWaveFileInfo *winfo = gsl_wave_file_info_load (input_file.c_str(), &error);
      if (winfo && winfo->n_waves == 1)
        {
          GslWaveDsc *wdsc = gsl_wave_dsc_load (winfo, 0, TRUE, &error);
          if (wdsc)
            {
              wave = new Wave (wdsc->name, wdsc->n_channels);
              guint i;
              for (i = 0; i < wdsc->n_chunks; i++)
                {
                  GslDataHandle *dhandle = gsl_wave_handle_create (wdsc, i, &error);
                  if (!dhandle)
                    {
                      sfi_warning ("failed to load wave chunk of wave \"%s\" in file \"%s\": %s (loader: %s)",
                                   wdsc->name, input_file.c_str(), bse_error_blurb (error), gsl_wave_file_info_loader (winfo));
                      delete wave;
                      wave = NULL;
                      break;
                      error = BSE_ERROR_NONE;
                    }
                  else
                    {
                      wave->add_chunk (dhandle);
                      gsl_data_handle_unref (dhandle);
                    }
                }
              gsl_wave_dsc_free (wdsc);
            }
          else
            {
              sfi_warning ("failed to load wave description from file \"%s\": %s (loader: %s)",
                           input_file.c_str(), bse_error_blurb (error), gsl_wave_file_info_loader (winfo));
              error = BSE_ERROR_NONE;
            }
          gsl_wave_file_info_unref (winfo);
        }
    }
  if (error)
    sfi_warning ("failed to load wave file \"%s\": %s", input_file.c_str(), bse_error_blurb (error));
  if (!wave)
    {
      sfi_error ("no wave to process, aborting...");
      exit (1);
    }

  /* process */
  sfi_debug ("main", "EXEC: %s", command_name.c_str());
  command->exec (wave);
  
  /* save */
  sfi_debug ("main", "SAVE: %s", output_file.c_str());
  wave->sort();
  error = wave->store (output_file);
  if (error)
    {
      sfi_error ("failed to store wave \"%s\" to file \"%s\": %s", wave->name.c_str(), output_file.c_str(), bse_error_blurb (error));
      exit (1);
    }

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
  g_print ("Usage: bsewavetool [options] command <file.bsewave> {command-arguments}\n");
  g_print ("Options:\n");
  g_print ("  -o <output.bsewave>   name of the destination file (default: <file.bsewave>)\n");
  g_print ("  -h, --help            show this help message\n");
  g_print ("  -v, --version         print version information\n");
  /*       "**********1*********2*********3*********4*********5*********6*********7*********" */
  g_print ("Commands:\n");
  for (list<Command*>::iterator it = Command::registry.begin(); it != Command::registry.end(); it++)
    {
      Command *cmd = *it;
      g_print ("  %s ", cmd->name.c_str());
      cmd->blurb();
    }
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
      else /* command & file names */
        {
          if (command_name == "")
            {
              command_name = argv[i];
              argv[i] = NULL;
            }
          else if (input_file == "")
            {
              input_file = argv[i];
              argv[i] = NULL;
              if (output_file == "")
                output_file = input_file;
            }
          else
            ; /* preserve remaining options */
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

/* --- command implementations --- */
class Store : public Command {
public:
  Store (const char *command_name) :
    Command (command_name)
  {}
  void
  exec (Wave   *wave)
  {
    /* nothing to do */
  }
  void
  blurb()
  {
    g_print ("\n");
    g_print ("    Store the input bsewave as output bsewave without any modifications.\n");
    /*       "**********1*********2*********3*********4*********5*********6*********7*********" */
  }
} cmd_store ("store");

class Create : public Command {
public:
  Create (const char *command_name) :
    Command (command_name)
  {}
  void
  blurb()
  {
    g_print ("<n_channels>\n");
    g_print ("    Create an empty bsewave file, <n_channels>=1 (mono) and <n_channels>=2\n");
    g_print ("    (stereo) are currently supported. Options:\n");
    g_print ("    -N <wave-name>      name of the wave stored inside of <output.bsewave>\n");
    /*       "**********1*********2*********3*********4*********5*********6*********7*********" */
  }
  string wave_name;
  string channel_str;
  guint
  parse_args (int    argc,
              char **argv)
  {
    for (int i = 1; i < argc; i++)
      {
        if (strcmp (argv[i], "-N") == 0 && i + 1 < argc)
          {
            argv[i++] = NULL;
            wave_name = argv[i];
            argv[i] = NULL;
          }
        else if (channel_str == "")
          {
            channel_str = argv[i];
            argv[i] = NULL;
          }
      }
    return channel_str == "";
  }
  Wave*
  create ()
  {
    /* figure n_channels */
    guint n_channels = g_ascii_strtoull (channel_str.c_str(), NULL, 10);
    if (n_channels < 1 || n_channels > 2)
      {
        g_printerr ("%s: invalid number of channels: %d\n", PRG_NAME, n_channels);
        exit (1);
      }
    /* figure name */
    if (wave_name == "")
      {
        gchar *file_name = g_path_get_basename (output_file.c_str());
        const gchar *dot = strrchr (file_name, '.');
        gchar *name = dot && dot > file_name ? g_strndup (file_name, dot - file_name) : g_strdup (file_name);
        g_free (file_name);
        wave_name = name;
        g_free (name);
      }
    /* create wave */
    Wave *wave = new Wave (wave_name.c_str(), n_channels);
    return wave;
  }
  void
  exec (Wave *wave)
  {
    if (sfi_file_check (output_file.c_str(), "e"))
      {
        g_printerr ("%s: not creating \"%s\": %s\n", PRG_NAME, output_file.c_str(), g_strerror (EEXIST));
        exit (1);
      }
  }
} cmd_create ("create");

/* FIXME: commands
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
static void
blurb1()
{
  g_print ("    Compress all chunks with the Vorbis audio codec and store the wave data\n");
  g_print ("    as Ogg/Vorbis streams internally. Options:\n");
  g_print ("    -q <n>              use quality level <n>, refer to oggenc(1) for details\n");
  /*       "**********1*********2*********3*********4*********5*********6*********7*********" */
  g_print ("  oggenc <===========\n");
}

} // BseWaveTool
