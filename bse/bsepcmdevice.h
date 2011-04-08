/* BSE - Better Sound Engine
 * Copyright (C) 1998-1999, 2000-2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 *
 * bsepcmdevice.h: pcm device driver class
 */
#ifndef __BSE_PCM_DEVICE_H__
#define __BSE_PCM_DEVICE_H__

#include <bse/bsedevice.h>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_PCM_DEVICE              (BSE_TYPE_ID (BsePcmDevice))
#define BSE_PCM_DEVICE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_PCM_DEVICE, BsePcmDevice))
#define BSE_PCM_DEVICE_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_PCM_DEVICE, BsePcmDeviceClass))
#define BSE_IS_PCM_DEVICE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_PCM_DEVICE))
#define BSE_IS_PCM_DEVICE_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PCM_DEVICE))
#define BSE_PCM_DEVICE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_PCM_DEVICE, BsePcmDeviceClass))


/* --- capabilities --- */
#define	BSE_PCM_FREQ_MIN	BSE_PCM_FREQ_8000
#define	BSE_PCM_FREQ_MAX	BSE_PCM_FREQ_192000
typedef enum	/*< skip >*/
{
  BSE_PCM_CMODE_MONO	= 1,
  BSE_PCM_CMODE_STEREO
} BsePcmChannelMode;


/* --- BsePcmDevice structs --- */
typedef struct _BsePcmStatus		BsePcmStatus;
typedef struct _BsePcmHandle		BsePcmHandle;
typedef struct _BsePcmDevice		BsePcmDevice;
typedef struct _BsePcmDeviceClass	BsePcmDeviceClass;
struct _BsePcmHandle
{
  guint			 readable : 1;
  guint			 writable : 1;
  guint			 n_channels;    /* should be req_n_channels */
  guint 		 mix_freq;      /* should be req_mix_freq within 1% tolerance */
  guint                  block_length;  /* in frames, filled in after open() before i/o */
  BirnetMutex		 mutex;
  gsize	   (*read)	(BsePcmHandle		*handle,
			 gfloat			*values);       /* n_channels * block_length values */
  void	   (*write)	(BsePcmHandle		*handle,
			 const gfloat		*values);       /* n_channels * block_length values */
  gboolean (*check_io)	(BsePcmHandle		*handle,
                         glong                  *timeoutp);
  guint    (*latency)   (BsePcmHandle           *handle);
};
struct _BsePcmDevice
{
  BseDevice		parent_instance;

  /* requested caps */
  guint			req_n_channels;
  guint                 req_mix_freq;
  guint                 req_latency_ms;   /* latency in milliseconds */
  guint                 req_block_length; /* in frames, a guess at block_length after open() */

  /* operational handle */
  BsePcmHandle	       *handle;
};
struct _BsePcmDeviceClass
{
  BseDeviceClass	parent_class;
};


/* --- prototypes --- */
void		bse_pcm_device_request		(BsePcmDevice		*pdev,
						 guint			 n_channels,
                                                 guint                   mix_freq,
                                                 guint                   latency_ms,
                                                 guint                   block_length); /* in frames */
guint           bse_pcm_device_get_mix_freq     (BsePcmDevice           *pdev);
BsePcmHandle*	bse_pcm_device_get_handle	(BsePcmDevice		*pdev,
                                                 guint                   block_length);
gsize		bse_pcm_handle_read		(BsePcmHandle		*handle,
						 gsize			 n_values,
						 gfloat			*values);
void		bse_pcm_handle_write		(BsePcmHandle		*handle,
						 gsize			 n_values,
						 const gfloat		*values);
gboolean        bse_pcm_handle_check_io		(BsePcmHandle		*handle,
                                                 glong                  *timeoutp);
guint           bse_pcm_handle_latency          (BsePcmHandle           *handle);


/* --- misc utils --- */
guint		bse_pcm_device_frequency_align 	(gint                    mix_freq);
     

G_END_DECLS

#endif /* __BSE_PCM_DEVICE_H__ */
