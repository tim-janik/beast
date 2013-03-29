// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BIRNET_UTILS_XX_HH__
#define __BIRNET_UTILS_XX_HH__
#include <birnet/birnetcdefs.h>
#include <rapicorn.hh>
#include <glib.h> /* g_free */
#include <string>
#include <vector>
#include <map>
#include <stdarg.h>

// We're migrating to Bse for everything and use Rapicorn core for the lower level stuff
#include <rapicorn-core.hh>
namespace Bse {
using namespace Rapicorn;
} // Bse

namespace Birnet {
using namespace Rapicorn;

using std::vector;
using std::map;
using std::min;
using std::max;

/* --- implement assertion macros --- */
#ifndef BIRNET__RUNTIME_PROBLEM
#define BIRNET__RUNTIME_PROBLEM(ErrorWarningReturnAssertNotreach,domain,file,line,funcname,...) \
        ::Birnet::birnet_runtime_problem (ErrorWarningReturnAssertNotreach, domain, file, line, funcname, __VA_ARGS__)
#endif
void birnet_runtime_problem  (char        ewran_tag,
                              const char *domain,
                              const char *file,
                              int         line,
                              const char *funcname,
                              const char *msgformat,
                              ...) BIRNET_PRINTF (6, 7);
void birnet_runtime_problemv (char        ewran_tag,
                              const char *domain,
                              const char *file,
                              int         line,
                              const char *funcname,
                              const char *msgformat,
                              va_list     msgargs);
/* --- private copy constructor and assignment operator --- */
#define BIRNET_PRIVATE_CLASS_COPY(Class)        private: Class (const Class&); Class& operator= (const Class&);
#ifdef  _BIRNET_SOURCE_EXTENSIONS
#define PRIVATE_CLASS_COPY                      BIRNET_PRIVATE_CLASS_COPY
#define return_if_fail                          g_return_if_fail
#define return_val_if_fail                      g_return_val_if_fail
#endif  /* _BIRNET_SOURCE_EXTENSIONS */
/* --- initialization --- */
typedef BirnetInitValue    InitValue;
typedef BirnetInitSettings InitSettings;
InitSettings init_settings     ();
void         birnet_init       (int        *argcp,
                                char     ***argvp,
                                const char *app_name,
                                InitValue   ivalues[] = NULL);
bool         init_value_bool   (InitValue  *value);
double       init_value_double (InitValue  *value);
int64        init_value_int    (InitValue  *value);
/* --- initialization hooks --- */
class InitHook {
  typedef void (*InitHookFunc) (void);
  InitHook    *next;
  int          priority;
  InitHookFunc hook;
  BIRNET_PRIVATE_CLASS_COPY (InitHook);
  static void  invoke_hooks (void);
public:
  explicit InitHook (InitHookFunc _func,
                     int          _priority = 0);
};

/* --- file/path functionality --- */
namespace Path {
const String    dirname   (const String &path);
const String    basename  (const String &path);
bool            isabs     (const String &path);
const String    skip_root (const String &path);
const String    join      (const String &frag0, const String &frag1,
                           const String &frag2 = "", const String &frag3 = "",
                           const String &frag4 = "", const String &frag5 = "",
                           const String &frag6 = "", const String &frag7 = "",
                           const String &frag8 = "", const String &frag9 = "",
                           const String &frag10 = "", const String &frag11 = "",
                           const String &frag12 = "", const String &frag13 = "",
                           const String &frag14 = "", const String &frag15 = "");
bool            check     (const String &file,
                           const String &mode);
bool            equals    (const String &file1,
                           const String &file2);
} // Path

/* --- url handling --- */
void url_show                   (const char           *url);
void url_show_with_cookie       (const char           *url,
                                 const char           *url_title,
                                 const char           *cookie);
bool url_test_show              (const char           *url);
bool url_test_show_with_cookie  (const char	      *url,
                                 const char           *url_title,
                                 const char           *cookie);

/* --- cleanup registration --- */
uint cleanup_add                (uint                  timeout_ms,
                                 void                (*destroy_data) (void*),
                                 void                 *data);
void cleanup_force_handlers     (void);

/* --- string utils --- */
void memset4		        (uint32              *mem,
                                 uint32               filler,
                                 uint                 length);

/* --- zintern support --- */
uint8*  zintern_decompress      (unsigned int          decompressed_size,
                                 const unsigned char  *cdata,
                                 unsigned int          cdata_size);
void    zintern_free            (uint8                *dc_data);

/* --- template errors --- */
namespace TEMPLATE_ERROR {
// to error out, call invalid_type<YourInvalidType>();
template<typename Type> void invalid_type () { bool force_compiler_error = void (0); }
// to error out, derive from InvalidType<YourInvalidType>
template<typename Type> class InvalidType;
}

typedef Rapicorn::ReferenceCountable ReferenceCountImpl;

} // Birnet

#endif /* __BIRNET_UTILS_XX_HH__ */
/* vim:set ts=8 sts=2 sw=2: */
