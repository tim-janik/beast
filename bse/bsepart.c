/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2002 Tim Janik
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
#include "bsemarshal.h"
#include "gslcommon.h"
#include "bswprivate.h"
#include <string.h>

/* --- macros --- */
#define	upper_power2(uint_n)	gsl_alloc_upper_power2 (MAX ((uint_n), 4))
#define parse_or_return         bse_storage_scanner_parse_or_return
#define peek_or_return          bse_storage_scanner_peek_or_return


/* --- prototypes --- */
static void	    bse_part_class_init		(BsePartClass	*class);
static void	    bse_part_init		(BsePart	*self);
static void	    bse_part_destroy		(BseObject	*object);
static void	    bse_part_finalize		(GObject	*object);
static void	    bse_part_store_private	(BseObject	*object,
						 BseStorage	*storage);
static BseTokenType bse_part_restore_private	(BseObject	*object,
						 BseStorage	*storage);


/* --- variables --- */
static gpointer parent_class = NULL;
static guint    signal_range_changed = 0;
static guint    range_changed_handler = 0;
static GSList  *range_changed_parts = NULL;
static GQuark   quark_insert_note = 0;


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

  quark_insert_note = g_quark_from_static_string ("insert-note");

  gobject_class->finalize = bse_part_finalize;

  object_class->store_private = bse_part_store_private;
  object_class->restore_private = bse_part_restore_private;
  object_class->destroy = bse_part_destroy;
  
  signal_range_changed = bse_object_class_add_signal (object_class, "range-changed",
						      bse_marshal_VOID__UINT_UINT_FLOAT_FLOAT, NULL,
						      G_TYPE_NONE, 4,
						      G_TYPE_UINT, G_TYPE_UINT,
						      G_TYPE_FLOAT, G_TYPE_FLOAT);
}

static void
bse_part_init (BsePart *self)
{
  self->n_nodes = 0;
  self->nodes = g_renew (BsePartNode, NULL, upper_power2 (self->n_nodes));
  self->ppqn = 384;
  self->range_tick = BSE_PART_MAX_TICK;
  self->range_bound = 0;
  self->range_min_freq = BSE_MAX_FREQUENCY;
  self->range_max_freq = 0;
}

