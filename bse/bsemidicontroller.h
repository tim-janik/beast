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
#ifndef __BSE_MIDI_CONTROLLER_H__
#define __BSE_MIDI_CONTROLLER_H__

#include <bse/bsesource.h>
#include <bse/bsemidievent.h>

G_BEGIN_DECLS


/* --- object type macros --- */
#define BSE_TYPE_MIDI_CONTROLLER	      (BSE_TYPE_ID (BseMidiController))
#define BSE_MIDI_CONTROLLER(object)	      (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_MIDI_CONTROLLER, BseMidiController))
#define BSE_MIDI_CONTROLLER_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_MIDI_CONTROLLER, BseMidiControllerClass))
#define BSE_IS_CONTROLLER(object)	      (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_MIDI_CONTROLLER))
#define BSE_IS_CONTROLLER_CLASS(class)	      (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_MIDI_CONTROLLER))
#define BSE_MIDI_CONTROLLER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_MIDI_CONTROLLER, BseMidiControllerClassOut))


/* --- BseMidiController source --- */
typedef struct _BseMidiController      BseMidiController;
typedef struct _BseMidiControllerClass BseMidiControllerClass;
struct _BseMidiController
{
  BseSource          parent_object;
  
  guint		     midi_channel;
  BseMidiSignalType  controls[4];
};
struct _BseMidiControllerClass
{
  BseSourceClass  parent_class;
};


/* --- channels --- */
enum
{
  BSE_MIDI_CONTROLLER_OCHANNEL_CONTROL1,
  BSE_MIDI_CONTROLLER_OCHANNEL_CONTROL2,
  BSE_MIDI_CONTROLLER_OCHANNEL_CONTROL3,
  BSE_MIDI_CONTROLLER_OCHANNEL_CONTROL4,
  BSE_MIDI_CONTROLLER_N_OCHANNELS
};


G_END_DECLS

#endif /* __BSE_MIDI_CONTROLLER_H__ */
