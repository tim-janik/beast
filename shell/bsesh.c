/* BSW-SCM - Bedevilled Sound Engine Scheme Wrapper
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
#define G_LOG_DOMAIN "BswShell"

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <guile/gh.h>
#include <bsw/bsw.h>
#include <bsw/bswglue.h>
#include "bswscminterp.h"
#include "../PKG_config.h"



/* --- prototypes --- */
static void	gh_main			(gint	 argc,
					 gchar	*argv[]);
static void	shell_parse_args	(gint    *argc_p,
					 gchar ***argv_p);


/* --- variables --- */
static BswSCMWire *bsw_scm_wire = NULL;
static gchar      *bsw_scm_remote_expr = NULL;


/* --- functions --- */
int
main (int   argc,
      char *argv[])
{
  const gchar *env_str;

  g_thread_init (NULL);
  env_str = g_getenv ("BSW_SHELL_SLEEP4GDB");
  if (env_str && atoi (env_str) > 0)
    {
      g_message ("going into sleep mode due to debugging request (pid=%u)", getpid ());
      g_usleep (2147483647);
    }

  bsw_init (&argc, &argv, NULL);
  shell_parse_args (&argc, &argv);

  gh_enter (argc, argv, gh_main);

  return 0;
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
      else if (strcmp (argv[i], "--bse-command-pipe") == 0)
	{
	  /* first two arguments are input and output pipes from BseServer,
	   * the third is supplied by the BSE Scheme code and is an
	   * expression for us to evaulate
	   */
	  if (i + 3 < argc)
	    {
	      gint remote_input = atoi (argv[i + 1]);
	      gint remote_output = atoi (argv[i + 2]);

	      bsw_scm_remote_expr = argv[i + 3];
	      if (remote_input > 2 && remote_output > 2 && bsw_scm_remote_expr)
		bsw_scm_wire = bsw_scm_wire_from_pipe ("BSW-Shell", remote_input, remote_output);
	      argv[i] = NULL;
	      i += 1;
	      argv[i] = NULL;
	      i += 1;
	      argv[i] = NULL;
	      i += 1;
	    }
	  argv[i] = NULL;
	  if (!bsw_scm_wire)
	    {
	      g_printerr ("bswshell: invalid arguments supplied for: --bse-command-pipe <inpipe> <outpipe> <expr>\n");
	      exit (1);
	    }
	}
      else if (strcmp (argv[i], "--bse-enable-register") == 0)
	{
	  bsw_scm_enable_script_register (TRUE);
	  argv[i] = NULL;
	}
      else if (strcmp (argv[i], "--bse-enable-server") == 0)
	{
	  bsw_scm_enable_server (TRUE);
	  argv[i] = NULL;
	}
      else if (strcmp (argv[i], "-p") == 0)
	{
	  static gboolean registered_plugins = FALSE;

	  if (!registered_plugins)
	    {
	      registered_plugins = TRUE;
	      bsw_register_plugins (NULL, TRUE, NULL);
	    }
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

#define	BOILERPLATE_SCM		BSW_PATH_SCRIPTS ## "/bsw-boilerplate.scm.x"

static void
gh_main (int   argc,
	 char *argv[])
{
  if (!bsw_scm_wire)
    bsw_scm_enable_server (TRUE);	/* do this before interp_init */

  bsw_scm_interp_init (bsw_scm_wire);

  gh_load (BOILERPLATE_SCM);

  if (bsw_scm_wire)
    {
      BSW_SCM_DEFER_INTS ();
      gh_eval_str (bsw_scm_remote_expr);
      bsw_scm_wire_died (bsw_scm_wire);
      BSW_SCM_ALLOW_INTS ();
    }
  else
    gh_repl (argc, argv);
}
