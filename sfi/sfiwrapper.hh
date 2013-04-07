// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __SFI_WRAPPER_H__
#define __SFI_WRAPPER_H__
#include <stdbool.h>
#include <sfi/glib-extra.hh>
#include <birnet/birnetcdefs.h> /* include glib before birnet for G_LOG_DOMAIN */
#include <birnet/birnetutils.hh>

namespace Bse {

// == Likelyness Hinting ==
#define BSE_ISLIKELY(expr)      RAPICORN_ISLIKELY(expr) ///< Compiler hint that @a expr is likely to be true.
#define BSE_UNLIKELY(expr)      RAPICORN_UNLIKELY(expr) ///< Compiler hint that @a expr is unlikely to be true.
#define BSE_LIKELY              BSE_ISLIKELY            ///< Compiler hint that @a expr is likely to be true.

// == Debugging ==
/// Issue a general purpose debugging message, configurable via #$BSE_DEBUG.
#define BSE_DEBUG(...)          do { if (BSE_UNLIKELY (Bse::_cached_bse_debug)) Bse::bse_debug (NULL, RAPICORN_PRETTY_FILE, __LINE__, __VA_ARGS__); } while (0)
/// Issue a debugging message if debugging for @a key is enabled via #$BSE_DEBUG.
#define BSE_KEY_DEBUG(key,...)  do { if (BSE_UNLIKELY (Bse::_cached_bse_debug)) Bse::bse_debug (key, RAPICORN_PRETTY_FILE, __LINE__, __VA_ARGS__); } while (0)
extern bool volatile _cached_bse_debug;
void        bse_debug         (const char*, const char*, int, const char*, ...) RAPICORN_PRINTF (4, 5);
bool       _bse_debug_enabled (const char *key);
inline bool bse_debug_enabled (const char *key = NULL) { return BSE_UNLIKELY (_cached_bse_debug) && _bse_debug_enabled (key); }
bool        bse_flipper_check (const char *key);

} // Bse

/* sfiwrapper.h is a thin C language wrapper around C++ features
 * provided by libbirnet.
 */
/* --- short integer types --- */
#ifdef __cplusplus
#include <birnet/birnetutils.hh>
using Birnet::uint8;
using Birnet::uint16;
using Birnet::uint32;
using Birnet::uint64;
using Birnet::int8;
using Birnet::int16;
using Birnet::int32;
using Birnet::int64;
using Birnet::unichar;
#else
typedef BirnetUInt8   uint8;
typedef BirnetUInt16  uint16;
typedef BirnetUInt32  uint32;
typedef BirnetUInt64  uint64;
typedef BirnetInt8    int8;
typedef BirnetInt16   int16;
typedef BirnetInt32   int32;
typedef BirnetInt64   int64;
typedef BirnetUnichar unichar;
#endif
BIRNET_EXTERN_C_BEGIN();
/* --- initialization --- */
typedef struct
{
  const char *value_name;       /* value list ends with value_name == NULL */
  const char *value_string;
  long double value_num;        /* valid if value_string == NULL */
} SfiInitValue;
void    sfi_init        (int            *argcp,
			 char         ***argvp,
			 const char     *app_name,
			 SfiInitValue    sivalues[]);
bool    sfi_init_value_bool   (SfiInitValue *value);
double  sfi_init_value_double (SfiInitValue *value);
gint64  sfi_init_value_int    (SfiInitValue *value);
typedef BirnetInitSettings SfiInitSettings;
SfiInitSettings sfi_init_settings (void);
/* --- file tests --- */
bool	birnet_file_check (const char *file,
			   const char *mode);
bool	birnet_file_equals (const char *file1,
			    const char *file2);
/* --- messaging --- */
typedef enum {
  SFI_MSG_NONE   = 0,   /* always off */
  SFI_MSG_ALWAYS = 1,   /* always on */
  SFI_MSG_ERROR,
  SFI_MSG_WARNING,
  SFI_MSG_SCRIPT,
  SFI_MSG_INFO,
  SFI_MSG_DIAG,
  SFI_MSG_DEBUG
} SfiMsgType;
typedef enum {
  SFI_MSG_TO_STDERR     = 1,
  SFI_MSG_TO_STDLOG     = 2,
  SFI_MSG_TO_HANDLER    = 4,
} SfiMsgLogFlags;
#define         sfi_error(...)                   sfi_msg_printf (SFI_MSG_ERROR, __VA_ARGS__)
#define         sfi_warning(...)                 sfi_msg_printf (SFI_MSG_WARNING, __VA_ARGS__)
#define         sfi_info(...)                    sfi_msg_printf (SFI_MSG_INFO, __VA_ARGS__)
#define         sfi_diag(...)                    sfi_msg_printf (SFI_MSG_DIAG, __VA_ARGS__)
#define         sfi_debug(lvl, ...)              sfi_msg_printf (lvl, __VA_ARGS__)
#define         sfi_nodebug(lvl, ...)            do { /* nothing */ } while (0)
bool            sfi_msg_check                   (SfiMsgType      mtype);
void            sfi_msg_enable                  (SfiMsgType      mtype);
void            sfi_msg_disable                 (SfiMsgType      mtype);
void            sfi_msg_allow                   (const char     *ident_list);
void            sfi_msg_deny                    (const char     *ident_list);
const char*     sfi_msg_type_ident              (SfiMsgType      mtype);
const char*     sfi_msg_type_label              (SfiMsgType      mtype);
SfiMsgType      sfi_msg_lookup_type             (const char     *ident);
SfiMsgType      sfi_msg_type_register           (const char     *ident,
                                                 SfiMsgType      default_ouput,
                                                 const char     *label);
