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


#define QUANTIZATION(self)	((self)->quant_rtools->tool_id)
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
  self->canvas_rtools = bst_radio_tools_new ();
  g_object_connect (self->canvas_rtools,
		    "swapped_signal::set_tool", controller_update_canvas_cursor, self,
		    NULL);
  {
    BstTool radio_tools[] = {
      { CKEY ("TrackRoll/Insert"),	BST_TRACK_ROLL_TOOL_INSERT,	BST_RADIO_TOOLS_DEFAULT },
      { CKEY ("TrackRoll/Link"),	BST_TRACK_ROLL_TOOL_LINK,	BST_RADIO_TOOLS_DEFAULT },
      { CKEY ("TrackRoll/Delete"),	BST_TRACK_ROLL_TOOL_DELETE,	BST_RADIO_TOOLS_DEFAULT },
      { CKEY ("TrackRoll/Editor"),	BST_TRACK_ROLL_TOOL_EDITOR_ONCE, BST_RADIO_TOOLS_DEFAULT },
    };
    bst_radio_tools_add_tools (self->canvas_rtools, G_N_ELEMENTS (radio_tools), radio_tools);
    bst_radio_tools_set_tool (self->canvas_rtools, BST_TRACK_ROLL_TOOL_INSERT);
  }
  /* register hpanel tools */
  self->hpanel_rtools = bst_radio_tools_new ();
  g_object_connect (self->hpanel_rtools,
		    "swapped_signal::set_tool", controller_update_hpanel_cursor, self,
		    NULL);
  {
    BstTool radio_tools[] = {
      { CKEY ("TrackRoll/TickLeft"),	BST_TRACK_ROLL_TOOL_MOVE_TICK_LEFT,	BST_RADIO_TOOLS_DEFAULT },
      { CKEY ("TrackRoll/TickPos"),	BST_TRACK_ROLL_TOOL_MOVE_TICK_POINTER,	BST_RADIO_TOOLS_DEFAULT },
      { CKEY ("TrackRoll/TickRight"),	BST_TRACK_ROLL_TOOL_MOVE_TICK_RIGHT,	BST_RADIO_TOOLS_DEFAULT },
    };
    bst_radio_tools_add_tools (self->hpanel_rtools, G_N_ELEMENTS (radio_tools), radio_tools);
    bst_radio_tools_set_tool (self->hpanel_rtools, BST_TRACK_ROLL_TOOL_MOVE_TICK_POINTER);
  }
  /* register quantization tools */
  self->quant_rtools = bst_radio_tools_new ();
  {
    BstTool radio_tools[] = {
      { CKEY ("Quant/Tact"),	BST_QUANTIZE_TACT,	BST_RADIO_TOOLS_DEFAULT },
      { CKEY ("Quant/None"),	BST_QUANTIZE_NONE,	BST_RADIO_TOOLS_DEFAULT },
      { CKEY ("Quant/1"),	BST_QUANTIZE_NOTE_1,	BST_RADIO_TOOLS_DEFAULT },
      { CKEY ("Quant/2"),	BST_QUANTIZE_NOTE_2,	BST_RADIO_TOOLS_DEFAULT },
      { CKEY ("Quant/4"),	BST_QUANTIZE_NOTE_4,	BST_RADIO_TOOLS_DEFAULT },
      { CKEY ("Quant/8"),	BST_QUANTIZE_NOTE_8,	BST_RADIO_TOOLS_DEFAULT },
      { CKEY ("Quant/16"),	BST_QUANTIZE_NOTE_16,	BST_RADIO_TOOLS_DEFAULT },
    };
    bst_radio_tools_add_tools (self->quant_rtools, G_N_ELEMENTS (radio_tools), radio_tools);
    bst_radio_tools_set_tool (self->quant_rtools, BST_QUANTIZE_TACT);
  }

  return self;
}

static BstTrackRollTool
hpanel_button_tool (BstTrackRollController *self,
		    guint                   button)
{
  switch (self->hpanel_rtools->tool_id)	/* user selected tool */
    {
    case BST_TRACK_ROLL_TOOL_MOVE_TICK_LEFT:
      switch (button)
	{
	case 1:	 return BST_TRACK_ROLL_TOOL_MOVE_TICK_LEFT;
	case 2:	 return BST_TRACK_ROLL_TOOL_MOVE_TICK_POINTER;
	default: return BST_TRACK_ROLL_TOOL_NONE;
	}
    case BST_TRACK_ROLL_TOOL_MOVE_TICK_POINTER:
      switch (button)
	{
	case 1:	 return BST_TRACK_ROLL_TOOL_MOVE_TICK_POINTER;
	case 2:	 return BST_TRACK_ROLL_TOOL_MOVE_TICK_POINTER;
	default: return BST_TRACK_ROLL_TOOL_NONE;
	}
    case BST_TRACK_ROLL_TOOL_MOVE_TICK_RIGHT:
      switch (button)
	{
	case 1:	 return BST_TRACK_ROLL_TOOL_MOVE_TICK_RIGHT;
	case 2:	 return BST_TRACK_ROLL_TOOL_MOVE_TICK_POINTER;
	default: return BST_TRACK_ROLL_TOOL_NONE;
	}
    }
  return BST_TRACK_ROLL_TOOL_NONE;
}

