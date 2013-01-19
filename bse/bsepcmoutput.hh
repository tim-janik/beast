// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_PCM_OUTPUT_H__
#define __BSE_PCM_OUTPUT_H__
#include <bse/bsesource.hh>
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
/* --- object type macros --- */
#define BSE_TYPE_PCM_OUTPUT		 (BSE_TYPE_ID (BsePcmOutput))
#define BSE_PCM_OUTPUT(object)		 (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_PCM_OUTPUT, BsePcmOutput))
#define BSE_PCM_OUTPUT_CLASS(class)	 (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_PCM_OUTPUT, BsePcmOutputClass))
#define BSE_IS_PCM_OUTPUT(object)	 (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_PCM_OUTPUT))
#define BSE_IS_PCM_OUTPUT_CLASS(class)	 (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PCM_OUTPUT))
#define BSE_PCM_OUTPUT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_PCM_OUTPUT, BsePcmOutputClass))
/* --- BsePcmOutput source --- */
typedef struct _BsePcmOutput      BsePcmOutput;
typedef struct _BsePcmOutputClass BsePcmOutputClass;
struct _BsePcmOutput
{
  BseSource       parent_object;
  gfloat	  volume_factor;
  /* PREPARED */
  BseModule	 *uplink;
};
struct _BsePcmOutputClass
{
  BseSourceClass     parent_class;
};
/* --- channels --- */
enum
{
  BSE_PCM_OUTPUT_ICHANNEL_LEFT,
  BSE_PCM_OUTPUT_ICHANNEL_RIGHT,
  BSE_PCM_OUTPUT_N_ICHANNELS
};
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __BSE_PCM_OUTPUT_H__ */
