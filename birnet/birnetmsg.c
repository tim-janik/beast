/* BirnetMsg
 * Copyright (C) 2002-2006 Tim Janik
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
#include "birnetmsg.h"
#include "birnetthread.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>

#ifndef _ // FIXME
#define _(x) x
#endif

typedef struct {
  gchar            *ident;
  gchar            *label;
  BirnetMsgType     default_type;
  BirnetMsgLogFlags log_flags : 16;
  guint             disabled : 1;
} MsgType;


/* --- variables --- */
static BirnetMutex       logging_mutex;
static GQuark            quark_log_handler = 0;
static GQuark            quark_msg_bits = 0;
static guint             n_msg_types = 0;
static MsgType          *msg_types = NULL;
guint8 * volatile        birnet_msg_flags = NULL;
volatile guint           birnet_msg_flags_max = 0;
static guint             stdlog_syslog_priority = 0; // LOG_USER | LOG_INFO;
static bool              stdlog_to_stderr = TRUE;
static FILE             *stdlog_file = NULL;

/* --- prototypoes --- */
static void     birnet_log_msg_process     (const BirnetMessage *msgp);
static void     birnet_msg_type_set        (BirnetMsgType     mtype,
                                            BirnetMsgLogFlags log_flags,
                                            bool           enabled);

/* --- functions --- */
static void
birnet_msg_type_init_internals (void)
{
  static volatile guint initialized = FALSE;
  if (initialized || !birnet_atomic_uint_compare_and_swap (&initialized, FALSE, TRUE))
    return;
  guint mtype;
  /* BIRNET_MSG_NONE (always disabled) */
  mtype = birnet_msg_type_register ("none", 0, NULL);
  g_assert (mtype == BIRNET_MSG_NONE);
  birnet_msg_type_set (BIRNET_MSG_NONE, 0, FALSE);
  /* BIRNET_MSG_FATAL (always enabled) */
  mtype = birnet_msg_type_register ("fatal", 1, _("Fatal Error"));
  g_assert (mtype == BIRNET_MSG_FATAL);
  birnet_msg_type_set (BIRNET_MSG_FATAL, BIRNET_MSG_TO_STDERR | BIRNET_MSG_TO_STDLOG | BIRNET_MSG_TO_HANDLER, TRUE);
  /* BIRNET_MSG_ERROR (enabled) */
  mtype = birnet_msg_type_register ("error", 1, _("Error"));
  g_assert (mtype == BIRNET_MSG_ERROR);
  birnet_msg_type_set (BIRNET_MSG_ERROR, BIRNET_MSG_TO_STDERR | BIRNET_MSG_TO_STDLOG | BIRNET_MSG_TO_HANDLER, TRUE);
  /* BIRNET_MSG_WARNING (enabled) */
  mtype = birnet_msg_type_register ("warning", 1, _("Warning"));
  g_assert (mtype == BIRNET_MSG_WARNING);
  birnet_msg_type_set (BIRNET_MSG_WARNING, BIRNET_MSG_TO_STDERR | BIRNET_MSG_TO_STDLOG | BIRNET_MSG_TO_HANDLER, TRUE);
  /* BIRNET_MSG_SCRIPT (enabled) */
  mtype = birnet_msg_type_register ("script", 1, _("Script"));
  g_assert (mtype == BIRNET_MSG_SCRIPT);
  birnet_msg_type_set (BIRNET_MSG_SCRIPT, BIRNET_MSG_TO_STDERR | BIRNET_MSG_TO_STDLOG | BIRNET_MSG_TO_HANDLER, TRUE);
  /* BIRNET_MSG_INFO (enabled) */
  mtype = birnet_msg_type_register ("info", 1, _("Information"));
  g_assert (mtype == BIRNET_MSG_INFO);
  birnet_msg_type_set (BIRNET_MSG_INFO, BIRNET_MSG_TO_STDERR | BIRNET_MSG_TO_STDLOG | BIRNET_MSG_TO_HANDLER, TRUE);
  /* BIRNET_MSG_DIAG (enabled) */
  mtype = birnet_msg_type_register ("diag", 1, _("Diagnostic"));
  g_assert (mtype == BIRNET_MSG_DIAG);
  birnet_msg_type_set (BIRNET_MSG_DIAG, BIRNET_MSG_TO_STDERR | BIRNET_MSG_TO_STDLOG, TRUE);
  /* BIRNET_MSG_DEBUG (disabled) */
  mtype = birnet_msg_type_register ("debug", 0, "Debug");
  g_assert (mtype == BIRNET_MSG_DEBUG);
  birnet_msg_type_set (BIRNET_MSG_DEBUG, BIRNET_MSG_TO_STDERR, FALSE);
}

