// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsescminterp.hh"
#include "../configure.h"
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <guile/gh.h>
#include <bse/bse.hh>
#include <bse/bsemain.hh>       // bse_bindtextdomain
#include <sfi/sfistore.hh> /* no bin-compat */
#include <sys/time.h>
#include <libintl.h>
#include <sys/resource.h>
#define	PRG_NAME	"bsescm"
#define BSE_EXIT_STATUS 102
/* --- prototypes --- */
static void	gh_main			(gint	 argc,
					 gchar	*argv[]);
static void	shell_parse_args	(int *argc_p, char **argv);
static void     shell_print_usage       (void);
/* --- variables --- */
static gint            bse_scm_pipe[2] = { -1, -1 };
static gchar          *bse_scm_eval_expr = NULL;
static gboolean        bse_scm_enable_register = FALSE;
static gboolean        bse_scm_auto_load = TRUE;
static gboolean        bse_scm_auto_play = TRUE;
static SfiComPort     *bse_scm_port = NULL;
static SfiGlueContext *bse_scm_context = NULL;
static std::string     boot_script_path = Bse::installpath (Bse::INSTALLPATH_DATADIR_SCRIPTS);

/* --- functions --- */
static void
port_closed (SfiComPort *port,
	     gpointer    close_data)
{
  /* we don't do anything fancy here */
  if (port)
    exit (BSE_EXIT_STATUS);
}
static void
dummy_dispatch (void *foo)
{
  // nothing to do
}
int
main (int   argc,
      char *argv[])
{
  const gchar *env_str;
  GSource *source;
  sfi_init (&argc, argv);
  bse_bindtextdomain();
  setlocale (LC_ALL, "");
  env_str = g_getenv ("BSESCM_SLEEP4GDB");
  if (env_str && atoi (env_str) >= 3)
    {
      g_message ("going into sleep mode due to debugging request (pid=%u)", getpid ());
      g_usleep (2147483647);
    }
  shell_parse_args (&argc, argv);
  if (env_str && (atoi (env_str) >= 2 ||
                  (atoi (env_str) >= 1 && !bse_scm_enable_register)))
    {
      g_message ("going into sleep mode due to debugging request (pid=%u)", getpid ());
      g_usleep (2147483647);
    }
  if (bse_scm_pipe[0] >= 0 && bse_scm_pipe[1] >= 0)
    {
      bse_scm_port = sfi_com_port_from_pipe (PRG_NAME, bse_scm_pipe[0], bse_scm_pipe[1]);
      sfi_com_port_set_close_func (bse_scm_port, port_closed, NULL);
      if (!bse_scm_port->connected)
	{
	  printerr ("%s: failed to connect to pipe (%d, %d)\n", PRG_NAME, bse_scm_pipe[0], bse_scm_pipe[1]);
	  exit (BSE_EXIT_STATUS);
	}
      bse_scm_context = sfi_glue_encoder_context (bse_scm_port);
    }
  if (!bse_scm_context)
    {
      // start our own core thread
      Bse::init_async (&argc, argv, "BSESCM");
      // allow g_main_context_wakeup to interrupt sleeps in bse_scm_context_iteration
      bse_scm_context = Bse::init_glue_context (PRG_NAME, []() { g_main_context_wakeup (g_main_context_default()); });
    }
  /* now that the BSE thread runs, drop scheduling priorities if we have any */
  setpriority (PRIO_PROCESS, getpid(), 0);
  source = g_source_simple (G_PRIORITY_DEFAULT, GSourcePending (sfi_glue_context_pending), dummy_dispatch, NULL, NULL, NULL);
  g_source_attach (source, NULL);
  g_source_unref (source);
  gh_enter (argc, argv, gh_main);
  return 0;
}
static void
gh_main (int   argc,
	 char *argv[])
{
  /* initial interpreter setup */
  if (bse_scm_enable_register)
    bse_scm_enable_script_register (TRUE);
  else
    bse_scm_enable_server (TRUE);
  sfi_glue_context_push (bse_scm_context);

  /* initialize interpreter */
  bse_scm_interp_init ();

  /* exec Bse Scheme bootup code */
  const gchar *boot_script = g_intern_format ("%s/%s", boot_script_path.c_str(), "bse-scm-glue.boot");
  gh_load (boot_script);

  /* eval, auto-play or interactive */
  if (bse_scm_eval_expr)
    gh_eval_str (bse_scm_eval_expr);
  else
    {
      gboolean call_auto_play = FALSE;
      /* detect bse file */
      if (argc >= 2 && bse_scm_auto_play)
        {
          SfiRStore *rstore = sfi_rstore_new_open (argv[1]);
          if (rstore)
            {
              if (g_scanner_get_next_token (rstore->scanner) == '(' &&
                  g_scanner_get_next_token (rstore->scanner) == G_TOKEN_IDENTIFIER)
                {
                  if (strcmp ("bse-version", rstore->scanner->value.v_string) == 0 &&
                      g_scanner_get_next_token (rstore->scanner) == G_TOKEN_STRING &&
                      g_scanner_get_next_token (rstore->scanner) == ')')
                    call_auto_play = TRUE;
                }
              sfi_rstore_destroy (rstore);
            }
        }
      /* auto-play or interactive */
      if (call_auto_play)
        gh_eval_str ("(bse-scm-auto-play)");
      else 
        {
          if (bse_scm_auto_load)
            gh_eval_str ("(bse-server-register-blocking bse-server-register-core-plugins #f)"
                         "(bse-server-register-blocking bse-server-register-scripts #f)"
                         "(bse-server-register-blocking bse-server-register-ladspa-plugins #f)");
          gh_repl (argc, argv);
        }
    }

  /* shutdown */
  sfi_glue_context_pop ();
  if (bse_scm_port)
    {
      sfi_com_port_set_close_func (bse_scm_port, NULL, NULL);
      sfi_com_port_close_remote (bse_scm_port, FALSE);
    }
  sfi_glue_context_destroy (bse_scm_context);
}

