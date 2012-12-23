// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_MIXER_H__
#define __BSE_MIXER_H__
#include <bse/bseplugin.hh>
#include <bse/bsesource.hh>
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
/* --- object type macros --- */
#define BSE_TYPE_MIXER              (bse_mixer_get_type())
#define BSE_MIXER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_MIXER, BseMixer))
#define BSE_MIXER_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_MIXER, BseMixerClass))
#define BSE_IS_MIXER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_MIXER))
#define BSE_IS_MIXER_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_MIXER))
#define BSE_MIXER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_MIXER, BseMixerClass))
#define	BSE_MIXER_N_INPUTS	(4)
/* --- BseMixer source --- */
typedef struct _BseMixer      BseMixer;
typedef struct _BseMixerClass BseMixerClass;
struct _BseMixer
{
  BseSource       parent_object;
  gfloat	  master_volume_factor;
  gfloat          volume_factors[BSE_MIXER_N_INPUTS];
};
struct _BseMixerClass
{
  BseSourceClass parent_class;
};
/* --- channels --- */
enum
{
  BSE_MIXER_OCHANNEL_MONO
};
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __BSE_MIXER_H__ */
