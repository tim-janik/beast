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
#include <bse/gslvorbis-enc.h>
#include <bse/gsldatahandle-vorbis.h>
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
list<string>   unlink_file_list;

/* --- main program --- */
extern "C" int
main (int   argc,
      char *argv[])
{
  std::set_terminate (__gnu_cxx::__verbose_terminate_handler);

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
        sfi_error ("missing command\n");
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
      sfi_error ("unknown command: %s\n", command_name.c_str());
      exit (1);
    }
  if (input_file == "")
    {
      sfi_error ("missing input file name\n");
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
      sfi_error ("missing %u arguments to command \"%s\"\n", missing, command->name.c_str());
      exit (1);
    }
  if (argc > 1)
    {
      sfi_error ("extra argument given to command \"%s\": %s\n", command->name.c_str(), argv[1]);
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
              wave = new Wave (wdsc->name, wdsc->n_channels, wdsc->xinfos);
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
      sfi_error ("problems encountered loading bsewave file, aborting...");
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

  /* cleanup */
  delete wave;
  for (list<string>::iterator it = unlink_file_list.begin(); it != unlink_file_list.end(); it++)
    unlink (it->c_str());

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
        sfi_error ("invalid number of channels: %d\n", n_channels);
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
    Wave *wave = new Wave (wave_name.c_str(), n_channels, NULL);
    return wave;
  }
  void
  exec (Wave *wave)
  {
    if (sfi_file_check (output_file.c_str(), "e"))
      {
        sfi_error ("not creating \"%s\": %s\n", output_file.c_str(), g_strerror (EEXIST));
        exit (1);
      }
  }
} cmd_create ("create");

