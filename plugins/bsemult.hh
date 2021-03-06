// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_MULT_H__
#define __BSE_MULT_H__

#include <bse/bseplugin.hh>
#include <bse/bsesource.hh>


/* --- object type macros --- */
#define BSE_TYPE_MULT              (bse_mult_get_type())
#define BSE_MULT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_MULT, BseMult))
#define BSE_MULT_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_MULT, BseMultClass))
#define BSE_IS_MULT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_MULT))
#define BSE_IS_MULT_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_MULT))
#define BSE_MULT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_MULT, BseMultClass))

struct BseMult : BseSource
{};

struct BseMultClass : BseSourceClass
{};

enum
{
  BSE_MULT_ICHANNEL_MONO1,
  BSE_MULT_ICHANNEL_MONO2,
  BSE_MULT_ICHANNEL_MONO3,
  BSE_MULT_ICHANNEL_MONO4
};
enum
{
  BSE_MULT_OCHANNEL_MONO
};





#endif /* __BSE_MULT_H__ */
