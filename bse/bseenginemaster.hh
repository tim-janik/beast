// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_ENGINE_MASTER_H__
#define __BSE_ENGINE_MASTER_H__
#include <bse/bseengine.hh>
G_BEGIN_DECLS
/* --- internal (EngineThread) --- */
gboolean	_engine_master_prepare		(BseEngineLoop		*loop);
gboolean	_engine_master_check		(const BseEngineLoop	*loop);
void		_engine_master_dispatch_jobs	(void);
void		_engine_master_dispatch		(void);
typedef struct {
  BirnetThread *user_thread;
  gint       wakeup_pipe[2];	/* read(wakeup_pipe[0]), write(wakeup_pipe[1]) */
} EngineMasterData;
void		bse_engine_master_thread	(EngineMasterData	*mdata);
G_END_DECLS
#endif /* __BSE_ENGINE_MASTER_H__ */
