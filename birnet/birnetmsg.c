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
#include <stdio.h>


/* --- functions --- */
void
sfi_log_message (const gchar *log_domain,
		 guint        level,
		 const gchar *message)
{
  g_return_if_fail (message != NULL);
  
  switch (level)
    {
    case SFI_LOG_INFO:
    case SFI_LOG_WARN:
    case SFI_LOG_ERROR:
      g_printerr ("%s%s%s: %s\n",
		  log_domain ? log_domain : "",
		  log_domain ? "-" : "",
		  level == SFI_LOG_INFO ? "INFO" : level == SFI_LOG_WARN ? "WARNING" : "ERROR",
		  message);
      break;
    default:
      if (log_domain)
	fprintf (stderr, "%s-DEBUG: %s\n", log_domain, message);
      else
	fprintf (stderr, "DEBUG: %s\n", message);
      // g_log (log_domain, G_LOG_LEVEL_DEBUG, "%s", message);
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
