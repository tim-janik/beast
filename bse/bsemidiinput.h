/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1999, 2000-2003 Tim Janik
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
#ifndef __BSE_MONO_KEYBOARD_H__
#define __BSE_MONO_KEYBOARD_H__

#include <bse/bsesource.h>
#include <bse/bsemidievent.h>


G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_MONO_KEYBOARD	            (BSE_TYPE_ID (BseMonoKeyboard))
#define BSE_MONO_KEYBOARD(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_MONO_KEYBOARD, BseMonoKeyboard))
#define BSE_MONO_KEYBOARD_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_MONO_KEYBOARD, BseMonoKeyboardClass))
#define BSE_IS_KEYBOARD(object)	            (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_MONO_KEYBOARD))
#define BSE_IS_KEYBOARD_CLASS(class)	    (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_MONO_KEYBOARD))
#define BSE_MONO_KEYBOARD_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_MONO_KEYBOARD, BseMonoKeyboardClassOut))


/* --- BseMonoKeyboard source --- */
typedef struct _BseMonoKeyboard      BseMonoKeyboard;
typedef struct _BseMonoKeyboardClass BseMonoKeyboardClass;
struct _BseMonoKeyboard
{
  BseSource          parent_object;
  
  guint		     midi_channel;
};
struct _BseMonoKeyboardClass
{
  BseSourceClass  parent_class;
};


/* --- channels --- */
enum
{
  BSE_MONO_KEYBOARD_OCHANNEL_FREQUENCY,
  BSE_MONO_KEYBOARD_OCHANNEL_GATE,
  BSE_MONO_KEYBOARD_OCHANNEL_VELOCITY,
  BSE_MONO_KEYBOARD_OCHANNEL_AFTERTOUCH,
  BSE_MONO_KEYBOARD_N_OCHANNELS
};


G_END_DECLS

#endif /* __BSE_MONO_KEYBOARD_H__ */
