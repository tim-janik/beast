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
#include <string.h>
#include <errno.h>


/* --- variables --- */

static GQuark quark_sfi_log_key = 0;

/* --- functions --- */


static inline const gchar*
sfi_log_pop_key (const gchar *fallback)
{
  const gchar *key = sfi_thread_get_qdata (quark_sfi_log_key);
  if (key)
    sfi_thread_set_qdata (quark_sfi_log_key, NULL);
  return key ? key : fallback;
}

void
sfi_log_push_key (const gchar *static_key)
{
  sfi_thread_set_qdata (quark_sfi_log_key, (gchar*) static_key);
}

static void
sfi_log_message (const gchar *log_domain,
		 const gchar *key,
		 guint        level,
		 const gchar *message)
{
  g_return_if_fail (message != NULL);
  
  switch (level)
    {
      const gchar *pname;
      SfiTime t;
      guint ts, tm, th;
    case SFI_LOG_WARN:
    case SFI_LOG_ERROR:
      pname = g_get_prgname ();
      if (pname)
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
      g_printerr ("%s(%s): %s\n",
		  log_domain ? log_domain : "",
		  key,
		  message);
      break;
    case SFI_LOG_DEBUG:
      t = sfi_time_from_utc (sfi_time_system ());
      t /= SFI_USEC_FACTOR;
      ts = t % 60;
      t /= 60;
      tm = t % 60;
      t /= 60;
      th = t % 24;
      fprintf (stderr, "=%02u:%02u:%02u %s(%s)[%u]: %s\n",
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

typedef struct {
  gchar  **keys;
  guint    n_keys;
  gboolean match_all;
} KeyList;

static inline gboolean
key_list_test (KeyList     *self,
	       const gchar *key)
{
  gint offs = 0, n = self->n_keys;
  while (offs < n)
    {
      gint i = (offs + n) >> 1;
      gint cmp = strcmp (key, self->keys[i]);
      if (cmp < 0)
	n = i;
      else if (cmp > 0)
	offs = i + 1;
      else
	return TRUE;
    }
  return FALSE;
}

static void
key_list_reset (KeyList *self)
{
  guint i = self->n_keys;
  self->n_keys = 0;
  while (i--)
    g_free (self->keys[i]);
  g_free (self->keys);
  self->keys = NULL;
  self->match_all = FALSE;
}

static void
key_list_add (KeyList     *self,
	      const gchar *string)
{
  gchar *s, *k, *p;
  GSList *slist = NULL;
  guint i, l;

  s = g_strconcat (":", string, ":", NULL);
  if (strstr (s, ":all:"))
    {
      g_free (s);
      self->match_all = TRUE;
      i = self->n_keys;
      self->n_keys = 0;
      while (i--)
	g_free (self->keys[i]);
      g_free (self->keys);
      self->keys = NULL;
      return;
    }

  k = s + 1;
  for (l = 0; l < self->n_keys; l++)
    slist = g_slist_prepend (slist, self->keys[l]);
  p = strchr (k, ':');
  while (p)
    {
      if (k < p)
	{
	  *p = 0;
	  slist = g_slist_prepend (slist, g_strdup (k));
	  l++;
	}
      k = p + 1;
      p = strchr (k, ':');
    }
  g_free (s);

  slist = g_slist_sort (slist, (GCompareFunc) strcmp);
  self->keys = g_renew (gchar*, self->keys, l);
  for (i = 0; slist; i++)
    {
      k = g_slist_pop_head (&slist);
      if (i && strcmp (k, self->keys[i - 1]) == 0)
	{
	  l--;
	  g_free (k);
	  i--;
	}
      else
	self->keys[i] = k;
    }
  self->keys = g_renew (gchar*, self->keys, l);
  self->n_keys = l;
}

static SfiMutex key_mutex = { 0, };
static KeyList debug_klist = { 0, };
static KeyList info_klist = { 0, };

void
_sfi_init_log (void)
{
  quark_sfi_log_key = g_quark_from_string ("SFI-log-key");
  sfi_mutex_init (&key_mutex);
  sfi_log_reset_info ();
  sfi_log_reset_debug ();
}

void
sfi_log_allow_info (const gchar *string)
{
  g_return_if_fail (string != NULL);

  SFI_SPIN_LOCK (&key_mutex);
  key_list_add (&info_klist, string);
  SFI_SPIN_UNLOCK (&key_mutex);
}

void
sfi_log_reset_info (void)
{
  SFI_SPIN_LOCK (&key_mutex);
  key_list_reset (&info_klist);
  key_list_add (&info_klist, "misc");
  SFI_SPIN_UNLOCK (&key_mutex);
}

void
sfi_log_allow_debug (const gchar *string)
{
  g_return_if_fail (string != NULL);

  SFI_SPIN_LOCK (&key_mutex);
  key_list_add (&debug_klist, string);
  SFI_SPIN_UNLOCK (&key_mutex);
}

void
sfi_log_reset_debug (void)
{
  SFI_SPIN_LOCK (&key_mutex);
  key_list_reset (&debug_klist);
  SFI_SPIN_UNLOCK (&key_mutex);
}

gboolean
sfi_debug_test_key (const gchar *key)
{
  gboolean match;

  g_return_val_if_fail (key != NULL, FALSE);

  if (debug_klist.match_all)
    match = TRUE;
  else
    {
      SFI_SPIN_LOCK (&key_mutex);
      match = key_list_test (&debug_klist, key);
      SFI_SPIN_UNLOCK (&key_mutex);
    }

  return match;
}

void
sfi_log_valist (const gchar *log_domain,
		guint        level,
		const gchar *format,
		va_list      args)
{
  gint saved_errno = errno;
  const gchar *key = sfi_log_pop_key ("misc");
  gboolean match;

  g_return_if_fail (format != NULL);

  if (level == SFI_LOG_DEBUG && !debug_klist.match_all)
    {
      SFI_SPIN_LOCK (&key_mutex);
      match = key_list_test (&debug_klist, key);
      SFI_SPIN_UNLOCK (&key_mutex);
    }
  else if (level == SFI_LOG_INFO && !info_klist.match_all)
    {
      SFI_SPIN_LOCK (&key_mutex);
      match = key_list_test (&info_klist, key);
      SFI_SPIN_UNLOCK (&key_mutex);
    }
  else
    match = TRUE;
  if (match)
    {
      gchar *buffer = g_strdup_vprintf (format, args);
      sfi_log_message (log_domain, key, level, buffer);
      g_free (buffer);
    }
  errno = saved_errno;
}

/**
 * sfi_log
 * @log_domain: log domain
 * @level:      one of %SFI_LOG_ERROR, %SFI_LOG_WARN, %SFI_LOG_INFO or %SFI_LOG_DEBUG
 * @format:     printf()-like format string
 * @...:        message args
 *
 * Log a message through SFIs logging mechanism. The current
 * value of errno is preserved around calls to this function.
 * Usually this function isn't used directly, but through one
 * of sfi_debug(), sfi_warn(), sfi_info() or sfi_error().
 * The @log_domain indicates the calling module and relates to
 * %G_LOG_DOMAIN as used by g_log().
 * This function is MT-safe and may be called from any thread.
 */
void
sfi_log (const gchar *log_domain,
	 guint        level,
	 const gchar *format,
	 ...)
{
  va_list args;
  va_start (args, format);
  sfi_log_valist (log_domain, level, format, args);
  va_end (args);
}
