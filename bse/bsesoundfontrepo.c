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
#include        "bsesoundfontrepo.h"
#include        "bsesoundfont.h"
#include        "bsesoundfontpreset.h"
#include        "bsedefs.h"
#include        "bseblockutils.hh"


/* --- parameters --- */
enum
{
  PARAM_0,
};


/* --- prototypes --- */
static void	bse_sound_font_repo_class_init		(BseSoundFontRepoClass	*class);
static void	bse_sound_font_repo_init		(BseSoundFontRepo	*wrepo);
static void	bse_sound_font_repo_dispose		(GObject		*object);
static void     bse_sound_font_repo_release_children    (BseContainer		*container);
static void	bse_sound_font_repo_set_property	(GObject                *object,
							 guint			 param_id,
							 const GValue		*value,
							 GParamSpec		*pspec);
static void	bse_sound_font_repo_get_property (GObject               *object,
						  guint			 param_id,
						  GValue		*value,
						  GParamSpec		*pspec);
static void     bse_sound_font_repo_add_item     (BseContainer		*container,
						  BseItem		*item);
static void     bse_sound_font_repo_forall_items (BseContainer		*container,
						  BseForallItemsFunc	 func,
						  gpointer		 data);
static void     bse_sound_font_repo_remove_item	 (BseContainer		*container,
						  BseItem		*item);
static void     bse_sound_font_repo_prepare      (BseSource             *source);


/* --- variables --- */
static GTypeClass     *parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseSoundFontRepo)
{
  GType sound_font_repo_type;

  static const GTypeInfo sfrepo_info = {
    sizeof (BseSoundFontRepoClass),

    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_sound_font_repo_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,

    sizeof (BseSoundFontRepo),
    0,
    (GInstanceInitFunc) bse_sound_font_repo_init,
  };

  sound_font_repo_type = bse_type_register_static (BSE_TYPE_SUPER,
					           "BseSoundFontRepo",
					           "BSE Sound Font Repository",
                                                   __FILE__, __LINE__,
                                                   &sfrepo_info);
  return sound_font_repo_type;
}

static void
bse_sound_font_repo_class_init (BseSoundFontRepoClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseContainerClass *container_class = BSE_CONTAINER_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  gobject_class->set_property = bse_sound_font_repo_set_property;
  gobject_class->get_property = bse_sound_font_repo_get_property;
  gobject_class->dispose = bse_sound_font_repo_dispose;

  container_class->add_item = bse_sound_font_repo_add_item;
  container_class->remove_item = bse_sound_font_repo_remove_item;
  container_class->forall_items = bse_sound_font_repo_forall_items;
  container_class->release_children = bse_sound_font_repo_release_children;

  source_class->prepare = bse_sound_font_repo_prepare;
}

static void
bse_sound_font_repo_init (BseSoundFontRepo *sfrepo)
{
  sfi_mutex_init (&sfrepo->fluid_synth_mutex);

  sfrepo->n_oscs = 0;
  sfrepo->oscs = NULL;
  sfrepo->channel_map = NULL;

  sfrepo->fluid_settings = new_fluid_settings();
  sfrepo->fluid_synth = new_fluid_synth (sfrepo->fluid_settings);
  sfrepo->fluid_events = NULL;
  sfrepo->sound_fonts = NULL;
  sfrepo->fluid_mix_freq = 0;

  sfrepo->n_fluid_channels = 0;
  sfrepo->channel_values_left = NULL;
  sfrepo->channel_values_right = NULL;
  sfrepo->n_silence_samples = NULL;

  sfrepo->n_channel_oscs_active = 0;
  sfrepo->channel_values_tick_stamp = 0;
}

static gboolean
reload_sound_font (BseItem *item,
                   gpointer data)
{
  BseSoundFont *sound_font = BSE_SOUND_FONT (item);
  bse_sound_font_reload (sound_font);
  return TRUE;
}

static gboolean
unload_sound_font (BseItem *item,
                   gpointer data)
{
  BseSoundFont *sound_font = BSE_SOUND_FONT (item);
  bse_sound_font_unload (sound_font);
  return TRUE;
}