static BstTrackRollTool
canvas_button_tool (BstTrackRollController *self,
		    guint                   button,
		    guint                   have_object)
{
  switch (self->canvas_rtools->tool_id | /* user selected tool */
	  (have_object ? HAVE_OBJECT : 0))
    {
    case BST_TRACK_ROLL_TOOL_INSERT | HAVE_OBJECT:
      switch (button)
	{
	case 1:  return BST_TRACK_ROLL_TOOL_EDIT_NAME;
	case 2:  return BST_TRACK_ROLL_TOOL_MOVE;
	default: return BST_TRACK_ROLL_TOOL_NONE;
	}
    case BST_TRACK_ROLL_TOOL_INSERT:
      switch (button)
	{
	case 1:  return BST_TRACK_ROLL_TOOL_INSERT;
	case 2:  return BST_TRACK_ROLL_TOOL_MOVE;	/* error */
	default: return BST_TRACK_ROLL_TOOL_NONE;
	}
    case BST_TRACK_ROLL_TOOL_LINK | HAVE_OBJECT:
      switch (button)
	{
	case 1:  return BST_TRACK_ROLL_TOOL_LINK;
	case 2:  return BST_TRACK_ROLL_TOOL_MOVE;
	default: return BST_TRACK_ROLL_TOOL_NONE;
	}
    case BST_TRACK_ROLL_TOOL_LINK:
      switch (button)
	{
	case 1:  return BST_TRACK_ROLL_TOOL_LINK;
	case 2:  return BST_TRACK_ROLL_TOOL_MOVE;	/* error */
	default: return BST_TRACK_ROLL_TOOL_NONE;
	}
    case BST_TRACK_ROLL_TOOL_EDIT_NAME | HAVE_OBJECT:
      switch (button)
	{
	case 1:  return BST_TRACK_ROLL_TOOL_EDIT_NAME;
	case 2:  return BST_TRACK_ROLL_TOOL_MOVE;
	default: return BST_TRACK_ROLL_TOOL_NONE;
	}
    case BST_TRACK_ROLL_TOOL_EDIT_NAME:
      switch (button)
	{
	case 1:  return BST_TRACK_ROLL_TOOL_EDIT_NAME;	/* error */
	case 2:  return BST_TRACK_ROLL_TOOL_MOVE;	/* error */
	default: return BST_TRACK_ROLL_TOOL_NONE;
	}
    case BST_TRACK_ROLL_TOOL_DELETE | HAVE_OBJECT:
      switch (button)
	{
	case 1:  return BST_TRACK_ROLL_TOOL_DELETE;
	case 2:  return BST_TRACK_ROLL_TOOL_MOVE;
	default: return BST_TRACK_ROLL_TOOL_NONE;
	}
    case BST_TRACK_ROLL_TOOL_DELETE:
      switch (button)
	{
	case 1:  return BST_TRACK_ROLL_TOOL_DELETE;	/* error */
	case 2:  return BST_TRACK_ROLL_TOOL_MOVE;	/* error */
	default: return BST_TRACK_ROLL_TOOL_NONE;
	}
    case BST_TRACK_ROLL_TOOL_EDITOR_ONCE | HAVE_OBJECT:
      switch (button)
	{
	case 1:  return BST_TRACK_ROLL_TOOL_EDITOR_ONCE;
	case 2:  return BST_TRACK_ROLL_TOOL_MOVE;
	default: return BST_TRACK_ROLL_TOOL_NONE;
	}
    case BST_TRACK_ROLL_TOOL_EDITOR_ONCE:
      switch (button)
	{
	case 1:  return BST_TRACK_ROLL_TOOL_EDITOR_ONCE;	/* error */
	case 2:  return BST_TRACK_ROLL_TOOL_MOVE;		/* error */
	default: return BST_TRACK_ROLL_TOOL_NONE;
	}
      break;
    }
  return BST_TRACK_ROLL_TOOL_NONE;
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
      bst_radio_tools_destroy (self->canvas_rtools);
      g_object_unref (self->canvas_rtools);
      bst_radio_tools_destroy (self->hpanel_rtools);
      g_object_unref (self->hpanel_rtools);
      bst_radio_tools_destroy (self->quant_rtools);
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

void
bst_track_roll_controller_set_quantization (BstTrackRollController *self,
					    BstQuantizationType     quantization)
{
  g_return_if_fail (self != NULL);

  switch (quantization)
    {
    case BST_QUANTIZE_TACT:
    case BST_QUANTIZE_NOTE_1:
    case BST_QUANTIZE_NOTE_2:
    case BST_QUANTIZE_NOTE_4:
    case BST_QUANTIZE_NOTE_8:
    case BST_QUANTIZE_NOTE_16:
    case BST_QUANTIZE_NOTE_32:
    case BST_QUANTIZE_NOTE_64:
    case BST_QUANTIZE_NOTE_128:
      bst_radio_tools_set_tool (self->quant_rtools, quantization);
      break;
    default:
      bst_radio_tools_set_tool (self->quant_rtools, BST_QUANTIZE_NONE);
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
  switch (tool_id)
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
				 guint                  tool_id)
{
  switch (tool_id)
    {
    case BST_TRACK_ROLL_TOOL_INSERT:
      bst_track_roll_set_canvas_cursor (self->troll, GDK_PENCIL);
      break;
    case BST_TRACK_ROLL_TOOL_LINK:
      bst_track_roll_set_canvas_cursor (self->troll, GDK_DIAMOND_CROSS);
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
  controller_update_canvas_cursor (self, self->canvas_rtools->tool_id);
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
move_link_start (BstTrackRollController *self,
		 BstTrackRollDrag       *drag,
		 gboolean		 link_pending)
{
  const gchar *action = link_pending ? "Link Part" : "Move Part";
  if (self->obj_part)	/* got part to move */
    {
      self->xoffset = drag->start_tick - self->obj_tick;	/* drag offset */
      controller_update_canvas_cursor (self, link_pending ? BST_TRACK_ROLL_TOOL_LINK : BST_TRACK_ROLL_TOOL_MOVE);
      gxk_status_set (GXK_STATUS_WAIT, action, NULL);
      drag->state = BST_DRAG_CONTINUE;
      self->skip_deletion = link_pending;
    }
  else
    {
      gxk_status_set (GXK_STATUS_ERROR, action, "No target");
      drag->state = BST_DRAG_HANDLED;
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
	  if (!self->skip_deletion)
	    bse_track_remove_tick (self->obj_track, self->obj_tick);
	  else
	    {
	      self->skip_deletion = FALSE;
	      controller_update_canvas_cursor (self, BST_TRACK_ROLL_TOOL_MOVE);
	    }
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
      gtk_widget_show (pdialog);
      bst_radio_tools_set_tool (self->canvas_rtools, BST_TRACK_ROLL_TOOL_INSERT);
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
struct _BstTrackRollUtil
{
  BstTrackRollTool tool;
  DragFunc start, motion, abort;
};

void
controller_drag (BstTrackRollController *self,
		 BstTrackRollDrag       *drag)
{
  static const BstTrackRollUtil canvas_tool_table[] = {
    { BST_TRACK_ROLL_TOOL_INSERT,	insert_start,		NULL,		NULL,		},
    { BST_TRACK_ROLL_TOOL_MOVE,		move_start,		move_motion,	move_abort,	},
    { BST_TRACK_ROLL_TOOL_LINK,		link_start,		move_motion,	move_abort,	},
    { BST_TRACK_ROLL_TOOL_DELETE,	delete_start,		NULL,		NULL,		},
    { BST_TRACK_ROLL_TOOL_EDIT_NAME,	edit_name_start,	NULL,		NULL,		},
    { BST_TRACK_ROLL_TOOL_EDITOR_ONCE,	editor_once,		NULL,		NULL,		},
  };
  static const BstTrackRollUtil hpanel_tool_table[] = {
    { BST_TRACK_ROLL_TOOL_MOVE_TICK_POINTER,	pointer_move,	 pointer_move,	  NULL,		},
    { BST_TRACK_ROLL_TOOL_MOVE_TICK_LEFT,	tick_left_move,	 tick_left_move,  NULL,		},
    { BST_TRACK_ROLL_TOOL_MOVE_TICK_RIGHT,	tick_right_move, tick_right_move, NULL,		},
  };

  /* initial drag handling */
  if (drag->type == BST_DRAG_START)
    {
      BstTrackRollTool obj_tool, tool;
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
      else if (drag->hpanel_drag)
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
    case BST_DRAG_START:
      if (self->current_tool->start)
	self->current_tool->start (self, drag);
      break;
    case BST_DRAG_MOTION:
    case BST_DRAG_DONE:
      if (self->current_tool->motion)
	self->current_tool->motion (self, drag);
      break;
    case BST_DRAG_ABORT:
      if (self->current_tool->abort)
	self->current_tool->abort (self, drag);
      break;
    }
  if (drag->type == BST_DRAG_DONE || drag->type == BST_DRAG_ABORT)
    {
      self->current_tool = NULL;
      controller_update_canvas_cursor (self, self->canvas_rtools->tool_id);
      controller_update_hpanel_cursor (self, self->hpanel_rtools->tool_id);
    }
}
