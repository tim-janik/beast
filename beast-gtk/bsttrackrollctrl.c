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
static void	controller_canvas_drag		(BstTrackRollController	*self,
						 BstTrackRollDrag	*drag);
static void	controller_update_cursor	(BstTrackRollController *self,
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
  g_signal_connect_data (troll, "canvas-drag",
			 G_CALLBACK (controller_canvas_drag),
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
bst_track_roll_controller_reset_handler (BstTrackRollController   *self,
					 void (*handler) (gpointer data),
					 gpointer                  data)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->ref_count >= 1);

  self->reset = handler;
  self->reset_data = self->reset ? data : NULL;
}

void
bst_track_roll_controller_set_obj_tools (BstTrackRollController *self,
					 BstTrackRollTool        tool1,
					 BstTrackRollTool        tool2,
					 BstTrackRollTool        tool3)
{
  g_return_if_fail (self != NULL);

  self->obj_tool1 = tool1;
  self->obj_tool2 = tool2;
  self->obj_tool3 = tool3;
}

void
bst_track_roll_controller_set_bg_tools (BstTrackRollController *self,
					BstTrackRollTool        tool1,
					BstTrackRollTool        tool2,
					BstTrackRollTool        tool3)
{
  g_return_if_fail (self != NULL);

  self->bg_tool1 = tool1;
  self->bg_tool2 = tool2;
  self->bg_tool3 = tool3;
  controller_update_cursor (self, self->bg_tool1);
}

static void
controller_update_cursor (BstTrackRollController *self,
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
      controller_update_cursor (self, BST_TRACK_ROLL_TOOL_EDIT_NAME);
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
  controller_update_cursor (self, self->bg_tool1);
}

static void
insert_start (BstTrackRollController *self,
	      BstTrackRollDrag       *drag)
{
  if (drag->current_track && drag->current_valid && !self->obj_part)
    {
      SfiProxy song = bse_item_get_parent (drag->current_track);
      SfiProxy item = bse_song_create_part (song);
      if (item && bse_track_insert_part (drag->current_track, drag->current_tick, item) == BSE_ERROR_NONE)
	gxk_status_set (GXK_STATUS_DONE, "Insert Part", NULL);
      else
	gxk_status_set (GXK_STATUS_ERROR, "Insert Part", "Lost Part");
      drag->state = BST_DRAG_HANDLED;
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
      controller_update_cursor (self, BST_TRACK_ROLL_TOOL_MOVE);
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
  /* FIXME: new_tick = bst_track_roll_quantize (drag->troll, new_tick); */
  track_changed = self->obj_track != drag->current_track;
  if (new_tick != self->obj_tick || self->obj_track != drag->current_track)
    {
      if (bse_track_insert_part (drag->current_track, new_tick, self->obj_part) == BSE_ERROR_NONE)
	{
	  bse_track_remove_tick (self->obj_track, self->obj_tick);
	  self->obj_track = drag->current_track;
	  self->obj_tick = new_tick;
	  if (0)
	    drag->state = BST_DRAG_ERROR;
	}
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
      if (self->reset)
	self->reset (self->reset_data);
      gtk_widget_show (pdialog);
    }
  else
    gxk_status_set (GXK_STATUS_ERROR, "Start Editor", "No target");
  drag->state = BST_DRAG_HANDLED;
}

typedef void (*DragFunc) (BstTrackRollController *,
			  BstTrackRollDrag       *);

void
controller_canvas_drag (BstTrackRollController *self,
			BstTrackRollDrag       *drag)
{
  static struct {
    BstTrackRollTool tool;
    DragFunc start, motion, abort;
  } tool_table[] = {
    { BST_TRACK_ROLL_TOOL_INSERT,      insert_start,	NULL,		NULL,		},
    { BST_TRACK_ROLL_TOOL_MOVE,	       move_start,	move_motion,	move_abort,	},
    { BST_TRACK_ROLL_TOOL_DELETE,      delete_start,	NULL,		NULL,		},
    { BST_TRACK_ROLL_TOOL_EDIT_NAME,   edit_name_start,	NULL,		NULL,		},
    { BST_TRACK_ROLL_TOOL_EDITOR_ONCE, editor_once,	NULL,		NULL,		},
  };
  guint i;

  if (drag->type == BST_DRAG_START)
    {
      BstTrackRollTool tool = BST_TRACK_ROLL_TOOL_NONE;
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
      tool = BST_TRACK_ROLL_TOOL_NONE;
      if (self->obj_part)		/* have object */
	switch (drag->button)
	  {
	  case 1:	tool = self->obj_tool1;	break;
	  case 2:	tool = self->obj_tool2;	break;
	  case 3:	tool = self->obj_tool3;	break;
	  }
      else
	switch (drag->button)
	  {
	  case 1:	tool = self->bg_tool1;	break;
	  case 2:	tool = self->bg_tool2;	break;
	  case 3:	tool = self->bg_tool3;	break;
	  }
      for (i = 0; i < G_N_ELEMENTS (tool_table); i++)
	if (tool_table[i].tool == tool)
	  break;
      self->tool_index = i;
      if (i >= G_N_ELEMENTS (tool_table))
	return;		/* unhandled */
    }
  i = self->tool_index;
  g_return_if_fail (i < G_N_ELEMENTS (tool_table));
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
  if (drag->type == BST_DRAG_DONE ||
      drag->type == BST_DRAG_ABORT)
    controller_update_cursor (self, self->bg_tool1);
}
