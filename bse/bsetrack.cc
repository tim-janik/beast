// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsetrack.hh"
#include "bseglobals.hh"
#include "bsestorage.hh"
#include "bsecsynth.hh"
#include "bsewave.hh"
#include "bsepart.hh"
#include "bsebus.hh"
#include "bsesequencer.hh"
#include "gslcommon.hh"
#include "bsesubsynth.hh"
#include "bseproject.hh"
#include "bsesong.hh"
#include "bsemidivoice.hh"
#include "bsemidireceiver.hh"
#include "bsewaverepo.hh"
#include "bsesoundfontrepo.hh"
#include "bsesoundfontpreset.hh"
#include "bsesoundfont.hh"
#include "bsepart.hh"
#include "bseserver.hh"
#include "bsecxxplugin.hh"
#include "processor.hh"
#include "bse/internal.hh"
#include <string.h>

#define XREF_DEBUG(...) Bse::debug ("xref", __VA_ARGS__)

#define upper_power2(uint_n)	sfi_alloc_upper_power2 (MAX ((uint_n), 4))
#define parse_or_return		bse_storage_scanner_parse_or_return
#define peek_or_return		bse_storage_scanner_peek_or_return

enum {
  PROP_0,
  PROP_SNET,
  PROP_WAVE,
  PROP_SOUND_FONT_PRESET,
  PROP_PNET,
  PROP_OUTPUTS
};

/* --- prototypes --- */
static void         bse_track_class_init          (BseTrackClass *klass);
static void         bse_track_init                (BseTrack      *self);
static void         bse_track_dispose             (GObject       *object);
static void         bse_track_finalize            (GObject       *object);
static void         bse_track_set_property        (GObject       *object,
                                                   guint          param_id,
                                                   const GValue  *value,
                                                   GParamSpec    *pspec);
static void         bse_track_get_property        (GObject       *object,
                                                   guint          param_id,
                                                   GValue        *value,
                                                   GParamSpec    *pspec);
static void         bse_track_store_private       (BseObject     *object,
                                                   BseStorage    *storage);
static GTokenType   bse_track_restore_private     (BseObject     *object,
                                                   BseStorage    *storage,
                                                   GScanner      *scanner);
static void         bse_track_update_midi_channel (BseTrack      *self);


/* --- variables --- */
static GTypeClass *parent_class = NULL;

/* --- functions --- */
BSE_BUILTIN_TYPE (BseTrack)
{
  static const GTypeInfo track_info = {
    sizeof (BseTrackClass),

    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_track_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,

    sizeof (BseTrack),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_track_init,
  };

  return bse_type_register_static (BSE_TYPE_CONTEXT_MERGER,
				   "BseTrack",
				   "BSE track type",
                                   __FILE__, __LINE__,
                                   &track_info);
}

static gulong
alloc_id_above (guint n)
{
  gulong tmp, id = bse_id_alloc ();
  if (id > n)
    return id;
  tmp = id;
  id = alloc_id_above (n);
  bse_id_free (tmp);
  return id;
}

static void
bse_track_init (BseTrack *self)
{
  self->set_flag (BSE_SOURCE_FLAG_PRIVATE_INPUTS);
  self->snet = NULL;
  self->pnet = NULL;
  self->sound_font_preset = NULL;
  self->wave = NULL;
  self->wnet = NULL;
  self->max_voices = 1;
  self->muted_SL = FALSE;
  self->n_entries_SL = 0;
  self->entries_SL = g_renew (BseTrackEntry, NULL, upper_power2 (self->n_entries_SL));
  self->channel_id = alloc_id_above (BSE_MIDI_MAX_CHANNELS);
  self->midi_channel_SL = self->channel_id;
  self->track_done_SL = FALSE;
}

static void
bse_track_dispose (GObject *object)
{
  BseTrack *self = BSE_TRACK (object);

  /* we may assert removal here, since if these assertions fail,
   * our parent (BseSong) doesn't properly implement track support
   */
  assert_return (self->sub_synth == NULL);

  /* check uncrossed references */
  assert_return (self->snet == NULL);
  assert_return (self->pnet == NULL);
  assert_return (self->n_entries_SL == 0);

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (object);

  assert_return (self->bus_outputs == NULL);
}

