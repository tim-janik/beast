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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * bsepcmdevice-alsa.h: pcm device implementation for ALSA driver
 */
#ifndef __BSE_PCM_DEVICE_ALSA_H__
#define __BSE_PCM_DEVICE_ALSA_H__

#include        <bse/bsepcmdevice.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_PCM_DEVICE_ALSA              (BSE_TYPE_ID (BsePcmDeviceAlsa))
#define BSE_PCM_DEVICE_ALSA(object)           (BSE_CHECK_STRUCT_CAST ((object), BSE_TYPE_PCM_DEVICE_ALSA, BsePcmDeviceAlsa))
#define BSE_PCM_DEVICE_ALSA_CLASS(class)      (BSE_CHECK_CLASS_CAST ((class), BSE_TYPE_PCM_DEVICE_ALSA, BsePcmDeviceAlsaClass))
#define BSE_IS_PCM_DEVICE_ALSA(object)        (BSE_CHECK_STRUCT_TYPE ((object), BSE_TYPE_PCM_DEVICE_ALSA))
#define BSE_IS_PCM_DEVICE_ALSA_CLASS(class)   (BSE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PCM_DEVICE_ALSA))
#define BSE_PCM_DEVICE_ALSA_GET_CLASS(object) ((BsePcmDeviceAlsaClass*) (((BseObject*) (object))->bse_struct.bse_class))


/* --- BsePcmDeviceAlsa object --- */
typedef struct _BsePcmDeviceAlsa      BsePcmDeviceAlsa;
typedef struct _BsePcmDeviceAlsaClass BsePcmDeviceAlsaClass;






#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_PCM_DEVICE_ALSA_H__ */
