// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_CONSTANT_H__
#define __BSE_CONSTANT_H__

#include <bse/bsesource.hh>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */




/* --- object type macros --- */
#define BSE_TYPE_CONSTANT              (BSE_TYPE_ID (BseConstant))
#define BSE_CONSTANT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_CONSTANT, BseConstant))
#define BSE_CONSTANT_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_CONSTANT, BseConstantClass))
#define BSE_IS_CONSTANT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_CONSTANT))
#define BSE_IS_CONSTANT_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_CONSTANT))
#define BSE_CONSTANT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_CONSTANT, BseConstantClass))
#define	BSE_CONSTANT_N_OUTPUTS	(4)
/* --- BseConstant source --- */
struct BseConstant;
struct BseConstantClass;
struct BseConstant : BseSource {
  gfloat	  constants[BSE_CONSTANT_N_OUTPUTS];
};
struct BseConstantClass : BseSourceClass
{};

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __BSE_CONSTANT_H__ */
