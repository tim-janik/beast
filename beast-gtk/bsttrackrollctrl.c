/* BEAST - Bedevilled Audio System
 * Copyright (C) 2003 Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bsttrackrollctrl.h"

#include "bstpartdialog.h"


#define QUANTIZATION(self)	((self)->quant_rtools->action_id)
#define	HAVE_OBJECT		(1 << 31)


/* --- prototypes --- */
static void	controller_drag			(BstTrackRollController	*self,
						 BstTrackRollDrag	*drag);
static void	controller_update_canvas_cursor	(BstTrackRollController *self,
						 guint                   tool_id);
static void	controller_update_hpanel_cursor	(BstTrackRollController *self,
						 guint                   tool_id);
static void	controller_stop_edit		(BstTrackRollController *self,
						 gboolean                canceled,
						 GtkCellEditable        *ecell);


/* --- functions --- */
GxkActionList*
bst_track_roll_controller_canvas_actions (BstTrackRollController *self)
{
  GxkActionList *alist = gxk_action_list_create_grouped (self->canvas_rtools);
  static const GxkStockAction actions[] = {
    { N_("Insert"),             "I",    N_("Insert/edit/move parts (mouse button 1 and 2)"),
      BST_COMMON_ROLL_TOOL_INSERT,     BST_STOCK_PART_EDITOR },
    { N_("Link"),               "K",    N_("Link or move parts (mouse button 1 and 2)"),
      BST_COMMON_ROLL_TOOL_LINK,       BST_STOCK_PART_LINK },
    { N_("Rename"),             "E",    N_("Rename parts"),
      BST_COMMON_ROLL_TOOL_RENAME,     BST_STOCK_PART_TEXT },
    { N_("Delete"),             "D",    N_("Delete parts"),
      BST_COMMON_ROLL_TOOL_DELETE,     BST_STOCK_TRASHCAN },
  };
  gxk_action_list_add_actions (alist, G_N_ELEMENTS (actions), actions,
                               NULL /*i18n_domain*/, NULL /*acheck*/, NULL /*aexec*/, NULL);
  return alist;
}

GxkActionList*
bst_track_roll_controller_hpanel_actions (BstTrackRollController *self)
{
  GxkActionList *alist = gxk_action_list_create_grouped (self->hpanel_rtools);
  static const GxkStockAction actions[] = {
    { N_("Left"),               "L",    N_("Use the horizontal ruler to adjust the left loop pointer"),
      BST_COMMON_ROLL_TOOL_MOVE_TICK_LEFT,     BST_STOCK_TICK_LOOP_LEFT },
    { N_("Position"),           "P",    N_("Use the horizontal ruler to adjust the play position pointer"),
      BST_COMMON_ROLL_TOOL_MOVE_TICK_POINTER,  BST_STOCK_TICK_POINTER },
    { N_("Right"),              "R",    N_("Use the horizontal ruler to adjust the right loop pointer"),
      BST_COMMON_ROLL_TOOL_MOVE_TICK_RIGHT,    BST_STOCK_TICK_LOOP_RIGHT },
  };
  gxk_action_list_add_actions (alist, G_N_ELEMENTS (actions), actions,
                               NULL /*i18n_domain*/, NULL /*acheck*/, NULL /*aexec*/, NULL);
  return alist;
}