void
_birnet_init_logging (void)
{
  g_assert (quark_log_handler == 0);
  quark_log_handler = g_quark_from_static_string ("BirnetMsgHandler");
  quark_msg_bits = g_quark_from_static_string ("BirnetMsgBit-threadlist");
  birnet_mutex_init (&logging_mutex);
  birnet_msg_type_init_internals();
}

static inline void
msg_type_set_intern (BirnetMsgType     mtype,
                     BirnetMsgLogFlags log_flags,
                     bool              enabled,
                     bool              uncouple_default)
{
  if (mtype < n_msg_types)
    {
      msg_types[mtype].log_flags = log_flags;
      msg_types[mtype].disabled = !enabled;
      enabled = msg_types[mtype].log_flags && !msg_types[mtype].disabled;
      if (enabled)
        birnet_msg_flags[mtype / 8] |= 1 << mtype % 8;
      else
        birnet_msg_flags[mtype / 8] &= ~(1 << mtype % 8);
      if (uncouple_default)
        msg_types[mtype].default_type = mtype;
    }
}

static void
birnet_msg_type_set (BirnetMsgType     mtype,
                     BirnetMsgLogFlags log_flags,
                     bool              enabled)
{
  msg_type_set_intern (mtype, log_flags, enabled, TRUE);
  guint i;
  for (i = mtype + 1; i < n_msg_types; i++)
    if (msg_types[i].default_type == mtype)
      msg_type_set_intern (mtype, log_flags, enabled, FALSE);
}

/**
 * @param ident	message identifier
 * @param default_ouput	an existing BirnetMsgType or FALSE or TRUE
 * @param label	a translated version of @a ident
 * @return		message type id
 *
 * Register a new message type with identifier @a ident and user digestible
 * name @a label. If this function is called multiple times with the same
 * identifier, the type id acquired by the first call will be returned
 * and the other arguments are ignored.
 * As long as the new message type isn't configured individually via
 * birnet_msg_enable(), birnet_msg_allow() or their complements, it shares
 * the configuration of @a default_ouput. If FALSE or TRUE is passed as
 * @a default_ouput, this corresponds to BIRNET_MSG_NONE or BIRNET_MSG_FATAL
 * respectively which are unconfigrable and always have their output
 * disabled or enabled respectively.
 * As an exception to the rest of the message API, this function may be
 * called before birnet_init(). However note, that MT-safety is only ensured
 * for calls occouring after birnet_init().
 * This function is MT-safe and may be called from any thread.
 */
