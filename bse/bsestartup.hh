// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_STARTUP_HH__
#define __BSE_STARTUP_HH__

#include <bse/sfi.hh>

/// The Bse namespace contains all functions of the synthesis engine.
namespace Bse {

/// The task registry keeps track of runtime threads for profiling and statistical purposes.
class TaskRegistry {            // FIXME: move this to IDL
public:
  typedef std::vector<Bse::TaskStatus> List;
  static void  add     (const std::string &name, int pid,
                        int tid = -1);  ///< Add process/thread to registry for runtime profiling.
  static bool  remove  (int tid);       ///< Remove process/thread based on thread_id.
  static void  update  ();              ///< Issue TaskStatus.update on all tasks in registry.
  static List  list    ();              ///< Retrieve a copy to the list of all tasks in registry.
};

// == BSE Initialization ==

void		init_async	    (int *argc, char **argv, const char *app_name, const StringVector &args = StringVector());
bool		init_needed	    ();
void		objects_debug_leaks ();

} // Bse

#endif // __BSE_STARTUP_HH__
