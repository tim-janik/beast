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


/* --- prototypes --- */
static void	controller_drag			(BstTrackRollController	*self,
						 BstTrackRollDrag	*drag);
static void	controller_update_canvas_cursor	(BstTrackRollController *self,
						 BstTrackRollTool	 tool);
static void	controller_update_hpanel_cursor	(BstTrackRollController *self,
						 BstTrackRollTool	 tool);
static void	controller_stop_edit		(BstTrackRollController *self,
						 gboolean                canceled,
						 GtkCellEditable        *ecell);


/* --- functions --- */
BstTrackRollController*
bst_track_roll_controller_new (BstTrackRoll *troll)
{
  BstTrackRollController *self;

  g_return_val_if_fail (BST_IS_TRACK_ROLL (troll), NULL);

  self = g_new0 (BstTrackRollController, 1);
  self->troll = troll;
  self->ref_count = 1;

  self->ref_count++;
  self->note_length = 1;
  g_signal_connect_data (troll, "drag",
			 G_CALLBACK (controller_drag),
			 self, (GClosureNotify) bst_track_roll_controller_unref,
			 G_CONNECT_SWAPPED);
  g_signal_connect_data (troll, "stop-edit",
			 G_CALLBACK (controller_stop_edit),
			 self, NULL,
			 G_CONNECT_SWAPPED);
  return self;
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
    g_free (self);
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

void
bst_track_roll_controller_set_quantization (BstTrackRollController *self,
					    BstQuantizationType     quantization)
{
  g_return_if_fail (self != NULL);

  switch (quantization)
    {
    case BST_QUANTIZE_TACT:
      self->quantization = quantization;
      break;
    case BST_QUANTIZE_NOTE_1:
    case BST_QUANTIZE_NOTE_2:
    case BST_QUANTIZE_NOTE_4:
    case BST_QUANTIZE_NOTE_8:
    case BST_QUANTIZE_NOTE_16:
    case BST_QUANTIZE_NOTE_32:
    case BST_QUANTIZE_NOTE_64:
    case BST_QUANTIZE_NOTE_128:
      self->quantization = quantization;
      break;
    default:
      self->quantization = BST_QUANTIZE_NONE;
      break;
    }
}

guint
bst_track_roll_controller_quantize (BstTrackRollController *self,
				    guint                   fine_tick)
{
  BseSongTiming *timing;
  guint quant, tick, qtick;

  g_return_val_if_fail (self != NULL, fine_tick);

  timing = bse_song_get_timing (self->song, fine_tick);
  if (self->quantization == BST_QUANTIZE_NONE)
    quant = 1;
  else if (self->quantization == BST_QUANTIZE_TACT)
    quant = timing->tpt;
  else
    quant = timing->tpqn * 4 / self->quantization;
  tick = fine_tick - timing->tick;
  qtick = tick / quant;
  qtick *= quant;
  if (tick - qtick > quant / 2)
    qtick += quant;
  tick = timing->tick + qtick;
  return tick;
}

void
bst_track_roll_controller_set_canvas_reset (BstTrackRollController   *self,
					    void (*handler) (gpointer data),
					    gpointer                  data)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->ref_count >= 1);

  self->canvas_reset = handler;
  self->canvas_reset_data = self->canvas_reset ? data : NULL;
}

void
bst_track_roll_controller_set_object_tools (BstTrackRollController *self,
					    BstTrackRollTool        tool1,
					    BstTrackRollTool        tool2,
					    BstTrackRollTool        tool3)
{
  g_return_if_fail (self != NULL);

  self->object_tool[0] = tool1;
  self->object_tool[1] = tool2;
  self->object_tool[2] = tool3;
}

void
bst_track_roll_controller_set_hpanel_tools (BstTrackRollController *self,
					    BstTrackRollTool        tool1,
					    BstTrackRollTool        tool2,
					    BstTrackRollTool        tool3)
{
  g_return_if_fail (self != NULL);

  self->hpanel_tool[0] = tool1;
  self->hpanel_tool[1] = tool2;
  self->hpanel_tool[2] = tool3;
  controller_update_hpanel_cursor (self, self->hpanel_tool[0]);
}

void
bst_track_roll_controller_set_canvas_tools (BstTrackRollController *self,
					    BstTrackRollTool        tool1,
					    BstTrackRollTool        tool2,
					    BstTrackRollTool        tool3)
{
  g_return_if_fail (self != NULL);
  
  self->canvas_tool[0] = tool1;
  self->canvas_tool[1] = tool2;
  self->canvas_tool[2] = tool3;
  controller_update_canvas_cursor (self, self->canvas_tool[0]);
}

static void
controller_update_hpanel_cursor (BstTrackRollController *self,
				 BstTrackRollTool        tool)
{
  switch (tool)
    {
    case BST_TRACK_ROLL_TOOL_MOVE_TICK_POINTER:
      bst_track_roll_set_hpanel_cursor (self->troll, GDK_SB_DOWN_ARROW);
      break;
    case BST_TRACK_ROLL_TOOL_MOVE_TICK_LEFT:
      bst_track_roll_set_hpanel_cursor (self->troll, GDK_LEFT_SIDE);
      break;
    case BST_TRACK_ROLL_TOOL_MOVE_TICK_RIGHT:
      bst_track_roll_set_hpanel_cursor (self->troll, GDK_RIGHT_SIDE);
      break;
    default:
      bst_track_roll_set_hpanel_cursor (self->troll, GXK_DEFAULT_CURSOR);
      break;
    }
}

