// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_PCM_INPUT_H__
#define __BSE_PCM_INPUT_H__

#include <bse/bsesource.hh>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_PCM_INPUT		(BSE_TYPE_ID (BsePcmInput))
#define BSE_PCM_INPUT(object)		(G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_PCM_INPUT, BsePcmInput))
#define BSE_PCM_INPUT_CLASS(class)	(G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_PCM_INPUT, BsePcmInputClass))
#define BSE_IS_PCM_INPUT(object)	(G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_PCM_INPUT))
#define BSE_IS_PCM_INPUT_CLASS(class)	(G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PCM_INPUT))
#define BSE_PCM_INPUT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_PCM_INPUT, BsePcmInputClass))
/* --- BsePcmInput source --- */

struct BsePcmInput : BseSource
{
  gfloat	  volume_factor;
  /* PREPARED */
  BseModule	 *uplink;
};
struct BsePcmInputClass : BseSourceClass
{};

enum
{
  BSE_PCM_INPUT_OCHANNEL_LEFT,
  BSE_PCM_INPUT_OCHANNEL_RIGHT,
  BSE_PCM_INPUT_N_OCHANNELS
};


G_END_DECLS

#endif /* __BSE_PCM_INPUT_H__ */
