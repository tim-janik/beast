// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __DAV_SYNDRUM_H__
#define __DAV_SYNDRUM_H__

#include <bse/bseplugin.hh>
#include <bse/bsesource.hh>

G_BEGIN_DECLS


/* --- object type macros --- */
#define DAV_TYPE_SYN_DRUM              (dav_syn_drum_get_type())
#define DAV_SYN_DRUM(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), DAV_TYPE_SYN_DRUM, DavSynDrum))
#define DAV_SYN_DRUM_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), DAV_TYPE_SYN_DRUM, DavSynDrumClass))
#define DAV_IS_SYN_DRUM(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), DAV_TYPE_SYN_DRUM))
#define DAV_IS_SYN_DRUM_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), DAV_TYPE_SYN_DRUM))
#define DAV_SYN_DRUM_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), DAV_TYPE_SYN_DRUM, DavSynDrumClass))

struct DavSynDrumParams {
  gfloat freq;
  gfloat trigger_vel;
  gfloat ratio;
  gfloat res; /* sample rate specific */
};
struct DavSynDrum : BseSource {
  DavSynDrumParams params; /* .res is unused (due to its sample rate dependency) */
  gfloat           half;
  gboolean         force_trigger;
};
struct DavSynDrumClass : BseSourceClass
{};
struct DavSynDrumModule {
  DavSynDrumParams params;
  gfloat last_trigger_level;
  gfloat spring_vel;
  gfloat spring_pos;
  gfloat env;
  gfloat freq_rad;
  gfloat freq_shift;
};

enum
{
  DAV_SYN_DRUM_ICHANNEL_FREQ,
  DAV_SYN_DRUM_ICHANNEL_RATIO,
  DAV_SYN_DRUM_ICHANNEL_TRIGGER,
  DAV_SYN_DRUM_N_ICHANNELS
};
enum
{
  DAV_SYN_DRUM_OCHANNEL_MONO,
  DAV_SYN_DRUM_N_OCHANNELS
};

G_END_DECLS

#endif /* __DAV_SYNDRUM_H__ */
