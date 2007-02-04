/* BseLoopback - BSE Recording output source
 * Copyright (C) 1999 Tim Janik
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
 * bseloopback.h: BseLoopback simply puts out it's input
 */
#ifndef __BSE_LOOPBACK_H__
#define __BSE_LOOPBACK_H__

#include <bse/bseplugin.h>
#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */





/* --- object type macros --- */
#define BSE_TYPE_LOOPBACK              (type_id_loopback)
#define BSE_LOOPBACK(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_LOOPBACK, BseLoopback))
#define BSE_LOOPBACK_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_LOOPBACK, BseLoopbackClass))
#define BSE_IS_LOOPBACK(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_LOOPBACK))
#define BSE_IS_LOOPBACK_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_LOOPBACK))
#define BSE_LOOPBACK_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_LOOPBACK, BseLoopbackClass))


/* --- BseLoopback source --- */
typedef struct _BseLoopback      BseLoopback;
typedef struct _BseLoopbackClass BseLoopbackClass;
struct _BseLoopback
{
  BseSource       parent_object;
};
struct _BseLoopbackClass
{
  BseSourceClass parent_class;
};


/* --- channels --- */
enum
{
  BSE_LOOPBACK_ICHANNEL_NONE,
  BSE_LOOPBACK_ICHANNEL_MONO,
  BSE_LOOPBACK_ICHANNEL_STEREO
};
enum
{
  BSE_LOOPBACK_OCHANNEL_NONE,
  BSE_LOOPBACK_OCHANNEL_MONO,
  BSE_LOOPBACK_OCHANNEL_STEREO
};




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_LOOPBACK_H__ */
