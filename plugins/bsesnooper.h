/* BseSnooper - BSE Snooper
 * Copyright (C) 1999, 2000-2001 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * bsesnooper.h: BSE Snooper - listen on incoming signal and printf stats
 */
#ifndef __BSE_SNOOPER_H__
#define __BSE_SNOOPER_H__

#define  BSE_PLUGIN_NAME  "BseSnooper"

#include <bse/bseplugin.h>
#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */





/* --- object type macros --- */
#define BSE_TYPE_SNOOPER              (type_id_snooper)
#define BSE_SNOOPER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SNOOPER, BseSnooper))
#define BSE_SNOOPER_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SNOOPER, BseSnooperClass))
#define BSE_IS_SNOOPER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SNOOPER))
#define BSE_IS_SNOOPER_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SNOOPER))
#define BSE_SNOOPER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SNOOPER, BseSnooperClass))


/* --- BseSnooper source --- */
typedef struct _BseSnooper     BseSnooper;
typedef struct _BseSourceClass BseSnooperClass;
struct _BseSnooper
{
  BseSource       parent_object;
};


/* --- channels --- */
enum
{
  BSE_SNOOPER_ICHANNEL_MONO
};





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_SNOOPER_H__ */
