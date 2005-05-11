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
static SfiMutex       logging_mutex;
static KeyList        key_list = { 0, 0, 0 };
static const char    *last_enabled_cache = (char*) 1;
static const char    *last_disabled_cache = (char*) 1;
static guint          error_actions = SFI_LOG_TO_STDLOG | SFI_LOG_TO_HANDLER;
static guint          warn_actions  = SFI_LOG_TO_STDLOG | SFI_LOG_TO_HANDLER;
static guint          info_actions  = SFI_LOG_TO_STDLOG | SFI_LOG_TO_HANDLER;
static guint          diag_actions  = SFI_LOG_TO_STDLOG;
static guint          debug_actions = SFI_LOG_TO_STDERR;
static guint          stdlog_syslog_priority = 0; // LOG_USER | LOG_INFO;
static gboolean       stdlog_to_stderr = TRUE;
static FILE          *stdlog_file = NULL;
static GQuark         quark_log_handler = 0;
static GQuark         quark_log_bits = 0;

/* --- prototypes --- */
static void     sfi_log_string_intern  (const char     *log_domain,
                                        unsigned char   level,
                                        const char     *key,
                                        const char     *config_blurb,
                                        const char     *string);

/* --- functions --- */
void
_sfi_init_logging (void)
{
  g_assert (quark_log_handler == 0);
  quark_log_handler = g_quark_from_static_string ("SfiLogHandler");
  quark_log_bits = g_quark_from_static_string ("SfiLogBit-threadlist");
  sfi_mutex_init (&logging_mutex);
}

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

  /* handle :all: special case */
  s = g_strconcat (":", string, ":", NULL);
  if (self->match_all || strstr (s, ":all:"))
    {
      g_free (s);
      key_list_reset (self);
      self->match_all = TRUE;
      return;
    }

  /* list all old keys */
  guint l;
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
  guint i;
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
      char **pp = g_slist_pop_head (&slist);
      g_free (*pp);
      *pp = NULL;
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
      SFI_SPIN_LOCK (&logging_mutex);
      last_disabled_cache = (char*) 1;
      last_enabled_cache = (char*) 1;
      key_list_allow (&key_list, key);
      SFI_SPIN_UNLOCK (&logging_mutex);
    }
}

void
sfi_debug_deny (const char *key)
{
  if (key)
    {
      SFI_SPIN_LOCK (&logging_mutex);
      last_disabled_cache = (char*) 1;
      last_enabled_cache = (char*) 1;
      key_list_deny (&key_list, key);
      SFI_SPIN_UNLOCK (&logging_mutex);
    }
}

int
sfi_debug_check (const char *key)
{
  if (key == last_disabled_cache)
    return FALSE;
  if (key == last_enabled_cache || key_list.match_all || !key)
    return TRUE;
  SFI_SPIN_LOCK (&logging_mutex);
  gboolean match = key_list_lookup (&key_list, key) != NULL;
  if (match)
    last_enabled_cache = key;
  else
    last_disabled_cache = key;
  SFI_SPIN_UNLOCK (&logging_mutex);
  return match;
}

void
sfi_log_assign_level (unsigned char level,
                      SfiLogFlags   actions)
{
  switch (level)
    {
    case SFI_LOG_ERROR:   error_actions = actions; break;
    case SFI_LOG_WARNING: warn_actions  = actions; break;
    case SFI_LOG_INFO:    info_actions  = actions; break;
    case SFI_LOG_DIAG:    diag_actions  = actions; break;
    case SFI_LOG_DEBUG:   debug_actions = actions; break;
    }
}

void
sfi_log_set_stdlog (gboolean    stdlog_to_stderr_bool,
                    const char *stdlog_filename,
                    guint       syslog_priority)
{
  stdlog_to_stderr = stdlog_to_stderr_bool != 0;
  stdlog_syslog_priority = syslog_priority;
  if (stdlog_file && stdlog_file != stdout)
    fclose (stdlog_file);
  stdlog_file = NULL;
  if (stdlog_filename && strcmp (stdlog_filename, "-") == 0)
    stdlog_file = stdout;
  else if (stdlog_filename)
    stdlog_file = fopen (stdlog_filename, "a");
}

void
sfi_log_set_thread_handler (SfiLogHandler handler)
{
  sfi_thread_set_qdata (quark_log_handler, handler);
}

