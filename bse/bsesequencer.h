/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2003 Tim Janik
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
#ifndef __BSE_SSEQUENCER_H__
#define __BSE_SSEQUENCER_H__

#include <bse/bsesong.h>
#include <bse/bsepart.h>

G_BEGIN_DECLS

#define	BSE_SSEQUENCER_PREPROCESS	(bse_engine_block_size () * 7)


typedef enum {
  BSE_SSEQUENCER_JOB_NOP,
  BSE_SSEQUENCER_JOB_ADD,
  BSE_SSEQUENCER_JOB_REMOVE,
} BseSSequencerJobType;

typedef struct {
  BseSSequencerJobType	 type;
  BseSuper		*super;
  SfiTime		 stamp;
} BseSSequencerJob;

typedef struct {
  SfiTime		 stamp;	/* sequencer time (ahead of real time) */
  SfiRing		*jobs;
  SfiRing		*supers;
} BseSSequencer;

extern SfiThread       *bse_ssequencer_thread;

void			bse_ssequencer_init_thread	(void);
void			bse_ssequencer_start_supers	(SfiRing	*supers,
							 BseTrans	*trans);
BseSSequencerJob*	bse_ssequencer_job_stop_super	(BseSuper	*super);
SfiTime			bse_ssequencer_queue_jobs	(SfiRing	 *jobs);
void			bse_ssequencer_handle_jobs	(SfiRing	 *jobs);
void			bse_ssequencer_remove_super_SL	(BseSuper	*super);


G_END_DECLS

#endif /* __BSE_SSEQUENCER_H__ */
