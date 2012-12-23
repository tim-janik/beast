// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_MIDI_NOTIFIER_H__
#define __BSE_MIDI_NOTIFIER_H__
#include <bse/bseitem.hh>
#include <bse/bsemidireceiver.hh>
G_BEGIN_DECLS
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
  BseItem	   parent_instance;
  BseMidiReceiver *midi_receiver;
};
struct _BseMidiNotifierClass
{
  BseItemClass	parent_class;
  void	(*midi_event)	(BseMidiNotifier	*self,
			 BseMidiEvent		*event);
};
/* --- prototypes --- */
void bse_midi_notifier_set_receiver     (BseMidiNotifier      *self,
                                         BseMidiReceiver      *midi_receiver);
void bse_midi_notifier_dispatch         (BseMidiNotifier      *self);
void bse_midi_notifiers_attach_source   (void);
void bse_midi_notifiers_wakeup          (void);
G_END_DECLS
#endif /* __BSE_MIDI_NOTIFIER_H__ */
