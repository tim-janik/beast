/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002-2004 Tim Janik
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
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>


typedef struct {
  int           match_all;
  unsigned int  n_keys;
  char        **keys;
} KeyList;


/* --- variables --- */
static G_LOCK_DEFINE (key_mutex);
static KeyList        key_list = { 0, 0, 0 };
static const char    *last_enabled_cache = (char*) 1;
static const char    *last_disabled_cache = (char*) 1;
static guint          error_actions = SFI_LOG_TO_STDLOG | SFI_LOG_TO_HANDLER;
static guint          warn_actions  = SFI_LOG_TO_STDLOG | SFI_LOG_TO_HANDLER;
static guint          info_actions  = SFI_LOG_TO_STDLOG | SFI_LOG_TO_HANDLER;
static guint          diag_actions  = SFI_LOG_TO_STDLOG;
static guint          debug_actions = SFI_LOG_TO_STDERR;
static int            stdlog_syslog_priority = 0; // LOG_USER | LOG_INFO;
static gboolean       stdlog_to_stderr = TRUE;
static FILE          *stdlog_file = NULL;
static SfiLogHandler  log_handler = sfi_log_default_handler;


/* --- functions --- */
static inline char**
key_list_lookup (KeyList    *self,
                 const char *key)
{
  int offs = 0, n = self->n_keys;
  while (offs < n)
    {
      int i = (offs + n) >> 1;
      int cmp = strcmp (key, self->keys[i]);
      if (cmp < 0)
	n = i;
      else if (cmp > 0)
	offs = i + 1;
      else
	return self->keys + i;
    }
  return NULL;
}

static void
key_list_reset (KeyList *self)
{
  unsigned int i = self->n_keys;
  self->n_keys = 0;
  while (i--)
    g_free (self->keys[i]);
  g_free (self->keys);
  self->keys = NULL;
  self->match_all = FALSE;
}

static void
key_list_allow (KeyList    *self,
                const char *string)
{
  char *s, *k, *p;
  GSList *slist = NULL;
  guint i, l;

  /* handle :all: special case */
  s = g_strconcat (":", string, ":", NULL);
  if (strstr (s, ":all:"))
    {
      g_free (s);
      key_list_reset (self);
      self->match_all = TRUE;
      return;
    }

  /* list all old keys */
  for (l = 0; l < self->n_keys; l++)
    slist = g_slist_prepend (slist, self->keys[l]);

  /* and list all new kyes */
  k = s + 1;
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

  /* sort away */
  slist = g_slist_sort (slist, (GCompareFunc) strcmp);

  /* and reinsert, dedup */
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
  self->n_keys = l;
  self->keys = g_renew (char*, self->keys, self->n_keys);
}

static void
key_list_deny (KeyList    *self,
               const char *string)
{
  char *s, *k, *p, **pp;
  GSList *slist = NULL;
  guint i, j;

  /* handle :all: special case */
  s = g_strconcat (":", string, ":", NULL);
  if (strstr (s, ":all:"))
    {
      g_free (s);
      key_list_reset (self);
      return;
    }

  /* find all places containing keys to be removed */
  k = s + 1;
  p = strchr (k, ':');
  while (p)
    {
      if (k < p)
	{
	  *p = 0;
          pp = key_list_lookup (self, k);
	  if (pp)
            slist = g_slist_prepend (slist, pp);
	}
      k = p + 1;
      p = strchr (k, ':');
    }
  g_free (s);

  /* remove keys */
  while (slist)
    {
      GSList *tmp = slist;
      char **pp = slist->data;
      slist = slist->next;
      g_slist_free_1 (tmp);
      g_free (pp[0]);
      pp[0] = NULL;
    }

  /* collapse list */
  for (i = 0, j = 0; i < self->n_keys; i++)
    if (self->keys[i])
      self->keys[j++] = self->keys[i];
  self->n_keys = j;
  self->keys = g_renew (char*, self->keys, self->n_keys);
}

void
sfi_debug_allow (const char *key)
{
  if (key)
    {
      G_LOCK (key_mutex);
      last_disabled_cache = (char*) 1;
      last_enabled_cache = (char*) 1;
      key_list_allow (&key_list, key);
      G_UNLOCK (key_mutex);
    }
}

