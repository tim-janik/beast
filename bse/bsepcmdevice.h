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
 * bsepcmdevice.h: pcm device base object
 */
#ifndef __BSE_PCM_DEVICE_H__
#define __BSE_PCM_DEVICE_H__

#include        <bse/bseobject.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_PCM_DEVICE              (BSE_TYPE_ID (BsePcmDevice))
#define BSE_PCM_DEVICE(object)           (BSE_CHECK_STRUCT_CAST ((object), BSE_TYPE_PCM_DEVICE, BsePcmDevice))
#define BSE_PCM_DEVICE_CLASS(class)      (BSE_CHECK_CLASS_CAST ((class), BSE_TYPE_PCM_DEVICE, BsePcmDeviceClass))
#define BSE_IS_PCM_DEVICE(object)        (BSE_CHECK_STRUCT_TYPE ((object), BSE_TYPE_PCM_DEVICE))
#define BSE_IS_PCM_DEVICE_CLASS(class)   (BSE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PCM_DEVICE))
#define BSE_PCM_DEVICE_GET_CLASS(object) ((BsePcmDeviceClass*) (((BseObject*) (object))->bse_struct.bse_class))


/* --- object member/convenience macros --- */
#define BSE_PCM_DEVICE_HAS_CAPS(pdev)    ((BSE_OBJECT_FLAGS (pdev) & BSE_PCM_FLAG_HAS_CAPS) != 0)
#define BSE_PCM_DEVICE_OPEN(pdev)        ((BSE_OBJECT_FLAGS (pdev) & BSE_PCM_FLAG_OPEN) != 0)
#define BSE_PCM_DEVICE_READABLE(pdev)    ((BSE_OBJECT_FLAGS (pdev) & BSE_PCM_FLAG_READABLE) != 0)
#define BSE_PCM_DEVICE_WRITABLE(pdev)    ((BSE_OBJECT_FLAGS (pdev) & BSE_PCM_FLAG_WRITABLE) != 0)
#define BSE_PCM_DEVICE_REGISTERED(pdev)  ((BSE_OBJECT_FLAGS (pdev) & BSE_PCM_FLAG_REGISTERED) != 0)
#define BSE_PCM_DEVICE_STATE_SYNC(pdev)  ((BSE_OBJECT_FLAGS (pdev) & BSE_PCM_FLAG_STATE_SYNC) != 0)


/* --- PcmDevice flags --- */
typedef enum
{
  BSE_PCM_FLAG_HAS_CAPS   = (1 << (BSE_OBJECT_FLAGS_USHIFT + 0)),
  BSE_PCM_FLAG_OPEN	  = (1 << (BSE_OBJECT_FLAGS_USHIFT + 1)),
  BSE_PCM_FLAG_READABLE   = (1 << (BSE_OBJECT_FLAGS_USHIFT + 2)),
  BSE_PCM_FLAG_WRITABLE   = (1 << (BSE_OBJECT_FLAGS_USHIFT + 3)),
  BSE_PCM_FLAG_REGISTERED = (1 << (BSE_OBJECT_FLAGS_USHIFT + 4)),
  BSE_PCM_FLAG_STATE_SYNC = (1 << (BSE_OBJECT_FLAGS_USHIFT + 5))
} BsePcmFlags;
#define BSE_PCM_FLAGS_USHIFT     (BSE_OBJECT_FLAGS_USHIFT + 6)


/* --- possible frequencies --- */
typedef enum
{
  BSE_PCM_FREQ_8000     = (1 <<  1),	  /*   8000 Hz */
  BSE_PCM_FREQ_11025    = (1 <<  2),	  /*  11025 Hz */
  BSE_PCM_FREQ_16000    = (1 <<  3),      /*  16000 Hz */
  BSE_PCM_FREQ_22050    = (1 <<  4),      /*  22050 Hz */
  BSE_PCM_FREQ_32000    = (1 <<  5),      /*  32000 Hz */
  BSE_PCM_FREQ_44100    = (1 <<  6),      /*  44100 Hz */
  BSE_PCM_FREQ_48000    = (1 <<  7),      /*  48000 Hz */
  BSE_PCM_FREQ_88200    = (1 <<  8),      /*  88200 Hz */
  BSE_PCM_FREQ_96000    = (1 <<  9),      /*  96000 Hz */
  BSE_PCM_FREQ_176400   = (1 << 10),      /* 176400 Hz */
  BSE_PCM_FREQ_192000   = (1 << 11),      /* 192000 Hz */
  BSE_PCM_FREQ_MIN	= BSE_PCM_FREQ_8000,
  BSE_PCM_FREQ_MAX	= BSE_PCM_FREQ_192000,
  BSE_PCM_FREQ_LAST_BIT = 12
} BsePcmFreqMask;


