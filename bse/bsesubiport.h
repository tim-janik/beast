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
#ifndef __BSE_SUB_IPORT_H__
#define __BSE_SUB_IPORT_H__

#include <bse/bsesource.h>
#include <bse/bseengine.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_SUB_IPORT		(BSE_TYPE_ID (BseSubIPort))
#define BSE_SUB_IPORT(object)		(G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SUB_IPORT, BseSubIPort))
#define BSE_SUB_IPORT_CLASS(class)	(G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SUB_IPORT, BseSubIPortClass))
#define BSE_IS_IPORT(object)		(G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SUB_IPORT))
#define BSE_IS_IPORT_CLASS(class)	(G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SUB_IPORT))
#define BSE_SUB_IPORT_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SUB_IPORT, BseSubIPortClass))

#define	BSE_SUB_IPORT_N_PORTS	(4)

/* --- BseSubIPort module --- */
typedef struct _BseSubIPort      BseSubIPort;
typedef struct _BseSubIPortClass BseSubIPortClass;
struct _BseSubIPort
{
  BseSource	 parent_object;
  
  gchar	       **input_ports;
};
struct _BseSubIPortClass
{
  BseSourceClass parent_class;

  guint		 n_input_ports;
  BseModuleClass	 gsl_class;
};


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_SUB_IPORT_H__ */