static void
bse_sound_font_repo_prepare (BseSource *source)
{
  BseSoundFontRepo *sfrepo = BSE_SOUND_FONT_REPO (source);
  int i, channels_required = 0;
  for (i = 0; i < sfrepo->n_oscs; i++)
    {
      if (sfrepo->oscs[i])
	sfrepo->channel_map[i] = channels_required++;
    }
  int mix_freq = bse_engine_sample_freq();
  if (sfrepo->n_fluid_channels != channels_required || sfrepo->fluid_mix_freq != mix_freq)
    {
      for (i = channels_required; i < sfrepo->n_fluid_channels; i++) // n_fluid_channels > channels_required
	{
	  g_free (sfrepo->channel_values_left[i]);
	  g_free (sfrepo->channel_values_right[i]);
	}
      sfrepo->channel_values_left = (float **)g_realloc (sfrepo->channel_values_left, sizeof (float *) * channels_required);
      sfrepo->channel_values_right = (float **)g_realloc (sfrepo->channel_values_right, sizeof (float *) * channels_required);
      sfrepo->n_silence_samples = (gint *) g_realloc (sfrepo->n_silence_samples, sizeof (gint) * channels_required);
      for (i = sfrepo->n_fluid_channels; i < channels_required; i++) // n_fluid_channels < channels_required
	{
	  sfrepo->channel_values_left[i] = g_new0 (float, BSE_STREAM_MAX_VALUES);
	  sfrepo->channel_values_right[i] = g_new0 (float, BSE_STREAM_MAX_VALUES);
	  sfrepo->n_silence_samples[i] = 0;
	}
      sfrepo->n_fluid_channels = channels_required;
      sfrepo->fluid_mix_freq = mix_freq;

      fluid_settings_setnum (sfrepo->fluid_settings, "synth.sample-rate", mix_freq);
      fluid_settings_setint (sfrepo->fluid_settings, "synth.midi-channels", channels_required);
      fluid_settings_setint (sfrepo->fluid_settings, "synth.audio-channels", channels_required);
      fluid_settings_setint (sfrepo->fluid_settings, "synth.audio-groups", channels_required);
      fluid_settings_setstr (sfrepo->fluid_settings, "synth.reverb.active", "no");
      fluid_settings_setstr (sfrepo->fluid_settings, "synth.chorus.active", "no");

      bse_sound_font_repo_forall_items (BSE_CONTAINER (sfrepo), unload_sound_font, sfrepo);
      if (sfrepo->fluid_synth)
	delete_fluid_synth (sfrepo->fluid_synth);

      sfrepo->fluid_synth = new_fluid_synth (sfrepo->fluid_settings);
      bse_sound_font_repo_forall_items (BSE_CONTAINER (sfrepo), reload_sound_font, sfrepo);
    }

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

static void
bse_sound_font_repo_release_children (BseContainer *container)
{
  BseSoundFontRepo *sfrepo = BSE_SOUND_FONT_REPO (container);

  while (sfrepo->sound_fonts)
    bse_container_remove_item (container, sfrepo->sound_fonts->data);

  /* chain parent class' handler */
  BSE_CONTAINER_CLASS (parent_class)->release_children (container);
}

static void
bse_sound_font_repo_dispose (GObject *object)
{
  BseSoundFontRepo *sfrepo = BSE_SOUND_FONT_REPO (object);
  int i;

  bse_sound_font_repo_forall_items (BSE_CONTAINER (sfrepo), unload_sound_font, sfrepo);

  if (sfrepo->fluid_synth)
    {
      delete_fluid_synth (sfrepo->fluid_synth);
      sfrepo->fluid_synth = NULL;
    }
  if (sfrepo->fluid_settings)
    {
      delete_fluid_settings (sfrepo->fluid_settings);
      sfrepo->fluid_settings = NULL;
    }
  g_free (sfrepo->channel_map);
  sfrepo->channel_map = NULL;
  g_free (sfrepo->oscs);
  sfrepo->oscs = NULL;
  for (i = 0; i < sfrepo->n_fluid_channels; i++)
    {
      g_free (sfrepo->channel_values_left[i]);
      g_free (sfrepo->channel_values_right[i]);
    }
  sfrepo->n_fluid_channels = 0;
  g_free (sfrepo->channel_values_left);
  sfrepo->channel_values_left = NULL;
  g_free (sfrepo->channel_values_right);
  sfrepo->channel_values_right = NULL;
  g_free (sfrepo->n_silence_samples);
  sfrepo->n_silence_samples = NULL;

  if (sfrepo->fluid_events != NULL)
    g_warning (G_STRLOC ": fluid event queue should be empty in dispose");

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
bse_sound_font_repo_set_property (GObject      *object,
			          guint         param_id,
			          const GValue *value,
			          GParamSpec   *pspec)
{
  BseSoundFontRepo *sfrepo = BSE_SOUND_FONT_REPO (object);
  switch (param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (sfrepo, param_id, pspec);
      break;
    }
}

static void
bse_sound_font_repo_get_property (GObject      *object,
			          guint         param_id,
			          GValue       *value,
			          GParamSpec   *pspec)
{
  BseSoundFontRepo *sfrepo = BSE_SOUND_FONT_REPO (object);
  switch (param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (sfrepo, param_id, pspec);
      break;
    }
}

static void
bse_sound_font_repo_add_item (BseContainer *container,
			      BseItem      *item)
{
  BseSoundFontRepo *sfrepo = BSE_SOUND_FONT_REPO (container);

  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_SOUND_FONT))
    sfrepo->sound_fonts = g_list_append (sfrepo->sound_fonts, item);
  else
    g_warning ("BseSoundFontRepo: cannot hold non-sound-font item type `%s'",
	       BSE_OBJECT_TYPE_NAME (item));

  /* chain parent class' add_item handler */
  BSE_CONTAINER_CLASS (parent_class)->add_item (container, item);
}

