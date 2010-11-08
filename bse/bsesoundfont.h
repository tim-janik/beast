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
#ifndef __BSE_SOUND_FONT_H__
#define __BSE_SOUND_FONT_H__

#include	<bse/bsecontainer.h>
#include        <bse/bsestorage.h>

G_BEGIN_DECLS

/* --- BSE type macros --- */
#define BSE_TYPE_SOUND_FONT		  (BSE_TYPE_ID (BseSoundFont))
#define BSE_SOUND_FONT(object)		  (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SOUND_FONT, BseSoundFont))
#define BSE_SOUND_FONT_CLASS(class)	  (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SOUND_FONT, BseSoundFontClass))
#define BSE_IS_SOUND_FONT(object)	  (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SOUND_FONT))
#define BSE_IS_SOUND_FONT_CLASS(class)	  (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SOUND_FONT))
#define BSE_SOUND_FONT_GET_CLASS(object)  (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SOUND_FONT, BseSoundFontClass))


/* --- BseSoundFont --- */
struct _BseSoundFont
{
  BseContainer	     parent_object;
  BseStorageBlob    *blob;
  int                sfont_id;
  BseSoundFontRepo  *sfrepo;
  GList             *presets;
};
struct _BseSoundFontClass
{
  BseContainerClass  parent_class;
};


/* --- prototypes -- */
BseErrorType    bse_sound_font_load_blob	(BseSoundFont       *sound_font,
						 BseStorageBlob     *blob,
						 gboolean            init_presets);
void		bse_sound_font_unload           (BseSoundFont       *sound_font);
BseErrorType    bse_sound_font_reload           (BseSoundFont       *sound_font);

G_END_DECLS

#endif /* __BSE_SOUND_FONT_H__ */
