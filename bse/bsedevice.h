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
 * bsedevice.h: BSE device base type
 */
#ifndef __BSE_DEVICE_H__
#define __BSE_DEVICE_H__

#include        <bse/bseobject.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_DEVICE              (BSE_TYPE_ID (BseDevice))
#define BSE_DEVICE(object)           (BSE_CHECK_STRUCT_CAST ((object), BSE_TYPE_DEVICE, BseDevice))
#define BSE_DEVICE_CLASS(class)      (BSE_CHECK_CLASS_CAST ((class), BSE_TYPE_DEVICE, BseDeviceClass))
#define BSE_IS_DEVICE(object)        (BSE_CHECK_STRUCT_TYPE ((object), BSE_TYPE_DEVICE))
#define BSE_IS_DEVICE_CLASS(class)   (BSE_CHECK_CLASS_TYPE ((class), BSE_TYPE_DEVICE))
#define BSE_DEVICE_GET_CLASS(object) ((BseDeviceClass*) (((BseObject*) (object))->bse_struct.bse_class))


/* --- object member/convenience macros --- */
#define BSE_DEVICE_OPEN(pdev)        ((BSE_OBJECT_FLAGS (pdev) & BSE_DEVICE_FLAG_OPEN) != 0)
#define BSE_DEVICE_READABLE(pdev)    ((BSE_OBJECT_FLAGS (pdev) & BSE_DEVICE_FLAG_READABLE) != 0)
#define BSE_DEVICE_WRITABLE(pdev)    ((BSE_OBJECT_FLAGS (pdev) & BSE_DEVICE_FLAG_WRITABLE) != 0)
#define BSE_DEVICE_REGISTERED(pdev)  ((BSE_OBJECT_FLAGS (pdev) & BSE_DEVICE_FLAG_REGISTERED) != 0)


/* --- BseDevice flags --- */
typedef enum
{
  BSE_DEVICE_FLAG_OPEN       = (1 << (BSE_OBJECT_FLAGS_USHIFT + 0)),
  BSE_DEVICE_FLAG_READABLE   = (1 << (BSE_OBJECT_FLAGS_USHIFT + 1)),
  BSE_DEVICE_FLAG_WRITABLE   = (1 << (BSE_OBJECT_FLAGS_USHIFT + 2)),
  BSE_DEVICE_FLAG_REGISTERED = (1 << (BSE_OBJECT_FLAGS_USHIFT + 3)),
} BseDeviceFlags;
#define BSE_DEVICE_FLAGS_USHIFT     (BSE_OBJECT_FLAGS_USHIFT + 4)


/* --- BseDevice structs --- */
typedef struct _BseDevice       BseDevice;
typedef struct _BseDeviceClass  BseDeviceClass;
struct _BseDevice
{
  BseObject	     parent_object;

  BseErrorType	     last_error;

  GPollFD	     pfd;
};
struct _BseDeviceClass
{
  BseObjectClass  parent_class;

  gchar*	(*device_name)	(BseDevice	*dev,
				 gboolean	 descriptive);
  guint		(*read)		(BseDevice	*dev,
				 guint		 n_bytes,
				 guint8		*bytes);
  guint		(*write)	(BseDevice	*dev,
				 guint		 n_bytes,
				 guint8		*bytes);
  void		(*close)	(BseDevice	*pdev);
};


/* --- prototypes --- */
gchar*		bse_device_get_device_name	 (BseDevice	 *dev);
gchar*		bse_device_get_device_blurb	 (BseDevice	 *dev);
void	     	bse_device_close		 (BseDevice	 *dev);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_DEVICE_H__ */
