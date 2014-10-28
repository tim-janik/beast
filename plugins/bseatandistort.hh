// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_ATAN_DISTORT_H__
#define __BSE_ATAN_DISTORT_H__

#include <bse/bseplugin.hh>
#include <bse/bsesource.hh>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_ATAN_DISTORT              (bse_atan_distort_get_type())
#define BSE_ATAN_DISTORT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_ATAN_DISTORT, BseAtanDistort))
#define BSE_ATAN_DISTORT_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_ATAN_DISTORT, BseAtanDistortClass))
#define BSE_IS_ATAN_DISTORT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_ATAN_DISTORT))
#define BSE_IS_ATAN_DISTORT_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_ATAN_DISTORT))
#define BSE_ATAN_DISTORT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_ATAN_DISTORT, BseAtanDistortClass))

struct BseAtanDistort : BseSource {
  float	 boost_amount;
  double prescale;
};
struct BseAtanDistortClass : BseSourceClass
{};

enum
{
  BSE_ATAN_DISTORT_ICHANNEL_MONO1,
  BSE_ATAN_DISTORT_N_ICHANNELS
};
enum
{
  BSE_ATAN_DISTORT_OCHANNEL_MONO1,
  BSE_ATAN_DISTORT_N_OCHANNELS
};



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_ATAN_DISTORT_H__ */
