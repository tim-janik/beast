/* Testos - BSE test Plugin for objects
 * Copyright (C) 1999 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * testos.c: BSE test Plugin for objects
 */
#define BSE_PLUGIN_NAME "Test-Objects"
#include        <bse/bseplugin.h>

#include	<bse/bsesource.h>



/* --- Greeting --- */
static void
foo (void)
{
  g_message ("foo");
}
static void
testos_class_init (BseSourceClass *class)
{
  BseSourceClass *source_class = class;
  
  bse_source_class_add_ichannel (source_class,
				 "Any In", "A Test Input Channel",
				 1, 1);
  bse_source_class_add_ochannel (source_class,
                                 "Any Out", "A Test Output Channel",
				 1);
}
static BseType           type_id_testos = 0;
static const BseTypeInfo testos_type_info = {
  sizeof (BseSourceClass),
  
  (BseBaseInitFunc) foo,
  (BseBaseDestroyFunc) foo,
  (BseClassInitFunc) testos_class_init,
  (BseClassDestroyFunc) NULL,
  NULL /* class_data */,
  
  sizeof (BseSource),
  0 /* n_preallocs */,
  (BseObjectInitFunc) foo,
};
static BseType           type_id_mic = 0;
static BseType           type_id_amp = 0;
static BseType           type_id_speaker = 0;
static BseType           type_id_broken = 0;


/* --- Export to BSE --- */
#include "./icons/testos.c"
#include "./icons/mic.c"
#include "./icons/amp.c"
#include "./icons/speaker.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_testos, "BseTestos", "BseSource",
    "Testos is a test object",
    &testos_type_info,
    "/Source/Testos",
    { TESTOS_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      TESTOS_WIDTH, TESTOS_HEIGHT,
      TESTOS_RLE_PIXEL_DATA, },
  },
  { &type_id_mic, "BseMic", "BseSource",
    "Mic is a test object",
    &testos_type_info,
    "/Source/Mic",
    { MIC_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      MIC_WIDTH, MIC_HEIGHT,
      MIC_RLE_PIXEL_DATA, },
  },
  { &type_id_amp, "BseAmp", "BseSource",
    "Amp is a test object",
    &testos_type_info,
    "/Source/Amp",
    { AMP_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      AMP_WIDTH, AMP_HEIGHT,
      AMP_RLE_PIXEL_DATA, },
  },
  { &type_id_speaker, "BseSpeaker", "BseSource",
    "Speaker is a test object",
    &testos_type_info,
    "/Source/Speaker",
    { SPEAKER_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      SPEAKER_WIDTH, SPEAKER_HEIGHT,
      SPEAKER_RLE_PIXEL_DATA, },
  },
  { &type_id_broken, "BseBroken", "BseSource",
    "Broken is a broken test object",
    &testos_type_info,
    "/Source/Broken",
  },
  { NULL, },
};
BSE_EXPORTS_END;
