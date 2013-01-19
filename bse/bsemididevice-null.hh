// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef	__BSE_MIDI_DEVICE_NULL_H__
#define	__BSE_MIDI_DEVICE_NULL_H__
#include	<bse/bsemididevice.hh>
G_BEGIN_DECLS
/* --- object type macros --- */
#define BSE_TYPE_MIDI_DEVICE_NULL		(BSE_TYPE_ID (BseMidiDeviceNULL))
#define BSE_MIDI_DEVICE_NULL(object)		(G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_MIDI_DEVICE_NULL, BseMidiDeviceNULL))
#define BSE_MIDI_DEVICE_NULL_CLASS(class)	(G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_MIDI_DEVICE_NULL, BseMidiDeviceNULLClass))
#define BSE_IS_MIDI_DEVICE_NULL(object)		(G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_MIDI_DEVICE_NULL))
#define BSE_IS_MIDI_DEVICE_NULL_CLASS(class)	(G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_MIDI_DEVICE_NULL))
#define BSE_MIDI_DEVICE_NULL_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_MIDI_DEVICE_NULL, BseMidiDeviceNULLClass))
/* --- BseMidiDeviceNULL object --- */
typedef	struct _BseMidiDeviceNULL      BseMidiDeviceNULL;
typedef	struct _BseMidiDeviceNULLClass BseMidiDeviceNULLClass;
struct _BseMidiDeviceNULL
{
  BseMidiDevice parent_object;
};
struct _BseMidiDeviceNULLClass
{
  BseMidiDeviceClass parent_class;
};
G_END_DECLS
#endif /* __BSE_MIDI_DEVICE_NULL_H__ */
