// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_SOUND_FONT_PRESET_HH__
#define __BSE_SOUND_FONT_PRESET_HH__

#include	<bse/bseitem.hh>
#include        <fluidsynth.h>

/* --- BSE type macros --- */
#define BSE_TYPE_SOUND_FONT_PRESET		(BSE_TYPE_ID (BseSoundFontPreset))
#define BSE_SOUND_FONT_PRESET(object)		(G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SOUND_FONT_PRESET, BseSoundFontPreset))
#define BSE_SOUND_FONT_PRESET_CLASS(class)	(G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SOUND_FONT_PRESET, BseSoundFontPresetClass))
#define BSE_IS_SOUND_FONT_PRESET(object)	(G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SOUND_FONT_PRESET))
#define BSE_IS_SOUND_FONT_PRESET_CLASS(class)	(G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SOUND_FONT_PRESET))
#define BSE_SOUND_FONT_PRESET_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SOUND_FONT_PRESET, BseSoundFontPresetClass))

struct BseSoundFontPreset : BseItem {
  int           program;
  int           bank;
};
struct BseSoundFontPresetClass : BseItemClass
{};

void   bse_sound_font_preset_init_preset (BseSoundFontPreset *self,
					  fluid_preset_t     *fluid_preset);

#endif /* __BSE_SOUND_FONT_PRESET_HH__ */
