// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_PCM_DEVICE_ALSA_H__
#define __BSE_PCM_DEVICE_ALSA_H__
#include <bse/bsepcmdevice.hh>
#include <bse/bseplugin.hh>
G_BEGIN_DECLS

#define BSE_TYPE_PCM_DEVICE_ALSA              (bse_pcm_device_alsa_get_type())
#define BSE_PCM_DEVICE_ALSA(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_PCM_DEVICE_ALSA, BsePcmDeviceALSA))
#define BSE_PCM_DEVICE_ALSA_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_PCM_DEVICE_ALSA, BsePcmDeviceALSAClass))
#define BSE_IS_PCM_DEVICE_ALSA(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_PCM_DEVICE_ALSA))
#define BSE_IS_PCM_DEVICE_ALSA_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PCM_DEVICE_ALSA))
#define BSE_PCM_DEVICE_ALSA_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_PCM_DEVICE_ALSA, BsePcmDeviceALSAClass))

struct BsePcmDeviceALSA : BsePcmDevice
{};
struct BsePcmDeviceALSAClass : BsePcmDeviceClass
{};

G_END_DECLS

#endif /* __BSE_PCM_DEVICE_ALSA_H__ */