BirnetMsgType
birnet_msg_type_register (const gchar   *ident,
                          BirnetMsgType  default_ouput, /* FALSE, TRUE, BIRNET_MSG_* */
                          const gchar   *label)
{
  /* ensure standard types are registered */
  birnet_msg_type_init_internals();
  /* check arguments */
  g_return_val_if_fail (ident != NULL, 0);
  if (default_ouput >= n_msg_types)
    default_ouput = 0;
  /* support concurrency after _birnet_init_logging() */
  bool     need_unlock = FALSE;
  if (quark_log_handler)
    {
      birnet_mutex_lock (&logging_mutex);
      need_unlock = TRUE;
    }
  /* allow duplicate registration */
  guint i;
  for (i = BIRNET_MSG_LAST; i < n_msg_types; i++)
    if (strcmp (ident, msg_types[i].ident) == 0)
      {
        /* found duplicate */
        if (need_unlock)
          birnet_mutex_unlock (&logging_mutex);
        return i;
      }
  /* add new message type */
  guint mtype = n_msg_types++;
  msg_types = g_renew (MsgType, msg_types, n_msg_types);
  memset (&msg_types[mtype], 0, sizeof (msg_types[mtype]));
  guint old_flags_size = (mtype + 7) / 8;
  guint new_flags_size = (n_msg_types + 7) / 8;
  if (old_flags_size < new_flags_size)
    {
      guint8 *msg_flags = g_new (guint8, new_flags_size);
      memcpy (msg_flags, (guint8*) birnet_msg_flags, sizeof (msg_flags[0]) * old_flags_size);
      msg_flags[new_flags_size - 1] = 0;
      guint8 *old_msg_flags = birnet_msg_flags;
      /* we are holding a lock in the multi-threaded case so no need for compare_and_swap */
      typedef guint8* X;
      birnet_atomic_pointer_set ((void*) &birnet_msg_flags, msg_flags);
      // FIXME: birnet_msg_flags should be registered as hazard pointer so we don't g_free() while other threads read old_msg_flags[*]
      g_free (old_msg_flags);
    }
  msg_types[mtype].ident = g_strdup (ident);
  msg_types[mtype].label = g_strdup (label);
  birnet_msg_type_set (mtype, msg_types[default_ouput].log_flags, !msg_types[default_ouput].disabled);
  msg_types[mtype].default_type = default_ouput;
  birnet_atomic_uint_set (&birnet_msg_flags_max, mtype); /* only ever grows */
  /* out of here */
  if (need_unlock)
    birnet_mutex_unlock (&logging_mutex);
  return mtype;
}

static void
key_list_change (const char *string,
                 bool        flag_value)
{
  guint i, n;
  /* ensure all keywords are enclosed in ':' */
  char *s = g_strconcat (":", string, ":", NULL);
  /* allow ',' seperation and collapse spaces */
  for (i = 0, n = 0; s[i]; i++)
    if (s[i] == ',')
      s[n++] = ':';
    else if (s[i] != ' ' && s[i] != '\t' && s[i] != '\n' && s[i] != '\r')
      s[n++] = s[i];
  s[n] = 0;
  /* handle :all: special case */
  if (strstr (s, ":all:"))
    {
      g_free (s);
      for (i = BIRNET_MSG_DEBUG; i < n_msg_types; i++)
        birnet_msg_type_set (i, msg_types[i].log_flags, flag_value);
      return;
    }
  
  /* walk all kyes */
  char *k = s + 1;
  char *p = strchr (k, ':');
  while (p)
    {
      if (k < p)
	{
	  *p = 0;
          for (i = BIRNET_MSG_DEBUG; i < n_msg_types; i++)
            if (strcmp (k, msg_types[i].ident) == 0)
              break;
          if (i < n_msg_types)
            birnet_msg_type_set (i, msg_types[i].log_flags, flag_value);
	}
      k = p + 1;
      p = strchr (k, ':');
    }
  g_free (s);
}

void
birnet_msg_allow (const char *key)
{
  birnet_mutex_lock (&logging_mutex);
  if (key)
    key_list_change (key, TRUE);
  birnet_mutex_unlock (&logging_mutex);
  
#if 0
  guint i;
  for (i = 0; i < n_msg_types; i++)
    g_printerr ("% 2d) %s: disabled=%d log_flags=0x%x label=%s cache=%d\n", i,
                msg_types[i].ident, msg_types[i].disabled,
                msg_types[i].log_flags, msg_types[i].label,
                birnet_msg_check (i));
#endif
}

void
birnet_msg_deny (const char *key)
{
  birnet_mutex_lock (&logging_mutex);
  if (key)
    key_list_change (key, FALSE);
  birnet_mutex_unlock (&logging_mutex);
}

void
birnet_msg_enable (BirnetMsgType mtype)
{
  birnet_mutex_lock (&logging_mutex);
  if (mtype > 1 && mtype < n_msg_types)
    birnet_msg_type_set (mtype, msg_types[mtype].log_flags, TRUE);
  birnet_mutex_unlock (&logging_mutex);
}