/**
 * sfi_log_string
 * @log_domain:   log domain
 * @level:        one of %SFI_LOG_ERROR, %SFI_LOG_WARNING, %SFI_LOG_INFO, %SFI_LOG_DIAG or %SFI_LOG_DEBUG
 * @key:          identifier string for the log message type
 * @config_blurb: GUI-usable testblurb about enabling the log message type
 * @string:       the actual log message
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
sfi_log_string (const char     *log_domain,
                unsigned char   level,
                const char     *key,
                const char     *config_blurb,
                const char     *string)
{
  gint saved_errno = errno;
  sfi_log_string_intern (log_domain, level, key, config_blurb, string);
  errno = saved_errno;
}

void
sfi_log_valist (const char     *log_domain,
                unsigned char   level,
                const char     *key,
                const char     *config_blurb,
                const char     *format,
                va_list         args)
{
  gint saved_errno = errno;
  char *message = g_strdup_vprintf (format, args);
  sfi_log_string_intern (log_domain, level, key, config_blurb, message);
  g_free (message);
  errno = saved_errno;
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

static const gchar*
prgname (gboolean maystrip)
{
  const gchar *pname = g_get_prgname();
  if (pname && maystrip)
    {
      const gchar *p = strrchr (pname, '/');
      pname = p ? p + 1 : pname;
    }
  return pname;
}

static void
sfi_log_msg_process (const SfiLogMessage *msgp)
{
  const SfiLogMessage msg = *msgp;
  /* determine log actions */
  const char *slevel;
  guint actions;
  switch (msg.level)
    {
    case SFI_LOG_ERROR:   actions = error_actions;     slevel = "ERROR";   break;
    case SFI_LOG_WARNING: actions = warn_actions;      slevel = "WARNING"; break;
    case SFI_LOG_INFO:    actions = info_actions;      slevel = "INFO";    break;
    case SFI_LOG_DIAG:    actions = diag_actions;      slevel = "DIAG";    break;
    case SFI_LOG_DEBUG:   actions = debug_actions;     slevel = "DEBUG";   break;
    default:              actions = SFI_LOG_TO_STDERR; slevel = NULL;      break;
    }
  /* log to syslog */
  if ((msg.primary || msg.secondary) && stdlog_syslog_priority && (actions & SFI_LOG_TO_STDLOG))
    {
      char *prefix = log_prefix (NULL, 0, slevel ? 0 : msg.level, msg.log_domain, slevel, msg.key);
      if (msg.title && FALSE) // skip title in syslog
        syslog (stdlog_syslog_priority, "%s:0: %s\n", prefix, msg.title);
      if (msg.primary)
        syslog (stdlog_syslog_priority, "%s:1: %s\n", prefix, msg.primary);
      if (msg.secondary)
        syslog (stdlog_syslog_priority, "%s:2: %s\n", prefix, msg.secondary);
      if (msg.details && FALSE) // skip details in syslog
        syslog (stdlog_syslog_priority, "%s:3: %s\n", prefix, msg.details);
      g_free (prefix);
    }
  /* log to stderr */
  gboolean tostderr = (actions & SFI_LOG_TO_STDERR) != 0;
  tostderr |= (actions & SFI_LOG_TO_STDLOG) && stdlog_to_stderr;
  if ((msg.primary || msg.secondary) && tostderr)
    {
      unsigned char print_level = 0;
      const gchar *print_domain = msg.log_domain, *lname = NULL, *log_key = NULL;
      switch (msg.level)
        {
        case SFI_LOG_ERROR:         lname = "ERROR";        break;
        case SFI_LOG_WARNING:       lname = "WARNING";      break;
        case SFI_LOG_INFO:          lname = "INFO";         break;
        case SFI_LOG_DIAG:          log_key = msg.key;      break;
        case SFI_LOG_DEBUG:
          print_domain = NULL;
          lname = "DEBUG";
          log_key = msg.key;
          break;
        default:
          print_level = msg.level;
          log_key = msg.key;
          break;
        }
      gchar *prefix = log_prefix (prgname (msg.level == SFI_LOG_DEBUG), sfi_thread_self_pid(), print_level, print_domain, lname, log_key);
      if (msg.title)
        fprintf (stderr, "%s:0: %s\n", prefix, msg.title);
      if (msg.primary)
        fprintf (stderr, "%s:1: %s\n", prefix, msg.primary);
      if (msg.secondary)
        fprintf (stderr, "%s:2: %s\n", prefix, msg.secondary);
      if (msg.details)
        fprintf (stderr, "%s:3: %s\n", prefix, msg.details);
      g_free (prefix);
    }
  /* log to logfile */
  if (stdlog_file && (actions & SFI_LOG_TO_STDLOG))
    {
      char *prefix = log_prefix (prgname (FALSE), sfi_thread_self_pid(), slevel ? 0 : msg.level, msg.log_domain, slevel, msg.key);
      if (msg.title)
        fprintf (stdlog_file, "%s:0: %s\n", prefix, msg.title);
      if (msg.primary)
        fprintf (stdlog_file, "%s:1: %s\n", prefix, msg.primary);
      if (msg.secondary)
        fprintf (stdlog_file, "%s:2: %s\n", prefix, msg.secondary);
      if (msg.details)
        fprintf (stdlog_file, "%s:3: %s\n", prefix, msg.details);
      g_free (prefix);
    }
  /* log to log handler */
  if (actions & SFI_LOG_TO_HANDLER)
    {
      SfiLogHandler log_handler = sfi_thread_get_qdata (quark_log_handler);
      if (!log_handler)
        log_handler = sfi_log_default_handler;
      log_handler (&msg);
    }
}

