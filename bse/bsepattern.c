/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997, 1998, 1999 Olaf Hoehmann and Tim Janik
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
#include	"bseprocedure.h"
#include	"bsestorage.h"


#define	INDX(n_channels, channel, row) ((row) * (n_channels) + (channel))
#define	PNOTE(pattern, channel, row)	( \
    ((BsePattern*) (pattern))->notes[ \
      INDX (((BsePattern*) (pattern))->n_channels, (channel), (row)) \
])


enum
{
  PARAM_0
};


/* --- prototypes --- */
static void	    bse_pattern_class_init	 (BsePatternClass	*class);
static void	    bse_pattern_init		 (BsePattern		*pattern);
static void	    bse_pattern_destroy		 (BseObject		*object);
static void	    bse_pattern_do_set_container (BseItem		*item,
						  BseItem		*container);
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
static BseTypeClass	*parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BsePattern)
{
  static const BseTypeInfo pattern_info = {
    sizeof (BsePatternClass),
    
    (BseClassInitBaseFunc) NULL,
    (BseClassDestroyBaseFunc) NULL,
    (BseClassInitFunc) bse_pattern_class_init,
    (BseClassDestroyFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BsePattern),
    16 /* n_preallocs */,
    (BseObjectInitFunc) bse_pattern_init,
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
  
  parent_class = bse_type_class_peek (BSE_TYPE_ITEM);
  object_class = BSE_OBJECT_CLASS (class);
  item_class = BSE_ITEM_CLASS (class);
  
  object_class->restore = bse_pattern_restore;
  object_class->store_private = bse_pattern_store_private;
  object_class->restore_private = bse_pattern_restore_private;
  object_class->destroy = bse_pattern_destroy;
  
  item_class->set_container = bse_pattern_do_set_container;
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
bse_pattern_destroy (BseObject *object)
{
  BsePattern *pattern;
  guint c, r;
  
  pattern = BSE_PATTERN (object);
  
  for (c = 0; c < pattern->n_channels; c++)
    for (r = 0; r < pattern->n_rows; r++)
      bse_pattern_reset_note (pattern, c, r);
  g_free (pattern->notes);
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_pattern_do_set_container (BseItem *item,
			      BseItem *container)
{
  BsePattern *pattern;
  
  if (container)
    g_return_if_fail (BSE_IS_SONG (container));
  
  pattern = BSE_PATTERN (item);
  
  /* chain parent class' set_container handler */
  BSE_ITEM_CLASS (parent_class)->set_container (item, container);
  
  if (container)
    {
      if (BSE_IS_SONG (container))
	{
	  BseSong *song;
	  
	  song = BSE_SONG (container);
	  
	  bse_pattern_set_n_channels (pattern, song->n_channels);
	  bse_pattern_set_n_rows (pattern, song->pattern_length);
	}
      else
	g_warning ("BsePattern is mean to be added to containers of type BseSong only");
    }
}

static void
bse_pattern_reset_note (BsePattern *pattern,
			guint	    channel,
			guint	    row)
{
  BseNote *note;
  
  note = &PNOTE (pattern, channel, row);

  note->note = BSE_NOTE_VOID;
  if (note->instrument)
    {
      bse_object_unref (BSE_OBJECT (note->instrument));
      note->instrument = NULL;
    }

  /* leave selected */

  if (note->effects || note->n_effects)
    g_warning ("effects are not yet imlemented, bogus non-NULL pointer encountered");
  note->effects = NULL;
  note->n_effects = 0;
}

void
bse_pattern_set_n_channels (BsePattern *pattern,
			    guint	n_channels)
{
  g_return_if_fail (BSE_IS_PATTERN (pattern));
  g_return_if_fail (n_channels >= 1 && n_channels <= BSE_MAX_N_CHANNELS);

  if (BSE_OBJECT_IS_LOCKED (pattern))
    return;
  
  if (pattern->n_channels != n_channels)
    {
      BseNote *notes;
      guint c, r;
      
      for (c = n_channels; c < pattern->n_channels; c++)
	for (r = 0; r < pattern->n_rows; r++)
	  bse_pattern_reset_note (pattern, c, r);
      
      notes = g_new (BseNote, n_channels * pattern->n_rows);
      
      for (c = 0; c < n_channels; c++)
	for (r = 0; r < pattern->n_rows; r++)
	  {
	    if (c < pattern->n_channels)
	      notes[INDX (n_channels, c, r)] = PNOTE (pattern, c, r);
	    else
	      {
		notes[INDX (n_channels, c, r)].note = BSE_NOTE_VOID;
		notes[INDX (n_channels, c, r)].instrument = NULL;
		notes[INDX (n_channels, c, r)].selected = FALSE;
		notes[INDX (n_channels, c, r)].n_effects = 0;
		notes[INDX (n_channels, c, r)].effects = NULL;
	      }
	  }
      
      g_free (pattern->notes);
      pattern->notes = notes;
      pattern->n_channels = n_channels;
      
      BSE_NOTIFY (pattern, size_changed, NOTIFY (OBJECT, DATA));
    }
}

void
bse_pattern_set_n_rows (BsePattern *pattern,
			guint	    n_rows)
{
  g_return_if_fail (BSE_IS_PATTERN (pattern));
  g_return_if_fail (n_rows >= BSE_MIN_PATTERN_LENGTH && n_rows <= BSE_MAX_PATTERN_LENGTH);
  
  if (BSE_OBJECT_IS_LOCKED (pattern))
    return;

  if (pattern->n_rows != n_rows)
    {
      BseNote *notes;
      guint c, r;
      
      for (r = n_rows; r < pattern->n_rows; r++)
	for (c = 0; c < pattern->n_channels; c++)
	  bse_pattern_reset_note (pattern, c, r);
      
      notes = g_new (BseNote, pattern->n_channels * n_rows);
      
      for (c = 0; c < pattern->n_channels; c++)
	for (r = 0; r < n_rows; r++)
	  {
	    if (r < pattern->n_rows)
	      notes[INDX (pattern->n_channels, c, r)] = PNOTE (pattern, c, r);
	    else
	      {
		notes[INDX (pattern->n_channels, c, r)].note = BSE_NOTE_VOID;
		notes[INDX (pattern->n_channels, c, r)].instrument = NULL;
		notes[INDX (pattern->n_channels, c, r)].selected = FALSE;
		notes[INDX (pattern->n_channels, c, r)].n_effects = 0;
		notes[INDX (pattern->n_channels, c, r)].effects = NULL;
	      }
	  }
      
      g_free (pattern->notes);
      pattern->notes = notes;
      pattern->n_rows = n_rows;
      
      BSE_NOTIFY (pattern, size_changed, NOTIFY (OBJECT, DATA));
    }
}

void
bse_pattern_modify_note (BsePattern    *pattern,
			 guint          channel,
			 guint          row,
			 guint          note_val,
			 BseInstrument *instrument,
			 gboolean       selected)
{
  BseNote *note;
  guint notify_changed = 0;

  g_return_if_fail (BSE_IS_PATTERN (pattern));
  g_return_if_fail (channel < pattern->n_channels);
  g_return_if_fail (row < pattern->n_rows);
  if (note_val != BSE_NOTE_VOID)
    g_return_if_fail (note_val >= BSE_MIN_NOTE && note_val <= BSE_MAX_NOTE);

  bse_object_lock (BSE_OBJECT (pattern));

  note = &PNOTE (pattern, channel, row);

  if (note->instrument != instrument)
    {
      if (note->instrument)
	bse_object_unref (BSE_OBJECT (note->instrument));
      note->instrument = instrument;
      if (note->instrument)
	bse_object_ref (BSE_OBJECT (note->instrument));
      notify_changed++;
    }
  if (note->note != note_val)
    {
      note->note = note_val;
      notify_changed++;
    }

  if (notify_changed)
    BSE_NOTIFY (pattern, note_changed, NOTIFY (OBJECT, channel, row, DATA));

  if (!note->selected && selected)
    {
      note->selected = TRUE;
      BSE_NOTIFY (pattern, note_selected, NOTIFY (OBJECT, channel, row, DATA));
    }
  else if (note->selected && !selected)
    {
      note->selected = FALSE;
      BSE_NOTIFY (pattern, note_unselected, NOTIFY (OBJECT, channel, row, DATA));
    }

  bse_object_unlock (BSE_OBJECT (pattern));
}

void
bse_pattern_set_note (BsePattern *pattern,
		      guint       channel,
		      guint       row,
		      guint       note)
{
  g_return_if_fail (BSE_IS_PATTERN (pattern));
  g_return_if_fail (channel < pattern->n_channels);
  g_return_if_fail (row < pattern->n_rows);
  if (note != BSE_NOTE_VOID)
    g_return_if_fail (note >= BSE_MIN_NOTE && note <= BSE_MAX_NOTE);

  bse_item_exec_void_proc (BSE_ITEM (pattern),
			   "set-note",
			   channel,
			   row,
			   note);
}

void
bse_pattern_set_instrument (BsePattern    *pattern,
			    guint          channel,
			    guint          row,
			    BseInstrument *instrument)
{
  g_return_if_fail (BSE_IS_PATTERN (pattern));
  g_return_if_fail (channel < pattern->n_channels);
  g_return_if_fail (row < pattern->n_rows);
  if (instrument)
    g_return_if_fail (BSE_IS_INSTRUMENT (instrument));

  bse_item_exec_void_proc (BSE_ITEM (pattern),
			   "set-instrument",
			   channel,
			   row,
			   instrument);
}

void
bse_pattern_select_note (BsePattern *pattern,
			 guint       channel,
			 guint       row)
{
  BseNote *note;

  g_return_if_fail (BSE_IS_PATTERN (pattern));
  g_return_if_fail (channel < pattern->n_channels);
  g_return_if_fail (row < pattern->n_rows);

  note = &PNOTE (pattern, channel, row);
  if (!note->selected)
    bse_pattern_modify_note (pattern,
			     channel, row,
			     note->note,
			     note->instrument,
			     TRUE);
  
  /*
    if (!PNOTE (pattern, channel, row).selected)
    bse_item_exec_void_proc (BSE_ITEM (pattern),
			     "select-note",
			     channel,
			     row);
  */
}

void
bse_pattern_unselect_note (BsePattern *pattern,
			   guint       channel,
			   guint       row)
{
  BseNote *note;

  g_return_if_fail (BSE_IS_PATTERN (pattern));
  g_return_if_fail (channel < pattern->n_channels);
  g_return_if_fail (row < pattern->n_rows);
  
  note = &PNOTE (pattern, channel, row);
  if (note->selected)
    bse_pattern_modify_note (pattern,
			     channel, row,
			     note->note,
			     note->instrument,
			     FALSE);
  /*
      if (PNOTE (pattern, channel, row).selected)
    bse_item_exec_void_proc (BSE_ITEM (pattern),
			     "unselect-note",
			     channel,
			     row);
  */
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
	BseNote *note = &PNOTE (pattern, c, r);

	if (note->selected)
	  list = g_list_prepend (list, note);
      }

  return g_list_reverse (list);
}

BseNote*
bse_pattern_peek_note (BsePattern *pattern,
		       guint       channel,
		       guint       row)
{
  g_return_val_if_fail (BSE_IS_PATTERN (pattern), NULL);
  g_return_val_if_fail (channel < pattern->n_channels, NULL);
  g_return_val_if_fail (row < pattern->n_rows, NULL);

  return &PNOTE (pattern, channel, row);
}

static BseNote*
get_note (BsePattern *pattern,
	  guint	      channel,
	  guint	      row)
{
  BseNote *note;
  
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
save_note (BseStorage *storage,
	   BseNote    *note)
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
	  BseNote *note, *note2, *note3;
	  
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
  guint note = BSE_NOTE_VOID;
  BseInstrument *instrument = NULL;
  BsePattern *pattern;
  
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
    }
  else
    return BSE_TOKEN_UNMATCHED;
  
  if (g_scanner_get_next_token (scanner) != ')')
    return ')';
  
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

      pattern->current_row++;
    }
  
  return G_TOKEN_NONE;
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
  
  pattern->current_channel = current_channel;
  pattern->current_row = current_row;
  
  return expected_token;
}