void
birnet_msg_disable (BirnetMsgType mtype)
{
  birnet_mutex_lock (&logging_mutex);
  if (mtype > 1 && mtype < n_msg_types)
    birnet_msg_type_set (mtype, msg_types[mtype].log_flags, FALSE);
  birnet_mutex_unlock (&logging_mutex);
}

void
birnet_msg_type_configure (BirnetMsgType        mtype,
                           BirnetMsgLogFlags    channel_mask,
                           const gchar         *dummy_filename)
{
  birnet_mutex_lock (&logging_mutex);
  if (mtype > 1 && mtype < n_msg_types)
    birnet_msg_type_set (mtype, channel_mask, !msg_types[mtype].disabled);
  birnet_mutex_unlock (&logging_mutex);
}

void
birnet_msg_configure_stdlog (bool              stdlog_to_stderr_bool,
                             const char       *stdlog_filename,
                             guint             syslog_priority) /* if != 0, stdlog to syslog */
{
  birnet_mutex_lock (&logging_mutex);
  stdlog_to_stderr = stdlog_to_stderr_bool != 0;
  stdlog_syslog_priority = syslog_priority;
  if (stdlog_file && stdlog_file != stdout)
    fclose (stdlog_file);
  stdlog_file = NULL;
  if (stdlog_filename && strcmp (stdlog_filename, "-") == 0)
    stdlog_file = stdout;
  else if (stdlog_filename)
    stdlog_file = fopen (stdlog_filename, "a");
  birnet_mutex_unlock (&logging_mutex);
}

/**
 * @param type	message type, e.g. BIRNET_MSG_ERROR, BIRNET_MSG_WARNING, BIRNET_MSG_INFO, etc...
 * @return		message identifier
 *
 * Retrive the string identifying the message type @a type. For invalid
 * (non registered) message types, NULL is returned.
 * This function is MT-safe and may be called from any thread.
 */
const gchar*
birnet_msg_type_ident (BirnetMsgType mtype)
{
  const gchar *string = NULL;
  birnet_mutex_lock (&logging_mutex);
  if (mtype >= 0 && mtype < n_msg_types)
    string = msg_types[mtype].ident;
  birnet_mutex_unlock (&logging_mutex);
  return string;
}

/**
 * @param type	message type, e.g. BIRNET_MSG_ERROR, BIRNET_MSG_WARNING, BIRNET_MSG_INFO, etc...
 * @return		translated message identifier or NULL
 *
 * Retrive the label identifying the message type @a type. Usually,
 * this is a translated version of birnet_msg_type_ident() or NULL
 * if non was registered with the message type.
 * This function is MT-safe and may be called from any thread.
 */
const gchar*
birnet_msg_type_label (BirnetMsgType mtype)
{
  const gchar *string = NULL;
  birnet_mutex_lock (&logging_mutex);
  if (mtype >= 0 && mtype < n_msg_types)
    string = msg_types[mtype].label;
  birnet_mutex_unlock (&logging_mutex);
  return string;
}

/**
 * @param ident	message identifier, e.g. "error", "warning", "info", etc...
 * @return		corresponding BirnetMsgType or 0
 *
 * Find the message type correspondign to @a ident. If no message
 * type was found 0 is returned (note that 0 is also the value of
 * BIRNET_MSG_NONE).
 * This function is MT-safe and may be called from any thread.
 */
BirnetMsgType
birnet_msg_type_lookup (const gchar *ident)
{
  g_return_val_if_fail (ident != NULL, 0);
  guint i;
  birnet_mutex_lock (&logging_mutex);
  for (i = 0; i < n_msg_types; i++)
    if (strcmp (ident, msg_types[i].ident) == 0)
      break;
  if (i >= n_msg_types)
    i = 0;
  birnet_mutex_unlock (&logging_mutex);
  return i;
}

/**
 * @param handler	a valid BirnetMsgHandler or NULL
 *
 * Set the handler function for messages logged in the current
 * thread. If NULL is specified as handler, the standard handler
 * will be used. For handler implementations that require an extra
 * data argument, see birnet_thread_set_qdata().
 * This function is MT-safe and may be called from any thread.
 */
void
birnet_msg_set_thread_handler (BirnetMsgHandler handler)
{
  birnet_thread_set_qdata (quark_log_handler, handler);
}

