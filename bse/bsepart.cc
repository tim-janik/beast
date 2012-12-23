// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsepart.hh"
#include "bsemain.hh"
#include "bsestorage.hh"
#include "bsesong.hh"
#include "bsetrack.hh"
#include "bsecxxplugin.hh"
#include "gslcommon.hh"
#include "bsemathsignal.hh" // bse_semitone_table
#include "bseieee754.hh"
#include <stdlib.h>
#include <string.h>
/* --- macros --- */
#define	upper_power2(uint_n)	sfi_alloc_upper_power2 (MAX ((uint_n), 4))
#define parse_or_return         bse_storage_scanner_parse_or_return
#define peek_or_return          bse_storage_scanner_peek_or_return
/* --- properties --- */
enum
{
  PROP_0,
  PROP_N_CHANNELS,
  PROP_LAST_TICK,
};
/* --- prototypes --- */
static void	    bse_part_class_init		(BsePartClass	*klass);
static void	    bse_part_init		(BsePart	*self);
static void	    bse_part_set_property	(GObject        *object,
						 guint           param_id,
						 const GValue   *value,
						 GParamSpec     *pspec);
static void	    bse_part_get_property	(GObject	*object,
						 guint           param_id,
						 GValue         *value,
						 GParamSpec     *pspec);
static void	    bse_part_dispose		(GObject	*object);
static void	    bse_part_finalize		(GObject	*object);
static void	    bse_part_store_private	(BseObject	*object,
						 BseStorage	*storage);
static GTokenType   bse_part_restore_private	(BseObject	*object,
						 BseStorage	*storage,
                                                 GScanner       *scanner);
