/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997, 1998, 1999 Olaf Hoehmann and Tim Janik
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
 * bsepcmstream-oss.h: pcm stream implementation for OSS Lite driver
 */
#ifndef	__BSE_PCM_STREAM_OSS_H__
#define	__BSE_PCM_STREAM_OSS_H__

#include	<bse/bsepcmstream.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_PCM_STREAM_OSS		     (BSE_TYPE_ID (BsePcmStreamOSS))
#define BSE_PCM_STREAM_OSS(object)	     (BSE_CHECK_STRUCT_CAST ((object), BSE_TYPE_PCM_STREAM_OSS, BsePcmStreamOSS))
#define BSE_PCM_STREAM_OSS_CLASS(class)	     (BSE_CHECK_CLASS_CAST ((class), BSE_TYPE_PCM_STREAM_OSS, BsePcmStreamOSSClass))
#define BSE_IS_PCM_STREAM_OSS(object)	     (BSE_CHECK_STRUCT_TYPE ((object), BSE_TYPE_PCM_STREAM_OSS))
#define BSE_IS_PCM_STREAM_OSS_CLASS(class)   (BSE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PCM_STREAM_OSS))
#define BSE_PCM_STREAM_OSS_GET_CLASS(object) ((BsePcmStreamOSSClass*) (((BseObject*) (object))->bse_struct.bse_class))


/* --- BsePcmStreamOSS object --- */
typedef	struct _BsePcmStreamOSS		BsePcmStreamOSS;
typedef	struct _BsePcmStreamOSSClass	BsePcmStreamOSSClass;


/* --- prototypes --- */
gint bse_pcm_stream_oss_get_unix_fd (BsePcmStreamOSS *pcm_stream_oss);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_PCM_STREAM_OSS_H__ */
