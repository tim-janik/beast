/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997, 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include	"bsemain.h"

#include	"bsesong.h"
#include	"bsesongsequencer.h"
#include	"bsestream.h"
#include	"bseplugin.h"
#include	<string.h>
#include	<stdlib.h>


/* --- variables --- */
static gboolean        bse_is_initialized = FALSE;
BseDebugFlags          bse_debug_flags = 0;
static GDebugKey       bse_debug_keys[] = { /* keep in sync with bsedefs.h */
  { "tables",		BSE_DEBUG_TABLES, },
  { "classes",		BSE_DEBUG_CLASSES, },
  { "objects",		BSE_DEBUG_OBJECTS, },
  { "notify",		BSE_DEBUG_NOTIFY, },
  { "plugins",		BSE_DEBUG_PLUGINS, },
  { "regs",		BSE_DEBUG_REGS, },
  { "chunks",		BSE_DEBUG_CHUNKS, },
};
static const guint bse_n_debug_keys = sizeof (bse_debug_keys) / sizeof (bse_debug_keys[0]);


/* --- functions --- */
gboolean
bse_initialized (void)
{
  return bse_is_initialized;
}

static void
bse_parse_args (gint    *argc_p,
		gchar ***argv_p)
{
  guint argc = *argc_p;
  gchar **argv = *argv_p;
  gchar *envar;
  guint i, e;
  
  envar = getenv ("BSE_DEBUG");
  if (envar)
    bse_debug_flags |= g_parse_debug_string (envar, bse_debug_keys, bse_n_debug_keys);
  envar = getenv ("BSE_NO_DEBUG");
  if (envar)
    bse_debug_flags &= ~g_parse_debug_string (envar, bse_debug_keys, bse_n_debug_keys);

  for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "--g-fatal-warnings") == 0)
	{
	  GLogLevelFlags fatal_mask;
	  
	  fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
	  fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
	  g_log_set_always_fatal (fatal_mask);
	  
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-debug", argv[i]) == 0 ||
	       strncmp ("--bse-debug=", argv[i], 12) == 0)
	{
	  gchar *equal = argv[i] + 11;

	  if (*equal == '=')
	    bse_debug_flags |= g_parse_debug_string (equal + 1, bse_debug_keys, bse_n_debug_keys);
	  else if (i + 1 < argc)
	    {
	      bse_debug_flags |= g_parse_debug_string (argv[i + 1],
						       bse_debug_keys,
						       bse_n_debug_keys);
	      argv[i] = NULL;
	      i += 1;
	    }
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-no-debug", argv[i]) == 0 ||
	       strncmp ("--bse-no-debug=", argv[i], 15) == 0)
	{
	  gchar *equal = argv[i] + 14;

	  if (*equal == '=')
	    bse_debug_flags &= ~g_parse_debug_string (equal + 1, bse_debug_keys, bse_n_debug_keys);
	  else if (i + 1 < argc)
	    {
	      bse_debug_flags &= ~g_parse_debug_string (argv[i + 1],
							bse_debug_keys,
							bse_n_debug_keys);
	      argv[i] = NULL;
	      i += 1;
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

void
bse_init (int	  *argc_p,
	  char	***argv_p)
{
  g_return_if_fail (bse_is_initialized == FALSE);
  bse_is_initialized = TRUE;
  
  g_assert (BSE_BYTE_ORDER == BSE_LITTLE_ENDIAN || BSE_BYTE_ORDER == BSE_BIG_ENDIAN);
  
  if (argc_p && argv_p)
    {
      g_set_prgname (**argv_p);
      bse_parse_args (argc_p, argv_p);
    }
  
  bse_globals_init ();
  
  bse_type_init ();

  bse_plugin_init ();
}
