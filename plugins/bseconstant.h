/* BseConstant - BSE Constant
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
 */
#ifndef __BSE_CONSTANT_H__
#define __BSE_CONSTANT_H__

#define  BSE_PLUGIN_NAME  "BseConstant"

#include <bse/bseplugin.h>
#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */





/* --- object type macros --- */
#define BSE_TYPE_CONSTANT              (type_id_constant)
#define BSE_CONSTANT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_CONSTANT, BseConstant))
#define BSE_CONSTANT_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_CONSTANT, BseConstantClass))
#define BSE_IS_CONSTANT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_CONSTANT))
#define BSE_IS_CONSTANT_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_CONSTANT))
#define BSE_CONSTANT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_CONSTANT, BseConstantClass))

#define	BSE_CONSTANT_N_INPUTS	(4)


/* --- BseConstant source --- */
typedef struct _BseConstant      BseConstant;
typedef struct _BseConstantClass BseConstantClass;
struct _BseConstant
{
  BseSource       parent_object;

  gfloat	  constant_value;
};
struct _BseConstantClass
{
  BseSourceClass parent_class;
};


/* --- channels --- */
enum
{
  BSE_CONSTANT_OCHANNEL_MONO
};





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_CONSTANT_H__ */
