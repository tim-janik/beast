/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1996-1999, 2000-2003 Tim Janik
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
#ifndef	__BSE_SOUND_FONT_REPO_H__
#define	__BSE_SOUND_FONT_REPO_H__

#include	<bse/bsesuper.h>
#include        <fluidsynth.h>
#include        <bse/bsesoundfontosc.h>
#include        <bse/bseengine.h>


G_BEGIN_DECLS


/* --- object type macros --- */
#define BSE_TYPE_SOUND_FONT_REPO	        (BSE_TYPE_ID (BseSoundFontRepo))
#define BSE_SOUND_FONT_REPO(object)	        (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SOUND_FONT_REPO, BseSoundFontRepo))
#define BSE_SOUND_FONT_REPO_CLASS(class)	(G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SOUND_FONT_REPO, BseSoundFontRepoClass))
#define BSE_IS_SOUND_FONT_REPO(object)		(G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SOUND_FONT_REPO))
#define BSE_IS_SOUND_FONT_REPO_CLASS(class)	(G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SOUND_FONT_REPO))
#define BSE_SOUND_FONT_REPO_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SOUND_FONT_REPO, BseSoundFontRepoClass))


/* --- BseSoundFontRepo object --- */
#define BSE_FLUID_SYNTH_PROGRAM_SELECT -1
typedef struct _BseFluidEvent BseFluidEvent;
struct _BseFluidEvent
{
  guint64            tick_stamp;
  int                channel;
  int		     command;
  int		     arg1;
  int		     arg2;
  int                sfont_id;	  /* required for program selection only */
};
struct _BseSoundFontRepo
{
  BseSuper	     parent_object;

  BirnetMutex	     fluid_synth_mutex;
  fluid_settings_t  *fluid_settings;
  fluid_synth_t     *fluid_synth;
  SfiRing           *fluid_events;
  guint              fluid_mix_freq;

  guint              n_fluid_channels;
  float		   **channel_values_left;     /* [0..n_fluid_channels-1] */
  float		   **channel_values_right;    /* [0..n_fluid_channels-1] */
  guint64	     channel_values_tick_stamp;
  gint              *n_silence_samples;       /* [0..n_fluid_channels-1] */

  guint              n_oscs;
  BseSoundFontOsc  **oscs;		      /* [0..n_oscs-1] */
  guint		    *channel_map;	      /* [0..n_oscs-1] */

  int		     n_channel_oscs_active;	  /* SoundFontOscs with an active module in the engine thread */

  GList             *sound_fonts;
};

struct _BseSoundFontRepoClass
{
  BseSuperClass  parent_class;
};


/* --- prototypes --- */
void	       bse_sound_font_repo_list_all_presets   (BseSoundFontRepo *sfrepo,
						       BseItemSeq       *items);
fluid_synth_t *bse_sound_font_repo_lock_fluid_synth   (BseSoundFontRepo *sfrepo);
void           bse_sound_font_repo_unlock_fluid_synth (BseSoundFontRepo *sfrepo);
int	       bse_sound_font_repo_add_osc            (BseSoundFontRepo *sfrepo,
						       BseSoundFontOsc  *osc);
void           bse_sound_font_repo_remove_osc         (BseSoundFontRepo *sfrepo,
						       int               osc_id);

G_END_DECLS

#endif /* __BSE_SOUND_FONT_REPO_H__ */
