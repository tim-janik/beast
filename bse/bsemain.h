/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2002 Tim Janik
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

#include	<bse/bseglobals.h>
#include	<bse/bse.h>	/* initialization */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- global variables --- */
extern GMainContext *bse_main_context;
extern SfiMutex	     bse_main_sequencer_mutex;
extern gboolean      bse_main_developer_extensions;
extern SfiThread    *bse_main_thread;


/* --- initialization --- */
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


/* --- global macros --- */
#define	BSE_THREADS_ENTER()			// bse_main_global_lock ()
#define	BSE_THREADS_LEAVE()			// bse_main_global_unlock ()
#define	BSE_SEQUENCER_LOCK()			SFI_SYNC_LOCK (&bse_main_sequencer_mutex)
#define	BSE_SEQUENCER_UNLOCK()			SFI_SYNC_UNLOCK (&bse_main_sequencer_mutex)
#define	BSE_DVL_EXT				(bse_main_developer_extensions != FALSE)


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_MAIN_H__ */
