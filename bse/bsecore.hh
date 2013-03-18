// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_CORE_HH__
#define __BSE_CORE_HH__

#include <bse/bse.hh>

/// The Bse namespace contains all functions of the synthesis engine.
namespace Bse {
using namespace Birnet;         // FIXME: using Rapicorn

/// The task registry keeps track of runtime threads for profiling and statistical purposes.
class TaskRegistry {            // FIXME: move this to IDL
public:
  typedef std::vector<Rapicorn::TaskStatus> List;
  static void  add     (const std::string &name, int pid,
                        int tid = -1);  ///< Add process/thread to registry for runtime profiling.
  static bool  remove  (int tid);       ///< Remove process/thread based on thread_id.
  static void  update  ();              ///< Issue TaskStatus.update on all tasks in registry.
  static List  list    ();              ///< Retrieve a copy to the list of all tasks in registry.
};

} // Bse

#endif /* __BSE_CORE_HH__ */