/**
 * @param message	a valid BirnetMessage
 *
 * This is the standard message handler, it produces @a message
 * in a prominent way on stderr.
 * This function is MT-safe and may be called from any thread.
 */
void
birnet_msg_default_handler (const BirnetMessage *msg)
{
  const gchar *level_name = birnet_msg_type_label (msg->type);
  g_printerr ("********************************************************************************\n");
  if (msg->log_domain)
    g_printerr ("** %s-%s: %s\n", msg->log_domain, level_name, msg->title ? msg->title : "");
  else
    g_printerr ("** %s: %s\n", level_name, msg->title ? msg->title : "");
  if (msg->primary)
    g_printerr ("** %s\n", msg->primary);
  if (msg->secondary)
    {
      GString *gstring = g_string_new (msg->secondary);
      guint i;
      for (i = 0; i < gstring->len; i++)
        if (gstring->str[i] == '\n')
          g_string_insert (gstring, i + 1, "**   ");
      g_printerr ("**   %s\n", gstring->str);
      g_string_free (gstring, TRUE);
    }
  if (msg->details)
    {
      GString *gstring = g_string_new (msg->details);
      guint i;
      for (i = 0; i < gstring->len; i++)
        if (gstring->str[i] == '\n')
          g_string_insert (gstring, i + 1, "** > ");
      g_printerr ("** > %s\n", gstring->str);
      g_string_free (gstring, TRUE);
    }
  if (msg->config_check)
    g_printerr ("** [X] %s\n", msg->config_check);
  g_printerr ("********************************************************************************\n");
}

typedef struct LogBit LogBit;
struct LogBit {
  BirnetMsgBit bit;
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

/**
 * @param log_domain    log domain
 * @param level         one of BIRNET_MSG_ERROR, BIRNET_MSG_WARNING, BIRNET_MSG_INFO, BIRNET_MSG_DIAG or BIRNET_MSG_DEBUG
 * @param format        printf-like format
 * @param ...           printf-like arguments
 *
 * Log a message through BIRNETs logging mechanism. The current
 * value of errno is preserved around calls to this function.
 * Usually this function isn't used directly, but through one
 * of birnet_debug(), birnet_diag(), birnet_info(), birnet_warn() or birnet_error().
 * The @a log_domain indicates the calling module and relates to
 * G_LOG_DOMAIN as used by g_log().
 * This function is MT-safe and may be called from any thread.
 */
void
birnet_msg_log_printf (const char       *log_domain,
                       BirnetMsgType     mtype,
                       const char       *format,
                       ...)
{
  gint saved_errno = errno;
  /* construct message */
  BirnetMessage msg = { 0, };
  msg.type = mtype;
  msg.log_domain = (char*) log_domain;
  va_list args;
  va_start (args, format);
  msg.primary = g_strdup_vprintf (format, args);
  va_end (args);
  msg.config_check = NULL;
  /* handle message */
  LogBit *lbit_list = birnet_thread_steal_qdata (quark_msg_bits);
  birnet_log_msg_process (&msg);
  g_free (msg.primary);
  free_lbits (lbit_list); /* purge thread local msg bit list */
  errno = saved_errno;
}

/**
 * @param log_domain    log domain
 * @param mtype         one of BIRNET_MSG_ERROR, BIRNET_MSG_WARNING, BIRNET_MSG_INFO, BIRNET_MSG_DIAG
 * @param lbit1         msg bit
 * @param lbit2         msg bit
 * @param ...           list of more msg bits, NULL terminated
 *
 * Log a message through BIRNETs logging mechanism. The current value of errno
 * is preserved around calls to this function. Usually this function isn't
 * used directly, but birnet_log_msg() is called instead which does not require
 * NULL termination of its argument list and automates the @a log_domain argument.
 * The @a log_domain indicates the calling module and relates to G_LOG_DOMAIN
 * as used by g_log().
 * The msg bit arguments passed in form various parts of the log message, the
 * following macro set is provided to construct the parts from printf-style
 * argument lists:
 * - BIRNET_MSG_TITLE(): format message title
 * - BIRNET_MSG_TEXT1(): format primary message (also BIRNET_MSG_PRIMARY())
 * - BIRNET_MSG_TEXT2(): format secondary message, optional (also BIRNET_MSG_SECONDARY())
 * - BIRNET_MSG_TEXT3(): format details of the message, optional (also BIRNET_MSG_DETAIL())
 * - BIRNET_MSG_CHECK(): format configuration check statement to enable/disable log messages of this type.
 * This function is MT-safe and may be called from any thread.
 */
void
birnet_msg_log_elist (const char     *log_domain,
                      BirnetMsgType   mtype,       /* BIRNET_MSG_DEBUG is not really useful here */
                      BirnetMsgBit   *lbit1,
                      BirnetMsgBit   *lbit2,
                      ...)
{
  gint saved_errno = errno;
  guint n = 0;
  BirnetMsgBit **bits = NULL;
  /* collect msg bits */
  if (lbit1)
    {
      bits = g_renew (BirnetMsgBit*, bits, n + 1);
      bits[n++] = lbit1;
      BirnetMsgBit *lbit = lbit2;
      va_list args;
      va_start (args, lbit2);
      while (lbit)
        {
          bits = g_renew (BirnetMsgBit*, bits, n + 1);
          bits[n++] = lbit;
          lbit = va_arg (args, BirnetMsgBit*);
        }
      va_end (args);
    }
  bits = g_renew (BirnetMsgBit*, bits, n + 1);
  bits[n] = NULL;
  birnet_msg_log_trampoline (log_domain, mtype, bits, birnet_log_msg_process);
  g_free (bits);
  errno = saved_errno;
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
msg_apply_bit (BirnetMessage *msg,
               BirnetMsgBit  *lbit)
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
      msg->msg_bits = g_renew (BirnetMsgBit*, msg->msg_bits, msg->n_msg_bits);
      msg->msg_bits[i] = lbit;
    }
}

