/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include	"bsepattern.h"
#include	"bseglobals.h"
#include	"bseinstrument.h"
#include	"bsesong.h"
#include	"bsemain.h"
#include	"bseprocedure.h"
#include	"bsestorage.h"
#include	<string.h>


#define	INDX(n_channels, channel, row) ((row) * (n_channels) + (channel))
#define	PNOTE(pattern, channel, row)	( \
    ((BsePattern*) (pattern))->notes[ \
      INDX (((BsePattern*) (pattern))->n_channels, (channel), (row)) \
])

enum {
  PARAM_0
};

enum {
  SIGNAL_SIZE_CHANGED,
  SIGNAL_NOTE_CHANGED,
  SIGNAL_NOTE_SELECTION_CHANGED,
  SIGNAL_LAST
};


/* --- prototypes --- */
static void	    bse_pattern_class_init	 (BsePatternClass	*class);
static void	    bse_pattern_init		 (BsePattern		*pattern);
static void	    bse_pattern_do_destroy	 (BseObject		*object);
static void	    bse_pattern_do_set_parent    (BseItem		*item,
						  BseItem		*parent);
static void	    bse_pattern_reset_note	 (BsePattern		*pattern,
						  guint			 channel,
						  guint			 row);
static void	    bse_pattern_store_private	 (BseObject		*object,
						  BseStorage		*storage);
static GTokenType   bse_pattern_restore		 (BseObject		*object,
						  BseStorage		*storage);
static BseTokenType bse_pattern_restore_private	 (BseObject		*object,
						  BseStorage		*storage);


/* --- variables --- */
static GTypeClass	*parent_class = NULL;
static guint             pattern_signals[SIGNAL_LAST] = { 0, };


/* --- functions --- */
BSE_BUILTIN_TYPE (BsePattern)
{
  static const GTypeInfo pattern_info = {
    sizeof (BsePatternClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_pattern_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BsePattern),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_pattern_init,
  };
  
  return bse_type_register_static (BSE_TYPE_ITEM,
				   "BsePattern",
				   "BSE pattern type",
				   &pattern_info);
}

static void
bse_pattern_class_init (BsePatternClass	*class)
{
  BseObjectClass *object_class;
  BseItemClass *item_class;
  
  parent_class = g_type_class_peek_parent (class);
  object_class = BSE_OBJECT_CLASS (class);
  item_class = BSE_ITEM_CLASS (class);
  
  object_class->restore = bse_pattern_restore;
  object_class->store_private = bse_pattern_store_private;
  object_class->restore_private = bse_pattern_restore_private;
  object_class->destroy = bse_pattern_do_destroy;
  
  item_class->set_parent = bse_pattern_do_set_parent;
  
  pattern_signals[SIGNAL_SIZE_CHANGED] = bse_object_class_add_signal (object_class, "size-changed",
								      G_TYPE_NONE, 0);
  pattern_signals[SIGNAL_NOTE_CHANGED] = bse_object_class_add_signal (object_class, "note-changed",
								      G_TYPE_NONE,
								      2, G_TYPE_UINT, G_TYPE_UINT);
  pattern_signals[SIGNAL_NOTE_SELECTION_CHANGED] = bse_object_class_add_signal (object_class, "note-selection-changed",
										G_TYPE_NONE,
										2, G_TYPE_UINT, G_TYPE_UINT);
}

static void
bse_pattern_init (BsePattern *pattern)
{
  pattern->n_channels = 0;
  pattern->n_rows = 0;
  pattern->current_channel = 0;
  pattern->current_row = 0;
  pattern->notes = NULL;
}