static void
bse_track_finalize (GObject *object)
{
  BseTrack *self = BSE_TRACK (object);

  assert_return (self->bus_outputs == NULL);

  assert_return (self->n_entries_SL == 0);
  g_free (self->entries_SL);
  bse_id_free (self->channel_id);

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
bse_track_needs_storage (BseItem *item, BseStorage *storage)
{
  BseTrack *self = BSE_TRACK (item);
  return self->n_entries_SL > 0; // has parts
}

static void	track_uncross_part	(BseItem *owner,
					 BseItem *ref_item);

static BseTrackEntry*
track_add_entry (BseTrack *self,
		 guint     index,
		 guint     tick,
		 BsePart  *part)
{
  guint n, size;

  assert_return (index <= self->n_entries_SL, NULL);
  if (index > 0)
    assert_return (self->entries_SL[index - 1].tick < tick, NULL);
  if (index < self->n_entries_SL)
    assert_return (self->entries_SL[index].tick > tick, NULL);
  auto timpl = self->as<Bse::TrackImpl*>();

  BSE_SEQUENCER_LOCK ();
  n = self->n_entries_SL++;
  size = upper_power2 (self->n_entries_SL);
  if (size > upper_power2 (n))
    self->entries_SL = g_renew (BseTrackEntry, self->entries_SL, size);
  memmove (self->entries_SL + index + 1, self->entries_SL + index, (n - index) * sizeof (self->entries_SL[0]));
  self->entries_SL[index].tick = tick;
  self->entries_SL[index].id = bse_id_alloc ();
  self->entries_SL[index].part = part;
  self->track_done_SL = FALSE;	/* let sequencer recheck if playing */
  BSE_SEQUENCER_UNLOCK ();
  bse_item_cross_link (BSE_ITEM (self), BSE_ITEM (part), track_uncross_part);
  XREF_DEBUG ("cross-link: %p %p", self, part);
  auto pimpl = part->as<Bse::PartImpl*>();
  Aida::IfaceEventConnection con = pimpl->on ("notify:last_tick", [timpl] (const Aida::Event &event) { timpl->emit_event ("changed"); });
  self->entries_SL[index].c1 = new Aida::IfaceEventConnection (con);
  return self->entries_SL + index;
}

static void
track_delete_entry (BseTrack *self,
		    guint     index)
{
  assert_return (index < self->n_entries_SL);

  BsePart *part = self->entries_SL[index].part;
  self->entries_SL[index].c1->disconnect();
  delete self->entries_SL[index].c1;
  self->entries_SL[index].c1 = NULL;
  XREF_DEBUG ("cross-unlink: %p %p", self, part);
  bse_item_cross_unlink (BSE_ITEM (self), BSE_ITEM (part), track_uncross_part);
  BSE_SEQUENCER_LOCK ();
  self->n_entries_SL -= 1;
  bse_id_free (self->entries_SL[index].id);
  memmove (self->entries_SL + index, self->entries_SL + index + 1, (self->n_entries_SL - index) * sizeof (self->entries_SL[0]));
  BSE_SEQUENCER_UNLOCK ();
}

static BseTrackEntry*
track_lookup_entry (BseTrack *self,
		    guint     tick)
{
  BseTrackEntry *nodes = self->entries_SL;
  guint n = self->n_entries_SL, offs = 0, i = 0;

  while (offs < n)
    {
      gint cmp;
      i = (offs + n) >> 1;
      cmp = tick > nodes[i].tick ? +1 : tick < nodes[i].tick ? -1 : 0;
      if (!cmp)
	return nodes + i;
      else if (cmp < 0)
	n = i;
      else /* (cmp > 0) */
	offs = i + 1;
    }

  /* return the closest entry with tick <= requested tick if possible */
  if (!self->n_entries_SL)
    return NULL;
  if (nodes[i].tick > tick)
    return i > 0 ? nodes + i - 1 : NULL;	/* previous entry */
  else
    return nodes + i;				/* closest match */
}

static void
track_uncross_part (BseItem *owner,
		    BseItem *item)
{
  BseTrack *self = BSE_TRACK (owner);
  BsePart *part = BSE_PART (item);
  guint i;
  for (i = 0; i < self->n_entries_SL; i++)
    if (self->entries_SL[i].part == part)
      {
        guint tick = self->entries_SL[i].tick;
	XREF_DEBUG ("uncrossing[start]: %p %p (%d)", self, part, tick);
        // delete track via TrackImpl so deletion is recorded to undo
        Bse::TrackImpl *track = owner->as<Bse::TrackImpl*>();
        track->remove_tick (tick);
	XREF_DEBUG ("uncrossing[done]: %p %p (%d)", self, part, tick);
	return;
      }
}

static void
bse_track_get_candidates (BseItem *item, uint param_id, Bse::PropertyCandidates &pc, GParamSpec *pspec)
{
  BseTrack *self = BSE_TRACK (item);
  switch (param_id)
    {
      BseProject *project;
      SfiRing *ring;
    case PROP_WAVE:
      pc.label = _("Available Waves");
      pc.tooltip = _("List of available waves to choose as track instrument");
      project = bse_item_get_project (item);
      if (project)
	{
	  BseWaveRepo *wrepo = bse_project_get_wave_repo (project);
	  bse_item_gather_items_typed (BSE_ITEM (wrepo), pc.items, BSE_TYPE_WAVE, BSE_TYPE_WAVE_REPO, FALSE);
	}
      break;
    case PROP_SOUND_FONT_PRESET:
      pc.label = _("Sound Fonts");
      pc.tooltip = _("List of available sound font presets to choose as track instrument");
      project = bse_item_get_project (item);
      if (project)
	bse_sound_font_repo_list_all_presets (bse_project_get_sound_font_repo (project), pc.items);
      break;
    case PROP_SNET:
      pc.label = _("Available Synthesizers");
      pc.tooltip = _("List of available synthesis networks to choose a track instrument from");
      bse_item_gather_items_typed (item, pc.items, BSE_TYPE_CSYNTH, BSE_TYPE_PROJECT, FALSE);
      break;
    case PROP_PNET:
      pc.label = _("Available Postprocessors");
      pc.tooltip = _("List of available synthesis networks to choose a postprocessor from");
      bse_item_gather_items_typed (item, pc.items, BSE_TYPE_CSYNTH, BSE_TYPE_PROJECT, FALSE);
      break;
    case PROP_OUTPUTS:
      pc.label = _("Available Outputs");
      pc.tooltip = _("List of available mixer busses to be used as track output");
      bse_bus_or_track_list_output_candidates (BSE_ITEM (self), pc.items);
      /* remove existing outputs */
      for (ring = self->bus_outputs; ring; ring = sfi_ring_walk (ring, self->bus_outputs))
        vector_erase_iface (pc.items, ((BseItem*) ring->data)->as<Bse::ItemIface*>());
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
track_uncross_snet (BseItem *owner,
		    BseItem *ref_item)
{
  BseTrack *self = BSE_TRACK (owner);
  bse_item_set (self, "snet", NULL, NULL);
}

static void
track_uncross_pnet (BseItem *owner,
		    BseItem *ref_item)
{
  BseTrack *self = BSE_TRACK (owner);
  bse_item_set (self, "pnet", NULL, NULL);
}

static void
track_uncross_wave (BseItem *owner,
		    BseItem *ref_item)
{
  BseTrack *self = BSE_TRACK (owner);
  bse_item_set (self, "wave", NULL, NULL);
}

static void
set_amp_master_volume (BseSNet *snet, const char *amp_name, gchar **xinfos)
{
  const gchar *master_volume_db = bse_xinfos_get_value (xinfos, "master-volume-db");
  if (master_volume_db)
    {
      BseItem *amp = bse_container_resolve_upath (BSE_CONTAINER (snet), amp_name);
      double volume = 0;
      g_object_get (amp, "master-volume", &volume, NULL);
      volume *= bse_db_to_factor (g_ascii_strtod (master_volume_db, NULL));
      g_object_set (amp, "master-volume", volume, NULL);
    }
}

static void
set_adsr_params (BseSNet *snet, const char *adsr_name, gchar **xinfos)
{
  const gchar *adsr_release_time = bse_xinfos_get_value (xinfos, "adsr-release-time");
  if (adsr_release_time)
    {
      BseItem *adsr = bse_container_resolve_upath (BSE_CONTAINER (snet), adsr_name);
      g_object_set (adsr, "release_time", g_ascii_strtod (adsr_release_time, NULL), NULL);
    }
  const gchar *adsr_attack_time = bse_xinfos_get_value (xinfos, "adsr-attack-time");
  if (adsr_attack_time)
    {
      BseItem *adsr = bse_container_resolve_upath (BSE_CONTAINER (snet), adsr_name);
      g_object_set (adsr, "attack_time", g_ascii_strtod (adsr_attack_time, NULL), NULL);
    }
}

static void
track_uncross_sound_font_preset (BseItem *owner,
                                 BseItem *ref_item)
{
  BseTrack *self = BSE_TRACK (owner);
  bse_item_set (self, "sound-font-preset", NULL, NULL);
}

static void
create_wnet (BseTrack *self,
	     BseWave  *wave)
{
  assert_return (self->wnet == NULL);

  const gchar *play_type = bse_xinfos_get_value (wave->xinfos, "play-type");
  const gchar *synthesis_network = play_type ? play_type : "adsr-wave-1";

  self->wnet = bse_project_create_intern_synth (bse_item_get_project (BSE_ITEM (self)),
						synthesis_network,
						BSE_TYPE_SNET);

  bse_item_cross_link (BSE_ITEM (self), BSE_ITEM (self->wnet), track_uncross_wave);

  if (self->sub_synth)
    g_object_set (self->sub_synth, /* no undo */
		  "snet", self->wnet,
		  NULL);

  if (strcmp (synthesis_network, "adsr-wave-1") == 0 ||
      strcmp (synthesis_network, "plain-wave-1") == 0)
    {
      g_object_set (bse_container_resolve_upath (BSE_CONTAINER (self->wnet), "wave-osc"), /* no undo */
						 "wave", wave,
					         NULL);
      set_amp_master_volume (self->wnet, "amplifier", wave->xinfos);
      set_adsr_params (self->wnet, "adsr", wave->xinfos);
    }
  else if (strcmp (synthesis_network, "adsr-wave-2") == 0 ||
           strcmp (synthesis_network, "plain-wave-2") == 0)
    {
      g_object_set (bse_container_resolve_upath (BSE_CONTAINER (self->wnet), "wave-osc-left"), /* no undo */
						 "wave", wave,
					         NULL);
      g_object_set (bse_container_resolve_upath (BSE_CONTAINER (self->wnet), "wave-osc-right"), /* no undo */
						 "wave", wave,
					         NULL);
      set_amp_master_volume (self->wnet, "amplifier-left", wave->xinfos);
      set_amp_master_volume (self->wnet, "amplifier-right", wave->xinfos);
      set_adsr_params (self->wnet, "adsr", wave->xinfos);
    }
  else if (strcmp (synthesis_network, "gus-patch") == 0)
    {
      g_object_set (bse_container_resolve_upath (BSE_CONTAINER (self->wnet), "wave-osc"), /* no undo */
						 "wave", wave,
					         NULL);
      g_object_set (bse_container_resolve_upath (BSE_CONTAINER (self->wnet), "gus-patch-envelope"), /* no undo */
						 "wave", wave,
					         NULL);
    }
  else
    {
      Bse::warning ("track: waves with the play-type \"%s\" are not supported by this version of beast\n", synthesis_network);
    }

}

static void
create_sound_font_net (BseTrack           *self,
                       BseSoundFontPreset *preset)
{
  assert_return (self->sound_font_net == NULL);

  self->sound_font_net =  bse_project_create_intern_synth (bse_item_get_project (BSE_ITEM (self)),
							   "sound-font-snet",
							   BSE_TYPE_SNET);

  bse_item_cross_link (BSE_ITEM (self), BSE_ITEM (self->sound_font_net), track_uncross_sound_font_preset);

  if (self->sub_synth)
    {
      g_object_set (self->sub_synth, /* no undo */
		    "snet", self->sound_font_net,
		    NULL);
    }

  g_object_set (bse_container_resolve_upath (BSE_CONTAINER (self->sound_font_net), "sound-font-osc"), /* no undo */
		"preset", preset,
		NULL);
}
static void
clear_snet_and_wave_and_sfpreset (BseTrack *self)
{
  assert_return (!self->sub_synth || !BSE_SOURCE_PREPARED (self->sub_synth));

  if (self->sub_synth)
    g_object_set (self->sub_synth, /* no undo */
		  "snet", NULL,
		  NULL);
  if (self->snet)
    {
      bse_item_cross_unlink (BSE_ITEM (self), BSE_ITEM (self->snet), track_uncross_snet);
      self->snet = NULL;
      g_object_notify ((GObject*) self, "snet");
    }
  if (self->wave)
    {
      bse_item_cross_unlink (BSE_ITEM (self), BSE_ITEM (self->wave), track_uncross_wave);
      self->wave = NULL;
      g_object_notify ((GObject*) self, "wave");
    }
  if (self->wnet)
    {
      BseSNet *wnet = self->wnet;
      bse_item_cross_unlink (BSE_ITEM (self), BSE_ITEM (self->wnet), track_uncross_wave);
      self->wnet = NULL;
      bse_container_remove_item (BSE_CONTAINER (bse_item_get_project (BSE_ITEM (self))), BSE_ITEM (wnet));
    }
  if (self->sound_font_preset)
    {
      bse_item_cross_unlink (BSE_ITEM (self), BSE_ITEM (self->sound_font_preset), track_uncross_sound_font_preset);
      self->sound_font_preset = NULL;
      g_object_notify ((GObject *) self, "sound_font_preset");
    }
  if (self->sound_font_net)
    {
      BseSNet *sound_font_net = self->sound_font_net;
      bse_item_cross_unlink (BSE_ITEM (self), BSE_ITEM (self->sound_font_net), track_uncross_sound_font_preset);
      self->sound_font_net = NULL;
      bse_container_remove_item (BSE_CONTAINER (bse_item_get_project (BSE_ITEM (self))), BSE_ITEM (sound_font_net));
    }
}

static void
bse_track_set_property (GObject      *object,
			guint         param_id,
			const GValue *value,
			GParamSpec   *pspec)
{
  BseTrack *self = BSE_TRACK (object);

  switch (param_id)
    {
    case PROP_SNET:
      if (!self->sub_synth || !BSE_SOURCE_PREPARED (self))
	{
	  BseSNet *snet = (BseSNet*) bse_value_get_object (value);
	  if (snet || self->snet)
	    {
	      clear_snet_and_wave_and_sfpreset (self);
	      self->snet = snet;
	      if (self->snet)
		{
		  bse_item_cross_link (BSE_ITEM (self), BSE_ITEM (self->snet), track_uncross_snet);
		}
	      if (self->sub_synth)
		g_object_set (self->sub_synth, /* no undo */
			      "snet", self->snet,
			      NULL);
	    }
	}
      break;
    case PROP_WAVE:
      if (!self->sub_synth || !BSE_SOURCE_PREPARED (self->sub_synth))
	{
	  BseWave *wave = (BseWave*) bse_value_get_object (value);
	  if (wave || self->wave)
	    {
	      clear_snet_and_wave_and_sfpreset (self);

	      self->wave = wave;
	      if (self->wave)
		{
		  create_wnet (self, wave);
		  bse_item_cross_link (BSE_ITEM (self), BSE_ITEM (self->wave), track_uncross_wave);
		}
	    }
	}
      break;
    case PROP_SOUND_FONT_PRESET:
      if (!self->sub_synth || !BSE_SOURCE_PREPARED (self->sub_synth))
	{
	  BseSoundFontPreset *sound_font_preset = (BseSoundFontPreset *) bse_value_get_object (value);
	  if (sound_font_preset || self->sound_font_preset)
	    {
	      clear_snet_and_wave_and_sfpreset (self);

	      self->sound_font_preset = sound_font_preset;
	      if (self->sound_font_preset)
		{
		  create_sound_font_net (self, sound_font_preset);
		  bse_item_cross_link (BSE_ITEM (self), BSE_ITEM (self->sound_font_preset), track_uncross_sound_font_preset);
		}
	    }
	}
      break;
    case PROP_PNET:
      if (!self->postprocess || !BSE_SOURCE_PREPARED (self))
	{
          if (self->pnet)
            {
              bse_item_cross_unlink (BSE_ITEM (self), BSE_ITEM (self->pnet), track_uncross_pnet);
              self->pnet = NULL;
            }
          self->pnet = (BseSNet*) bse_value_get_object (value);
          if (self->pnet)
            {
              bse_item_cross_link (BSE_ITEM (self), BSE_ITEM (self->pnet), track_uncross_pnet);
            }
          if (self->postprocess)
            g_object_set (self->postprocess, /* no undo */
                          "snet", self->pnet,
                          NULL);
	}
      break;
    case PROP_OUTPUTS:
      {
#if 0
        BseIt3mSeq *i3s = (BseIt3mSeq*) g_value_get_boxed (value);
        Bse::ItemSeq items = bse_item_seq_from_it3m_seq (i3s);
        bse_bus_or_track_set_outputs (BSE_ITEM (self), items);
#endif
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_track_get_property (GObject    *object,
			guint       param_id,
			GValue     *value,
			GParamSpec *pspec)
{
  BseTrack *self = BSE_TRACK (object);

  switch (param_id)
    {
      // BseIt3mSeq *iseq;
      // SfiRing *ring;
    case PROP_SNET:
      bse_value_set_object (value, self->snet);
      break;
    case PROP_PNET:
      bse_value_set_object (value, self->pnet);
      break;
    case PROP_OUTPUTS:
#if 0
      iseq = bse_it3m_seq_new();
      for (ring = self->bus_outputs; ring; ring = sfi_ring_walk (ring, self->bus_outputs))
        bse_it3m_seq_append (iseq, (BseItem*) ring->data);
      g_value_take_boxed (value, iseq);
#endif
      break;
    case PROP_WAVE:
      bse_value_set_object (value, self->wave);
      break;
    case PROP_SOUND_FONT_PRESET:
      bse_value_set_object (value, self->sound_font_preset);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

guint
bse_track_insert_part (BseTrack *self,
		       guint     tick,
		       BsePart  *part)
{
  BseTrackEntry *entry;

  assert_return (BSE_IS_TRACK (self), 0);
  assert_return (BSE_IS_PART (part), 0);

  entry = track_lookup_entry (self, tick);
  if (entry && entry->tick == tick)
    return 0;
  else
    {
      gint i = entry ? entry - self->entries_SL : 0;
      entry = track_add_entry (self, entry ? i + 1 : 0, tick, part);
    }
  bse_part_links_changed (part);
  auto impl = self->as<Bse::TrackImpl*>();
  impl->notify ("changed");
  return entry ? entry->id : 0;
}

void
bse_track_remove_tick (BseTrack *self,
		       guint     tick)
{
  BseTrackEntry *entry;

  assert_return (BSE_IS_TRACK (self));

  entry = track_lookup_entry (self, tick);
  if (entry && entry->tick == tick)
    {
      BsePart *part = entry->part;
      track_delete_entry (self, entry - self->entries_SL);
      bse_part_links_changed (part);
      auto impl = self->as<Bse::TrackImpl*>();
      impl->notify ("changed");
    }
}

static Bse::TrackPartSeq
bse_track_list_parts_intern (BseTrack *self, BsePart *part)
{
  BseItem *item = BSE_ITEM (self);
  BseSong *song = NULL;
  if (BSE_IS_SONG (item->parent))
    song = BSE_SONG (item->parent);
  Bse::SongTiming timing;
  bse_song_timing_get_default (&timing);
  Bse::TrackPartSeq tps;
  gint i;
  for (i = 0; i < self->n_entries_SL; i++)
    {
      BseTrackEntry *entry = self->entries_SL + i;
      if (entry->part && (entry->part == part || !part))
	{
          Bse::TrackPart tp;
	  tp.tick = entry->tick;
	  tp.part = entry->part->as<Bse::PartIfaceP>();
	  if (song)
	    bse_song_get_timing (song, tp.tick, &timing);
	  tp.duration = MAX (uint (timing.tpt), entry->part->last_tick_SL);
	  if (i + 1 < self->n_entries_SL)
	    tp.duration = MIN (uint (tp.duration), entry[1].tick - entry->tick);
	  tps.push_back (tp);
	}
    }
  return tps;
}

Bse::TrackPartSeq
bse_track_list_parts (BseTrack *self)
{
  assert_return (BSE_IS_TRACK (self), Bse::TrackPartSeq());
  return bse_track_list_parts_intern (self, NULL);
}

Bse::TrackPartSeq
bse_track_list_part (BseTrack *self, BsePart *part)
{
  assert_return (BSE_IS_TRACK (self), Bse::TrackPartSeq());
  assert_return (BSE_IS_PART (part), Bse::TrackPartSeq());
  return bse_track_list_parts_intern (self, part);
}

gboolean
bse_track_find_part (BseTrack *self,
		     BsePart  *part,
		     guint    *start_p)
{
  guint i;

  assert_return (BSE_IS_TRACK (self), FALSE);
  assert_return (BSE_IS_PART (part), FALSE);

  for (i = 0; i < self->n_entries_SL; i++)
    if (self->entries_SL[i].part == part)
      {
	if (start_p)
	  *start_p = self->entries_SL[i].tick;
	return TRUE;
      }
  return FALSE;
}

BseTrackEntry*
bse_track_lookup_tick (BseTrack               *self,
		       guint                   tick)
{
  BseTrackEntry *entry;

  assert_return (BSE_IS_TRACK (self), NULL);

  entry = track_lookup_entry (self, tick);
  if (entry && entry->tick == tick)
    return entry;
  return NULL;
}

BseTrackEntry*
bse_track_find_link (BseTrack *self,
                     guint     id)
{
  guint i;

  assert_return (BSE_IS_TRACK (self), NULL);

  for (i = 0; i < self->n_entries_SL; i++)
    if (self->entries_SL[i].id == id)
      return self->entries_SL + i;
  return NULL;
}

BsePart*
bse_track_get_part_SL (BseTrack *self,
		       guint     tick,
		       guint    *start,
		       guint    *next)
{
  BseTrackEntry *entry;

  assert_return (BSE_IS_TRACK (self), NULL);

  /* we return the nearest part with start <= tick and
   * set *next to the start of the following part if any
   */

  entry = track_lookup_entry (self, tick);
  if (entry)
    {
      guint i = entry - self->entries_SL;
      if (i + 1 < self->n_entries_SL)
	*next = self->entries_SL[i + 1].tick;
      else
	*next = 0;	/* no next part */
      *start = entry->tick;
      return entry->part;
    }
  *start = 0;
  *next = self->n_entries_SL ? self->entries_SL[0].tick : 0;
  return NULL;
}

void
bse_track_add_modules (BseTrack        *self,
		       BseContainer    *container,
                       BseMidiReceiver *midi_receiver)
{
  assert_return (BSE_IS_TRACK (self));
  assert_return (BSE_IS_CONTAINER (container));
  assert_return (self->sub_synth == NULL);
  assert_return (midi_receiver != NULL);

  /* midi voice input */
  self->voice_input = (BseSource*) bse_container_new_child (container, BSE_TYPE_MIDI_VOICE_INPUT, NULL);
  bse_item_set_internal (self->voice_input, TRUE);

  /* sub synth */
  self->sub_synth = (BseSource*) bse_container_new_child_bname (container, BSE_TYPE_SUB_SYNTH, "Track-Instrument",
                                                                "in_port_1", "frequency",
                                                                "in_port_2", "gate",
                                                                "in_port_3", "velocity",
                                                                "in_port_4", "aftertouch",
                                                                "out_port_1", "left-audio",
                                                                "out_port_2", "right-audio",
                                                                "out_port_3", "unused",
                                                                "out_port_4", "synth-done",
                                                                "snet", self->snet,
                                                                NULL);
  bse_item_set_internal (self->sub_synth, TRUE);

  /* voice input <-> sub-synth */
  bse_source_must_set_input (self->sub_synth, 0,
			     self->voice_input, BSE_MIDI_VOICE_INPUT_OCHANNEL_FREQUENCY);
  bse_source_must_set_input (self->sub_synth, 1,
			     self->voice_input, BSE_MIDI_VOICE_INPUT_OCHANNEL_GATE);
  bse_source_must_set_input (self->sub_synth, 2,
			     self->voice_input, BSE_MIDI_VOICE_INPUT_OCHANNEL_VELOCITY);
  bse_source_must_set_input (self->sub_synth, 3,
			     self->voice_input, BSE_MIDI_VOICE_INPUT_OCHANNEL_AFTERTOUCH);

  /* midi voice switch */
  self->voice_switch = (BseSource*) bse_container_new_child (container, BSE_TYPE_MIDI_VOICE_SWITCH, NULL);
  bse_item_set_internal (self->voice_switch, TRUE);
  bse_midi_voice_input_set_voice_switch (BSE_MIDI_VOICE_INPUT (self->voice_input), BSE_MIDI_VOICE_SWITCH (self->voice_switch));

  /* sub-synth <-> voice switch */
  bse_source_must_set_input (self->voice_switch, BSE_MIDI_VOICE_SWITCH_ICHANNEL_LEFT,
			     self->sub_synth, 0);
  bse_source_must_set_input (self->voice_switch, BSE_MIDI_VOICE_SWITCH_ICHANNEL_RIGHT,
			     self->sub_synth, 1);
  bse_source_must_set_input (self->voice_switch, BSE_MIDI_VOICE_SWITCH_ICHANNEL_DISCONNECT,
			     self->sub_synth, 3);

  /* midi voice switch <-> context merger */
  bse_source_must_set_input (BSE_SOURCE (self), 0,
			     self->voice_switch, BSE_MIDI_VOICE_SWITCH_OCHANNEL_LEFT);
  bse_source_must_set_input (BSE_SOURCE (self), 1,
			     self->voice_switch, BSE_MIDI_VOICE_SWITCH_OCHANNEL_RIGHT);

  /* postprocess */
  self->postprocess = (BseSource*) bse_container_new_child_bname (container, BSE_TYPE_SUB_SYNTH, "Track-Postprocess", NULL);
  bse_item_set_internal (self->postprocess, TRUE);
  bse_sub_synth_set_null_shortcut (BSE_SUB_SYNTH (self->postprocess), TRUE);

  /* context merger <-> postprocess */
  bse_source_must_set_input (self->postprocess, 0, BSE_SOURCE (self), 0);
  bse_source_must_set_input (self->postprocess, 1, BSE_SOURCE (self), 1);

  /* propagate midi channel to modules */
  bse_track_update_midi_channel (self);
}

BseSource*
bse_track_get_output (BseTrack *self)
{
  return self->postprocess;
}

guint
bse_track_get_last_tick (BseTrack *self)
{
  int last_tick = 0;

  /* find last part */
  BsePart *part = NULL;
  guint i, offset = 0;
  for (i = 0; i < self->n_entries_SL; i++)
    if (self->entries_SL[i].part)
      {
        part = self->entries_SL[i].part;
        offset = self->entries_SL[i].tick;
      }
  if (part)
    {
      BseItem *item = BSE_ITEM (self);
      Bse::SongTiming timing;
      auto partimpl = part->as<Bse::PartImpl*>();
      last_tick = partimpl->last_tick();
      if (BSE_IS_SONG (item->parent))
        bse_song_get_timing (BSE_SONG (item->parent), offset, &timing);
      else
        bse_song_timing_get_default (&timing);
      last_tick = MAX (last_tick, timing.tpt);  /* MAX duration with tact size, see bse_track_list_parts() */
      last_tick += offset;
    }
  else
    last_tick += 1;     /* always return one after */

  return last_tick;
}

static void
bse_track_update_midi_channel (BseTrack *self)
{
  if (self->voice_switch)
    {
      bse_sub_synth_set_midi_channel (BSE_SUB_SYNTH (self->sub_synth), self->midi_channel_SL);
      bse_sub_synth_set_midi_channel (BSE_SUB_SYNTH (self->postprocess), self->midi_channel_SL);
      bse_midi_voice_switch_set_midi_channel (BSE_MIDI_VOICE_SWITCH (self->voice_switch), self->midi_channel_SL);
    }
}

static void
bse_track_context_create (BseSource      *source,
                          guint           context_handle,
                          BseTrans       *trans)
{
  BseTrack *self = BSE_TRACK (source);
  BseMidiContext mcontext = bse_snet_get_midi_context (bse_item_get_snet (BSE_ITEM (self)), context_handle);
  if (self->snet || self->wave)
    bse_midi_receiver_channel_enable_poly (mcontext.midi_receiver, self->midi_channel_SL);
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

static void
bse_track_context_dismiss (BseSource      *source,
                           guint           context_handle,
                           BseTrans       *trans)
{
  BseTrack *self = BSE_TRACK (source);
  BseMidiContext mcontext = bse_snet_get_midi_context (bse_item_get_snet (BSE_ITEM (self)), context_handle);
  if (self->snet || self->wave)
    bse_midi_receiver_channel_disable_poly (mcontext.midi_receiver, self->midi_channel_SL);
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_dismiss (source, context_handle, trans);
}

void
bse_track_remove_modules (BseTrack     *self,
                          BseContainer *container)
{
  assert_return (BSE_IS_TRACK (self));
  assert_return (BSE_IS_CONTAINER (container));
  assert_return (self->sub_synth != NULL);

  bse_container_remove_item (container, BSE_ITEM (self->sub_synth));
  self->sub_synth = NULL;
  bse_container_remove_item (container, BSE_ITEM (self->voice_input));
  self->voice_input = NULL;
  bse_container_remove_item (container, BSE_ITEM (self->voice_switch));
  self->voice_switch = NULL;
  bse_container_remove_item (container, BSE_ITEM (self->postprocess));
  self->postprocess = NULL;
}

void
bse_track_clone_voices (BseTrack       *self,
                        BseSNet        *snet,
                        guint           context,
                        BseMidiContext  mcontext,
                        BseTrans       *trans)
{
  guint i;

  assert_return (BSE_IS_TRACK (self));
  assert_return (BSE_IS_SNET (snet));
  assert_return (trans != NULL);

  if (!self->sound_font_preset)
    {
      for (i = 0; i < self->max_voices - 1; i++)
	bse_snet_context_clone_branch (snet, context, BSE_SOURCE (self), mcontext, trans);
    }
}

static void
bse_track_store_private (BseObject  *object,
			 BseStorage *storage)
{
  BseTrack *self = BSE_TRACK (object);
  BseItem *item = BSE_ITEM (self);
  guint i;

  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->store_private)
    BSE_OBJECT_CLASS (parent_class)->store_private (object, storage);

  for (i = 0; i < self->n_entries_SL; i++)
    {
      BseTrackEntry *e = self->entries_SL + i;
      if (e->part)
	{
	  bse_storage_break (storage);
	  bse_storage_printf (storage, "(insert-part %u ",
			      e->tick);
	  bse_storage_put_item_link (storage, item, BSE_ITEM (e->part));
	  bse_storage_putc (storage, ')');
	}
    }
}

static void
part_link_resolved (gpointer        data,
		    BseStorage     *storage,
		    BseItem        *from_item,
		    BseItem        *to_item,
		    const gchar    *error)
{
  BseTrack *self = BSE_TRACK (from_item);

  if (error)
    bse_storage_warn (storage, "%s", error);
  else if (!BSE_IS_PART (to_item))
    bse_storage_warn (storage, "skipping invalid part reference: %s", bse_object_debug_name (to_item));
  else if (to_item->parent != from_item->parent)
    bse_storage_warn (storage, "skipping out-of-branch part reference: %s", bse_object_debug_name (to_item));
  else
    {
      guint tick = GPOINTER_TO_UINT (data);
      if (bse_track_insert_part (self, tick, BSE_PART (to_item)) < 1)
	bse_storage_warn (storage, "failed to insert part reference to %s",
			  bse_object_debug_name (to_item));
    }
}

static GTokenType
bse_track_restore_private (BseObject  *object,
			   BseStorage *storage,
                           GScanner   *scanner)
{
  BseTrack *self = BSE_TRACK (object);

  if (g_scanner_peek_next_token (scanner) == G_TOKEN_IDENTIFIER &&
      bse_string_equals ("insert-part", scanner->next_value.v_identifier))
    {
      GTokenType token;
      guint tick;

      g_scanner_get_next_token (scanner);       /* eat quark */

      parse_or_return (scanner, G_TOKEN_INT);
      tick = scanner->value.v_int64;
      token = bse_storage_parse_item_link (storage, BSE_ITEM (self), part_link_resolved, GUINT_TO_POINTER (tick));
      if (token != G_TOKEN_NONE)
	return token;
      parse_or_return (scanner, ')');
      return G_TOKEN_NONE;
    }
  else /* chain parent class' handler */
    return BSE_OBJECT_CLASS (parent_class)->restore_private (object, storage, scanner);
}

static void
bse_track_prepare (BseSource *source)
{
  Bse::TrackImpl &self = *source->as<Bse::TrackImplP>();
  auto &devcon = *dynamic_cast<Bse::DeviceContainerImpl*> (&*self.device_container());
  self.render_setup (true);
  BSE_SOURCE_CLASS (parent_class)->prepare (source); // chain up
  devcon.processor()->reset_state (self.render_setup());
  BSE_SERVER.add_pcm_output_processor (devcon.processor());
}

static void
bse_track_reset (BseSource *source)
{
  Bse::TrackImpl &self = *source->as<Bse::TrackImplP>();
  auto &devcon = *dynamic_cast<Bse::DeviceContainerImpl*> (&*self.device_container());
  BSE_SERVER.del_pcm_output_processor (devcon.processor());
  BSE_SOURCE_CLASS (parent_class)->reset (source); // chain up
}

static void
bse_track_class_init (BseTrackClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (klass);
  BseItemClass *item_class = BSE_ITEM_CLASS (klass);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (klass);

  parent_class = (GTypeClass*) g_type_class_peek_parent (klass);

  gobject_class->set_property = bse_track_set_property;
  gobject_class->get_property = bse_track_get_property;
  gobject_class->dispose = bse_track_dispose;
  gobject_class->finalize = bse_track_finalize;

  object_class->store_private = bse_track_store_private;
  object_class->restore_private = bse_track_restore_private;

  item_class->get_candidates = bse_track_get_candidates;
  item_class->needs_storage = bse_track_needs_storage;

  source_class->prepare = bse_track_prepare;
  source_class->context_create = bse_track_context_create;
  source_class->context_dismiss = bse_track_context_dismiss;
  source_class->reset = bse_track_reset;

  bse_source_class_inherit_channels (BSE_SOURCE_CLASS (klass));

  bse_object_class_add_param (object_class, _("Synth Input"),
			      PROP_SNET,
			      bse_param_spec_object ("snet", _("Synthesizer"), _("Synthesis network to be used as instrument"),
						     BSE_TYPE_CSYNTH,
						     SFI_PARAM_STANDARD ":unprepared"));
  bse_object_class_add_param (object_class, _("Synth Input"),
			      PROP_WAVE,
			      bse_param_spec_object ("wave", _("Wave"), _("Wave to be used as instrument"),
						     BSE_TYPE_WAVE,
						     SFI_PARAM_STANDARD ":unprepared"));
  bse_object_class_add_param (object_class, _("Synth Input"),
			      PROP_SOUND_FONT_PRESET,
                              bse_param_spec_object (("sound_font_preset"), _("Sound Font Preset"),
                                                     _("Sound font preset to be used as instrument"),
						     BSE_TYPE_SOUND_FONT_PRESET,
						     SFI_PARAM_STANDARD ":unprepared"));
  bse_object_class_add_param (object_class, _("MIDI Instrument"),
			      PROP_PNET,
			      bse_param_spec_object ("pnet", _("Postprocessor"), _("Synthesis network to be used as postprocessor"),
						     BSE_TYPE_CSYNTH, SFI_PARAM_STANDARD ":unprepared"));
#if 0
  bse_object_class_add_param (object_class, _("Signal Outputs"),
                              PROP_OUTPUTS,
                              bse_param_spec_boxed ("outputs", _("Output Signals"),
                                                    _("Mixer busses used as output for this track"),
                                                    BSE_TYPE_IT3M_SEQ, SFI_PARAM_GUI ":item-sequence"));
#endif
}

namespace Bse {

TrackImpl::TrackImpl (BseObject *bobj) :
  ContextMergerImpl (bobj)
{}

TrackImpl::~TrackImpl ()
{}

void
TrackImpl::xml_serialize (SerializationNode &xs)
{
  ContextMergerImpl::xml_serialize (xs);
  for (auto &xc : xs.children ("Device"))                        // in_load
    {
      const String uuiduri = xc.get ("type");
      if (!uuiduri.empty())
        {
          DeviceIfaceP devicei = device_container()->create_device (uuiduri);
          if (devicei)
            {
              auto *dimpl = dynamic_cast<DeviceImpl*> (devicei.get());
              if (dimpl)
                {
                  xc.load (*dimpl);
                  continue;
                }
            }
        }
      printerr ("Bse::TrackImpl::%s: failed to create device: %s\n", __func__, uuiduri);
    }
  for (DeviceIfaceP device : device_container()->list_devices()) // in_save
    xs.save_under ("Device", *dynamic_cast<DeviceImpl*> (device.get()));
}

void
TrackImpl::xml_reflink (SerializationNode &xs)
{
  ContextMergerImpl::xml_reflink (xs);
}

bool
TrackImpl::needs_serialize()
{
  return device_container()->list_devices().size() > 0;
}

bool
TrackImpl::muted() const
{
  BseTrack *self = const_cast<TrackImpl*> (this)->as<BseTrack*>();

  return self->muted_SL;
}

void
TrackImpl::muted (bool muted)
{
  BseTrack *self = as<BseTrack*>();

  bool value = self->muted_SL;
  if (APPLY_IDL_PROPERTY (value, muted))
    {
      BSE_SEQUENCER_LOCK ();
      self->muted_SL = value;
      BSE_SEQUENCER_UNLOCK ();
    }
}

int
TrackImpl::midi_channel() const
{
  BseTrack *self = const_cast<TrackImpl*> (this)->as<BseTrack*>();

  return self->midi_channel_SL <= BSE_MIDI_MAX_CHANNELS ? self->midi_channel_SL : 0;
}

void
TrackImpl::midi_channel (int channel)
{
  BseTrack *self = as<BseTrack*>();

  int value = midi_channel();
  if (APPLY_IDL_PROPERTY (value, channel) && !BSE_SOURCE_PREPARED (self))
    {
      self->midi_channel_SL = value > 0 ? value : self->channel_id;
      bse_track_update_midi_channel (self);
    }
}

int
TrackImpl::n_voices() const
{
  BseTrack *self = const_cast<TrackImpl*> (this)->as<BseTrack*>();

  return self->max_voices;
}

void
TrackImpl::n_voices (int voices)
{
  BseTrack *self = as<BseTrack*>();

  const bool post_prepared = self->postprocess && BSE_SOURCE_PREPARED (self->postprocess);
  int value = n_voices();
  if (APPLY_IDL_PROPERTY (value, voices) && !post_prepared)
      self->max_voices = value;
}

SongImplP
TrackImpl::get_song ()
{
  ItemIfaceP parent = get_parent();
  return parent ? parent->as<SongImplP>() : nullptr;
}

SongTiming
TrackImpl::get_timing (int tick)
{
  SongTiming timing;
  SongImplP song = get_song();
  if (song)
    bse_song_get_timing (song->as<BseSong*>(), tick, &timing);
  else
    bse_song_timing_get_default (&timing);
  return timing;
}

PartIfaceP
TrackImpl::create_part (int32 tick)
{
  PartIfaceP part;
  SongImplP song = get_song();
  return_unless (song, part);
  group_undo (__func__);
  part = song->create_part();
  if (part)
    insert_part (0, *part);
  ungroup_undo();
  return part;
}

int
TrackImpl::insert_part (int tick, PartIface &parti)
{
  BseTrack *self = as<BseTrack*>();
  PartImpl &part = dynamic_cast<PartImpl&> (parti);
  assert_return (parent() != NULL && parent() == part.parent(), 0); // parent is SongImpl
  uint id = bse_track_insert_part (self, tick, part.as<BsePart*>());
  if (id)
    {
      // can't use remove_link() here, since id will have changed after undo
      push_undo (__func__, *this, &TrackImpl::remove_tick, tick);
    }
  return id;
}

void
TrackImpl::remove_tick (int tick)
{
  BseTrack *self = as<BseTrack*>();
  BseTrackEntry *entry = bse_track_lookup_tick (self, tick);
  if (entry)
    {
      // undoing part removal needs an undo_descriptor b/c future deletions may invalidate the part handle
      const uint utick = entry->tick;
      UndoDescriptor<PartImpl> part_descriptor = undo_descriptor (*entry->part->as<PartImpl*>());
      auto lambda = [utick, part_descriptor] (TrackImpl &self, BseUndoStack *ustack) -> Error {
        PartImpl &part = self.undo_resolve (part_descriptor);
        const uint id = self.insert_part (utick, part);
        return id ? Error::NONE : Error::INVALID_OVERLAP;
      };
      bse_track_remove_tick (self, tick);
      push_undo (__func__, *this, lambda);
    }
}

void
TrackImpl::remove_link (int link_id)
{
  BseTrack *self = as<BseTrack*>();
  BseTrackEntry *entry = bse_track_find_link (self, link_id);
  if (entry)
    remove_tick (entry->tick);
}

PartSeq
TrackImpl::list_parts_uniq ()
{
  BseTrack *self = as<BseTrack*>();
  const TrackPartSeq &tpseq = bse_track_list_parts (self);
  PartSeq parts;
  for (const auto &tp : tpseq)
    parts.push_back (tp.part);
  std::sort (parts.begin(), parts.end());
  parts.erase (std::unique (parts.begin(), parts.end()), parts.end());
  return parts;
}

TrackPartSeq
TrackImpl::list_parts ()
{
  BseTrack *self = as<BseTrack*>();
  return bse_track_list_parts (self);
}

PartIfaceP
TrackImpl::get_part (int tick)
{
  BseTrack *self = as<BseTrack*>();
  BseTrackEntry *entry = bse_track_lookup_tick (self, tick);
  return entry ? entry->part->as<PartIfaceP>() : NULL;
}

int
TrackImpl::get_last_tick ()
{
  BseTrack *self = as<BseTrack*>();
  return bse_track_get_last_tick (self);
}

Error
TrackImpl::ensure_output ()
{
  BseTrack *self = as<BseTrack*>();
  Error error = Error::NONE;
  BseItem *bparent = self->parent;
  if (BSE_IS_SONG (bparent) && !self->bus_outputs)
    {
      BseSong *song = BSE_SONG (bparent);
      BseBus *bmaster = bse_song_find_master (song);
      if (bmaster)
        {
          BusImpl &master = *bmaster->as<BusImpl*>();
          error = master.connect_track (*this);
        }
    }
  return error;
}

SourceIfaceP
TrackImpl::get_output_source ()
{
  BseTrack *self = as<BseTrack*>();
  BseSource *child = bse_track_get_output (self);
  return child ? child->as<SourceIfaceP>() : NULL;
}

ItemSeq
TrackImpl::outputs () const
{
  BseTrack *self = const_cast<TrackImpl*> (this)->as<BseTrack*>();
  ItemSeq items;
  for (SfiRing *ring = self->bus_outputs; ring; ring = sfi_ring_walk (ring, self->bus_outputs))
    {
      BseItem *item = (BseItem*) ring->data;
      items.push_back (item->as<ItemIfaceP>());
    }
  return items;
}

void
TrackImpl::outputs (const ItemSeq &newoutputs)
{
  BseTrack *self = as<BseTrack*>();
  bse_bus_or_track_set_outputs (self, newoutputs);
}

DeviceContainerIfaceP
TrackImpl::device_container()
{
  if (!device_container_)
    {
      DeviceImplP devicep = DeviceImpl::create_single_device ("Bse.AudioSignal.Chain");
      assert_return (devicep != nullptr, nullptr);
      DeviceContainerImplP device_container = std::dynamic_pointer_cast<DeviceContainerImpl> (devicep);
      assert_return (device_container != nullptr, nullptr);
      device_container_ = device_container;
      BSE_SERVER.assign_event_source (*device_container_->processor());
    }
  return device_container_;
}

AudioSignal::RenderSetup&
TrackImpl::render_setup (bool needsreset)
{
  if (needsreset)
    {
      delete render_setup_;
      static AudioSignal::AudioTiming audio_timing { 120, 0 }; // FIXME
      render_setup_ = new AudioSignal::RenderSetup (bse_engine_sample_freq(), audio_timing);
    }
  return *render_setup_;
}

} // Bse
