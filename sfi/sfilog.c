/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "sfilog.h"
#include "sfithreads.h"
#include "sfitime.h"
#include <unistd.h>
#include <stdio.h>


/* --- variables --- */
static SfiLogVerbosity log_verbosity = SFI_LOG_VERBOSITY_NORMAL;


/* --- functions --- */
void
sfi_log_set_verbosity (SfiLogVerbosity verbosity)
{
  switch (verbosity)
    {
    case SFI_LOG_VERBOSITY_DETAILED:
    case SFI_LOG_VERBOSITY_DEVELOPMENT:
      log_verbosity = verbosity;
      break;
    default:
      log_verbosity = SFI_LOG_VERBOSITY_NORMAL;
      break;
    }
}

static const gchar*
_sfi_log_pop_key (const gchar *fallback)
{
  const gchar *key = sfi_thread_get_data ("SFI-log-key");
  if (key)
    sfi_thread_set_data ("SFI-log-key", NULL);
  return key ? key : fallback;
}

void
sfi_log_push_key (const gchar *static_key)
{
  sfi_thread_set_data ("SFI-log-key", (gchar*) static_key);
}

void
sfi_log_message (const gchar *log_domain,
		 guint        level,
		 const gchar *message)
{
  const gchar *key = _sfi_log_pop_key ("misc");

  g_return_if_fail (message != NULL);
  
  switch (level)
    {
      const gchar *pname;
      SfiTime t;
      guint ts, tm, th;
    case SFI_LOG_WARN:
    case SFI_LOG_ERROR:
      pname = g_get_prgname ();
      if (log_verbosity == SFI_LOG_VERBOSITY_DEVELOPMENT)
	g_printerr ("%s[%u]:%s%s: %s\n",
		    pname ? pname : "process", getpid (),
		    log_domain ? log_domain : "",
		    level == SFI_LOG_WARN ? "-WARNING" : "-ERROR",
		    message);
      else if (pname)
	g_printerr ("%s:%s%s: %s\n",
		    pname,
		    log_domain ? log_domain : "",
		    level == SFI_LOG_WARN ? "-WARNING" : "-ERROR",
		    message);
      else
	g_printerr ("%s%s: %s\n",
		    log_domain ? log_domain : "",
		    level == SFI_LOG_WARN ? "-WARNING" : "-ERROR",
		    message);
      break;
    case SFI_LOG_INFO:
      switch (log_verbosity)
	{
	case SFI_LOG_VERBOSITY_NORMAL:
	  g_printerr ("%s\n", message);
	  break;
	case SFI_LOG_VERBOSITY_DETAILED:
	  g_printerr ("%s(%s): %s\n",
		      log_domain ? log_domain : "",
		      key,
		      message);
	  break;
	case SFI_LOG_VERBOSITY_DEVELOPMENT:
	  g_printerr ("%s(%s)[%u]: %s\n",
		      log_domain ? log_domain : "",
		      key,
		      getpid (),
		      message);
	  break;
	}
      break;
    case SFI_LOG_DEBUG:
      t = sfi_time_from_utc (sfi_time_system ());
      t /= SFI_USEC_FACTOR;
      ts = t % 60;
      t /= 60;
      tm = t % 60;
      t /= 60;
      th = t % 24;
      fprintf (stderr, "%02u:%02u:%02u|%s(%s)[%u]: %s\n",
	       th, tm, ts,
	       log_domain ? log_domain : "",
	       key,
	       getpid (),
	       message);
      break;
    default:
      pname = g_get_prgname ();
      g_printerr ("%s[%u]:%s(%s)<%d>: %s\n",
		  pname ? pname : "process", getpid (),
		  log_domain ? log_domain : "",
		  key,
		  level,
		  message);
      break;
    }
}

void
sfi_log_valist (const gchar *log_domain,
		guint        level,
		const gchar *format,
		va_list      args)
{
  gchar *buffer;

  g_return_if_fail (format != NULL);

  buffer = g_strdup_vprintf (format, args);
  sfi_log_message (log_domain, level, buffer);
  g_free (buffer);
}
