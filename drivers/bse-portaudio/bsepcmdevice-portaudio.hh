// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_PCM_DEVICE_PORTAUDIO_H__
#define __BSE_PCM_DEVICE_PORTAUDIO_H__
#include <bse/bsepcmdevice.hh>
#include <bse/bseplugin.hh>
G_BEGIN_DECLS
/* --- object type macros --- */
#define BSE_TYPE_PCM_DEVICE_PORT_AUDIO              (BSE_EXPORT_TYPE_ID (BsePcmDevicePortAudio))
#define BSE_PCM_DEVICE_PORT_AUDIO(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_PCM_DEVICE_PORT_AUDIO, BsePcmDevicePortAudio))
#define BSE_PCM_DEVICE_PORT_AUDIO_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_PCM_DEVICE_PORT_AUDIO, BsePcmDevicePortAudioClass))
#define BSE_IS_PCM_DEVICE_PORT_AUDIO(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_PCM_DEVICE_PORT_AUDIO))
#define BSE_IS_PCM_DEVICE_PORT_AUDIO_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PCM_DEVICE_PORT_AUDIO))
#define BSE_PCM_DEVICE_PORT_AUDIO_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_PCM_DEVICE_PORT_AUDIO, BsePcmDevicePortAudioClass))
/* --- BsePcmDevicePortAudio object --- */
typedef struct _BsePcmDevicePortAudio             BsePcmDevicePortAudio;
typedef struct _BsePcmDevicePortAudioClass	  BsePcmDevicePortAudioClass;
struct _BsePcmDevicePortAudio
{
  BsePcmDevice parent_object;
};
struct _BsePcmDevicePortAudioClass
{
  BsePcmDeviceClass parent_class;
};
G_END_DECLS
#endif /* __BSE_PCM_DEVICE_PORTAUDIO_H__ */