static void
bse_sound_font_repo_forall_items (BseContainer      *container,
			          BseForallItemsFunc func,
			          gpointer           data)
{
  BseSoundFontRepo *sfrepo = BSE_SOUND_FONT_REPO (container);
  GList *list;

  list = sfrepo->sound_fonts;
  while (list)
    {
      BseItem *item;

      item = list->data;
      list = list->next;
      if (!func (item, data))
	return;
    }
}

static void
bse_sound_font_repo_remove_item (BseContainer *container,
			         BseItem      *item)
{
  BseSoundFontRepo *sfrepo = BSE_SOUND_FONT_REPO (container);

  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_SOUND_FONT))
    sfrepo->sound_fonts = g_list_remove (sfrepo->sound_fonts, item);
  else
    g_warning ("BseSoundFontRepo: cannot hold non-sound-font item type `%s'",
	       BSE_OBJECT_TYPE_NAME (item));

  /* chain parent class' remove_item handler */
  BSE_CONTAINER_CLASS (parent_class)->remove_item (container, item);
}

static gboolean
gather_presets (BseItem  *item,
                gpointer  items)
{
  if (BSE_IS_SOUND_FONT (item) || BSE_IS_SOUND_FONT_REPO (item))
    bse_container_forall_items (BSE_CONTAINER (item), gather_presets, items);
  else if (BSE_IS_SOUND_FONT_PRESET (item))
    bse_item_seq_append (items, item);
  else
    g_warning ("Searching for sound font presets, an unexpected `%s' item was found", BSE_OBJECT_TYPE_NAME (item));
  return TRUE;
}

void
bse_sound_font_repo_list_all_presets (BseSoundFontRepo *sfrepo,
                                      BseItemSeq       *items)
{
  gather_presets (BSE_ITEM (sfrepo), items);
}

fluid_synth_t *
bse_sound_font_repo_lock_fluid_synth (BseSoundFontRepo *sfrepo)
{
  sfi_mutex_lock (&sfrepo->fluid_synth_mutex);
  return sfrepo->fluid_synth;
}

void
bse_sound_font_repo_unlock_fluid_synth (BseSoundFontRepo *sfrepo)
{
  sfi_mutex_unlock (&sfrepo->fluid_synth_mutex);
}

int
bse_sound_font_repo_add_osc (BseSoundFontRepo *sfrepo,
                             BseSoundFontOsc  *osc)
{
  int i;
  for (i = 0; i < sfrepo->n_oscs; i++)
    {
      if (sfrepo->oscs[i] == 0)
	{
	  sfrepo->oscs[i] = osc;
	  return i;
	}
    }
  sfrepo->oscs = (BseSoundFontOsc **)g_realloc (sfrepo->oscs, sizeof (BseSoundFontOsc *) * (i + 1));
  sfrepo->oscs[i] = osc;
  sfrepo->channel_map = g_realloc (sfrepo->channel_map, sizeof (guint) * (i + 1));
  return sfrepo->n_oscs++;
}

void
bse_sound_font_repo_remove_osc (BseSoundFontRepo *sfrepo,
                                int               osc_id)
{
  g_return_if_fail (osc_id >= 0 && osc_id < sfrepo->n_oscs);

  sfrepo->oscs[osc_id] = 0;
}