static void
sfi_log_string_intern (const char     *log_domain,
                       unsigned char   level,
                       const char     *key,
                       const char     *config_blurb,
                       const char     *string)
{
  /* construct message */
  SfiLogMessage msg = { 0, };
  msg.log_domain = (char*) log_domain;
  msg.level = level;
  msg.key = (char*) key;
  msg.config_check = (char*) config_blurb;
  msg.primary = (char*) string;
  /* handle message */
  sfi_log_msg_process (&msg);
}

static inline char*
log_msg_concat (char       *former,
                const char *next)
{
  if (former && !next)
    return former;
  if (!former && next)
    return g_strdup (next);
  char *result = g_strconcat (former, "\n", next, NULL);
  g_free (former);
  return result;
}

struct SfiLogBit {
  guint type;
  gchar *string;
  SfiLogBit *next;
};

static void
free_lbits (gpointer data)
{
  SfiLogBit *lbit = data;
  while (lbit)
    {
      SfiLogBit *current = lbit;
      lbit = current->next;
      g_free (current->string);
      g_free (current);
    }
}

SfiLogBit*
sfi_log_bit_printf (char            log_bit_type,
                    const char     *format,
                    ...)
{
  gint saved_errno = errno;
  SfiLogBit *lbit = g_new0 (SfiLogBit, 1);
  lbit->type = log_bit_type;
  va_list args;
  va_start (args, format);
  lbit->string = g_strdup_vprintf (format, args);
  va_end (args);
  lbit->next = sfi_thread_steal_qdata (quark_log_bits);
  sfi_thread_set_qdata_full (quark_log_bits, lbit, free_lbits);
  errno = saved_errno;
  return lbit;
}

static void
purge_log_bits (void)
{
  sfi_thread_set_qdata (quark_log_bits, NULL);
}

static inline void
sfi_log_msg_apply_bit (SfiLogMessage   *msg,
                       const SfiLogBit *lbit)
{
  gsize lbs = (gsize) lbit;
  if (lbs < 256)
    msg->level = lbs;
  else switch (lbit->type)
    {
    case 't': msg->title = log_msg_concat (msg->title, lbit->string);               break;
    case '1': msg->primary = log_msg_concat (msg->primary, lbit->string);           break;
    case '2': msg->secondary = log_msg_concat (msg->secondary, lbit->string);       break;
    case '3': msg->details = log_msg_concat (msg->details, lbit->string);           break;
    case 'c': msg->config_check = log_msg_concat (msg->config_check, lbit->string); break;
    }
}

void
sfi_log_msg_valist (const char     *domain,
                    SfiLogBit      *lbit1,
                    SfiLogBit      *lbit2,
                    SfiLogBit      *lbit3,
                    ...)
{
  gint saved_errno = errno;
  /* construct message */
  SfiLogMessage msg = { 0, };
  msg.log_domain = (char*) domain;
  msg.level = SFI_LOG_INFO;
  if (lbit1)
    {
      sfi_log_msg_apply_bit (&msg, lbit1);
      if (lbit2)
        {
          sfi_log_msg_apply_bit (&msg, lbit2);
          if (lbit3)
            {
              const SfiLogBit *lbit = lbit3;
              va_list args;
              va_start (args, lbit3);
              while (lbit)
                {
                  sfi_log_msg_apply_bit (&msg, lbit);
                  lbit = va_arg (args, const SfiLogBit*);
                }
              va_end (args);
            }
        }
    }
  purge_log_bits();
  /* handle message */
  sfi_log_msg_process (&msg);
  /* clean up */
  g_free (msg.title);
  g_free (msg.primary);
  g_free (msg.secondary);
  g_free (msg.details);
  g_free (msg.config_check);
  /* restore errno */
  errno = saved_errno;
}

void
sfi_log_default_handler (const SfiLogMessage *msg)
{
  const gchar *level_name;
  switch (msg->level)
    {
    case SFI_LOG_ERROR:   level_name = "ERROR";      break;
    case SFI_LOG_WARNING: level_name = "WARNING";    break;
    case SFI_LOG_INFO:    level_name = "INFO";       break;
    case SFI_LOG_DIAG:    level_name = "DIAGNOSTIC"; break;
    case SFI_LOG_DEBUG:   level_name = "DEBUG";      break;
    default:              level_name = NULL;         break;
    }
  g_printerr ("********************************************************************************\n");
  char *prefix;
  if (level_name)
    prefix = g_strdup (level_name);
  else if (msg->level >= 32 && msg->level <= 126)
    prefix = g_strdup_printf ("LOG<%c>", msg->level);
  else
    prefix = g_strdup_printf ("LOG<0x%02x>", msg->level);
  g_printerr ("** %s: %s\n", prefix, msg->title ? msg->title : "");
  if (msg->primary)
    g_printerr ("** %s\n", msg->primary);
  if (msg->secondary)
    g_printerr ("** %s\n", msg->secondary);
  if (msg->details)
    g_printerr ("** %s\n", msg->details);
  g_free (prefix);
  g_printerr ("********************************************************************************\n");
}