/**
 * @param log_domain	log domain
 * @param mtype	one of BIRNET_MSG_ERROR, BIRNET_MSG_WARNING, BIRNET_MSG_INFO, BIRNET_MSG_DIAG
 * @param lbit1	msg bit
 * @param lbit2	msg bit
 * @param lbitargs	va_list list of more msg bits, NULL terminated
 * @param handler	message handler
 * @param vbitlist	NULL terminated array of msg bits
 *
 * Construct a log message from the arguments given and let @a handler process
 * it. This function performs no logging on its own, it is used internally by
 * birnet_log_msg_elist() to collect arguments and construct a message. All logging
 * functionality has to be implemented by @a handler. Note that all thread-local
 * msg bits are deleted after invokation of this funtcion, so all msg bits
 * created in the current thread are invalid after calling this function.
 * Direct use of this function is not recommended except for implementations
 * of logging mechanisms.
 * This function is MT-safe and may be called from any thread.
 */
void
birnet_msg_log_trampoline (const char        *log_domain,
                           BirnetMsgType      mtype,       /* BIRNET_MSG_DEBUG is not really useful here */
                           BirnetMsgBit     **lbits,
                           BirnetMsgHandler   handler)
{
  gint saved_errno = errno;
  /* construct message */
  BirnetMessage msg = { 0, };
  msg.type = mtype;
  msg.log_domain = (char*) log_domain;
  /* apply msg bits */
  if (lbits)
    {
      guint j;
      for (j = 0; lbits[j]; j++)
        msg_apply_bit (&msg, lbits[j]);
    }
  /* reset thread local msg bit list */
  LogBit *lbit_list = birnet_thread_steal_qdata (quark_msg_bits);
  /* handle message */
  if (!handler)
    handler = birnet_log_msg_process;
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

BirnetMsgBit*
birnet_msg_bit_appoint (gconstpointer   owner,
                        gpointer        data,
                        void          (*data_free) (gpointer))
{
  gint saved_errno = errno;
  LogBit *lbit = g_new0 (LogBit, 1);
  lbit->bit.owner = owner;
  lbit->bit.data = data;
  lbit->data_free = data_free;
  lbit->next = birnet_thread_steal_qdata (quark_msg_bits);
  birnet_thread_set_qdata_full (quark_msg_bits, lbit, (GDestroyNotify) free_lbits);
  errno = saved_errno;
  return &lbit->bit;
}

BirnetMsgBit*
birnet_msg_bit_printf (guint8      msg_bit_type,
                       const char *format,
                       ...)
{
  gint saved_errno = errno;
  va_list args;
  va_start (args, format);
  gchar *string = g_strdup_vprintf (format, args);
  va_end (args);
  errno = saved_errno;
  return birnet_msg_bit_appoint ((void*) (gsize) msg_bit_type, string, g_free);
}

static const gchar*
prgname (bool     maystrip)
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
            const char  *log_domain,
            const char  *label,
            const char  *ident)
{
  GString *gstring = g_string_new (prg_name);
  if (pid)
    g_string_append_printf (gstring, "[%u]", pid);
  if (gstring->len)
    g_string_append (gstring, ":");
  if (log_domain)
    g_string_append (gstring, log_domain);
  if (log_domain && label)
    g_string_append (gstring, "-");
  if (label)
    g_string_append (gstring, label);
  if (ident)
    {
      if (log_domain || label)
        g_string_append_printf (gstring, "(%s)", ident);
      else
        g_string_append (gstring, ident);
    }
  if (log_domain || label || ident)
    g_string_append (gstring, ":");
  /* more components can come here */
  /* ... */
  if (gstring->str[gstring->len - 1] == ':') /* strip final ':' */
    gstring->str[gstring->len - 1] = 0;
  return g_string_free (gstring, FALSE);
}

