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


/* --- structures --- */
typedef struct
{
  BswProxy wave;
  BswProxy wave_osc;
  BswProxy const_freq;
} BstPlayBackHandle;


/* --- functions --- */
void			bst_play_back_init		(void);
BstPlayBackHandle*	bst_play_back_handle_new	(void);
void			bst_play_back_handle_set	(BstPlayBackHandle	*handle,
							 GslWaveChunk		*wave_chunk,
							 gdouble		 osc_freq);
void			bst_play_back_handle_start	(BstPlayBackHandle	*handle);
void			bst_play_back_handle_stop	(BstPlayBackHandle	*handle);
gboolean		bst_play_back_handle_done	(BstPlayBackHandle	*handle);
void			bst_play_back_handle_destroy	(BstPlayBackHandle	*handle);

#include	<gsl/gslwavechunk.h>




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_PLAY_BACK_H__ */