GxkActionList*
bst_track_roll_controller_quant_actions (BstTrackRollController *self)
{
  GxkActionList *alist = gxk_action_list_create_grouped (self->quant_rtools);
  static const GxkStockAction actions[] = {
    { N_("Q: Tact"),          "<ctrl>T",      N_("Quantize to tact boundaries"),
      BST_QUANTIZE_TACT,      BST_STOCK_QTACT, },
    { N_("Q: None"),          "<ctrl>0",      N_("No quantization selected"),
      BST_QUANTIZE_NONE,      BST_STOCK_QNOTE_NONE, },
    { N_("Q: 1\\/1"),         "<ctrl>1",      N_("Quantize to whole note boundaries"),
      BST_QUANTIZE_NOTE_1,    BST_STOCK_QNOTE_1, },
    { N_("Q: 1\\/2"),         "<ctrl>2",      N_("Quantize to half note boundaries"),
      BST_QUANTIZE_NOTE_2,    BST_STOCK_QNOTE_2, },
    { N_("Q: 1\\/4"),         "<ctrl>4",      N_("Quantize to quarter note boundaries"),
      BST_QUANTIZE_NOTE_4,    BST_STOCK_QNOTE_4, },
    { N_("Q: 1\\/8"),         "<ctrl>8",      N_("Quantize to eighths note boundaries"),
      BST_QUANTIZE_NOTE_8,    BST_STOCK_QNOTE_8, },
    { N_("Q: 1\\/16"),        "<ctrl>6",      N_("Quantize to sixteenth note boundaries"),
      BST_QUANTIZE_NOTE_16,   BST_STOCK_QNOTE_16, },
    { N_("Q: 1\\/32"),        "<ctrl>3",      N_("Quantize to thirty-second note boundaries"),
      BST_QUANTIZE_NOTE_32,   BST_STOCK_QNOTE_32, },
    { N_("Q: 1\\/64"),        "<ctrl>5",      N_("Quantize to sixty-fourth note boundaries"),
      BST_QUANTIZE_NOTE_64,   BST_STOCK_QNOTE_64, },
    { N_("Q: 1\\/128"),       "<ctrl>7",      N_("Quantize to hundred twenty-eighth note boundaries"),
      BST_QUANTIZE_NOTE_128,  BST_STOCK_QNOTE_128, },
  };
  gxk_action_list_add_actions (alist, G_N_ELEMENTS (actions), actions,
                               NULL /*i18n_domain*/, NULL /*acheck*/, NULL /*aexec*/, NULL);
  return alist;
}

static void
controller_reset_canvas_cursor (BstTrackRollController *self)
{
  controller_update_canvas_cursor (self, self->canvas_rtools->action_id);
}

static void
controller_reset_hpanel_cursor (BstTrackRollController *self)
{
  controller_update_hpanel_cursor (self, self->hpanel_rtools->action_id);
}

BstTrackRollController*
bst_track_roll_controller_new (BstTrackRoll *troll)
{
  BstTrackRollController *self;

  g_return_val_if_fail (BST_IS_TRACK_ROLL (troll), NULL);

  self = g_new0 (BstTrackRollController, 1);
  self->troll = troll;
  self->ref_count = 1;
  self->note_length = 1;
  self->current_tool = NULL;
  g_signal_connect_data (troll, "drag",
			 G_CALLBACK (controller_drag),
			 bst_track_roll_controller_ref (self),
			 (GClosureNotify) bst_track_roll_controller_unref,
			 G_CONNECT_SWAPPED);
  g_signal_connect_data (troll, "stop-edit",
			 G_CALLBACK (controller_stop_edit),
			 self, NULL,
			 G_CONNECT_SWAPPED);
  /* register canvas tools */
  self->canvas_rtools = gxk_action_group_new ();
  gxk_action_group_select (self->canvas_rtools, BST_COMMON_ROLL_TOOL_INSERT);
  g_object_connect (self->canvas_rtools,
		    "swapped_signal::changed", controller_reset_canvas_cursor, self,
		    NULL);
  controller_reset_canvas_cursor (self);
  /* register hpanel tools */
  self->hpanel_rtools = gxk_action_group_new ();
  gxk_action_group_select (self->hpanel_rtools, BST_COMMON_ROLL_TOOL_MOVE_TICK_POINTER);
  g_object_connect (self->hpanel_rtools,
		    "swapped_signal::changed", controller_reset_hpanel_cursor, self,
		    NULL);
  /* register quantization tools */
  self->quant_rtools = gxk_action_group_new ();
  gxk_action_group_select (self->quant_rtools, BST_QUANTIZE_TACT);

  return self;
}

static BstCommonRollTool
hpanel_button_tool (BstTrackRollController *self,
		    guint                   button)
{
  switch (self->hpanel_rtools->action_id)	/* user selected tool */
    {
    case BST_COMMON_ROLL_TOOL_MOVE_TICK_LEFT:
      switch (button)
	{
	case 1:	 return BST_COMMON_ROLL_TOOL_MOVE_TICK_LEFT;
	case 2:	 return BST_COMMON_ROLL_TOOL_MOVE_TICK_POINTER;
	default: return BST_COMMON_ROLL_TOOL_NONE;
	}
    case BST_COMMON_ROLL_TOOL_MOVE_TICK_POINTER:
      switch (button)
	{
	case 1:	 return BST_COMMON_ROLL_TOOL_MOVE_TICK_POINTER;
	case 2:	 return BST_COMMON_ROLL_TOOL_MOVE_TICK_POINTER;
	default: return BST_COMMON_ROLL_TOOL_NONE;
	}
    case BST_COMMON_ROLL_TOOL_MOVE_TICK_RIGHT:
      switch (button)
	{
	case 1:	 return BST_COMMON_ROLL_TOOL_MOVE_TICK_RIGHT;
	case 2:	 return BST_COMMON_ROLL_TOOL_MOVE_TICK_POINTER;
	default: return BST_COMMON_ROLL_TOOL_NONE;
	}
    }
  return BST_COMMON_ROLL_TOOL_NONE;
}

