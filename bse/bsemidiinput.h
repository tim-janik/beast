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
#ifndef __BSE_MIDI_INPUT_H__
#define __BSE_MIDI_INPUT_H__

#include <bse/bsesource.h>
#include <bse/bsemidievent.h>


G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_MIDI_INPUT	         (BSE_TYPE_ID (BseMidiInput))
#define BSE_MIDI_INPUT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_MIDI_INPUT, BseMidiInput))
#define BSE_MIDI_INPUT_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_MIDI_INPUT, BseMidiInputClass))
#define BSE_IS_INPUT(object)	         (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_MIDI_INPUT))
#define BSE_IS_INPUT_CLASS(class)	 (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_MIDI_INPUT))
#define BSE_MIDI_INPUT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_MIDI_INPUT, BseMidiInputClassOut))


/* --- BseMidiInput source --- */
typedef struct _BseMidiInput      BseMidiInput;
typedef struct _BseMidiInputClass BseMidiInputClass;
struct _BseMidiInput
{
  BseSource          parent_object;
  
  guint		     midi_channel;
};
struct _BseMidiInputClass
{
  BseSourceClass  parent_class;
};


/* --- channels --- */
enum
{
  BSE_MIDI_INPUT_OCHANNEL_FREQUENCY,
  BSE_MIDI_INPUT_OCHANNEL_GATE,
  BSE_MIDI_INPUT_OCHANNEL_VELOCITY,
  BSE_MIDI_INPUT_OCHANNEL_AFTERTOUCH,
  BSE_MIDI_INPUT_N_OCHANNELS
};


G_END_DECLS

#endif /* __BSE_MIDI_INPUT_H__ */