static guint
birnet_msg_type_actions (BirnetMsgType mtype)
{
  guint actions = 0;
  birnet_mutex_lock (&logging_mutex);
  if (mtype >= 0 && mtype < n_msg_types &&
      !msg_types[mtype].disabled)
    actions = msg_types[mtype].log_flags;
  birnet_mutex_unlock (&logging_mutex);
  return actions;
}

static void
birnet_log_msg_process (const BirnetMessage *msgp)
{
  const BirnetMessage msg = *msgp;
  /* determine log actions */
  guint actions = birnet_msg_type_actions (msg.type);
  const char *ident = birnet_msg_type_ident (msg.type);
  const char *label = birnet_msg_type_label (msg.type);
  /* log to syslog */
  if ((msg.primary || msg.secondary) && stdlog_syslog_priority && (actions & BIRNET_MSG_TO_STDLOG))
    {
      char *prefix = log_prefix (NULL, 0, msg.log_domain, NULL, ident);
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
  bool tostderr = (actions & BIRNET_MSG_TO_STDERR) != 0;
  tostderr |= (actions & BIRNET_MSG_TO_STDLOG) && stdlog_to_stderr;
  if ((msg.primary || msg.secondary) && tostderr)
    {
      bool   is_debug = msg.type == BIRNET_MSG_DEBUG, is_diag = msg.type == BIRNET_MSG_DIAG;
      gchar *prefix = log_prefix (prgname (is_debug),                                   /* strip prgname path for debugging */
                                  birnet_thread_self_pid(),                                /* always print pid */
                                  is_debug ? NULL : msg.log_domain,                     /* print domain except when debugging */
                                  is_debug || is_diag ? NULL : label,                   /* print translated message type execpt for debug/diagnosis */
                                  is_debug ? ident : NULL);                             /* print identifier if debugging */
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
  if (stdlog_file && (actions & BIRNET_MSG_TO_STDLOG))
    {
      char *prefix = log_prefix (prgname (FALSE),                                       /* printf fully qualified program name */
                                 birnet_thread_self_pid(),                                 /* always print pid */
                                 msg.log_domain,                                        /* always print log domain */
                                 NULL,                                                  /* skip translated message type */
                                 ident);                                                /* print machine readable message type */
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
  if (actions & BIRNET_MSG_TO_HANDLER)
    {
      BirnetMsgHandler log_handler = birnet_thread_get_qdata (quark_log_handler);
      if (!log_handler)
        log_handler = birnet_msg_default_handler;
      log_handler (&msg);
    }
}