static void
shell_parse_args (int *argc_p, char **argv)
{
  uint argc = *argc_p;
  uint i, e;
  bool initialize_bse_and_exit = false;
  for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "--") == 0 ||
          strcmp (argv[i], "-s") == 0 ||
          strcmp (argv[i], "-c") == 0)
	break;
      else if (strcmp (argv[i], "--g-fatal-warnings") == 0)
	{
	  GLogLevelFlags fatal_mask;

	  fatal_mask = g_log_set_always_fatal (GLogLevelFlags (G_LOG_FATAL_MASK));
          fatal_mask = GLogLevelFlags (fatal_mask | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL);
	  g_log_set_always_fatal (fatal_mask);
	  argv[i] = NULL;
	}
      else if (strcmp (argv[i], "--bse-pipe") == 0)
	{
	  /* first two arguments are input and output pipes from BseServer */
	  bse_scm_pipe[0] = -1;
	  bse_scm_pipe[1] = -1;
	  if (i + 2 < argc)
	    {
	      bse_scm_pipe[0] = atoi (argv[i + 1]);
	      bse_scm_pipe[1] = atoi (argv[i + 2]);
	      argv[i++] = NULL;
	      argv[i++] = NULL;
	    }
	  argv[i] = NULL;
	  if (bse_scm_pipe[0] < 2 || bse_scm_pipe[1] < 2)
	    {
	      printerr ("%s: invalid arguments supplied for: --bse-pipe <inpipe> <outpipe>\n", PRG_NAME);
	      exit (BSE_EXIT_STATUS);
	    }
	}
      else if (strcmp (argv[i], "--bse-eval") == 0)
	{
	  bse_scm_eval_expr = NULL;
	  if (i + 1 < argc)
	    {
	      bse_scm_eval_expr = argv[i + 1];
	      argv[i++] = NULL;
	    }
	  argv[i] = NULL;
	  if (!bse_scm_eval_expr)
	    {
	      printerr ("%s: invalid arguments supplied for: --bse-eval <expression>\n", PRG_NAME);
	      exit (BSE_EXIT_STATUS);
	    }
	}
      else if (strcmp (argv[i], "--bse-enable-register") == 0)
	{
	  bse_scm_enable_register = TRUE;
	  argv[i] = NULL;
	}
      else if (strcmp (argv[i], "--bse-no-load") == 0)
	{
	  bse_scm_auto_load = FALSE;
	  argv[i] = NULL;
	}
      else if (strcmp (argv[i], "--bse-no-play") == 0)
	{
	  bse_scm_auto_play = FALSE;
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-driver-list", argv[i]) == 0)
        {
          initialize_bse_and_exit = TRUE;
          /* leave args for BSE */
        }
      else if (strcmp ("-p", argv[i]) == 0)
        {
          /* modify args for BSE */
          argv[i] = (char*) "--bse-pcm-driver";
        }
      else if (strcmp ("-m", argv[i]) == 0)
        {
          /* modify args for BSE */
          argv[i] = (char*) "--bse-midi-driver";
        }
      else if (strcmp ("--bse-override-script-path", argv[i]) == 0 && i + 1 < argc)
        {
          argv[i++] = NULL;
          boot_script_path = argv[i];
          argv[i] = NULL;
        }
      else if (strcmp (argv[i], "-n") == 0 && i + 1 < argc)
        { /* handled by priviledged launcher */
          argv[i++] = NULL;
          argv[i] = NULL;
        }
      else if (strncmp (argv[i], "-n=", 3) == 0 ||
               (strncmp (argv[i], "-n", 2) == 0 && argv[i][3] >= '0' && argv[i][3] <= '9') ||
               strcmp (argv[i], "-N") == 0)
        { /* handled by priviledged launcher */
          argv[i] = NULL;
        }
      else if (strcmp ("-h", argv[i]) == 0 ||
               strcmp ("--help", argv[i]) == 0)
        {
          shell_print_usage();
          exit (0);
        }
      else if (strcmp ("-v", argv[i]) == 0 ||
               strcmp ("--version", argv[i]) == 0)
        {
          printout ("BSESCM version %s (%s)\n", BST_VERSION, BST_VERSION_HINT);
          printout ("Libraries: ");
          printout ("GLib %u.%u.%u", glib_major_version, glib_minor_version, glib_micro_version);
          printout (", SFI %s", BST_VERSION);
          printout (", BSE %s", BST_VERSION);
          printout (", Guile %u.%u.%u", SCM_MAJOR_VERSION, SCM_MINOR_VERSION, SCM_MICRO_VERSION);
          printout ("\n");
          printout ("Compiled for: %s\n", BST_ARCH_NAME);
          printout ("BSESCM comes with ABSOLUTELY NO WARRANTY.\n");
          printout ("You may redistribute copies of BSESCM under the terms of\n");
          printout ("the GNU Lesser General Public License which can be found in\n");
          printout ("the BEAST source package. Sources, examples and contact\n");
          printout ("information are available at http://beast.testbit.eu/.\n");
          exit (0);
        }
    }

  e = 1;
  for (i = 1; i < argc; i++)
    if (argv[i])
      {
        argv[e++] = argv[i];
        if (i >= e)
          argv[i] = NULL;
      }
  *argc_p = e;
  if (initialize_bse_and_exit)
    {
      Bse::init_async (argc_p, argv, "BSESCM");
      exit (0);
    }
}
static void
shell_print_usage (void)
{
  printout ("Usage: bsescm [options] [files...]\n");
  printout ("Play BSE files and evaluate Scheme code, interactively or from a script.\n");
  printout ("  -h, --help              show this help message\n");
  printout ("  -v, --version           print version and exit\n");
  printout ("  -n NICELEVEL            run with priority NICELEVEL (for suid wrapper bsescm)\n");
  printout ("  -N                      disable renicing\n");
  printout ("  --bse-pipe INFD OUTFD   remote operation filedescriptors\n");
  printout ("  --bse-eval STRING       execute (eval-string STRING) and exit\n");
  printout ("  --bse-enable-register   allow registration of BSE procedures\n");
  printout ("  --bse-no-load           prevent automated plugin and script registration\n");
  printout ("  --bse-no-play           prevent detection of BSE file command line arguments\n");
  printout ("  --bse-latency=USECONDS  specify synthesis latency in milliseconds\n");
  printout ("  --bse-mixing-freq=FREQ  specify synthesis mixing frequency in Hz \n");
  printout ("  --bse-control-freq=FREQ specify control frequency in Hz\n");
  printout ("  --bse-pcm-driver DRIVERCONF\n");
  printout ("  -p DRIVERCONF           try to use the PCM driver DRIVERCONF, multiple\n");
  printout ("                          options may be supplied to try a variety of\n");
  printout ("                          drivers, unless -p auto is given, only the\n");
  printout ("                          drivers listed by -p options are used; each\n");
  printout ("                          DRIVERCONF consists of a driver name and an\n");
  printout ("                          optional comma seperated list of arguments,\n");
  printout ("                          e.g.: -p oss=/dev/dsp2,rw\n");
  printout ("  --bse-midi-driver DRIVERCONF\n");
  printout ("  -m DRIVERCONF           try to use the MIDI driver DRIVERCONF, multiple\n");
  printout ("                          options may be specified similarly to the\n");
  printout ("                          option handling for --bse-pcm-driver\n");
  printout ("  --bse-driver-list       list available PCM and MIDI drivers\n");
  printout ("Guile Options:\n");
  printout ("  -s SCRIPT               load Scheme source code from FILE, and exit\n");
  printout ("  -c EXPR                 evalute Scheme expression EXPR, and exit\n");
  printout ("  --                      stop scanning arguments; run interactively\n");
  printout ("The above switches stop argument processing, and pass all\n");
  printout ("remaining arguments as the value of (command-line).\n");
  printout ("  -l FILE                 load Scheme source code from FILE\n");
  printout ("  -e FUNCTION             after reading script, apply FUNCTION to\n");
  printout ("                          command line arguments\n");
  printout ("  -ds                     do -s script at this point\n");
  printout ("  --debug                 start with debugging evaluator and backtraces\n");
  printout ("  -q                      inhibit loading of user init file\n");
  printout ("  --emacs                 enable Emacs protocol (experimental)\n");
  printout ("  --use-srfi=LS           load SRFI modules for the SRFIs in LS,\n");
  printout ("                          which is a list of numbers like \"2,13,14\"\n");
  printout ("  \\                       read arguments from following script lines\n");
}
