/* BseSoundFontOsc - BSE Fluid Synth sound font player
 * Copyright (C) 1999-2002 Tim Janik
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
#ifndef __BSE_SOUND_FONT_OSC_H__
#define __BSE_SOUND_FONT_OSC_H__

#include <bse/bsesource.h>
#include <bse/bsesoundfontpreset.h>

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

#endif /* __BSE_SOUND_FONT_OSC_H__ */