/* --- variables --- */
static gpointer parent_class = NULL;
static guint    signal_range_changed = 0;
static guint	handler_id_range_changed = 0;
static SfiRing *plist_range_changed = NULL;
static guint    signal_links_changed = 0;
static guint	handler_id_links_changed = 0;
static SfiRing *plist_links_changed = NULL;
static GQuark   quark_insert_note = 0;
static GQuark   quark_insert_notes = 0;
static GQuark   quark_insert_control = 0;
static GQuark   quark_insert_controls = 0;
/* --- functions --- */
BSE_BUILTIN_TYPE (BsePart)
{
  static const GTypeInfo info = {
    sizeof (BsePartClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_part_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    sizeof (BsePart),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_part_init,
  };
  return bse_type_register_static (BSE_TYPE_ITEM,
				   "BsePart",
				   "BSE part type",
                                   __FILE__, __LINE__,
                                   &info);
}
static void
bse_part_class_init (BsePartClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (klass);
  // BseItemClass *item_class = BSE_ITEM_CLASS (klass);
  parent_class = g_type_class_peek_parent (klass);
  gobject_class->set_property = bse_part_set_property;
  gobject_class->get_property = bse_part_get_property;
  gobject_class->dispose = bse_part_dispose;
  gobject_class->finalize = bse_part_finalize;
  object_class->store_private = bse_part_store_private;
  object_class->restore_private = bse_part_restore_private;
  quark_insert_note = g_quark_from_static_string ("insert-note");
  quark_insert_notes = g_quark_from_static_string ("insert-notes");
  quark_insert_control = g_quark_from_static_string ("insert-control");
  quark_insert_controls = g_quark_from_static_string ("insert-controls");
  bse_object_class_add_param (object_class, "Limits",
			      PROP_N_CHANNELS,
			      sfi_pspec_int ("n_channels", "Channels", NULL,
					     1, 1, BSE_PART_MAX_CHANNELS, 4,
					     SFI_PARAM_STANDARD));
  bse_object_class_add_param (object_class, "Limits",
			      PROP_LAST_TICK,
			      sfi_pspec_int ("last_tick", "Last Tick", NULL,
					     0, 0, BSE_PART_MAX_TICK, 384,
					     SFI_PARAM_GUI_READABLE));
  signal_range_changed = bse_object_class_add_signal (object_class, "range-changed",
						      G_TYPE_NONE, 4,
						      G_TYPE_INT, G_TYPE_INT,
						      G_TYPE_INT, G_TYPE_INT);
  signal_links_changed = bse_object_class_add_signal (object_class, "links-changed", G_TYPE_NONE, 0);
}
static void
bse_part_init (BsePart *self)
{
  self->semitone_table = bse_semitone_table_from_tuning (BSE_MUSICAL_TUNING_12_TET);
  self->n_ids = 0;
  self->ids = NULL;
  self->last_id = 0;
  self->last_tick_SL = 0;
  self->links_queued = FALSE;
  self->range_queued = FALSE;
  self->range_tick = BSE_PART_MAX_TICK;
  self->range_bound = 0;
  self->range_min_note = BSE_MAX_NOTE;
  self->range_max_note = 0;
  bse_part_controls_init (&self->controls);
  self->n_channels = 1;
  self->channels = g_renew (BsePartNoteChannel, self->channels, self->n_channels);
  bse_part_note_channel_init (&self->channels[0]);
}
static void
part_add_channel (BsePart *self,
                  gboolean notify)
{
  guint i = self->n_channels++;
  self->channels = g_renew (BsePartNoteChannel, self->channels, self->n_channels);
  bse_part_note_channel_init (&self->channels[i]);
  g_object_notify ((GObject*) self, "n_channels");
}
static void
bse_part_set_property (GObject        *object,
		       guint           param_id,
		       const GValue   *value,
		       GParamSpec     *pspec)
{
  BsePart *self = BSE_PART (object);
  switch (param_id)
    {
      guint n;
    case PROP_N_CHANNELS:
      n = g_value_get_int (value);
      while (self->n_channels < n)
        part_add_channel (self, FALSE);
      while (self->n_channels > n)
        bse_part_note_channel_destroy (&self->channels[--self->n_channels]);
      break;
    case PROP_LAST_TICK:
      g_assert_not_reached ();
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}
static void
bse_part_get_property (GObject	  *object,
		       guint       param_id,
		       GValue     *value,
		       GParamSpec *pspec)
{
  BsePart *self = BSE_PART (object);
  switch (param_id)
    {
    case PROP_N_CHANNELS:
      g_value_set_int (value, self->n_channels);
      break;
    case PROP_LAST_TICK:
      g_value_set_int (value, self->last_tick_SL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}
static void
bse_part_dispose (GObject *object)
{
  BsePart *self = BSE_PART (object);
  plist_links_changed = sfi_ring_remove (plist_links_changed, self);
  self->links_queued = FALSE;
  plist_range_changed = sfi_ring_remove (plist_range_changed, self);
  self->range_queued = FALSE;
  self->range_tick = BSE_PART_MAX_TICK;
  self->range_bound = 0;
  self->range_min_note = BSE_MAX_NOTE;
  self->range_max_note = 0;
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (object);
}
static void
bse_part_finalize (GObject *object)
{
  BsePart *self = BSE_PART (object);
  guint i;
  self->links_queued = TRUE;
  plist_links_changed = sfi_ring_remove (plist_links_changed, self);
  self->range_queued = TRUE;
  plist_range_changed = sfi_ring_remove (plist_range_changed, self);
  self->n_ids = 0;
  g_free (self->ids);
  self->ids = NULL;
  self->last_id = 0;
  bse_part_controls_destroy (&self->controls);
  for (i = 0; i < self->n_channels; i++)
    bse_part_note_channel_destroy (&self->channels[i]);
  g_free (self->channels);
  self->channels = NULL;
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}
void
bse_part_set_semitone_table (BsePart      *self,
                             const double *semitone_table)
{
  g_return_if_fail (BSE_IS_PART (self));
  g_return_if_fail (semitone_table != NULL);
  self->semitone_table = semitone_table;
}
static guint
bse_part_alloc_id (BsePart *self,
		   guint    tick)
{
  guint id;
  g_return_val_if_fail (tick <= BSE_PART_MAX_TICK, 0);
  /* we keep an array of ids to implement a fast lookup
   * from id to tick of the event containing id. ticks
   * >= BSE_PART_INVAL_TICK_FLAG indicate non-allocated
   * ids. last_id is the head of a list of freed ids,
   * so we can hand out new ids in reversed order they
   * were freed in, to provide deterministic id assignment
   * for state rollbacks as required by undo.
   */
  if (self->last_id)
    {
      guint i = self->last_id - 1;
      g_assert (self->ids[i] >= BSE_PART_INVAL_TICK_FLAG);
      self->last_id = self->ids[i] - BSE_PART_INVAL_TICK_FLAG;
      id = i + 1;
    }
  else
    {
      guint i = self->n_ids++;
      self->ids = g_renew (guint, self->ids, self->n_ids);
      id = i + 1;
    }
  self->ids[id - 1] = tick;
  return id;
}
static void
bse_part_move_id (BsePart *self,
		  guint	   id,
		  guint    tick)
{
  g_return_if_fail (tick <= BSE_PART_MAX_TICK);
  g_return_if_fail (id > 0 && id <= self->n_ids);
  g_return_if_fail (self->ids[id - 1] < BSE_PART_INVAL_TICK_FLAG);	/* check !freed id */
  self->ids[id - 1] = tick;
}
static void
bse_part_free_id (BsePart *self,
		  guint    id)
{
  guint i;
  g_return_if_fail (id > 0 && id <= self->n_ids);
  g_return_if_fail (self->ids[id - 1] < BSE_PART_INVAL_TICK_FLAG);	/* check !freed id */
  i = id - 1;
  self->ids[i] = self->last_id + BSE_PART_INVAL_TICK_FLAG;
  self->last_id = id;
}
static guint	/* returns tick (<= BSE_PART_MAX_TICK) if id is valid */
bse_part_tick_from_id (BsePart *self,
		       guint    id)
{
  return id > 0 && id <= self->n_ids ? self->ids[id - 1] : BSE_PART_INVAL_TICK_FLAG;
}
static void
part_update_last_tick (BsePart *self)
{
  guint channel, last_tick;
  last_tick = bse_part_controls_get_last_tick (&self->controls);
  for (channel = 0; channel < self->n_channels; channel++)
    {
      guint t = bse_part_note_channel_get_last_tick (&self->channels[channel]);
      last_tick = MAX (last_tick, t);
    }
  BSE_SEQUENCER_LOCK ();
  self->last_tick_SL = last_tick;
  BSE_SEQUENCER_UNLOCK ();
  g_object_notify ((GObject*) self, "last-tick");
  bse_part_links_changed (self);
}
static gboolean
range_changed_notify_handler (gpointer data)
{
  while (plist_range_changed)
    {
      BsePart *self = (BsePart*) sfi_ring_pop_head (&plist_range_changed);
      self->range_queued = FALSE;
      guint tick = self->range_tick, duration = self->range_bound - tick;
      gint min_note = self->range_min_note, max_note = self->range_max_note;
      self->range_tick = BSE_PART_MAX_TICK;
      self->range_bound = 0;
      self->range_min_note = BSE_MAX_NOTE;
      self->range_max_note = 0;
      if (min_note <= max_note)
	g_signal_emit (self, signal_range_changed, 0, tick, duration, min_note, max_note);
    }
  handler_id_range_changed = 0;
  return FALSE;
}
static void
queue_update (BsePart *self,
	      guint    tick,
	      guint    duration,
	      gint     note)
{
  guint bound = tick + duration;
  g_return_if_fail (duration > 0);
  if (!BSE_OBJECT_DISPOSING (self))
    {
      if (self->range_tick >= self->range_bound && !self->range_queued)
        {
          self->range_queued = TRUE;
          plist_range_changed = sfi_ring_append (plist_range_changed, self);
          if (!handler_id_range_changed)
            handler_id_range_changed = bse_idle_update (range_changed_notify_handler, NULL);
        }
      self->range_tick = MIN (self->range_tick, tick);
      self->range_bound = MAX (self->range_bound, bound);
      self->range_min_note = MIN (self->range_min_note, note);
      self->range_max_note = MAX (self->range_max_note, note);
    }
}
static void
queue_note_update (BsePart          *self,
                   BsePartEventNote *note)
{
  queue_update (self, note->tick, note->duration, note->note);
}
static void
queue_control_update (BsePart *self,
                      guint    tick)
{
  guint bound = tick + 1;
  if (!BSE_OBJECT_DISPOSING (self))
    {
      if (self->range_tick >= self->range_bound && !self->range_queued)
        {
          self->range_queued = TRUE;
          plist_range_changed = sfi_ring_append (plist_range_changed, self);
          if (!handler_id_range_changed)
            handler_id_range_changed = bse_idle_update (range_changed_notify_handler, NULL);
        }
      self->range_tick = MIN (self->range_tick, tick);
      self->range_bound = MAX (self->range_bound, bound);
      self->range_min_note = BSE_MIN_NOTE;
      self->range_max_note = BSE_MAX_NOTE;
    }
}
static gboolean
links_changed_notify_handler (gpointer data)
{
  while (plist_links_changed)
    {
      BsePart *self = (BsePart*) sfi_ring_pop_head (&plist_links_changed);
      self->links_queued = FALSE;
      g_signal_emit (self, signal_links_changed, 0);
    }
  handler_id_links_changed = 0;
  return FALSE;
}
void
bse_part_links_changed (BsePart *self)
{
  g_return_if_fail (BSE_IS_PART (self));
  if (!BSE_OBJECT_DISPOSING (self) && !self->links_queued)
    {
      self->links_queued = TRUE;
      plist_links_changed = sfi_ring_append (plist_links_changed, self);
      if (!handler_id_links_changed)
        handler_id_links_changed = bse_idle_update (links_changed_notify_handler, NULL);
    }
}
static int
part_link_compare (const void *p1,
                   const void *p2)
{
  const BsePartLink *const*lp1 = (const BsePartLink *const*) p1;
  const BsePartLink *const*lp2 = (const BsePartLink *const*) p2;
  const BsePartLink *l1 = lp1[0];
  const BsePartLink *l2 = lp2[0];
  if (l1->tick == l2->tick)
    {
      if (l1->duration == l2->duration)
        return l1->track < l2->track ? -1 : l1->track > l2->track;
      else
        return l1->duration < l2->duration ? -1 : 1;
    }
  else
    return l1->tick < l2->tick ? -1 : 1;
}
BsePartLinkSeq*
bse_part_list_links (BsePart *self)
{
  g_return_val_if_fail (BSE_IS_PART (self), NULL);
  BsePartLinkSeq *pls = bse_part_link_seq_new ();
  BseSong *song = (BseSong*) bse_item_get_super (BSE_ITEM (self));
  if (BSE_IS_SONG (song))
    {
      SfiRing *ring;
      for (ring = song->tracks_SL; ring; ring = sfi_ring_walk (ring, song->tracks_SL))
        {
          BseTrack *track = (BseTrack*) ring->data;
          BseTrackPartSeq *tps = bse_track_list_part (track, self);
          for (uint i = 0; i < tps->n_tparts; i++)
            {
              BseTrackPart *tp = tps->tparts[i];
              BsePartLink pl;
              pl.track = track;
              pl.tick = tp->tick;
              pl.part = self;
              pl.duration = tp->duration;
              bse_part_link_seq_append (pls, &pl);
            }
          bse_track_part_seq_free (tps);
        }
      qsort (pls->plinks, pls->n_plinks, sizeof (pls->plinks[0]), part_link_compare);
    }
  return pls;
}
void
bse_part_select_notes (BsePart *self,
                       guint    match_channel,
                       guint    tick,
                       guint    duration,
                       gint     min_note,
                       gint     max_note,
                       gboolean selected)
{
  guint channel;
  g_return_if_fail (BSE_IS_PART (self));
  selected = selected != FALSE;
  min_note = BSE_NOTE_CLAMP (min_note);
  max_note = BSE_NOTE_CLAMP (max_note);
  for (channel = 0; channel < self->n_channels; channel++)
    {
      BsePartEventNote *note, *last;
      if (channel != match_channel && match_channel != ~uint (0))
        continue;
      note = bse_part_note_channel_lookup_ge (&self->channels[channel], tick);
      last = bse_part_note_channel_lookup_lt (&self->channels[channel], tick + duration);
      if (!note)
        continue;
      while (note <= last)
        {
          if (note->selected != selected && note->note >= min_note && note->note <= max_note)
            {
              bse_part_note_channel_change_note (&self->channels[channel], note, note->id, selected,
                                                 note->note, note->fine_tune, note->velocity);
              queue_note_update (self, note);
            }
          note++;
        }
    }
}
void
bse_part_select_controls (BsePart          *self,
                          guint             tick,
                          guint             duration,
                          BseMidiSignalType ctype,
                          gboolean          selected)
{
  g_return_if_fail (BSE_IS_PART (self));
  selected = selected != FALSE;
  if (BSE_PART_NOTE_CONTROL (ctype))
    {
      bse_part_select_notes (self, ~0, tick, duration, BSE_MIN_NOTE, BSE_MAX_NOTE, selected);
      return;
    }
  BsePartTickNode *node = bse_part_controls_lookup_ge (&self->controls, tick);
  if (!node)
    return;
  BsePartTickNode *last = bse_part_controls_lookup_lt (&self->controls, tick + duration);
  while (node <= last)
    {
      BsePartEventControl *cev;
      for (cev = node->events; cev; cev = cev->next)
        if (cev->ctype == ctype && cev->selected != selected)
          {
            bse_part_controls_change_selected (cev, selected);
            queue_control_update (self, node->tick);
          }
      node++;
    }
}
void
bse_part_select_notes_exclusive (BsePart *self,
                                 guint    match_channel,
                                 guint    tick,
                                 guint    duration,
                                 gint     min_note,
                                 gint     max_note)
{
  BsePartTickNode *node, *cbound;
  guint channel;
  g_return_if_fail (BSE_IS_PART (self));
  min_note = BSE_NOTE_CLAMP (min_note);
  max_note = BSE_NOTE_CLAMP (max_note);
  for (channel = 0; channel < self->n_channels; channel++)
    {
      BsePartEventNote *note = bse_part_note_channel_lookup_ge (&self->channels[channel], 0);
      BsePartEventNote *nbound = bse_part_note_channel_get_bound (&self->channels[channel]);
      while (note < nbound)
        {
          gboolean selected = (note->tick >= tick && note->tick < tick + duration &&
                               note->note >= min_note && note->note <= max_note &&
                               (channel == match_channel || match_channel == ~uint (0)));
          if (note->selected != selected)
            {
              bse_part_note_channel_change_note (&self->channels[channel], note, note->id, selected,
                                                 note->note, note->fine_tune, note->velocity);
              queue_note_update (self, note);
            }
          note++;
        }
    }
  /* deselect all control events */
  node = bse_part_controls_lookup_ge (&self->controls, tick);
  if (!node)
    return;
  cbound = bse_part_controls_lookup_lt (&self->controls, tick + duration);
  while (node <= cbound)
    {
      BsePartEventControl *cev;
      for (cev = node->events; cev; cev = cev->next)
        if (cev->selected)
          {
            bse_part_controls_change_selected (cev, FALSE);
            queue_control_update (self, node->tick);
          }
      node++;
    }
}
void
bse_part_select_controls_exclusive (BsePart           *self,
                                    guint              tick,
                                    guint              duration,
                                    BseMidiSignalType  ctype)
{
  BsePartTickNode *node, *bound;
  g_return_if_fail (BSE_IS_PART (self));
  if (BSE_PART_NOTE_CONTROL (ctype))
    {
      bse_part_select_notes_exclusive (self, ~0, tick, duration, BSE_MIN_NOTE, BSE_MAX_NOTE);
      return;
    }
  bse_part_select_notes (self, ~0, 0, BSE_PART_MAX_TICK, BSE_MIN_NOTE, BSE_MAX_NOTE, FALSE);
  node = bse_part_controls_lookup_ge (&self->controls, 0);
  if (!node)
    return;
  bound = bse_part_controls_get_bound (&self->controls);
  while (node < bound)
    {
      BsePartEventControl *cev;
      gboolean selected = node->tick >= tick && node->tick < tick + duration;
      for (cev = node->events; cev; cev = cev->next)
        if (cev->ctype != ctype && cev->selected)
          {
            bse_part_controls_change_selected (cev, FALSE);
            queue_control_update (self, node->tick);
          }
        else if (cev->ctype == ctype && cev->selected != selected)
          {
            bse_part_controls_change_selected (cev, selected);
            queue_control_update (self, node->tick);
          }
      node++;
    }
}
gboolean
bse_part_set_note_selected (BsePart           *self,
                            guint              id,
                            guint              channel,
                            gboolean           selected)
{
  BsePartEventNote *note;
  guint tick;
  g_return_val_if_fail (BSE_IS_PART (self), FALSE);
  g_return_val_if_fail (channel < self->n_channels, FALSE);
  tick = bse_part_tick_from_id (self, id);
  if (tick > BSE_PART_MAX_TICK)
    return FALSE;       /* invalid id */
  note = bse_part_note_channel_lookup (&self->channels[channel], tick);
  if (!note || note->id != id)
    return FALSE;       /* invalid id or channel */
  bse_part_note_channel_change_note (&self->channels[channel], note, note->id, selected,
                                     note->note, note->fine_tune, note->velocity);
  queue_note_update (self, note);
  return TRUE;
}
gboolean
bse_part_set_control_selected (BsePart           *self,
                               guint              id,
                               gboolean           selected)
{
  BsePartEventControl *cev;
  guint tick;
  g_return_val_if_fail (BSE_IS_PART (self), FALSE);
  selected = selected != FALSE;
  tick = bse_part_tick_from_id (self, id);
  if (tick > BSE_PART_MAX_TICK)
    return FALSE;       /* invalid id */
  cev = bse_part_controls_lookup_event (&self->controls, tick, id);
  if (cev)
    {
      if (cev->selected != selected)
        {
          bse_part_controls_change_selected (cev, selected);
          queue_control_update (self, tick);
        }
      return TRUE;
    }
  else
    return FALSE;
}
gboolean
bse_part_delete_note (BsePart           *self,
                      guint              id,
                      guint              channel)
{
  BsePartEventNote *note;
  guint tick;
  g_return_val_if_fail (BSE_IS_PART (self), FALSE);
  g_return_val_if_fail (channel < self->n_channels, FALSE);
  tick = bse_part_tick_from_id (self, id);
  if (tick > BSE_PART_MAX_TICK)
    return FALSE;       /* invalid id */
  note = bse_part_note_channel_lookup (&self->channels[channel], tick);
  if (!note || note->id != id)
    return FALSE;       /* invalid id or channel */
  /* remove note */
  queue_note_update (self, note);
  tick = note->tick + note->duration;
  bse_part_note_channel_remove (&self->channels[channel], note->tick);
  bse_part_free_id (self, id);
  if (tick >= self->last_tick_SL)
    part_update_last_tick (self);
  return TRUE;
}
gboolean
bse_part_delete_control (BsePart *self,
                         guint    id)
{
  BsePartEventControl *cev;
  guint tick;
  g_return_val_if_fail (BSE_IS_PART (self), FALSE);
  tick = bse_part_tick_from_id (self, id);
  if (tick > BSE_PART_MAX_TICK)
    return FALSE;       /* invalid id */
  cev = bse_part_controls_lookup_event (&self->controls, tick, id);
  if (cev)
    {
      queue_control_update (self, tick);
      bse_part_controls_remove (&self->controls, tick, cev);
      bse_part_free_id (self, id);
      if (tick >= self->last_tick_SL)
        part_update_last_tick (self);
      return TRUE;
    }
  else
    return FALSE;
}
guint
bse_part_insert_note (BsePart *self,
		      guint    channel,
		      guint    tick,
		      guint    duration,
		      gint     note,
		      gint     fine_tune,
		      gfloat   velocity)
{
  BsePartEventNote key = { 0 };
  const bool use_any_channel = channel == ~uint (0);
  g_return_val_if_fail (BSE_IS_PART (self), BSE_ERROR_INTERNAL);
  if (use_any_channel)
    channel = 0;
  else if (channel >= self->n_channels)
    g_object_set (self, "n_channels", channel + 1, NULL);
  if (!(BSE_NOTE_IS_VALID (note) &&
	BSE_FINE_TUNE_IS_VALID (fine_tune) &&
	tick < BSE_PART_MAX_TICK &&
	duration > 0 &&
	duration < BSE_PART_MAX_TICK &&
	tick + duration <= BSE_PART_MAX_TICK))
    return 0;
  key.tick = tick;
  key.id = bse_part_alloc_id (self, tick);
  key.duration = duration;
  key.note = note;
  key.fine_tune = fine_tune;
  key.velocity = velocity;
  /* check free channel */
  if (bse_part_note_channel_lookup (&self->channels[channel], key.tick))
    {
      if (!use_any_channel)
        return 0;       /* slot taken */
      /* find (create) a free channel */
      for (channel += 1; channel < self->n_channels; channel++)
        if (!bse_part_note_channel_lookup (&self->channels[channel], key.tick))
          break;
      if (channel >= self->n_channels)
        part_add_channel (self, TRUE);
    }
  /* insert note */
  bse_part_note_channel_insert (&self->channels[channel], key);
  queue_note_update (self, &key);
  if (key.tick + key.duration >= self->last_tick_SL)
    part_update_last_tick (self);
  return key.id;
}
static gboolean
check_valid_control_type (BseMidiSignalType ctype)
{
  if (ctype >= BSE_MIDI_SIGNAL_PROGRAM && ctype <= BSE_MIDI_SIGNAL_FINE_TUNE)
    return TRUE;
  if (ctype >= BSE_MIDI_SIGNAL_CONTINUOUS_0 && ctype <= BSE_MIDI_SIGNAL_CONTINUOUS_31)
    return TRUE;
  if (ctype >= BSE_MIDI_SIGNAL_PARAMETER && ctype <= BSE_MIDI_SIGNAL_NON_PARAMETER)
    return TRUE;
  if (ctype >= BSE_MIDI_SIGNAL_CONTROL_0 && ctype <= BSE_MIDI_SIGNAL_CONTROL_127)
    return TRUE;
  return FALSE;
}
guint
bse_part_insert_control (BsePart          *self,
                         guint             tick,
                         BseMidiSignalType ctype,
                         gfloat            value)
{
  BsePartTickNode *node;
  BsePartEventControl *cev;
  guint id;
  g_return_val_if_fail (BSE_IS_PART (self), BSE_ERROR_INTERNAL);
  if (!(value >= -1 && value <= +1 &&
        tick < BSE_PART_MAX_TICK &&
        check_valid_control_type (ctype) &&
        !BSE_PART_NOTE_CONTROL (ctype)))
    return 0;
  node = bse_part_controls_ensure_tick (&self->controls, tick);
  /* coalesce multiple inserts */
  for (cev = node->events; cev; cev = cev->next)
    if (cev->ctype == ctype)
      {
        bse_part_controls_change (&self->controls, node, cev,
                                  cev->id, cev->selected, cev->ctype, value);
        queue_control_update (self, tick);
        return cev->id;
      }
  /* insert new event */
  id = bse_part_alloc_id (self, tick);
  bse_part_controls_insert (&self->controls, node, id, FALSE, ctype, value);
  queue_control_update (self, tick);
  if (tick >= self->last_tick_SL)
    part_update_last_tick (self);
  return id;
}
gboolean
bse_part_change_note (BsePart *self,
		      guint    id,
                      guint    channel,
		      guint    tick,
		      guint    duration,
		      gint     vnote,
		      gint     fine_tune,
		      gfloat   velocity)
{
  BsePartEventNote key = { 0 }, *note;
  const bool use_any_channel = channel == ~uint (0);
  guint i, old_tick;
  g_return_val_if_fail (BSE_IS_PART (self), FALSE);
  if (use_any_channel)
    channel = 0;
  g_return_val_if_fail (channel < self->n_channels, FALSE);
  if (!(BSE_NOTE_IS_VALID (vnote) && channel < self->n_channels &&
	BSE_FINE_TUNE_IS_VALID (fine_tune) &&
	tick < BSE_PART_MAX_TICK &&
	duration > 0 &&
	duration < BSE_PART_MAX_TICK &&
	tick + duration <= BSE_PART_MAX_TICK))
    return FALSE;
  old_tick = bse_part_tick_from_id (self, id);
  if (old_tick > BSE_PART_MAX_TICK)
    return FALSE;       /* invalid id */
  /* ensure (target) tick is valid */
  note = bse_part_note_channel_lookup (&self->channels[channel], tick);
  if (note && note->id != id)
    {
      if (!use_any_channel)
        return FALSE;   /* slot taken */
      /* find (create) a free channel */
      for (channel += 1; channel < self->n_channels; channel++)
        if (!bse_part_note_channel_lookup (&self->channels[channel], tick))
          break;
      if (channel >= self->n_channels)
        part_add_channel (self, TRUE);
      note = NULL;
    }
  /* find note */
  if (!note)
    {
      for (i = 0; i < self->n_channels; i++)
        {
          note = bse_part_note_channel_lookup (&self->channels[i], old_tick);
          if (note && note->id == id)
            break;
        }
      if (!note)
        return FALSE;   /* no such note */
    }
  else
    i = channel;
  /* move note */
  queue_note_update (self, note);
  key.tick = tick;
  key.id = note->id;
  key.selected = note->selected;
  key.duration = duration;
  key.note = vnote;
  key.fine_tune = fine_tune;
  key.velocity = velocity;
  if (note->tick != key.tick || note->duration != key.duration)
    {
      guint ltick = note->tick + note->duration;
      bse_part_note_channel_remove (&self->channels[i], note->tick);
      bse_part_move_id (self, id, tick);
      bse_part_note_channel_insert (&self->channels[channel], key);
      if (MAX (ltick, key.tick + key.duration) >= self->last_tick_SL)
        part_update_last_tick (self);
    }
  else
    bse_part_note_channel_change_note (&self->channels[channel], note, key.id, key.selected,
                                       key.note, key.fine_tune, key.velocity);
  queue_note_update (self, &key);
  return TRUE;
}
gboolean
bse_part_change_control (BsePart           *self,
                         guint              id,
                         guint              tick,
                         BseMidiSignalType  ctype,
                         gfloat             value)
{
  guint old_tick;
  g_return_val_if_fail (BSE_IS_PART (self), FALSE);
  if (!(tick < BSE_PART_MAX_TICK &&
        check_valid_control_type (ctype) &&
        value >= -1 && value <= +1))
    return FALSE;
  old_tick = bse_part_tick_from_id (self, id);
  if (old_tick > BSE_PART_MAX_TICK)
    return FALSE;       /* invalid id */
  if (!BSE_PART_NOTE_CONTROL (ctype))
    {
      BsePartEventControl *cev = NULL;
      BsePartTickNode *node;
      gboolean selected;
      /* check target */
      node = bse_part_controls_ensure_tick (&self->controls, tick);
      for (cev = node->events; cev; cev = cev->next)
        if (cev->ctype == ctype)
          {
            if (cev->id != id)
              return FALSE;   /* slot taken */
            break;
          }
      /* find event */
      if (!cev)
        cev = bse_part_controls_lookup_event (&self->controls, old_tick, id);
      if (!cev)
        return FALSE;   /* no such control */
      /* move control */
      queue_control_update (self, old_tick);
      selected = cev->selected;
      if (tick != old_tick)
        {
          bse_part_controls_remove (&self->controls, old_tick, cev);    /* invalidates node */
          bse_part_move_id (self, id, tick);
          node = bse_part_controls_ensure_tick (&self->controls, tick);
          bse_part_controls_insert (&self->controls, node, id, selected, ctype, value);
          queue_control_update (self, tick);
          if (MAX (old_tick, tick) >= self->last_tick_SL)
            part_update_last_tick (self);
        }
      else
        bse_part_controls_change (&self->controls, node, cev, id, selected, ctype, value);
      return TRUE;
    }
  else
    {
      guint channel;
      /* find note */
      for (channel = 0; channel < self->n_channels; channel++)
        {
          BsePartEventNote *note = bse_part_note_channel_lookup (&self->channels[channel], old_tick);
          if (note && note->id == id)
            {
              gint   fine_tune = note->fine_tune;
              gfloat velocity = note->velocity;
              switch (ctype)
                {
                case BSE_MIDI_SIGNAL_VELOCITY:
                  velocity = CLAMP (value, 0, +1);
                  break;
                case BSE_MIDI_SIGNAL_FINE_TUNE:
                  fine_tune = bse_ftoi (value * BSE_MAX_FINE_TUNE);
                  fine_tune = CLAMP (fine_tune, BSE_MIN_FINE_TUNE, BSE_MAX_FINE_TUNE);
                  break;
                default: ;
                }
              return bse_part_change_note (self, note->id, channel, tick, note->duration,
                                           note->note, fine_tune, velocity);
            }
        }
      return FALSE;
    }
}
static inline gfloat
note_get_control_value (BsePartEventNote *note,
                        BseMidiSignalType ctype)
{
  switch (ctype)
    {
    case BSE_MIDI_SIGNAL_VELOCITY:
      return note->velocity;
    case BSE_MIDI_SIGNAL_FINE_TUNE:
      return note->fine_tune * (1.0 / BSE_MAX_FINE_TUNE);
    default:
      return 0;
    }
}
BsePartEventType
bse_part_query_event (BsePart           *self,
                      guint              id,
                      BsePartQueryEvent *equery)
{
  BsePartEventNote *note = NULL;
  BsePartEventControl *cev;
  guint tick, channel;
  g_return_val_if_fail (BSE_IS_PART (self), BSE_PART_EVENT_NONE);
  tick = bse_part_tick_from_id (self, id);
  if (tick > BSE_PART_MAX_TICK)
    return BSE_PART_EVENT_NONE; /* invalid id */
  /* lookup control */
  cev = bse_part_controls_lookup_event (&self->controls, tick, id);
  if (cev)
    {
      if (equery)
        {
          equery->id = id;
          equery->event_type = BSE_PART_EVENT_CONTROL;
          equery->channel = 0;
          equery->tick = tick;
          equery->selected = cev->selected;
          equery->duration = 0;
          equery->note = 0;
          equery->fine_tune = 0;
          equery->velocity = 0;
          equery->fine_tune_value = 0;
          equery->velocity_value = 0;
          equery->control_type = BseMidiSignalType (cev->ctype);
          equery->control_value = cev->value;
        }
      return BSE_PART_EVENT_CONTROL;
    }
  /* find note */
  for (channel = 0; channel < self->n_channels; channel++)
    {
      note = bse_part_note_channel_lookup (&self->channels[channel], tick);
      if (note && note->id == id)
        break;
    }
  if (note)
    {
      if (equery)
        {
          equery->id = id;
          equery->event_type = BSE_PART_EVENT_NOTE;
          equery->channel = channel;
          equery->tick = note->tick;
          equery->selected = note->selected;
          equery->duration = note->duration;
          equery->note = note->note;
          equery->fine_tune = note->fine_tune;
          equery->velocity = note->velocity;
          equery->fine_tune_value = note_get_control_value (note, BSE_MIDI_SIGNAL_FINE_TUNE);
          equery->velocity_value = note_get_control_value (note, BSE_MIDI_SIGNAL_VELOCITY);
          equery->control_type = BseMidiSignalType (0);
          equery->control_value = 0;
        }
      return BSE_PART_EVENT_NOTE;
    }
  return BSE_PART_EVENT_NONE;
}
static void
part_note_seq_append (BsePartNoteSeq   *pseq,
                      guint             channel,
                      BsePartEventNote *note)
{
  BsePartNote *pnote = bse_part_note (note->id,
                                      channel,
                                      note->tick,
                                      note->duration,
                                      note->note,
                                      note->fine_tune,
                                      note->velocity,
                                      note->selected);
  bse_part_note_seq_take_append (pseq, pnote);
}
static void
part_control_seq_append_note (BsePartControlSeq *cseq,
                              BsePartEventNote  *note,
                              BseMidiSignalType  ctype)
{
  BsePartControl *ctrl = bse_part_control (note->id,
                                           note->tick,
                                           ctype,
                                           note_get_control_value (note, ctype),
                                           note->selected);
  bse_part_control_seq_take_append (cseq, ctrl);
}
BsePartNoteSeq*
bse_part_list_notes (BsePart *self,
                     guint    match_channel,
                     guint    tick,
                     guint    duration,
                     gint     min_note,
                     gint     max_note,
                     gboolean include_crossings)
{
  BsePartEventNote *bound, *note;
  BsePartNoteSeq *pseq;
  guint n, j, channel;
  gulong *ids;
  g_return_val_if_fail (BSE_IS_PART (self), NULL);
  g_return_val_if_fail (tick < BSE_PART_MAX_TICK, NULL);
  g_return_val_if_fail (duration > 0 && duration <= BSE_PART_MAX_TICK, NULL);
  pseq = bse_part_note_seq_new ();
  for (channel = 0; channel < self->n_channels; channel++)
    {
      SfiUPool *tickpool;
      if (channel != match_channel && match_channel != ~uint (0))
        continue;
      tickpool = sfi_upool_new ();
      /* gather notes spanning across tick */
      note = include_crossings ? bse_part_note_channel_lookup_lt (&self->channels[channel], tick) : NULL;
      if (note)
        for (j = 0; j < BSE_PART_NOTE_N_CROSSINGS (note); j++)
          {
            BsePartEventNote *xnote = bse_part_note_channel_lookup (&self->channels[channel],
                                                                    BSE_PART_NOTE_CROSSING (note, j));
            if (xnote->tick + xnote->duration > tick &&
                xnote->note >= min_note && xnote->note <= max_note)
              sfi_upool_set (tickpool, xnote->tick);
          }
      if (note && note->tick + note->duration > tick && include_crossings &&
          note->note >= min_note && note->note <= max_note)
        sfi_upool_set (tickpool, note->tick);
      /* gather notes starting during duration */
      note = bse_part_note_channel_lookup_ge (&self->channels[channel], tick);
      bound = note ? bse_part_note_channel_get_bound (&self->channels[channel]) : NULL;
      while (note < bound && note->tick < tick + duration)
        {
          if (note->note >= min_note && note->note <= max_note)
            sfi_upool_set (tickpool, note->tick);
          note++;
        }
      /* add notes to sequence */
      ids = sfi_upool_list (tickpool, &n);
      sfi_upool_destroy (tickpool);
      for (j = 0; j < n; j++)
        {
          note = bse_part_note_channel_lookup (&self->channels[channel], ids[j]);
          part_note_seq_append (pseq, channel, note);
        }
      g_free (ids);
    }
  return pseq;
}
BsePartControlSeq*
bse_part_list_controls (BsePart          *self,
                        guint             match_channel, /* for note events */
                        guint             tick,
                        guint             duration,
                        BseMidiSignalType ctype)
{
  BsePartControlSeq *cseq;
  g_return_val_if_fail (BSE_IS_PART (self), NULL);
  g_return_val_if_fail (tick < BSE_PART_MAX_TICK, NULL);
  g_return_val_if_fail (duration > 0 && duration <= BSE_PART_MAX_TICK, NULL);
  cseq = bse_part_control_seq_new ();
  if (BSE_PART_NOTE_CONTROL (ctype))
    {
      guint channel;
      for (channel = 0; channel < self->n_channels; channel++)
        {
          BsePartEventNote *note = bse_part_note_channel_lookup_ge (&self->channels[channel], tick);
          BsePartEventNote *last = bse_part_note_channel_lookup_lt (&self->channels[channel], tick + duration);
          if (!note)
            continue;
          if (channel != match_channel && match_channel != ~uint (0))
            continue;
          while (note <= last)
            {
              part_control_seq_append_note (cseq, note, ctype);
              note++;
            }
        }
    }
  else
    {
      BsePartTickNode *node = bse_part_controls_lookup_ge (&self->controls, tick);
      BsePartTickNode *last = bse_part_controls_lookup_lt (&self->controls, tick + duration);
      if (!node)
        return cseq;
      while (node <= last)
        {
          BsePartEventControl *cev;
          for (cev = node->events; cev; cev = cev->next)
            if (cev->ctype == ctype)
              bse_part_control_seq_take_append (cseq,
                                                bse_part_control (cev->id,
                                                                  node->tick,
                                                                  BseMidiSignalType (cev->ctype),
                                                                  cev->value,
                                                                  cev->selected));
          node++;
        }
    }
  return cseq;
}
void
bse_part_queue_notes_within (BsePart *self,
			     guint    tick,
			     guint    duration,
			     gint     min_note,
			     gint     max_note)
{
  guint end_tick, channel;
  g_return_if_fail (BSE_IS_PART (self));
  g_return_if_fail (tick < BSE_PART_MAX_TICK);
  g_return_if_fail (duration > 0 && duration <= BSE_PART_MAX_TICK);
  min_note = BSE_NOTE_CLAMP (min_note);
  max_note = BSE_NOTE_CLAMP (max_note);
  end_tick = tick + MAX (duration, 1);
  /* widen area to right if notes span across right boundary */
  for (channel = 0; channel < self->n_channels; channel++)
    {
      BsePartEventNote *note = bse_part_note_channel_lookup_lt (&self->channels[channel], tick + duration);
      if (note && note->tick >= tick)
        {
          guint j;
          for (j = 0; j < BSE_PART_NOTE_N_CROSSINGS (note); j++)
            {
              BsePartEventNote *xnote = bse_part_note_channel_lookup (&self->channels[channel],
                                                                      BSE_PART_NOTE_CROSSING (note, j));
              if (xnote->tick >= tick && xnote->note >= min_note && xnote->note <= max_note)
                end_tick = MAX (end_tick, xnote->tick + xnote->duration);
            }
          if (note->note >= min_note && note->note <= max_note)
            end_tick = MAX (end_tick, note->tick + note->duration);
        }
    }
  queue_update (self, tick, end_tick - tick, min_note);
  queue_update (self, tick, end_tick - tick, max_note);
}
BsePartNoteSeq*
bse_part_list_selected_notes (BsePart *self)
{
  BsePartNoteSeq *pseq;
  guint channel;
  g_return_val_if_fail (BSE_IS_PART (self), NULL);
  pseq = bse_part_note_seq_new ();
  for (channel = 0; channel < self->n_channels; channel++)
    {
      BsePartEventNote *note = bse_part_note_channel_lookup_ge (&self->channels[channel], 0);
      BsePartEventNote *bound = note ? bse_part_note_channel_get_bound (&self->channels[channel]) : NULL;
      while (note < bound)
        {
          if (note->selected)
            part_note_seq_append (pseq, channel, note);
          note++;
        }
    }
  return pseq;
}
BsePartControlSeq*
bse_part_list_selected_controls (BsePart           *self,
                                 BseMidiSignalType  ctype)
{
  BsePartControlSeq *cseq;
  g_return_val_if_fail (BSE_IS_PART (self), NULL);
  cseq = bse_part_control_seq_new ();
  if (BSE_PART_NOTE_CONTROL (ctype))
    {
      guint channel;
      for (channel = 0; channel < self->n_channels; channel++)
        {
          BsePartEventNote *note = bse_part_note_channel_lookup_ge (&self->channels[channel], 0);
          BsePartEventNote *bound = bse_part_note_channel_get_bound (&self->channels[channel]);
          while (note < bound)
            {
              if (note->selected)
                part_control_seq_append_note (cseq, note, ctype);
              note++;
            }
        }
    }
  else
    {
      BsePartTickNode *node = bse_part_controls_lookup_ge (&self->controls, 0);
      BsePartTickNode *bound = bse_part_controls_get_bound (&self->controls);
      while (node < bound)
        {
          BsePartEventControl *cev;
          for (cev = node->events; cev; cev = cev->next)
            if (cev->ctype == ctype && cev->selected)
              bse_part_control_seq_take_append (cseq,
                                                bse_part_control (cev->id,
                                                                  node->tick,
                                                                  BseMidiSignalType (cev->ctype),
                                                                  cev->value,
                                                                  cev->selected));
          node++;
        }
    }
  return cseq;
}
static void
bse_part_store_private (BseObject  *object,
			BseStorage *storage)
{
  BsePart *self = BSE_PART (object);
  BsePartTickNode *node, *bound;
  gboolean statement_started = FALSE;
  guint channel;
  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->store_private)
    BSE_OBJECT_CLASS (parent_class)->store_private (object, storage);
  for (channel = 0; channel < self->n_channels; channel++)
    {
      BsePartEventNote *note = bse_part_note_channel_lookup_ge (&self->channels[channel], 0);
      BsePartEventNote *bound = bse_part_note_channel_get_bound (&self->channels[channel]);
      if (!note)
        continue;
      if (note < bound && !statement_started)
        {
          bse_storage_break (storage);
          bse_storage_printf (storage, "(insert-notes %u", channel);
          bse_storage_push_level (storage);
        }
      while (note < bound)
        {
          bse_storage_break (storage);
          bse_storage_printf (storage, "(0x%05x 0x%03x %d",
                              note->tick, note->duration, note->note);
          if (note->fine_tune != 0 || note->velocity != 1.0)
            {
              bse_storage_printf (storage, " %d", note->fine_tune);
              if (note->velocity != 1.0)
                {
                  bse_storage_putc (storage, ' ');
                  bse_storage_putf (storage, note->velocity);
                }
            }
          bse_storage_putc (storage, ')');
          note++;
        }
      if (!statement_started)
        {
          bse_storage_pop_level (storage);
          bse_storage_putc (storage, ')');
          statement_started = FALSE;
        }
    }
  node = bse_part_controls_lookup_ge (&self->controls, 0);
  bound = bse_part_controls_get_bound (&self->controls);
  while (node < bound)
    {
      BsePartEventControl *cev;
      if (node->events && !statement_started)
        {
          statement_started = TRUE;
          bse_storage_break (storage);
          bse_storage_printf (storage, "(insert-controls");
          bse_storage_push_level (storage);
        }
      for (cev = node->events; cev; cev = cev->next)
        {
          const gchar *choice = sfi_enum2choice (cev->ctype, BSE_TYPE_MIDI_SIGNAL_TYPE);
          bse_storage_break (storage);
          if (strncmp (choice, "bse-midi-signal-", 16) == 0)
            choice += 16;
          bse_storage_printf (storage, "(0x%05x %s ", node->tick, choice);
          bse_storage_putf (storage, cev->value);
          bse_storage_putc (storage, ')');
        }
      node++;
    }
  if (statement_started)
    {
      bse_storage_pop_level (storage);
      bse_storage_putc (storage, ')');
    }
}
static GTokenType
bse_part_restore_private (BseObject  *object,
			  BseStorage *storage,
                          GScanner   *scanner)
{
  BsePart *self = BSE_PART (object);
  GQuark quark;
  /* chain parent class' handler */
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return BSE_OBJECT_CLASS (parent_class)->restore_private (object, storage, scanner);
  /* parse storage commands */
  quark = g_quark_try_string (scanner->next_value.v_identifier);
  if (quark == quark_insert_notes)
    {
      guint channel;
      parse_or_return (scanner, G_TOKEN_IDENTIFIER);	/* eat identifier */
      parse_or_return (scanner, G_TOKEN_INT);           /* channel */
      channel = scanner->value.v_int64;
      if (channel >= self->n_channels)
        return bse_storage_warn_skip (storage, "ignoring notes with invalid channel: %u", channel);
      while (g_scanner_peek_next_token (scanner) != ')')
        {
          guint tick, duration, note;
          gint fine_tune = 0;
          gfloat velocity = 1.0;
          gboolean negate;
          parse_or_return (scanner, '(');
          parse_or_return (scanner, G_TOKEN_INT);       /* tick */
          tick = scanner->value.v_int64;
          parse_or_return (scanner, G_TOKEN_INT);       /* duration */
          duration = scanner->value.v_int64;
          parse_or_return (scanner, G_TOKEN_INT);       /* note */
          note = scanner->value.v_int64;
          negate = bse_storage_check_parse_negate (storage);
          if (g_scanner_peek_next_token (scanner) == G_TOKEN_INT)
            {
              g_scanner_get_next_token (scanner);       /* fine_tune */
              fine_tune = scanner->value.v_int64;
              if (negate)
                fine_tune = -fine_tune;
              negate = bse_storage_check_parse_negate (storage);
              if (g_scanner_peek_next_token (scanner) == G_TOKEN_INT)
                {
                  g_scanner_get_next_token (scanner);   /* velocity */
                  velocity = scanner->value.v_int64;
                  velocity = negate ? -velocity : velocity;
                }
              else if (g_scanner_peek_next_token (scanner) == G_TOKEN_FLOAT)
                {
                  g_scanner_get_next_token (scanner);   /* velocity */
                  velocity = negate ? -scanner->value.v_float : scanner->value.v_float;
                }
            }
          parse_or_return (scanner, ')');
          if (!bse_part_insert_note (self, channel, tick, duration, note, fine_tune, velocity))
            bse_storage_warn (storage, "note insertion (channel=%u tick=%u duration=%u note=%u) failed",
                              channel, tick, duration, note);
        }
      parse_or_return (scanner, ')');
      return G_TOKEN_NONE;
    }
  else if (quark == quark_insert_controls)
    {
      parse_or_return (scanner, G_TOKEN_IDENTIFIER);	/* eat identifier */
      while (g_scanner_peek_next_token (scanner) != ')')
        {
          guint tick, ctype;
          gfloat value;
          gboolean negate;
          GError *error = NULL;
          parse_or_return (scanner, '(');
          parse_or_return (scanner, G_TOKEN_INT);       /* tick */
          tick = scanner->value.v_int64;
          parse_or_return (scanner, G_TOKEN_IDENTIFIER); /* type */
          ctype = sfi_choice2enum_checked (scanner->value.v_identifier, BSE_TYPE_MIDI_SIGNAL_TYPE, &error);
          negate = bse_storage_check_parse_negate (storage);
          if (g_scanner_peek_next_token (scanner) == G_TOKEN_INT)
            {
              g_scanner_get_next_token (scanner);       /* value as int */
              value = scanner->value.v_int64;
              if (negate)
                value = -value;
            }
          else if (g_scanner_peek_next_token (scanner) == G_TOKEN_FLOAT)
            {
              g_scanner_get_next_token (scanner);	/* value as float */
              value = negate ? -scanner->value.v_float : scanner->value.v_float;
            }
          else
            {
              g_clear_error (&error);
              return G_TOKEN_FLOAT;
            }
          if (g_scanner_peek_next_token (scanner) != ')')
            g_clear_error (&error);
          parse_or_return (scanner, ')');
          if (error)
            bse_storage_warn (storage, "unknown control event: %s", error->message);
          else if (!bse_part_insert_control (self, tick, BseMidiSignalType (ctype), CLAMP (value, -1, +1)))
            bse_storage_warn (storage, "failed to insert control event of type: %d", ctype);
          g_clear_error (&error);
        }
      parse_or_return (scanner, ')');
      return G_TOKEN_NONE;
    }
  else if (quark == quark_insert_note)       /* pre-0.6.0 */
    {
      guint tick, duration, note;
      gint fine_tune = 0;
      gfloat velocity = 1.0;
      gboolean negate;
      parse_or_return (scanner, G_TOKEN_IDENTIFIER);	/* eat identifier */
      parse_or_return (scanner, G_TOKEN_INT);
      tick = scanner->value.v_int64;
      parse_or_return (scanner, G_TOKEN_INT);
      duration = scanner->value.v_int64;
      parse_or_return (scanner, G_TOKEN_INT);
      note = scanner->value.v_int64;
      negate = bse_storage_check_parse_negate (storage);
      if (g_scanner_peek_next_token (scanner) == G_TOKEN_INT)
	{
	  g_scanner_get_next_token (scanner);		/* eat int */
	  fine_tune = scanner->value.v_int64;
	  if (negate)
            fine_tune = -fine_tune;
          negate = bse_storage_check_parse_negate (storage);
	  if (g_scanner_peek_next_token (scanner) == G_TOKEN_FLOAT)
	    {
	      g_scanner_get_next_token (scanner);	/* eat float */
	      velocity = negate ? -scanner->value.v_float : scanner->value.v_float;
	    }
	}
      parse_or_return (scanner, ')');
      if (!bse_part_insert_note (self, ~0, tick, duration, note, fine_tune, velocity))
	bse_storage_warn (storage, "note insertion (note=%d tick=%u duration=%u) failed",
			  note, tick, duration);
      return G_TOKEN_NONE;
    }
  else if (quark == quark_insert_control)       /* pre-0.6.0 */
    {
      guint tick, ctype;
      gfloat value;
      gboolean negate;
      parse_or_return (scanner, G_TOKEN_IDENTIFIER);	/* eat identifier */
      parse_or_return (scanner, G_TOKEN_INT);
      tick = scanner->value.v_int64;
      parse_or_return (scanner, G_TOKEN_IDENTIFIER);
      ctype = sfi_choice2enum (scanner->value.v_identifier, BSE_TYPE_MIDI_SIGNAL_TYPE);
      negate = bse_storage_check_parse_negate (storage);
      if (g_scanner_peek_next_token (scanner) == G_TOKEN_INT)
	{
	  g_scanner_get_next_token (scanner);		/* eat int */
	  value = scanner->value.v_int64;
          if (negate)
            value = -value;
        }
      else if (g_scanner_peek_next_token (scanner) == G_TOKEN_FLOAT)
        {
          g_scanner_get_next_token (scanner);	/* eat float */
          value = negate ? -scanner->value.v_float : scanner->value.v_float;
        }
      else
        return G_TOKEN_FLOAT;
      parse_or_return (scanner, ')');
      if (!bse_part_insert_control (self, tick, BseMidiSignalType (ctype), CLAMP (value, -1, +1)))
        bse_storage_warn (storage, "skipping control event of invalid type: %d", ctype);
      return G_TOKEN_NONE;
    }
  else /* chain parent class' handler */
    return BSE_OBJECT_CLASS (parent_class)->restore_private (object, storage, scanner);
}
/* --- BsePartControls --- */
static gint
part_controls_cmp_tick_nodes (gconstpointer bsearch_node1, /* key */
                              gconstpointer bsearch_node2)
{
  const BsePartTickNode *n1 = (const BsePartTickNode*) bsearch_node1;
  const BsePartTickNode *n2 = (const BsePartTickNode*) bsearch_node2;
  return G_BSEARCH_ARRAY_CMP (n1->tick, n2->tick);
}
static const GBSearchConfig controls_bsc = {
  sizeof (BsePartTickNode),
  part_controls_cmp_tick_nodes,
  G_BSEARCH_ARRAY_ALIGN_POWER2,
};
void
bse_part_controls_init (BsePartControls *self)
{
  self->bsa = g_bsearch_array_create (&controls_bsc);
}
BsePartTickNode*
bse_part_controls_lookup (BsePartControls     *self,
                          guint                tick)
{
  BsePartTickNode key, *node;
  key.tick = tick;
  node = (BsePartTickNode*) g_bsearch_array_lookup (self->bsa, &controls_bsc, &key);
  return node;
}
BsePartEventControl*
bse_part_controls_lookup_event (BsePartControls     *self,
                                guint                tick,
                                guint                id)
{
  BsePartTickNode key, *node;
  key.tick = tick;
  node = (BsePartTickNode*) g_bsearch_array_lookup (self->bsa, &controls_bsc, &key);
  if (node)
    {
      BsePartEventControl *cev;
      for (cev = node->events; cev; cev = cev->next)
        if (cev->id == id)
          return cev;
    }
  return NULL;
}
BsePartTickNode*
bse_part_controls_lookup_ge (BsePartControls     *self,
                             guint                tick)
{
  BsePartTickNode key, *node;
  key.tick = tick;
  node = (BsePartTickNode*) g_bsearch_array_lookup_sibling (self->bsa, &controls_bsc, &key);
  if (node && node->tick < tick)        /* adjust smaller ticks */
    {
      guint ix = 1 + g_bsearch_array_get_index (self->bsa, &controls_bsc, node);
      node = (BsePartTickNode*) g_bsearch_array_get_nth (self->bsa, &controls_bsc, ix); /* returns NULL for i >= n_nodes */
      g_assert (!node || node->tick >= tick);
    }
  return node;
}
BsePartTickNode*
bse_part_controls_lookup_le (BsePartControls     *self,
                             guint                tick)
{
  BsePartTickNode key, *node;
  key.tick = tick;
  node = (BsePartTickNode*) g_bsearch_array_lookup_sibling (self->bsa, &controls_bsc, &key);
  if (node && node->tick > tick)        /* adjust smaller ticks */
    {
      node = g_bsearch_array_get_index (self->bsa, &controls_bsc, node) > 0 ? node - 1 : NULL;
      g_assert (!node || node->tick <= tick);
    }
  return node;
}
BsePartTickNode*
bse_part_controls_lookup_lt (BsePartControls     *self,
                             guint                tick)
{
  return tick ? bse_part_controls_lookup_le (self, tick - 1) : NULL;
}
BsePartTickNode*
bse_part_controls_get_bound (BsePartControls *self)
{
  guint nn = g_bsearch_array_get_n_nodes (self->bsa);
  BsePartTickNode *first = (BsePartTickNode*) g_bsearch_array_get_nth (self->bsa, &controls_bsc, 0);
  return first ? first + nn : NULL;
}
guint
bse_part_controls_get_last_tick (BsePartControls *self)
{
  guint n_nodes = g_bsearch_array_get_n_nodes (self->bsa);
  if (n_nodes)
    {
      BsePartTickNode *node = (BsePartTickNode*) g_bsearch_array_get_nth (self->bsa, &controls_bsc, n_nodes - 1);
      return node->tick + 1;
    }
  return 0;
}
BsePartTickNode*
bse_part_controls_ensure_tick (BsePartControls *self,
                               guint            tick)
{
  BsePartTickNode key = { 0 }, *node;
  key.tick = tick;
  node = (BsePartTickNode*) g_bsearch_array_lookup (self->bsa, &controls_bsc, &key);
  if (!node)
    {
      BSE_SEQUENCER_LOCK ();
      self->bsa = g_bsearch_array_insert (self->bsa, &controls_bsc, &key);
      BSE_SEQUENCER_UNLOCK ();
      node = (BsePartTickNode*) g_bsearch_array_lookup (self->bsa, &controls_bsc, &key);
    }
  return node;
}
void
bse_part_controls_insert (BsePartControls     *self,
                          BsePartTickNode     *node,
                          guint                id,
                          guint                selected,
                          guint                ctype,
                          gfloat               value)
{
  BsePartEventControl *cev = sfi_new_struct0 (BsePartEventControl, 1);
  cev->id = id;
  cev->selected = selected;
  cev->ctype = ctype;
  cev->value = value;
  BSE_SEQUENCER_LOCK ();
  cev->next = node->events;
  node->events = cev;
  BSE_SEQUENCER_UNLOCK ();
}
void
bse_part_controls_change (BsePartControls     *self,
                          BsePartTickNode     *node,
                          BsePartEventControl *cev,
                          guint                id,
                          guint                selected,
                          guint                ctype,
                          gfloat               value)
{
  /* carefull with sequencer lock here */
  cev->id = id;
  cev->selected = selected != FALSE;
  if (cev->ctype != ctype || cev->value != value)
    {
      BSE_SEQUENCER_LOCK ();
      cev->ctype = ctype;
      cev->value = value;
      BSE_SEQUENCER_UNLOCK ();
    }
}
void
bse_part_controls_change_selected (BsePartEventControl *cev,
                                   guint                selected)
{
  /* carefull with sequencer lock here */
  cev->selected = selected != FALSE;
}
void
bse_part_controls_remove (BsePartControls     *self,
                          guint                tick,
                          BsePartEventControl *delcev)
{
  BsePartTickNode *node = bse_part_controls_lookup (self, tick);
  BsePartEventControl *last = NULL, *cev;
  g_return_if_fail (node != NULL);
  for (cev = node->events; cev; last = cev, cev = cev->next)
    if (cev == delcev)
      {
        BSE_SEQUENCER_LOCK ();
        if (last)
          last->next = cev->next;
        else
          node->events = cev->next;
        BSE_SEQUENCER_UNLOCK ();
        sfi_delete_struct (BsePartEventControl, cev);
        break;
      }
  if (!cev)
    g_warning ("%s: failed to remove event at tick=%u", G_STRFUNC, tick);
  else if (!node->events)
    {
      /* remove node */
      BSE_SEQUENCER_LOCK ();
      self->bsa = g_bsearch_array_remove_node (self->bsa, &controls_bsc, node);
      BSE_SEQUENCER_UNLOCK ();
    }
}
void
bse_part_controls_destroy (BsePartControls *self)
{
  guint nn = g_bsearch_array_get_n_nodes (self->bsa);
  while (nn)
    {
      BsePartTickNode *node = (BsePartTickNode*) g_bsearch_array_get_nth (self->bsa, &controls_bsc, --nn);
      BsePartEventControl *cev, *next;
      for (cev = node->events; cev; cev = next)
        {
          next = cev->next;
          sfi_delete_struct (BsePartEventControl, cev);
        }
    }
  g_bsearch_array_free (self->bsa, &controls_bsc);
  self->bsa = NULL;
}
/* --- BsePartNoteChannel --- */
static gint
part_note_channel_cmp_notes (gconstpointer bsearch_node1, /* key */
                             gconstpointer bsearch_node2)
{
  const BsePartEventNote *n1 = (const BsePartEventNote*) bsearch_node1;
  const BsePartEventNote *n2 = (const BsePartEventNote*) bsearch_node2;
  return G_BSEARCH_ARRAY_CMP (n1->tick, n2->tick);
}
static const GBSearchConfig note_channel_bsc = {
  sizeof (BsePartEventNote),
  part_note_channel_cmp_notes,
  G_BSEARCH_ARRAY_ALIGN_POWER2,
};
void
bse_part_note_channel_init (BsePartNoteChannel *self)
{
  self->bsa = g_bsearch_array_create (&note_channel_bsc);
}
BsePartEventNote*
bse_part_note_channel_lookup (BsePartNoteChannel     *self,
                              guint                   tick)
{
  BsePartEventNote key, *note;
  key.tick = tick;
  note = (BsePartEventNote*) g_bsearch_array_lookup (self->bsa, &note_channel_bsc, &key);
  return note;
}
BsePartEventNote*
bse_part_note_channel_get_bound (BsePartNoteChannel *self)
{
  guint nn = g_bsearch_array_get_n_nodes (self->bsa);
  BsePartEventNote *first = (BsePartEventNote*) g_bsearch_array_get_nth (self->bsa, &note_channel_bsc, 0);
  return first ? first + nn : NULL;
}
BsePartEventNote*
bse_part_note_channel_lookup_le (BsePartNoteChannel     *self,
                                 guint                   tick)
{
  BsePartEventNote key, *note;
  key.tick = tick;
  note = (BsePartEventNote*) g_bsearch_array_lookup_sibling (self->bsa, &note_channel_bsc, &key);
  if (note && note->tick > tick)        /* adjust greater ticks */
    {
      note = g_bsearch_array_get_index (self->bsa, &note_channel_bsc, note) > 0 ? note - 1 : NULL;
      g_assert (!note || note->tick <= tick);
    }
  return note;
}
BsePartEventNote*
bse_part_note_channel_lookup_lt (BsePartNoteChannel     *self,
                                 guint                   tick)
{
  return tick ? bse_part_note_channel_lookup_le (self, tick - 1) : NULL;
}
BsePartEventNote*
bse_part_note_channel_lookup_ge (BsePartNoteChannel     *self,
                                 guint                   tick)
{
  BsePartEventNote key, *note;
  key.tick = tick;
  note = (BsePartEventNote*) g_bsearch_array_lookup_sibling (self->bsa, &note_channel_bsc, &key);
  if (note && note->tick < tick)        /* adjust smaller ticks */
    {
      guint ix = 1 + g_bsearch_array_get_index (self->bsa, &note_channel_bsc, note);
      note = (BsePartEventNote*) g_bsearch_array_get_nth (self->bsa, &note_channel_bsc, ix); /* returns NULL for i >= n_nodes */
      g_assert (!note || note->tick >= tick);
    }
  return note;
}
guint
bse_part_note_channel_get_last_tick (BsePartNoteChannel *self)
{
  guint last_tick = 0, n_nodes = g_bsearch_array_get_n_nodes (self->bsa);
  if (n_nodes)
    {
      BsePartEventNote *note = (BsePartEventNote*) g_bsearch_array_get_nth (self->bsa, &note_channel_bsc, n_nodes - 1);
      BsePartEventNote key = { 0 };
      guint i;
      for (i = 0; i < BSE_PART_NOTE_N_CROSSINGS (note); i++)
        {
          BsePartEventNote *xnote;
          key.tick = BSE_PART_NOTE_CROSSING (note, i);
          xnote = (BsePartEventNote*) g_bsearch_array_lookup (self->bsa, &note_channel_bsc, &key);
          last_tick = MAX (last_tick, xnote->tick + xnote->duration);
        }
      last_tick = MAX (last_tick, note->tick + note->duration);
    }
  return last_tick;
}
static inline gboolean
part_note_channel_check_crossing (BsePartNoteChannel *self,
                                  guint               note_tick,
                                  guint               tick_mark)
{
  BsePartEventNote key, *note;
  key.tick = note_tick;
  note = (BsePartEventNote*) g_bsearch_array_lookup (self->bsa, &note_channel_bsc, &key);
  g_assert (note);
  return note->tick + note->duration > tick_mark;
}
static inline guint*
part_note_channel_crossings_add (guint *crossings,
                                 guint  tick)
{
  guint n_crossings = crossings ? crossings[0] : 0;
  n_crossings++;
  crossings = g_renew (guint, crossings, 1 + n_crossings);
  crossings[0] = n_crossings;
  crossings[n_crossings] = tick;
  return crossings;
}
static inline guint*
part_note_channel_crossings_remove (guint *crossings,
                                    guint  tick)
{
  guint i, n_crossings = crossings[0];
  for (i = 1; i <= n_crossings; i++)
    if (crossings[i] == tick)
      {
        crossings[i] = crossings[n_crossings];
        break;
      }
  g_assert (i <= n_crossings);  /* must have found one */
  n_crossings--;
  if (n_crossings)
    crossings[0] = n_crossings;
  else
    {
      g_free (crossings);
      crossings = NULL;
    }
  return crossings;
}
BsePartEventNote*
bse_part_note_channel_insert (BsePartNoteChannel     *self,
                              BsePartEventNote        key)
{
  BsePartEventNote *note;
  guint ix, i;
  key.crossings = NULL;
  /* insert node */
  BSE_SEQUENCER_LOCK ();
  self->bsa = g_bsearch_array_insert (self->bsa, &note_channel_bsc, &key);
  BSE_SEQUENCER_UNLOCK ();
  note = (BsePartEventNote*) g_bsearch_array_lookup (self->bsa, &note_channel_bsc, &key);
  g_assert (note->crossings == NULL && note->id == key.id);
  ix = g_bsearch_array_get_index (self->bsa, &note_channel_bsc, note);
  /* copy predecessor crossings */
  if (ix > 0)
    {
      BsePartEventNote *pre = (BsePartEventNote*) g_bsearch_array_get_nth (self->bsa, &note_channel_bsc, ix - 1);
      guint *crossings = NULL;
      for (i = 0; i < BSE_PART_NOTE_N_CROSSINGS (pre); i++)
        if (part_note_channel_check_crossing (self, BSE_PART_NOTE_CROSSING (pre, i), key.tick))
          crossings = part_note_channel_crossings_add (crossings, BSE_PART_NOTE_CROSSING (pre, i));
      if (part_note_channel_check_crossing (self, pre->tick, key.tick))
        crossings = part_note_channel_crossings_add (crossings, pre->tick);
      BSE_SEQUENCER_LOCK ();
      note->crossings = crossings;
      BSE_SEQUENCER_UNLOCK ();
    }
  /* update successor crossings */
  for (i = ix + 1; i < g_bsearch_array_get_n_nodes (self->bsa); i++)
    {
      BsePartEventNote *node = (BsePartEventNote*) g_bsearch_array_get_nth (self->bsa, &note_channel_bsc, i);
      if (key.tick + key.duration > node->tick)
        {
          BSE_SEQUENCER_LOCK ();
          node->crossings = part_note_channel_crossings_add (node->crossings, key.tick);
          BSE_SEQUENCER_UNLOCK ();
        }
      else
        break;
    }
  return note;
}
void
bse_part_note_channel_change_note (BsePartNoteChannel *self,
                                   BsePartEventNote   *note,
                                   guint               id,
                                   gboolean            selected,
                                   gint                vnote,
                                   gint                fine_tune,
                                   gfloat              velocity)
{
  /* carefull with sequencer lock here */
  note->id = id;
  note->selected = selected != FALSE;
  if (note->note != vnote || note->fine_tune != fine_tune || note->velocity != velocity)
    {
      BSE_SEQUENCER_LOCK ();
      note->note = vnote;
      note->fine_tune = fine_tune;
      note->velocity = velocity;
      BSE_SEQUENCER_UNLOCK ();
    }
}
void
bse_part_note_channel_remove (BsePartNoteChannel     *self,
                              guint                   tick)
{
  BsePartEventNote key, *note, *next, *bound = bse_part_note_channel_get_bound (self);
  key.tick = tick;
  note = (BsePartEventNote*) g_bsearch_array_lookup (self->bsa, &note_channel_bsc, &key);
  key = *note;
  /* update successor crossings */
  for (next = note + 1; next < bound; next++)
    if (next->tick < key.tick + key.duration)
      {
        BSE_SEQUENCER_LOCK ();
        next->crossings = part_note_channel_crossings_remove (next->crossings, key.tick);
        BSE_SEQUENCER_UNLOCK ();
      }
    else
      break;
  /* remove node */
  BSE_SEQUENCER_LOCK ();
  self->bsa = g_bsearch_array_remove_node (self->bsa, &note_channel_bsc, note);
  BSE_SEQUENCER_UNLOCK ();
  /* free predecessor crossings */
  g_free (key.crossings);
}
void
bse_part_note_channel_destroy (BsePartNoteChannel *self)
{
  guint nn = g_bsearch_array_get_n_nodes (self->bsa);
  while (nn)
    {
      BsePartEventNote *note = (BsePartEventNote*) g_bsearch_array_get_nth (self->bsa, &note_channel_bsc, --nn);
      g_free (note->crossings);
    }
  g_bsearch_array_free (self->bsa, &note_channel_bsc);
  self->bsa = NULL;
}
