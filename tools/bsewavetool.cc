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
#include "bsewavetool.hh"
#include "topconfig.h"
#include "sfiutils.h"
#include "bwtwave.hh"
#include <bse/bsemain.h>	/* for bse_init_intern() */
#include <bse/bseloader.h>
#include <bse/gslvorbis-enc.h>
#include <bse/gsldatahandle-vorbis.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <vector>


namespace BseWaveTool {


#define PRG_NAME        ("bsewavetool")
#define	IROUND(dbl)	((int) (floor (dbl + 0.5)))

/* --- prototypes --- */
static void     wavetool_parse_args     (int    *argc_p,
                                         char ***argv_p);
static void     wavetool_print_blurb    (bool    bshort);

/* --- variables --- */
static bool   continue_on_error = false;
static bool   quiet_infos = false;
static string command_name;
static string input_file;
static string output_file;
list<Command*> Command::registry;
list<string>   unlink_file_list;

/* --- main program --- */
static void
wavetool_log_handler (const SfiLogMessage  *msg)
{
  if (msg->level == SFI_LOG_INFO)
    {
      if (!quiet_infos)
        {
          if (msg->primary)
            g_printerr ("%s\n", msg->primary);
          if (msg->secondary)
            g_printerr ("%s\n", msg->secondary);
          if (msg->details)
            g_printerr ("%s\n", msg->details);
        }
    }
  else
    sfi_log_default_handler (msg);
}

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
  sfi_init ();
  int orig_argc = argc;
  bse_init_intern (&argc, &argv, NULL);
  sfi_debug_allow ("main"); // FIXME
  sfi_log_set_thread_handler (wavetool_log_handler);
  sfi_log_assign_level (SFI_LOG_INFO, SFI_LOG_TO_HANDLER);
  
  /* pre-parse argument list to decide command */
  wavetool_parse_args (&argc, &argv);