static void
bse_part_destroy (BseObject *object)
{
  BsePart *self = BSE_PART (object);

  range_changed_parts = g_slist_remove (range_changed_parts, self);
  self->range_tick = 0;
  self->range_bound = BSE_PART_MAX_TICK;
  self->range_min_freq = 0;
  self->range_max_freq = BSE_MAX_FREQUENCY;
  
  /* chain parent class' handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_part_finalize (GObject *object)
{
  BsePart *self = BSE_PART (object);
  BsePartEvent *ev, *next;
  guint i;

  for (i = 0; i < self->n_nodes; i++)
    for (ev = self->nodes[i].events; ev; ev = next)
      {
	next = ev->any.next;
	gsl_delete_struct (BsePartEvent, ev);
      }
  g_free (self->nodes);
  self->nodes = NULL;

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
range_changed_notify_handler (gpointer data)
{
  BSE_THREADS_ENTER ();

  while (range_changed_parts)
    {
      GSList *slist = range_changed_parts;
      BsePart *self = slist->data;
      guint tick = self->range_tick, duration = self->range_bound - tick;
      gfloat min_freq = self->range_min_freq, max_freq = self->range_max_freq;

      range_changed_parts = slist->next;
      g_slist_free_1 (slist);

      self->range_tick = BSE_PART_MAX_TICK;
      self->range_bound = 0;
      self->range_min_freq = BSE_MAX_FREQUENCY;
      self->range_max_freq = 0;
      g_signal_emit (self, signal_range_changed, 0, tick, duration, min_freq, max_freq);
    }
  range_changed_handler = 0;

  BSE_THREADS_LEAVE ();

  return FALSE;
}

static void
queue_update (BsePart *self,
	      guint    tick,
	      guint    duration,
	      gfloat   freq)
{
  guint bound = tick + duration;

  g_return_if_fail (duration > 0);

  if (self->range_tick >= self->range_bound)
    range_changed_parts = g_slist_prepend (range_changed_parts, self);
  self->range_tick = MIN (self->range_tick, tick);
  self->range_bound = MAX (self->range_bound, bound);
  self->range_min_freq = MIN (self->range_min_freq, freq);
  self->range_max_freq = MAX (self->range_max_freq, freq);
  if (!range_changed_handler)
    range_changed_handler = g_idle_add_full (BSE_NOTIFY_PRIORITY, range_changed_notify_handler, NULL, NULL);
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
insert_tick (BsePart *self,
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
ensure_tick (BsePart *self,
	     guint    tick)
{
  if (!self->n_nodes)
    {
      insert_tick (self, 0, tick);
      return 0;
    }
  else
    {
      guint index = lookup_tick (self, tick);

      if (self->nodes[index].tick < tick)
	insert_tick (self, ++index, tick);
      else if (self->nodes[index].tick > tick)
	insert_tick (self, index, tick);
      return index;
    }
}

static void
remove_tick (BsePart *self,
	     guint    index)
{
  guint n;

  g_return_if_fail (index < self->n_nodes);
  g_return_if_fail (self->nodes[index].events == NULL);

  n = self->n_nodes--;
  g_memmove (self->nodes + index, self->nodes + index + 1, (self->n_nodes - index) * sizeof (self->nodes[0]));
}

static void
insert_event (BsePart      *self,
	      guint         index,
	      BsePartEvent *ev)
{
  g_return_if_fail (index < self->n_nodes);
  g_return_if_fail (ev->any.next == NULL);

  ev->any.next = self->nodes[index].events;
  self->nodes[index].events = ev;
}

static BsePartEvent*
find_note_at (BsePart *self,
	      guint    tick,
	      guint    ifreq,
	      guint   *index_p)
{
  if (self->n_nodes && tick >= self->nodes[0].tick)
    {
      guint index = lookup_tick (self, tick);	/* nextmost */

      if (self->nodes[index].tick <= tick)
	index++;	/* adjust index to one after tick */
      /* search backward until ifreq is found */
      while (index-- > 0)
	{
	  BsePartEvent *ev;

	  for (ev = self->nodes[index].events; ev; ev = ev->any.next)
	    if (ev->type == BSE_PART_EVENT_NOTE && ev->note.ifreq == ifreq)
	      {
		/* ok, found ifreq, now check whether it spans across tick */
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
		  guint    ifreq,
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
	    if (ev->type == BSE_PART_EVENT_NOTE && ev->note.ifreq == ifreq)
	      {
		if (index_p)
		  *index_p = index;
		return ev;
	      }
	}
    }
  return NULL;
}

static void
queue_rectangle (BsePart *self,
		 guint    tick,
		 guint    duration,
		 gfloat   min_freq,
		 gfloat   max_freq)
{
  guint end_tick = tick + MAX (duration, 1);

  /* widen area to right if notes span across right boundary */
  if (self->n_nodes)
    {
      guint bound = end_tick;
      guint min_ifreq = BSE_PART_IFREQ (min_freq);
      guint max_ifreq = BSE_PART_IFREQ (max_freq);
      guint index = lookup_tick (self, tick);   /* nextmost */
      if (self->nodes[index].tick < tick)
	index++;        /* adjust index to be >= tick */
      /* search forward against boundary */
      for (; index < self->n_nodes && self->nodes[index].tick < bound; index++)
	{
	  BsePartEvent *ev;
	  for (ev = self->nodes[index].events; ev; ev = ev->any.next)
	    if (ev->type == BSE_PART_EVENT_NOTE && ev->note.ifreq >= min_ifreq && ev->note.ifreq <= max_ifreq)
	      end_tick = MAX (end_tick, self->nodes[index].tick + ev->note.duration);
	}
    }
  queue_update (self, tick, end_tick - tick, min_freq);
  queue_update (self, tick, end_tick - tick, max_freq);
}

