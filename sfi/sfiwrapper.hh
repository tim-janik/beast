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
void sfi_init (int *argcp, char **argv, const char *app_name, const Bse::StringVector &args = Bse::StringVector());

/* --- file tests --- */
bool	birnet_file_check (const char *file,
			   const char *mode);
bool	birnet_file_equals (const char *file1,
			    const char *file2);
/* --- messaging --- */
#define         sfi_error(...)                   RAPICORN_FATAL (__VA_ARGS__)
#define         sfi_warning(...)                 RAPICORN_CRITICAL (__VA_ARGS__)
#define         sfi_info(...)                    BSE_DEBUG (__VA_ARGS__)
#define         sfi_diag(...)                    BSE_DEBUG (__VA_ARGS__)
#define         sfi_debug(lvl, ...)              BSE_DEBUG (__VA_ARGS__)
#define         sfi_nodebug(lvl, ...)            do { /* nothing */ } while (0)

/* --- url handling --- */
void sfi_url_show                   	(const char           *url);
void sfi_url_show_with_cookie       	(const char           *url,
					 const char           *url_title,
					 const char           *cookie);
bool sfi_url_test_show              	(const char           *url);
bool sfi_url_test_show_with_cookie	(const char           *url,
					 const char           *url_title,
					 const char           *cookie);

BIRNET_EXTERN_C_END();
#endif /* __SFI_WRAPPER_H__ */
/* vim:set ts=8 sts=2 sw=2: */
