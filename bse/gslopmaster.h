/* GSL Engine - Flow module operation engine
 * Copyright (C) 2001-2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __GSL_ENGINE_MASTER_H__
#define __GSL_ENGINE_MASTER_H__

#include <bse/gslengine.h>

G_BEGIN_DECLS

/* --- internal (EngineThread) --- */
gboolean	_engine_master_prepare		(GslEngineLoop		*loop);
gboolean	_engine_master_check		(const GslEngineLoop	*loop);
void		_engine_master_dispatch_jobs	(void);
void		_engine_master_dispatch		(void);
typedef struct {
  SfiThread *user_thread;
  gint       wakeup_pipe[2];	/* read(wakeup_pipe[0]), write(wakeup_pipe[1]) */
} EngineMasterData;
void		_engine_master_thread		(EngineMasterData	*mdata);

G_END_DECLS

#endif /* __GSL_ENGINE_MASTER_H__ */