static void
bse_pattern_do_destroy (BseObject *object)
{
  BsePattern *pattern;
  guint c, r;
  
  pattern = BSE_PATTERN (object);
  
  for (c = 0; c < pattern->n_channels; c++)
    for (r = 0; r < pattern->n_rows; r++)
      bse_pattern_reset_note (pattern, c, r);
  g_free (pattern->notes);
  pattern->n_channels = 0;
  pattern->n_rows = 0;
  pattern->notes = NULL;
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_pattern_do_set_parent (BseItem *item,
			   BseItem *parent)
{
  BsePattern *pattern;
  
  if (parent)
    g_return_if_fail (BSE_IS_SONG (parent));
  
  pattern = BSE_PATTERN (item);
  
  /* chain parent class' set_parent handler */
  BSE_ITEM_CLASS (parent_class)->set_parent (item, parent);
  
  if (parent)
    {
      BseSong *song = BSE_SONG (parent);
      
      bse_pattern_set_n_channels (pattern, BSE_SONG_N_CHANNELS (song));
      bse_pattern_set_n_rows (pattern, BSE_SONG_PATTERN_LENGTH (song));
    }
}

static void
bse_pattern_reset_note (BsePattern *pattern,
			guint       channel,
			guint       row)
{
  BsePatternNote *note;
  guint i;
  
  note = &PNOTE (pattern, channel, row);
  
  note->note = BSE_NOTE_VOID;
  if (note->instrument)
    {
      g_object_unref (note->instrument);
      note->instrument = NULL;
    }
  
  for (i = 0; i < note->n_effects; i++)
    g_object_unref (G_OBJECT (note->effects[i]));
  g_free (note->effects);
  note->effects = NULL;
  note->n_effects = 0;
  
  /* leave note->selected state alone */
}

void
bse_pattern_set_n_channels (BsePattern *pattern,
			    guint	n_channels)
{
  g_return_if_fail (BSE_IS_PATTERN (pattern));
  g_return_if_fail (n_channels >= 1 && n_channels <= 256);
  
  if (BSE_OBJECT_IS_LOCKED (pattern))
    return;
  
  if (pattern->n_channels != n_channels)
    {
      BsePatternNote *notes;
      guint c, r;
      
      for (c = n_channels; c < pattern->n_channels; c++)
	for (r = 0; r < pattern->n_rows; r++)
	  bse_pattern_reset_note (pattern, c, r);
      
      notes = g_new (BsePatternNote, n_channels * pattern->n_rows);
      
      for (c = 0; c < n_channels; c++)
	for (r = 0; r < pattern->n_rows; r++)
	  {
	    if (c < pattern->n_channels)
	      notes[INDX (n_channels, c, r)] = PNOTE (pattern, c, r);
	    else
	      {
		notes[INDX (n_channels, c, r)].note = BSE_NOTE_VOID;
		notes[INDX (n_channels, c, r)].instrument = NULL;
		notes[INDX (n_channels, c, r)].n_effects = 0;
		notes[INDX (n_channels, c, r)].selected = FALSE;
		notes[INDX (n_channels, c, r)].effects = NULL;
	      }
	  }
      
      g_free (pattern->notes);
      pattern->notes = notes;
      pattern->n_channels = n_channels;
      
      g_signal_emit (pattern, pattern_signals[SIGNAL_SIZE_CHANGED], 0);
    }
}

void
bse_pattern_set_n_rows (BsePattern *pattern,
			guint	    n_rows)
{
  g_return_if_fail (BSE_IS_PATTERN (pattern));
  g_return_if_fail (n_rows >= 4 && n_rows <= 256);
  
  if (BSE_OBJECT_IS_LOCKED (pattern))
    return;
  
  if (pattern->n_rows != n_rows)
    {
      BsePatternNote *notes;
      guint c, r;
      
      for (r = n_rows; r < pattern->n_rows; r++)
	for (c = 0; c < pattern->n_channels; c++)
	  bse_pattern_reset_note (pattern, c, r);
      
      notes = g_new (BsePatternNote, pattern->n_channels * n_rows);
      
      for (c = 0; c < pattern->n_channels; c++)
	for (r = 0; r < n_rows; r++)
	  {
	    if (r < pattern->n_rows)
	      notes[INDX (pattern->n_channels, c, r)] = PNOTE (pattern, c, r);
	    else
	      {
		notes[INDX (pattern->n_channels, c, r)].note = BSE_NOTE_VOID;
		notes[INDX (pattern->n_channels, c, r)].instrument = NULL;
		notes[INDX (pattern->n_channels, c, r)].n_effects = 0;
		notes[INDX (pattern->n_channels, c, r)].selected = FALSE;
		notes[INDX (pattern->n_channels, c, r)].effects = NULL;
	      }
	  }
      
      g_free (pattern->notes);
      pattern->notes = notes;
      pattern->n_rows = n_rows;
      
      g_signal_emit (pattern, pattern_signals[SIGNAL_SIZE_CHANGED], 0);
    }
}

guint
bse_pattern_note_get_n_effects (BsePattern *pattern,
				guint       channel,
				guint       row)
{
  BsePatternNote *note;
  
  g_return_val_if_fail (BSE_IS_PATTERN (pattern), 0);
  g_return_val_if_fail (channel < pattern->n_channels, 0);
  g_return_val_if_fail (row < pattern->n_rows, 0);
  
  note = &PNOTE (pattern, channel, row);
  
  return note->n_effects;
}

BseEffect*
bse_pattern_note_get_effect (BsePattern *pattern,
			     guint       channel,
			     guint       row,
			     guint       index)
{
  BsePatternNote *note;
  
  g_return_val_if_fail (BSE_IS_PATTERN (pattern), NULL);
  g_return_val_if_fail (channel < pattern->n_channels, NULL);
  g_return_val_if_fail (row < pattern->n_rows, NULL);
  
  note = &PNOTE (pattern, channel, row);
  if (index < note->n_effects)
    return note->effects[index];
  
  return NULL;
}

BseEffect*
bse_pattern_note_find_effect (BsePattern *pattern,
			      guint       channel,
			      guint       row,
			      GType       effect_type)
{
  BseEffect *effect = NULL;
  BsePatternNote *note;
  guint i;
  
  g_return_val_if_fail (BSE_IS_PATTERN (pattern), NULL);
  g_return_val_if_fail (channel < pattern->n_channels, NULL);
  g_return_val_if_fail (row < pattern->n_rows, NULL);
  g_return_val_if_fail (BSE_TYPE_IS_EFFECT (effect_type), NULL);
  
  note = &PNOTE (pattern, channel, row);
  for (i = 0; i < note->n_effects; i++)
    if (g_type_is_a (BSE_OBJECT_TYPE (note->effects[i]), effect_type))
      {
	effect = note->effects[i];
	break;
      }
  
  return effect;
}

void
bse_pattern_note_actuate_effect (BsePattern *pattern,
				 guint       channel,
				 guint       row,
				 GType	     effect_type)
{
  g_return_if_fail (BSE_IS_PATTERN (pattern));
  g_return_if_fail (channel < pattern->n_channels);
  g_return_if_fail (row < pattern->n_rows);
  g_return_if_fail (BSE_TYPE_IS_EFFECT (effect_type));
  
  if (!bse_pattern_note_find_effect (pattern, channel, row, g_type_next_base (effect_type, BSE_TYPE_EFFECT)))
    {
      BsePatternNote *note;
      gpointer effect;
      guint i;
      
      effect = g_object_new (effect_type, NULL);
      
      BSE_SEQUENCER_LOCK ();
      
      note = &PNOTE (pattern, channel, row);
      i = note->n_effects++;
      
      note->effects = g_renew (BseEffect*, note->effects, note->n_effects);
      note->effects[i] = effect;
      
      BSE_SEQUENCER_UNLOCK ();
      
      g_signal_emit (pattern, pattern_signals[SIGNAL_NOTE_CHANGED], 0, channel, row);
    }
}


void
bse_pattern_note_drop_effect (BsePattern *pattern,
			      guint       channel,
			      guint       row,
			      GType       effect_type)
{
  BsePatternNote *note;
  guint i;
  
  g_return_if_fail (BSE_IS_PATTERN (pattern));
  g_return_if_fail (channel < pattern->n_channels);
  g_return_if_fail (row < pattern->n_rows);
  g_return_if_fail (BSE_TYPE_IS_EFFECT (effect_type));
  
  note = &PNOTE (pattern, channel, row);
  for (i = 0; i < note->n_effects; i++)
    if (g_type_is_a (BSE_OBJECT_TYPE (note->effects[i]), effect_type))
      {
	BseEffect *effect = note->effects[i];
	
	BSE_SEQUENCER_LOCK ();
	
	note->n_effects--;
	g_memmove (note->effects + i, note->effects + i + 1, sizeof (note->effects[0]) * (note->n_effects - i));
	
	BSE_SEQUENCER_UNLOCK ();
	
	g_object_unref (effect);
	
	g_signal_emit (pattern, pattern_signals[SIGNAL_NOTE_CHANGED], 0, channel, row);
	return;
      }
}

void
bse_pattern_modify_note (BsePattern    *pattern,
			 guint          channel,
			 guint          row,
			 gint           note_val,
			 BseInstrument *instrument)
{
  BsePatternNote *note;
  guint notify_changed = 0;
  
  g_return_if_fail (BSE_IS_PATTERN (pattern));
  g_return_if_fail (channel < pattern->n_channels);
  g_return_if_fail (row < pattern->n_rows);
  if (note_val != BSE_NOTE_VOID)
    g_return_if_fail (note_val >= BSE_MIN_NOTE && note_val <= BSE_MAX_NOTE);
  
  g_object_ref (pattern);
  
  note = &PNOTE (pattern, channel, row);
  if (note->instrument != instrument)
    {
      BseInstrument *oi = note->instrument;
      
      if (instrument)
	g_object_ref (instrument);
      
      BSE_SEQUENCER_LOCK ();
      note->instrument = instrument;
      note->note = note_val;
      BSE_SEQUENCER_UNLOCK ();
      
      if (oi)
	g_object_unref (oi);
      notify_changed++;
    }
  else if (note->note != note_val)
    {
      BSE_SEQUENCER_LOCK ();
      note->note = note_val;
      notify_changed++;
      BSE_SEQUENCER_UNLOCK ();
    }
  
  if (notify_changed)
    g_signal_emit (pattern, pattern_signals[SIGNAL_NOTE_CHANGED], 0, channel, row);
  
  g_object_unref (pattern);
}

void
bse_pattern_set_note (BsePattern *pattern,
		      guint       channel,
		      guint       row,
		      gint        note)
{
  BsePatternNote *pnote;
  
  g_return_if_fail (BSE_IS_PATTERN (pattern));
  g_return_if_fail (channel < pattern->n_channels);
  g_return_if_fail (row < pattern->n_rows);
  if (note != BSE_NOTE_VOID)
    g_return_if_fail (note >= BSE_MIN_NOTE && note <= BSE_MAX_NOTE);
  
  pnote = bse_pattern_peek_note (pattern, channel, row);
  bse_pattern_modify_note (pattern, channel, row, note, pnote->instrument);
}

void
bse_pattern_set_instrument (BsePattern    *pattern,
			    guint          channel,
			    guint          row,
			    BseInstrument *instrument)
{
  BsePatternNote *pnote;
  
  g_return_if_fail (BSE_IS_PATTERN (pattern));
  g_return_if_fail (channel < pattern->n_channels);
  g_return_if_fail (row < pattern->n_rows);
  if (instrument)
    g_return_if_fail (BSE_IS_INSTRUMENT (instrument));
  
  pnote = bse_pattern_peek_note (pattern, channel, row);
  bse_pattern_modify_note (pattern, channel, row, pnote->note, instrument);
}

void
bse_pattern_select_note (BsePattern *pattern,
			 guint       channel,
			 guint       row)
{
  BsePatternNote *note;
  
  g_return_if_fail (BSE_IS_PATTERN (pattern));
  g_return_if_fail (channel < pattern->n_channels);
  g_return_if_fail (row < pattern->n_rows);
  
  note = &PNOTE (pattern, channel, row);
  if (!note->selected)
    {
      BSE_SEQUENCER_LOCK ();
      note->selected = TRUE;
      BSE_SEQUENCER_UNLOCK ();
      g_signal_emit (pattern, pattern_signals[SIGNAL_NOTE_SELECTION_CHANGED], 0, channel, row);
    }
}

void
bse_pattern_unselect_note (BsePattern *pattern,
			   guint       channel,
			   guint       row)
{
  BsePatternNote *note;
  
  g_return_if_fail (BSE_IS_PATTERN (pattern));
  g_return_if_fail (channel < pattern->n_channels);
  g_return_if_fail (row < pattern->n_rows);
  
  note = &PNOTE (pattern, channel, row);
  if (note->selected)
    {
      BSE_SEQUENCER_LOCK ();
      note->selected = FALSE;
      BSE_SEQUENCER_UNLOCK ();
      g_signal_emit (pattern, pattern_signals[SIGNAL_NOTE_SELECTION_CHANGED], 0, channel, row);
    }
}

guint32*
bse_pattern_selection_new (guint n_channels,
			   guint n_rows)
{
  guint32 *selection;
  
  g_return_val_if_fail (n_channels >= 1 && n_channels <= 256, NULL);
  g_return_val_if_fail (n_rows >= 1 && n_rows <= 256, NULL);
  
  selection = g_new0 (guint32, (n_channels * n_rows + 31) / 32 + 2);
  BSE_PATTERN_SELECTION_N_CHANNELS (selection) = n_channels;
  BSE_PATTERN_SELECTION_N_ROWS (selection) = n_rows;
  
  return selection;
}

guint32*
bse_pattern_selection_copy (guint32 *src_selection)
{
  guint32 *selection;
  guint n_channels, n_rows, s;
  
  g_return_val_if_fail (src_selection != NULL, NULL);
  
  n_channels = BSE_PATTERN_SELECTION_N_CHANNELS (src_selection);
  n_rows = BSE_PATTERN_SELECTION_N_ROWS (src_selection);
  s = (n_channels * n_rows + 31) / 32 + 2;
  selection = g_new (guint32, s);
  memcpy (selection, src_selection, sizeof (guint32) * s);
  
  return selection;
}

void
bse_pattern_selection_free (guint32 *selection)
{
  g_return_if_fail (selection != NULL);
  
  g_free (selection);
}

void
bse_pattern_selection_fill (guint32 *selection,
			    gboolean selected)
{
  guint n;
  
  g_return_if_fail (selection != NULL);
  
  n = BSE_PATTERN_SELECTION_N_CHANNELS (selection) * BSE_PATTERN_SELECTION_N_ROWS (selection);
  memset (selection + 2, selected ? 0xff : 0, (n / 32) * sizeof (guint32));
}

void
bse_pattern_save_selection (BsePattern *pattern,
			    guint32    *selection)
{
  guint c, r;
  
  g_return_if_fail (BSE_IS_PATTERN (pattern));
  g_return_if_fail (selection != NULL);
  g_return_if_fail (BSE_PATTERN_SELECTION_N_CHANNELS (selection) == pattern->n_channels);
  g_return_if_fail (BSE_PATTERN_SELECTION_N_ROWS (selection) == pattern->n_rows);
  
  for (c = 0; c < pattern->n_channels; c++)
    for (r = 0; r < pattern->n_rows; r++)
      {
	BsePatternNote *note = &PNOTE (pattern, c, r);
	
	if (note->selected)
	  BSE_PATTERN_SELECTION_MARK (selection, c, r);
	else
	  BSE_PATTERN_SELECTION_UNMARK (selection, c, r);
      }
}

void
bse_pattern_restore_selection (BsePattern *pattern,
			       guint32    *selection)
{
  guint c, r;
  
  g_return_if_fail (BSE_IS_PATTERN (pattern));
  g_return_if_fail (selection != NULL);
  g_return_if_fail (BSE_PATTERN_SELECTION_N_CHANNELS (selection) == pattern->n_channels);
  g_return_if_fail (BSE_PATTERN_SELECTION_N_ROWS (selection) == pattern->n_rows);
  
  for (c = 0; c < pattern->n_channels; c++)
    for (r = 0; r < pattern->n_rows; r++)
      {
	BsePatternNote *note = &PNOTE (pattern, c, r);
	gboolean selected = BSE_PATTERN_SELECTION_TEST (selection, c, r);
	
	if (note->selected && !selected)
	  bse_pattern_unselect_note (pattern, c, r);
	else if (!note->selected && selected)
	  bse_pattern_select_note (pattern, c, r);
      }
}

GList* /* free list */
bse_pattern_list_selection (BsePattern *pattern)
{
  GList *list = NULL;
  guint r, c;
  
  g_return_val_if_fail (BSE_IS_PATTERN (pattern), NULL);
  
  for (c = 0; c < pattern->n_channels; c++)
    for (r = 0; r < pattern->n_rows; r++)
      {
	BsePatternNote *note = &PNOTE (pattern, c, r);
	
	if (note->selected)
	  list = g_list_prepend (list, note);
      }
  
  return g_list_reverse (list);
}

gboolean
bse_pattern_has_selection (BsePattern *pattern)
{
  guint c, r;
  
  g_return_val_if_fail (BSE_IS_PATTERN (pattern), FALSE);
  
  for (c = 0; c < pattern->n_channels; c++)
    for (r = 0; r < pattern->n_rows; r++)
      {
	BsePatternNote *note = &PNOTE (pattern, c, r);
	
	if (note->selected)
	  return TRUE;
      }
  return FALSE;
}

BsePatternNote*
bse_pattern_peek_note (BsePattern *pattern,
		       guint       channel,
		       guint       row)
{
  g_return_val_if_fail (BSE_IS_PATTERN (pattern), NULL);
  g_return_val_if_fail (channel < pattern->n_channels, NULL);
  g_return_val_if_fail (row < pattern->n_rows, NULL);
  
  return &PNOTE (pattern, channel, row);
}

static BsePatternNote*
get_note (BsePattern *pattern,
	  guint	      channel,
	  guint	      row)
{
  BsePatternNote *note;
  
  while (row >= pattern->n_rows)
    {
      channel++;
      row -= pattern->n_rows;
    }
  if (channel < pattern->n_channels)
    {
      note = &PNOTE (pattern, channel, row);
      
      if (note->note == BSE_NOTE_VOID &&
	  !note->instrument &&
	  !note->effects)
	note = NULL;
    }
  else
    note = NULL;
  
  return note;
}

static void
save_note (BseStorage     *storage,
	   BsePatternNote *note)
{
  bse_storage_break (storage);
  bse_storage_putc (storage, '(');
  
  if (!note)
    {
      bse_storage_puts (storage, "skip)");
      return;
    }
  
  bse_storage_puts (storage, "note");
  bse_storage_putc (storage, ' ');
  
  if (note->note == BSE_NOTE_VOID)
    bse_storage_puts (storage, "void");
  else
    {
      gchar *name;
      
      name = bse_note_to_string (note->note);
      bse_storage_puts (storage, name);
      g_free (name);
    }
  
  if (note->instrument)
    bse_storage_printf (storage, " %u",
			bse_item_get_seqid (BSE_ITEM (note->instrument)));
  
  if (note->n_effects)
    {
      guint i;
      
      bse_storage_push_level (storage);
      
      for (i = 0; i < note->n_effects; i++)
	{
	  bse_storage_break (storage);
	  bse_storage_putc (storage, '(');
	  
	  bse_storage_puts (storage, BSE_OBJECT_TYPE_NAME (note->effects[i]));
	  
	  bse_storage_push_level (storage);
	  bse_object_store (BSE_OBJECT (note->effects[i]), storage);
	  bse_storage_pop_level (storage);
	}
      bse_storage_pop_level (storage);
    }
  
  bse_storage_putc (storage, ')');
}

static void
bse_pattern_store_private (BseObject  *object,
			   BseStorage *storage)
{
  BsePattern *pattern;
  guint c, r, n_c, n_r;
  gboolean force_next;
  
  pattern = BSE_PATTERN (object);
  
  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->store_private)
    BSE_OBJECT_CLASS (parent_class)->store_private (object, storage);
  
  n_c = n_r = 0;
  force_next = get_note (pattern, 0, 0) || get_note (pattern, 0, 1);
  for (c = 0; c < pattern->n_channels; c++)
    {
      for (r = 0; r < pattern->n_rows; r++)
	{
	  BsePatternNote *note, *note2, *note3;
	  
	  note = get_note (pattern, c, r);
	  note2 = get_note (pattern, c, r + 1);
	  note3 = get_note (pattern, c, r + 2);
	  
	  if (note || force_next)
	    {
	      gint delta;
	      
	      delta = c;
	      delta -= n_c;
	      if (delta)
		{
		  bse_storage_break (storage);
		  bse_storage_printf (storage, "(move-channel %+d)", delta);
		}
	      delta = r;
	      delta -= n_r;
	      if (delta)
		{
		  bse_storage_break (storage);
		  bse_storage_printf (storage, "(move-row %+d)", delta);
		}
	      
	      save_note (storage, note);
	      force_next = !note2 && note3;
	      
	      n_c = c;
	      n_r = r + 1;
	    }
	}
    }
}

