/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999, 2000 Olaf Hoehmann and Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * bstpatterneditorselection.c: implement selection functionality for BsePatternEditor
 */


#define SELECTION_TIMEOUT                       (33)
#define	SAVED_SELECTION_TEST(pe, channel, row)	(BSE_PATTERN_SELECTION_TEST ((pe)->saved_selection, (channel), (row)))


/* --- prototypes --- */
static void bst_pattern_editor_selection_meminit (BstPatternEditor *pe);
static void bst_pattern_editor_selection_start   (BstPatternEditor *pe,
						  guint             channel,
						  guint             row,
						  gint              panel_sa_x,
						  gint              panel_sa_y,
						  gboolean          keep_selection,
						  gboolean          subtract);
static void bst_pattern_editor_selection_update  (BstPatternEditor *pe,
						  guint             channel,
						  guint             row,
						  gint              panel_sa_x,
						  gint              panel_sa_y,
						  gboolean          force_update);
static void bst_pattern_editor_selection_motion  (BstPatternEditor *pe,
						  gint              panel_sa_x,
						  gint              panel_sa_y);
static void bst_pattern_editor_selection_done    (BstPatternEditor *pe);


/* --- functions --- */
static void
bst_pattern_editor_selection_meminit (BstPatternEditor *pe)
{
  /* initialize the selection relevant memory fields
   * of a pattern editor
   */
  pe->in_selection = FALSE;
  pe->selection_subtract = FALSE;
  pe->saved_selection = NULL;
  pe->selection_channel = 0;
  pe->selection_row = 0;
  pe->selection_timer = 0;
  pe->selection_timer_channel = 0;
  pe->selection_timer_row = 0;
}

static void
bst_pattern_editor_selection_start (BstPatternEditor *pe,
				    guint	      channel,
				    guint	      row,
				    gint	      panel_sa_x,
				    gint	      panel_sa_y,
				    gboolean	      keep_selection,
				    gboolean	      subtract)
{
  GdkCursor *cursor;
  gboolean failed;

  g_return_if_fail (pe->in_selection == FALSE);
  
  channel = MIN (channel, N_CHANNELS (pe) - 1);
  row = MIN (row, N_ROWS (pe) - 1);
  
  cursor = gdk_cursor_new (GDK_FLEUR);
  failed = gdk_pointer_grab (pe->panel_sa, FALSE,
			     GDK_BUTTON_PRESS_MASK |
			     GDK_BUTTON_RELEASE_MASK |
			     GDK_POINTER_MOTION_MASK |
			     GDK_POINTER_MOTION_HINT_MASK,
			     NULL,
			     cursor,
			     GDK_CURRENT_TIME);
  gdk_cursor_destroy (cursor);
  if (failed)
    {
      gdk_beep ();
      return;
    }
  
  pe->in_selection = TRUE;
  bse_object_lock (BSE_OBJECT (pe->pattern));
  
  pe->selection_subtract = subtract != FALSE;
  pe->saved_selection = bse_pattern_selection_new (pe->pattern->n_channels, pe->pattern->n_rows);
  bse_pattern_save_selection (pe->pattern, pe->saved_selection);
  if (!keep_selection)
    {
      guint32 *tmp_selection = bse_pattern_selection_new (pe->pattern->n_channels, pe->pattern->n_rows);

      bse_pattern_selection_fill (tmp_selection, FALSE);
      bse_pattern_restore_selection (pe->pattern, tmp_selection);
      bse_pattern_selection_free (tmp_selection);
    }
  pe->selection_timer = 0;
  
  bst_pattern_editor_selection_update (pe, channel, row, panel_sa_x, panel_sa_y, TRUE);
}

