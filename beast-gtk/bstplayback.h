/* BEAST - Bedevilled Audio System
 * Copyright (C) 2001 Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BST_PLAY_BACK_H__
#define __BST_PLAY_BACK_H__

#include        "bstdefs.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- typedefs & structures --- */
typedef void (*BstPlayBackNotify)	(gpointer	data,
					 guint		pcm_position);
typedef struct
{
  BswProxy project;
  BswProxy snet;
  BswProxy speaker;
  BswProxy wosc;
  BswProxy constant;
  guint             pcm_timeout;
  BstPlayBackNotify pcm_notify;
  gpointer          pcm_data;
} BstPlayBackHandle;


/* --- functions --- */
BstPlayBackHandle*	bst_play_back_handle_new	(void);
void			bst_play_back_handle_set	(BstPlayBackHandle	*handle,
							 BswProxy		 esample,
							 gdouble		 osc_freq);
void			bst_play_back_handle_start	(BstPlayBackHandle	*handle);
void			bst_play_back_handle_stop	(BstPlayBackHandle	*handle);
void			bst_play_back_handle_toggle	(BstPlayBackHandle	*handle);
void			bst_play_back_handle_pcm_notify	(BstPlayBackHandle	*handle,
							 guint			 timeout,
							 BstPlayBackNotify	 notify,
							 gpointer		 data);
gboolean		bst_play_back_handle_is_playing	(BstPlayBackHandle	*handle);
gboolean		bst_play_back_handle_done	(BstPlayBackHandle	*handle);
void			bst_play_back_handle_destroy	(BstPlayBackHandle	*handle);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_PLAY_BACK_H__ */
