/* BseConstant - BSE Constant
 * Copyright (C) 1999-2002 Tim Janik
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
#ifndef __BSE_CONSTANT_H__
#define __BSE_CONSTANT_H__

#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */




/* --- object type macros --- */
#define BSE_TYPE_CONSTANT              (BSE_TYPE_ID (BseConstant))
#define BSE_CONSTANT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_CONSTANT, BseConstant))
#define BSE_CONSTANT_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_CONSTANT, BseConstantClass))
#define BSE_IS_CONSTANT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_CONSTANT))
#define BSE_IS_CONSTANT_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_CONSTANT))
#define BSE_CONSTANT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_CONSTANT, BseConstantClass))

#define	BSE_CONSTANT_N_OUTPUTS	(4)

/* --- BseConstant source --- */
typedef struct _BseConstant      BseConstant;
typedef struct _BseConstantClass BseConstantClass;
struct _BseConstant
{
  BseSource       parent_object;

  gfloat	  constants[BSE_CONSTANT_N_OUTPUTS];
};
struct _BseConstantClass
{
  BseSourceClass parent_class;
};



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_CONSTANT_H__ */
