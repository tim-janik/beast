/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
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
 *
 * bsemaster.h: BSE master source
 */
#ifndef __BSE_MASTER_H__
#define __BSE_MASTER_H__

#include        <bse/bsesource.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- BseMaster type macros --- */
#define BSE_TYPE_MASTER              (BSE_TYPE_ID (BseMaster))
#define BSE_MASTER(object)           (BSE_CHECK_STRUCT_CAST ((object), BSE_TYPE_MASTER, BseMaster))
#define BSE_MASTER_CLASS(class)      (BSE_CHECK_CLASS_CAST ((class), BSE_TYPE_MASTER, BseMasterClass))
#define BSE_IS_MASTER(object)        (BSE_CHECK_STRUCT_TYPE ((object), BSE_TYPE_MASTER))
#define BSE_IS_MASTER_CLASS(class)   (BSE_CHECK_CLASS_TYPE ((class), BSE_TYPE_MASTER))
#define BSE_MASTER_GET_CLASS(object) ((BseMasterClas*) (((BseObject*) (object))->bse_struct.bse_class))


/* --- BseMaster object --- */
typedef struct _BseMaster      BseMaster;
typedef struct _BseMasterClass BseMasterClass;
struct _BseMaster
{
  BseSource       parent_object;

  guint		  n_tracks;
  BseStream	 *stream;
  BseChunk      **chunks;
};
struct _BseMasterClass
{
  BseSourceClass parent_class;
};


/* --- prototypes --- */
BseMaster*	bse_master_new			(BseStream	*output_stream,
						 guint		 n_output_tracks);
void		bse_master_set_output		(BseMaster	*master,
						 BseStream	*output_stream,
						 guint		 n_output_tracks);
void		bse_master_add_source		(BseMaster	*master,
						 BseSource	*source,
						 guint		 ochannel_id);
gboolean	bse_master_remove_source	(BseMaster	*master,
						 BseSource	*source);

BseIndex	bse_masters_cycle		(void);
void		bse_masters_reset		(void);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_MASTER_H__ */
