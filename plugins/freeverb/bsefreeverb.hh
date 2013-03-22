// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_FREE_VERB_H__
#define __BSE_FREE_VERB_H__
#define  BSE_PLUGIN_NAME  "BseFreeVerb"
#include <bse/bseplugin.hh>
#include <bse/bsesource.hh>
#include "bsefreeverbcpp.hh"
G_BEGIN_DECLS

#define BSE_TYPE_FREE_VERB              (bse_free_verb_get_type())
#define BSE_FREE_VERB(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_FREE_VERB, BseFreeVerb))
#define BSE_FREE_VERB_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_FREE_VERB, BseFreeVerbClass))
#define BSE_IS_FREE_VERB(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_FREE_VERB))
#define BSE_IS_FREE_VERB_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_FREE_VERB))
#define BSE_FREE_VERB_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_FREE_VERB, BseFreeVerbClass))

struct BseFreeVerb : BseSource {
  BseFreeVerbConfig config;
};
struct BseFreeVerbClass : BseSourceClass {
  BseFreeVerbConstants constants;
};

enum /*< skip >*/
{
  BSE_FREE_VERB_ICHANNEL_LEFT,
  BSE_FREE_VERB_ICHANNEL_RIGHT,
  BSE_FREE_VERB_N_ICHANNELS
};
enum /*< skip >*/
{
  BSE_FREE_VERB_OCHANNEL_LEFT,
  BSE_FREE_VERB_OCHANNEL_RIGHT,
  BSE_FREE_VERB_N_OCHANNELS
};
G_END_DECLS
#endif /* __BSE_FREE_VERB_H__ */