#define         sfi_msg_printf(level, ...)       SFI_MSG_PRINTF (level, __VA_ARGS__)    /* level, printf_format, ... */
#define         sfi_msg_display(level, ...)      SFI_MSG_DISPLAY (level, __VA_ARGS__)   /* level, part, part, ... */
#define         SFI_MSG_TEXT0(...)               sfi_msg_part_printf ('0', __VA_ARGS__) /* message title */
#define         SFI_MSG_TEXT1(...)               sfi_msg_part_printf ('1', __VA_ARGS__) /* primary message */
#define         SFI_MSG_TEXT2(...)               sfi_msg_part_printf ('2', __VA_ARGS__) /* secondary message */
#define         SFI_MSG_TEXT3(...)               sfi_msg_part_printf ('3', __VA_ARGS__) /* message details */
#define         SFI_MSG_CHECK(...)               sfi_msg_part_printf ('c', __VA_ARGS__) /* user switch */
#define         SFI_MSG_TITLE                    SFI_MSG_TEXT0 /* alias */
#define         SFI_MSG_PRIMARY                  SFI_MSG_TEXT1 /* alias */
#define         SFI_MSG_SECONDARY                SFI_MSG_TEXT2 /* alias */
#define         SFI_MSG_DETAIL                   SFI_MSG_TEXT3 /* alias */
#define         SFI_MSG_TYPE_DEFINE(variable, ident, default_ouput, label) SFI_MSG__TYPEDEF (variable, ident, default_ouput, label)

/* --- messaging implementation --- */
typedef struct SfiMsgPart SfiMsgPart;
SfiMsgPart*     sfi_msg_part_printf     (uint8          msg_part_id,
                                         const char    *format,
                                         ...) G_GNUC_PRINTF (2, 3);
void            sfi_msg_display_parts   (const char     *log_domain,
                                         SfiMsgType      mtype,
                                         guint           n_mparts,
                                         SfiMsgPart    **mparts);
void            sfi_msg_display_printf  (const char    *log_domain,
                                         SfiMsgType     mtype,
                                         const char    *format,
                                         ...) G_GNUC_PRINTF (3, 4);
#define SFI_MSG_PRINTF(lvl, ...)        do { SfiMsgType __mt = lvl; if (sfi_msg_check (__mt)) \
                                             sfi_msg_display_printf (BIRNET_LOG_DOMAIN, __mt, __VA_ARGS__); } while (0)
#define SFI_MSG_DISPLAY(lvl, ...)       do { SfiMsgType __mt = lvl; if (sfi_msg_check (__mt)) { \
                                               SfiMsgPart *__pa[] = { __VA_ARGS__ };            \
                                               sfi_msg_display_parts (BIRNET_LOG_DOMAIN, __mt,  \
                                                 BIRNET_ARRAY_SIZE (__pa), __pa); } } while (0)
#define SFI_MSG__TYPEDEF(variable, identifier, default_ouput, label) \
  SfiMsgType variable = (SfiMsgType) 0; \
  static void BIRNET_CONSTRUCTOR \
  BIRNET_CPP_PASTE4 (__sfi_msg_type__init, __LINE__, __, variable) (void) \
  { variable = sfi_msg_type_register (identifier, default_ouput, label); }

/* --- url handling --- */
void sfi_url_show                   	(const char           *url);
void sfi_url_show_with_cookie       	(const char           *url,
					 const char           *url_title,
					 const char           *cookie);
bool sfi_url_test_show              	(const char           *url);
bool sfi_url_test_show_with_cookie	(const char           *url,
					 const char           *url_title,
					 const char           *cookie);

#ifndef BIRNET__RUNTIME_PROBLEM
#define BIRNET__RUNTIME_PROBLEM(ErrorWarningReturnAssertNotreach,domain,file,line,funcname,...) \
        sfi_runtime_problem (ErrorWarningReturnAssertNotreach, domain, file, line, funcname, __VA_ARGS__)
#endif
void sfi_runtime_problem (char        ewran_tag,
			  const char *domain,
			  const char *file,
			  int         line,
			  const char *funcname,
			  const char *msgformat,
			  ...) BIRNET_PRINTF (6, 7);
BIRNET_EXTERN_C_END();
#endif /* __SFI_WRAPPER_H__ */
/* vim:set ts=8 sts=2 sw=2: */
