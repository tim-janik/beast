/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BSE_MIDI_NOTIFIER_H__
#define __BSE_MIDI_NOTIFIER_H__

#include <bse/bseitem.h>
#include <bse/bsemidievent.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_MIDI_NOTIFIER              (BSE_TYPE_ID (BseMidiNotifier))
#define BSE_MIDI_NOTIFIER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_MIDI_NOTIFIER, BseMidiNotifier))
#define BSE_MIDI_NOTIFIER_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_MIDI_NOTIFIER, BseMidiNotifierClass))
#define BSE_IS_MIDI_NOTIFIER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_MIDI_NOTIFIER))
#define BSE_IS_MIDI_NOTIFIER_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_MIDI_NOTIFIER))
#define BSE_MIDI_NOTIFIER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_MIDI_NOTIFIER, BseMidiNotifierClass))


/* --- BseMidiNotifier structs --- */
struct _BseMidiNotifier
{
  BseItem	parent_instance;
};
struct _BseMidiNotifierClass
{
  BseItemClass	parent_class;
  
  void	(*midi_event)	(BseMidiNotifier	*self,
			 BseMidiEvent		*event);
};


/* --- prototypes --- */
gboolean	bse_midi_notifier_needs_dispatch	(BseMidiNotifier	*self);
void		bse_midi_notifier_dispatch		(BseMidiNotifier	*self);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_MIDI_NOTIFIER_H__ */
