/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-1999, 2000-2003 Tim Janik
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
 * bsepcmdevice.h: pcm device driver class
 */
#ifndef __BSE_PCM_DEVICE_H__
#define __BSE_PCM_DEVICE_H__

#include <bse/bseitem.h>
#include <bse/gsldefs.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_PCM_DEVICE              (BSE_TYPE_ID (BsePcmDevice))
#define BSE_PCM_DEVICE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_PCM_DEVICE, BsePcmDevice))
#define BSE_PCM_DEVICE_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_PCM_DEVICE, BsePcmDeviceClass))
#define BSE_IS_PCM_DEVICE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_PCM_DEVICE))
#define BSE_IS_PCM_DEVICE_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PCM_DEVICE))
#define BSE_PCM_DEVICE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_PCM_DEVICE, BsePcmDeviceClass))
/* flag tests */
#define BSE_PCM_DEVICE_OPEN(pdev)	 ((BSE_OBJECT_FLAGS (pdev) & BSE_PCM_FLAG_OPEN) != 0)
#define BSE_PCM_DEVICE_READABLE(pdev)	 ((BSE_OBJECT_FLAGS (pdev) & BSE_PCM_FLAG_READABLE) != 0)
#define BSE_PCM_DEVICE_WRITABLE(pdev)	 ((BSE_OBJECT_FLAGS (pdev) & BSE_PCM_FLAG_WRITABLE) != 0)


/* --- capabilities --- */
typedef enum	/*< skip >*/
{
  BSE_PCM_FREQ_8000	= 1,
  BSE_PCM_FREQ_11025,
  BSE_PCM_FREQ_16000,
  BSE_PCM_FREQ_22050,
  BSE_PCM_FREQ_32000,
  BSE_PCM_FREQ_44100,
  BSE_PCM_FREQ_48000,
  BSE_PCM_FREQ_88200,
  BSE_PCM_FREQ_96000,
  BSE_PCM_FREQ_176400,
  BSE_PCM_FREQ_192000
} BsePcmFreqMode;
#define	BSE_PCM_FREQ_MIN	BSE_PCM_FREQ_8000
#define	BSE_PCM_FREQ_MAX	BSE_PCM_FREQ_192000
typedef enum	/*< skip >*/
{
  BSE_PCM_CMODE_MONO	= 1,
  BSE_PCM_CMODE_STEREO
} BsePcmChannelMode;
typedef enum	/*< skip >*/
{
  BSE_PCM_FLAG_OPEN	= 1 << (BSE_ITEM_FLAGS_USHIFT + 0),
  BSE_PCM_FLAG_READABLE	= 1 << (BSE_ITEM_FLAGS_USHIFT + 1),
  BSE_PCM_FLAG_WRITABLE	= 1 << (BSE_ITEM_FLAGS_USHIFT + 2)
} BsePcmFlags;
#define	BSE_PCM_FLAGS_USHIFT	(BSE_ITEM_FLAGS_USHIFT + 3)


/* --- BsePcmDevice structs --- */
typedef struct _BsePcmStatus		BsePcmStatus;
typedef struct _BsePcmHandle		BsePcmHandle;
typedef struct _BsePcmDevice		BsePcmDevice;
typedef struct _BsePcmDeviceClass	BsePcmDeviceClass;
struct _BsePcmStatus
{
  guint	total_playback_values;
  guint	n_playback_values_available;
  guint	total_capture_values;
  guint	n_capture_values_available;
};
struct _BsePcmHandle
{
  guint			 writable : 1;
  guint			 readable : 1;
  guint			 n_channels;
  gfloat		 mix_freq;
  guint			 playback_watermark;
  SfiMutex		 mutex;
  gsize	(*read)		(BsePcmHandle		*handle,
			 gsize			 n_values,
			 gfloat			*values);
  void	(*write)	(BsePcmHandle		*handle,
			 gsize			 n_values,
			 const gfloat		*values);
  void	(*status)	(BsePcmHandle		*handle,
			 BsePcmStatus		*status);
};
struct _BsePcmDevice
{
  BseItem		parent_instance;

  /* requested caps */
  BsePcmFreqMode	req_freq_mode;
  guint			req_n_channels;

  /* operational handle */
  BsePcmHandle	       *handle;
};
struct _BsePcmDeviceClass
{
  BseItemClass		parent_class;

  guint			driver_rating;
  BseErrorType	(*open)		(BsePcmDevice	*pdev);
  void		(*suspend)	(BsePcmDevice	*pdev);
};


/* --- prototypes --- */
void		bse_pcm_device_request		(BsePcmDevice		*pdev,
						 guint			 n_channels,
						 BsePcmFreqMode		 freq_mode);
BseErrorType	bse_pcm_device_open		(BsePcmDevice		*pdev);
void		bse_pcm_device_suspend		(BsePcmDevice		*pdev);
BsePcmHandle*	bse_pcm_device_get_handle	(BsePcmDevice		*pdev);
gsize		bse_pcm_handle_read		(BsePcmHandle		*handle,
						 gsize			 n_values,
						 gfloat			*values);
void		bse_pcm_handle_write		(BsePcmHandle		*handle,
						 gsize			 n_values,
						 const gfloat		*values);
void		bse_pcm_handle_status		(BsePcmHandle		*handle,
						 BsePcmStatus		*status);
void		bse_pcm_handle_set_watermark	(BsePcmHandle		*handle,
						 guint			 watermark);


/* --- misc utils --- */
gfloat		bse_pcm_freq_from_freq_mode	(BsePcmFreqMode	freq_mode);
BsePcmFreqMode	bse_pcm_freq_mode_from_freq	(gfloat		freq);
     

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_PCM_DEVICE_H__ */
