/* BSE - Better Sound Engine
 * Copyright (C) 1997-1999, 2000-2004 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#ifndef __BSE_SSEQUENCER_H__
#define __BSE_SSEQUENCER_H__

#include <bse/bsesong.hh>

G_BEGIN_DECLS

typedef struct {
  guint64		 stamp;	/* sequencer time (ahead of real time) */
  SfiRing		*songs;
} BseSequencer;

extern BirnetThread       *bse_sequencer_thread;

void			bse_sequencer_init_thread	(void);
void                    bse_sequencer_add_io_watch      (guint           n_pfds,
                                                         const GPollFD  *pfds,
                                                         BseIOWatch      watch_func,
                                                         gpointer        data);
void                    bse_sequencer_remove_io_watch   (BseIOWatch      watch_func,
                                                         gpointer        data);
void			bse_sequencer_start_song	(BseSong        *song,
                                                         guint64         start_stamp);
void                    bse_sequencer_remove_song	(BseSong        *song);
gboolean                bse_sequencer_thread_lagging    (guint           n_blocks);

G_END_DECLS

#endif /* __BSE_SSEQUENCER_H__ */