static BstCommonRollTool
canvas_button_tool (BstTrackRollController *self,
		    guint                   button,
		    guint                   have_object)
{
  switch (self->canvas_rtools->action_id | /* user selected tool */
	  (have_object ? HAVE_OBJECT : 0))
    {
    case BST_COMMON_ROLL_TOOL_INSERT | HAVE_OBJECT:
      switch (button)
	{
	case 1:  return BST_COMMON_ROLL_TOOL_EDITOR;
	case 2:  return BST_COMMON_ROLL_TOOL_MOVE;
	default: return BST_COMMON_ROLL_TOOL_NONE;
	}
    case BST_COMMON_ROLL_TOOL_INSERT:
      switch (button)
	{
	case 1:  return BST_COMMON_ROLL_TOOL_INSERT;
	case 2:  return BST_COMMON_ROLL_TOOL_MOVE;	/* error */
	default: return BST_COMMON_ROLL_TOOL_NONE;
	}
    case BST_COMMON_ROLL_TOOL_LINK | HAVE_OBJECT:
      switch (button)
	{
	case 1:  return BST_COMMON_ROLL_TOOL_LINK;
	case 2:  return BST_COMMON_ROLL_TOOL_MOVE;
	default: return BST_COMMON_ROLL_TOOL_NONE;
	}
    case BST_COMMON_ROLL_TOOL_LINK:
      switch (button)
	{
	case 1:  return BST_COMMON_ROLL_TOOL_LINK;
	case 2:  return BST_COMMON_ROLL_TOOL_MOVE;	/* error */
	default: return BST_COMMON_ROLL_TOOL_NONE;
	}
    case BST_COMMON_ROLL_TOOL_RENAME | HAVE_OBJECT:
      switch (button)
	{
	case 1:  return BST_COMMON_ROLL_TOOL_RENAME;
	case 2:  return BST_COMMON_ROLL_TOOL_MOVE;
	default: return BST_COMMON_ROLL_TOOL_NONE;
	}
    case BST_COMMON_ROLL_TOOL_RENAME:
      switch (button)
	{
	case 1:  return BST_COMMON_ROLL_TOOL_RENAME;	/* error */
	case 2:  return BST_COMMON_ROLL_TOOL_MOVE;	/* error */
	default: return BST_COMMON_ROLL_TOOL_NONE;
	}
    case BST_COMMON_ROLL_TOOL_DELETE | HAVE_OBJECT:
      switch (button)
	{
	case 1:  return BST_COMMON_ROLL_TOOL_DELETE;
	case 2:  return BST_COMMON_ROLL_TOOL_MOVE;
	default: return BST_COMMON_ROLL_TOOL_NONE;
	}
    case BST_COMMON_ROLL_TOOL_DELETE:
      switch (button)
	{
	case 1:  return BST_COMMON_ROLL_TOOL_DELETE;	/* error */
	case 2:  return BST_COMMON_ROLL_TOOL_MOVE;	/* error */
	default: return BST_COMMON_ROLL_TOOL_NONE;
	}
    case BST_COMMON_ROLL_TOOL_EDITOR | HAVE_OBJECT:
      switch (button)
	{
	case 1:  return BST_COMMON_ROLL_TOOL_EDITOR;
	case 2:  return BST_COMMON_ROLL_TOOL_MOVE;
	default: return BST_COMMON_ROLL_TOOL_NONE;
	}
    case BST_COMMON_ROLL_TOOL_EDITOR:
      switch (button)
	{
	case 1:  return BST_COMMON_ROLL_TOOL_EDITOR;     	/* error */
	case 2:  return BST_COMMON_ROLL_TOOL_MOVE;		/* error */
	default: return BST_COMMON_ROLL_TOOL_NONE;
	}
      break;
    }
  return BST_COMMON_ROLL_TOOL_NONE;
}

