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


/* --- variables --- */
static gboolean bse_is_initialized = FALSE;


/* --- functions --- */
gboolean
bse_initialized (void)
{
  return bse_is_initialized;
}

static void
bse_parse_args (int *argc_p,
		char ***argv_p)
{
  guint i, e;
  
  for (i = 1; i < *argc_p; i++)
    {
      if (strcmp ((*argv_p)[i], "--g-fatal-warnings") == 0)
	{
	  GLogLevelFlags fatal_mask;
	  
	  fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
	  fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
	  g_log_set_always_fatal (fatal_mask);
	  
	  (*argv_p)[i] = NULL;
	}
    }
  
  e = 0;
  for (i = 1; i < *argc_p; i++)
    {
      if (e)
	{
	  if ((*argv_p)[i])
	    {
	      (*argv_p)[e++] = (*argv_p)[i];
	      (*argv_p)[i] = NULL;
	    }
	}
      else if (!(*argv_p)[i])
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