static void
controller_update_canvas_cursor (BstTrackRollController *self,
				 BstTrackRollTool        tool)
{
  switch (tool)
    {
    case BST_TRACK_ROLL_TOOL_INSERT:
      bst_track_roll_set_canvas_cursor (self->troll, GDK_PENCIL);
      break;
    case BST_TRACK_ROLL_TOOL_EDIT_NAME:
      bst_track_roll_set_canvas_cursor (self->troll, GXK_DEFAULT_CURSOR);
      break;
    case BST_TRACK_ROLL_TOOL_MOVE:
      bst_track_roll_set_canvas_cursor (self->troll, GDK_FLEUR);
      break;
    case BST_TRACK_ROLL_TOOL_DELETE:
      bst_track_roll_set_canvas_cursor (self->troll, GDK_TARGET);
      break;
    case BST_TRACK_ROLL_TOOL_EDITOR_ONCE:
      bst_track_roll_set_canvas_cursor (self->troll, GDK_HAND2);
      break;
    default:
      bst_track_roll_set_canvas_cursor (self->troll, GXK_DEFAULT_CURSOR);
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
      controller_update_canvas_cursor (self, BST_TRACK_ROLL_TOOL_EDIT_NAME);
    }
  else
    gxk_status_set (GXK_STATUS_ERROR, "Edit Part", "No Part");
  drag->state = BST_DRAG_HANDLED;
}

static void
controller_stop_edit (BstTrackRollController *self,
		      gboolean                canceled,
		      GtkCellEditable        *ecell)
{
  if (!canceled)
    {
      bse_item_set_name (self->obj_part, gtk_entry_get_text (GTK_ENTRY (ecell)));
      gxk_status_set (GXK_STATUS_DONE, "Edit Part", NULL);
    }
  controller_update_canvas_cursor (self, self->canvas_tool[0]);
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
	  item = bse_song_create_part (song);
	  if (item && bse_track_insert_part (drag->current_track, tick, item) == BSE_ERROR_NONE)
	    gxk_status_set (GXK_STATUS_DONE, "Insert Part", NULL);
	  else
	    gxk_status_set (GXK_STATUS_ERROR, "Insert Part", "Lost Part");
	  drag->state = BST_DRAG_HANDLED;
	}
      else
	gxk_status_set (GXK_STATUS_ERROR, "Insert Part", "Position taken");
    }
  else
    {
      if (self->obj_part)
	gxk_status_set (GXK_STATUS_ERROR, "Insert Part", "Position taken");
      else
	gxk_status_set (GXK_STATUS_ERROR, "Insert part", "No Track");
      drag->state = BST_DRAG_HANDLED;
    }
}

static void
delete_start (BstTrackRollController *self,
	      BstTrackRollDrag       *drag)
{
  if (self->obj_part)	/* got part to delete */
    {
      bse_track_remove_tick (self->obj_track, self->obj_tick);
      gxk_status_set (GXK_STATUS_WAIT, "Delete Part", NULL);
    }
  else
    gxk_status_set (GXK_STATUS_ERROR, "Delete Part", "No target");
  drag->state = BST_DRAG_HANDLED;
}

static void
move_start (BstTrackRollController *self,
	    BstTrackRollDrag       *drag)
{
  if (self->obj_part)	/* got part to move */
    {
      self->xoffset = drag->start_tick - self->obj_tick;	/* drag offset */
      controller_update_canvas_cursor (self, BST_TRACK_ROLL_TOOL_MOVE);
      gxk_status_set (GXK_STATUS_WAIT, "Move Part", NULL);
      drag->state = BST_DRAG_CONTINUE;
    }
  else
    {
      gxk_status_set (GXK_STATUS_ERROR, "Move Part", "No target");
      drag->state = BST_DRAG_HANDLED;
    }
}

static void
move_motion (BstTrackRollController *self,
	     BstTrackRollDrag       *drag)
{
  gint new_tick;
  gboolean track_changed;

  new_tick = MAX (drag->current_tick, self->xoffset) - self->xoffset;
  new_tick = bst_track_roll_controller_quantize (self, new_tick);
  track_changed = self->obj_track != drag->current_track;
  if (new_tick != self->obj_tick || self->obj_track != drag->current_track)
    {
      BseErrorType error = bse_track_insert_part (drag->current_track, new_tick, self->obj_part);
      if (error == BSE_ERROR_NONE)
	{
	  bse_track_remove_tick (self->obj_track, self->obj_tick);
	  self->obj_track = drag->current_track;
	  self->obj_tick = new_tick;
	}
      /* else gxk_status_set (GXK_STATUS_ERROR, "Move Part", bse_error_blurb (error)); */
    }
}

