// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_ENGINE_MASTER_H__
#define __BSE_ENGINE_MASTER_H__
#include <bse/bseengine.hh>
/* --- internal (EngineThread) --- */
gboolean	_engine_master_prepare		(BseEngineLoop		*loop);
gboolean	_engine_master_check		(const BseEngineLoop	*loop);
void		_engine_master_dispatch_jobs	(void);
void		_engine_master_dispatch		(void);

namespace Bse {

class MasterThread {
  std::thread           thread_;
  EventFd               event_fd_;
  std::function<void()> caller_wakeup_;
  static void reap_master_thread ();
  void        master_thread      ();
  explicit    MasterThread       (const std::function<void()> &caller_wakeup);
public:
  static void wakeup             ();
  static void start              (const std::function<void()> &caller_wakeup);
};

} // Bse


namespace BseInternal {

void    engine_run_slave        ();
void    engine_start_slaves     ();
void    engine_stop_slaves      ();
void    engine_wakeup_slaves    ();

} // BseInternal

#endif /* __BSE_ENGINE_MASTER_H__ */