  /* check args */
  if (command_name == "")
    {
      if (orig_argc > 1)
        sfi_error ("missing command\n");
      else
        wavetool_print_blurb (true);
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
  g_printerr ("LOAD: %s\n", input_file.c_str());
  Wave *wave = command->create ();
  BseErrorType error = BSE_ERROR_NONE;
  if (!wave)
    {
      BseWaveFileInfo *winfo = bse_wave_file_info_load (input_file.c_str(), &error);
      if (winfo && winfo->n_waves == 1)
        {
          BseWaveDsc *wdsc = bse_wave_dsc_load (winfo, 0, TRUE, &error);
          if (wdsc)
            {
              wave = new Wave (wdsc->name, wdsc->n_channels, wdsc->xinfos);
              guint i;
              for (i = 0; i < wdsc->n_chunks; i++)
                {
                  GslDataHandle *dhandle = bse_wave_handle_create (wdsc, i, &error);
                  if (!dhandle)
                    {
                      sfi_warning ("failed to load wave chunk (%.3f) of wave \"%s\" in file \"%s\": %s (loader: %s)",
                                   wdsc->chunks[i].osc_freq, wdsc->name, input_file.c_str(), bse_error_blurb (error), bse_wave_file_info_loader (winfo));
                      if (continue_on_error)
                        error = BSE_ERROR_NONE;
                      else
                        {
                          delete wave;
                          wave = NULL;
                          break;
                        }
                    }
                  else
                    {
                      wave->add_chunk (dhandle);
                      gsl_data_handle_unref (dhandle);
                    }
                }
              bse_wave_dsc_free (wdsc);
            }
          else
            {
              sfi_warning ("failed to load wave description from file \"%s\": %s (loader: %s)",
                           input_file.c_str(), bse_error_blurb (error), bse_wave_file_info_loader (winfo));
              error = BSE_ERROR_NONE;
            }
          bse_wave_file_info_unref (winfo);
        }
    }
  if (!wave && !error)
    error = BSE_ERROR_IO;       /* unknown */
  if (error)
    {
      sfi_error ("problems encountered loading bsewave file \"%s\": %s", input_file.c_str(), bse_error_blurb (error));
      exit (1);
    }

  /* process */
  g_printerr ("EXEC: %s\n", command_name.c_str());
  command->exec (wave);
  
  /* save */
  g_printerr ("SAVE: %s\n", output_file.c_str());
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
wavetool_print_blurb (bool bshort)
{
  g_print ("Usage: bsewavetool [tool-options] command <file.bsewave> {command-arguments}\n");
  g_print ("Tool options:\n");
  g_print ("  -o <output.bsewave>   name of the destination file (default: <file.bsewave>)\n");
  g_print ("  -q                    quiet, suppress extra processing information\n");
  g_print ("  -k                    continue on errors (may overwrite bsewave files after\n");
  g_print ("                        load errors occoured for part of its contents)\n");
  g_print ("  -h, --help            show elaborated help message with command documentation\n");
  g_print ("  -v, --version         print version information\n");
  /*       "**********1*********2*********3*********4*********5*********6*********7*********" */
  g_print ("Commands:\n");
  for (list<Command*>::iterator it = Command::registry.begin(); it != Command::registry.end(); it++)
    {
      Command *cmd = *it;
      g_print ("  %s ", cmd->name.c_str());
      cmd->blurb (bshort);
    }
}

static bool
parse_str_option (char        **argv,
                  guint        &i,
                  const gchar  *arg,
                  const gchar *&str,
                  guint         argc)
{
  guint length = strlen (arg);
  if (strncmp (argv[i], arg, length) == 0)
    {
      const gchar *equal = argv[i] + length;
      if (*equal == '=')              /* -x=Arg */
        str = equal + 1;
      else if (*equal)                /* -xArg */
        str = equal;
      else if (i + 1 < argc)          /* -x Arg */
        {
          argv[i++] = NULL;
          str = argv[i];
        }
      argv[i] = NULL;
      if (str)
        return true;
    }
  return false;
}

static bool
parse_bool_option (char        **argv,
                   guint        &i,
                   const gchar  *arg)
{
  guint length = strlen (arg);
  if (strncmp (argv[i], arg, length) == 0)
    {
      argv[i] = NULL;
      return true;
    }
  return false;
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
      const gchar *str;
      if (strcmp (argv[i], "--") == 0)
        {
          argv[i] = NULL;
          break;
        }
      else if (parse_bool_option (argv, i, "--debug-list"))
        {
          g_print ("debug keys: all");
          g_print ("\n");
          exit (0);
        }
      else if (parse_str_option (argv, i, "--debug", str, argc))
        sfi_debug_allow (str);
      else if (parse_str_option (argv, i, "--no-debug", str, argc))
        sfi_debug_deny (str);
      else if (parse_bool_option (argv, i, "-h") ||
               parse_bool_option (argv, i, "--help"))
        {
          wavetool_print_blurb (false);
          exit (0);
        }
      else if (parse_bool_option (argv, i, "-v") ||
               parse_bool_option (argv, i, "--version"))
        {
          wavetool_print_version ();
          exit (0);
        }
      else if (parse_bool_option (argv, i, "-k"))
        continue_on_error = true;
      else if (parse_bool_option (argv, i, "-q"))
        quiet_infos = true;
      else if (parse_str_option (argv, i, "-o", str, argc))
        output_file = str;
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
  blurb (bool bshort)
  {
    g_print ("\n");
    if (bshort)
      return;
    g_print ("    Store the input bsewave as output bsewave. If both file names are the same,\n");
    g_print ("    the bsewave file is simply rewritten. Allthough no explicit modifications\n");
    g_print ("    are performed on the bsewave, externally referenced sample files will be\n");
    g_print ("    inlined, chunks may be reordered, and other changes related to the bsewave\n");
    g_print ("    storage process may occour.\n");
    /*       "**********1*********2*********3*********4*********5*********6*********7*********" */
  }
} cmd_store ("store");

class Create : public Command {
  bool force_creation;
public:
  Create (const char *command_name) :
    Command (command_name),
    force_creation (false)
  {}
  void
  blurb (bool bshort)
  {
    g_print ("<n_channels> [options]\n");
    if (bshort)
      return;
    g_print ("    Create an empty bsewave file, <n_channels>=1 (mono) and <n_channels>=2\n");
    g_print ("    (stereo) are currently supported. Options:\n");
    g_print ("    -N <wave-name>      name of the wave stored inside of <output.bsewave>\n");
    g_print ("    -f                  force creation even if the file exists already\n");
    /*       "**********1*********2*********3*********4*********5*********6*********7*********" */
  }
  string wave_name;
  string channel_str;
  guint
  parse_args (guint  argc,
              char **argv)
  {
    for (guint i = 1; i < argc; i++)
      {
        const gchar *str;
        if (parse_str_option (argv, i, "-N", str, argc))
          wave_name = str;
        else if (parse_bool_option (argv, i, "-f"))
          force_creation = true;
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
        sfi_error ("invalid number of channels: %s (%d)\n", channel_str.c_str(), n_channels);
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
    if (!force_creation && sfi_file_check (output_file.c_str(), "e"))
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
  blurb (bool bshort)
  {
    g_print ("[options]\n");
    if (bshort)
      return;
    g_print ("    Compress all chunks with the Vorbis audio codec and store the wave data\n");
    g_print ("    as Ogg/Vorbis streams inside the bsewave file. Options:\n");
    g_print ("    -Q <n>              use quality level <n>, refer to oggenc(1) for details\n");
    /*       "**********1*********2*********3*********4*********5*********6*********7*********" */
  }
  guint
  parse_args (guint  argc,
              char **argv)
  {
    for (guint i = 1; i < argc; i++)
      {
        const gchar *str;
        if (parse_str_option (argv, i, "-Q", str, argc))
          quality = g_ascii_strtod (str, NULL);
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
    GslVorbis1Handle *vhandle = gsl_vorbis1_handle_new (dhandle, 0);
    if (vhandle)
      gsl_vorbis1_handle_destroy (vhandle);
    return vhandle != NULL;
  }
  void
  exec (Wave *wave)
  {
    /* get the wave into storage order */
    wave->sort();
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
        BseErrorType error = gsl_vorbis_encoder_setup_stream (enc, gsl_vorbis_make_serialno());
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
        sfi_info ("ENCODING: chunk % 7.2f/%.0f", gsl_data_handle_osc_freq (chunk->dhandle), gsl_data_handle_mix_freq (chunk->dhandle));
        SfiNum n = 0, v = 0, l = gsl_data_handle_length (dhandle);
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
                v += MAX (r, 0);
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
                    v += MAX (r, 0);
                  }
              }
            if (!quiet_infos)
              g_printerr ("chunk % 7.2f/%.0f, processed %0.1f%%       \r",
                          gsl_data_handle_osc_freq (chunk->dhandle), gsl_data_handle_mix_freq (chunk->dhandle),
                          n * 99.999999 / l);
          }
        gsl_vorbis_encoder_pcm_done (enc);
        while (!gsl_vorbis_encoder_ogg_eos (enc))
          {
            guint8 buf[ENCODER_BUFFER];
            SfiNum j, r = gsl_vorbis_encoder_read_ogg (enc, ENCODER_BUFFER, buf);
            v += MAX (r, 0);
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
        guint n_bytes = (gsl_data_handle_bit_depth (dhandle) + 7) / 8;
        if (!quiet_infos)
          g_printerr ("chunk % 7.2f/%.0f, processed %0.1f%% (reduced to: %5.2f%%)      \n",
                      gsl_data_handle_osc_freq (chunk->dhandle), gsl_data_handle_mix_freq (chunk->dhandle),
                  n * 100.0 / l, v * 100.0 / (l * MAX (1, n_bytes)));
        if (close (tmpfd) < 0)
          {
            sfi_error ("chunk % 7.2f/%.0f: failed to write to tmp file: %s",
                       gsl_data_handle_osc_freq (chunk->dhandle), gsl_data_handle_mix_freq (chunk->dhandle),
                       g_strerror (errno));
            exit (1);
          }
        error = chunk->set_dhandle_from_file (temp_file, gsl_data_handle_osc_freq (dhandle), dhandle->setup.xinfos);
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

class AddChunk : public Command {
  struct OptChunk {
    const gchar *midi_note;
    gfloat       osc_freq;
    const gchar *sample_file;
    guint        auto_extract_type; /* 1=midi-note, 2=osc-freq */
    const gchar *auto_extract_str;
    GslWaveFormatType load_format;
    guint             load_mix_freq;
    guint             load_byte_order;
    OptChunk()
    {
      memset (this, 0, sizeof (*this));
      load_format = GSL_WAVE_FORMAT_SIGNED_16;
      load_mix_freq = 44100;
      load_byte_order = G_LITTLE_ENDIAN;
    }
  };
  list<OptChunk> opt_chunks;
  OptChunk&
  create_opt_chunk()
  {
    if (opt_chunks.empty())
      {
        opt_chunks.push_front (OptChunk());
        return opt_chunks.front();
      }
    else
      {
        opt_chunks.push_front (OptChunk (opt_chunks.front()));
        OptChunk &ochunk = opt_chunks.front();
        ochunk.sample_file = NULL;
        return ochunk;
      }
  }
  OptChunk&
  top_empty_opt_chunk()
  {
    if (opt_chunks.empty() || opt_chunks.front().sample_file)
      create_opt_chunk();
    return opt_chunks.front();
  }
  bool              load_raw;
public:
  enum LoadType { AUTO, RAW };
  AddChunk (const char *command_name,
            LoadType    ltype = AUTO) :
    Command (command_name),
    load_raw (false)
  {
    if (ltype == RAW)
      load_raw = true;
  }
  void
  blurb (bool bshort)
  {
    g_print ("[options] {-m=midi-note|-f=osc-freq} <sample-file> ...\n");
    if (!bshort && !load_raw)
      blurb_auto();
    else if (!bshort && load_raw)
      blurb_raw();
  }
  void
  blurb_auto ()
  {
    g_print ("    Add a new chunk containing <sample-file> to the wave file. For each chunk,\n");
    g_print ("    a unique oscillator frequency must be given to determine what note the\n");
    g_print ("    chunk is to be played back for. Multi oscillator frequency + sample-file\n");
    g_print ("    option combinations may be given on the command line to add multiple wave\n");
    g_print ("    chunks. The -f and -m options can be omitted for a sample file, if the\n");
    g_print ("    oscillator frequency can be determined through auto extract options.\n");
    g_print ("    Options:\n");
    g_print ("    -f <osc-freq>       oscillator frequency for the next chunk\n");
    g_print ("    -m <midi-note>      alternative way to specify oscillator frequency\n");
    g_print ("    --auto-extract-midi-note <nth>\n");
    g_print ("                        automatically retrieve the midi note by extracting the\n");
    g_print ("                        <nth> number from the base name of <sample-file>\n");
    g_print ("    --auto-extract-osc-freq <nth>\n");
    g_print ("                        automatically retrieve the oscillator frequency by\n");
    g_print ("                        extracting the <nth> number from the base name\n");
    g_print ("                        of <sample-file>\n");
    /*       "**********1*********2*********3*********4*********5*********6*********7*********" */
  }
  guint
  parse_args (guint  argc,
              char **argv)
  {
    for (guint i = 1; i < argc; i++)
      {
        const gchar *str;
        if (parse_str_option (argv, i, "--auto-extract-midi-note", str, argc))
          {
            OptChunk &ochunk = create_opt_chunk();
            ochunk.auto_extract_type = 1;
            ochunk.auto_extract_str = str;
          }
        else if (parse_str_option (argv, i, "--auto-extract-osc-freq", str, argc))
          {
            OptChunk &ochunk = create_opt_chunk();
            ochunk.auto_extract_type = 2;
            ochunk.auto_extract_str = str;
          }
        else if (parse_str_option (argv, i, "-m", str, argc))
          {
            OptChunk &ochunk = create_opt_chunk();
            ochunk.osc_freq = 0;
            ochunk.midi_note = str;
          }
        else if (parse_str_option (argv, i, "-f", str, argc))
          {
            OptChunk &ochunk = create_opt_chunk();
            ochunk.osc_freq = g_ascii_strtod (str, NULL);
            ochunk.midi_note = NULL;
          }
        else if (load_raw && parse_str_option (argv, i, "-R", str, argc))
          {
            OptChunk &ochunk = top_empty_opt_chunk();
            ochunk.load_mix_freq = g_ascii_strtoull (str, NULL, 10);
          }
        else if (load_raw && parse_str_option (argv, i, "-F", str, argc))
          {
            OptChunk &ochunk = top_empty_opt_chunk();
            ochunk.load_format = gsl_wave_format_from_string (str);
          }
        else if (load_raw && parse_str_option (argv, i, "-B", str, argc))
          {
            OptChunk &ochunk = top_empty_opt_chunk();
            ochunk.load_byte_order = gsl_byte_order_from_string (str);
          }
        else /* sample file name */
          {
            OptChunk &ochunk = top_empty_opt_chunk();
            ochunk.sample_file = argv[i];
            argv[i] = NULL;
          }
      }
    return 0; // no missing args
  }
  void
  blurb_raw ()
  {
    g_print ("    Add a new chunk just like with \"add-chunk\", but load raw sample data.\n");
    g_print ("    Additional raw sample format options are supported.\n");
    g_print ("    Raw sample format options:\n");
    g_print ("    -R <mix-freq>       mixing frequency for the next chunk [44100]\n");
    g_print ("    -F <format>         raw sample format, supported formats are:\n");
    g_print ("                        alaw, ulaw, float, signed-8, signed-12, signed-16,\n");
    g_print ("                        unsigned-8, unsigned-12, unsigned-16 [signed-16]\n");
    g_print ("    -B <byte-order>     raw sample byte order, supported types:\n");
    g_print ("                        little-endian, big-endian [little-endian]\n");
    /*       "**********1*********2*********3*********4*********5*********6*********7*********" */
  }
  static gdouble
  str2num (const gchar *str,
           guint        nth)
  {
    gchar *num_any = ".0123456789", *num_first = num_any + 1;
    while (nth--)
      {
        /* skip number */
        if (*str && strchr (num_first, *str))
          do
            str++;
          while (*str && strchr (num_any, *str));
        /* and trailing non-number stuff */
        while (*str && !strchr (num_first, *str))
          str++;
        if (!*str)
          return BSE_DOUBLE_NAN;
      }
    if (strchr (num_first, *str))
      return atof (str);
    return BSE_DOUBLE_NAN;
  }
  GslDataHandle*
  load_file_auto (const OptChunk &opt,
                  BseErrorType   &error)
  {
    GslDataHandle *dhandle = NULL;
    const char *sample_file = opt.sample_file;
    /* load sample file, auto-detecting file type */
    BseWaveFileInfo *winfo = bse_wave_file_info_load (sample_file, &error);
    if (winfo && winfo->n_waves == 1)
      {
        BseWaveDsc *wdsc = bse_wave_dsc_load (winfo, 0, TRUE, &error);
        if (wdsc && wdsc->n_chunks == 1)
          dhandle = bse_wave_handle_create (wdsc, 0, &error);
        else if (wdsc)
          error = BSE_ERROR_FORMAT_INVALID;
        if (wdsc)
          bse_wave_dsc_free (wdsc);
      }
    else if (winfo)
      error = BSE_ERROR_FORMAT_INVALID;
    if (winfo)
      bse_wave_file_info_unref (winfo);
    return dhandle;
  }
  GslDataHandle*
  load_file_raw (const OptChunk &opt,
                 guint           n_channels,
                 gfloat          osc_freq,
                 BseErrorType   &error)
  {
    GslDataHandle *dhandle = NULL;
    const char *sample_file = opt.sample_file;
    /* load sample file from raw data */
    dhandle = gsl_wave_handle_new (sample_file, n_channels,
                                   opt.load_format, opt.load_byte_order, opt.load_mix_freq, osc_freq,
                                   0, -1, NULL);
    error = dhandle ? BSE_ERROR_NONE : BSE_ERROR_IO;
    return dhandle;
  }
  void
  exec (Wave *wave)
  {
    for (list<OptChunk>::iterator it = opt_chunks.begin(); it != opt_chunks.end(); it++)
      if (it->sample_file)
        {
          const OptChunk &ochunk = *it;
          gchar **xinfos = NULL;
          /* figure osc freq */
          gfloat osc_freq = 0;
          if (ochunk.midi_note)
            {
              SfiNum num = g_ascii_strtoull (ochunk.midi_note, NULL, 10);
              osc_freq = 440.0 /* MIDI standard pitch */ * pow (BSE_2_POW_1_DIV_12, num - 69 /* MIDI kammer note */);
              xinfos = bse_xinfos_add_num (xinfos, "midi-note", num);
            }
          else if (ochunk.osc_freq)
            osc_freq = ochunk.osc_freq;
          else
            {
              /* find auto extract option */
              const gchar *auto_extract = NULL;
              guint auto_extract_type = 0;
              for (list<OptChunk>::iterator it2 = it; it2 != opt_chunks.end(); it2++)
                if (it2->auto_extract_type)
                  {
                    auto_extract_type = it2->auto_extract_type;
                    auto_extract = it2->auto_extract_str;
                    break;
                  }
              /* auto extract */
              if (auto_extract_type)
                {
                  gchar *bname = g_path_get_basename (ochunk.sample_file);
                  SfiNum nth = g_ascii_strtoull (auto_extract, NULL, 10);
                  if (auto_extract_type == 1)
                    {
                      SfiNum num = int (str2num (bname, nth));
                      osc_freq = 440.0 /* MIDI standard pitch */ * pow (BSE_2_POW_1_DIV_12, num - 69 /* MIDI kammer note */);
                      xinfos = bse_xinfos_add_num (xinfos, "midi-note", num);
                    }
                  else
                    osc_freq = str2num (bname, nth);
                  g_free (bname);
                }
            }
          xinfos = bse_xinfos_add_float (xinfos, "osc-freq", osc_freq);
          /* check osc_freq */
          if (osc_freq <= 0)
            {
              sfi_error ("none or invalid oscillator frequency given for wave chunk \"%s\": %.2f",
                         ochunk.sample_file, osc_freq);
              exit (1);
            }
          if (wave->lookup (osc_freq))
            {
              sfi_error ("wave \"%s\" already contains a chunk with oscillator frequency %.2f",
                         wave->name.c_str(), osc_freq);
              exit (1);
            }
          /* add sample file */
          BseErrorType error = BSE_ERROR_NONE;
          GslDataHandle *dhandle;
          sfi_info ("LOAD: osc-freq=%g file=%s", osc_freq, ochunk.sample_file);
          if (load_raw)
            dhandle = load_file_raw (ochunk, wave->n_channels, osc_freq, error);
          else
            dhandle = load_file_auto (ochunk, error);
          if (dhandle)
            {
              error = wave->add_chunk (dhandle, xinfos);
              gsl_data_handle_unref (dhandle);
            }
          if (error)
            {
              (continue_on_error ? sfi_warning : sfi_error) ("failed to add wave chunk from file \"%s\": %s",
                                                             ochunk.sample_file, bse_error_blurb (error));
              if (!continue_on_error)
                exit (1);
            }
          g_strfreev (xinfos);
        }
  }
} cmd_add_chunk ("add-chunk"), cmd_add_raw_chunk ("add-raw-chunk", AddChunk::RAW);

class XInfoCmd : public Command {
  vector<char*> args;
public:
  XInfoCmd (const char *command_name) :
    Command (command_name)
  {
  }
  void
  blurb (bool bshort)
  {
    g_print ("{-m=midi-note|-f=osc-freq|--all-chunks|--wave} key=[value] ...\n");
    if (bshort)
      return;
    g_print ("    Add, change or remove an XInfo string of a bsewave file.\n");
    g_print ("    Omission of [value] deletes the XInfo associated with the key.\n");
    g_print ("    Key and value pairs may be specified multiple times, optionally preceeded\n");
    g_print ("    by location options to control what portion of a bsewave file (the wave,\n");
    g_print ("    individual wave chunks or all wave chunks) should be affected.\n");
    g_print ("    Options:\n");
    g_print ("    -f <osc-freq>       oscillator frequency to select a wave chunk\n");
    g_print ("    -m <midi-note>      alternative way to specify oscillator frequency\n");
    g_print ("    --all-chunks        apply XInfo modification to all chunks\n");
    g_print ("    --wave              apply XInfo modifications to the wave itself\n");
    /*       "**********1*********2*********3*********4*********5*********6*********7*********" */
  }
  guint
  parse_args (guint  argc,
              char **argv)
  {
    args.clear();
    for (guint i = 1; i < argc; i++)
      {
        args.push_back (argv[i]);
        argv[i] = NULL;
      }
    return 0; /* no args missing */
  }
  typedef enum {
    NONE,
    OSC_FREQ,
    ALL_CHUNKS,
    WAVE
  } Location;
  void
  exec (Wave *wave)
  {
    gfloat osc_freq = 0;
    Location location = NONE;
    for (vector<char*>::iterator it = args.begin(); it != args.end(); it++)
      {
        const char *arg = *it;
        if (strcmp ("--all-chunks", arg) == 0)
          location = ALL_CHUNKS;
        else if (strcmp ("--wave", arg) == 0)
          location = WAVE;
        else if (strcmp ("-m", arg) == 0 ||
                 strncmp ("-m", arg, 2) == 0)
          {
            const gchar *equal = arg + 2;
            const gchar *note_str = NULL;
            if (*equal == '=')            /* -m=Arg */
              note_str = equal + 1;
            else if (*equal)              /* -mArg */
              note_str = equal;
            else if (it + 1 != args.end())/* -m Arg */
              {
                it++;
                note_str = *it;
              }
            if (note_str)
              {
                SfiNum num = g_ascii_strtoull (note_str, NULL, 10);
                osc_freq = 440.0 /* MIDI standard pitch */ * pow (BSE_2_POW_1_DIV_12, num - 69 /* MIDI kammer note */);
                location = OSC_FREQ;
              }
          }
        else if (strcmp ("-f", arg) == 0 ||
                 strncmp ("-f", arg, 2) == 0)
          {
            const gchar *equal = arg + 2;
            const gchar *freq_str = NULL;
            if (*equal == '=')            /* -f=Arg */
              freq_str = equal + 1;
            else if (*equal)              /* -fArg */
              freq_str = equal;
            else if (it + 1 != args.end())/* -f Arg */
              {
                it++;
                freq_str = *it;
              }
            if (freq_str)
              {
                osc_freq = g_ascii_strtod (freq_str, NULL);
                location = OSC_FREQ;
              }
          }
        else /* XInfo string */
          {
            const gchar *equal = strchr (arg, '=');
            if (!equal)
              {
                sfi_error ("missing \"=\" in XInfo assignment: %s\n", arg);
                exit (1);
              }
            gchar *key = g_strndup (arg, equal - arg);
            const gchar *value = equal + 1;
            g_strcanon (key, G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS, '-');
            if (!key[0] || strncmp (key, arg, equal - arg) != 0)
              {
                sfi_error ("invalid key specified in XInfo assignment: %s\n", arg);
                exit (1);
              }
            switch (location)
              {
              case NONE:
                sfi_error ("missing location option for XInfo assignment: %s\n", arg);
                exit (1);
              case WAVE:
                wave->set_xinfo (key, value);
                break;
              case OSC_FREQ:
                if (wave->lookup (osc_freq))
                  wave->set_chunk_xinfo (osc_freq, key, value);
                else
                  {
                    sfi_error ("failed to find wave chunk with oscillator frequency: %.2f", osc_freq);
                    if (!continue_on_error)
                      exit (1);
                  }
                break;
              case ALL_CHUNKS:
                wave->set_all_xinfo (key, value);
                break;
              }
            g_free (key);
          }
      }
  }
} cmd_xinfo ("xinfo");

class ClipCmd : public Command {
  gfloat threshold;
  guint head_samples, tail_samples, fade_samples, pad_samples, tail_silence;
  bool all_chunks;
  vector<gfloat> freq_list;
public:
  ClipCmd (const char *command_name) :
    Command (command_name),
    threshold (16 / 32767.),
    head_samples (0),
    tail_samples (0),
    fade_samples (16),
    pad_samples (16),
    tail_silence (0),
    all_chunks (false)
  {
  }
  void
  blurb (bool bshort)
  {
    g_print ("{-m=midi-note|-f=osc-freq|--all-chunks} [options]\n");
    if (bshort)
      return;
    g_print ("    Clip head and or tail of a wave chunk and produce fade-in ramps at the\n");
    g_print ("    beginning. Wave chunks which are clipped to an essential 0-length will\n");
    g_print ("    automatically be deleted.\n");
    g_print ("    Options:\n");
    g_print ("    -f <osc-freq>       oscillator frequency to select a wave chunk\n");
    g_print ("    -m <midi-note>      alternative way to specify oscillator frequency\n");
    g_print ("    --all-chunks        try to clip all chunks\n");
#if 0
    g_print ("    --config=\"s h t f p r\"\n");
    g_print ("                        clipping configuration, consisting of space-seperated\n");
    g_print ("                        configuration values:\n");
    g_print ("                        s) minimum signal threshold [%u]\n", guint (threshold * 32767));
    g_print ("                        h) silence samples to verify at head [%u]\n", head_samples);
    g_print ("                        t) silence samples to verify at tail [%u]\n", tail_samples);
    g_print ("                        f) samples to fade-in before signal starts [%u]\n", fade_samples);
    g_print ("                        p) padding samples after signal ends [%u]\n", pad_samples);
    g_print ("                        r) silence samples required at tail for clipping [%u]\n", tail_silence);
#endif
    g_print ("    -s=<threshold>      set the minimum signal threshold (0..32767) [%u]\n", guint (threshold * 32767));
    g_print ("    -h=<head-samples>   number of silence samples to verify at head [%u]\n", head_samples);
    g_print ("    -t=<tail-samples>   number of silence samples to verify at tail [%u]\n", tail_samples);
    g_print ("    -f=<fade-samples>   number of samples to fade-in before signal starts [%u]\n", fade_samples);
    g_print ("    -p=<pad-samples>    number of padding samples after signal ends [%u]\n", pad_samples);
    g_print ("    -r=<tail-silence>   number of silence samples required at tail to allow\n");
    g_print ("                        tail clipping [%u]\n", tail_silence);
    /*       "**********1*********2*********3*********4*********5*********6*********7*********" */
  }
  guint
  parse_args (guint  argc,
              char **argv)
  {
    bool seen_selection = false;
    for (guint i = 1; i < argc; i++)
      {
        const gchar *str;
        if (parse_bool_option (argv, i, "--all-chunks"))
          {
            all_chunks = true;
            seen_selection = true;
          }
        else if (parse_str_option (argv, i, "-f", str, argc))
          {
            freq_list.push_back (g_ascii_strtod (str, NULL));
            seen_selection = true;
          }
        else if (parse_str_option (argv, i, "-m", str, argc))
          {
            SfiNum num = g_ascii_strtoull (str, NULL, 10);
            gfloat osc_freq = 440.0 /* MIDI standard pitch */ * pow (BSE_2_POW_1_DIV_12, num - 69 /* MIDI kammer note */);
            freq_list.push_back (osc_freq);
            seen_selection = true;
          }
        else if (parse_str_option (argv, i, "-s", str, argc))
          threshold = g_ascii_strtod (str, NULL);
        else if (parse_str_option (argv, i, "-h", str, argc))
          head_samples = g_ascii_strtoull (str, NULL, 10);
        else if (parse_str_option (argv, i, "-t", str, argc))
          tail_samples = g_ascii_strtoull (str, NULL, 10);
        else if (parse_str_option (argv, i, "-f", str, argc))
          fade_samples = g_ascii_strtoull (str, NULL, 10);
        else if (parse_str_option (argv, i, "-p", str, argc))
          pad_samples = g_ascii_strtoull (str, NULL, 10);
        else if (parse_str_option (argv, i, "-r", str, argc))
          tail_silence = g_ascii_strtoull (str, NULL, 10);
      }
    return !seen_selection ? 1 : 0; /* # args missing */
  }
  void
  exec (Wave *wave)
  {
    sort (freq_list.begin(), freq_list.end());
    vector<list<WaveChunk>::iterator> deleted;
    /* level clipping */
    for (list<WaveChunk>::iterator it = wave->chunks.begin(); it != wave->chunks.end(); it++)
      if (all_chunks || wave->match (*it, freq_list))
        {
          WaveChunk *chunk = &*it;
          sfi_info ("CLIP: chunk %f", gsl_data_handle_osc_freq (chunk->dhandle));
          GslDataClipConfig cconfig = { 0 };
          cconfig.produce_info = TRUE;
          cconfig.threshold = threshold;
          cconfig.head_samples = head_samples;
          cconfig.tail_samples = tail_samples;
          cconfig.fade_samples = fade_samples;
          cconfig.pad_samples = pad_samples;
          cconfig.tail_silence = tail_silence;
          GslDataClipResult cresult;
          GslDataHandle *dhandle = chunk->dhandle;
          BseErrorType error;
          error = gsl_data_clip_sample (dhandle, &cconfig, &cresult);
          if (error == BSE_ERROR_DATA_UNMATCHED && cresult.clipped_to_0length)
            {
              sfi_info ("Deleting 0-length chunk");
              deleted.push_back (it);
              error = BSE_ERROR_NONE;
            }
          else if (error)
            {
              const gchar *reason = bse_error_blurb (error);
              if (!cresult.tail_detected)
                reason = "failed to detect silence at tail";
              if (!cresult.head_detected)
                reason = "failed to detect silence at head";
              sfi_error ("level clipping failed: %s", reason);
            }
          else
            {
              gchar **xinfos = bse_xinfos_dup_consolidated (chunk->dhandle->setup.xinfos, FALSE);
              if (cresult.clipped_tail)
                xinfos = bse_xinfos_add_value (xinfos, "loop-type", "unloopable");
              if (cresult.dhandle != chunk->dhandle)
                {
                  error = chunk->change_dhandle (cresult.dhandle, gsl_data_handle_osc_freq (chunk->dhandle), xinfos);
                  if (error)
                    sfi_error ("level clipping failed: %s", bse_error_blurb (error));
                }
              g_strfreev (xinfos);
            }
          if (error && !continue_on_error)
            exit (1);
        }
    /* really delete chunks */
    while (deleted.size())
      {
        wave->remove (deleted.back());
        deleted.pop_back();
      }
  }
} cmd_clip ("clip");

#if 0
class LoopCmd : public Command {
  bool all_chunks;
  vector<gfloat> freq_list;
public:
  LoopCmd (const char *command_name) :
    Command (command_name),
    all_chunks (false)
  {
  }
  void
  blurb (bool bshort)
  {
    g_print ("{-m=midi-note|-f=osc-freq|--all-chunks} [options]\n");
    if (bshort)
      return;
    g_print ("    Find suitable loop points.\n");
    /* bsewavetool loop <file.bsewave> [-a loop-algorithm] ...
     *         don't loop chunks with loop-type=unloopable xinfos
     *         automatically add xinfos with looping errors.
     *         allow ogg/vorbis package cutting at end-loop point
     */
    g_print ("    Options:\n");
    g_print ("    -f <osc-freq>       oscillator frequency to select a wave chunk\n");
    g_print ("    -m <midi-note>      alternative way to specify oscillator frequency\n");
    g_print ("    --all-chunks        try to loop all chunks\n");
    /*       "**********1*********2*********3*********4*********5*********6*********7*********" */
  }
  guint
  parse_args (guint  argc,
              char **argv)
  {
    bool seen_selection = false;
    for (guint i = 1; i < argc; i++)
      {
        const gchar *str;
        if (parse_bool_option (argv, i, "--all-chunks"))
          {
            all_chunks = true;
            seen_selection = true;
          }
        else if (parse_str_option (argv, i, "-f", str, argc))
          {
            freq_list.push_back (g_ascii_strtod (str, NULL));
            seen_selection = true;
          }
        else if (parse_str_option (argv, i, "-m", str, argc))
          {
            SfiNum num = g_ascii_strtoull (str, NULL, 10);
            gfloat osc_freq = 440.0 /* MIDI standard pitch */ * pow (BSE_2_POW_1_DIV_12, num - 69 /* MIDI kammer note */);
            freq_list.push_back (osc_freq);
            seen_selection = true;
          }
      }
    return !seen_selection ? 1 : 0; /* # args missing */
  }
  void
  exec (Wave *wave)
  {
    sort (freq_list.begin(), freq_list.end());
    vector<list<WaveChunk>::iterator> deleted;
    /* level clipping */
    for (list<WaveChunk>::iterator it = wave->chunks.begin(); it != wave->chunks.end(); it++)
      if (all_chunks || wave->match (*it, freq_list))
        {
          WaveChunk *chunk = &*it;
          sfi_info ("LOOP: chunk %f", gsl_data_handle_osc_freq (chunk->dhandle));
        }
  }
} cmd_loop ("loop");
#endif

/* TODO commands:
 * bsewavetool.1 # need manual page
 * bsewavetool merge <file.bsewave> <second.bsewave>
 * bsewavetool omit <file.bsewave> [-a remove-algorithm] ...
 *   -L    drop samples based on loop errors
 * bsewavetool del-wave <file.bsewave> {-m midi-note|-f osc-freq} ...
 * bsewavetool config-gus-patch <file.bsewave> {-f=<f>|-m=<m>|--all-chunks}
 *   -f osc-freq                select chunk to modify by frequency
 *   -m midi-note               alternative to -f
 *   --all-chunks               select all chunks for modifications
 *   --sustain-envelope=<a,d,s,r>
 *   --envelope=<a,d,s,r>       set envelope
 *   --reset-envelope           unset envelope
 *   --panning=<p>              set output panning
 *   --tremolo=<s,r,d>          tremolo, s.., r..., d...
 *   --vibrato=<s,r,d>          vibrato, s.., r..., d...
 */

} // BseWaveTool
