/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bsepart.h"
#include "bsemain.h"
#include "bsestorage.h"
#include "gslcommon.h"
#include <string.h>

/* --- macros --- */
#define	upper_power2(uint_n)	sfi_alloc_upper_power2 (MAX ((uint_n), 4))
#define parse_or_return         bse_storage_scanner_parse_or_return
#define peek_or_return          bse_storage_scanner_peek_or_return


/* --- properties --- */
enum
{
  PROP_0,
  PROP_LAST_TICK,
};


/* --- prototypes --- */
static void	    bse_part_class_init		(BsePartClass	*class);
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
static BseTokenType bse_part_restore_private	(BseObject	*object,
						 BseStorage	*storage);


/* --- variables --- */
static gpointer parent_class = NULL;
static guint    signal_range_changed = 0;
static guint	handler_id_range_changed = 0;
static GSList  *plist_range_changed = NULL;
static guint	handler_id_last_tick_changed = 0;
static GSList  *plist_last_tick_changed = NULL;


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
				   &info);
}

static void
bse_part_class_init (BsePartClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  // BseItemClass *item_class = BSE_ITEM_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_part_set_property;
  gobject_class->get_property = bse_part_get_property;
  gobject_class->dispose = bse_part_dispose;
  gobject_class->finalize = bse_part_finalize;
  
  object_class->store_private = bse_part_store_private;
  object_class->restore_private = bse_part_restore_private;

  bse_object_class_add_param (object_class, "Limits",
			      PROP_LAST_TICK,
			      sfi_pspec_int ("last_tick", "Last Tick", NULL,
					     0, 0, BSE_PART_MAX_TICK, 384,
					     SFI_PARAM_GUI_READABLE));

  signal_range_changed = bse_object_class_add_signal (object_class, "range-changed",
						      G_TYPE_NONE, 4,
						      G_TYPE_INT, G_TYPE_INT,
						      G_TYPE_INT, G_TYPE_INT);
}