/* --- BsePcmDevice structs --- */
typedef struct _BsePcmCapabilities BsePcmCapabilities;
typedef struct _BsePcmDevice       BsePcmDevice;
typedef struct _BsePcmDeviceClass  BsePcmDeviceClass;
struct _BsePcmCapabilities
{
  guint		 writable : 1;
  guint		 readable : 1;
  guint		 duplex : 1;
  guint          max_n_channels;
  BsePcmFreqMask playback_freq_mask;
  BsePcmFreqMask capture_freq_mask;
  guint          max_fragment_size;
};
struct _BsePcmDevice
{
  BseObject	     parent_object;

  BseErrorType	     last_error;
  BsePcmCapabilities caps;

  GPollFD	     pfd;

  /* current state */
  guint              n_channels;
  gdouble            sample_freq;

  /* state */
  guint              playback_buffer_size;
  guint		     n_playback_bytes;	/* left to write */
  guint              capture_buffer_size;
  guint		     n_capture_bytes;	/* left to read */

  GSList	    *iqueue;	/* of type BseChunk* */
  GSList	    *oqueue;	/* of type BseChunk* */
};
struct _BsePcmDeviceClass
{
  BseObjectClass  parent_class;

  gchar*	(*device_name)	(BsePcmDevice	*pdev,
				 gboolean	 descriptive);
  BseErrorType	(*update_caps)	(BsePcmDevice	*pdev);
  BseErrorType	(*open)		(BsePcmDevice	*pdev,
				 gboolean	 readable,
				 gboolean	 writable,
				 guint           n_channels,
				 BsePcmFreqMask  rate,
				 guint           fragment_size);
  void		(*update_state)	(BsePcmDevice	*pdev);
  void		(*retrigger)	(BsePcmDevice	*pdev);
  guint		(*read)		(BsePcmDevice	*pdev,
				 guint		 n_bytes,
				 guint8		*bytes);
  guint		(*write)	(BsePcmDevice	*pdev,
				 guint		 n_bytes,
				 guint8		*bytes);
  void		(*close)	(BsePcmDevice	*pdev);
};


/* --- prototypes --- */
gchar*		bse_pcm_device_get_device_name   (BsePcmDevice	 *pdev);
gchar*		bse_pcm_device_get_device_blurb  (BsePcmDevice	 *pdev);
BseErrorType 	bse_pcm_device_update_caps	 (BsePcmDevice	 *pdev);
void	 	bse_pcm_device_invalidate_caps	 (BsePcmDevice	 *pdev);
BseErrorType 	bse_pcm_device_open		 (BsePcmDevice	 *pdev,
						  gboolean	  readable,
						  gboolean	  writable,
						  guint           n_channels,
						  gdouble         sample_freq);
void	     	bse_pcm_device_close		 (BsePcmDevice	 *pdev);
void	     	bse_pcm_device_retrigger	 (BsePcmDevice	 *pdev);
void	     	bse_pcm_device_read		 (BsePcmDevice	 *pdev,
						  guint		  n_values,
						  BseSampleValue *values);
void	     	bse_pcm_device_write		 (BsePcmDevice	 *pdev,
						  guint           n_values,
						  BseSampleValue *values);
void		bse_pcm_device_time_warp	 (BsePcmDevice	 *pdev);
void		bse_pcm_device_update_state	 (BsePcmDevice	 *pdev);
gulong		bse_pcm_device_n_values_to_msecs (BsePcmDevice	 *pdev,
						  gulong	  n_values);
gulong		bse_pcm_device_msecs_to_n_values (BsePcmDevice	 *pdev,
						  gulong	  msecs);


/* --- queue/process API --- */
void		bse_pcm_device_iqueue_push	 (BsePcmDevice	 *pdev,
						  BseChunk	 *chunk);
BseChunk*	bse_pcm_device_iqueue_peek	 (BsePcmDevice	 *pdev);
void		bse_pcm_device_iqueue_pop	 (BsePcmDevice	 *pdev);
void		bse_pcm_device_oqueue_push	 (BsePcmDevice	 *pdev,
						  BseChunk	 *chunk);
BseChunk*	bse_pcm_device_oqueue_peek	 (BsePcmDevice	 *pdev);
void		bse_pcm_device_oqueue_pop	 (BsePcmDevice	 *pdev);
gulong		bse_pcm_device_need_processing	 (BsePcmDevice	 *pdev,
						  gulong	  latency);
gboolean	bse_pcm_device_process		 (BsePcmDevice	 *pdev,
						  gulong          latency);


/* --- frequency utilities --- */
gdouble	       bse_pcm_freq_to_freq		 (BsePcmFreqMask  pcm_freq);
BsePcmFreqMask bse_pcm_freq_from_freq		 (gdouble	  freq);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_PCM_DEVICE_H__ */
