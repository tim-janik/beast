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
#ifndef __BSE_SUB_KEYBOARD_H__
#define __BSE_SUB_KEYBOARD_H__

#include <bse/bsesubiport.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_SUB_KEYBOARD		   (BSE_TYPE_ID (BseSubKeyboard))
#define BSE_SUB_KEYBOARD(object)	   (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SUB_KEYBOARD, BseSubKeyboard))
#define BSE_SUB_KEYBOARD_CLASS(class)	   (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SUB_KEYBOARD, BseSubKeyboardClass))
#define BSE_IS_KEYBOARD(object)		   (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SUB_KEYBOARD))
#define BSE_IS_KEYBOARD_CLASS(class)	   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SUB_KEYBOARD))
#define BSE_SUB_KEYBOARD_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SUB_KEYBOARD, BseSubKeyboardClass))


/* --- BseSubKeyboard source --- */
typedef struct _BseSubKeyboard      BseSubKeyboard;
typedef struct _BseSubKeyboardClass BseSubKeyboardClass;
struct _BseSubKeyboard
{
  BseSubIPort parent_object;
};
struct _BseSubKeyboardClass
{
  BseSubIPortClass parent_class;
};


/* --- channels --- */
enum
{
  BSE_SUB_KEYBOARD_OCHANNEL_FREQUENCY,
  BSE_SUB_KEYBOARD_OCHANNEL_GATE,
  BSE_SUB_KEYBOARD_OCHANNEL_VELOCITY,
  BSE_SUB_KEYBOARD_OCHANNEL_AFTERTOUCH
};



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_SUB_KEYBOARD_H__ */
