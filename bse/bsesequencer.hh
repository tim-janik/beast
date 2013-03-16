// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_SSEQUENCER_H__
#define __BSE_SSEQUENCER_H__
#include <bse/bsesong.hh>

G_BEGIN_DECLS

typedef struct {
  guint64		 stamp;	/* sequencer time (ahead of real time) */
  SfiRing		*songs;
} BseSequencer;

extern BirnetThread*    bse_sequencer_thread; // FIXME

void			bse_sequencer_wakeup            ();
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