BstTrackRollController*
bst_track_roll_controller_ref (BstTrackRollController *self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (self->ref_count >= 1, NULL);

  self->ref_count++;

  return self;
}

void
bst_track_roll_controller_unref (BstTrackRollController *self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->ref_count >= 1);

  self->ref_count--;
  if (!self->ref_count)
    {
      gxk_action_group_dispose (self->canvas_rtools);
      g_object_unref (self->canvas_rtools);
      gxk_action_group_dispose (self->hpanel_rtools);
      g_object_unref (self->hpanel_rtools);
      gxk_action_group_dispose (self->quant_rtools);
      g_object_unref (self->quant_rtools);
      g_free (self);
    }
}

void
bst_track_roll_controller_set_song (BstTrackRollController *self,
				    SfiProxy                song)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->ref_count >= 1);

  if (BSE_IS_SONG (song))
    self->song = song;
  else
    self->song = 0;
}

guint
bst_track_roll_controller_quantize (BstTrackRollController *self,
				    guint                   fine_tick)
{
  BseSongTiming *timing;
  guint quant, tick, qtick;

  g_return_val_if_fail (self != NULL, fine_tick);

  timing = bse_song_get_timing (self->song, fine_tick);
  if (QUANTIZATION (self) == BST_QUANTIZE_NONE)
    quant = 1;
  else if (QUANTIZATION (self) == BST_QUANTIZE_TACT)
    quant = timing->tpt;
  else
    quant = timing->tpqn * 4 / QUANTIZATION (self);
  tick = fine_tick - timing->tick;
  qtick = tick / quant;
  qtick *= quant;
  if (tick - qtick > quant / 2)
    qtick += quant;
  tick = timing->tick + qtick;
  return tick;
}

static void
controller_update_hpanel_cursor (BstTrackRollController *self,
				 guint                  tool_id)
{
  GxkScrollCanvas *scc = GXK_SCROLL_CANVAS (self->troll);
  switch (tool_id)
    {
    case BST_COMMON_ROLL_TOOL_MOVE_TICK_POINTER:
      gxk_scroll_canvas_set_top_panel_cursor (scc, GDK_SB_DOWN_ARROW);
      break;
    case BST_COMMON_ROLL_TOOL_MOVE_TICK_LEFT:
      gxk_scroll_canvas_set_top_panel_cursor (scc, GDK_LEFT_SIDE);
      break;
    case BST_COMMON_ROLL_TOOL_MOVE_TICK_RIGHT:
      gxk_scroll_canvas_set_top_panel_cursor (scc, GDK_RIGHT_SIDE);
      break;
    default:
      gxk_scroll_canvas_set_top_panel_cursor (scc, GXK_DEFAULT_CURSOR);
      break;
    }
}

static void
controller_update_canvas_cursor (BstTrackRollController *self,
				 guint                  tool_id)
{
  GxkScrollCanvas *scc = GXK_SCROLL_CANVAS (self->troll);
  switch (tool_id)
    {
    case BST_COMMON_ROLL_TOOL_INSERT:
      gxk_scroll_canvas_set_canvas_cursor (scc, GDK_PENCIL);
      break;
    case BST_COMMON_ROLL_TOOL_LINK:
      gxk_scroll_canvas_set_canvas_cursor (scc, GDK_DIAMOND_CROSS);
      break;
    case BST_COMMON_ROLL_TOOL_RENAME:
      gxk_scroll_canvas_set_canvas_cursor (scc, GXK_DEFAULT_CURSOR);
      break;
    case BST_COMMON_ROLL_TOOL_MOVE:
      gxk_scroll_canvas_set_canvas_cursor (scc, GDK_FLEUR);
      break;
    case BST_COMMON_ROLL_TOOL_DELETE:
      gxk_scroll_canvas_set_canvas_cursor (scc, GDK_TARGET);
      break;
    case BST_COMMON_ROLL_TOOL_EDITOR:
      gxk_scroll_canvas_set_canvas_cursor (scc, GDK_HAND2);
      break;
    default:
      gxk_scroll_canvas_set_canvas_cursor (scc, GXK_DEFAULT_CURSOR);
      break;
    }
}

