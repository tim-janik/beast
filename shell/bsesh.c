/* BSE-SCM - Bedevilled Sound Engine Scheme Wrapper
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <guile/gh.h>
#include <bse/bse.h>
#include <sfi/sfistore.h> /* no bin-compat */
#include <sys/time.h>
#include <sys/resource.h>
#include "bsescminterp.h"
#include "topconfig.h"

#define	PRG_NAME	"bsesh"

#define BSE_EXIT_STATUS 102


/* --- prototypes --- */
static void	gh_main			(gint	 argc,
					 gchar	*argv[]);
static void	shell_parse_args	(gint    *argc_p,
					 gchar ***argv_p);
static void     shell_print_usage       (void);


/* --- variables --- */
static gint            bse_scm_pipe[2] = { -1, -1 };
static gchar          *bse_scm_eval_expr = NULL;
static gboolean        bse_scm_enable_register = FALSE;
static gboolean        bse_scm_auto_load = TRUE;
static gboolean        bse_scm_auto_play = TRUE;
static SfiComPort     *bse_scm_port = NULL;
static SfiGlueContext *bse_scm_context = NULL;
static const gchar    *boot_script_path = BSE_PATH_SCRIPTS;

/* --- functions --- */
static void
port_closed (SfiComPort *port,
	     gpointer    close_data)
{
  /* we don't do anything fancy here */
  if (port)
    exit (BSE_EXIT_STATUS);
}

int
main (int   argc,
      char *argv[])
{
  const gchar *env_str;

  g_thread_init (NULL);
  g_set_prgname (PRG_NAME);
  sfi_init ();

  env_str = g_getenv ("BSE_SHELL_SLEEP4GDB");
  if (env_str && atoi (env_str) > 0)
    {
      g_message ("going into sleep mode due to debugging request (pid=%u)", getpid ());
      g_usleep (2147483647);
    }

  shell_parse_args (&argc, &argv);

  if (bse_scm_pipe[0] >= 0 && bse_scm_pipe[1] >= 0)
    {
      bse_scm_port = sfi_com_port_from_pipe (PRG_NAME, bse_scm_pipe[0], bse_scm_pipe[1]);
      sfi_com_port_set_close_func (bse_scm_port, port_closed, NULL);
      if (!bse_scm_port->connected)
	{
	  g_printerr ("%s: failed to connect to pipe (%d, %d)\n", PRG_NAME, bse_scm_pipe[0], bse_scm_pipe[1]);
	  exit (BSE_EXIT_STATUS);
	}
      bse_scm_context = sfi_glue_encoder_context (bse_scm_port);
    }

  if (!bse_scm_context)
    {
      /* start our own core thread */
      bse_init_async (&argc, &argv, NULL);
      bse_scm_context = bse_init_glue_context (PRG_NAME);
    }

  /* now that the BSE thread runs, drop scheduling priorities if we have any */
  setpriority (PRIO_PROCESS, getpid(), 0);

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
  const gchar *boot_script = g_intern_printf ("%s/%s", boot_script_path, "bse-scm-glue.boot");
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
        gh_eval_str ("(bse-shell-auto-play)");
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
shell_parse_args (gint    *argc_p,
		  gchar ***argv_p)
{
  guint argc = *argc_p;
  gchar **argv = *argv_p;
  guint i, e;
  gboolean initialize_bse_and_exit = FALSE;

  for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "--") == 0 ||
          strcmp (argv[i], "-s") == 0 ||
          strcmp (argv[i], "-c") == 0)
	break;
      else if (strcmp (argv[i], "--g-fatal-warnings") == 0)
	{
	  GLogLevelFlags fatal_mask;

	  fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
	  fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
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
	      g_printerr ("%s: invalid arguments supplied for: --bse-pipe <inpipe> <outpipe>\n", PRG_NAME);
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
	      g_printerr ("%s: invalid arguments supplied for: --bse-eval <expression>\n", PRG_NAME);
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
          argv[i] = "--bse-pcm-driver";
        }
      else if (strcmp ("-m", argv[i]) == 0)
        {
          /* modify args for BSE */
          argv[i] = "--bse-midi-driver";
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
          g_print ("BSESH version %s (%s)\n", BST_VERSION, BST_VERSION_HINT);
          g_print ("Libraries: ");
          g_print ("GLib %u.%u.%u", glib_major_version, glib_minor_version, glib_micro_version);
          g_print (", SFI %u.%u.%u", bse_major_version, bse_minor_version, bse_micro_version);
          g_print (", BSE %u.%u.%u", bse_major_version, bse_minor_version, bse_micro_version);
          g_print (", Guile %u.%u.%u", SCM_MAJOR_VERSION, SCM_MINOR_VERSION, SCM_MICRO_VERSION);
          g_print ("\n");
          g_print ("Compiled for: %s\n", BST_ARCH_NAME);
          g_print ("BSESH comes with ABSOLUTELY NO WARRANTY.\n");
          g_print ("You may redistribute copies of BSESH under the terms of\n");
          g_print ("the GNU General Public License which can be found in the\n");
          g_print ("BEAST source package. Sources, examples and contact\n");
          g_print ("information are available at http://beast.gtk.org/.\n");
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
      bse_init_async (argc_p, argv_p, NULL);
      exit (0);
    }
}

