// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_PCM_DEVICE_JACK_HH__
#define __BSE_PCM_DEVICE_JACK_HH__

#include <bse/bsepcmdevice.hh>
#include <bse/bseplugin.hh>
#include <jack/jack.h>

/* --- object type macros --- */
#define BSE_TYPE_PCM_DEVICE_JACK              (bse_pcm_device_jack_get_type())
#define BSE_PCM_DEVICE_JACK(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_PCM_DEVICE_JACK, BsePcmDeviceJACK))
#define BSE_PCM_DEVICE_JACK_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_PCM_DEVICE_JACK, BsePcmDeviceJACKClass))
#define BSE_IS_PCM_DEVICE_JACK(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_PCM_DEVICE_JACK))
#define BSE_IS_PCM_DEVICE_JACK_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PCM_DEVICE_JACK))
#define BSE_PCM_DEVICE_JACK_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_PCM_DEVICE_JACK, BsePcmDeviceJACKClass))

/* --- BsePcmDeviceJACK object --- */
struct BsePcmDeviceJACK : BsePcmDevice
{
  BsePcmDevice    parent_object;
  jack_client_t  *jack_client;
};
struct BsePcmDeviceJACKClass : BsePcmDeviceClass
{
};

#endif /* __BSE_PCM_DEVICE_JACK_HH__ */
