// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_ADDER_H__
#define __BSE_ADDER_H__

#include <bse/bseplugin.hh>
#include <bse/bsesource.hh>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_ADDER              (bse_adder_get_type())
#define BSE_ADDER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_ADDER, BseAdder))
#define BSE_ADDER_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_ADDER, BseAdderClass))
#define BSE_IS_ADDER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_ADDER))
#define BSE_IS_ADDER_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_ADDER))
#define BSE_ADDER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_ADDER, BseAdderClass))

struct BseAdder : BseSource {
  gboolean	  subtract;
};
struct BseAdderClass : BseSourceClass {
  BseIcon	*sub_icon;
};

enum
{
  BSE_ADDER_JCHANNEL_AUDIO1,
  BSE_ADDER_JCHANNEL_AUDIO2,
  BSE_ADDER_N_JCHANNELS
};
enum
{
  BSE_ADDER_OCHANNEL_AUDIO_OUT,
  BSE_ADDER_N_OCHANNELS
};



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_ADDER_H__ */