static void
shell_print_usage (void)
{
  g_print ("Usage: bsesh [options] [files...]\n");
  g_print ("Play BSE files and evaluate Scheme code, interactively or from a script.\n");
  g_print ("  -h, --help              show this help message\n");
  g_print ("  -v, --version           print version and exit\n");
  g_print ("  -n NICELEVEL            run with priority NICELEVEL (for suid wrapper bsesh)\n");
  g_print ("  -N                      disable renicing\n");
  g_print ("  --bse-pipe INFD OUTFD   remote operation filedescriptors\n");
  g_print ("  --bse-eval STRING       execute (eval-string STRING) and exit\n");
  g_print ("  --bse-enable-register   allow registration of BSE procedures\n");
  g_print ("  --bse-no-load           prevent automated plugin and script registration\n");
  g_print ("  --bse-no-play           prevent detection of BSE file command line arguments\n");
  g_print ("  --bse-latency=USECONDS  specify synthesis latency in milliseconds\n");
  g_print ("  --bse-mixing-freq=FREQ  specify synthesis mixing frequency in Hz \n");
  g_print ("  --bse-control-freq=FREQ specify control frequency in Hz\n");
  g_print ("  --bse-pcm-driver DRIVERCONF\n");
  g_print ("  -p DRIVERCONF           try to use the PCM driver DRIVERCONF, multiple\n");
  g_print ("                          options may be supplied to try a variety of\n");
  g_print ("                          drivers, unless -p auto is given, only the\n");
  g_print ("                          drivers listed by -p options are used; each\n");
  g_print ("                          DRIVERCONF consists of a driver name and an\n");
  g_print ("                          optional comma seperated list of arguments,\n");
  g_print ("                          e.g.: -p oss=/dev/dsp2,rw\n");
  g_print ("  --bse-midi-driver DRIVERCONF\n");
  g_print ("  -m DRIVERCONF           try to use the MIDI driver DRIVERCONF, multiple\n");
  g_print ("                          options may be specified similarly to the\n");
  g_print ("                          option handling for --bse-pcm-driver\n");
  g_print ("  --bse-driver-list       list available PCM and MIDI drivers\n");
  g_print ("Guile Options:\n");
  g_print ("  -s SCRIPT               load Scheme source code from FILE, and exit\n");
  g_print ("  -c EXPR                 evalute Scheme expression EXPR, and exit\n");
  g_print ("  --                      stop scanning arguments; run interactively\n");
  g_print ("The above switches stop argument processing, and pass all\n");
  g_print ("remaining arguments as the value of (command-line).\n");
  g_print ("  -l FILE                 load Scheme source code from FILE\n");
  g_print ("  -e FUNCTION             after reading script, apply FUNCTION to\n");
  g_print ("                          command line arguments\n");
  g_print ("  -ds                     do -s script at this point\n");
  g_print ("  --debug                 start with debugging evaluator and backtraces\n");
  g_print ("  -q                      inhibit loading of user init file\n");
  g_print ("  --emacs                 enable Emacs protocol (experimental)\n");
  g_print ("  --use-srfi=LS           load SRFI modules for the SRFIs in LS,\n");
  g_print ("                          which is a list of numbers like \"2,13,14\"\n");
  g_print ("  \\                       read arguments from following script lines\n");
}
