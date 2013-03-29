// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BIRNET_UTILS_XX_HH__
#define __BIRNET_UTILS_XX_HH__
#include <birnet/birnetcdefs.h>
#include <rapicorn.hh>
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

/* --- url handling --- */
void url_show                   (const char           *url);
void url_show_with_cookie       (const char           *url,
                                 const char           *url_title,
                                 const char           *cookie);
bool url_test_show              (const char           *url);
bool url_test_show_with_cookie  (const char	      *url,
                                 const char           *url_title,
                                 const char           *cookie);

} // Birnet

#endif /* __BIRNET_UTILS_XX_HH__ */
/* vim:set ts=8 sts=2 sw=2: */
