/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1999, 2000-2002 Tim Janik
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
 */
#ifndef __BSE_IN_PORT_H__
#define __BSE_IN_PORT_H__

#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_IN_PORT		(BSE_TYPE_ID (BseInPort))
#define BSE_IN_PORT(object)		(G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_IN_PORT, BseInPort))
#define BSE_IN_PORT_CLASS(class)	(G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_IN_PORT, BseInPortClass))
#define BSE_IS_PORT(object)		(G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_IN_PORT))
#define BSE_IS_PORT_CLASS(class)	(G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_IN_PORT))
#define BSE_IN_PORT_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_IN_PORT, BseInPortClass))


/* --- BseInPort module --- */
typedef struct _BseInPort      BseInPort;
typedef struct _BseInPortClass BseInPortClass;
struct _BseInPort
{
  BseSource	 parent_object;
  
  gchar		*port_name;
};
struct _BseInPortClass
{
  BseSourceClass parent_class;
};


/* --- channels --- */
enum
{
  BSE_IN_PORT_OCHANNEL_VIN
};


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_IN_PORT_H__ */