static void
bse_part_init (BsePart *self)
{
  self->n_ids = 1;
  self->ids = g_renew (guint, NULL, 1);
  self->ids[0] = BSE_PART_INVAL_TICK_FLAG + 0;
  self->head_id = 1;
  self->tail_id = 1;
  self->n_nodes = 0;
  self->nodes = g_renew (BsePartNode, NULL, upper_power2 (self->n_nodes));
  self->last_tick_SL = 0;
  self->ltu_queued = FALSE;
  self->ltu_recalc = FALSE;
  self->range_queued = FALSE;
  self->range_tick = BSE_PART_MAX_TICK;
  self->range_bound = 0;
  self->range_min_note = BSE_MAX_NOTE;
  self->range_max_note = 0;
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

  plist_range_changed = g_slist_remove (plist_range_changed, self);
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
  BsePartEvent *ev, *next;
  guint i;
  
  self->ltu_queued = TRUE;
  plist_last_tick_changed = g_slist_remove (plist_last_tick_changed, self);
  self->range_queued = TRUE;
  plist_range_changed = g_slist_remove (plist_range_changed, self);

  g_free (self->ids);
  self->n_ids = 0;
  self->ids = NULL;
  self->head_id = 0;
  self->tail_id = 0;
  
  for (i = 0; i < self->n_nodes; i++)
    for (ev = self->nodes[i].events; ev; ev = next)
      {
	next = ev->any.next;
	sfi_delete_struct (BsePartEvent, ev);
      }
  g_free (self->nodes);
  self->nodes = NULL;
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static guint
bse_part_alloc_id (BsePart *self,
		   guint    tick)
{
  guint next, id;
  
  g_return_val_if_fail (tick <= BSE_PART_MAX_TICK, 0);

  /* we keep a list of ids to implement a fast lookup
   * from id to tick of the id owning event. ticks
   * >= BSE_PART_INVAL_TICK_FLAG are not allocated.
   */

  id = self->head_id;
  next = self->ids[id - 1];
  g_assert (next >= BSE_PART_INVAL_TICK_FLAG);	// FIXME: paranoid
  next -= BSE_PART_INVAL_TICK_FLAG;
  if (!next)	/* last id, allocate new next */
    {
      self->n_ids++;
      self->ids = g_renew (guint, self->ids, self->n_ids);
      self->ids[self->n_ids - 1] = BSE_PART_INVAL_TICK_FLAG + 0;
      self->head_id = self->n_ids;
      self->tail_id = self->n_ids;
    }
  else
    self->head_id = next;
  self->ids[id - 1] = tick;
  return id;
}

static guint
bse_part_move_id (BsePart *self,
		  guint	   id,
		  guint    tick)
{
  g_return_val_if_fail (tick <= BSE_PART_MAX_TICK, 0);
  g_return_val_if_fail (id > 0 && id <= self->n_ids, 0);
  g_return_val_if_fail (self->ids[id - 1] <= BSE_PART_MAX_TICK, 0);	/* check !freed id */
  
  self->ids[id - 1] = tick;
  
  return id;
}

static void
bse_part_free_id (BsePart *self,
		  guint    id)
{
  g_return_if_fail (id > 0 && id <= self->n_ids);
  g_return_if_fail (self->ids[id - 1] <= BSE_PART_MAX_TICK);	/* check !freed id */
  
  self->ids[self->tail_id - 1] = BSE_PART_INVAL_TICK_FLAG + id;
  self->tail_id = id;
  self->ids[id - 1] = BSE_PART_INVAL_TICK_FLAG + 0;
}

static guint	/* returns tick (<= BSE_PART_MAX_TICK) if id is valid */
bse_part_tick_from_id (BsePart *self,
		       guint    id)
{
  return id > 0 && id <= self->n_ids ? self->ids[id - 1] : BSE_PART_INVAL_TICK_FLAG;
}

static gboolean
last_tick_update_handler (gpointer data)
{
  while (plist_last_tick_changed)
    {
      GSList *slist = plist_last_tick_changed;
      BsePart *self = slist->data;
      plist_last_tick_changed = slist->next;
      g_slist_free_1 (slist);
      self->ltu_queued = FALSE;
      if (self->ltu_recalc)
	{
	  guint i, last_tick = 0;
	  self->ltu_recalc = FALSE;
	  for (i = 0; i < self->n_nodes; i++)
	    {
	      BsePartEvent *ev;
	      guint duration = 1;
	      for (ev = self->nodes[i].events; ev; ev = ev->any.next)
		if (ev && ev->type == BSE_PART_EVENT_NOTE)
		  duration = MAX (duration, ev->note.duration);
	      if (self->nodes[i].events)
		last_tick = MAX (last_tick, self->nodes[i].tick + duration);
	    }
	  BSE_SEQUENCER_LOCK ();
	  self->last_tick_SL = last_tick;
	  BSE_SEQUENCER_UNLOCK ();
	}
      g_object_notify (self, "last-tick");
    }
  handler_id_last_tick_changed = 0;
  return FALSE;
}

static void
queue_ltu (BsePart *self,
	   gboolean needs_recalc)
{
  self->ltu_recalc |= needs_recalc != FALSE;
  if (!self->ltu_queued)
    {
      self->ltu_queued = TRUE;
      plist_last_tick_changed = g_slist_prepend (plist_last_tick_changed, self);
    }
  if (!handler_id_last_tick_changed)
    handler_id_last_tick_changed = bse_idle_now (last_tick_update_handler, NULL);
}

static gboolean
range_changed_notify_handler (gpointer data)
{
  while (plist_range_changed)
    {
      GSList *slist = plist_range_changed;
      BsePart *self = slist->data;
      guint tick = self->range_tick, duration = self->range_bound - tick;
      gint min_note = self->range_min_note, max_note = self->range_max_note;
      plist_range_changed = slist->next;
      g_slist_free_1 (slist);
      
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
      if (self->range_tick >= self->range_bound)
	plist_range_changed = g_slist_prepend (plist_range_changed, self);
      self->range_tick = MIN (self->range_tick, tick);
      self->range_bound = MAX (self->range_bound, bound);
      self->range_min_note = MIN (self->range_min_note, note);
      self->range_max_note = MAX (self->range_max_note, note);
      if (!handler_id_range_changed)
	handler_id_range_changed = bse_idle_update (range_changed_notify_handler, NULL);
    }
}

static guint
lookup_tick (BsePart *self,
	     guint    tick)
{
  BsePartNode *nodes = self->nodes;
  guint n = self->n_nodes, offs = 0, i = 0;
  
  while (offs < n)
    {
      gint cmp;
      
      i = (offs + n) >> 1;
      
      cmp = tick > nodes[i].tick ? +1 : tick < nodes[i].tick ? -1 : 0;
      if (!cmp)
	return i;
      else if (cmp < 0)
	n = i;
      else /* (cmp > 0) */
	offs = i + 1;
    }
  
  /* for self->n_nodes==0 we return 0, otherwise we return a
   * valid index, which is either an exact match, or one off
   * into either direction
   */
  return i;	/* last mismatch */
}

static void
queue_rectangle_update (BsePart *self,
			guint    tick,
			guint    duration,
			gint     min_note,
			gint     max_note)
{
  guint end_tick = tick + MAX (duration, 1);
  
  /* widen area to right if notes span across right boundary */
  if (self->n_nodes)
    {
      guint bound = end_tick;
      guint index = lookup_tick (self, tick);   /* nextmost */
      if (self->nodes[index].tick < tick)
	index++;        /* adjust index to be >= tick */
      /* search forward against boundary */
      for (; index < self->n_nodes && self->nodes[index].tick < bound; index++)
	{
	  BsePartEvent *ev;
	  for (ev = self->nodes[index].events; ev; ev = ev->any.next)
	    if (ev->type == BSE_PART_EVENT_NOTE && ev->note.note >= min_note && ev->note.note <= max_note)
	      end_tick = MAX (end_tick, self->nodes[index].tick + ev->note.duration);
	}
    }
  queue_update (self, tick, end_tick - tick, min_note);
  queue_update (self, tick, end_tick - tick, max_note);
}

static void
insert_tick_SL (BsePart *self,
		guint    index,
		guint    tick)
{
  guint n, size;
  
  g_return_if_fail (index <= self->n_nodes);
  if (index > 0)
    g_return_if_fail (self->nodes[index - 1].tick < tick);
  if (index < self->n_nodes)
    g_return_if_fail (self->nodes[index].tick > tick);
  
  n = self->n_nodes++;
  size = upper_power2 (self->n_nodes);
  if (size > upper_power2 (n))
    self->nodes = g_renew (BsePartNode, self->nodes, size);
  g_memmove (self->nodes + index + 1, self->nodes + index, (n - index) * sizeof (self->nodes[0]));
  self->nodes[index].tick = tick;
  self->nodes[index].events = NULL;
}

static guint	/* new index */
ensure_tick_SL (BsePart *self,
		guint    tick)
{
  if (!self->n_nodes)
    {
      insert_tick_SL (self, 0, tick);
      return 0;
    }
  else
    {
      guint index = lookup_tick (self, tick);
      
      if (self->nodes[index].tick < tick)
	insert_tick_SL (self, ++index, tick);
      else if (self->nodes[index].tick > tick)
	insert_tick_SL (self, index, tick);
      return index;
    }
}

static void
insert_event_SL (BsePart      *self,
		 guint         index,
		 BsePartEvent *ev)
{
  g_return_if_fail (index < self->n_nodes);
  g_return_if_fail (ev->any.next == NULL);
  
  ev->any.next = self->nodes[index].events;
  self->nodes[index].events = ev;
}

static BsePartEvent*
find_event (BsePart *self,
	    guint    id,
	    guint   *tick_p)
{
  guint etick = bse_part_tick_from_id (self, id);
  guint index = lookup_tick (self, etick);   /* nextmost */
  if (index < self->n_nodes && self->nodes[index].tick == etick)
    {
      BsePartEvent *ev;
      for (ev = self->nodes[index].events; ev; ev = ev->any.next)
	if (ev->any.id == id)
	  {
	    if (tick_p)
	      *tick_p = etick;
	    return ev;
	  }
    }
  return NULL;
}

static BsePartEvent*
find_note_at (BsePart *self,
	      guint    tick,
	      gint     note,
	      guint   *index_p)
{
  if (self->n_nodes && tick >= self->nodes[0].tick)
    {
      guint index = lookup_tick (self, tick);	/* nextmost */
      
      if (self->nodes[index].tick <= tick)
	index++;	/* adjust index to one after tick */
      /* search backward until note is found */
      while (index-- > 0)
	{
	  BsePartEvent *ev;
	  
	  for (ev = self->nodes[index].events; ev; ev = ev->any.next)
	    if (ev->type == BSE_PART_EVENT_NOTE && ev->note.note == note)
	      {
		/* ok, found note, now check whether it spans across tick */
		if (self->nodes[index].tick + ev->note.duration - 1 >= tick)
		  {
		    if (index_p)
		      *index_p = index;
		    return ev;
		  }
		else /* earlier notes won't span until here */
		  return NULL;
	      }
	}
    }
  return NULL;
}

static BsePartEvent*
find_note_within (BsePart *self,
		  guint    tick,
		  guint    bound,
		  gint     note,
		  guint   *index_p)
{
  if (self->n_nodes && bound > self->nodes[0].tick)
    {
      guint index = lookup_tick (self, tick);	/* nextmost */
      
      if (self->nodes[index].tick < tick)
	index++;	/* adjust nextmost to >= tick */
      
      for (; index < self->n_nodes && self->nodes[index].tick < bound; index++)
	{
	  BsePartEvent *ev;
	  
	  for (ev = self->nodes[index].events; ev; ev = ev->any.next)
	    if (ev->type == BSE_PART_EVENT_NOTE && ev->note.note == note)
	      {
		if (index_p)
		  *index_p = index;
		return ev;
	      }
	}
    }
  return NULL;
}

void
bse_part_select_rectangle (BsePart *self,
			   guint    tick,
			   guint    duration,
			   gint     min_note,
			   gint     max_note)
{
  guint bound;
  
  g_return_if_fail (BSE_IS_PART (self));
  
  min_note = BSE_NOTE_CLAMP (min_note);
  max_note = BSE_NOTE_CLAMP (max_note);
  bound = MIN (tick, BSE_PART_MAX_TICK - 1) + MIN (duration, BSE_PART_MAX_TICK);
  if (self->n_nodes)
    {
      guint index = lookup_tick (self, tick);   /* nextmost */
      if (self->nodes[index].tick < tick)
	index++;                                /* adjust nextmost to >= tick */
      for (; index < self->n_nodes && self->nodes[index].tick < bound; index++)
	{
	  guint etick = self->nodes[index].tick;
	  BsePartEvent *ev;
	  
	  for (ev = self->nodes[index].events; ev; ev = ev->any.next)
	    if (ev->type == BSE_PART_EVENT_NOTE && !ev->note.selected &&
		ev->note.note >= min_note && ev->note.note <= max_note)
	      {
		ev->note.selected = TRUE;
		queue_update (self, etick, ev->note.duration, ev->note.note);
	      }
	}
    }
}

void
bse_part_deselect_rectangle (BsePart *self,
			     guint    tick,
			     guint    duration,
			     gint     min_note,
			     gint     max_note)
{
  guint bound;
  
  g_return_if_fail (BSE_IS_PART (self));
  
  min_note = BSE_NOTE_CLAMP (min_note);
  max_note = BSE_NOTE_CLAMP (max_note);
  bound = MIN (tick, BSE_PART_MAX_TICK - 1) + MIN (duration, BSE_PART_MAX_TICK);
  if (self->n_nodes)
    {
      guint index = lookup_tick (self, tick);   /* nextmost */
      if (self->nodes[index].tick < tick)
	index++;                                /* adjust nextmost to >= tick */
      for (; index < self->n_nodes && self->nodes[index].tick < bound; index++)
	{
	  guint etick = self->nodes[index].tick;
	  BsePartEvent *ev;
	  
	  for (ev = self->nodes[index].events; ev; ev = ev->any.next)
	    if (ev->type == BSE_PART_EVENT_NOTE && ev->note.selected &&
		ev->note.note >= min_note && ev->note.note <= max_note)
	      {
		ev->note.selected = FALSE;
		queue_update (self, etick, ev->note.duration, ev->note.note);
	      }
	}
    }
}

void
bse_part_select_rectangle_ex (BsePart *self,
			      guint    tick,
			      guint    duration,
			      gint     min_note,
			      gint     max_note)
{
  guint bound, index;
  
  g_return_if_fail (BSE_IS_PART (self));
  
  min_note = BSE_NOTE_CLAMP (min_note);
  max_note = BSE_NOTE_CLAMP (max_note);
  bound = MIN (tick, BSE_PART_MAX_TICK - 1) + MIN (duration, BSE_PART_MAX_TICK);
  if (self->n_nodes)
    {
      BsePartEvent *ev;
      for (index = 0; index < self->n_nodes && self->nodes[index].tick < tick; index++)
	{
	  guint etick = self->nodes[index].tick;
	  for (ev = self->nodes[index].events; ev; ev = ev->any.next)
	    if (ev->type == BSE_PART_EVENT_NOTE && ev->note.selected)
	      {
		ev->note.selected = FALSE;
		queue_update (self, etick, ev->note.duration, ev->note.note);
	      }
	}
      for (; index < self->n_nodes && self->nodes[index].tick < bound; index++)
	{
	  guint etick = self->nodes[index].tick;
	  for (ev = self->nodes[index].events; ev; ev = ev->any.next)
	    if (ev->type == BSE_PART_EVENT_NOTE)
	      {
		if (!ev->note.selected && ev->note.note >= min_note && ev->note.note <= max_note)
		  {
		    ev->note.selected = TRUE;
		    queue_update (self, etick, ev->note.duration, ev->note.note);
		  }
		else if (ev->note.selected && (ev->note.note < min_note || ev->note.note > max_note))
		  {
		    ev->note.selected = FALSE;
		    queue_update (self, etick, ev->note.duration, ev->note.note);
		  }
	      }
	}
      for (; index < self->n_nodes; index++)
	{
	  guint etick = self->nodes[index].tick;
	  for (ev = self->nodes[index].events; ev; ev = ev->any.next)
	    if (ev->type == BSE_PART_EVENT_NOTE && ev->note.selected)
	      {
		ev->note.selected = FALSE;
		queue_update (self, etick, ev->note.duration, ev->note.note);
	      }
	}
    }
}

gboolean
bse_part_select_event (BsePart *self,
		       guint    id)
{
  BsePartEvent *ev;
  guint etick;
  
  g_return_val_if_fail (BSE_IS_PART (self), FALSE);
  
  ev = find_event (self, id, &etick);
  if (ev && !ev->any.selected)
    {
      ev->any.selected = TRUE;
      if (ev->type == BSE_PART_EVENT_NOTE)
	queue_update (self, etick, ev->note.duration, ev->note.note);
    }
  return ev != NULL;
}

gboolean
bse_part_deselect_event (BsePart *self,
			 guint    id)
{
  BsePartEvent *ev;
  guint etick;
  
  g_return_val_if_fail (BSE_IS_PART (self), FALSE);
  
  ev = find_event (self, id, &etick);
  if (ev && ev->any.selected)
    {
      ev->any.selected = FALSE;
      if (ev->type == BSE_PART_EVENT_NOTE)
	queue_update (self, etick, ev->note.duration, ev->note.note);
    }
  return ev != NULL;
}

static guint
insert_note (BsePart *self,
	     guint    id,
	     guint    tick,
	     guint    duration,
	     gint     note,
	     gint     fine_tune,
	     gfloat   velocity,
	     gboolean selected)
{
  BsePartEvent *ev;
  guint index, last_tick;
  gboolean last_tick_changed;

  ev = sfi_new_struct0 (BsePartEvent, 1);
  ev->type = BSE_PART_EVENT_NOTE;
  ev->note.id = id;
  ev->note.selected = selected;
  ev->note.duration = duration;
  ev->note.note = note;
  ev->note.fine_tune = fine_tune;
  ev->note.velocity = velocity;
  
  last_tick = tick + ev->note.duration;
  last_tick_changed = last_tick > self->last_tick_SL;
  
  BSE_SEQUENCER_LOCK ();
  index = ensure_tick_SL (self, tick);
  insert_event_SL (self, index, ev);
  if (last_tick_changed)
    self->last_tick_SL = last_tick;
  BSE_SEQUENCER_UNLOCK ();

  if (last_tick_changed)
    queue_ltu (self, FALSE);
  queue_update (self, tick, duration, note);
  
  return ev->note.id;
}

static gboolean
delete_event (BsePart *self,
	      guint    id)
{
  BsePartEvent *last = NULL, *ev = NULL;
  guint tick, index;
  
  tick = bse_part_tick_from_id (self, id);
  g_return_val_if_fail (tick <= BSE_PART_MAX_TICK, FALSE);
  
  index = lookup_tick (self, tick);	/* nextmost */
  if (index < self->n_nodes && self->nodes[index].tick == tick)
    for (ev = self->nodes[index].events; ev; last = ev, ev = last->any.next)
      if (ev->any.id == id)
	break;
  if (ev)
    {
      gboolean selected = ev->any.selected;	// FIXME: hack
      guint need_ltu, last_tick = tick;

      if (ev->type == BSE_PART_EVENT_NOTE)
	{
	  last_tick += ev->note.duration;
	  queue_update (self, self->nodes[index].tick, ev->note.duration, ev->note.note);
	}
      else
	queue_update (self, self->nodes[index].tick, 1, 0 /* FIXME: non-note update */);

      BSE_SEQUENCER_LOCK ();
      if (last)
	last->any.next = ev->any.next;
      else
	self->nodes[index].events = ev->any.next;
      need_ltu = last_tick >= self->last_tick_SL;
      BSE_SEQUENCER_UNLOCK ();

      if (need_ltu)
	queue_ltu (self, TRUE);
      
      sfi_delete_struct (BsePartEvent, ev);
      /* caller does: bse_part_free_id (self, id); */
      
      return selected;
    }
  else
    {
      g_warning ("%s: event (id=%u) not found at tick=%u",
		 G_STRLOC, id, tick);
      return FALSE;
    }
}

guint
bse_part_insert_note (BsePart *self,
		      guint    tick,
		      guint    duration,
		      gint     note,
		      gint     fine_tune,
		      gfloat   velocity)
{
  g_return_val_if_fail (BSE_IS_PART (self), BSE_ERROR_INTERNAL);
  
  if (!(BSE_NOTE_IS_VALID (note) &&
	BSE_FINE_TUNE_IS_VALID (fine_tune) &&
	tick < BSE_PART_MAX_TICK &&
	duration > 0 &&
	duration < BSE_PART_MAX_TICK &&
	tick + duration <= BSE_PART_MAX_TICK))
    return 0;
  
  return insert_note (self,
		      bse_part_alloc_id (self, tick),
		      tick, duration, note, fine_tune, velocity,
		      FALSE);
}

gboolean
bse_part_delete_event (BsePart *self,
		       guint    id)
{
  guint tick;
  
  g_return_val_if_fail (BSE_IS_PART (self), FALSE);
  
  tick = bse_part_tick_from_id (self, id);
  if (tick <= BSE_PART_MAX_TICK)
    {
      delete_event (self, id);
      bse_part_free_id (self, id);
      return TRUE;
    }
  return FALSE;
}

gboolean
bse_part_change_note (BsePart *self,
		      guint    id,
		      guint    tick,
		      guint    duration,
		      gint     note,
		      gint     fine_tune,
		      gfloat   velocity)
{
  guint old_tick;
  
  g_return_val_if_fail (BSE_IS_PART (self), FALSE);
  
  if (!(BSE_NOTE_IS_VALID (note) &&
	BSE_FINE_TUNE_IS_VALID (fine_tune) &&
	tick < BSE_PART_MAX_TICK &&
	duration > 0 &&
	duration < BSE_PART_MAX_TICK &&
	tick + duration <= BSE_PART_MAX_TICK))
    return FALSE;
  
  old_tick = bse_part_tick_from_id (self, id);
  if (old_tick <= BSE_PART_MAX_TICK)
    {
      gboolean selected = delete_event (self, id);
      insert_note (self,
		   bse_part_move_id (self, id, tick),
		   tick, duration, note, fine_tune, velocity, selected);
      return TRUE;
    }
  else
    return FALSE;
}

gboolean
bse_part_is_selected_event (BsePart *self,
			    guint    id)
{
  guint tick;
  
  g_return_val_if_fail (BSE_IS_PART (self), FALSE);
  
  tick = bse_part_tick_from_id (self, id);
  if (tick <= BSE_PART_MAX_TICK)
    {
      guint index = lookup_tick (self, tick);     /* nextmost */
      BsePartEvent *ev;
      if (index < self->n_nodes && self->nodes[index].tick == tick)
	for (ev = self->nodes[index].events; ev; ev = ev->any.next)
	  if (ev->any.id == id)
	    return ev->any.selected;
    }
  return FALSE;
}

BsePartNoteSeq*
bse_part_list_notes_around (BsePart *self,
			    guint    tick,
			    guint    duration,
			    gint     min_note,
			    gint     max_note)
{
  guint bound, index;
  BsePartNoteSeq *pseq;
  
  g_return_val_if_fail (BSE_IS_PART (self), NULL);
  g_return_val_if_fail (tick < BSE_PART_MAX_TICK, NULL);
  g_return_val_if_fail (duration > 0 && duration <= BSE_PART_MAX_TICK, NULL);
  
  bound = tick + duration;
  min_note = BSE_NOTE_CLAMP (min_note);
  max_note = BSE_NOTE_CLAMP (max_note);
  pseq = bse_part_note_seq_new ();
  
  /* find notes crossing span. any early note may span across tick,
   * so we always need to start searching at the top ;(
   */
  for (index = 0; index < self->n_nodes && self->nodes[index].tick < bound; index++)
    {
      guint etick = self->nodes[index].tick;
      BsePartEvent *ev;
      
      for (ev = self->nodes[index].events; ev; ev = ev->any.next)
	if (ev->type == BSE_PART_EVENT_NOTE &&
	    ev->note.note >= min_note && ev->note.note <= max_note)
	  {
	    if (etick + ev->note.duration > tick)
	      bse_part_note_seq_take_append (pseq,
					     bse_part_note (ev->note.id,
							    etick, ev->note.duration,
							    ev->note.note,
							    ev->note.fine_tune,
							    ev->note.velocity,
							    ev->note.selected));
	  }
    }
  
  return pseq;
}

void
bse_part_queue_notes_within (BsePart *self,
			     guint    tick,
			     guint    duration,
			     gint     min_note,
			     gint     max_note)
{
  g_return_if_fail (BSE_IS_PART (self));
  g_return_if_fail (tick < BSE_PART_MAX_TICK);
  g_return_if_fail (duration > 0 && duration <= BSE_PART_MAX_TICK);
  
  queue_rectangle_update (self, tick, duration,
			  BSE_NOTE_CLAMP (min_note),
			  BSE_NOTE_CLAMP (max_note));
}

BsePartNoteSeq*
bse_part_list_selected_notes (BsePart *self)
{
  BsePartNoteSeq *pseq;
  guint index;
  
  g_return_val_if_fail (BSE_IS_PART (self), NULL);
  
  pseq = bse_part_note_seq_new ();
  if (self->n_nodes)
    for (index = 0; index < self->n_nodes; index++)
      {
	guint etick = self->nodes[index].tick;
	BsePartEvent *ev;
	for (ev = self->nodes[index].events; ev; ev = ev->any.next)
	  if (ev->type == BSE_PART_EVENT_NOTE && ev->note.selected)
	    bse_part_note_seq_take_append (pseq,
					   bse_part_note (ev->note.id,
							  etick, ev->note.duration,
							  ev->note.note,
							  ev->note.fine_tune,
							  ev->note.velocity,
							  ev->note.selected));
      }
  return pseq;
}

BsePartNoteSeq*
bse_part_list_notes_at (BsePart *self,
			guint    tick,
			gint     note)
{
  BsePartNoteSeq *pseq;
  BsePartEvent *ev;
  guint index;
  
  g_return_val_if_fail (BSE_IS_PART (self), NULL);
  
  note = BSE_NOTE_CLAMP (note);
  pseq = bse_part_note_seq_new ();
  ev = find_note_at (self, tick, note, &index);
  if (ev)
    bse_part_note_seq_take_append (pseq,	// FIXME: return _all_ notes
				   bse_part_note (ev->note.id,
						  self->nodes[index].tick,
						  ev->note.duration,
						  ev->note.note,
						  ev->note.fine_tune,
						  ev->note.velocity,
						  ev->note.selected));
  return pseq;
}

guint
bse_part_node_lookup_SL (BsePart *self,
			 guint    tick)
{
  guint index;
  
  g_return_val_if_fail (BSE_IS_PART (self), 0);
  
  /* we return the index of the first node wich is >= tick
   * or index == self->n_nodes
   */
  
  index = lookup_tick (self, tick);
  
  if (self->n_nodes && self->nodes[index].tick < tick)
    index++;
  
  return index;
}

static void
bse_part_store_private (BseObject  *object,
			BseStorage *storage)
{
  BsePart *self = BSE_PART (object);
  guint index;
  
  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->store_private)
    BSE_OBJECT_CLASS (parent_class)->store_private (object, storage);
  
  for (index = 0; index < self->n_nodes; index++)
    {
      BsePartEvent *ev;
      
      for (ev = self->nodes[index].events; ev; ev = ev->any.next)
	{
	  if (ev->type == BSE_PART_EVENT_NOTE)
	    {
	      bse_storage_break (storage);
	      if (ev->note.fine_tune != 0 || ev->note.velocity < 1.0)
		{
		  bse_storage_printf (storage, "(insert-note %u %u %d %d",
				      self->nodes[index].tick, ev->note.duration,
				      ev->note.note, ev->note.fine_tune);
		  if (ev->note.velocity < 1.0)
		    {
		      bse_storage_putc (storage, ' ');
		      bse_storage_putf (storage, ev->note.velocity);
		    }
		  bse_storage_putc (storage, ')');
		}
	      else
		bse_storage_printf (storage, "(insert-note %u %u %d)",
				    self->nodes[index].tick, ev->note.duration,
				    ev->note.note);
	    }
	}
    }
}