void
sfi_debug_deny (const char *key)
{
  if (key)
    {
      G_LOCK (key_mutex);
      last_disabled_cache = (char*) 1;
      last_enabled_cache = (char*) 1;
      key_list_deny (&key_list, key);
      G_UNLOCK (key_mutex);
    }
}

#ifndef g_atomic_pointer_get    /* handle glib pre 2.4 */
#  define g_atomic_pointer_get(loc) (*(loc))
#endif

int
sfi_debug_check (const char *key)
{
  gboolean match;
  if (key == g_atomic_pointer_get (&last_disabled_cache))
    return FALSE;
  if (key == g_atomic_pointer_get (&last_enabled_cache) || key_list.match_all || !key)
    return TRUE;
  G_LOCK (key_mutex);
  match = key_list_lookup (&key_list, key) != NULL;
  if (match)
    last_enabled_cache = key;
  else
    last_disabled_cache = key;
  G_UNLOCK (key_mutex);
  return match;
}

static char*
log_prefix (const char  *prg_name,
            guint        pid,
            guchar       level,
            const char  *log_domain,
            const char  *postfix,
            const char  *key)
{
  GString *gstring = g_string_new (prg_name);
  if (pid)
    g_string_append_printf (gstring, "[%u]", pid);
  if (gstring->len)
    g_string_append (gstring, ":");
  if (level)
    g_string_append_printf (gstring, "%02x:", level);
  if (log_domain)
    g_string_append (gstring, log_domain);
  if (log_domain && postfix)
    g_string_append (gstring, "-");
  if (postfix)
    g_string_append (gstring, postfix);
  if (key)
    {
      if (log_domain || postfix)
        g_string_append_printf (gstring, "(%s)", key);
      else
        g_string_append (gstring, key);
    }
  if (log_domain || postfix || key)
    g_string_append (gstring, ":");
  if (gstring->str[gstring->len - 1] == ':')
    gstring->str[gstring->len - 1] = 0;
  return g_string_free (gstring, FALSE);
}

static void
log_printerr (const char        *log_domain,
              unsigned char      level,
              const char        *key,
              const char        *message)
{
  const gchar *lname = NULL;
  switch (level)
    {
      gchar *prefix;
    case SFI_LOG_ERROR:
      if (!lname)
        lname = "ERROR";
    case SFI_LOG_WARN:
      if (!lname)
        lname = "WARNING";
    case SFI_LOG_INFO:
      if (!lname)
        lname = "INFO";
      prefix = log_prefix (g_get_prgname(), sfi_thread_self_pid(), 0,
                           log_domain, lname,
                           NULL);
      fprintf (stderr, "%s: %s\n", prefix, message);
      g_free (prefix);
      break;
    case SFI_LOG_DIAG:
      prefix = log_prefix (g_get_prgname(), sfi_thread_self_pid(), 0,
                           log_domain, NULL,
                           key);
      fprintf (stderr, "%s: %s\n", prefix, message);
      g_free (prefix);
      break;
    case SFI_LOG_DEBUG:
      prefix = log_prefix (g_get_prgname(), sfi_thread_self_pid(), 0,
                           1 ? NULL : log_domain, "DEBUG",
                           key);
      fprintf (stderr, "%s: %s\n", prefix, message);
      g_free (prefix);
      break;
    default:
      prefix = log_prefix (g_get_prgname(), sfi_thread_self_pid(), level,
                           log_domain, NULL,
                           key);
      fprintf (stderr, "%s: %s\n", prefix, message);
      g_free (prefix);
      break;
    }
}

