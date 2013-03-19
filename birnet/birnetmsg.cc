// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include <glib.h>
#include "birnetmsg.hh"
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#ifndef _ // FIXME
#define _(x)    (x)
#endif
namespace Birnet {
Msg::Part::Part() :
  ptype (0)
{}
void
Msg::Part::setup (uint8       _ptype,
                  String      smsg)
{
  ptype = _ptype;
  string = smsg;
}
void
Msg::Part::setup (uint8       _ptype,
                  const char *format,
                  va_list     varargs)
{
  char *s = g_strdup_vprintf (format, varargs);
  setup (_ptype, String (s));
  g_free (s);
}
const Msg::Part &Msg::empty_part = Part();
Rapicorn::Atomic<int>    Msg::n_msg_types = 0;
Rapicorn::Atomic<uint8*> Msg::msg_type_bits = NULL;
struct MsgType {
  /* this structure cannot use C++ types because it's not properly constructed */
  const char *ident;
  const char *label;
  uint32      flags;
  Msg::Type   default_type;
  bool        enabled;
};
static Rapicorn::Mutex msg_mutex;
static MsgType* msg_types = NULL; /* cannot use a vector<> here, because of constructor ordering */
static bool     msg_log_to_stderr = true;
static uint     msg_syslog_priority = 0; // LOG_USER | LOG_INFO;
static FILE    *msg_log_file = NULL;
void
Msg::set_msg_type_L (uint   mtype,
                     uint32 flags,
                     bool   enabled)
{
  /* adjust and uncouple mtype */
  if (mtype < (uint) n_msg_types)
    {
      msg_types[mtype].flags = flags;
      msg_types[mtype].enabled = enabled;
      if (msg_types[mtype].flags && msg_types[mtype].enabled)
        msg_type_bits[mtype / 8] |= 1 << mtype % 8;
      else
        msg_type_bits[mtype / 8] &= ~(1 << mtype % 8);
      msg_types[mtype].default_type = (Type) mtype; /* uncouple from default_type */
      BIRNET_ASSERT (mtype == (uint) msg_types[mtype].default_type);
    }
  /* adjust all types which default to mtype */
  for (int i = mtype + 1; i < n_msg_types; i++)
    if (mtype == (uint) msg_types[i].default_type)
      {
        msg_types[i].flags = flags;
        msg_types[i].enabled = enabled;
        if (msg_types[i].flags && msg_types[i].enabled)
          msg_type_bits[i / 8] |= 1 << i % 8;
        else
          msg_type_bits[i / 8] &= ~(1 << i % 8);
      }
}
void
Msg::init_standard_types()
{
  static bool initialized = false;
  if (initialized)
    return;
  initialized++;
  Type mtype;
  /* NONE (always disabled) */
  mtype = register_type ("none", NONE, NULL);
  BIRNET_ASSERT (mtype == NONE);
  set_msg_type_L (mtype, NONE, false);
  /* ALWAYS (always enabled) */
  mtype = register_type ("always", ALWAYS, _("Always"));
  BIRNET_ASSERT (mtype == ALWAYS);
  set_msg_type_L (mtype, LOG_TO_STDERR | LOG_TO_STDLOG | LOG_TO_HANDLER, true);
  /* ERROR (enabled) */
  mtype = register_type ("error", ALWAYS, _("Error"));
  BIRNET_ASSERT (mtype == ERROR);
  set_msg_type_L (mtype, LOG_TO_STDERR | LOG_TO_STDLOG | LOG_TO_HANDLER, true);
  /* WARNING (enabled) */
  mtype = register_type ("warning", ALWAYS, _("Warning"));
  BIRNET_ASSERT (mtype == WARNING);
  set_msg_type_L (mtype, LOG_TO_STDERR | LOG_TO_STDLOG | LOG_TO_HANDLER, true);
  /* SCRIPT (enabled) */
  mtype = register_type ("script", ALWAYS, _("Script"));
  BIRNET_ASSERT (mtype == SCRIPT);
  set_msg_type_L (mtype, LOG_TO_STDERR | LOG_TO_STDLOG | LOG_TO_HANDLER, true);
  /* INFO (enabled) */
  mtype = register_type ("info", ALWAYS, _("Information"));
  BIRNET_ASSERT (mtype == INFO);
  set_msg_type_L (mtype, LOG_TO_STDERR | LOG_TO_STDLOG | LOG_TO_HANDLER, true);
  /* DIAG (enabled) */
  mtype = register_type ("diag", ALWAYS, _("Diagnostic"));
  BIRNET_ASSERT (mtype == DIAG);
  set_msg_type_L (mtype, LOG_TO_STDERR | LOG_TO_STDLOG, true);
  /* DEBUG (disabled) */
  mtype = register_type ("debug", NONE, "Debug");
  set_msg_type_L (mtype, LOG_TO_STDERR, false);
}
/**
 * @param ident         message identifier
 * @param default_ouput an existing SfiMsgType
 * @param label         a translated version of @a ident
 * @return              message type id
 *
 * Register a new message type with identifier @a ident and user digestible
 * name @a label. If this function is called multiple times with the same
 * identifier, the type id acquired by the first call will be returned
 * and the other arguments are ignored.
 * As long as the new message type isn't configured individually via
 * msg_enable(), allow_msgs() or their complements, it shares
 * the configuration of @a default_ouput. If NONE or ALWAYS is passed as
 * @a default_ouput, this corresponds to the first two message types which
 * are unconfigrable and always have their output disabled or enabled respectively.
 * As an exception to the rest of the message API, this function may be
 * called before birnet_init(). However note, that MT-safety is only ensured
 * for calls occouring after birnet_init().
 * This function is MT-safe and may be called from any thread.
 */
Msg::Type
Msg::register_type (const char *ident,
                    Type        default_ouput,
                    const char *label)
{
  /* ensure standard types are registered */
  init_standard_types();
  /* check arguments */
  g_return_val_if_fail (ident != NULL, NONE);
  if (default_ouput >= (int) n_msg_types)
    default_ouput = NONE;
  /* lock messages */
  uint8 *old_mbits = NULL;
  msg_mutex.lock();
  /* allow duplicate registration */
  for (int i = 0; i < n_msg_types; i++)
    if (strcmp (msg_types[i].ident, ident) == 0)
      {
        /* found duplicate */
        msg_mutex.unlock();
        return Type (i);
      }
  /* add new message type */
  int n_mtypes = n_msg_types;
  Type mtype = Type (n_mtypes++);
  uint old_flags_size = (mtype + 7) / 8;
  uint new_flags_size = (n_mtypes + 7) / 8;
  if (old_flags_size < new_flags_size)
    {
      uint8 *mflags = g_new (guint8, new_flags_size);
      memcpy (mflags, (uint8*) msg_type_bits, sizeof (msg_type_bits[0]) * old_flags_size);
      mflags[new_flags_size - 1] = 0;
      old_mbits = msg_type_bits;
      /* we are holding a lock in the multi-threaded case so no need for compare_and_swap */
      msg_type_bits = mflags;
    }
  msg_types = g_renew (MsgType, msg_types, n_mtypes);
  memset (&msg_types[mtype], 0, sizeof (msg_types[mtype]));
  msg_types[mtype].ident = g_strdup (ident);
  msg_types[mtype].label = g_strdup (label ? label : "");
  msg_types[mtype].default_type = default_ouput; /* couple to default_ouput */
  n_msg_types = n_mtypes; /* only ever grows */
  /* adjust msg type config (after n_msg_types was incremented) */
  set_msg_type_L (mtype, msg_types[default_ouput].flags, msg_types[default_ouput].enabled);
  // FIXME: msg_type_bits should be registered as hazard pointer so we don't g_free() while other threads read old_mbits[*]
  g_free (old_mbits);
  msg_mutex.unlock();
  return mtype;
}
static struct AutoConstruct {
  AutoConstruct()
  {
    Msg::register_type ("none", Msg::NONE, "");
  }
} auto_construct;
/**
 * @param ident message identifier, e.g. "error", "warning", "info", etc...
 * @return      corresponding Type or 0
 *
 * Find the message type correspondign to @a ident. If no message
 * type was found NONE is returned.
 * This function is MT-safe and may be called from any thread.
 */
Msg::Type
Msg::lookup_type (const String &ident)
{
  Rapicorn::ScopedLock<Rapicorn::Mutex> locker (msg_mutex);
  for (int i = 0; i < n_msg_types; i++)
    if (ident == msg_types[i].ident)
      return Type (i);
  return Msg::NONE;
}
void
Msg::enable (Type mtype)
{
  Rapicorn::ScopedLock<Rapicorn::Mutex> locker (msg_mutex);
  if (mtype > 1 && mtype < (int) n_msg_types)
    set_msg_type_L (mtype, msg_types[mtype].flags, true);
}
void
Msg::disable (Type mtype)
{
  Rapicorn::ScopedLock<Rapicorn::Mutex> locker (msg_mutex);
  if (mtype > 1 && mtype < (int) n_msg_types)
    set_msg_type_L (mtype, msg_types[mtype].flags, false);
}
/**
 * @param type  message type, e.g. Msg::ERROR, Msg::WARNING, Msg::INFO, etc...
 * @return              translated message identifier or NULL
 *
 * Retrive the string identifying the message type @a type. For invalid
 * (non registered) message types, "" is returned.
 * This function is MT-safe and may be called from any thread.
 */
const char*
Msg::type_ident (Type mtype)
{
  Rapicorn::ScopedLock<Rapicorn::Mutex> locker (msg_mutex);
  if (mtype >= 0 && mtype < n_msg_types)
    return msg_types[mtype].ident;
  return NULL;
}
/**
 * @param type  message type, e.g. Msg::ERROR, Msg::WARNING, Msg::INFO, etc...
 * @return              translated message identifier or NULL
 *
 * Retrive the label identifying the message type @a type. Usually,
 * this is a translated version of Msg::type_ident() or ""
 * if non was registered with the message type.
 * This function is MT-safe and may be called from any thread.
 */
const char*
Msg::type_label (Type mtype)
{
  Rapicorn::ScopedLock<Rapicorn::Mutex> locker (msg_mutex);
  if (mtype >= 0 && mtype < n_msg_types)
    return msg_types[mtype].label;
  return NULL;
}
uint32
Msg::type_flags (Type mtype)
{
  Rapicorn::ScopedLock<Rapicorn::Mutex> locker (msg_mutex);
  uint flags = 0;
  if (mtype >= 0 && mtype < n_msg_types)
    flags = msg_types[mtype].flags;
  return flags;
}
void
Msg::configure (Type                mtype,
                LogFlags            log_mask,
                const String       &logfile)
{
  Rapicorn::ScopedLock<Rapicorn::Mutex> locker (msg_mutex);
  if (mtype > 1 && mtype < n_msg_types)
    set_msg_type_L (mtype, log_mask, msg_types[mtype].enabled);
}
void
Msg::key_list_change_L (const String &keylist,
                        bool          isenabled)
{
  /* ensure all keywords are enclosed in ':' */
  String s = ":" + keylist + ":";
  /* allow ',' seperation and collapse spaces */
  uint n = 0;
  for (uint i = 0; s[i]; i++)
    if (s[i] == ',')
      s[n++] = ':';
    else if (s[i] != ' ' && s[i] != '\t' && s[i] != '\n' && s[i] != '\r')
      s[n++] = s[i];
  s[n] = 0;
  /* handle :all: special case */
  if (strstr (s.c_str(), ":all:"))
    {
      for (int i = DEBUG; i < n_msg_types; i++)
        set_msg_type_L (i, msg_types[i].flags, isenabled);
      return;
    }
  /* walk all kyes */
  String::size_type k = 1;
  String::size_type c = s.find (':', k);
  while (c)
    {
      if (k < c)
        {
          s[c] = 0;
          int i;
          for (i = DEBUG; i < n_msg_types; i++)
            if (String (s.c_str() + k) == msg_types[i].ident)
              break;
          if (i < n_msg_types)
            set_msg_type_L (i, msg_types[i].flags, isenabled);
        }
      k = c + 1;
      c = s.find (':', k);
    }
}
void
Msg::allow_msgs (const String &key)
{
  Rapicorn::ScopedLock<Rapicorn::Mutex> locker (msg_mutex);
  if (key.size())
    key_list_change_L (key, true);
#if 0
  for (uint i = 0; i < n_msg_types; i++)
    g_printerr ("% 2d) %s: enabled=%d flags=0x%x label=%s cache=%d\n", i,
                msg_types[i].ident, msg_types[i].enabled,
                msg_types[i].flags, msg_types[i].label,
                check (i));
#endif
}
void
Msg::deny_msgs (const String &key)
{
  Rapicorn::ScopedLock<Rapicorn::Mutex> locker (msg_mutex);
  if (key.size())
    key_list_change_L (key, false);
}
void
Msg::configure_stdlog (bool                redirect_stdlog_to_stderr,
                       const String       &stdlog_filename,
                       uint                syslog_priority)
{
  Rapicorn::ScopedLock<Rapicorn::Mutex> locker (msg_mutex);
  msg_log_to_stderr = redirect_stdlog_to_stderr;
  if (msg_log_file && msg_log_file != stdout && msg_log_file != stderr)
    fclose (msg_log_file);
  msg_log_file = NULL;
  if (stdlog_filename == "-")
    msg_log_file = stdout;
  else if (stdlog_filename.size())
    msg_log_file = fopen (stdlog_filename.c_str(), "a");
  msg_syslog_priority = syslog_priority;
}

static String
prgname (bool maystrip)
{
  const gchar *pname = g_get_prgname();
  if (pname && maystrip)
    {
      const gchar *p = strrchr (pname, '/');
      pname = p ? p + 1 : pname;
    }
  return pname;
}

static String
log_prefix (const String &prg_name,
            uint          pid,
            const String &log_domain,
            const String &label,
            const String &ident)
{
  String str = prg_name;
  if (pid)
    str += string_printf ("[%u]", pid);
  if (str.size() && *str.rbegin() != ':')
    str += ":";
  str += log_domain;
  if (log_domain.size() && label.size())
    str += "-";
  str += label;
  if (ident.size())
    {
      if (log_domain.size() || label.size())
        str += "(" + ident + ")";
      else
        str += ident;
    }
  /* more components can come here */
  /* ... */
  return str;
}

static Rapicorn::DataKey<Msg::Handler> msg_thread_handler_key;

/**
 * @param handler       a valid Msg::Handler or NULL
 *
 * Set the handler function for messages logged in the current
 * thread. If NULL is specified as handler, the standard handler
 * will be used. For handler implementations that require an extra
 * data argument, see Thread::set_data().
 * This function is MT-safe and may be called from any thread.
 */
void
Msg::set_thread_handler (Handler handler)
{
  Rapicorn::ThreadInfo &self = Rapicorn::ThreadInfo::self();
  self.set_data (&msg_thread_handler_key, handler);
}

void
Msg::display_parts (const char         *domain,
                    Type                message_type,
                    const vector<Part> &parts)
{
  int saved_errno = errno;
  String title, primary, secondary, details, checkmsg;
  for (uint i = 0; i < parts.size(); i++)
    switch (parts[i].ptype)
      {
      case '0': title     += (title.size()     ? "\n" : "") + parts[i].string; break;
      case '1': primary   += (primary.size()   ? "\n" : "") + parts[i].string; break;
      case '2': secondary += (secondary.size() ? "\n" : "") + parts[i].string; break;
      case '3': details   += (details.size()   ? "\n" : "") + parts[i].string; break;
      case 'c': checkmsg  += (checkmsg.size()  ? "\n" : "") + parts[i].string; break;
      }
  String ident = type_ident (message_type);
  uint32 actions = type_flags (message_type);
  /* log to stderr */
  bool tostderr = (actions & LOG_TO_STDERR) || (msg_log_to_stderr && (actions & LOG_TO_STDLOG));
  if (tostderr && (primary.size() || secondary.size()))
    {
      bool   is_debug = message_type == DEBUG, is_diag = message_type == DIAG;
      String label = type_label (message_type);
      String prefix = log_prefix (prgname (is_debug),                                   /* strip prgname path for debugging */
                                  Rapicorn::ThisThread::thread_pid(),                   /* always print pid */
                                  is_debug ? "" : domain,                               /* print domain except when debugging */
                                  is_debug || is_diag ? "" : label,                     /* print translated message type execpt for debug/diagnosis */
                                  is_debug ? ident : "");                               /* print identifier if debugging */
      if (title.size())
        fprintf (stderr, "%s:0: %s\n", prefix.c_str(), title.c_str());
      if (primary.size())
        fprintf (stderr, "%s:1: %s\n", prefix.c_str(), primary.c_str());
      if (secondary.size())
        fprintf (stderr, "%s:2: %s\n", prefix.c_str(), secondary.c_str());
      if (details.size())
        fprintf (stderr, "%s:3: %s\n", prefix.c_str(), details.c_str());
      if (0 && checkmsg.size())
        fprintf (stderr, "%s:c: %s\n", prefix.c_str(), checkmsg.c_str());
    }
  /* log to syslog */
  if (msg_syslog_priority && (primary.size() || secondary.size()) && (actions & LOG_TO_STDLOG))
    {
      String prefix = log_prefix ("", 0, domain, "", ident);
      if (title.size() && false) // skip title in syslog
        syslog (msg_syslog_priority, "%s:0: %s\n", prefix.c_str(), title.c_str());
      if (primary.size())
        syslog (msg_syslog_priority, "%s:1: %s\n", prefix.c_str(), primary.c_str());
      if (secondary.size())
        syslog (msg_syslog_priority, "%s:2: %s\n", prefix.c_str(), secondary.c_str());
      if (details.size() && false) // skip details in syslog
        syslog (msg_syslog_priority, "%s:3: %s\n", prefix.c_str(), details.c_str());
    }
  /* log to logfile */
  if (msg_log_file && (actions & LOG_TO_STDLOG))
    {
      String prefix = log_prefix (prgname (false),                                      /* printf fully qualified program name */
                                  Rapicorn::ThisThread::thread_pid(),                   /* always print pid */
                                  domain,                                               /* always print log domain */
                                  "",                                                   /* skip translated message type */
                                  ident);                                               /* print machine readable message type */
      if (title.size())
        fprintf (msg_log_file, "%s:0: %s\n", prefix.c_str(), title.c_str());
      if (primary.size())
        fprintf (msg_log_file, "%s:1: %s\n", prefix.c_str(), primary.c_str());
      if (secondary.size())
        fprintf (msg_log_file, "%s:2: %s\n", prefix.c_str(), secondary.c_str());
      if (details.size())
        fprintf (msg_log_file, "%s:3: %s\n", prefix.c_str(), details.c_str());
    }
  /* log to log handler */
  if (actions & LOG_TO_HANDLER)
    {
      Rapicorn::ThreadInfo &self = Rapicorn::ThreadInfo::self();
      Handler log_handler = self.get_data (&msg_thread_handler_key);
      if (!log_handler)
        log_handler = default_handler;
      log_handler (domain, message_type, parts);
    }
  errno = saved_errno;
}
void
Msg::display_aparts (const char         *log_domain,
                     Type                message_type,
                     const Part &p0, const Part &p1,
                     const Part &p2, const Part &p3,
                     const Part &p4, const Part &p5,
                     const Part &p6, const Part &p7,
                     const Part &p8, const Part &p9)
{
  int saved_errno = errno;
  vector<Part> parts;
  parts.push_back (p0);
  parts.push_back (p1);
  parts.push_back (p2);
  parts.push_back (p3);
  parts.push_back (p4);
  parts.push_back (p5);
  parts.push_back (p6);
  parts.push_back (p7);
  parts.push_back (p8);
  parts.push_back (p9);
  display_parts (log_domain, message_type, parts);
  errno = saved_errno;
}
void
Msg::display_vmsg (const char         *log_domain,
                   Type                message_type,
                   const char         *format,
                   va_list             args)
{
  int saved_errno = errno;
  char *text = g_strdup_vprintf (format, args);
  vector<Part> parts;
  parts.push_back (Primary (String (text)));
  g_free (text);
  display_parts (log_domain, message_type, parts);
  errno = saved_errno;
}
/**
 * @param domain message domain
 * @param parts  message parts
 *
 * This is the standard message handler, it produces @a message
 * in a prominent way on stderr.
 * This function is MT-safe and may be called from any thread.
 */
void
Msg::default_handler (const char         *domain,
                      Type                mtype,
                      const vector<Part> &parts)
{
  String level_name = type_label (mtype);
  String title, primary, secondary, details, checkmsg;
  for (uint i = 0; i < parts.size(); i++)
    switch (parts[i].ptype)
      {
      case '0': title     += (title.size()     ? "\n" : "") + parts[i].string; break;
      case '1': primary   += (primary.size()   ? "\n" : "") + parts[i].string; break;
      case '2': secondary += (secondary.size() ? "\n" : "") + parts[i].string; break;
      case '3': details   += (details.size()   ? "\n" : "") + parts[i].string; break;
      case 'c': checkmsg  += (checkmsg.size()  ? "\n" : "") + parts[i].string; break;
      }
  g_printerr ("********************************************************************************\n");
  if (domain)
    g_printerr ("** %s-%s: %s\n", domain, level_name.c_str(), title.c_str());
  else
    g_printerr ("** %s: %s\n", level_name.c_str(), title.c_str());
  if (primary.size())
    g_printerr ("** %s\n", primary.c_str());
  if (secondary.size())
    {
      String str = secondary;
      for (uint i = 0; i < str.size(); i++)
        if (str[i] == '\n')
          str.insert (i + 1, "**   ");
      g_printerr ("**   %s\n", str.c_str());
    }
  if (details.size())
    {
      String str = details;
      for (uint i = 0; i < str.size(); i++)
        if (str[i] == '\n')
          str.insert (i + 1, "** > ");
      g_printerr ("** > %s\n", str.c_str());
    }
  if (checkmsg.size())
    g_printerr ("** [X] %s\n", checkmsg.c_str());
  g_printerr ("********************************************************************************\n");
}
} // Birnet
