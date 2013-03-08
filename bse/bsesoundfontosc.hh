// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_SOUND_FONT_OSC_HH__
#define __BSE_SOUND_FONT_OSC_HH__

#include <bse/bsesource.hh>
#include <bse/bsesoundfontpreset.hh>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */




/* --- object type macros --- */
#define BSE_TYPE_SOUND_FONT_OSC		      (BSE_TYPE_ID (BseSoundFontOsc))
#define BSE_SOUND_FONT_OSC(object)	      (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SOUND_FONT_OSC, BseSoundFontOsc))
#define BSE_SOUND_FONT_OSC_CLASS(class)	      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SOUND_FONT_OSC, BseSoundFontOscClass))
#define BSE_IS_SOUND_FONT_OSC(object)	      (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SOUND_FONT_OSC))
#define BSE_IS_SOUND_FONT_OSC_CLASS(class)    (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SOUND_FONT_OSC))
#define BSE_SOUND_FONT_OSC_GET_CLASS(object)  (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SOUND_FONT_OSC, BseSoundFontOscClass))

enum
{
  BSE_SOUND_FONT_OSC_OCHANNEL_LEFT_OUT,
  BSE_SOUND_FONT_OSC_OCHANNEL_RIGHT_OUT,
  BSE_SOUND_FONT_OSC_OCHANNEL_DONE_OUT,
  BSE_SOUND_FONT_OSC_N_OCHANNELS
};

/* --- BseSoundFontOsc source --- */
typedef struct _BseSoundFontOsc	      BseSoundFontOsc;
typedef struct _BseSoundFontOscClass  BseSoundFontOscClass;
typedef struct _BseSoundFontOscConfig BseSoundFontOscConfig;
struct _BseSoundFontOscConfig
{
  int			osc_id;
  int			sfont_id;
  int			bank;
  int			program;
  int                   silence_bound;
  BseSoundFontRepo     *sfrepo;

  int                   update_preset;  /* preset changed indicator */
};
struct _BseSoundFontOsc
{
  BseSource		parent_object;
  BseSoundFontPreset   *preset;
  BseSoundFontOscConfig	config;
};
struct _BseSoundFontOscClass
{
  BseSourceClass parent_class;
};



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_SOUND_FONT_OSC_HH__ */
