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

G_BEGIN_DECLS

#define	BSE_SSEQUENCER_PREPROCESS	(bse_engine_block_size () * 7)

typedef struct {
  guint64		 stamp;	/* sequencer time (ahead of real time) */
  SfiRing		*songs;
} BseSSequencer;

extern SfiThread       *bse_ssequencer_thread;

void			bse_ssequencer_init_thread	(void);
void			bse_ssequencer_start_song	(BseSong        *song,
                                                         guint64         start_stamp);
void                    bse_ssequencer_remove_song	(BseSong        *song);
gboolean                bse_sequencer_thread_lagging    (void);

G_END_DECLS

#endif /* __BSE_SSEQUENCER_H__ */