static void
log_valist (const char       *log_domain,
            unsigned char     level,
            const SfiLogContext *lcontext,
            const char       *format,
            va_list           args)
{
  guint actions;
  char *message = g_strdup_vprintf (format, args);
  const char *slevel;
  gboolean tostderr;
  switch (level)
    {
    case SFI_LOG_ERROR: actions = error_actions;     slevel = "ERROR";   break;
    case SFI_LOG_WARN:  actions = warn_actions;      slevel = "WARNING"; break;
    case SFI_LOG_INFO:  actions = info_actions;      slevel = "INFO";    break;
    case SFI_LOG_DIAG:  actions = diag_actions;      slevel = "DIAG";    break;
    case SFI_LOG_DEBUG: actions = debug_actions;     slevel = "DEBUG";   break;
    default:            actions = SFI_LOG_TO_STDERR; slevel = NULL;      break;
    }
  tostderr = (actions & SFI_LOG_TO_STDERR) != 0;
  tostderr |= (actions & SFI_LOG_TO_STDLOG) && stdlog_to_stderr;
  if (stdlog_syslog_priority && (actions & SFI_LOG_TO_STDLOG))
    {
      char *prefix = log_prefix (NULL, 0, slevel ? 0 : level,
                                 log_domain, slevel,
                                 lcontext->key);
      syslog (stdlog_syslog_priority, "%s: %s\n", prefix, message);
      g_free (prefix);
    }
  if (tostderr)
    log_printerr (log_domain, level, lcontext->key, message);
  if (stdlog_file && (actions & SFI_LOG_TO_STDLOG))
    {
      char *prefix = log_prefix (g_get_prgname(), sfi_thread_self_pid(), slevel ? 0 : level,
                                 log_domain, slevel,
                                 lcontext->key);
      fprintf (stdlog_file, "%s: %s\n", prefix, message);
      g_free (prefix);
    }
  if (log_handler && (actions & SFI_LOG_TO_HANDLER))
    log_handler (log_domain, level, lcontext, message);
  g_free (message);
}

/**
 * sfi_log_printf
 * @log_domain: log domain
 * @level:      one of %SFI_LOG_ERROR, %SFI_LOG_WARN, %SFI_LOG_INFO, %SFI_LOG_DIAG or %SFI_LOG_DEBUG
 * @lcontext:   structure of type SfiLogContext
 * @format:     printf()-like format string
 * @...:        message args
 *
 * Log a message through SFIs logging mechanism. The current
 * value of errno is preserved around calls to this function.
 * Usually this function isn't used directly, but through one
 * of sfi_debug(), sfi_diag(), sfi_info(), sfi_warn() or sfi_error().
 * The @log_domain indicates the calling module and relates to
 * %G_LOG_DOMAIN as used by g_log().
 * This function is MT-safe and may be called from any thread.
 */
void
sfi_log_printf (const char         *log_domain,
                unsigned char       level,
                const SfiLogContext lcontext,
                const char         *format,
                ...)
{
  gint saved_errno = errno;
  va_list args;

  /* debug messages are selected via keys */
  if (level == SFI_LOG_DEBUG && (lcontext.key == last_disabled_cache ||
                                 !sfi_debug_check (lcontext.key)))
    {
      errno = saved_errno;
      return;
    }
  va_start (args, format);
  log_valist (log_domain, level, &lcontext, format, args);
  va_end (args);
  errno = saved_errno;
}

void
sfi_log_valist (const char         *log_domain,
                unsigned char       level,
                const SfiLogContext lcontext,
                const char         *format,
                va_list             args)
{
  gint saved_errno = errno;
  /* debug messages are selected via keys */
  if (level == SFI_LOG_DEBUG && (lcontext.key == last_disabled_cache ||
                                 !sfi_debug_check (lcontext.key)))
    {
      errno = saved_errno;
      return;
    }
  log_valist (log_domain, level, &lcontext, format, args);
  errno = saved_errno;
}

void
sfi_log_configure_level (unsigned char level,
                         SfiLogActions actions)
{
  switch (level)
    {
    case SFI_LOG_ERROR: error_actions = actions; break;
    case SFI_LOG_WARN:  warn_actions  = actions; break;
    case SFI_LOG_INFO:  info_actions  = actions; break;
    case SFI_LOG_DIAG:  diag_actions  = actions; break;
    case SFI_LOG_DEBUG: debug_actions = actions; break;
    }
}

void
sfi_log_configure_stdlog (int         stdlog_to_err_bool,
                          const char *stdlog_filename,
                          int         syslog_priority)
{
  stdlog_to_stderr = stdlog_to_err_bool != 0;
  stdlog_syslog_priority = syslog_priority;
  if (stdlog_file)
    fclose (stdlog_file);
  stdlog_file = NULL;
  if (stdlog_filename)
    stdlog_file = fopen (stdlog_filename, "a");
}

void
sfi_log_default_handler (const char             *log_domain,
                         unsigned char           level,
                         const SfiLogContext   *lcontext,
                         const char             *message)
{
  if (level >= 32 && level <= 126)
    g_printerr ("\t(*) UNHANDLED<%c>: %s\n", level, message);
  else
    g_printerr ("\t(*) UNHANDLED<0x%02x>: %s\n", level, message);
}

void
sfi_log_set_handler (SfiLogHandler handler)
{
  log_handler = handler;
}
