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
static void	    bse_part_init		(BsePart	*part);
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
bse_part_init (BsePart *part)
{
  part->n_nodes = 0;
  part->nodes = g_renew (BsePartNode, NULL, upper_power2 (part->n_nodes));
  part->ppqn = 384;
  part->range_tick = BSE_PART_MAX_TICK;
  part->range_bound = 0;
  part->range_min_freq = BSE_MAX_FREQUENCY;
  part->range_max_freq = 0;
}

static void
bse_part_destroy (BseObject *object)
{
  BsePart *part = BSE_PART (object);

  range_changed_parts = g_slist_remove (range_changed_parts, part);
  part->range_tick = 0;
  part->range_bound = BSE_PART_MAX_TICK;
  part->range_min_freq = 0;
  part->range_max_freq = BSE_MAX_FREQUENCY;
  
  /* chain parent class' handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_part_finalize (GObject *object)
{
  BsePart *part = BSE_PART (object);
  BsePartEvent *ev, *next;
  guint i;

  for (i = 0; i < part->n_nodes; i++)
    for (ev = part->nodes[i].events; ev; ev = next)
      {
	next = ev->any.next;
	gsl_delete_struct (BsePartEvent, ev);
      }
  g_free (part->nodes);
  part->nodes = NULL;

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
      BsePart *part = slist->data;
      guint tick = part->range_tick, duration = part->range_bound - tick;
      gfloat min_freq = part->range_min_freq, max_freq = part->range_max_freq;

      range_changed_parts = slist->next;
      g_slist_free_1 (slist);

      part->range_tick = BSE_PART_MAX_TICK;
      part->range_bound = 0;
      part->range_min_freq = BSE_MAX_FREQUENCY;
      part->range_max_freq = 0;
      g_signal_emit (part, signal_range_changed, 0, tick, duration, min_freq, max_freq);
    }
  range_changed_handler = 0;

  BSE_THREADS_LEAVE ();

  return FALSE;
}

static void
queue_update (BsePart *part,
	      guint    tick,
	      guint    duration,
	      gfloat   freq)
{
  guint bound = tick + duration;

  g_return_if_fail (duration > 0);

  if (part->range_tick >= part->range_bound)
    range_changed_parts = g_slist_prepend (range_changed_parts, part);
  part->range_tick = MIN (part->range_tick, tick);
  part->range_bound = MAX (part->range_bound, bound);
  part->range_min_freq = MIN (part->range_min_freq, freq);
  part->range_max_freq = MAX (part->range_max_freq, freq);
  if (!range_changed_handler)
    range_changed_handler = g_idle_add_full (BSE_NOTIFY_PRIORITY, range_changed_notify_handler, NULL, NULL);
}

static guint
lookup_tick (BsePart *part,
	     guint    tick)
{
  BsePartNode *nodes = part->nodes;
  guint n = part->n_nodes, offs = 0, i = 0;

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

  /* for part->n_nodes==0 we return 0, otherwise we return a
   * valid index, which is either an exact match, or one off
   * into either direction
   */
  return i;	/* last mismatch */
}

static void
insert_tick (BsePart *part,
	     guint    index,
	     guint    tick)
{
  guint n, size;

  g_return_if_fail (index <= part->n_nodes);
  if (index > 0)
    g_return_if_fail (part->nodes[index - 1].tick < tick);
  if (index < part->n_nodes)
    g_return_if_fail (part->nodes[index].tick > tick);

  n = part->n_nodes++;
  size = upper_power2 (part->n_nodes);
  if (size > upper_power2 (n))
    part->nodes = g_renew (BsePartNode, part->nodes, size);
  g_memmove (part->nodes + index + 1, part->nodes + index, (n - index) * sizeof (part->nodes[0]));
  part->nodes[index].tick = tick;
  part->nodes[index].events = NULL;
}

static guint	/* new index */
ensure_tick (BsePart *part,
	     guint    tick)
{
  if (!part->n_nodes)
    {
      insert_tick (part, 0, tick);
      return 0;
    }
  else
    {
      guint index = lookup_tick (part, tick);

      if (part->nodes[index].tick < tick)
	insert_tick (part, ++index, tick);
      else if (part->nodes[index].tick > tick)
	insert_tick (part, index, tick);
      return index;
    }
}

static void
remove_tick (BsePart *part,
	     guint    index)
{
  guint n;

  g_return_if_fail (index < part->n_nodes);
  g_return_if_fail (part->nodes[index].events == NULL);

  n = part->n_nodes--;
  g_memmove (part->nodes + index, part->nodes + index + 1, (part->n_nodes - index) * sizeof (part->nodes[0]));
}

