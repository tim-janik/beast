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

#define INVALID_KEYSTRING       ((char*) 256) /* NULL also is a valid keystring */

/* --- variables --- */
static SfiMutex       logging_mutex;
static KeyList        key_list = { 0, 0, 0 };
static const char    *last_enabled_cache = INVALID_KEYSTRING;
static const char    *last_disabled_cache = INVALID_KEYSTRING;
static guint          error_actions = SFI_LOG_TO_STDLOG | SFI_LOG_TO_HANDLER;
static guint          warn_actions  = SFI_LOG_TO_STDLOG | SFI_LOG_TO_HANDLER;
static guint          info_actions  = SFI_LOG_TO_STDLOG | SFI_LOG_TO_HANDLER;
static guint          diag_actions  = SFI_LOG_TO_STDLOG;
static guint          debug_actions = SFI_LOG_TO_STDERR;
static guint          stdlog_syslog_priority = 0; // LOG_USER | LOG_INFO;
static gboolean       stdlog_to_stderr = TRUE;
static FILE          *stdlog_file = NULL;
static GQuark         quark_log_handler = 0;
static GQuark         quark_msg_bits = 0;

/* --- prototypoes --- */
static void     sfi_log_msg_process (const SfiLogMessage *msgp);

/* --- functions --- */
void
_sfi_init_logging (void)
{
  g_assert (quark_log_handler == 0);
  quark_log_handler = g_quark_from_static_string ("SfiLogHandler");
  quark_msg_bits = g_quark_from_static_string ("SfiMsgBit-threadlist");
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
      last_disabled_cache = INVALID_KEYSTRING;
      last_enabled_cache = INVALID_KEYSTRING;
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
      last_disabled_cache = INVALID_KEYSTRING;
      last_enabled_cache = INVALID_KEYSTRING;
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
    case SFI_MSG_ERROR:   error_actions = actions; break;
    case SFI_MSG_WARNING: warn_actions  = actions; break;
    case SFI_MSG_INFO:    info_actions  = actions; break;
    case SFI_MSG_DIAG:    diag_actions  = actions; break;
    case SFI_MSG_DEBUG:   debug_actions = actions; break;
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
 * sfi_log_printf
 * @log_domain:   log domain
 * @level:        one of %SFI_MSG_ERROR, %SFI_MSG_WARNING, %SFI_MSG_INFO, %SFI_MSG_DIAG or %SFI_MSG_DEBUG
 * @key:          identifier string for the log message type
 * @format:       printf-like format
 * @...:          printf-like arguments
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
sfi_log_printf (const char     *log_domain,
                guint           level,
                const char     *key,
                const char     *format,
                ...)
{
  gint saved_errno = errno;
  /* construct message */
  SfiLogMessage msg = { 0, };
  msg.log_domain = (char*) log_domain;
  msg.level = level;
  msg.key = (char*) key;
  msg.config_check = NULL;
  va_list args;
  va_start (args, format);
  msg.primary = g_strdup_vprintf (format, args);
  va_end (args);
  /* handle message */
  sfi_log_msg_process (&msg);
  g_free (msg.primary);
  /* purge thread local msg bit list */
  sfi_thread_set_qdata (quark_msg_bits, NULL);
  errno = saved_errno;
}

/**
 * sfi_log_msg_elist
 * @log_domain:   log domain
 * @level:        one of %SFI_MSG_ERROR, %SFI_MSG_WARNING, %SFI_MSG_INFO, %SFI_MSG_DIAG
 * @lbit1:        msg bit
 * @lbit2:        msg bit
 * @...:          list of more msg bits, NULL terminated
 *
 * Log a message through SFIs logging mechanism. The current value of errno
 * is preserved around calls to this function. Usually this function isn't
 * used directly, but sfi_log_msg() is called instead which does not require
 * %NULL termination of its argument list and automates the @log_domain argument.
 * The @log_domain indicates the calling module and relates to %G_LOG_DOMAIN
 * as used by g_log().
 * The msg bit arguments passed in form various parts of the log message, the
 * following macro set is provided to construct the parts from printf-style
 * argument lists:
 * - SFI_MSG_TITLE(): format message title
 * - SFI_MSG_TEXT1(): format primary message (also SFI_MSG_PRIMARY())
 * - SFI_MSG_TEXT2(): format secondary message, optional (also SFI_MSG_SECONDARY())
 * - SFI_MSG_TEXT3(): format details of the message, optional (also SFI_MSG_DETAIL())
 * - SFI_MSG_CHECK(): format configuration check statement to enable/disable log messages of this type.
 * This function is MT-safe and may be called from any thread.
 */
void
sfi_log_msg_elist (const char     *log_domain,
                   guint           level,       /* SFI_MSG_DEBUG is not really useful here */
                   SfiMsgBit      *lbit1,
                   SfiMsgBit      *lbit2,
                   ...)
{
  gint saved_errno = errno;
  va_list args;
  va_start (args, lbit2);
  sfi_log_msg_trampoline (log_domain, level, lbit1, lbit2, args, sfi_log_msg_process, NULL);
  va_end (args);
  errno = saved_errno;
}

static const gchar*
log_msg_level_classify (guint  level,
                        guint *actionsp)
{
  guint actions;
  const gchar *slevel;
  switch (level)
    {
    case SFI_MSG_ERROR:   actions = error_actions;     slevel = "ERROR";   break;
    case SFI_MSG_WARNING: actions = warn_actions;      slevel = "WARNING"; break;
    case SFI_MSG_INFO:    actions = info_actions;      slevel = "INFO";    break;
    case SFI_MSG_DIAG:    actions = diag_actions;      slevel = "DIAG";    break;
    case SFI_MSG_DEBUG:   actions = debug_actions;     slevel = "DEBUG";   break;
    default:              actions = SFI_LOG_TO_STDERR; slevel = NULL;      break;
    }
  if (actionsp)
    *actionsp = actions;
  return slevel;
}

/**
 * sfi_log_msg_level_name
 * @level:      one of %SFI_MSG_ERROR, %SFI_MSG_WARNING, %SFI_MSG_INFO, %SFI_MSG_DIAG or %SFI_MSG_DEBUG
 * @RETURN:     newly allocated string naming level
 *
 * Construct a string describing the log message level passed as
 * @level. The string is newly allocated and must be freed with g_free().
 * This function is MT-safe and may be called from any thread.
 */
gchar*
sfi_log_msg_level_name (guint level)
{
  const gchar *clevel = log_msg_level_classify (level, NULL);
  gchar *slevel;
  if (clevel)
    slevel = g_strdup (clevel);
  else if (level >= 32 && level <= 126)
    slevel = g_strdup_printf ("LOG<%c>", level);
  else
    slevel = g_strdup_printf ("LOG<0x%02x>", level);
  return slevel;
}

typedef struct LogBit LogBit;
struct LogBit {
  SfiMsgBit bit;
  void    (*data_free) (void*);
  LogBit   *next;
};

static void
free_lbits (LogBit *first)
{
  while (first)
    {
      LogBit *current = first;
      first = current->next;
      if (current->data_free)
        current->data_free (current->bit.data);
      g_free (current);
    }
}

SfiMsgBit*
sfi_msg_bit_appoint (gconstpointer   owner,
                     gpointer        data,
                     void          (*data_free) (gpointer))
{
  gint saved_errno = errno;
  LogBit *lbit = g_new0 (LogBit, 1);
  lbit->bit.owner = owner;
  lbit->bit.data = data;
  lbit->data_free = data_free;
  lbit->next = sfi_thread_steal_qdata (quark_msg_bits);
  sfi_thread_set_qdata_full (quark_msg_bits, lbit, (GDestroyNotify) free_lbits);
  errno = saved_errno;
  return &lbit->bit;
}

SfiMsgBit*
sfi_msg_bit_printf (guint8      log_msg_tag,
                    const char *format,
                    ...)
{
  gint saved_errno = errno;
  va_list args;
  va_start (args, format);
  gchar *string = g_strdup_vprintf (format, args);
  va_end (args);
  errno = saved_errno;
  return sfi_msg_bit_appoint ((void*) (gsize) log_msg_tag, string, g_free);
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

static inline void
sfi_log_msg_apply_bit (SfiLogMessage *msg,
                       SfiMsgBit     *lbit)
{
  gsize ltype = (gsize) lbit->owner;
  if (ltype < 256)
    {
      switch (ltype)
        {
        case '0': msg->title = log_msg_concat (msg->title, lbit->data);               break;
        case '1': msg->primary = log_msg_concat (msg->primary, lbit->data);           break;
        case '2': msg->secondary = log_msg_concat (msg->secondary, lbit->data);       break;
        case '3': msg->details = log_msg_concat (msg->details, lbit->data);           break;
        case 'c': msg->config_check = log_msg_concat (msg->config_check, lbit->data); break;
        }
    }
  else
    {
      guint i = msg->n_msg_bits++;
      msg->msg_bits = g_renew (SfiMsgBit*, msg->msg_bits, msg->n_msg_bits);
      msg->msg_bits[i] = lbit;
    }
}

/**
 * sfi_log_msg_trampoline
 * @log_domain:   log domain
 * @level:        one of %SFI_MSG_ERROR, %SFI_MSG_WARNING, %SFI_MSG_INFO, %SFI_MSG_DIAG
 * @lbit1:        msg bit
 * @lbit2:        msg bit
 * @lbitargs:     va_list list of more msg bits, NULL terminated
 * @handler:      message handler
 * @lbit3:        msg bit
 * @...:          list of more msg bits, NULL terminated
 *
 * Construct a log message from the arguments given and let @handler process
 * it. This function performs no logging on its own, it is used internally by
 * sfi_log_msg_elist() to collect arguments and construct a message. All logging
 * functionality has to be implemented by @handler. Note that all thread-local
 * msg bits are deleted after invokation of this funtcion, so all msg bits
 * created in the current thread are invalid after calling this function.
 * Direct use of this function is not recommended except for implementations
 * of logging mechanisms.
 * This function is MT-safe and may be called from any thread.
 */
void
sfi_log_msg_trampoline (const char     *log_domain,
                        guint           level,
                        SfiMsgBit      *lbit1,
                        SfiMsgBit      *lbit2,
                        va_list         lbitargs,
                        SfiLogHandler   handler,
                        SfiMsgBit      *lbit3,
                        ...)
{
  gint saved_errno = errno;
  g_assert (handler != NULL);
  /* construct message */
  SfiLogMessage msg = { 0, };
  msg.log_domain = (char*) log_domain;
  msg.level = level;
  /* apply msg bits */
  if (lbit1)
    {
      sfi_log_msg_apply_bit (&msg, lbit1);
      SfiMsgBit *lbit = lbit2;
      while (lbit)
        {
          sfi_log_msg_apply_bit (&msg, lbit);
          lbit = va_arg (lbitargs, SfiMsgBit*);
        }
    }
  /* apply extra bits */
  if (lbit3)
    {
      SfiMsgBit *lbit = lbit3;
      va_list args;
      va_start (args, lbit3);
      while (lbit)
        {
          sfi_log_msg_apply_bit (&msg, lbit);
          lbit = va_arg (args, SfiMsgBit*);
        }
      va_end (args);
    }
  /* reset thread local msg bit list */
  LogBit *lbit_list = sfi_thread_steal_qdata (quark_msg_bits);
  /* handle message */
  handler (&msg);
  /* clean up */
  g_free (msg.title);
  g_free (msg.primary);
  g_free (msg.secondary);
  g_free (msg.details);
  g_free (msg.config_check);
  g_free (msg.msg_bits);
  free_lbits (lbit_list);
  /* restore errno */
  errno = saved_errno;
}

void
sfi_log_default_handler (const SfiLogMessage *msg)
{
  gchar *level_name = sfi_log_msg_level_name (msg->level);
  g_printerr ("********************************************************************************\n");
  g_printerr ("** %s: %s\n", level_name, msg->title ? msg->title : "");
  if (msg->primary)
    g_printerr ("** %s\n", msg->primary);
  if (msg->secondary)
    g_printerr ("** %s\n", msg->secondary);
  if (msg->details)
    g_printerr ("** %s\n", msg->details);
  g_printerr ("********************************************************************************\n");
  g_free (level_name);
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

static char*
log_prefix (const char  *prg_name,
            guint        pid,
            guint        level,
            const char  *log_domain,
            const char  *postfix,
            const char  *key)
{
  GString *gstring = g_string_new (prg_name);
  if (pid)
    g_string_append_printf (gstring, "[%u]", pid);
  if (gstring->len)
    g_string_append (gstring, ":");
  if (level >= 32 && level <= 126)
    g_string_append_printf (gstring, "LOG<%c>:", level);
  else if (level)
    g_string_append_printf (gstring, "LOG<0x%02x>:", level);
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
  /* more components can come here */
  /* ... */
  if (gstring->str[gstring->len - 1] == ':') /* strip final ':' */
    gstring->str[gstring->len - 1] = 0;
  return g_string_free (gstring, FALSE);
}

static void
sfi_log_msg_process (const SfiLogMessage *msgp)
{
  const SfiLogMessage msg = *msgp;
  /* determine log actions */
  guint actions = 0;
  const char *slevel = log_msg_level_classify (msg.level, &actions);
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
      gchar *prefix = log_prefix (prgname (msg.level == SFI_MSG_DEBUG),                 /* strip prgname path for debugging */
                                  sfi_thread_self_pid(),                                /* always print pid */
                                  slevel ? 0 : msg.level,                               /* print numeric level for custom levels */
                                  msg.level == SFI_MSG_DEBUG ? NULL : msg.log_domain,   /* print domain except when debugging */
                                  msg.level == SFI_MSG_DIAG ? NULL : slevel,            /* skip domain postfix for diagnosis */
                                  msg.key);                                             /* print debugging key */
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
