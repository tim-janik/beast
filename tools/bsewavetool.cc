// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsewavetool.hh"
#include "topconfig.h"
#include "bwtwave.hh"
#include <bse/bsemain.hh>	/* for bse_init_intern() */
#include <bse/bseloader.hh>
#include <bse/gslvorbis-enc.hh>
#include <bse/gsldatahandle-vorbis.hh>
#include <bse/bseresamplerimpl.hh>
#include <birnet/birnettests.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>
#include <map>
#include <algorithm>

/// The BseWaveTool namespace contains all functions of the bse wave tool utility.
namespace BseWaveTool {
using namespace Birnet;
#define PRG_NAME        ("bsewavetool")
#define	IROUND(dbl)	((int) (floor (dbl + 0.5)))
/* --- prototypes --- */
static void     wavetool_parse_args     (int    *argc_p,
                                         char ***argv_p);
static void     wavetool_print_blurb    (bool    bshort);
/* --- variables --- */
static bool   skip_errors = false;
static bool   silent_infos = false;
static string command_name;
static string input_file;
static string output_file;
list<Command*> Command::registry;
list<string>   unlink_file_list;
/* --- Command --- */
Command::Command (const char *command_name) :
  name (command_name)
{
  registry.push_back (this);
}
void
Command::blurb (bool bshort)
{
  g_print ("\n");
  if (bshort)
    return;
}
/* --- main program --- */
static void
wavetool_message_handler (const char              *domain,
                          Msg::Type                mtype,
                          const vector<Msg::Part> &parts)
{
  if (mtype == Msg::INFO)
    {
      if (!silent_infos)
        {
          String title, primary, secondary, details, checkmsg;
          for (uint i = 0; i < parts.size(); i++)
            switch (parts[i].ptype)
              {
              case '0': title     += (title.size()     ? "\n" : "") + parts[i].string; break;
              case '1': primary   += (primary.size()   ? "\n" : "") + parts[i].string; break;
              case '2': secondary += (secondary.size() ? "\n" : "") + parts[i].string; break;
              case '3': details   += (details.size()   ? "\n" : "") + parts[i].string; break;
              case 'c': checkmsg  += (checkmsg.size()  ? "\n" : "") + parts[i].string; break;
              }
          if (primary.size())
            g_printerr ("%s\n", primary.c_str());
          if (secondary.size())
            g_printerr ("%s\n", secondary.c_str());
          if (details.size())
            g_printerr ("%s\n", details.c_str());
        }
    }
  else
    Msg::default_handler (domain, mtype, parts);
}
extern "C" int
main (int   argc,
      char *argv[])
{
  std::set_terminate (__gnu_cxx::__verbose_terminate_handler);
  /* initialization */
  int orig_argc = argc;
  SfiInitValue values[] = {
    { "stand-alone",            "true" }, /* no rcfiles etc. */
    { "wave-chunk-padding",     NULL, 1, },
    { "dcache-block-size",      NULL, 8192, },
    { "dcache-cache-memory",    NULL, 5 * 1024 * 1024, },
    { NULL }
  };
  bse_init_inprocess (&argc, &argv, "BseWaveTool", values);
  Msg::allow_msgs ("main"); // FIXME
  Msg::set_thread_handler (wavetool_message_handler);
  Msg::configure (Msg::INFO, Msg::LOG_TO_HANDLER, "");
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
                      if (skip_errors)
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
  bool needs_saving = command->exec (wave);
  /* save */
  if (needs_saving)
    {
      g_printerr ("SAVE: %s\n", output_file.c_str());
      wave->sort();
      error = wave->store (output_file);
      if (error)
        {
          sfi_error ("failed to store wave \"%s\" to file \"%s\": %s", wave->name.c_str(), output_file.c_str(), bse_error_blurb (error));
          exit (1);
        }
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
  g_print ("of the GNU Lesser General Public License which can be found in\n");
  g_print ("the BEAST source package. Sources, examples and contact\n");
  g_print ("information are available at http://beast.testbit.eu/.\n");
}
static void
wavetool_print_blurb (bool bshort)
{
  /* Since global and command specific options can collide, we reserve short
   * options for command specific options, and use long options for global
   * options, so that short options are generally available for individual
   * commands.
   *
   * The only exceptions are -o, -h and -v, which are global but short options,
   * since these are common practice.
   */
  g_print ("Usage: bsewavetool [tool-options] command <file.bsewave> {command-arguments}\n");
  g_print ("Tool options:\n");
  g_print ("  -o <output.bsewave>   name of the destination file (default: <file.bsewave>)\n");
  g_print ("  --silent              suppress extra processing information\n");
  g_print ("  --skip-errors         skip errors (may overwrite bsewave files after load\n");
  g_print ("                        errors occoured for part of its contents)\n");
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
                  const gchar **strp,
                  guint         argc)
{
  guint length = strlen (arg);
  if (strncmp (argv[i], arg, length) == 0)
    {
      const gchar *equal = argv[i] + length;
      if (*equal == '=')              /* -x=Arg */
        *strp = equal + 1;
      else if (*equal)                /* -xArg */
        *strp = equal;
      else if (i + 1 < argc)          /* -x Arg */
        {
          argv[i++] = NULL;
          *strp = argv[i];
        }
      argv[i] = NULL;
      if (*strp)
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
/* Since the function g_str_hash has changed between glib 2.26 and glib 2.28,
 * we include the original code here to make chunk keys behave the same with
 * both glib versions, so the unit tests pass with any glib version.
 */
static guint
old_g_str_hash (gconstpointer v)
{
  /* 31 bit hash function */
  const signed char *p = static_cast<const signed char *> (v);
  guint32 h = *p;
  if (h)
    for (p += 1; *p != '\0'; p++)
      h = (h << 5) - h + *p;
  return h;
}
/* wave chunk keys for shell iteration */
class WaveChunkKey {
  BseFloatIEEE754 m_osc_freq;
public:
  WaveChunkKey (const String& key_string)
  {
    // mark key as invalid
    m_osc_freq.v_float = -1;
    if (key_string.size() != 7)
      return;  // invalid key
    uint64 key_uint = 0;
    for (string::const_reverse_iterator si = key_string.rbegin(); si != key_string.rend(); si++)
      {
        key_uint *= 62;
        if (*si >= '0' && *si <= '9')
          key_uint += *si - '0';
        else if (*si >= 'A' && *si <= 'Z')
          key_uint += *si - 'A' + 10;
        else if (*si >= 'a' && *si <= 'z')
          key_uint += *si - 'a' + 10 + 26;
        else
          return; // invalid key
      }
    const uint64 key_checksum = key_uint & 0x1ff;
    key_uint ^= key_checksum << 32LL;  // deobfuscate high bits with checksum
    const uint64 checksum = old_g_str_hash (string_printf ("%lld", key_uint - key_checksum).c_str()) % 509;
    if (key_checksum != checksum)
      return; // invalid key
    key_uint >>= 9;
    // decode float components in byte order independent way
    m_osc_freq.mpn.mantissa = key_uint;
    key_uint >>= 23;
    m_osc_freq.mpn.biased_exponent = key_uint;
    key_uint >>= 8;
    m_osc_freq.mpn.sign = key_uint;
  }
  WaveChunkKey (float osc_freq)
  {
    m_osc_freq.v_float = osc_freq;
  }
  String
  as_string() const
  {
    uint64 key_uint = 0;
    // put float components to key in byte order independent way
    key_uint |= m_osc_freq.mpn.sign;            //  1 bit
    key_uint <<= 8;
    key_uint |= m_osc_freq.mpn.biased_exponent; //  8 bit
    key_uint <<= 23;
    key_uint |= m_osc_freq.mpn.mantissa;        // 23 bit
    key_uint <<= 9;                             // +9 bit checksum
    const uint64 checksum = old_g_str_hash (string_printf ("%lld", key_uint).c_str()) % 509;
    key_uint |= checksum;
    key_uint ^= checksum << 32LL;               // obfuscate high bits with checksum
    string key_string;
    for (int i = 0; i < 7; i++) /* encode in custom base-62 format; 7 digits  <=>  2^41 < 62^7 */
      {
        uint64 digit = key_uint % 62;
        if (digit < 10)
          key_string += '0' + digit;
        else if (digit < (10 + 26))
          key_string += 'A' + digit - 10;
        else if (digit < (10 + 26 + 26))
          key_string += 'a' + digit - 10 - 26;
        key_uint /= 62;
      }
    g_assert (key_uint == 0);
    return key_string;
  }
  bool
  is_valid() const
  {
    return m_osc_freq.v_float >= 0;
  }
  float
  osc_freq() const
  {
    return m_osc_freq.v_float;
  }
  static void
  unit_test()
  {
    struct TestData {
      const char *key;
      double      value;
    } decode_test[] = {
      { "YJv6nCS", 1.000000000e-03 }, { "gcQWdrQ", 1.256637061e-03 }, { "dXeM2zG", 1.579136704e-03 },
      { "3BSjFXD", 1.984401708e-03 }, { "z2ncv68", 2.493672730e-03 }, { "3FgEbrN", 3.133641572e-03 },
      { "3udggXH", 3.937850137e-03 }, { "mwJOdP8", 4.948448424e-03 }, { "Wq8Df29", 6.218403687e-03 },
      { "1PCvWKP", 7.814276536e-03 }, { "6prz6TI", 9.819709503e-03 }, { "uAVbweQ", 1.233981089e-02 },
      { "WiAHl0H", 1.550666370e-02 }, { "QTga6LU", 1.948624831e-02 }, { "R5IamYE", 2.448714181e-02 },
      { "X8Hq3EV", 3.077144993e-02 }, { "bTysq86", 3.866854441e-02 }, { "O44r1F6", 4.859232602e-02 },
      { "6Xtdz9H", 6.106291778e-02 }, { "oTnEADX", 7.673392556e-02 }, { "lYHPo92", 9.642669472e-02 },
      { "JIN6b3N", 1.211733583e-01 }, { "YHQ64CY", 1.522709329e-01 }, { "IYvE8J2", 1.913492977e-01 },
      { "H3PGUBB", 2.404566191e-01 }, { "RPKNVNW", 3.021666993e-01 }, { "oKEsrTE", 3.797138730e-01 },
      { "Ib6py6X", 4.771625256e-01 }, { "vUBdWbW", 5.996201140e-01 }, { "NeXqt5A", 7.535048580e-01 },
      { "onjLlCN", 9.468821305e-01 }, { "KqKpl8b", 1.189887178e+00 }, { "sFUvRra", 1.495256327e+00 },
      { "iDMmswb", 1.878994517e+00 }, { "Qi98zf1", 2.361214148e+00 }, { "fwGfS3U", 2.967189208e+00 },
      { "3QiAhhc", 3.728679927e+00 }, { "KjOWeVX", 4.685597387e+00 }, { "qd1u3XX", 5.888095331e+00 },
      { "Sjny6ta", 7.399198815e+00 }, { "Y7H8x3b", 9.298107455e+00 }, { "OPQUQ92", 1.168434643e+01 },
      { "WDDAiXY", 1.468298276e+01 }, { "NW2gCXR", 1.845118031e+01 }, { "Z3jckdV", 2.318643701e+01 },
      { "vHT1zmF", 2.913693606e+01 }, { "B1nbexX", 3.661455372e+01 }, { "Rm9vGPI", 4.601120519e+01 },
      { "NRKoXwb", 5.781938568e+01 }, { "n3ckrYC", 7.265798291e+01 }, { "hePAxF9", 9.130471414e+01 },
      { "1tMGZWQ", 1.147368877e+02 }, { "DEDCuGT", 1.441826254e+02 }, { "WhpTjIX", 1.811852306e+02 },
      { "7Qfdr93", 2.276840758e+02 }, { "sDV9XtJ", 2.861162480e+02 }, { "9BhSRT5", 3.595442811e+02 },
      { "wVwxSBb", 4.518166688e+02 }, { "ZIun3lR", 5.677695710e+02 }, { "kfGMuca", 7.134802853e+02 },
      { "xm31dSY", 8.965857691e+02 }, { "3Jw8sqH", 1.126682906e+03 }, { "KV35vV7", 1.415831496e+03 },
      { "hgCCJWQ", 1.779186331e+03 }, { "EbldxII", 2.235791483e+03 }, { "I29fmr4", 2.809578439e+03 },
      { "kso3Dxa", 3.530620394e+03 }, { "v7xZPlV", 4.436708436e+03 }, { "rJErUN6", 5.575332252e+03 },
      { "5IleXBA", 7.006169138e+03 }, { "N9TbUyN", 8.804211797e+03 }, { "oChbIqW", 1.106369884e+04 },
      { "wezNP3V", 1.390305400e+04 }, { "RisqTI5", 1.747109292e+04 }, { "QuypC2Z", 2.195482287e+04 },
      { "7xalGea", 2.758924410e+04 }, { "XmOEsnX", 3.466966663e+04 }, { "hiKocjG", 4.356718800e+04 },
      { "70aj5NU", 5.474814310e+04 }, { "sOSufiT", 6.879854566e+04 }, { "xBZnuLY", 8.645480225e+04 },
      { "SoNwkyC", 1.086423086e+05 }, { "ul4qK0V", 1.365239515e+05 }, { "ka8lEEJ", 1.715610572e+05 },
      { "KE6hhDL", 2.155899828e+05 }, { "sP4x8p5", 2.709183625e+05 }, { NULL, 0 }
    };
#if 0
    for (double value = 0.001; value < 300000; value *= PI / 2.5) /* useful oscillator freqs */
      printf ("{ \"%s\", %-.9e }, \n", WaveChunkKey (value).as_string().c_str(), value);
#endif
    /* The test consists of these steps which determine the choice of the epsilon value:
     *   (1) encode a double (see above) to a float key
     *     -> precision loss will occur here as since we only encode floats, the double
     *        will be converted to the nearest float and then will be encoded
     *   (2) store the double (into this sourcefile) with somewhat more than float precision
     *     -> precision loss will occur because we do not store the double itself, but only
     *        a %-.9e representation
     *   (3) decode the float key (see below), and compare it with the original double
     *     -> precision wise, fabs, subtraction, division will cause some more precision loss,
     *        but not much since we're operating on doubles
     *
     * The dominating factor here is the precision loss caused by step (1), the
     * precision loss introduced in the other steps is small compared to that. Thus the
     * epsilon below is choosen somewhat higher than BSE_FLOAT_EPSILON.
     */
    const double epsilon = 6e-8;
    TSTART ("Decoding valid chunk keys");
    for (int i = 0; decode_test[i].key; i++)
      {
        WaveChunkKey key = WaveChunkKey (decode_test[i].key);
        TCHECK (key.is_valid());
        TCHECK (fabs ((decode_test[i].value - key.osc_freq()) / decode_test[i].value) < epsilon);
        if (i % 2 == 0)
          TICK();
      }
    TDONE();
    TSTART ("Encoding chunk keys");
    int i = 0;
    for (double value = 1e-20; value < 1e+20; value *= PI / 3.1)
      {
        WaveChunkKey encoded_key (value);
        WaveChunkKey decoded_key (encoded_key.as_string());
        TCHECK (decoded_key.is_valid());
        TCHECK (fabs ((value - decoded_key.osc_freq()) / value) < epsilon);
        if (i++ % 150 == 0)
          TICK();
      }
    TDONE();
    TSTART ("Invalid chunk keys");
    TASSERT (!WaveChunkKey ("badchr+").is_valid());
    TASSERT (!WaveChunkKey ("badlen").is_valid());
    TASSERT (!WaveChunkKey ("badlength").is_valid());
    TASSERT (!WaveChunkKey ("1234567").is_valid());
    TASSERT (!WaveChunkKey ("KE6hhDx").is_valid());
    TASSERT (!WaveChunkKey ("ka8leeJ").is_valid());
    TASSERT (!WaveChunkKey ("coolkey").is_valid());
    TDONE();
  }
};
static bool
parse_chunk_selection (char          **argv,
                       uint           &i,
                       uint            argc,
                       bool           &all_chunks,
                       vector<float>  &freq_list)
{
  const gchar *str = NULL;
  if (parse_bool_option (argv, i, "--all-chunks"))
    {
      all_chunks = true;
      return true;
    }
  else if (parse_str_option (argv, i, "-f", &str, argc))
    {
      freq_list.push_back (g_ascii_strtod (str, NULL));
      return true;
    }
  else if (parse_str_option (argv, i, "--chunk-key", &str, argc))
    {
      WaveChunkKey key (str);
      if (key.is_valid())
        {
          freq_list.push_back (key.osc_freq());
          return true;
        }
      else
        {
          sfi_error ("invalid bsewavetool chunk key: %s", str);
          exit (1);
        }
    }
  else if (parse_str_option (argv, i, "-m", &str, argc))
    {
      SfiNum num = g_ascii_strtoull (str, NULL, 10);
      gfloat osc_freq = 440.0 /* MIDI standard pitch */ * pow (BSE_2_POW_1_DIV_12, num - 69. /* MIDI kammer note */);
      freq_list.push_back (osc_freq);
      return true;
    }
  return false;
}
static void
verify_chunk_selection (const vector<float> &freq_list,
                        Wave                *wave)
{
  for (vector<float>::const_iterator fi = freq_list.begin(); fi != freq_list.end(); fi++)
    {
      if (!wave->lookup (*fi))
        {
          Msg::display (skip_errors ? Msg::WARNING : Msg::ERROR,
                        Msg::Primary ("failed to find wave chunk with oscillator frequency: %.2f", *fi));
          if (!skip_errors)
            exit (1);
        }
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
    Msg::allow_msgs (envar);
  envar = getenv ("BSEWAVETOOL_NO_DEBUG");
  if (envar)
    Msg::deny_msgs (envar);
  for (i = 1; i < argc; i++)
    {
      const gchar *str = NULL;
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
      else if (parse_str_option (argv, i, "--debug", &str, argc))
        Msg::allow_msgs (str);
      else if (parse_str_option (argv, i, "--no-debug", &str, argc))
        Msg::deny_msgs (str);
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
      else if (parse_bool_option (argv, i, "--skip-errors"))
        skip_errors = true;
      else if (parse_bool_option (argv, i, "--silent"))
        silent_infos = true;
      else if (parse_bool_option (argv, i, "--unit-test"))
        {
          WaveChunkKey::unit_test();
          exit (0);
        }
      else if (parse_str_option (argv, i, "-o", &str, argc))
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
  bool
  exec (Wave   *wave)
  {
    /* nothing to do */
    return true;
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
        const gchar *str = NULL;
        if (parse_str_option (argv, i, "-N", &str, argc))
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
  bool
  exec (Wave *wave)
  {
    if (!force_creation && birnet_file_check (output_file.c_str(), "e"))
      {
        sfi_error ("not creating \"%s\": %s\n", output_file.c_str(), g_strerror (EEXIST));
        exit (1);
      }
    return true;
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
    g_print ("    -q <n>              use quality level <n>, refer to oggenc(1) for details\n");
    /*       "**********1*********2*********3*********4*********5*********6*********7*********" */
  }
  guint
  parse_args (guint  argc,
              char **argv)
  {
    for (guint i = 1; i < argc; i++)
      {
        const gchar *str = NULL;
        if (parse_str_option (argv, i, "-q", &str, argc))
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
  bool
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
            if (!silent_infos)
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
        if (!silent_infos)
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
    return true;
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
        const gchar *str = NULL;
        if (parse_str_option (argv, i, "--auto-extract-midi-note", &str, argc))
          {
            OptChunk &ochunk = create_opt_chunk();
            ochunk.auto_extract_type = 1;
            ochunk.auto_extract_str = str;
          }
        else if (parse_str_option (argv, i, "--auto-extract-osc-freq", &str, argc))
          {
            OptChunk &ochunk = create_opt_chunk();
            ochunk.auto_extract_type = 2;
            ochunk.auto_extract_str = str;
          }
        else if (parse_str_option (argv, i, "-m", &str, argc))
          {
            OptChunk &ochunk = create_opt_chunk();
            ochunk.osc_freq = 0;
            ochunk.midi_note = str;
          }
        else if (parse_str_option (argv, i, "-f", &str, argc))
          {
            OptChunk &ochunk = create_opt_chunk();
            ochunk.osc_freq = g_ascii_strtod (str, NULL);
            ochunk.midi_note = NULL;
          }
        else if (load_raw && parse_str_option (argv, i, "-R", &str, argc))
          {
            OptChunk &ochunk = top_empty_opt_chunk();
            ochunk.load_mix_freq = g_ascii_strtoull (str, NULL, 10);
          }
        else if (load_raw && parse_str_option (argv, i, "-F", &str, argc))
          {
            OptChunk &ochunk = top_empty_opt_chunk();
            ochunk.load_format = gsl_wave_format_from_string (str);
          }
        else if (load_raw && parse_str_option (argv, i, "-B", &str, argc))
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
    const gchar *num_any = ".0123456789", *num_first = num_any + 1;
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
  bool
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
              osc_freq = 440.0 /* MIDI standard pitch */ * pow (BSE_2_POW_1_DIV_12, num - 69. /* MIDI kammer note */);
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
                      osc_freq = 440.0 /* MIDI standard pitch */ * pow (BSE_2_POW_1_DIV_12, num - 69. /* MIDI kammer note */);
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
              Msg::display (skip_errors ? Msg::WARNING : Msg::ERROR,
                            Msg::Primary (_("failed to add wave chunk from file \"%s\": %s"),
                                          ochunk.sample_file, bse_error_blurb (error)));
              if (!skip_errors)
                exit (1);
            }
          g_strfreev (xinfos);
        }
    return true;
  }
} cmd_add_chunk ("add-chunk"), cmd_add_raw_chunk ("add-raw-chunk", AddChunk::RAW);
class DelChunkCmd : public Command {
  vector<gfloat> m_freq_list;
  bool           m_all_chunks;
public:
  void
  blurb (bool bshort)
  {
    g_print ("{-m=midi-note|-f=osc-freq|--chunk-key=key|--all-chunks}\n");
    if (bshort)
      return;
    g_print ("    Removes one or more chunks from the bsewave file.\n");
    g_print ("    Options:\n");
    g_print ("    -f <osc-freq>       oscillator frequency to select a wave chunk\n");
    g_print ("    -m <midi-note>      alternative way to specify oscillator frequency\n");
    g_print ("    --chunk-key <key>   select wave chunk using chunk key from list-chunks\n");
    g_print ("    --all-chunks        delete all chunks\n");
    /*       "**********1*********2*********3*********4*********5*********6*********7*********" */
  }
  guint
  parse_args (guint  argc,
              char **argv)
  {
    bool seen_selection = false;
    for (guint i = 1; i < argc; i++)
      {
	if (parse_chunk_selection (argv, i, argc, m_all_chunks, m_freq_list))
          seen_selection = true;
      }
    return !seen_selection ? 1 : 0; /* # args missing */
  }
  bool
  exec (Wave *wave)
  {
    /* get the wave into storage order */
    wave->sort();
    list<WaveChunk>::iterator it = wave->chunks.begin();
    while (it != wave->chunks.end())
      {
        list<WaveChunk>::iterator next_it = it;
        next_it++;
        if (m_all_chunks || wave->match (*it, m_freq_list))
          {
            GslDataHandle *dhandle = it->dhandle;
            double osc_freq = gsl_data_handle_osc_freq (dhandle);
            sfi_info ("DELETE: osc-freq=%g", osc_freq);
            wave->remove (it);
          }
        it = next_it;
      }
    return true;
  }
  DelChunkCmd (const char *command_name) :
    Command (command_name),
    m_all_chunks (false)
  {
  }
} cmd ("del-chunk");
class XInfoCmd : public Command {
  vector<char*> args;
public:
  XInfoCmd (const char *command_name) :
    Command (command_name)
  {}
  void
  blurb (bool bshort)
  {
    g_print ("{-m=midi-note|-f=osc-freq|--chunk-key=key|--all-chunks|--wave} key=[value] ...\n");
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
    g_print ("    --chunk-key <key>   select wave chunk using chunk key from list-chunks\n");
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
  bool
  exec (Wave *wave)
  {
    float osc_freq = 0;
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
            const char *equal = arg + 2;
            const char *note_str = NULL;
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
                osc_freq = 440.0 /* MIDI standard pitch */ * pow (BSE_2_POW_1_DIV_12, num - 69. /* MIDI kammer note */);
                location = OSC_FREQ;
              }
          }
        else if (strcmp ("-f", arg) == 0 ||
                 strncmp ("-f", arg, 2) == 0)
          {
            const char *equal = arg + 2;
            const char *freq_str = NULL;
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
        else if (strcmp ("--chunk-key", arg) == 0 ||
                 strncmp ("--chunk-key", arg, 11) == 0)
          {
            const char *equal = arg + 11;
            const char *key_str = NULL;
            if (*equal == '=')              /* --chunk-key=Arg */
              key_str = equal + 1;
            else if (it + 1 != args.end())  /* --chunk-key Arg */
              {
                it++;
                key_str = *it;
              }
            if (key_str)
              {
                WaveChunkKey key (key_str);
                if (key.is_valid())
                  {
                    osc_freq = key.osc_freq();
                    location = OSC_FREQ;
                  }
                else
                  {
                    sfi_error ("invalid bsewavetool chunk key: %s", key_str);
                    exit (1);
                  }
              }
          }
        else /* XInfo string */
          {
            const char *equal = strchr (arg, '=');
            if (!equal)
              {
                sfi_error ("missing \"=\" in XInfo assignment: %s\n", arg);
                exit (1);
              }
            char *key = g_strndup (arg, equal - arg);
            const char *value = equal + 1;
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
                    if (!skip_errors)
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
    return true;
  }
} cmd_xinfo ("xinfo");
class InfoCmd : public Command {
  vector<gfloat> m_freq_list;
  vector<String> m_fields;
  bool           m_all_chunks;
  bool           m_location_wave;
  enum
  {
    SCRIPT,
    MEDIUM,
    FULL
  }              m_output_format;
public:
  InfoCmd (const char *command_name) :
    Command (command_name),
    m_all_chunks (false),
    m_location_wave (false),
    m_output_format (MEDIUM)
  {
  }
  void
  blurb (bool bshort)
  {
    g_print ("{-m=midi-note|-f=osc-freq|--chunk-key=key|--all-chunks|--wave} [options]\n");
    if (bshort)
      return;
    g_print ("    Print information about the chunks of a bsewave file.\n");
    g_print ("    Options:\n");
    g_print ("    -f <osc-freq>       oscillator frequency to select a wave chunk\n");
    g_print ("    -m <midi-note>      alternative way to specify oscillator frequency\n");
    g_print ("    --all-chunks        show information for all chunks (default)\n");
    g_print ("    --chunk-key <key>   select wave chunk using chunk key from list-chunks\n");
    g_print ("    --wave              show information for the wave\n");
    g_print ("    --pretty=medium     use human readable format (default)\n");
    g_print ("    --pretty=full       use human readable format with all details\n");
    g_print ("    --script <field1>,<field2>,<field3>,...,<fieldN>\n");
    g_print ("                        use script readable line based space separated output\n");
    g_print ("    Valid wave or chunk fields:\n");
    g_print ("      channels          number of channels\n");
    g_print ("      label             user interface label\n");
    g_print ("      blurb             associated comment\n");
    g_print ("    Valid wave fields:\n");
    g_print ("      authors           authors who participated in creating the wave file\n");
    g_print ("      license           license specifying redistribution and other legal terms\n");
    g_print ("      play-type         set of required play back facilities for a wave\n");
    g_print ("    Valid chunk fields:\n");
    g_print ("      osc-freq          frequency of the chunk\n");
    g_print ("      mix-freq          sampling rate of the chunk\n");
    g_print ("      midi-note         midi note of a chunk\n");
    g_print ("      length            length of the chunk in sample frames\n");
    g_print ("      volume            volume at which the chunk is to be played\n");
    g_print ("      format            storage format used to save the chunk data\n");
    g_print ("      loop-type         whether the chunk is to be looped\n");
    g_print ("      loop-start        offset in sample frames for the start of the loop\n");
    g_print ("      loop-end          offset in sample frames for the end of the loop\n");
    g_print ("      loop-count        maximum limit for how often the loop should be repeated\n");
    g_print ("    Chunk fields that can be computed for the signal:\n");
    g_print ("      +avg-energy-raw   average signal energy (dB) of the raw data of the chunk\n");
    g_print ("      +avg-energy       average signal energy (dB) using volume xinfo\n");
    g_print ("    The script output consists of one line per chunk. The individual fields\n");
    g_print ("    of a line are separated by a single space. Special characters are escaped,\n");
    g_print ("    such as spaces, tabs, newlines and backslashes. So each line of script\n");
    g_print ("    parsable output can be parsed using the read(P) shell command.\n");
    g_print ("    Optional fields will printed as a single (escaped) space.\n");
    g_print ("    The human readable output formats (--pretty) may vary in future versions\n");
    g_print ("    and are not recommended as script input.\n");
    /*       "**********1*********2*********3*********4*********5*********6*********7*********" */
  }
  static vector<String>
  string_tokenize (const String& str)
  {
    vector<String> words;
    String::const_iterator word_start = str.begin();
    for (String::const_iterator si = str.begin(); si != str.end(); si++)
      {
        if (*si == ',') /* colon indicates word boundary */
          {
            words.push_back (String (word_start, si));
            word_start = si + 1;
          }
      }
    if (!str.empty()) /* handle last word in string */
      words.push_back (String (word_start, str.end()));
    return words;
  }
  guint
  parse_args (guint  argc,
              char **argv)
  {
    bool seen_selection = false;
    for (guint i = 1; i < argc; i++)
      {
        const char *str = NULL;
	if (parse_chunk_selection (argv, i, argc, m_all_chunks, m_freq_list))
          seen_selection = true;
        else if (parse_bool_option (argv, i, "--wave"))
          {
            m_location_wave = true;
            seen_selection = true;
          }
        else if (parse_str_option (argv, i, "--script", &str, argc))
          {
            m_output_format = SCRIPT;
            m_fields = string_tokenize (str);
          }
        else if (parse_str_option (argv, i, "--pretty", &str, argc))
          {
            if (strcmp (str, "medium") == 0)
              m_output_format = MEDIUM;
            else if (strcmp (str, "full") == 0)
              m_output_format = FULL;
            else
              {
                sfi_error ("invalid argument to --pretty in info command: %s", str);
                exit (1);
              }
          }
      }
    if (!seen_selection) /* default to all chunks */
      m_all_chunks = true;
    return 0; /* # args missing */
  }
  string
  dhandle_storage_format (GslDataHandle *dhandle)
  {
    GslDataHandle *tmp_handle = dhandle;
    do        /* skip comment or cache handles */
      {
        dhandle = tmp_handle;
        tmp_handle = gsl_data_handle_get_source (dhandle);
      }
    while (tmp_handle);
    GslVorbis1Handle *vhandle = gsl_vorbis1_handle_new (dhandle, 0);
    if (vhandle)
      {
        gsl_vorbis1_handle_destroy (vhandle);
        return "ogg";
      }
    return "raw";
  }
  int64
  dhandle_n_frames (GslDataHandle *dhandle)
  {
    int64 n_values = gsl_data_handle_length (dhandle);
    int64 n_channels = gsl_data_handle_n_channels (dhandle);
    int64 n_frames = n_values / n_channels;
    g_return_val_if_fail (n_values % n_channels == 0, n_frames);  /* a datahandle cannot contain half frames */
    return n_frames;
  }
  static double
  feature_avg_energy (GslDataHandle *dhandle,
                      double         volume_adjustment)
  {
    const double min_db = -200;
    g_return_val_if_fail (GSL_DATA_HANDLE_OPENED (dhandle), min_db);
    /* We do not take into account that a data handle can contain many separate
     * channels, so we're effectively averaging over all channels here.
     */
    GslDataPeekBuffer peek_buffer = { +1 /* incremental direction */, 0, };
    double avg_energy = 0;
    for (int64 i = 0; i < dhandle->setup.n_values; i++)
      {
        double v = gsl_data_handle_peek_value (dhandle, i, &peek_buffer);
        avg_energy += v * v;
      }
    avg_energy /= MAX (dhandle->setup.n_values, 1);
    avg_energy *= volume_adjustment * volume_adjustment;
    if (avg_energy > 0)
      return 10 * log (avg_energy) / log (10);
    else
      return min_db;
  }
  void
  script_output_escaped (const char *s)
  {
    if (!s || strlen (s) == 0)
      s = " ";
    while (*s)
      {
        if (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\\')  /* shell IFS and backslash */
          g_print ("\\");
        g_print ("%c", *s++);
      }
  }
  void
  script_output_xstr (char       **xinfos,
                      const char  *what)
  {
    script_output_escaped (bse_xinfos_get_value (xinfos, what));
  }
  void
  script_output_xnum (char       **xinfos,
                      const char  *what)
  {
    if (bse_xinfos_get_value (xinfos, what))
      g_print ("%lld", bse_xinfos_get_num (xinfos, what));
    else
      script_output_escaped (NULL);
  }
  bool
  exec (Wave *wave)
  {
    sort (m_freq_list.begin(), m_freq_list.end());
    verify_chunk_selection (m_freq_list, wave);
    if (m_output_format == SCRIPT && m_location_wave)
      {
        for (vector<string>::const_iterator fi = m_fields.begin(); fi != m_fields.end(); fi++)
          {
            if (fi != m_fields.begin()) // not first field
              g_print (" ");
            if (*fi == "channels")
              g_print ("%d", wave->n_channels);
            else if (*fi == "label")
              script_output_escaped (wave->name.c_str());
            else if (*fi == "blurb")
              script_output_xstr (wave->wave_xinfos, "blurb");
            else if (*fi == "authors")
              script_output_xstr (wave->wave_xinfos, "authors");
            else if (*fi == "license")
              script_output_xstr (wave->wave_xinfos, "license");
            else if (*fi == "play-type")
              script_output_xstr (wave->wave_xinfos, "play-type");
            else
              {
                sfi_error ("info command on the wave: invalid field for --script format: %s", fi->c_str());
                exit (1);
              }
          }
        g_print ("\n");
      }
    else if (m_output_format != SCRIPT)
      {
        g_print ("\n");
        g_print ("Wave\n");
        if (wave->name != "")
          g_print ("  Label                  %s\n", wave->name.c_str());
        const char *blurb = bse_xinfos_get_value (wave->wave_xinfos, "blurb");
        if (blurb)
          g_print ("  Comment                %s\n", blurb);
        const char *authors = bse_xinfos_get_value (wave->wave_xinfos, "authors");
        if (authors)
          g_print ("  Authors                %s\n", authors);
        const char *license = bse_xinfos_get_value (wave->wave_xinfos, "license");
        if (license)
          g_print ("  License                %s\n", license);
        g_print ("  Channels         %7d\n", wave->n_channels);
        g_print ("\n");
      }
    /* get the wave into storage order */
    wave->sort();
    for (list<WaveChunk>::iterator it = wave->chunks.begin(); it != wave->chunks.end(); it++)
      if (m_all_chunks || wave->match (*it, m_freq_list))
        {
          WaveChunk *chunk = &*it;
          GslDataHandle *dhandle = chunk->dhandle;
          if (m_output_format == SCRIPT)
            {
              for (vector<string>::const_iterator fi = m_fields.begin(); fi != m_fields.end(); fi++)
                {
                  if (fi != m_fields.begin()) // not first field
                    g_print (" ");
                  if (*fi == "channels")
                    g_print ("%d", wave->n_channels);
                  else if (*fi == "length")
                    g_print ("%lld", dhandle_n_frames (dhandle));
                  else if (*fi == "osc-freq")
                    g_print ("%.3f", gsl_data_handle_osc_freq (dhandle));
                  else if (*fi == "mix-freq")
                    g_print ("%.3f", gsl_data_handle_mix_freq (dhandle));
                  else if (*fi == "chunk-key")
                    g_print ("%s", WaveChunkKey (gsl_data_handle_osc_freq (dhandle)).as_string().c_str());
                  else if (*fi == "volume")
                    g_print ("%.3f", gsl_data_handle_volume (dhandle));
                  else if (*fi == "format")
                    g_print ("%s", dhandle_storage_format (dhandle).c_str());
                  else if (*fi == "label")
                    script_output_xstr (dhandle->setup.xinfos, "label");
                  else if (*fi == "blurb")
                    script_output_xstr (dhandle->setup.xinfos, "blurb");
                  else if (*fi == "loop-type")
                    script_output_xstr (dhandle->setup.xinfos, "loop-type");
                  else if (*fi == "loop-start")
                    script_output_xnum (dhandle->setup.xinfos, "loop-start");
                  else if (*fi == "loop-end")
                    script_output_xnum (dhandle->setup.xinfos, "loop-end");
                  else if (*fi == "loop-count")
                    script_output_xnum (dhandle->setup.xinfos, "loop-count");
                  else if (*fi == "midi-note")
                    script_output_xnum (dhandle->setup.xinfos, "midi-note");
                  else if (*fi == "+avg-energy-raw")
                    g_print ("%.3f", feature_avg_energy (dhandle, 1.0));
                  else if (*fi == "+avg-energy")
                    g_print ("%.3f", feature_avg_energy (dhandle, gsl_data_handle_volume (dhandle)));
                  else
                    {
                      sfi_error ("info command: invalid field for --script format: %s", fi->c_str());
                      exit (1);
                    }
                }
              g_print ("\n");
            }
          else  // MEDIUM or FULL output
            {
              g_print ("Chunk '%s'\n", WaveChunkKey (gsl_data_handle_osc_freq (dhandle)).as_string().c_str());
              const char *label = bse_xinfos_get_value (dhandle->setup.xinfos, "label");
              if (label)
                g_print ("  Label                  %s\n", label);
              const char *blurb = bse_xinfos_get_value (dhandle->setup.xinfos, "blurb");
              if (blurb)
                g_print ("  Comment                %s\n", blurb);
              g_print ("  Osc Freq   %13.2f Hz\n", gsl_data_handle_osc_freq (dhandle));
              g_print ("  Mix Freq   %13.2f Hz\n", gsl_data_handle_mix_freq (dhandle));
              if (bse_xinfos_get_value (dhandle->setup.xinfos, "midi-note"))
                {
                  int note = bse_xinfos_get_num (dhandle->setup.xinfos, "midi-note");
                  char *note_str = bse_note_to_string (note);
                  g_print ("  MIDI Note        %7d     (%s)\n", note, note_str);
                  g_free (note_str);
                }
              if (m_output_format == FULL)
                {
                  g_print ("  Sample Frames %10lld     (%.2f s)\n",
                           dhandle_n_frames (dhandle),
                           dhandle_n_frames (dhandle) / gsl_data_handle_mix_freq (dhandle));
                  if (bse_xinfos_get_value (dhandle->setup.xinfos, "volume"))
                    {
                      const double volume = gsl_data_handle_volume (dhandle);
                      const double volume_db = bse_db_from_factor (volume, -200);
                      g_print ("  Volume           %7.2f%%    (%.2f dB)\n", gsl_data_handle_volume (dhandle) * 100, volume_db);
                    }
                  g_print ("  Stored as       %s data\n", dhandle_storage_format (dhandle).c_str());
                  g_print ("  Avg Energy %+13.2f dB", feature_avg_energy (dhandle, gsl_data_handle_volume (dhandle)));
                  if (bse_xinfos_get_value (dhandle->setup.xinfos, "volume"))
                    g_print ("  (%.2f dB before volume adjustment)",
                             feature_avg_energy (dhandle, 1.0));
                  g_print ("\n");
                }
              const char *loop_type = bse_xinfos_get_value (dhandle->setup.xinfos, "loop-type");
              if (loop_type && m_output_format == FULL)
                {
                  g_print ("  Loop type %14s", loop_type);
                  if (strcmp (loop_type, "none") != 0)
                    {
                      SfiNum loop_start = bse_xinfos_get_num (dhandle->setup.xinfos, "loop-start");
                      SfiNum loop_end = bse_xinfos_get_num (dhandle->setup.xinfos, "loop-end");
                      g_print ("     (start: %lld, end: %lld, ", loop_start, loop_end);
                      SfiNum loop_count = bse_xinfos_get_num (dhandle->setup.xinfos, "loop-count");
                      if (!loop_count)
                        g_print ("forever)\n");
                      else
                        g_print ("count: %lld)\n", loop_count);
                    }
                }
            }
        }
    return false; /* bsewave didn't change */
  }
} cmd_info ("info");
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
    g_print ("{-m=midi-note|-f=osc-freq|--chunk-key=key|--all-chunks} [options]\n");
    if (bshort)
      return;
    g_print ("    Clip head and or tail of a wave chunk and produce fade-in ramps at the\n");
    g_print ("    beginning. Wave chunks which are clipped to an essential 0-length will\n");
    g_print ("    automatically be deleted.\n");
    g_print ("    Options:\n");
    g_print ("    -f <osc-freq>       oscillator frequency to select a wave chunk\n");
    g_print ("    -m <midi-note>      alternative way to specify oscillator frequency\n");
    g_print ("    --chunk-key <key>   select wave chunk using chunk key from list-chunks\n");
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
        const gchar *str = NULL;
	if (parse_chunk_selection (argv, i, argc, all_chunks, freq_list))
          seen_selection = true;
        else if (parse_str_option (argv, i, "-s", &str, argc))
          threshold = g_ascii_strtod (str, NULL);
        else if (parse_str_option (argv, i, "-h", &str, argc))
          head_samples = g_ascii_strtoull (str, NULL, 10);
        else if (parse_str_option (argv, i, "-t", &str, argc))
          tail_samples = g_ascii_strtoull (str, NULL, 10);
        else if (parse_str_option (argv, i, "-f", &str, argc))
          fade_samples = g_ascii_strtoull (str, NULL, 10);
        else if (parse_str_option (argv, i, "-p", &str, argc))
          pad_samples = g_ascii_strtoull (str, NULL, 10);
        else if (parse_str_option (argv, i, "-r", &str, argc))
          tail_silence = g_ascii_strtoull (str, NULL, 10);
      }
    return !seen_selection ? 1 : 0; /* # args missing */
  }
  bool
  exec (Wave *wave)
  {
    sort (freq_list.begin(), freq_list.end());
    verify_chunk_selection (freq_list, wave);
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
          if (error && !skip_errors)
            exit (1);
        }
    /* really delete chunks */
    while (deleted.size())
      {
        wave->remove (deleted.back());
        deleted.pop_back();
      }
    return true;
  }
} cmd_clip ("clip");
class NormalizeCmd : public Command {
  bool all_chunks;
  vector<gfloat> freq_list;
public:
  NormalizeCmd (const char *command_name) :
    Command (command_name),
    all_chunks (false)
  {}
  void
  blurb (bool bshort)
  {
    g_print ("{-m=midi-note|-f=osc-freq|--chunk-key=key|--all-chunks} [options]\n");
    if (bshort)
      return;
    g_print ("    Normalize wave chunk. This is used to extend (or compress) the signal\n");
    g_print ("    range to optimally fit the available unclipped dynamic range.\n");
    g_print ("    Options:\n");
    g_print ("    -f <osc-freq>       oscillator frequency to select a wave chunk\n");
    g_print ("    -m <midi-note>      alternative way to specify oscillator frequency\n");
    g_print ("    --chunk-key <key>   select wave chunk using chunk key from list-chunks\n");
    g_print ("    --all-chunks        try to normalize all chunks\n");
    /*       "**********1*********2*********3*********4*********5*********6*********7*********" */
  }
  guint
  parse_args (guint  argc,
              char **argv)
  {
    bool seen_selection = false;
    for (guint i = 1; i < argc; i++)
      {
	if (parse_chunk_selection (argv, i, argc, all_chunks, freq_list))
	  seen_selection = true;
      }
    return !seen_selection ? 1 : 0; /* # args missing */
  }
  bool
  exec (Wave *wave)
  {
    sort (freq_list.begin(), freq_list.end());
    verify_chunk_selection (freq_list, wave);
    /* normalization */
    for (list<WaveChunk>::iterator it = wave->chunks.begin(); it != wave->chunks.end(); it++)
      if (all_chunks || wave->match (*it, freq_list))
        {
          WaveChunk *chunk = &*it;
          sfi_info ("NORMALIZE: chunk %f", gsl_data_handle_osc_freq (chunk->dhandle));
          double absmax = gsl_data_find_min_max (chunk->dhandle, NULL, NULL);
          gchar **xinfos = bse_xinfos_dup_consolidated (chunk->dhandle->setup.xinfos, FALSE);
          BseErrorType error = BSE_ERROR_NONE;
          if (absmax > 4.6566e-10) /* 32bit threshold */
            {
              GslDataHandle *shandle = gsl_data_handle_new_scale (chunk->dhandle, 1. / absmax);
              error = chunk->change_dhandle (shandle, gsl_data_handle_osc_freq (chunk->dhandle), xinfos);
              if (error)
                sfi_error ("level normalizeping failed: %s", bse_error_blurb (error));
              gsl_data_handle_unref (shandle);
            }
          g_strfreev (xinfos);
          if (error && !skip_errors)
            exit (1);
        }
    return true;
  }
} cmd_normalize ("normalize");
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
    g_print ("    --chunk-key <key>   select wave chunk using chunk key from list-chunks\n");
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
	if (parse_chunk_selection (argv, i, argc, all_chunks, freq_list))
	  seen_selection = true;
      }
    return !seen_selection ? 1 : 0; /* # args missing */
  }
  bool
  exec (Wave *wave)
  {
    sort (freq_list.begin(), freq_list.end());
    verify_chunk_selection (freq_list, wave);
    vector<list<WaveChunk>::iterator> deleted;
    /* level clipping */
    for (list<WaveChunk>::reverse_iterator it = wave->chunks.rbegin(); it != wave->chunks.rend(); it++)
      if (all_chunks || wave->match (*it, freq_list))
        {
          WaveChunk *chunk = &*it;
	  GslDataHandle *dhandle = chunk->dhandle;
          sfi_info ("LOOP: chunk %f", gsl_data_handle_osc_freq (dhandle));
	  gdouble mix_freq = gsl_data_handle_mix_freq (dhandle);
	  GslDataLoopConfig lconfig;
	  lconfig.block_start = (GslLong) mix_freq;  /* skip first second */
	  lconfig.block_length = -1; 	             /* to end */
	  lconfig.analysis_points = 7;
	  lconfig.repetitions = 2;
	  lconfig.min_loop = (GslLong) MAX (mix_freq / 10, /* at least 100ms */
			                    8820 /* FIXME: hardcoded values in gsl_data_loop*() -> 200ms */);
	  gboolean found_loop = gsl_data_find_loop5 (dhandle, &lconfig, NULL, gsl_progress_printerr);
	  const char *loop_algorithm =       "loop5";
	  if (found_loop)
	    {
	      /* FIXME: assumes n_channels == 1 */
              gchar **xinfos = bse_xinfos_dup_consolidated (dhandle->setup.xinfos, FALSE);
	      xinfos = bse_xinfos_add_num (xinfos, "loop-count", 1000000);
	      xinfos = bse_xinfos_add_num (xinfos, "loop-start", lconfig.loop_start);
	      xinfos = bse_xinfos_add_num (xinfos, "loop-end", lconfig.loop_start + lconfig.loop_length);
	      xinfos = bse_xinfos_add_value (xinfos, "loop-type", gsl_wave_loop_type_to_string (GSL_WAVE_LOOP_JUMP));
	      xinfos = bse_xinfos_add_float (xinfos, "loop-score", lconfig.score);
	      if (lconfig.n_details > 0)
		xinfos = bse_xinfos_add_float (xinfos, "loop-score-detail1", lconfig.detail_scores[0]);
	      if (lconfig.n_details > 1)
		xinfos = bse_xinfos_add_float (xinfos, "loop-score-detail2", lconfig.detail_scores[1]);
	      xinfos = bse_xinfos_add_value (xinfos, "loop-algorithm", loop_algorithm);
	      gsl_data_handle_ref (dhandle);
	      BseErrorType error = chunk->change_dhandle (dhandle, gsl_data_handle_osc_freq (dhandle), xinfos);
	      if (error)
		sfi_error ("looping failed: %s", bse_error_blurb (error));
	      g_strfreev (xinfos);
	    }
        }
    return true;
  }
} cmd_loop ("loop");
#if 0
class ThinOutCmd : public Command {
  GslLong max_total_size;
  gdouble max_chunk_error;
  uint64 gal_iterations; /* genetic algorithm iterations (should we use "time" as API)? */
public:
  ThinOutCmd (const char *command_name) :
    Command (command_name),
    max_total_size (-1),
    max_chunk_error (-1),
    gal_iterations (5000)
  {
  }
  void
  blurb (bool bshort)
  {
    g_print ("{-s=max_total_size|-e=max_chunk_error|-g=gal_iterations} [options]\n");
    if (bshort)
      return;
    g_print (
"    Thin out bsewave file by omitting some chunks. It currently only is useful\n"
"    if the input wave contains one chunk per midi note, and these notes were looped,\n"
"    so that xinfos[\"loop-score\"] of each chunk contains a loop error that can\n"
"    be assumed to be an indicator of how good the looped file sounds (bigger errors\n"
"    mean that the file is more likely to click).\n"
"    For minimizing the storage such files will later take, and minimizing the risk\n"
"    that some of the notes will contain clicks, the thinout command will select a\n"
"    subset of the chunks (that is, it will throw some of the chunks away).\n"
"    Typically, there are a lot of possible subsets which could be selected, so the\n"
"    algorithm to search for an optimum implemented here is a genetic algorithm which\n"
"    can cope with the size of the search space.\n"
"    An optimum will be determined from a number of rules that determine what is to be\n"
"    considered \"optimal\":\n"
"     (a) omitting a chunk will mean that the next most chunk will be played, and\n"
"         that the chunk error of this chunk is heard: if you have sampled chunks for\n"
"         C with an error of 3.1 and C# with an error of 1.1, then omitting the C\n"
"         chunk from the output set will improve the sound quality, because the total\n"
"         error drops from 1.1+3.1 to 1.1+1.1\n"
"     (b) it is _bad_ to omit so many chunks that a chunk will have to be replaced\n"
"         by a chunk that is more than for midi notes away: while it is safe to omit\n"
"         so many chunks that instead of playing C, a resampled version of E will be\n"
"         played, it is not safe to omit so many chunks a resampled version of F\n"
"         will be played\n"
"     (c) if the -s option is present: it is _bad_ if the number of chunks exceeds\n"
"         the maximum size the user requested\n"
"     (d) if the -e option is present: is is _bad_ to keep a chunk that has a chunk\n"
"         error larger than the maximum error the user requested\n"
"    Here, it is _bad_ means that the optimizer will avoid that it happens by\n"
"    placing a big penalty on the score of such an output set. However, as it is\n"
"    possible that the constraints (a)-(d) contradict each other, or that a\n"
"    suitable solution can not be found in the available number of iterations\n"
"    (which can be increased using the -g option), there is no guarantee that\n"
"    the resulting chunk set fulfills all conditions; the optimizer will only try\n"
"    to produce the best possible result.\n");
    g_print ("    Options:\n");
    g_print ("    -s <max-total-size>     restrict the resulting bsewave file to a max size\n");
    g_print ("    -e <max-chunk-error>    ensure that no chunk exceeds this error margin\n");
    g_print ("    -g <gal-iterations>     iterations for genetic algorithm optimizer [5000]\n");
    /*       "**********1*********2*********3*********4*********5*********6*********7*********" */
  }
  guint
  parse_args (guint  argc,
              char **argv)
  {
    for (guint i = 1; i < argc; i++)
      {
        const gchar *str = NULL;
        if (parse_str_option (argv, i, "-s", &str, argc))
          {
            max_total_size = g_ascii_strtoull (str, NULL, 10);
          }
	else if (parse_str_option (argv, i, "-g", &str, argc))
          {
	    gal_iterations = g_ascii_strtoull (str, NULL, 10);
          }
        else if (parse_str_option (argv, i, "-e", &str, argc))
          {
            max_chunk_error = g_ascii_strtod (str, NULL);
          }
      }
    return 0; /* # args missing */
  }
  struct ChunkData
  {
    vector<GslLong> sizes;
    vector<gdouble> errors;
    vector<gdouble> freqs;
    vector<list<WaveChunk>::iterator> iterators;
    int
    n_chunks() const
    {
      return sizes.size();
    }
  };
  struct ChunkSet
  {
    /*
    ChunkSet (const ChunkSet& s)
      : chunks (s.chunks),
	error (s.error)
    {
    }
    ChunkSet ()
    {
      error = -1;
    }
    const ChunkSet& operator =(const ChunkSet& s)
    {
      chunks = s.chunks;
      error = s.error;
      return *this;
    }
    */
    vector<guint8> chunks; /* which chunks should be used */
    double error;	   /* total_error (chunk_data, chunks) <- to be minimized */
    /* syntactic sugar */
    guint8& operator[] (size_t n)
    {
      return chunks[n];
    }
    const guint8& operator[] (size_t n) const
    {
      return chunks[n];
    }
  };
  bool
  exec (Wave *wave)
  {
    ChunkData chunk_data;
    /* level clipping */
    for (list<WaveChunk>::iterator it = wave->chunks.begin(); it != wave->chunks.end(); it++)
      {
	WaveChunk *chunk = &*it;
	GslDataHandle *dhandle = chunk->dhandle;
	gfloat error = bse_xinfos_get_float (dhandle->setup.xinfos, "loop-score");
	GslLong size = 1;
	gdouble freq = gsl_data_handle_osc_freq (chunk->dhandle);
	sfi_info ("THINOUT: chunk %f: error %f, size %lld", freq, error, size);
	chunk_data.sizes.push_back (size);
	chunk_data.errors.push_back (error);
	chunk_data.freqs.push_back (freq);
	chunk_data.iterators.push_back (it);
      }
    g_assert (chunk_data.sizes.size() > 0);
    g_assert (chunk_data.sizes.size() == chunk_data.errors.size());
    ChunkSet chunk_set;
    init_empty_set (chunk_data, chunk_set);
    if (gal_iterations)
      gal_optimize (chunk_data, chunk_set);
    else
      optimize (chunk_data, chunk_set);
    /* really delete chunks */
    for (int i = 0; i < chunk_data.n_chunks(); i++)
      {
        if (!chunk_set[i])
	  wave->remove (chunk_data.iterators[i]);
      }
    return true;
  }
  void
  init_empty_set (const ChunkData& chunk_data, ChunkSet& chunk_set)
  {
    chunk_set.chunks.resize (chunk_data.n_chunks());
    chunk_set.error = total_error (chunk_data, chunk_set);
  }
  void gal_create_child (const ChunkSet& father, const ChunkSet& mother, ChunkSet& child)
  {
    if (rand() % 2)
      {
	child.chunks = father.chunks;
      }
    else
      {
	for (size_t i = 0; i < child.chunks.size(); i++)
	  {
	    if (rand() % 2)
	      child.chunks[i] = father.chunks[i];
	    else
	      child.chunks[i] = mother.chunks[i];
	  }
      }
    int mutations = (rand() % 7) + 1;
    for (int i = 0; i < mutations; i++)
      {
	int k = rand() % child.chunks.size();
	child.chunks[k] = !child.chunks[k];
      }
  }
  struct GalErrorSort
  {
    bool operator () (const ChunkSet& set1, const ChunkSet& set2) const
    {
      return set1.error < set2.error;
    }
  };
  void gal_optimize (const ChunkData& chunk_data, ChunkSet& chunk_set)
  {
    const int POPULATION_SIZE = 64;
    vector<ChunkSet> population;
    for (int i = 0; i < POPULATION_SIZE; i++)
      population.push_back (chunk_set);
    for (uint64 giteration = 0; giteration < gal_iterations; giteration++)
      {
	/*
	 * the upper half of the individuums are "loosers", and are replaced by
	 * new candidates
	 */
	for (int i = POPULATION_SIZE / 2; i < POPULATION_SIZE; i++)
	  {
	    int father = rand() % (POPULATION_SIZE / 2);
	    int mother = rand() % (POPULATION_SIZE / 2);
	    gal_create_child (population[father], population[mother], population[i]);
	    population[i].error = total_error (chunk_data, population[i]);
	  }
	sort (population.begin(), population.end(), GalErrorSort());
	/*
	 * if one individuum is present more than once in the population
	 * replace it with "less fit" individuums
	 */
	int k = 0;
	int p = POPULATION_SIZE/2;
	for (int i = 1; i < POPULATION_SIZE/2; i++)
	  {
	    if (population[i].chunks == population[i-1].chunks)
	      {
		population[i] = population[p++];
		k++;
	      }
	  }
#if 0
	printf ("k = %d, p = %d\n", k, p);
	printf ("giteration %lld error %.5f (middle: %.5f, worst: %.5f)\n -> %s\n", giteration,
		population[0].error, population[POPULATION_SIZE/2-1].error, population.back().error,
		set_to_string (chunk_data, population[0]).c_str());
#endif
      }
    /*
     * better individuums are at the beginning of the population after sort
     */
    chunk_set = population[0];
    sfi_info ("THINOUT: error %.5f %s", chunk_set.error, set_to_string (chunk_data, chunk_set).c_str());
  }
  double
  total_error (const ChunkData& chunk_data, const ChunkSet& chunk_set)
  {
    double error = 0;
    /*
     * approximation error (created by replacing S original chunks with R looped and
     * resampled chunks, where S is often a lot smaller than R)
     */
    for (int source_chunk = 0; source_chunk < chunk_data.n_chunks(); source_chunk++)
      {
	double best_fdiff = 44100;
	double best_fdiff_error = 1000; /* should be a lot more than conventional loop scores */
	/* FIXME: speed! */
	for (int replacement_chunk = 0; replacement_chunk < chunk_data.n_chunks(); replacement_chunk++)
	  {
	    if (chunk_set[replacement_chunk])
	      {
		double fdiff = fabs (chunk_data.freqs[source_chunk] - chunk_data.freqs[replacement_chunk]);
		if (fdiff < best_fdiff &&
		    abs (source_chunk - replacement_chunk) <= 4) /* <- make me configurable, and/or use frequencies */
		  {
		    best_fdiff = fdiff;
		    best_fdiff_error = chunk_data.errors[replacement_chunk];
		  }
	      }
	  }
	error += best_fdiff_error;
      }
    /*
     * user constraint: maximal total size
     */
    if (max_total_size > 0)
      {
	GslLong total_size = 0;
	for (int i = 0; i < chunk_data.n_chunks(); i++)
	  if (chunk_set[i])
	    total_size += chunk_data.sizes[i];
	if (total_size > max_total_size)
	  error += (total_size - max_total_size) * 1000.0;
      }
    /*
     * user constraint: maximum chunk error
     */
    if (max_chunk_error > 0)
      {
	for (int i = 0; i < chunk_data.n_chunks(); i++)
	  if (chunk_set[i] && chunk_data.errors[i] > max_chunk_error)
	    error += 1000.0;
      }
    return error;
  }
  string
  set_to_string (const ChunkData& chunk_data, const ChunkSet& chunk_set)
  {
    string result = "[ ";
    for (int i = 0; i < chunk_data.n_chunks(); i++)
      {
	if (chunk_set[i])
	  {
	    char *x = g_strdup_printf ("%4.2f ", chunk_data.freqs[i]);
	    result += x;
	    g_free (x);
	  }
      }
    result += "]";
    return result;
  }
  void
  optimize (const ChunkData& chunk_data, ChunkSet& chunk_set)
  {
    double best_error = total_error (chunk_data, chunk_set);
    int toggle = -1;
    for (int i = 0; i < chunk_data.n_chunks(); i++)
      {
	chunk_set[i] = !chunk_set[i];
	double error = total_error (chunk_data, chunk_set);
	chunk_set[i] = !chunk_set[i];
	if (error < best_error)
	  {
	    best_error = error;
	    toggle = i;
	  }
      }
    if (toggle >= 0)
      {
	chunk_set[toggle] = !chunk_set[toggle];
	printf ("toggle %d error %.5f -> %s\n", toggle, best_error,
	        set_to_string (chunk_data, chunk_set).c_str());
	optimize (chunk_data, chunk_set);
      }
  }
} cmd_thinout ("thinout");
#endif
class FirCommand : public Command {
protected:
  gdouble m_cutoff_freq;
  guint   m_order;
  virtual GslDataHandle* create_fir_handle (GslDataHandle* dhandle) = 0;
public:
  FirCommand (const char *command_name) :
    Command (command_name)
  {
    m_cutoff_freq = -1;
    m_order = 64;
  }
  void
  blurb (bool bshort)
  {
    g_print ("[options]\n");
    if (bshort)
      return;
    g_print ("    Apply %s filter to wave data\n", name.c_str());
    g_print ("    --cutoff-freq <f>   filter cutoff frequency in Hz\n");
    g_print ("    --order <o>         filter order [%u]\n", m_order);
    /*       "**********1*********2*********3*********4*********5*********6*********7*********" */
  }
  guint
  parse_args (guint  argc,
              char **argv)
  {
    for (guint i = 1; i < argc; i++)
      {
	const gchar *str = NULL;
	if (parse_str_option (argv, i, "--cutoff-freq", &str, argc))
	  {
	    m_cutoff_freq = g_ascii_strtod (str, NULL);
	  }
	else if (parse_str_option (argv, i, "--order", &str, argc))
	  {
	    m_order = g_ascii_strtoull (str, NULL, 10);
	  }
      }
    return (m_cutoff_freq <= 0); // missing args
  }
  BseErrorType
  print_effective_stopband_start (GslDataHandle *fir_handle)
  {
    BseErrorType error = gsl_data_handle_open (fir_handle);
    if (error)
      return error;
    Birnet::int64 freq_inc = 5; // FIXME
    while (freq_inc * 1000 < gsl_data_handle_mix_freq (fir_handle))
      freq_inc *= 2;
    double	  best_diff_db = 100;
    Birnet::int64 best_freq = 0;
    for (Birnet::int64 freq = 0; freq < gsl_data_handle_mix_freq (fir_handle) / 2.0; freq += freq_inc)
      {
	double diff_db = fabs (bse_data_handle_fir_response_db (fir_handle, freq) + 48);
	if (diff_db < best_diff_db)
	  {
	    best_diff_db = diff_db;
	    best_freq = freq;
	  }
      }
    sfi_info ("%s: => %.2f dB at %lld Hz", string_toupper (name).c_str(),
	bse_data_handle_fir_response_db (fir_handle, best_freq), best_freq);
    gsl_data_handle_close (fir_handle);
    return BSE_ERROR_NONE;
  }
  bool
  exec (Wave *wave)
  {
    /* get the wave into storage order */
    wave->sort();
    for (list<WaveChunk>::iterator it = wave->chunks.begin(); it != wave->chunks.end(); it++)
      {
        WaveChunk *chunk = &*it;
        GslDataHandle *dhandle = chunk->dhandle;
	sfi_info ("%s: chunk %f: cutoff_freq=%f order=%d", string_toupper (name).c_str(),
	          gsl_data_handle_osc_freq (chunk->dhandle), m_cutoff_freq, m_order);
	if (m_cutoff_freq >= gsl_data_handle_mix_freq (dhandle) / 2.0)
	  {
	    sfi_error ("chunk % 7.2f/%.0f: IGNORED - can't filter this chunk, cutoff frequency (%f) too high\n",
	    gsl_data_handle_osc_freq (chunk->dhandle), gsl_data_handle_mix_freq (dhandle), m_cutoff_freq);
	  }
	else
	  {
	    GslDataHandle *fir_handle = create_fir_handle (dhandle);
	    BseErrorType error = print_effective_stopband_start (fir_handle);
	    if (!error)
	      error = chunk->change_dhandle (fir_handle, 0, 0);
	    if (error)
	      {
		sfi_error ("chunk % 7.2f/%.0f: %s",
			   gsl_data_handle_osc_freq (chunk->dhandle), gsl_data_handle_mix_freq (chunk->dhandle),
			   bse_error_blurb (error));
		exit (1);
	      }
	  }
      }
    return true;
  }
};
class Highpass : public FirCommand {
protected:
  GslDataHandle*
  create_fir_handle (GslDataHandle* dhandle)
  {
    return bse_data_handle_new_fir_highpass (dhandle, m_cutoff_freq, m_order);
  }
public:
  Highpass (const char *command_name) :
    FirCommand (command_name)
  {
  }
} cmd_highpass ("highpass");
class Lowpass : public FirCommand {
protected:
  GslDataHandle*
  create_fir_handle (GslDataHandle* dhandle)
  {
    return bse_data_handle_new_fir_lowpass (dhandle, m_cutoff_freq, m_order);
  }
public:
  Lowpass (const char *command_name) :
    FirCommand (command_name)
  {
  }
} cmd_lowpass ("lowpass");
class Upsample2 : public Command {
private:
  vector<gfloat> m_freq_list;
  bool           m_all_chunks;
  int            m_precision_bits;
public:
  Upsample2 (const char *command_name) :
    Command (command_name),
    m_all_chunks (false),
    m_precision_bits (24)
  {
  }
  void
  blurb (bool bshort)
  {
    g_print ("[options]\n");
    if (bshort)
      return;
    g_print ("    Resample wave data to twice the sampling frequency.\n");
    g_print ("    --precision <bits>      set resampler precision bits [%d]\n", m_precision_bits);
    g_print ("                            supported precisions: 1, 8, 12, 16, 20, 24\n");
    g_print ("                            1 is a special value for linear interpolation\n");
    g_print ("    -f <osc-freq>           oscillator frequency to select a wave chunk\n");
    g_print ("    -m <midi-note>          alternative way to specify oscillator frequency\n");
    g_print ("    --chunk-key <key>       select wave chunk using chunk key from list-chunks\n");
    g_print ("    --all-chunks            upsample all chunks\n");
    /*       "**********1*********2*********3*********4*********5*********6*********7*********" */
  }
  guint
  parse_args (guint  argc,
              char **argv)
  {
    bool seen_selection = false;
    for (guint i = 1; i < argc; i++)
      {
	const gchar *str = NULL;
	if (parse_chunk_selection (argv, i, argc, m_all_chunks, m_freq_list))
          seen_selection = true;
	else if (parse_str_option (argv, i, "--precision", &str, argc))
	  m_precision_bits = atoi (str);
      }
    if (!seen_selection) /* default to all chunks */
      m_all_chunks = true;
    return 0; // no missing args
  }
  bool
  exec (Wave *wave)
  {
    /* get the wave into storage order */
    wave->sort();
    for (list<WaveChunk>::iterator it = wave->chunks.begin(); it != wave->chunks.end(); it++)
      if (m_all_chunks || wave->match (*it, m_freq_list))
        {
          WaveChunk *chunk = &*it;
          GslDataHandle *dhandle = chunk->dhandle;
          sfi_info ("UPSAMPLE2: chunk %f: mix_freq=%f -> mix_freq=%f",
                    gsl_data_handle_osc_freq (chunk->dhandle),
                    gsl_data_handle_mix_freq (chunk->dhandle),
                    gsl_data_handle_mix_freq (chunk->dhandle) * 2);
          sfi_info ("  using resampler precision: %s\n",
                    bse_resampler2_precision_name (bse_resampler2_find_precision_for_bits (m_precision_bits)));
          BseErrorType error = chunk->change_dhandle (bse_data_handle_new_upsample2 (dhandle, m_precision_bits), 0, 0);
          if (error)
            {
              sfi_error ("chunk % 7.2f/%.0f: %s",
                         gsl_data_handle_osc_freq (chunk->dhandle), gsl_data_handle_mix_freq (chunk->dhandle),
                         bse_error_blurb (error));
              exit (1);
            }
        }
    return true;
  }
} cmd_upsample2 ("upsample2");
class Downsample2 : public Command {
private:
  vector<gfloat> m_freq_list;
  bool           m_all_chunks;
  int            m_precision_bits;
public:
  Downsample2 (const char *command_name) :
    Command (command_name),
    m_all_chunks (false),
    m_precision_bits (24)
  {
  }
  void
  blurb (bool bshort)
  {
    g_print ("[options]\n");
    if (bshort)
      return;
    g_print ("    Resample wave data to half the sampling frequency.\n");
    g_print ("    --precision <bits>      set resampler precision bits [%d]\n", m_precision_bits);
    g_print ("                            supported precisions: 1, 8, 12, 16, 20, 24\n");
    g_print ("                            1 is a special value for linear interpolation\n");
    g_print ("    -f <osc-freq>           oscillator frequency to select a wave chunk\n");
    g_print ("    -m <midi-note>          alternative way to specify oscillator frequency\n");
    g_print ("    --chunk-key <key>       select wave chunk using chunk key from list-chunks\n");
    g_print ("    --all-chunks            downsample all chunks\n");
     /*       "**********1*********2*********3*********4*********5*********6*********7*********" */
  }
  guint
  parse_args (guint  argc,
              char **argv)
  {
    bool seen_selection = false;
    for (guint i = 1; i < argc; i++)
      {
	const gchar *str = NULL;
	if (parse_chunk_selection (argv, i, argc, m_all_chunks, m_freq_list))
          seen_selection = true;
	else if (parse_str_option (argv, i, "--precision", &str, argc))
	  m_precision_bits = atoi (str);
      }
    if (!seen_selection) /* default to all chunks */
      m_all_chunks = true;
    return 0; // no missing args
  }
  bool
  exec (Wave *wave)
  {
    /* get the wave into storage order */
    wave->sort();
    for (list<WaveChunk>::iterator it = wave->chunks.begin(); it != wave->chunks.end(); it++)
      if (m_all_chunks || wave->match (*it, m_freq_list))
        {
          WaveChunk *chunk = &*it;
          GslDataHandle *dhandle = chunk->dhandle;
          sfi_info ("DOWNSAMPLE2: chunk %f: mix_freq=%f -> mix_freq=%f",
                    gsl_data_handle_osc_freq (chunk->dhandle),
                    gsl_data_handle_mix_freq (chunk->dhandle),
                    gsl_data_handle_mix_freq (chunk->dhandle) / 2);
          sfi_info ("  using resampler precision: %s\n",
                    bse_resampler2_precision_name (bse_resampler2_find_precision_for_bits (m_precision_bits)));
          BseErrorType error = chunk->change_dhandle (bse_data_handle_new_downsample2 (dhandle, 24), 0, 0);
          if (error)
            {
              sfi_error ("chunk % 7.2f/%.0f: %s",
                         gsl_data_handle_osc_freq (chunk->dhandle), gsl_data_handle_mix_freq (chunk->dhandle),
                         bse_error_blurb (error));
              exit (1);
            }
        }
    return true;
  }
} cmd_downsample2 ("downsample2");
class Export : public Command {
public:
  vector<gfloat> freq_list;
  bool           all_chunks;
  string         export_filename;
  Export (const char *command_name) :
    Command (command_name)
  {
    all_chunks = false;
  }
  void
  blurb (bool bshort)
  {
    g_print ("{-m=midi-note|-f=osc-freq|--chunk-key=key|--all-chunks|-x=filename} [options]\n");
    if (bshort)
      return;
    g_print ("    Export chunks from bsewave as WAV file.\n");
    g_print ("    Options:\n");
    g_print ("    -x <filename>       set export filename (supports %%N %%F and %%C, see below)\n");
    g_print ("    -f <osc-freq>       oscillator frequency to select a wave chunk\n");
    g_print ("    -m <midi-note>      alternative way to specify oscillator frequency\n");
    g_print ("    --chunk-key <key>   select wave chunk using chunk key from list-chunks\n");
    g_print ("    --all-chunks        try to export all chunks\n");
    g_print ("    The export filename can contain the following extra information:\n");
    g_print ("      %%F  -  the frequency of the chunk\n");
    g_print ("      %%N  -  the midi note of the chunk\n");
    g_print ("      %%C  -  cent detuning of the midi note\n");
    /*       "**********1*********2*********3*********4*********5*********6*********7*********" */
  }
  guint
  parse_args (guint  argc,
              char **argv)
  {
    bool seen_selection = false;
    bool seen_export_filename = false;
    for (guint i = 1; i < argc; i++)
      {
        const gchar *str = NULL;
	if (parse_chunk_selection (argv, i, argc, all_chunks, freq_list))
          seen_selection = true;
        else if (parse_str_option (argv, i, "-x", &str, argc))
	  {
	    export_filename = str;
	    seen_export_filename = true;
	  }
      }
    guint missing_args = 0;
    if (!seen_selection)
      missing_args++;
    if (!seen_export_filename)
      missing_args++;
    return missing_args; /* # args missing */
  }
  /*
   * for ch == 'X', substitute searches for occurrences of %X
   * in pattern and replaces them with the string subst
   */
  void
  substitute (string& pattern,
              char    ch,
	      const   string& subst)
  {
    string result;
    bool need_subst = false;
    for (size_t i = 0; i < pattern.size(); i++)
      {
	if (need_subst)
	  {
	    if (pattern[i] == ch)
	      result += subst;
	    else
	      {
		result += '%';
		result += pattern[i];
	      }
	    need_subst = false;
	  }
	else if (pattern[i] == '%')
	  need_subst = true;
	else
	  result += pattern[i];
      }
    if (need_subst)
      pattern += '%';
    pattern = result;
  }
  bool
  exec (Wave *wave)
  {
    map<string,bool> used_filenames;
    /* validate format */
    bool have_export_pattern = false;
    for (int i = 0; i < int(export_filename.size()) - 1; i++)
      if (export_filename[i] == '%')
	{
	  if (export_filename[i+1] == 'N'
	   || export_filename[i+1] == 'F'
	   || export_filename[i+1] == 'C')
	    have_export_pattern = true;
	  else
	    {
	      sfi_error ("export filename contains bad formatting expression %%%c", export_filename[i+1]);
	      exit (1);
	    }
      }
    /* validate that we have a pattern if more than one chunk gets exported */
    if ((all_chunks && wave->chunks.size() > 1) || (freq_list.size() > 1))
      {
	if (!have_export_pattern)
	  {
	    sfi_error ("when exporting more than one chunk, the output filename needs to contain %%N %%C or %%F");
	    exit (1);
	  }
      }
    /* validate freq list */
    sort (freq_list.begin(), freq_list.end());
    verify_chunk_selection (freq_list, wave);
    /* get the wave into storage order */
    wave->sort();
    for (list<WaveChunk>::iterator it = wave->chunks.begin(); it != wave->chunks.end(); it++)
      if (all_chunks || wave->match (*it, freq_list))
        {
          WaveChunk *chunk = &*it;
	  GslDataHandle *dhandle = chunk->dhandle;
	  gchar *name_addon = NULL;
	  string filename = export_filename;
	  int note = bse_xinfos_get_num (dhandle->setup.xinfos, "midi-note");
	  int cent = 0;
	  if (!note)
	    {
	      note = bse_note_from_freq_bounded (BSE_MUSICAL_TUNING_12_TET, gsl_data_handle_osc_freq (dhandle));
	      cent = bse_note_fine_tune_from_note_freq (BSE_MUSICAL_TUNING_12_TET, note, gsl_data_handle_osc_freq (dhandle));
	    }
	  name_addon = g_strdup_printf ("%d", note);
	  substitute (filename, 'N', name_addon);
	  g_free (name_addon);
	  name_addon = g_strdup_printf ("%.2f", gsl_data_handle_osc_freq (dhandle));
	  substitute (filename, 'F', name_addon);
	  g_free (name_addon);
	  if (cent >= 0)
	    name_addon = g_strdup_printf ("u%03d", cent); /* up */
	  else
	    name_addon = g_strdup_printf ("d%03d", cent); /* down */
	  substitute (filename, 'C', name_addon);
	  g_free (name_addon);
          sfi_info ("EXPORTING: chunk %f to %s", gsl_data_handle_osc_freq (dhandle), filename.c_str());
	  if (used_filenames[filename])
	    {
	      sfi_warning ("another chunk was already exported to %s. skipping this chunk export.", filename.c_str());
	      continue;
	    }
	  else
	    {
	      used_filenames[filename] = true;
	    }
	  int fd = open (filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
	  if (fd < 0)
	    {
	      BseErrorType error = bse_error_from_errno (errno, BSE_ERROR_FILE_OPEN_FAILED);
	      sfi_error ("export to file %s failed: %s", filename.c_str(), bse_error_blurb (error));
	    }
	  int xerrno = gsl_data_handle_dump_wav (dhandle, fd, 16, dhandle->setup.n_channels, (guint) dhandle->setup.mix_freq);
	  if (xerrno)
	    {
	      BseErrorType error = bse_error_from_errno (xerrno, BSE_ERROR_FILE_WRITE_FAILED);
	      sfi_error ("export to file %s failed: %s", filename.c_str(), bse_error_blurb (error));
	    }
	  close (fd);
        }
    return true;
  }
} cmd_export ("export");
class ListChunks : public Command {
public:
  ListChunks (const char *command_name) :
    Command (command_name)
  {
  }
  void
  blurb (bool bshort)
  {
    g_print ("[options]\n");
    if (bshort)
      return;
    g_print ("    Prints a list of chunk keys of the chunks contained in the bsewave file.\n");
    g_print ("    A chunk key for a given chunk identifies the chunk uniquely and stays valid\n");
    g_print ("    if other chunks are inserted and deleted.\n");
    g_print ("    This bash script shows the length of all chunks (like info --all-chunks):\n");
    g_print ("      for key in `bsewavetool list-chunks foo.bsewave`\n");
    g_print ("      do\n");
    g_print ("        bsewavetool info foo.bsewave --chunk-key $key --script length\n");
    g_print ("      done\n");
    /*       "**********1*********2*********3*********4*********5*********6*********7*********" */
  }
  guint
  parse_args (guint  argc,
              char **argv)
  {
    return 0; // no missing args
  }
  bool
  exec (Wave *wave)
  {
    /* get the wave into storage order */
    wave->sort();
    for (list<WaveChunk>::iterator it = wave->chunks.begin(); it != wave->chunks.end(); it++)
      {
        WaveChunk     *chunk = &*it;
        WaveChunkKey   chunk_key (gsl_data_handle_osc_freq (chunk->dhandle));
        g_print ("%s\n", chunk_key.as_string().c_str());
      }
    return true;
  }
} cmd_list_chunks ("list-chunks");
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
/* vim:set ts=8 sts=2 sw=2: */