static void
insert_event (BsePart      *part,
	      guint         index,
	      BsePartEvent *ev)
{
  g_return_if_fail (index < part->n_nodes);
  g_return_if_fail (ev->any.next == NULL);

  ev->any.next = part->nodes[index].events;
  part->nodes[index].events = ev;
}

static BsePartEvent*
find_note_at (BsePart *part,
	      guint    tick,
	      guint    ifreq,
	      guint   *index_p)
{
  if (part->n_nodes && tick >= part->nodes[0].tick)
    {
      guint index = lookup_tick (part, tick);	/* nextmost */

      if (part->nodes[index].tick <= tick)
	index++;	/* adjust index to one after tick */
      /* search backward until ifreq is found */
      while (index-- > 0)
	{
	  BsePartEvent *ev;

	  for (ev = part->nodes[index].events; ev; ev = ev->any.next)
	    if (ev->type == BSE_PART_EVENT_NOTE && ev->note.ifreq == ifreq)
	      {
		/* ok, found ifreq, now check whether it spans across tick */
		if (part->nodes[index].tick + ev->note.duration - 1 >= tick)
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
find_note_within (BsePart *part,
		  guint    tick,
		  guint    bound,
		  guint    ifreq,
		  guint   *index_p)
{
  if (part->n_nodes && bound > part->nodes[0].tick)
    {
      guint index = lookup_tick (part, tick);	/* nextmost */
      
      if (part->nodes[index].tick < tick)
	index++;	/* adjust nextmost to >= tick */
      
      for (; index < part->n_nodes && part->nodes[index].tick < bound; index++)
	{
	  BsePartEvent *ev;
	  
	  for (ev = part->nodes[index].events; ev; ev = ev->any.next)
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

BseErrorType
bse_part_insert_note (BsePart *part,
		      guint    tick,
		      guint    duration,
		      gfloat   freq,
		      gfloat   velocity)
{
  BsePartEvent *ev;
  guint64 tick_end;
  guint index, ifreq;

  g_return_val_if_fail (BSE_IS_PART (part), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (freq <= BSE_MAX_FREQUENCY, BSE_ERROR_INTERNAL);

  if (tick >= BSE_PART_MAX_TICK)
    return BSE_ERROR_INVALID_OFFSET;
  tick_end = tick;
  tick_end += duration;
  if (duration < 1 || tick_end > BSE_PART_MAX_TICK)
    return BSE_ERROR_INVALID_DURATION;

  ifreq = BSE_PART_IFREQ (freq);
  ev = find_note_at (part, tick, ifreq, &index);
  if (!ev)
    ev = find_note_within (part, tick, tick + duration, ifreq, &index);
  if (ev)
    {
      g_message ("not inserting note %f from %u to %u which overlaps with note from %u to %u",
		 BSE_PART_FREQ (ifreq), tick, tick + duration - 1,
		 part->nodes[index].tick, part->nodes[index].tick + ev->note.duration - 1);
      return BSE_ERROR_INVALID_OVERLAP;
    }

  ev = gsl_new_struct0 (BsePartEvent, 1);
  ev->type = BSE_PART_EVENT_NOTE;
  ev->note.ifreq = ifreq;
  ev->note.duration = duration;
  ev->note.velocity = velocity;

  BSE_SEQUENCER_LOCK ();
  index = ensure_tick (part, tick);
  insert_event (part, index, ev);
  BSE_SEQUENCER_UNLOCK ();

  queue_update (part, tick, duration, BSE_PART_FREQ (ifreq));

  return BSE_ERROR_NONE;
}

void
bse_part_delete_note (BsePart *part,
		      guint    tick,
		      gfloat   freq)
{
  BsePartEvent *last = NULL, *ev = NULL;
  guint index, ifreq;

  g_return_if_fail (BSE_IS_PART (part));

  ifreq = BSE_PART_IFREQ (freq);
  index = lookup_tick (part, tick);	/* nextmost */
  if (index < part->n_nodes && part->nodes[index].tick == tick)
    for (ev = part->nodes[index].events; ev; last = ev, ev = last->any.next)
      if (ev->type == BSE_PART_EVENT_NOTE && ev->note.ifreq == ifreq)
	break;
  if (ev)
    {
      queue_update (part, part->nodes[index].tick, ev->note.duration, BSE_PART_FREQ (ev->note.ifreq));

      BSE_SEQUENCER_LOCK ();
      if (last)
	last->any.next = ev->any.next;
      else
	part->nodes[index].events = ev->any.next;
      BSE_SEQUENCER_UNLOCK ();
      
      gsl_delete_struct (BsePartEvent, ev);
    }
  else
    g_warning ("note %f to delete does not start at %u",
	       BSE_PART_FREQ (ifreq), tick);
}

BswIterPartNote*
bse_part_list_notes (BsePart *part,
		     guint    tick,
		     guint    duration,
		     gfloat   min_freq,
		     gfloat   max_freq)
{
  guint bound, min_ifreq, max_ifreq, index;
  BswIterPartNote *iter;

  g_return_val_if_fail (BSE_IS_PART (part), NULL);
  g_return_val_if_fail (duration > 0, NULL);
  bound = tick + duration;
  g_return_val_if_fail (bound > tick, NULL);	/* stupid wrap-around check */

  min_ifreq = BSE_PART_IFREQ (min_freq);
  max_ifreq = BSE_PART_IFREQ (max_freq);
  iter = bsw_iter_create (BSW_TYPE_ITER_PART_NOTE, 16);

  /* find notes crossing span. any early note may span across tick,
   * so we always need to start searching at the top ;(
   */
  for (index = 0; index < part->n_nodes && part->nodes[index].tick < bound; index++)
    {
      guint etick = part->nodes[index].tick;
      BsePartEvent *ev;

      for (ev = part->nodes[index].events; ev; ev = ev->any.next)
	if (ev->type == BSE_PART_EVENT_NOTE &&
	    ev->note.ifreq >= min_ifreq && ev->note.ifreq <= max_ifreq)
	  {
	    if (etick + ev->note.duration > tick)
	      bsw_iter_add_part_note_take_ownership (iter,
						     bsw_part_note (etick,
								    ev->note.duration,
								    BSE_PART_FREQ (ev->note.ifreq),
								    ev->note.velocity));
	  }
    }

  return iter;
}

BswIter*
bse_part_get_note_at (BsePart *part,
		      guint    tick,
		      gfloat   freq)
{
  BswIterPartNote *iter;
  BsePartEvent *ev;
  guint index;

  g_return_val_if_fail (BSE_IS_PART (part), NULL);

  iter = bsw_iter_create (BSW_TYPE_ITER_PART_NOTE, 1);
  ev = find_note_at (part, tick, BSE_PART_IFREQ (freq), &index);
  if (ev)
    bsw_iter_add_part_note_take_ownership (iter,
					   bsw_part_note (part->nodes[index].tick,
							  ev->note.duration,
							  BSE_PART_FREQ (ev->note.ifreq),
							  ev->note.velocity));
  return iter;
}

guint
bse_part_node_lookup_SL (BsePart *part,
			 guint    tick)
{
  guint index;

  g_return_val_if_fail (BSE_IS_PART (part), 0);

  /* we return the index of the first node wich is >= tick
   * or index == part->n_nodes
   */

  index = lookup_tick (part, tick);

  if (part->n_nodes && part->nodes[index].tick < tick)
    index++;

  return index;
}

static void
bse_part_store_private (BseObject  *object,
			BseStorage *storage)
{
  BsePart *part = BSE_PART (object);
  guint index;

  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->store_private)
    BSE_OBJECT_CLASS (parent_class)->store_private (object, storage);

  for (index = 0; index < part->n_nodes; index++)
    {
      BsePartEvent *ev;

      for (ev = part->nodes[index].events; ev; ev = ev->any.next)
	{
	  if (ev->type == BSE_PART_EVENT_NOTE)
	    {
	      bse_storage_break (storage);
	      if (ev->note.velocity < 1.0)
		bse_storage_printf (storage, "(insert-note %u %u %.6f %g)",
				    part->nodes[index].tick, ev->note.duration,
				    BSE_PART_FREQ (ev->note.ifreq), ev->note.velocity);
	      else
		bse_storage_printf (storage, "(insert-note %u %u %.6f)",
				    part->nodes[index].tick, ev->note.duration,
				    BSE_PART_FREQ (ev->note.ifreq));
	    }
	}
    }
}

static BseTokenType
bse_part_restore_private (BseObject  *object,
			  BseStorage *storage)
{
  BsePart *part = BSE_PART (object);
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

      error = bse_part_insert_note (part, tick, duration, freq, velocity);
      if (error)
	bse_storage_warn (storage, "note insertion (freq: %f at: %u duration: %u) failed: %s",
			  freq, tick, duration, bse_error_blurb (error));

      return G_TOKEN_NONE;
    }
  else
    return BSE_TOKEN_UNMATCHED;
}