static void
edit_name_start (BstTrackRollController *self,
		 BstTrackRollDrag       *drag)
{
  if (drag->current_track && self->obj_part)
    {
      GtkEntry *entry = g_object_new (GTK_TYPE_ENTRY,
				      "visible", TRUE,
				      "has_frame", FALSE,
				      NULL);
      const gchar *name = bse_item_get_name (self->obj_part);
      if (name)
	gtk_entry_set_text (entry, name);
      bst_track_roll_start_edit (self->troll, drag->current_row,
				 self->obj_tick, self->obj_duration,
				 GTK_CELL_EDITABLE (entry));
      g_signal_connect_data (entry, "editing_done", G_CALLBACK (bst_track_roll_stop_edit),
			     self->troll, NULL, G_CONNECT_SWAPPED);
      controller_update_canvas_cursor (self, BST_COMMON_ROLL_TOOL_RENAME);
    }
  else
    gxk_status_set (GXK_STATUS_ERROR, _("Edit Part"), _("No Part"));
  drag->state = GXK_DRAG_HANDLED;
}

static void
controller_stop_edit (BstTrackRollController *self,
		      gboolean                canceled,
		      GtkCellEditable        *ecell)
{
  if (!canceled)
    {
      bse_item_set_name (self->obj_part, gtk_entry_get_text (GTK_ENTRY (ecell)));
      gxk_status_set (GXK_STATUS_DONE, _("Edit Part"), NULL);
    }
  controller_reset_canvas_cursor (self);
}

static void
insert_start (BstTrackRollController *self,
	      BstTrackRollDrag       *drag)
{
  if (drag->current_track && drag->current_valid && !self->obj_part)
    {
      guint tick = bst_track_roll_controller_quantize (self, drag->current_tick);
      SfiProxy item = bse_track_get_part (drag->current_track, tick);
      if (!item)
	{
	  SfiProxy song = bse_item_get_parent (drag->current_track);
          bse_item_group_undo (song, "Insert part");
	  item = bse_song_create_part (song);
	  if (item && bse_track_insert_part (drag->current_track, tick, item) > 0)
	    gxk_status_set (GXK_STATUS_DONE, _("Insert Part"), NULL);
	  else
	    gxk_status_set (GXK_STATUS_ERROR, _("Insert Part"), _("Lost Part"));
          bse_item_ungroup_undo (song);
	  drag->state = GXK_DRAG_HANDLED;
	}
      else
	gxk_status_set (GXK_STATUS_ERROR, _("Insert Part"), _("Position taken"));
    }
  else
    {
      if (self->obj_part)
	gxk_status_set (GXK_STATUS_ERROR, _("Insert Part"), _("Position taken"));
      else
	gxk_status_set (GXK_STATUS_ERROR, _("Insert part"), _("No Track"));
      drag->state = GXK_DRAG_HANDLED;
    }
}

static void
delete_start (BstTrackRollController *self,
	      BstTrackRollDrag       *drag)
{
  if (self->obj_part)	/* got part to delete */
    {
      bse_item_group_undo (self->song, "Delete Part");
      bse_track_remove_tick (self->obj_track, self->obj_tick);
      if (!bse_song_find_any_track_for_part (self->song, self->obj_part))
        bse_song_remove_part (self->song, self->obj_part);
      bse_item_ungroup_undo (self->song);
      gxk_status_set (GXK_STATUS_DONE, _("Delete Part"), NULL);
    }
  else
    gxk_status_set (GXK_STATUS_ERROR, _("Delete Part"), _("No target"));
  drag->state = GXK_DRAG_HANDLED;
}

static void
move_link_start (BstTrackRollController *self,
		 BstTrackRollDrag       *drag,
		 gboolean		 link_pending)
{
  const gchar *action = link_pending ? _("Link Part") : _("Move Part");
  if (self->obj_part)	/* got part to move */
    {
      self->xoffset = drag->start_tick - self->obj_tick;	/* drag offset */
      controller_update_canvas_cursor (self, link_pending ? BST_COMMON_ROLL_TOOL_LINK : BST_COMMON_ROLL_TOOL_MOVE);
      gxk_status_set (GXK_STATUS_PROGRESS, action, NULL);
      drag->state = GXK_DRAG_CONTINUE;
      self->skip_deletion = link_pending;
    }
  else
    {
      gxk_status_set (GXK_STATUS_ERROR, action, _("No target"));
      drag->state = GXK_DRAG_HANDLED;
    }
}