static void
bst_pattern_editor_selection_update (BstPatternEditor *pe,
				     guint	       channel,
				     guint	       row,
				     gint	       panel_sa_x,
				     gint	       panel_sa_y,
				     gboolean	       force_update)
{
  guint r, c;
  gboolean selection_started = FALSE;

  g_return_if_fail (pe->in_selection == TRUE);

  channel = MIN (channel, N_CHANNELS (pe) - 1);
  row = MIN (row, N_ROWS (pe) - 1);
  
  if (pe->selection_channel != channel ||
      pe->selection_row != row ||
      force_update)
    {
      /* begin and end coordinates of the selection region */
      guint b_c = MIN (pe->selection_channel, channel);
      guint b_r = MIN (pe->selection_row, row);
      guint e_c = MAX (pe->selection_channel, channel);
      guint e_r = MAX (pe->selection_row, row);
      
      if (selection_started)
	{
	  b_c = 0;
	  b_r = 0;
	  e_c = N_CHANNELS (pe) - 1;
	  e_r = N_ROWS (pe) - 1;
	}
      else
	{
	  b_c = MIN (pe->focus_channel, b_c);
	  b_r = MIN (pe->focus_row, b_r);
	  e_c = MAX (pe->focus_channel, e_c);
	  e_r = MAX (pe->focus_row, e_r);
	}
      
      pe->selection_channel = channel;
      pe->selection_row = row;
      
      for (c = b_c; c <= e_c; c++)
	for (r = b_r; r <= e_r; r++)
	  {
	    BsePatternNote *note = bse_pattern_peek_note (pe->pattern, c, r);
	    gboolean want_selection, selected = note->selected;
	    gboolean in_selection = (c >= MIN (pe->focus_channel, pe->selection_channel) &&
				     c <= MAX (pe->focus_channel, pe->selection_channel) &&
				     r >= MIN (pe->focus_row, pe->selection_row) &&
				     r <= MAX (pe->focus_row, pe->selection_row));
	    
	    if (pe->selection_subtract)
	      want_selection = !in_selection && SAVED_SELECTION_TEST (pe, c, r);
	    else
	      want_selection = in_selection || SAVED_SELECTION_TEST (pe, c, r);
	    
	    if (want_selection && !selected)
	      bse_pattern_select_note (pe->pattern, c, r);
	    else if (!want_selection && selected)
	      bse_pattern_unselect_note (pe->pattern, c, r);
	  }
      
      bst_pattern_editor_adjust_sas (pe, FALSE);
      bst_pattern_editor_selection_motion (pe, panel_sa_x, panel_sa_y);
    }
}

static gint
selection_timeout (gpointer data)
{
  BstPatternEditor *pe = BST_PATTERN_EDITOR (data);
  
  g_return_val_if_fail (pe->in_selection == TRUE, FALSE);

  pe->selection_timer = 0;
  
  if (pe->in_selection)
    {
      gint x, y;
      
      gdk_window_get_pointer (pe->panel_sa, &x, &y, NULL);
      bst_pattern_editor_selection_update (pe,
					   pe->selection_timer_channel,
					   pe->selection_timer_row,
					   x, y,
					   FALSE);
    }
  
  return FALSE;
}

static void
bst_pattern_editor_selection_motion (BstPatternEditor *pe,
				     gint	       panel_sa_x,
				     gint	       panel_sa_y)
{
  gint channel, row;
  gint sa_x, sa_y, sa_width, sa_height;
  BstCellType cell_type;

  g_return_if_fail (pe->in_selection == TRUE);
  
  gdk_window_get_position (pe->panel, &sa_x, &sa_y);
  gdk_window_get_size (pe->panel_sa, &sa_width, &sa_height);
  
  if (bst_pattern_editor_get_cell (pe,
				   panel_sa_x - sa_x,
				   panel_sa_y - sa_y,
				   &cell_type,
				   &channel, &row) &&
      /* cell_type && */
      panel_sa_x > 0 && panel_sa_x < sa_width &&
      panel_sa_y > 0 && panel_sa_y < sa_height)
    {
      /* ok, pointer is *within* the scroll area, update selection */
      
      if (pe->selection_timer)
	{
	  gtk_timeout_remove (pe->selection_timer);
	  pe->selection_timer = 0;
	}
      
      bst_pattern_editor_selection_update (pe,
					   channel, row,
					   panel_sa_x, panel_sa_y,
					   FALSE);
    }
  else if (panel_sa_x < 0 || panel_sa_x > sa_width ||
	   panel_sa_y < 0 || panel_sa_y > sa_height)
    {
      /* pointer is *outside* the scroll area, fire off timer to adjust scroll region */

      pe->selection_timer_channel = MAX (0, channel);
      pe->selection_timer_row = MAX (0, row);
      
      if (!pe->selection_timer)
	pe->selection_timer = gtk_timeout_add (SELECTION_TIMEOUT,
					       selection_timeout,
					       pe);
    }
}

static void
bst_pattern_editor_selection_done (BstPatternEditor *pe)
{
  g_return_if_fail (pe->in_selection == TRUE);
  
  gdk_pointer_ungrab (GDK_CURRENT_TIME);
  
  bse_pattern_selection_free (pe->saved_selection);
  pe->saved_selection = NULL;
  
  pe->selection_subtract = FALSE;
  pe->selection_channel = 0;
  pe->selection_row = 0;
  
  if (pe->selection_timer)
    {
      gtk_timeout_remove (pe->selection_timer);
      pe->selection_timer = 0;
    }

  bse_object_unlock (BSE_OBJECT (pe->pattern));
  pe->in_selection = FALSE;
}

void
bst_pattern_editor_reset_selection (BstPatternEditor *pe)
{
  g_return_if_fail (BST_IS_PATTERN_EDITOR (pe));
  g_return_if_fail (pe->pattern != NULL);

  if (!pe->in_selection)
    {
      guint32 *tmp_selection = bse_pattern_selection_new (pe->pattern->n_channels, pe->pattern->n_rows);

      bse_pattern_selection_fill (tmp_selection, FALSE);
      bse_pattern_restore_selection (pe->pattern, tmp_selection);
      bse_pattern_selection_free (tmp_selection);
    }
}
