/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1996-1999, 2000-2003 Tim Janik
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
 *
 * bsemidisynth.h: bse midi synthesizer
 */
#ifndef	__BSE_MIDI_SYNTH_H__
#define	__BSE_MIDI_SYNTH_H__

#include	<bse/bsesnet.h>
#include	<bse/bsesubsynth.h>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_MIDI_SYNTH	         (BSE_TYPE_ID (BseMidiSynth))
#define BSE_MIDI_SYNTH(object)	         (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_MIDI_SYNTH, BseMidiSynth))
#define BSE_MIDI_SYNTH_CLASS(class)	 (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_MIDI_SYNTH, BseMidiSynthClass))
#define BSE_IS_MIDI_SYNTH(object)	 (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_MIDI_SYNTH))
#define BSE_IS_MIDI_SYNTH_CLASS(class)	 (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_MIDI_SYNTH))
#define BSE_MIDI_SYNTH_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_MIDI_SYNTH, BseMidiSynthClass))


/* --- BseMidiSynth object --- */
struct _BseMidiSynth
{
  BseSNet	 parent_object;

  guint		 midi_channel_id;
  guint		 n_voices;
  gfloat	 volume_factor;         /* 1-based factor */

  BseSNet       *snet;
  BseSNet       *pnet;

  BseSource	*voice_input;
  BseSource	*voice_switch;
  BseSource	*context_merger;
  BseSource	*postprocess;
  BseSource	*output;
  BseSource	*sub_synth;
};
struct _BseMidiSynthClass
{
  BseSNetClass parent_class;
};


G_END_DECLS

#endif /* __BSE_MIDI_SYNTH_H__ */