class Oggenc : public Command {
public:
  float quality;
  Oggenc (const char *command_name) :
    Command (command_name)
  {
    quality = 3.0;
  }
  void
  blurb()
  {
    g_print ("\n");
    g_print ("    Compress all chunks with the Vorbis audio codec and store the wave data\n");
    g_print ("    as Ogg/Vorbis streams inside the bsewave file. Options:\n");
    g_print ("    -q <n>              use quality level <n>, refer to oggenc(1) for details\n");
    /*       "**********1*********2*********3*********4*********5*********6*********7*********" */
  }
  guint
  parse_args (int    argc,
              char **argv)
  {
    for (int i = 1; i < argc; i++)
      {
        if (strcmp (argv[i], "-q") == 0 && i + 1 < argc)
          {
            argv[i++] = NULL;
            quality = g_ascii_strtod (argv[i], NULL);
            argv[i] = NULL;
          }
      }
    return 0; // no missing args
  }
  bool
  is_ogg_vorbis_dhandle (GslDataHandle *dhandle)
  {
    GslDataHandle *tmp_handle = dhandle;
    do          /* skip comment or cache handles */
      {
        dhandle = tmp_handle;
        tmp_handle = gsl_data_handle_get_source (dhandle);
      }
    while (tmp_handle);
    GslVorbis1Handle *vhandle = gsl_vorbis1_handle_new (dhandle);
    if (vhandle)
      gsl_vorbis1_handle_destroy (vhandle);
    return vhandle != NULL;
  }
  void
  exec (Wave *wave)
  {
    /* ogg encoder */
    guint nth = 1;
    for (list<WaveChunk>::iterator it = wave->chunks.begin(); it != wave->chunks.end(); it++, nth++)
      {
        WaveChunk *chunk = &*it;
        GslVorbisEncoder *enc = gsl_vorbis_encoder_new ();
        GslDataHandle *dhandle = chunk->dhandle;
        if (is_ogg_vorbis_dhandle (dhandle))
          continue;
        gsl_vorbis_encoder_set_quality (enc, quality);
        gsl_vorbis_encoder_set_n_channels (enc, wave->n_channels);
        gsl_vorbis_encoder_set_sample_freq (enc, guint (gsl_data_handle_mix_freq (dhandle)));
        BseErrorType error = gsl_vorbis_encoder_setup_stream (enc, (rand () << 16) ^ rand ()); // FIXME: better are deterministic serials
        if (error)
          {
            sfi_error ("chunk % 7.2f/%.0f: failed to encode: %s",
                       gsl_data_handle_osc_freq (chunk->dhandle), gsl_data_handle_mix_freq (chunk->dhandle),
                       bse_error_blurb (error));
            exit (1);
          }
        gchar *temp_file = g_strdup_printf ("%s/bsewavetool-pid%u-oggchunk%04X.tmp%06xyXXXXXX", g_get_tmp_dir(), getpid(), 0x1000 + nth, rand() & 0xfffffd);
        gint tmpfd = mkstemp (temp_file);
        if (tmpfd < 0)
          {
            sfi_error ("chunk % 7.2f/%.0f: failed to open tmp file \"%s\": %s",
                       gsl_data_handle_osc_freq (chunk->dhandle), gsl_data_handle_mix_freq (chunk->dhandle),
                       temp_file, g_strerror (errno));
            exit (1);
          }
        unlink_file_list.push_back (temp_file);
        const guint ENCODER_BUFFER = 16 * 1024;
        g_printerr ("ENCODING: chunk % 7.2f/%.0f\r", gsl_data_handle_osc_freq (chunk->dhandle), gsl_data_handle_mix_freq (chunk->dhandle));
        SfiNum n = 0, l = gsl_data_handle_length (dhandle);
        while (n < l)
          {
            gfloat buffer[ENCODER_BUFFER];
            SfiNum j, r = gsl_data_handle_read (dhandle, n, ENCODER_BUFFER, buffer);
            if (r > 0)
              {
                n += r;
                gsl_vorbis_encoder_write_pcm (enc, r, buffer);
                guint8 *buf = reinterpret_cast<guint8*> (buffer);
                r = gsl_vorbis_encoder_read_ogg (enc, ENCODER_BUFFER, buf);
                while (r > 0)
                  {
                    do
                      j = write (tmpfd, buf, r);
                    while (j < 0 && errno == EINTR);
                    if (j < 0)
                      {
                        sfi_error ("chunk % 7.2f/%.0f: failed to write to tmp file: %s",
                                   gsl_data_handle_osc_freq (chunk->dhandle), gsl_data_handle_mix_freq (chunk->dhandle),
                                   g_strerror (errno));
                        exit (1);
                      }
                    r = gsl_vorbis_encoder_read_ogg (enc, ENCODER_BUFFER, buf);
                  }
              }
            g_printerr ("ENCODING: chunk % 7.2f/%.0f, processed %0.1f%%       \r",
                        gsl_data_handle_osc_freq (chunk->dhandle), gsl_data_handle_mix_freq (chunk->dhandle),
                        n * 100.0 / l);
          }
        gsl_vorbis_encoder_pcm_done (enc);
        g_printerr ("\n");
        while (!gsl_vorbis_encoder_ogg_eos (enc))
          {
            guint8 buf[ENCODER_BUFFER];
            SfiNum j, r = gsl_vorbis_encoder_read_ogg (enc, ENCODER_BUFFER, buf);
            if (r > 0)
              {
                do
                  j = write (tmpfd, buf, r);
                while (j < 0 && errno == EINTR);
                if (j < 0)
                  {
                    sfi_error ("chunk % 7.2f/%.0f: failed to write to tmp file: %s",
                               gsl_data_handle_osc_freq (chunk->dhandle), gsl_data_handle_mix_freq (chunk->dhandle),
                               g_strerror (errno));
                    exit (1);
                  }
              }
          }
        gsl_vorbis_encoder_destroy (enc);
        if (close (tmpfd) < 0)
          {
            sfi_error ("chunk % 7.2f/%.0f: failed to write to tmp file: %s",
                       gsl_data_handle_osc_freq (chunk->dhandle), gsl_data_handle_mix_freq (chunk->dhandle),
                       g_strerror (errno));
            exit (1);
          }
        error = chunk->set_dhandle_from_temporary (temp_file, gsl_data_handle_osc_freq (dhandle));
        if (error)
          {
            sfi_error ("chunk % 7.2f/%.0f: failed to read wave \"%s\": %s",
                       gsl_data_handle_osc_freq (chunk->dhandle), gsl_data_handle_mix_freq (chunk->dhandle),
                       temp_file, bse_error_blurb (error));
            exit (1);
          }
        g_free (temp_file);
      }
  }
} cmd_oggenc ("oggenc");

/* FIXME: TODO items:
 * bsewavetool config-gus-patch <file.bsewave> {--chunk=<freq>|--all-chunks}
 *   --chunk=<freq>             select chunk to modify by frequency
 *   --all-chunks               select all chunks for modifications
 *   --envelope=<a,d,s,r>       set envelope
 *   --sustain-envelope=<a,d,s,r> set envelope
 *   --reset-envelope           set envelope
 *   --panning=<p>              set output panning
 *   --tremolo=<s,r,d>          tremolo, s.., r..., d...
 *   --vibrato=<s,r,d>          vibrato, s.., r..., d...
 * bsewavetool merge <file.bsewave> <second.bsewave>
 * bsewavetool add-wave <file.bsewave> <wavefile> [-n note] ...
 * bsewavetool del-wave <file.bsewave> {<note>|-f <freq>} ...
 * bsewavetool loop <file.bsewave> [-a loop-algorithm] ...
 * bsewavetool clip <file.bsewave> [-c clip-config] ...
 * bsewavetool omit <file.bsewave> [-a remove-algorithm] ...
 * bsewavetool.1 # need manual page
 */

} // BseWaveTool