void
bse_part_set_selection (BsePart *self,
			guint    tick,
			guint    duration,
			gfloat   min_freq,
			gfloat   max_freq)
{
  guint64 tick_end;

  g_return_if_fail (BSE_IS_PART (self));

  tick = MIN (tick, BSE_PART_MAX_TICK - 1);
  tick_end = tick;
  tick_end += duration;
  tick_end = MIN (tick_end, BSE_PART_MAX_TICK);
  min_freq = MIN (min_freq, BSE_MAX_FREQUENCY);
  max_freq = MIN (max_freq, BSE_MAX_FREQUENCY);
  /* quantize frequencies */
  min_freq = BSE_PART_FREQ (BSE_PART_IFREQ (min_freq));
  max_freq = BSE_PART_FREQ (BSE_PART_IFREQ (max_freq));
  if (tick_end <= tick || min_freq > max_freq)
    {
      /* queue old selection */
      if (self->selection_bound > self->selection_tick)
	queue_rectangle (self, self->selection_tick, self->selection_bound - self->selection_tick,
			 self->selection_min_freq, self->selection_max_freq);
      /* delete selection */
      self->selection_tick = 0;
      self->selection_bound = 0;
      self->selection_min_freq = BSE_MAX_FREQUENCY;
      self->selection_max_freq = 0;
    }
  else if (tick != self->selection_tick ||
	   tick_end != self->selection_bound ||
	   min_freq != self->selection_min_freq ||
	   max_freq != self->selection_max_freq )
    {
      /* queue old selection */
      queue_rectangle (self, self->selection_tick, self->selection_bound - self->selection_tick,
		       self->selection_min_freq, self->selection_max_freq);
      /* update selection */
      self->selection_tick = tick;
      self->selection_bound = tick_end;
      self->selection_min_freq = min_freq;
      self->selection_max_freq = max_freq;
      /* queue new selection */
      queue_rectangle (self, self->selection_tick, self->selection_bound - self->selection_tick,
		       self->selection_min_freq, self->selection_max_freq);
    }
}