static BseTokenType
bse_part_restore_private (BseObject  *object,
			  BseStorage *storage)
{
  BsePart *self = BSE_PART (object);
  GScanner *scanner = storage->scanner;

  if (g_scanner_peek_next_token (scanner) == G_TOKEN_IDENTIFIER &&
      bse_string_equals ("insert-note", scanner->next_value.v_identifier))
    {
      guint id, tick, duration, note, fine_tune = 0;
      gfloat velocity = 1.0;
      
      parse_or_return (scanner, G_TOKEN_IDENTIFIER);	/* eat identifier */
      parse_or_return (scanner, G_TOKEN_INT);
      tick = scanner->value.v_int;
      parse_or_return (scanner, G_TOKEN_INT);
      duration = scanner->value.v_int;
      parse_or_return (scanner, G_TOKEN_INT);
      note = scanner->value.v_int;
      if (g_scanner_peek_next_token (scanner) == G_TOKEN_INT)
	{
	  g_scanner_get_next_token (scanner);		/* eat int */
	  fine_tune = scanner->value.v_int;
	  if (g_scanner_peek_next_token (scanner) == G_TOKEN_FLOAT)
	    {
	      g_scanner_get_next_token (scanner);	/* eat float */
	      velocity = scanner->value.v_float;
	    }
	}
      parse_or_return (scanner, ')');
      
      id = bse_part_insert_note (self, tick, duration, note, fine_tune, velocity);
      if (!id)
	bse_storage_warn (storage, "note insertion (note=%d tick=%u duration=%u) failed",
			  note, tick, duration);
      return G_TOKEN_NONE;
    }
  else /* chain parent class' handler */
    return BSE_OBJECT_CLASS (parent_class)->restore_private (object, storage);
}