static void
move_start (BstTrackRollController *self,
	    BstTrackRollDrag       *drag)
{
  move_link_start (self, drag, FALSE);
}

static void
link_start (BstTrackRollController *self,
	    BstTrackRollDrag       *drag)
{
  move_link_start (self, drag, TRUE);
}

static void
move_motion (BstTrackRollController *self,
	     BstTrackRollDrag       *drag)
{
  const gchar *action = self->skip_deletion ? _("Link Part") : _("Move Part");
  gint new_tick;
  gboolean track_changed;

  new_tick = MAX (drag->current_tick, self->xoffset) - self->xoffset;
  new_tick = bst_track_roll_controller_quantize (self, new_tick);
  track_changed = self->obj_track != drag->current_track;
  if (new_tick != self->obj_tick || self->obj_track != drag->current_track)
    {
      bse_item_group_undo (drag->current_track, "Move part");
      if (bse_track_insert_part (drag->current_track, new_tick, self->obj_part) > 0)
	{
	  if (!self->skip_deletion)
	    bse_track_remove_tick (self->obj_track, self->obj_tick);
	  else
	    {
	      self->skip_deletion = FALSE;
	      controller_update_canvas_cursor (self, BST_COMMON_ROLL_TOOL_MOVE);
	    }
	  self->obj_track = drag->current_track;
	  self->obj_tick = new_tick;
	  gxk_status_set (GXK_STATUS_PROGRESS, action, NULL);
	}
      /* else gxk_status_set (GXK_STATUS_ERROR, "Move Part", bse_error_blurb (error)); */
      bse_item_ungroup_undo (drag->current_track);
    }
}

static void
move_abort (BstTrackRollController *self,
	    BstTrackRollDrag       *drag)
{
  gxk_status_set (GXK_STATUS_ERROR, _("Move Part"), _("Lost Part"));
}

static void
editor_create (BstTrackRollController *self,
               BstTrackRollDrag       *drag)
{
  if (self->obj_part)	/* got part */
    {
      GtkWidget *pdialog = g_object_new (BST_TYPE_PART_DIALOG, NULL);
      bst_part_dialog_set_proxy (BST_PART_DIALOG (pdialog), self->obj_part);
      g_signal_connect_object (self->troll, "destroy", G_CALLBACK (gtk_widget_destroy), pdialog, G_CONNECT_SWAPPED);
      gxk_status_set (GXK_STATUS_DONE, _("Start Editor"), NULL);
      gxk_idle_show_widget (pdialog);
      gxk_action_group_select (self->canvas_rtools, BST_COMMON_ROLL_TOOL_INSERT);
    }
  else
    gxk_status_set (GXK_STATUS_ERROR, _("Start Editor"), _("No target"));
  drag->state = GXK_DRAG_HANDLED;
}

static void
pointer_move (BstTrackRollController *self,
	      BstTrackRollDrag       *drag)
{
  if (self->song &&
      drag->type != GXK_DRAG_DONE) /* skip release events */
    {
      guint tick = bst_track_roll_controller_quantize (self, drag->current_tick);
      bse_proxy_set (self->song, "tick-pointer", tick, NULL);
      drag->state = GXK_DRAG_CONTINUE;
    }
}

static void
tick_left_move (BstTrackRollController *self,
		BstTrackRollDrag       *drag)
{
  if (self->song)
    {
      guint tick = bst_track_roll_controller_quantize (self, drag->current_tick);
      bse_proxy_set (self->song, "loop-left", tick, NULL);
      drag->state = GXK_DRAG_CONTINUE;
    }
}

static void
tick_right_move (BstTrackRollController *self,
		 BstTrackRollDrag       *drag)
{
  if (self->song)
    {
      guint tick = bst_track_roll_controller_quantize (self, drag->current_tick);
      bse_proxy_set (self->song, "loop-right", tick, NULL);
      drag->state = GXK_DRAG_CONTINUE;
    }
}

typedef void (*DragFunc) (BstTrackRollController *,
			  BstTrackRollDrag       *);
struct _BstTrackRollUtil
{
  BstCommonRollTool tool;
  DragFunc start, motion, abort;
};

