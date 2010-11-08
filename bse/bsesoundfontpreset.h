/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2005 Tim Janik
 * Copyright (C) 2009 Stefan Westerfeld
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#ifndef __BSE_SOUND_FONT_PRESET_H__
#define __BSE_SOUND_FONT_PRESET_H__

#include	<bse/bseitem.h>
#include        <fluidsynth.h>

G_BEGIN_DECLS

/* --- BSE type macros --- */
#define BSE_TYPE_SOUND_FONT_PRESET		(BSE_TYPE_ID (BseSoundFontPreset))
#define BSE_SOUND_FONT_PRESET(object)		(G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SOUND_FONT_PRESET, BseSoundFontPreset))
#define BSE_SOUND_FONT_PRESET_CLASS(class)	(G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SOUND_FONT_PRESET, BseSoundFontPresetClass))
#define BSE_IS_SOUND_FONT_PRESET(object)	(G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SOUND_FONT_PRESET))
#define BSE_IS_SOUND_FONT_PRESET_CLASS(class)	(G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SOUND_FONT_PRESET))
#define BSE_SOUND_FONT_PRESET_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SOUND_FONT_PRESET, BseSoundFontPresetClass))


/* --- BseSoundFontPreset --- */
struct _BseSoundFontPreset
{
  BseItem	parent_object;
  int           program;
  int           bank;
};
struct _BseSoundFontPresetClass
{
  BseItemClass  parent_class;
};


/* --- prototypes -- */
void   bse_sound_font_preset_init_preset (BseSoundFontPreset *self,
					  fluid_preset_t     *fluid_preset);

G_END_DECLS

#endif /* __BSE_SOUND_FONT_PRESET_H__ */
