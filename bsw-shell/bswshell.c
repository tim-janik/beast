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
#include <guile/gh.h>
#include <bsw/bsw.h>
#include <bsw/bswglue.h>
#include "bswscminterp.h"



/* --- prototypes --- */
static void	gh_main	(gint	 argc,
			 gchar	*argv[]);


/* --- functions --- */
int
main (int   argc,
      char *argv[])
{
  g_thread_init (NULL);
  // g_log_set_always_fatal (g_log_set_always_fatal (G_LOG_FATAL_MASK) | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL);
  bsw_init (&argc, &argv, NULL);
  bsw_register_plugins (NULL, FALSE, NULL);

  gh_enter (argc, argv, gh_main);

  return 0;
}

static SCM
bsw_scm_wrap_server_get (void)
{
  BswProxy server;
  SCM s_retval;

  BSW_SCM_DEFER_INTS ();
  server = BSW_SERVER;
  BSW_SCM_ALLOW_INTS ();
  s_retval = gh_ulong2scm (server);

  return s_retval;
}

static void
gh_main (int   argc,
	 char *argv[])
{
  gh_define ("bsw-server", gh_ulong2scm (BSW_SERVER));
  gh_new_procedure0_0 ("bsw-server-get", bsw_scm_wrap_server_get);

  bsw_scm_interp_init ();

  gh_repl (argc, argv);
}
