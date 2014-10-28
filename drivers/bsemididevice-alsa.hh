// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_MIDI_DEVICE_ALSA_H__
#define __BSE_MIDI_DEVICE_ALSA_H__
#include <bse/bsemididevice.hh>
#include <bse/bseplugin.hh>
G_BEGIN_DECLS

#define BSE_TYPE_MIDI_DEVICE_ALSA              (bse_midi_device_alsa_get_type())
#define BSE_MIDI_DEVICE_ALSA(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_MIDI_DEVICE_ALSA, BseMidiDeviceALSA))
#define BSE_MIDI_DEVICE_ALSA_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_MIDI_DEVICE_ALSA, BseMidiDeviceALSAClass))
#define BSE_IS_MIDI_DEVICE_ALSA(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_MIDI_DEVICE_ALSA))
#define BSE_IS_MIDI_DEVICE_ALSA_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_MIDI_DEVICE_ALSA))
#define BSE_MIDI_DEVICE_ALSA_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_MIDI_DEVICE_ALSA, BseMidiDeviceALSAClass))

struct BseMidiDeviceALSA : BseMidiDevice
{};
struct BseMidiDeviceALSAClass : BseMidiDeviceClass
{};

G_END_DECLS

#endif /* __BSE_MIDI_DEVICE_ALSA_H__ */
