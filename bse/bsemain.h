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


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- bse global lock --- */
typedef struct
{
  gpointer	  lock_data;
  void		(*lock)		(gpointer lock_data);
  void		(*unlock)	(gpointer lock_data);
} BseLockFuncs;


/* --- variables --- */
#if 0 /* bsedefs.h: */
extern BseDebugFlags bse_debug_flags;
#endif


/* --- prototypes --- */
gboolean	bse_initialized			(void);
void		bse_init			(gint		    *argc,
						 gchar	          ***argv,
						 const BseLockFuncs *lock_funcs);
#define	BSE_THREADS_ENTER()			bse_main_global_lock ()
#define	BSE_THREADS_LEAVE()			bse_main_global_unlock ()
#define	BSE_SEQUENCER_LOCK()			bse_main_sequencer_lock ()
#define	BSE_SEQUENCER_UNLOCK()			bse_main_sequencer_unlock ()


/* --- internal --- */
void		bse_main_global_lock		(void);
void		bse_main_global_unlock		(void);
void		bse_main_sequencer_lock		(void);
void		bse_main_sequencer_unlock	(void);
extern gboolean	bse_developer_extensions;
#define		BSE_DVL_EXT			(bse_developer_extensions != FALSE)



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_MAIN_H__ */
