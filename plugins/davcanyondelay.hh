// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __DAV_CANYON_DELAY_H__
#define __DAV_CANYON_DELAY_H__
#include <bse/bseplugin.hh>
#include <bse/bsesource.hh>
G_BEGIN_DECLS

#define DAV_TYPE_CANYON_DELAY              (dav_canyon_delay_get_type())
#define DAV_CANYON_DELAY(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), DAV_TYPE_CANYON_DELAY, DavCanyonDelay))
#define DAV_CANYON_DELAY_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), DAV_TYPE_CANYON_DELAY, DavCanyonDelayClass))
#define DAV_IS_CANYON_DELAY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), DAV_TYPE_CANYON_DELAY))
#define DAV_IS_CANYON_DELAY_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), DAV_TYPE_CANYON_DELAY))
#define DAV_CANYON_DELAY_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), DAV_TYPE_CANYON_DELAY, DavCanyonDelayClass))

struct DavCanyonDelayParams {
  gdouble l_to_r_mag;
  gdouble l_to_r_invmag;
  gdouble r_to_l_mag;
  gdouble r_to_l_invmag;
  gint32  l_to_r_pos;
  gint32  r_to_l_pos;
  gdouble filter_mag;
  gdouble filter_invmag;
};
struct DavCanyonDelay : BseSource {
  gdouble l_to_r_seconds;
  gdouble l_to_r_feedback;
  gdouble r_to_l_seconds;
  gdouble r_to_l_feedback;
  gdouble filter_freq;
  DavCanyonDelayParams params;
};
struct DavCanyonDelayModule {
  gint32   pos;
  gint32   datasize;
  gdouble  accum_l;
  gdouble  accum_r;
  gdouble *data_l;
  gdouble *data_r;
  DavCanyonDelayParams params;
};
struct DavCanyonDelayClass : BseSourceClass
{};

enum
{
  DAV_CANYON_DELAY_ICHANNEL_LEFT,
  DAV_CANYON_DELAY_ICHANNEL_RIGHT,
  DAV_CANYON_DELAY_N_ICHANNELS
};
enum
{
  DAV_CANYON_DELAY_OCHANNEL_LEFT,
  DAV_CANYON_DELAY_OCHANNEL_RIGHT,
  DAV_CANYON_DELAY_N_OCHANNELS
};



G_END_DECLS

#endif /* __DAV_CANYON_DELAY_H__ */