static BseTokenType
bse_pattern_restore_private (BseObject	*object,
			     BseStorage *storage)
{
  GScanner *scanner = storage->scanner;
  GTokenType expected_token;
  static GQuark quark_skip = 0;
  static GQuark quark_note = 0;
  static GQuark quark_set_channel = 0;
  static GQuark quark_set_row = 0;
  static GQuark quark_move_channel = 0;
  static GQuark quark_move_row = 0;
  GQuark token_quark;
  gint note = BSE_NOTE_VOID;
  BseInstrument *instrument = NULL;
  BsePattern *pattern;
  gboolean parse_effects = FALSE;
  
  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->restore_private)
    expected_token = BSE_OBJECT_CLASS (parent_class)->restore_private (object, storage);
  else
    expected_token = BSE_TOKEN_UNMATCHED;
  
  if (expected_token != BSE_TOKEN_UNMATCHED ||
      g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return expected_token;
  
  pattern = BSE_PATTERN (object);
  
  if (!quark_skip)
    {
      quark_skip = g_quark_from_static_string ("skip");
      quark_note = g_quark_from_static_string ("note");
      quark_set_channel = g_quark_from_static_string ("set-channel");
      quark_set_row = g_quark_from_static_string ("set-row");
      quark_move_channel = g_quark_from_static_string ("move-channel");
      quark_move_row = g_quark_from_static_string ("move-row");
    }
  
  token_quark = g_quark_try_string (scanner->next_value.v_identifier);
  
  if (token_quark == quark_set_channel)
    {
      g_scanner_get_next_token (scanner);
      if (g_scanner_get_next_token (scanner) != G_TOKEN_INT)
	return G_TOKEN_INT;
      if (scanner->value.v_int < 1 || scanner->value.v_int > pattern->n_channels)
	return bse_storage_warn_skip (storage, "channel index exceeds pattern bounds");
      
      pattern->current_channel = scanner->value.v_int - 1;
    }
  else if (token_quark == quark_set_row)
    {
      g_scanner_get_next_token (scanner);
      if (g_scanner_get_next_token (scanner) != G_TOKEN_INT)
	return G_TOKEN_INT;
      if (scanner->value.v_int < 1 || scanner->value.v_int > pattern->n_rows)
	return bse_storage_warn_skip (storage, "row index exceeds pattern bounds");
      
      pattern->current_row = scanner->value.v_int - 1;
    }
  else if (token_quark == quark_move_channel)
    {
      gboolean negate = FALSE;
      gint pos;
      
      g_scanner_get_next_token (scanner);
      
      g_scanner_get_next_token (scanner);
      if (scanner->token == '+' || scanner->token == '-')
	{
	  negate = scanner->token == '-';
	  g_scanner_get_next_token (scanner);
	}
      if (scanner->token != G_TOKEN_INT)
	return G_TOKEN_INT;
      pos = (negate
	     ? pattern->current_channel - scanner->value.v_int
	     : pattern->current_channel + scanner->value.v_int);
      if (pos < 0 || pos > pattern->n_channels - 1)
	return bse_storage_warn_skip (storage, "channel index exceeds pattern bounds");
      
      pattern->current_channel = pos;
    }
  else if (token_quark == quark_move_row)
    {
      gboolean negate = FALSE;
      gint pos;
      
      g_scanner_get_next_token (scanner);
      
      g_scanner_get_next_token (scanner);
      if (scanner->token == '+' || scanner->token == '-')
	{
	  negate = scanner->token == '-';
	  g_scanner_get_next_token (scanner);
	}
      if (scanner->token != G_TOKEN_INT)
	return G_TOKEN_INT;
      pos = (negate
	     ? pattern->current_row - scanner->value.v_int
	     : pattern->current_row + scanner->value.v_int);
      if (pos < 0 || pos > pattern->n_rows - 1)
	return bse_storage_warn_skip (storage, "row index exceeds pattern bounds");
      
      pattern->current_row = pos;
    }
  else if (token_quark == quark_skip)
    g_scanner_get_next_token (scanner);
  else if (token_quark == quark_note)
    {
      gchar buffer[2], *string = buffer;
      
      g_scanner_get_next_token (scanner);
      if (g_scanner_get_next_token (scanner) == G_TOKEN_IDENTIFIER)
	{
	  string = scanner->value.v_identifier;
	  if (string[0] == '\'')
	    string++;
	}
      else if ((scanner->token >= 'A' && scanner->token <= 'Z') ||
	       (scanner->token >= 'a' && scanner->token <= 'z'))
	{
	  buffer[0] = scanner->token;
	  buffer[1] = 0;
	}
      else
	return G_TOKEN_IDENTIFIER;
      
      note = bse_note_from_string (string);
      if (note == BSE_NOTE_UNPARSABLE)
	return bse_storage_warn_skip (storage, "invalid note `%s'", string);
      
      if (g_scanner_peek_next_token (scanner) == G_TOKEN_INT)
	{
	  BseSuper *super = bse_item_get_super (BSE_ITEM (pattern));
	  
	  g_scanner_get_next_token (scanner);
	  
	  if (scanner->value.v_int && BSE_IS_SONG (super))
	    instrument = bse_song_get_instrument (BSE_SONG (super),
						  scanner->value.v_int);
	  if (!instrument)
	    return bse_storage_warn_skip (storage,
					  "invalid instrument id `%lu'",
					  scanner->value.v_int);
	}
      
      parse_effects = TRUE;
    }
  else
    return BSE_TOKEN_UNMATCHED;
  
  if (g_scanner_peek_next_token (scanner) != ')' && (!parse_effects || g_scanner_peek_next_token (scanner) != '('))
    {
      g_scanner_get_next_token (scanner);
      
      return ')';
    }
  
  if (token_quark == quark_note || token_quark == quark_skip)
    {
      if (pattern->current_channel < pattern->n_channels &&
	  pattern->current_row < pattern->n_rows)
	{
	  bse_pattern_set_note (pattern,
				pattern->current_channel,
				pattern->current_row,
				note);
	  bse_pattern_set_instrument (pattern,
				      pattern->current_channel,
				      pattern->current_row,
				      instrument);
	}
      
      if (parse_effects)
	while (g_scanner_peek_next_token (scanner) == '(')
	  {
	    BseEffect *effect;
	    GType effect_type;
	    
	    g_scanner_get_next_token (scanner);
	    if (g_scanner_get_next_token (scanner) != G_TOKEN_IDENTIFIER)
	      return G_TOKEN_IDENTIFIER;
	    
	    effect_type = g_type_from_name (scanner->value.v_identifier);
	    
	    if (!BSE_TYPE_IS_EFFECT (effect_type))
	      {
		expected_token = bse_storage_warn_skip (storage,
							"invalid effect type `%s'",
							scanner->value.v_identifier);
		if (expected_token != G_TOKEN_NONE)
		  return expected_token;
	      }
	    
	    effect = bse_pattern_note_find_effect (pattern,
						   pattern->current_channel,
						   pattern->current_row,
						   effect_type);
	    if (!effect)
	      {
		bse_pattern_note_actuate_effect (pattern,
						 pattern->current_channel,
						 pattern->current_row,
						 effect_type);
		effect = bse_pattern_note_find_effect (pattern,
						       pattern->current_channel,
						       pattern->current_row,
						       effect_type);
	      }
	    expected_token = bse_object_restore (BSE_OBJECT (effect), storage);
	    if (expected_token != G_TOKEN_NONE)
	      return expected_token;
	  }
      
      pattern->current_row++;
    }
  
  return g_scanner_get_next_token (scanner) == ')' ? G_TOKEN_NONE : ')';
}

static GTokenType
bse_pattern_restore (BseObject	*object,
		     BseStorage *storage)
{
  BsePattern *pattern = BSE_PATTERN (object);
  GTokenType expected_token;
  guint current_channel, current_row;
  
  /* asure clean values during parsing phase */
  current_channel = pattern->current_channel;
  current_row = pattern->current_row;
  pattern->current_channel = 0;
  pattern->current_row = 0;
  
  /* chain parent class' handler */
  expected_token = BSE_OBJECT_CLASS (parent_class)->restore (object, storage);
  
  /* clean up, parsing is done */
  pattern->current_channel = current_channel;
  pattern->current_row = current_row;
  
  return expected_token;
}