static void
move_abort (BstTrackRollController *self,
	    BstTrackRollDrag       *drag)
{
  gxk_status_set (GXK_STATUS_ERROR, "Move Part", "Lost Part");
}

static void
editor_once (BstTrackRollController *self,
	     BstTrackRollDrag       *drag)
{
  if (self->obj_part)	/* got part */
    {
      GtkWidget *pdialog = g_object_new (BST_TYPE_PART_DIALOG, NULL);
      bst_part_dialog_set_proxy (BST_PART_DIALOG (pdialog), self->obj_part);
      g_signal_connect_object (self->troll, "destroy", G_CALLBACK (gtk_widget_destroy), pdialog, G_CONNECT_SWAPPED);
      gxk_status_set (GXK_STATUS_DONE, "Start Editor", NULL);
      if (self->canvas_reset)
	self->canvas_reset (self->canvas_reset_data);
      gtk_widget_show (pdialog);
    }
  else
    gxk_status_set (GXK_STATUS_ERROR, "Start Editor", "No target");
  drag->state = BST_DRAG_HANDLED;
}

static void
pointer_move (BstTrackRollController *self,
	      BstTrackRollDrag       *drag)
{
  if (self->song &&
      drag->type != BST_DRAG_DONE) /* skip release events */
    {
      guint tick = bst_track_roll_controller_quantize (self, drag->current_tick);
      bse_proxy_set (self->song, "tick-pointer", tick, NULL);
      drag->state = BST_DRAG_CONTINUE;
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
      drag->state = BST_DRAG_CONTINUE;
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
      drag->state = BST_DRAG_CONTINUE;
    }
}

typedef void (*DragFunc) (BstTrackRollController *,
			  BstTrackRollDrag       *);

void
controller_drag (BstTrackRollController *self,
		 BstTrackRollDrag       *drag)
{
  typedef struct {
    BstTrackRollTool tool;
    DragFunc start, motion, abort;
  } TrackRollTool;
  static const TrackRollTool canvas_tool_table[] = {
    { BST_TRACK_ROLL_TOOL_INSERT,	insert_start,		NULL,		NULL,		},
    { BST_TRACK_ROLL_TOOL_MOVE,		move_start,		move_motion,	move_abort,	},
    { BST_TRACK_ROLL_TOOL_DELETE,	delete_start,		NULL,		NULL,		},
    { BST_TRACK_ROLL_TOOL_EDIT_NAME,	edit_name_start,	NULL,		NULL,		},
    { BST_TRACK_ROLL_TOOL_EDITOR_ONCE,	editor_once,		NULL,		NULL,		},
  };
  static const TrackRollTool hpanel_tool_table[] = {
    { BST_TRACK_ROLL_TOOL_MOVE_TICK_POINTER,	pointer_move,	 pointer_move,	  NULL,		},
    { BST_TRACK_ROLL_TOOL_MOVE_TICK_LEFT,	tick_left_move,	 tick_left_move,  NULL,		},
    { BST_TRACK_ROLL_TOOL_MOVE_TICK_RIGHT,	tick_right_move, tick_right_move, NULL,		},
  };
  const TrackRollTool *tool_table;
  BstTrackRollTool obj_tool, tool;
  guint i, n_tools;

  /* area specific handling */
  if (drag->canvas_drag)
    {
      tool_table = canvas_tool_table;
      n_tools = G_N_ELEMENTS (canvas_tool_table);
      obj_tool = self->object_tool[CLAMP (drag->button, 1, 3) - 1];
      tool = self->canvas_tool[CLAMP (drag->button, 1, 3) - 1];
    }
  else if (drag->hpanel_drag)
    {
      tool_table = hpanel_tool_table;
      n_tools = G_N_ELEMENTS (hpanel_tool_table);
      tool = self->hpanel_tool[CLAMP (drag->button, 1, 3) - 1];
      obj_tool = tool;
    }
  else
    return;

  /* initial drag handling */
  if (drag->type == BST_DRAG_START)
    {
      BseTrackPartSeq *tps;
      BseTrackPart *tpart = NULL;
      /* setup drag data */
      if (!drag->start_valid)
	drag->start_track = 0;
      tps = drag->start_track ? bse_track_list_parts (drag->start_track) : NULL;
      if (tps && tps->n_tparts)
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
	  break;
      self->tool_index = i;
    }

  /* generic drag handling */
  i = self->tool_index;
  if (i >= n_tools)
    return;	/* unhandled */
  switch (drag->type)
    {
    case BST_DRAG_START:
      if (tool_table[i].start)
	tool_table[i].start (self, drag);
      break;
    case BST_DRAG_MOTION:
    case BST_DRAG_DONE:
      if (tool_table[i].motion)
	tool_table[i].motion (self, drag);
      break;
    case BST_DRAG_ABORT:
      if (tool_table[i].abort)
	tool_table[i].abort (self, drag);
      break;
    }
  if (drag->type == BST_DRAG_DONE || drag->type == BST_DRAG_ABORT)
    {
      controller_update_canvas_cursor (self, self->canvas_tool[0]);
      controller_update_hpanel_cursor (self, self->hpanel_tool[0]);
    }
}
