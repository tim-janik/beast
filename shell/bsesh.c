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
#define G_LOG_DOMAIN "BseShell"

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <guile/gh.h>
#include <bse/bse.h>
#include "bsescminterp.h"
#include "../PKG_config.h"

#define	PRG_NAME	"bsesh"

#define	BOILERPLATE_SCM		BSE_PATH_SCRIPTS ## "/bse-scm-glue.boot"


/* --- prototypes --- */
static void	gh_main			(gint	 argc,
					 gchar	*argv[]);
static void	shell_parse_args	(gint    *argc_p,
					 gchar ***argv_p);


/* --- variables --- */
static gint            bse_scm_pipe[2] = { -1, -1 };
static gchar          *bse_scm_eval_expr = NULL;
static gboolean        bse_scm_enable_register = FALSE;
static gboolean        bse_scm_register_plugins = TRUE;
static SfiComPort     *bse_scm_port = NULL;
static SfiGlueContext *bse_scm_context = NULL;


/* --- functions --- */
static void
port_closed (SfiComPort *port,
	     gpointer    close_data)
{
  /* we don't do anything fancy here */
  if (port)
    exit (0);
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
	  exit (1);
	}
      bse_scm_context = sfi_glue_encoder_context (bse_scm_port);
    }

  if (!bse_scm_context)
    {
      /* start our own core thread */
      bse_init_async (&argc, &argv, NULL);
      bse_scm_context = bse_init_glue_context (PRG_NAME);
    }

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
  gh_load (BOILERPLATE_SCM);

  /* eval or interactive */
  if (bse_scm_eval_expr)
    gh_eval_str (bse_scm_eval_expr);
  else
    gh_repl (argc, argv);

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

  for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "--") == 0)
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
	      exit (1);
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
	      exit (1);
	    }
	}
      else if (strcmp (argv[i], "--bse-enable-register") == 0)
	{
	  bse_scm_enable_register = TRUE;
	  argv[i] = NULL;
	}
      else if (strcmp (argv[i], "--bse-no-plugins") == 0)
	{
	  bse_scm_register_plugins = FALSE;
	  argv[i] = NULL;
	}
    }

  e = 0;
  for (i = 1; i < argc; i++)
    {
      if (e)
	{
	  if (argv[i])
	    {
	      argv[e++] = argv[i];
	      argv[i] = NULL;
	    }
	}
      else if (!argv[i])
	e = i;
    }
  if (e)
    *argc_p = e;
}
