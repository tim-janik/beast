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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BSE_MAIN_H__
#define __BSE_MAIN_H__

#include	<bse/bse.h>	/* initialization */

G_BEGIN_DECLS


/* --- initialization --- */
void		bse_init_textdomain_only (void);
#if 0	// prototyped in bse.h */
void		bse_init_async		(gint		*argc,
					 gchar	      ***argv,
					 SfiRec		*config);
SfiGlueContext* bse_init_glue_context	(const gchar    *client);
gchar*          bse_check_version	(guint		 required_major,
                                         guint		 required_minor,
                                         guint		 required_micro);
#endif
/* initialization for internal utilities */
void		bse_init_intern		(gint		*argc,
					 gchar	      ***argv,
					 SfiRec		*config);

/* BSE thread pid (or 0) */
guint           bse_main_getpid         (void);

/* MT-safe log handler */
void            bse_msg_handler         (const BirnetMessage *message);

/* --- global macros --- */
#define	BSE_THREADS_ENTER()			// bse_main_global_lock ()
#define	BSE_THREADS_LEAVE()			// bse_main_global_unlock ()
#define	BSE_SEQUENCER_LOCK()			birnet_mutex_lock (&bse_main_sequencer_mutex)
#define	BSE_SEQUENCER_UNLOCK()			birnet_mutex_unlock (&bse_main_sequencer_mutex)
#define	BSE_DBG_EXT     			(bse_main_debug_extensions != FALSE)

/* --- argc/argv overide settings --- */
typedef struct {
  SfiInt       latency;
  SfiInt       mixing_freq;
  SfiInt       control_freq;
  SfiRing     *pcm_drivers;
  SfiRing     *midi_drivers;
  gboolean     load_drivers_early;
  gboolean     dump_driver_list;
  const gchar *override_plugin_globs;
  const gchar *override_script_path;
  const gchar *path_binaries;
} BseMainArgs;


/* --- internal --- */
void    _bse_init_c_wrappers    ();
extern BseMainArgs  *bse_main_args;
extern GMainContext *bse_main_context;
extern BirnetMutex	     bse_main_sequencer_mutex;
extern gboolean      bse_main_debug_extensions;
extern BirnetThread    *bse_main_thread;


G_END_DECLS

#endif /* __BSE_MAIN_H__ */
