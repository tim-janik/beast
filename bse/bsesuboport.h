/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1999, 2000-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __BSE_SUB_OPORT_H__
#define __BSE_SUB_OPORT_H__

#include <bse/bsesource.h>
#include <gsl/gslengine.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_SUB_OPORT		(BSE_TYPE_ID (BseSubOPort))
#define BSE_SUB_OPORT(object)		(G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SUB_OPORT, BseSubOPort))
#define BSE_SUB_OPORT_CLASS(class)	(G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SUB_OPORT, BseSubOPortClass))
#define BSE_IS_OPORT(object)		(G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SUB_OPORT))
#define BSE_IS_OPORT_CLASS(class)	(G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SUB_OPORT))
#define BSE_SUB_OPORT_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SUB_OPORT, BseSubOPortClass))

#define	BSE_SUB_OPORT_N_PORTS	(4)

/* --- BseSubOPort module --- */
typedef struct _BseSubOPort      BseSubOPort;
typedef struct _BseSubOPortClass BseSubOPortClass;
struct _BseSubOPort
{
  BseSource	 parent_object;
  
  gchar	       **output_ports;
};
struct _BseSubOPortClass
{
  BseSourceClass parent_class;

  guint          n_output_ports;
  GslClass       gsl_class;
};


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_SUB_OPORT_H__ */
