// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_PCM_DEVICE_NULL_H__
#define __BSE_PCM_DEVICE_NULL_H__

#include        <bse/bsepcmdevice.hh>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_PCM_DEVICE_NULL              (BSE_TYPE_ID (BsePcmDeviceNull))
#define BSE_PCM_DEVICE_NULL(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_PCM_DEVICE_NULL, BsePcmDeviceNull))
#define BSE_PCM_DEVICE_NULL_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_PCM_DEVICE_NULL, BsePcmDeviceNullClass))
#define BSE_IS_PCM_DEVICE_NULL(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_PCM_DEVICE_NULL))
#define BSE_IS_PCM_DEVICE_NULL_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PCM_DEVICE_NULL))
#define BSE_PCM_DEVICE_NULL_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_PCM_DEVICE_NULL, BsePcmDeviceNullClass))


/* --- BsePcmDeviceNull object --- */

struct BsePcmDeviceNull : BsePcmDevice
{};
struct BsePcmDeviceNullClass : BsePcmDeviceClass
{};

G_END_DECLS
#endif /* __BSE_PCM_DEVICE_NULL_H__ */
