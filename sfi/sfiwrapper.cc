// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "sfiwrapper.hh"
#include <birnet/birnet.hh>
#include <errno.h>

namespace Bse {

/// Caching flag to inhibit useless bse_debug() calls.
bool volatile _cached_bse_debug = true;

/// Issue a debugging message, configurable via #$BSE_DEBUG.
void
bse_debug (const char *key, const char *file_path, const int line, const char *format, ...)
{
  va_list vargs;
  va_start (vargs, format);
  envkey_debug_message ("BSE_DEBUG", key, file_path, line, format, vargs, &_cached_rapicorn_debug);
  va_end (vargs);
}

#ifdef DOXYGEN
/** Check if debugging is enabled for @a key.
 * This function checks if #$BSE_DEBUG contains @a key or "all" and returns true
 * if debugging is enabled for the given key. The @a key argument may be NULL in which
 * case the function checks if general debugging is enabled.
 */
bool bse_debug_enabled (const char *key);
#endif // DOXYGEN

bool
_bse_debug_enabled (const char *key)
{
  return envkey_debug_check ("BSE_DEBUG", key, &_cached_bse_debug);
}

/// Check if the feature toggle @a key is enabled in #$BSE_FLIPPER.
bool
bse_flipper_check (const char *key)
{
  return envkey_flipper_check ("BSE_FLIPPER", key);
}

} // Bse

/* --- initialization --- */
void
sfi_init (int            *argcp,
          char         ***argvp,
          const char     *app_name,
          SfiInitValue    sivalues[])
{
  BIRNET_STATIC_ASSERT (sizeof (SfiInitValue) == sizeof (BirnetInitValue));
  BIRNET_STATIC_ASSERT (offsetof (SfiInitValue, value_name) == offsetof (BirnetInitValue, value_name));
  BIRNET_STATIC_ASSERT (offsetof (SfiInitValue, value_string) == offsetof (BirnetInitValue, value_string));
  BIRNET_STATIC_ASSERT (offsetof (SfiInitValue, value_num) == offsetof (BirnetInitValue, value_num));
  Birnet::birnet_init (argcp, argvp, app_name, (BirnetInitValue*) sivalues);
}
bool
sfi_init_value_bool (SfiInitValue *value)
{
  return Birnet::init_value_bool ((BirnetInitValue*) value);
}
double
sfi_init_value_double (SfiInitValue *value)
{
  return Birnet::init_value_double ((BirnetInitValue*) value);
}
gint64
sfi_init_value_int (SfiInitValue *value)
{
  return Birnet::init_value_int ((BirnetInitValue*) value);
}
SfiInitSettings
sfi_init_settings (void)
{
  return ::Birnet::init_settings();
}
/* --- file testing --- */
bool
birnet_file_check (const char *file,
                   const char *mode)
{
  return Birnet::Path::check (file ? file : "", mode ? mode : "");
}
bool
birnet_file_equals (const char *file1,
                    const char *file2)
{
  return Birnet::Path::equals (file1 ? file1 : "", file2 ? file2 : "");
}
/* --- message handling --- */
SfiMsgType
sfi_msg_type_register (const gchar *ident,
                       SfiMsgType   default_ouput,
                       const gchar *label)
{
  BIRNET_STATIC_ASSERT (Birnet::Msg::NONE == (uint) SFI_MSG_NONE);
  BIRNET_STATIC_ASSERT (Birnet::Msg::ALWAYS == (uint) SFI_MSG_ALWAYS);
  BIRNET_STATIC_ASSERT (Birnet::Msg::ERROR == (uint) SFI_MSG_ERROR);
  BIRNET_STATIC_ASSERT (Birnet::Msg::WARNING == (uint) SFI_MSG_WARNING);
  BIRNET_STATIC_ASSERT (Birnet::Msg::SCRIPT == (uint) SFI_MSG_SCRIPT);
  BIRNET_STATIC_ASSERT (Birnet::Msg::INFO == (uint) SFI_MSG_INFO);
  BIRNET_STATIC_ASSERT (Birnet::Msg::DIAG == (uint) SFI_MSG_DIAG);
  BIRNET_STATIC_ASSERT (Birnet::Msg::DEBUG == (uint) SFI_MSG_DEBUG);
  return (SfiMsgType) Birnet::Msg::register_type (ident, Birnet::Msg::Type (default_ouput), label);
}
bool
sfi_msg_check (SfiMsgType mtype)
{
  return Birnet::Msg::check (Birnet::Msg::Type (mtype));
}
void
sfi_msg_enable (SfiMsgType mtype)
{
  return Birnet::Msg::enable (Birnet::Msg::Type (mtype));
}
void
sfi_msg_disable (SfiMsgType mtype)
{
  return Birnet::Msg::disable (Birnet::Msg::Type (mtype));
}
void
sfi_msg_allow (const gchar *ident_list)
{
  return Birnet::Msg::allow_msgs (ident_list);
}
void
sfi_msg_deny (const gchar *ident_list)
{
  return Birnet::Msg::deny_msgs (ident_list);
}
const char*
sfi_msg_type_ident (SfiMsgType mtype)
{
  return Birnet::Msg::type_ident (Birnet::Msg::Type (mtype));
}
const char*
sfi_msg_type_label (SfiMsgType mtype)
{
  return Birnet::Msg::type_label (Birnet::Msg::Type (mtype));
}
SfiMsgType
sfi_msg_lookup_type (const char *ident)
{
  return (SfiMsgType) Birnet::Msg::lookup_type (ident);
}
SfiMsgPart*
sfi_msg_part_printf (uint8          msg_part_id,
                     const char    *format,
                     ...)
{
  int saved_errno = errno;
  /* construct message */
  va_list args;
  va_start (args, format);
  char *text = g_strdup_vprintf (format, args);
  va_end (args);
  Birnet::Msg::Part *part;
  switch (msg_part_id)
    {
    case '0':   part = new Birnet::Msg::Text0 (Birnet::String (text)); break;
    case '1':   part = new Birnet::Msg::Text1 (Birnet::String (text)); break;
    case '2':   part = new Birnet::Msg::Text2 (Birnet::String (text)); break;
    case '3':   part = new Birnet::Msg::Text3 (Birnet::String (text)); break;
    case 'c':   part = new Birnet::Msg::Check (Birnet::String (text)); break;
    default:    part = new Birnet::Msg::Custom (msg_part_id, Birnet::String (text)); break;
    }
  g_free (text);
  errno = saved_errno;
  return (SfiMsgPart*) part;
}
void
sfi_msg_display_parts (const char     *log_domain,
                       SfiMsgType      mtype,
                       guint           n_mparts,
                       SfiMsgPart    **mparts)
{
  int saved_errno = errno;
  std::vector<Birnet::Msg::Part> parts;
  for (uint i = 0; i < n_mparts; i++)
    {
      Birnet::Msg::Part *part = (Birnet::Msg::Part*) mparts[i];
      parts.push_back (*part);
      delete part;
    }
  Birnet::Msg::display_parts (log_domain, Birnet::Msg::Type (mtype), parts);
  errno = saved_errno;
}
/**
 * @param log_domain    log domain
 * @param level         one of SFI_MSG_ERROR, SFI_MSG_WARNING, SFI_MSG_INFO, SFI_MSG_DIAG or SFI_MSG_DEBUG
 * @param format        printf-like format
 * @param ...           printf-like arguments
 *
 * Log a message through SFIs logging mechanism. The current
 * value of errno is preserved around calls to this function.
 * Usually this function isn't used directly, but through one
 * of sfi_debug(), sfi_diag(), sfi_info(), sfi_warn() or sfi_error().
 * The @a log_domain indicates the calling module and relates to
 * G_LOG_DOMAIN as used by g_log().
 * This function is MT-safe and may be called from any thread.
 */