void
controller_drag (BstTrackRollController *self,
		 BstTrackRollDrag       *drag)
{
  static const BstTrackRollUtil canvas_tool_table[] = {
    { BST_COMMON_ROLL_TOOL_INSERT,	insert_start,		NULL,		NULL,		},
    { BST_COMMON_ROLL_TOOL_MOVE,	move_start,		move_motion,	move_abort,	},
    { BST_COMMON_ROLL_TOOL_LINK,	link_start,		move_motion,	move_abort,	},
    { BST_COMMON_ROLL_TOOL_DELETE,	delete_start,		NULL,		NULL,		},
    { BST_COMMON_ROLL_TOOL_RENAME,	edit_name_start,	NULL,		NULL,		},
    { BST_COMMON_ROLL_TOOL_EDITOR,	editor_create,		NULL,		NULL,		},
  };
  static const BstTrackRollUtil hpanel_tool_table[] = {
    { BST_COMMON_ROLL_TOOL_MOVE_TICK_POINTER,	pointer_move,	 pointer_move,	  NULL,		},
    { BST_COMMON_ROLL_TOOL_MOVE_TICK_LEFT,	tick_left_move,	 tick_left_move,  NULL,		},
    { BST_COMMON_ROLL_TOOL_MOVE_TICK_RIGHT,	tick_right_move, tick_right_move, NULL,		},
  };

  /* initial drag handling */
  if (drag->type == GXK_DRAG_START)
    {
      BstCommonRollTool obj_tool, tool;
      BseTrackPart *tpart = NULL;
      const BstTrackRollUtil *tool_table;
      guint i, n_tools;
      BseTrackPartSeq *tps;

      self->current_tool = NULL;	/* paranoid */

      /* figure area specific tool */
      if (drag->canvas_drag)
	{
	  tool_table = canvas_tool_table;
	  n_tools = G_N_ELEMENTS (canvas_tool_table);
	  obj_tool = canvas_button_tool (self, drag->button, HAVE_OBJECT);
	  tool = canvas_button_tool (self, drag->button, 0);
	}
      else if (drag->top_panel_drag)
	{
	  tool_table = hpanel_tool_table;
	  n_tools = G_N_ELEMENTS (hpanel_tool_table);
	  tool = hpanel_button_tool (self, drag->button);
	  obj_tool = tool;
	}
      else	/* unsupported area */
	return;

      /* setup drag data */
      if (!drag->start_valid)
	drag->start_track = 0;
      tps = drag->start_track ? bse_track_list_parts (drag->start_track) : NULL;
      if (tps && tps->n_tparts)	/* FIXME: BSE should have a convenience function to find a part */
	{
	  gint j;
	  for (j = 0; j < tps->n_tparts; j++)
	    if (tps->tparts[j]->tick <= drag->start_tick &&
		tps->tparts[j]->tick + tps->tparts[j]->duration > drag->start_tick)
	      {
		tpart = tps->tparts[j];
		break;
	      }
	}
      self->obj_track = drag->start_track;
      self->obj_part = tpart ? tpart->part : 0;
      self->obj_tick = tpart ? tpart->tick : 0;
      self->obj_duration = tpart ? tpart->duration : 0;
      self->xoffset = 0;
      self->tick_bound = 0;
      
      /* find drag tool */
      if (self->obj_part)		/* have object */
	tool = obj_tool;
      for (i = 0; i < n_tools; i++)
	if (tool_table[i].tool == tool)
	  {
	    self->current_tool = (BstTrackRollUtil*) (tool_table + i);
	    break;
	  }
    }
  if (!self->current_tool)
    return;

  /* generic drag handling */
  switch (drag->type)
    {
    case GXK_DRAG_START:
      if (self->current_tool->start)
	self->current_tool->start (self, drag);
      break;
    case GXK_DRAG_MOTION:
    case GXK_DRAG_DONE:
      if (self->current_tool->motion)
	self->current_tool->motion (self, drag);
      break;
    case GXK_DRAG_ABORT:
      if (self->current_tool->abort)
	self->current_tool->abort (self, drag);
      break;
    }
  if (drag->type == GXK_DRAG_DONE || drag->type == GXK_DRAG_ABORT)
    {
      self->current_tool = NULL;
      controller_reset_canvas_cursor (self);
      controller_reset_hpanel_cursor (self);
    }
}