BseErrorType
bse_part_insert_note (BsePart *self,
		      guint    tick,
		      guint    duration,
		      gfloat   freq,
		      gfloat   velocity)
{
  BsePartEvent *ev;
  guint64 tick_end;
  guint index, ifreq;

  g_return_val_if_fail (BSE_IS_PART (self), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (freq <= BSE_MAX_FREQUENCY, BSE_ERROR_INTERNAL);

  if (tick >= BSE_PART_MAX_TICK)
    return BSE_ERROR_INVALID_OFFSET;
  tick_end = tick;
  tick_end += duration;
  if (duration < 1 || tick_end > BSE_PART_MAX_TICK)
    return BSE_ERROR_INVALID_DURATION;

  ifreq = BSE_PART_IFREQ (freq);
  ev = find_note_at (self, tick, ifreq, &index);
  if (!ev)
    ev = find_note_within (self, tick, tick + duration, ifreq, &index);
  if (ev)
    {
      g_message ("not inserting note %f from %u to %u which overlaps with note from %u to %u",
		 BSE_PART_FREQ (ifreq), tick, tick + duration - 1,
		 self->nodes[index].tick, self->nodes[index].tick + ev->note.duration - 1);
      return BSE_ERROR_INVALID_OVERLAP;
    }

  ev = gsl_new_struct0 (BsePartEvent, 1);
  ev->type = BSE_PART_EVENT_NOTE;
  ev->note.ifreq = ifreq;
  ev->note.duration = duration;
  ev->note.velocity = velocity;

  BSE_SEQUENCER_LOCK ();
  index = ensure_tick (self, tick);
  insert_event (self, index, ev);
  BSE_SEQUENCER_UNLOCK ();

  queue_update (self, tick, duration, BSE_PART_FREQ (ifreq));

  return BSE_ERROR_NONE;
}

void
bse_part_delete_note (BsePart *self,
		      guint    tick,
		      gfloat   freq)
{
  BsePartEvent *last = NULL, *ev = NULL;
  guint index, ifreq;

  g_return_if_fail (BSE_IS_PART (self));

  ifreq = BSE_PART_IFREQ (freq);
  index = lookup_tick (self, tick);	/* nextmost */
  if (index < self->n_nodes && self->nodes[index].tick == tick)
    for (ev = self->nodes[index].events; ev; last = ev, ev = last->any.next)
      if (ev->type == BSE_PART_EVENT_NOTE && ev->note.ifreq == ifreq)
	break;
  if (ev)
    {
      queue_update (self, self->nodes[index].tick, ev->note.duration, BSE_PART_FREQ (ev->note.ifreq));

      BSE_SEQUENCER_LOCK ();
      if (last)
	last->any.next = ev->any.next;
      else
	self->nodes[index].events = ev->any.next;
      BSE_SEQUENCER_UNLOCK ();
      
      gsl_delete_struct (BsePartEvent, ev);
    }
  else
    g_warning ("note %f to delete does not start at %u",
	       BSE_PART_FREQ (ifreq), tick);
}

static BswPartNote*
make_part_note (BsePart *self,
		guint    tick,
		guint    duration,
		gfloat   freq,
		gfloat   velocity)
{
  gboolean selected = (tick >= self->selection_tick &&
		       tick < self->selection_bound &&
		       freq >= self->selection_min_freq &&
		       freq <= self->selection_max_freq);
  return bsw_part_note (tick, duration, freq, velocity, selected);
}

BswIterPartNote*
bse_part_list_notes (BsePart *self,
		     guint    tick,
		     guint    duration,
		     gfloat   min_freq,
		     gfloat   max_freq)
{
  guint bound, min_ifreq, max_ifreq, index;
  BswIterPartNote *iter;

  g_return_val_if_fail (BSE_IS_PART (self), NULL);
  g_return_val_if_fail (duration > 0, NULL);
  bound = tick + duration;
  g_return_val_if_fail (bound > tick, NULL);	/* stupid wrap-around check */

  min_ifreq = BSE_PART_IFREQ (min_freq);
  max_ifreq = BSE_PART_IFREQ (max_freq);
  iter = bsw_iter_create (BSW_TYPE_ITER_PART_NOTE, 16);

  /* find notes crossing span. any early note may span across tick,
   * so we always need to start searching at the top ;(
   */
  for (index = 0; index < self->n_nodes && self->nodes[index].tick < bound; index++)
    {
      guint etick = self->nodes[index].tick;
      BsePartEvent *ev;

      for (ev = self->nodes[index].events; ev; ev = ev->any.next)
	if (ev->type == BSE_PART_EVENT_NOTE &&
	    ev->note.ifreq >= min_ifreq && ev->note.ifreq <= max_ifreq)
	  {
	    if (etick + ev->note.duration > tick)
	      bsw_iter_add_part_note_take_ownership (iter,
						     make_part_note (self,
								     etick, ev->note.duration,
								     BSE_PART_FREQ (ev->note.ifreq),
								     ev->note.velocity));
	  }
    }

  return iter;
}

BswIterPartNote*
bse_part_list_selected_notes (BsePart *self)
{
  guint bound, min_ifreq, max_ifreq, tick;
  BswIterPartNote *iter;

  g_return_val_if_fail (BSE_IS_PART (self), NULL);

  tick = self->selection_tick;
  bound = self->selection_bound;

  min_ifreq = BSE_PART_IFREQ (self->selection_min_freq);
  max_ifreq = BSE_PART_IFREQ (self->selection_max_freq);
  iter = bsw_iter_create (BSW_TYPE_ITER_PART_NOTE, 16);

  if (self->n_nodes)
    {
      guint index = lookup_tick (self, tick);   /* nextmost */
      if (self->nodes[index].tick < tick)
	index++;        /* adjust index to be >= tick */
      for (; index < self->n_nodes && self->nodes[index].tick < bound; index++)
	{
	  guint etick = self->nodes[index].tick;
	  BsePartEvent *ev;
	  
	  for (ev = self->nodes[index].events; ev; ev = ev->any.next)
	    if (ev->type == BSE_PART_EVENT_NOTE &&
		ev->note.ifreq >= min_ifreq && ev->note.ifreq <= max_ifreq)
	      {
		if (etick + ev->note.duration > tick)
		  bsw_iter_add_part_note_take_ownership (iter,
							 make_part_note (self,
									 etick, ev->note.duration,
									 BSE_PART_FREQ (ev->note.ifreq),
									 ev->note.velocity));
	      }
	}
    }
  return iter;
}

BswIter*
bse_part_get_note_at (BsePart *self,
		      guint    tick,
		      gfloat   freq)
{
  BswIterPartNote *iter;
  BsePartEvent *ev;
  guint index;

  g_return_val_if_fail (BSE_IS_PART (self), NULL);

  iter = bsw_iter_create (BSW_TYPE_ITER_PART_NOTE, 1);
  ev = find_note_at (self, tick, BSE_PART_IFREQ (freq), &index);
  if (ev)
    bsw_iter_add_part_note_take_ownership (iter,
					   make_part_note (self,
							   self->nodes[index].tick,
							   ev->note.duration,
							   BSE_PART_FREQ (ev->note.ifreq),
							   ev->note.velocity));
  return iter;
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
	      if (ev->note.velocity < 1.0)
		bse_storage_printf (storage, "(insert-note %u %u %.6f %g)",
				    self->nodes[index].tick, ev->note.duration,
				    BSE_PART_FREQ (ev->note.ifreq), ev->note.velocity);
	      else
		bse_storage_printf (storage, "(insert-note %u %u %.6f)",
				    self->nodes[index].tick, ev->note.duration,
				    BSE_PART_FREQ (ev->note.ifreq));
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
  GTokenType expected_token;
  GQuark token_quark;

  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->restore_private)
    expected_token = BSE_OBJECT_CLASS (parent_class)->restore_private (object, storage);
  else
    expected_token = BSE_TOKEN_UNMATCHED;

  if (expected_token != BSE_TOKEN_UNMATCHED ||
      g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return expected_token;

  token_quark = g_quark_try_string (scanner->next_value.v_identifier);

  if (token_quark == quark_insert_note)
    {
      guint tick, duration;
      gfloat freq, velocity = 1.0;
      BseErrorType error;

      g_scanner_get_next_token (scanner);	/* eat quark */

      parse_or_return (scanner, G_TOKEN_INT);
      tick = scanner->value.v_int;
      parse_or_return (scanner, G_TOKEN_INT);
      duration = scanner->value.v_int;
      parse_or_return (scanner, G_TOKEN_FLOAT);
      freq = scanner->value.v_float;
      if (g_scanner_peek_next_token (scanner) == G_TOKEN_FLOAT)
	{
	  g_scanner_get_next_token (scanner);	/* eat float */
	  velocity = scanner->value.v_float;
	}
      parse_or_return (scanner, ')');

      error = bse_part_insert_note (self, tick, duration, freq, velocity);
      if (error)
	bse_storage_warn (storage, "note insertion (freq: %f at: %u duration: %u) failed: %s",
			  freq, tick, duration, bse_error_blurb (error));

      return G_TOKEN_NONE;
    }
  else
    return BSE_TOKEN_UNMATCHED;
}