void
sfi_msg_display_printf (const char    *log_domain,
                        SfiMsgType     mtype,
                        const char    *format,
                        ...)
{
  int saved_errno = errno;
  /* construct message */
  va_list args;
  va_start (args, format);
  char *primary = g_strdup_vprintf (format, args);
  va_end (args);
  std::vector<Birnet::Msg::Part> parts;
  parts.push_back (Birnet::Msg::Primary (std::string (primary)));
  g_free (primary);
  Birnet::Msg::display_parts (log_domain, Birnet::Msg::Type (mtype), parts);
  errno = saved_errno;
}

/* --- url handling --- */
void
sfi_url_show (const char *url)
{
  return Birnet::url_show (url);
}
void
sfi_url_show_with_cookie (const char *url,
                          const char *url_title,
                          const char *cookie)
{
  return Birnet::url_show_with_cookie (url, url_title, cookie);
}
bool
sfi_url_test_show (const char *url)
{
  return Birnet::url_test_show (url);
}
bool
sfi_url_test_show_with_cookie (const char *url,
                               const char *url_title,
                               const char *cookie)
{
  return Birnet::url_test_show_with_cookie (url, url_title, cookie);
}

void
sfi_runtime_problem (char        ewran_tag,
                     const char *domain,
                     const char *file,
                     int         line,
                     const char *funcname,
                     const char *msgformat,
                     ...)
{
  va_list args;
  va_start (args, msgformat);
  ::Birnet::birnet_runtime_problemv (ewran_tag, domain, file, line, funcname, msgformat, args);
  va_end (args);
}
/* vim:set ts=8 sts=2 sw=2: */
