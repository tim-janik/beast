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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * bseheart.h: BSE's heart that keeps everything going ;)
 */
#ifndef __BSE_HEART_H__
#define __BSE_HEART_H__

#include	<bse/bseproject.h>
#include	<bse/bsepcmdevice.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_HEART		    (BSE_TYPE_ID (BseHeart))
#define BSE_HEART(object)	    (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_HEART, BseHeart))
#define BSE_HEART_CLASS(class)	    (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_HEART, BseHeartClass))
#define BSE_IS_HEART(object)	    (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_HEART))
#define BSE_IS_HEART_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_HEART))
#define BSE_HEART_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BseHeartClass))


/* --- BseHeart structs --- */
typedef struct _BseHeart       BseHeart;
typedef struct _BseHeartClass  BseHeartClass;
typedef struct _BseHeartDevice BseHeartDevice;
struct _BseHeart
{
  BseObject	  parent_object;
  
  guint		   latency;
  
  guint		   n_sources;
  BseSource	 **sources;
  guint		   n_devices;
  BseHeartDevice  *devices;
  guint		   n_open_devices;
  gchar		  *default_odevice;
  gchar		  *default_idevice;
  
  guint		   device_open_handler_id;
  GSList	  *device_open_list;
  
  BseMixValue	  *mix_buffer;
  
  guint		   gsource_id;
};
struct _BseHeartClass
{
  BseObjectClass parent_class;
};
struct _BseHeartDevice
{
  BsePcmDevice	*device;
  gchar		*name;
  guint		 n_isources;
  BseSource    **isources;
  guint		 n_osources;
  BseSource    **osources;
  BseChunk     **ochunks;
};


/* --- prototypes --- */
void		bse_heart_register_device	 (const gchar	 *symbolic_name,
						  BsePcmDevice	 *pdev);
void		bse_heart_unregister_device	 (BsePcmDevice	 *pdev);
void		bse_heart_unregister_all_devices (void);
void		bse_heart_set_default_odevice	 (const gchar	 *symbolic_name);
void		bse_heart_set_default_idevice	 (const gchar	 *symbolic_name);
BsePcmDevice*	bse_heart_get_device		 (const gchar	 *symbolic_name);
gchar*		bse_heart_get_default_odevice	 (void);
gchar*		bse_heart_get_default_idevice	 (void);
gchar*		bse_heart_get_device_name	 (BsePcmDevice	 *pdev);
void		bse_heart_attach		 (BseSource	 *source);
void		bse_heart_detach		 (BseSource	 *source);
void		bse_heart_reset_all_attach	 (void);
void		bse_heart_source_add_idevice	 (BseSource	 *source,
						  BsePcmDevice	 *idev);
void		bse_heart_source_add_odevice	 (BseSource	 *source,
						  BsePcmDevice	 *odev);
void		bse_heart_source_remove_idevice	 (BseSource	 *source,
						  BsePcmDevice	 *idev);
void		bse_heart_source_remove_odevice	 (BseSource	 *source,
						  BsePcmDevice	 *odev);


/* --- private --- */
BseHeart*	bse_heart_get_global		 (gboolean	  with_ref);
BseIndex	bse_heart_get_beat_index	 (void);
void		bse_heart_beat			 (BseHeart	 *heart);
GSList* /*fr*/	bse_heart_collect_chunks	 (BseHeart	 *heart,
						  BseHeartDevice *odevice);
BseChunk*/*fr*/ bse_heart_mix_chunks		 (BseHeart	 *heart,
						  GSList /*af*/	 *chunk_list,
						  guint		  n_tracks);
void		bse_heart_queue_device		 (BseHeart	 *heart,
						  BseHeartDevice *hdevice);







#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_HEART_H__ */
