/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002-2003 Tim Janik
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
#include "bstpianorollctrl.h"
#include "bsteventrollctrl.h"


#define NOTE_LENGTH(self)       ((self)->note_rtools->tool_id)
#define QUANTIZATION(self)      ((self)->quant_rtools->tool_id)
#define HAVE_OBJECT             (1 << 31)


/* --- prototypes --- */
static void	controller_canvas_drag		(BstPianoRollController	*self,
						 BstPianoRollDrag	*drag);
static void	controller_piano_drag		(BstPianoRollController	*self,
						 BstPianoRollDrag	*drag);
static void	controller_update_canvas_cursor	(BstPianoRollController *self,
						 BstGenericRollTool	 tool);


/* --- variables --- */
static BsePartNoteSeq *clipboard_pseq = NULL;


/* --- functions --- */
GxkActionList*
bst_piano_roll_controller_canvas_actions (BstPianoRollController *self)
{
  GxkActionList *alist = gxk_action_list_create_grouped (self->canvas_atools);
  static const GxkStockAction piano_canvas_tools[] = {
    { N_("Insert"),           "I",    N_("Insert/resize/move notes (mouse button 1 and 2)"),
      BST_GENERIC_ROLL_TOOL_INSERT,   BST_STOCK_PART_TOOL },
    { N_("Delete"),           "D",    N_("Delete note (mouse button 1)"),
      BST_GENERIC_ROLL_TOOL_DELETE,   BST_STOCK_TRASHCAN },
    { N_("Align Events"),     "A",    N_("Draw a line to align events to"),
      BST_GENERIC_ROLL_TOOL_ALIGN,    BST_STOCK_EVENT_CONTROL },
    { N_("Select"),           "S",    N_("Rectangle select notes"),
      BST_GENERIC_ROLL_TOOL_SELECT,   BST_STOCK_RECT_SELECT },
    { N_("Vertical Select"),  "V",    N_("Select tick range vertically"),
      BST_GENERIC_ROLL_TOOL_VSELECT,  BST_STOCK_VERT_SELECT },
  };
  gxk_action_list_add_actions (alist,
                               G_N_ELEMENTS (piano_canvas_tools), piano_canvas_tools,
                               NULL /*i18n_domain*/, NULL /*acheck*/, NULL /*aexec*/, NULL);
  return alist;
}

void
bst_piano_roll_controller_set_clipboard (BsePartNoteSeq *pseq)
{
  if (clipboard_pseq)
    bse_part_note_seq_free (clipboard_pseq);
  clipboard_pseq = pseq && pseq->n_pnotes ? bse_part_note_seq_copy_shallow (pseq) : NULL;
  if (clipboard_pseq)
    bst_event_roll_controller_set_clipboard (NULL);
}

BsePartNoteSeq*
bst_piano_roll_controller_get_clipboard (void)
{
  return clipboard_pseq;
}

static void
controller_reset_canvas_cursor (BstPianoRollController *self)
{
  controller_update_canvas_cursor (self, self->canvas_atools->action_id);
}

BstPianoRollController*
bst_piano_roll_controller_new (BstPianoRoll *proll)
{
  BstPianoRollController *self;

  g_return_val_if_fail (BST_IS_PIANO_ROLL (proll), NULL);

  self = g_new0 (BstPianoRollController, 1);
  self->proll = proll;
  self->ref_count = 1;

  self->ref_count++;
  g_signal_connect_data (proll, "canvas-drag",
			 G_CALLBACK (controller_canvas_drag),
			 self, (GClosureNotify) bst_piano_roll_controller_unref,
			 G_CONNECT_SWAPPED);
  g_signal_connect_data (proll, "piano-drag",
			 G_CALLBACK (controller_piano_drag),
			 self, NULL,
			 G_CONNECT_SWAPPED);
  /* canvas tool default */
  self->canvas_atools = gxk_action_group_new ();
  gxk_action_group_select (self->canvas_atools, BST_GENERIC_ROLL_TOOL_INSERT);
  /* register note length tools */
  self->note_rtools = bst_radio_tools_new ();
  {
    BstTool radio_tools[] = {
      { CKEY ("Note/1"),                1,    BST_RADIO_TOOLS_EVERYWHERE },
      { CKEY ("Note/2"),                2,    BST_RADIO_TOOLS_EVERYWHERE },
      { CKEY ("Note/4"),                4,    BST_RADIO_TOOLS_EVERYWHERE },
      { CKEY ("Note/8"),                8,    BST_RADIO_TOOLS_EVERYWHERE },
      { CKEY ("Note/16"),               16,   BST_RADIO_TOOLS_EVERYWHERE },
    };
    bst_radio_tools_add_tools (self->note_rtools, G_N_ELEMENTS (radio_tools), radio_tools);
    bst_radio_tools_set_tool (self->note_rtools, 4);
  }
  /* register quantization tools */
  self->quant_rtools = bst_radio_tools_new ();
  {
    BstTool radio_tools[] = {
      { CKEY ("Quant/None"),    BST_QUANTIZE_NONE,      BST_RADIO_TOOLS_EVERYWHERE },
      { CKEY ("Quant/1"),       BST_QUANTIZE_NOTE_1,    BST_RADIO_TOOLS_EVERYWHERE },
      { CKEY ("Quant/2"),       BST_QUANTIZE_NOTE_2,    BST_RADIO_TOOLS_EVERYWHERE },
      { CKEY ("Quant/4"),       BST_QUANTIZE_NOTE_4,    BST_RADIO_TOOLS_EVERYWHERE },
      { CKEY ("Quant/8"),       BST_QUANTIZE_NOTE_8,    BST_RADIO_TOOLS_EVERYWHERE },
      { CKEY ("Quant/16"),      BST_QUANTIZE_NOTE_16,   BST_RADIO_TOOLS_EVERYWHERE },
    };
    bst_radio_tools_add_tools (self->quant_rtools, G_N_ELEMENTS (radio_tools), radio_tools);
    bst_radio_tools_set_tool (self->quant_rtools, BST_QUANTIZE_NOTE_8);
  }
  /* update from rtools */
  g_signal_connect_swapped (self->canvas_atools, "changed",
                            G_CALLBACK (controller_reset_canvas_cursor), self);
  controller_reset_canvas_cursor (self);
  return self;
}

BstPianoRollController*
bst_piano_roll_controller_ref (BstPianoRollController *self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (self->ref_count >= 1, NULL);

  self->ref_count++;

  return self;
}

void
bst_piano_roll_controller_unref (BstPianoRollController *self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->ref_count >= 1);

  self->ref_count--;
  if (!self->ref_count)
    {
      gxk_action_group_dispose (self->canvas_atools);
      g_object_unref (self->canvas_atools);
      bst_radio_tools_dispose (self->note_rtools);
      g_object_unref (self->note_rtools);
      bst_radio_tools_dispose (self->quant_rtools);
      g_object_unref (self->quant_rtools);
      g_free (self);
    }
}

static BstGenericRollTool
piano_canvas_button_tool (BstPianoRollController *self,
                          guint                   button,
                          guint                   have_object)
{
  switch (self->canvas_atools->action_id | /* user selected tool */
          (have_object ? HAVE_OBJECT : 0))
    {
    case BST_GENERIC_ROLL_TOOL_INSERT: /* background */
      switch (button) {
      case 1:  return BST_GENERIC_ROLL_TOOL_INSERT;
      case 2:  return BST_GENERIC_ROLL_TOOL_MOVE;         /* user error */
      default: return BST_GENERIC_ROLL_TOOL_NONE;
      }
    case BST_GENERIC_ROLL_TOOL_INSERT | HAVE_OBJECT:
      switch (button) {
      case 1:  return BST_GENERIC_ROLL_TOOL_RESIZE;
      case 2:  return BST_GENERIC_ROLL_TOOL_MOVE;
      default: return BST_GENERIC_ROLL_TOOL_NONE;
      }
    case BST_GENERIC_ROLL_TOOL_DELETE: /* background */
      switch (button) {
      case 1:  return BST_GENERIC_ROLL_TOOL_DELETE;       /* user error */
      case 2:  return BST_GENERIC_ROLL_TOOL_MOVE;         /* user error */
      default: return BST_GENERIC_ROLL_TOOL_NONE;
      }
    case BST_GENERIC_ROLL_TOOL_DELETE | HAVE_OBJECT:
      switch (button) {
      case 1:  return BST_GENERIC_ROLL_TOOL_DELETE;
      case 2:  return BST_GENERIC_ROLL_TOOL_MOVE;
      default: return BST_GENERIC_ROLL_TOOL_NONE;
      }
    case BST_GENERIC_ROLL_TOOL_ALIGN: /* background */
      switch (button) {
      case 1:  return BST_GENERIC_ROLL_TOOL_ALIGN;
      case 2:  return BST_GENERIC_ROLL_TOOL_MOVE;         /* user error */
      default: return BST_GENERIC_ROLL_TOOL_NONE;
      }
    case BST_GENERIC_ROLL_TOOL_ALIGN | HAVE_OBJECT:
      switch (button) {
      case 1:  return BST_GENERIC_ROLL_TOOL_ALIGN;
      case 2:  return BST_GENERIC_ROLL_TOOL_MOVE;
      default: return BST_GENERIC_ROLL_TOOL_NONE;
      }
    case BST_GENERIC_ROLL_TOOL_SELECT: /* background */
      switch (button) {
      case 1:  return BST_GENERIC_ROLL_TOOL_SELECT;
      case 2:  return BST_GENERIC_ROLL_TOOL_MOVE;         /* user error */
      default: return BST_GENERIC_ROLL_TOOL_NONE;
      }
    case BST_GENERIC_ROLL_TOOL_SELECT | HAVE_OBJECT:
      switch (button) {
      case 1:  return BST_GENERIC_ROLL_TOOL_SELECT;
      case 2:  return BST_GENERIC_ROLL_TOOL_MOVE;
      default: return BST_GENERIC_ROLL_TOOL_NONE;
      }
    case BST_GENERIC_ROLL_TOOL_VSELECT: /* background */
      switch (button) {
      case 1:  return BST_GENERIC_ROLL_TOOL_VSELECT;
      case 2:  return BST_GENERIC_ROLL_TOOL_MOVE;         /* user error */
      default: return BST_GENERIC_ROLL_TOOL_NONE;
      }
    case BST_GENERIC_ROLL_TOOL_VSELECT | HAVE_OBJECT:
      switch (button) {
      case 1:  return BST_GENERIC_ROLL_TOOL_VSELECT;
      case 2:  return BST_GENERIC_ROLL_TOOL_MOVE;
      default: return BST_GENERIC_ROLL_TOOL_NONE;
      }
    }
  return BST_GENERIC_ROLL_TOOL_NONE;
}

void
bst_piano_roll_controller_clear (BstPianoRollController *self)
{
  BsePartNoteSeq *pseq;
  SfiProxy proxy;
  guint i;

  g_return_if_fail (self != NULL);

  proxy = self->proll->proxy;
  pseq = bse_part_list_selected_notes (proxy);
  bse_item_group_undo (proxy, "Clear Selection");
  for (i = 0; i < pseq->n_pnotes; i++)
    {
      BsePartNote *pnote = pseq->pnotes[i];
      bse_part_delete_event (proxy, pnote->id);
    }
  bse_item_ungroup_undo (proxy);
}

void
bst_piano_roll_controller_cut (BstPianoRollController *self)
{
  BsePartNoteSeq *pseq;
  SfiProxy proxy;
  guint i;

  g_return_if_fail (self != NULL);

  proxy = self->proll->proxy;
  pseq = bse_part_list_selected_notes (proxy);
  bse_item_group_undo (proxy, "Cut Selection");
  for (i = 0; i < pseq->n_pnotes; i++)
    {
      BsePartNote *pnote = pseq->pnotes[i];
      bse_part_delete_event (proxy, pnote->id);
    }
  bst_piano_roll_controller_set_clipboard (pseq);
  bse_item_ungroup_undo (proxy);
}

gboolean
bst_piano_roll_controller_copy (BstPianoRollController *self)
{
  BsePartNoteSeq *pseq;
  SfiProxy proxy;

  g_return_val_if_fail (self != NULL, FALSE);

  proxy = self->proll->proxy;
  pseq = bse_part_list_selected_notes (proxy);
  bst_piano_roll_controller_set_clipboard (pseq);
  return pseq && pseq->n_pnotes;
}

void
bst_piano_roll_controller_paste (BstPianoRollController *self)
{
  BsePartNoteSeq *pseq;
  SfiProxy proxy;

  g_return_if_fail (self != NULL);

  proxy = self->proll->proxy;
  pseq = bst_piano_roll_controller_get_clipboard ();
  if (pseq)
    {
      guint i, paste_tick, ctick = self->proll->max_ticks;
      gint cnote = 0;
      gint paste_note;
      bse_item_group_undo (proxy, "Paste Clipboard");
      bse_part_deselect_notes (proxy, 0, self->proll->max_ticks, self->proll->min_note, self->proll->max_note);
      bst_piano_roll_get_paste_pos (self->proll, &paste_tick, &paste_note);
      paste_tick = bst_piano_roll_controller_quantize (self, paste_tick);
      for (i = 0; i < pseq->n_pnotes; i++)
	{
	  BsePartNote *pnote = pseq->pnotes[i];
	  ctick = MIN (ctick, pnote->tick);
	  cnote = MAX (cnote, pnote->note);
	}
      cnote = paste_note - cnote;
      for (i = 0; i < pseq->n_pnotes; i++)
	{
	  BsePartNote *pnote = pseq->pnotes[i];
	  guint id;
	  gint note;
	  note = pnote->note + cnote;
	  if (note >= 0)
	    {
	      id = bse_part_insert_note (proxy,
					 pnote->tick - ctick + paste_tick,
					 pnote->duration,
					 note,
					 pnote->fine_tune,
					 pnote->velocity);
              bse_part_select_event (proxy, id);
            }
	}
      bse_item_ungroup_undo (proxy);
    }
}

gboolean
bst_piano_roll_controler_clipboard_full (BstPianoRollController *self)
{
  BsePartNoteSeq *pseq = bst_piano_roll_controller_get_clipboard ();
  return pseq && pseq->n_pnotes;
}

guint
bst_piano_roll_controller_quantize (BstPianoRollController *self,
                                    guint                   fine_tick)
{
  g_return_val_if_fail (self != NULL, fine_tick);

  /* quantize tick */
  if (QUANTIZATION (self) && self->proll)
    {
      guint quant = self->proll->ppqn * 4 / QUANTIZATION (self);
      guint qtick = fine_tick / quant;
      qtick *= quant;
      if (fine_tick - qtick > quant / 2 &&
          qtick + quant > fine_tick)
        fine_tick = qtick + quant;
      else
        fine_tick = qtick;
    }
  return fine_tick;
}

static void
controller_update_canvas_cursor (BstPianoRollController *self,
                                 BstGenericRollTool      tool)
{
  switch (tool)
    {
    case BST_GENERIC_ROLL_TOOL_INSERT:
      bst_piano_roll_set_canvas_cursor (self->proll, GDK_PENCIL);
      break;
    case BST_GENERIC_ROLL_TOOL_RESIZE:
      bst_piano_roll_set_canvas_cursor (self->proll, GDK_SB_H_DOUBLE_ARROW);
      break;
    case BST_GENERIC_ROLL_TOOL_MOVE:
      bst_piano_roll_set_canvas_cursor (self->proll, GDK_FLEUR);
      break;
    case BST_GENERIC_ROLL_TOOL_DELETE:
      bst_piano_roll_set_canvas_cursor (self->proll, GDK_TARGET);
      break;
    case BST_GENERIC_ROLL_TOOL_SELECT:
      bst_piano_roll_set_canvas_cursor (self->proll, GDK_CROSSHAIR);
      break;
    case BST_GENERIC_ROLL_TOOL_VSELECT:
      bst_piano_roll_set_canvas_cursor (self->proll, GDK_LEFT_SIDE);
      break;
    default:
      bst_piano_roll_set_canvas_cursor (self->proll, GXK_DEFAULT_CURSOR);
      break;
    }
}

static gboolean
check_hoverlap (SfiProxy part,
		guint    tick,
		guint    duration,
		gint     note,
		guint    except_tick,
		guint    except_duration)
{
  if (duration)
    {
      BsePartNoteSeq *pseq = bse_part_check_overlap (part, tick, duration, note);
      BsePartNote *pnote;
      
      if (pseq->n_pnotes == 0)
	return FALSE;     /* no overlap */
      if (pseq->n_pnotes > 1)
	return TRUE;      /* definite overlap */
      pnote = pseq->pnotes[0];
      if (pnote->tick == except_tick &&
	  pnote->duration == except_duration)
	return FALSE;     /* overlaps with exception */
    }
  return TRUE;
}

static void
move_start (BstPianoRollController *self,
	    BstPianoRollDrag       *drag)
{
  SfiProxy part = self->proll->proxy;
  if (self->obj_id)	/* got note to move */
    {
      self->xoffset = drag->start_tick - self->obj_tick;	/* drag offset */
      controller_update_canvas_cursor (self, BST_GENERIC_ROLL_TOOL_MOVE);
      gxk_status_set (GXK_STATUS_WAIT, _("Move Note"), NULL);
      drag->state = BST_DRAG_CONTINUE;
      if (bse_part_is_selected_event (part, self->obj_id))
	self->sel_pseq = bse_part_note_seq_copy_shallow (bse_part_list_selected_notes (part));
    }
  else
    {
      gxk_status_set (GXK_STATUS_ERROR, _("Move Note"), _("No target"));
      drag->state = BST_DRAG_HANDLED;
    }
}

static void
move_group_motion (BstPianoRollController *self,
		   BstPianoRollDrag       *drag)
{
  SfiProxy part = self->proll->proxy;
  gint i, new_tick, old_note, new_note, delta_tick, delta_note;

  new_tick = MAX (drag->current_tick, self->xoffset) - self->xoffset;
  new_tick = bst_piano_roll_controller_quantize (self, new_tick);
  old_note = self->obj_note;
  new_note = drag->current_note;
  delta_tick = self->obj_tick;
  delta_note = old_note;
  delta_tick -= new_tick;
  delta_note -= new_note;
  bse_item_group_undo (part, "Move Selection");
  for (i = 0; i < self->sel_pseq->n_pnotes; i++)
    {
      BsePartNote *pnote = self->sel_pseq->pnotes[i];
      gint tick = pnote->tick;
      gint note = pnote->note;
      note -= delta_note;
      bse_part_change_note (part, pnote->id,
			    MAX (tick - delta_tick, 0),
			    pnote->duration,
			    SFI_NOTE_CLAMP (note),
			    pnote->fine_tune,
			    pnote->velocity);
    }
  if (drag->type == BST_DRAG_DONE)
    {
      bse_part_note_seq_free (self->sel_pseq);
      self->sel_pseq = NULL;
    }
  bse_item_ungroup_undo (part);
}

static void
move_motion (BstPianoRollController *self,
	     BstPianoRollDrag       *drag)
{
  SfiProxy part = self->proll->proxy;
  gint new_tick;
  gboolean note_changed;

  if (self->sel_pseq)
    {
      move_group_motion (self, drag);
      return;
    }

  new_tick = MAX (drag->current_tick, self->xoffset) - self->xoffset;
  new_tick = bst_piano_roll_controller_quantize (self, new_tick);
  note_changed = self->obj_note != drag->current_note;
  if ((new_tick != self->obj_tick || note_changed) &&
      !check_hoverlap (part, new_tick, self->obj_duration, drag->current_note,
		       self->obj_tick, note_changed ? 0 : self->obj_duration))
    {
      bse_item_group_undo (part, "Move Note");
      if (bse_part_delete_event (part, self->obj_id) != BSE_ERROR_NONE)
	drag->state = BST_DRAG_ERROR;
      else
	{
	  self->obj_id = bse_part_insert_note (part, new_tick, self->obj_duration,
					       drag->current_note, self->obj_fine_tune, self->obj_velocity);
	  self->obj_tick = new_tick;
	  self->obj_note = drag->current_note;
	  if (!self->obj_id)
	    drag->state = BST_DRAG_ERROR;
	}
      bse_item_ungroup_undo (part);
    }
}

static void
move_abort (BstPianoRollController *self,
	    BstPianoRollDrag       *drag)
{
  if (self->sel_pseq)
    {
      bse_part_note_seq_free (self->sel_pseq);
      self->sel_pseq = NULL;
    }
  gxk_status_set (GXK_STATUS_ERROR, _("Move Note"), _("Lost Note"));
}

static void
resize_start (BstPianoRollController *self,
	      BstPianoRollDrag       *drag)
{
  if (self->obj_id)	/* got note for resize */
    {
      guint bound = self->obj_tick + self->obj_duration + 1;

      /* set the fix-point (either note start or note end) */
      if (drag->start_tick - self->obj_tick <= bound - drag->start_tick)
	self->tick_bound = bound;
      else
	self->tick_bound = self->obj_tick;
      controller_update_canvas_cursor (self, BST_GENERIC_ROLL_TOOL_RESIZE);
      gxk_status_set (GXK_STATUS_WAIT, _("Resize Note"), NULL);
      drag->state = BST_DRAG_CONTINUE;
    }
  else
    {
      gxk_status_set (GXK_STATUS_ERROR, _("Resize Note"), _("No target"));
      drag->state = BST_DRAG_HANDLED;
    }
}

static void
resize_motion (BstPianoRollController *self,
	       BstPianoRollDrag       *drag)
{
  SfiProxy part = self->proll->proxy;
  guint new_bound, new_tick, new_duration;

  /* calc new note around fix-point */
  new_tick = bst_piano_roll_controller_quantize (self, drag->current_tick);
  new_bound = MAX (new_tick, self->tick_bound);
  new_tick = MIN (new_tick, self->tick_bound);
  new_duration = new_bound - new_tick;
  new_duration = MAX (new_duration, 1) - 1;

  /* apply new note size */
  if ((self->obj_tick != new_tick || new_duration != self->obj_duration) &&
      !check_hoverlap (part, new_tick, new_duration, self->obj_note,
		       self->obj_tick, self->obj_duration))
    {
      bse_item_group_undo (part, "Resize Note");
      if (self->obj_id)
	{
	  BseErrorType error = bse_part_delete_event (part, self->obj_id);
	  if (error)
	    drag->state = BST_DRAG_ERROR;
	  self->obj_id = 0;
	}
      if (new_duration && drag->state != BST_DRAG_ERROR)
	{
	  self->obj_id = bse_part_insert_note (part, new_tick, new_duration,
					       self->obj_note, self->obj_fine_tune, self->obj_velocity);
	  self->obj_tick = new_tick;
	  self->obj_duration = new_duration;
	  if (!self->obj_id)
	    drag->state = BST_DRAG_ERROR;
	}
      bse_item_ungroup_undo (part);
    }
}

static void
resize_abort (BstPianoRollController *self,
	      BstPianoRollDrag       *drag)
{
  gxk_status_set (GXK_STATUS_ERROR, _("Resize Note"), _("Lost Note"));
}

static void
delete_start (BstPianoRollController *self,
	      BstPianoRollDrag       *drag)
{
  SfiProxy part = self->proll->proxy;
  if (self->obj_id)	/* got note to delete */
    {
      BseErrorType error = bse_part_delete_event (part, self->obj_id);
      bst_status_eprintf (error, _("Delete Note"));
    }
  else
    gxk_status_set (GXK_STATUS_ERROR, _("Delete Note"), _("No target"));
  drag->state = BST_DRAG_HANDLED;
}

static void
insert_start (BstPianoRollController *self,
	      BstPianoRollDrag       *drag)
{
  SfiProxy part = self->proll->proxy;
  BseErrorType error = BSE_ERROR_NO_TARGET;
  if (drag->start_valid)
    {
      guint qtick = bst_piano_roll_controller_quantize (self, drag->start_tick);
      guint duration = drag->proll->ppqn * 4 / NOTE_LENGTH (self);
      if (check_hoverlap (part, qtick, duration, drag->start_note, 0, 0))
	error = BSE_ERROR_INVALID_OVERLAP;
      else
	{
	  bse_part_insert_note (part, qtick, duration, drag->start_note, 0, 1.0);
	  error = BSE_ERROR_NONE;
	}
    }
  bst_status_eprintf (error, _("Insert Note"));
  drag->state = BST_DRAG_HANDLED;
}

static void
select_start (BstPianoRollController *self,
	      BstPianoRollDrag       *drag)
{
  drag->start_tick = bst_piano_roll_controller_quantize (self, drag->start_tick);
  bst_piano_roll_set_view_selection (drag->proll, drag->start_tick, 0, 0, 0);
  gxk_status_set (GXK_STATUS_WAIT, _("Select Region"), NULL);
  drag->state = BST_DRAG_CONTINUE;
}

static void
select_motion (BstPianoRollController *self,
	       BstPianoRollDrag       *drag)
{
  SfiProxy part = self->proll->proxy;
  guint start_tick = MIN (drag->start_tick, drag->current_tick);
  guint end_tick = MAX (drag->start_tick, drag->current_tick);
  gint min_note = MIN (drag->start_note, drag->current_note);
  gint max_note = MAX (drag->start_note, drag->current_note);

  bst_piano_roll_set_view_selection (drag->proll, start_tick, end_tick - start_tick, min_note, max_note);
  if (drag->type == BST_DRAG_DONE)
    {
      bse_part_select_notes_exclusive (part, start_tick, end_tick - start_tick, min_note, max_note);
      bst_piano_roll_set_view_selection (drag->proll, 0, 0, 0, 0);
    }
}

static void
select_abort (BstPianoRollController *self,
	      BstPianoRollDrag       *drag)
{
  gxk_status_set (GXK_STATUS_ERROR, _("Select Region"), _("Aborted"));
  bst_piano_roll_set_view_selection (drag->proll, 0, 0, 0, 0);
}

static void
vselect_start (BstPianoRollController *self,
	       BstPianoRollDrag       *drag)
{
  drag->start_tick = bst_piano_roll_controller_quantize (self, drag->start_tick);
  bst_piano_roll_set_view_selection (drag->proll, drag->start_tick, 0, drag->proll->min_note, drag->proll->max_note);
  gxk_status_set (GXK_STATUS_WAIT, _("Vertical Select"), NULL);
  drag->state = BST_DRAG_CONTINUE;
}

static void
vselect_motion (BstPianoRollController *self,
		BstPianoRollDrag       *drag)
{
  SfiProxy part = self->proll->proxy;
  guint start_tick = MIN (drag->start_tick, drag->current_tick);
  guint end_tick = MAX (drag->start_tick, drag->current_tick);

  bst_piano_roll_set_view_selection (drag->proll, start_tick, end_tick - start_tick,
				     drag->proll->min_note, drag->proll->max_note);
  if (drag->type == BST_DRAG_DONE)
    {
      bse_part_select_notes_exclusive (part, start_tick, end_tick - start_tick,
                                       drag->proll->min_note, drag->proll->max_note);
      bst_piano_roll_set_view_selection (drag->proll, 0, 0, 0, 0);
    }
}

static void
vselect_abort (BstPianoRollController *self,
	       BstPianoRollDrag       *drag)
{
  gxk_status_set (GXK_STATUS_ERROR, _("Vertical Region"), _("Aborted"));
  bst_piano_roll_set_view_selection (drag->proll, 0, 0, 0, 0);
}

#if 0
static void
generic_abort (BstPianoRollController *self,
	       BstPianoRollDrag       *drag)
{
  gxk_status_set (GXK_STATUS_ERROR, _("Abortion"), NULL);
}
#endif

typedef void (*DragFunc) (BstPianoRollController *,
			  BstPianoRollDrag       *);

void
controller_canvas_drag (BstPianoRollController *self,
			BstPianoRollDrag       *drag)
{
  static struct {
    BstGenericRollTool tool;
    DragFunc start, motion, abort;
  } tool_table[] = {
    { BST_GENERIC_ROLL_TOOL_INSERT,	insert_start,	NULL,		NULL,		},
    { BST_GENERIC_ROLL_TOOL_ALIGN,	insert_start,	NULL,		NULL,		},
    { BST_GENERIC_ROLL_TOOL_RESIZE,	resize_start,	resize_motion,	resize_abort,	},
    { BST_GENERIC_ROLL_TOOL_MOVE,	move_start,	move_motion,	move_abort,	},
    { BST_GENERIC_ROLL_TOOL_DELETE,	delete_start,	NULL,		NULL,		},
    { BST_GENERIC_ROLL_TOOL_SELECT,	select_start,	select_motion,	select_abort,	},
    { BST_GENERIC_ROLL_TOOL_VSELECT,	vselect_start,	vselect_motion,	vselect_abort,	},
  };
  guint i;

  if (drag->type == BST_DRAG_START)
    {
      BstGenericRollTool tool = BST_GENERIC_ROLL_TOOL_NONE;
      BsePartNoteSeq *pseq;

      /* setup drag data */
      pseq = bse_part_get_notes (drag->proll->proxy, drag->start_tick, drag->start_note);
      if (pseq->n_pnotes)
	{
	  BsePartNote *pnote = pseq->pnotes[0];
	  self->obj_id = pnote->id;
	  self->obj_tick = pnote->tick;
	  self->obj_duration = pnote->duration;
	  self->obj_note = pnote->note;
	  self->obj_fine_tune = pnote->fine_tune;
	  self->obj_velocity = pnote->velocity;
	}
      else
	{
	  self->obj_id = 0;
	  self->obj_tick = 0;
	  self->obj_duration = 0;
	  self->obj_note = 0;
	  self->obj_fine_tune = 0;
	  self->obj_velocity = 0;
	}
      if (self->sel_pseq)
	g_warning ("leaking old drag selection (%p)", self->sel_pseq);
      self->sel_pseq = NULL;
      self->xoffset = 0;
      self->tick_bound = 0;

      /* find drag tool */
      tool = piano_canvas_button_tool (self, drag->button, self->obj_id > 0);
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
    controller_reset_canvas_cursor (self);
}

void
controller_piano_drag (BstPianoRollController *self,
		       BstPianoRollDrag       *drag)
{
  SfiProxy part = self->proll->proxy;
  SfiProxy song = bse_item_get_parent (part);
  SfiProxy project = song ? bse_item_get_parent (song) : 0;
  SfiProxy track = song ? bse_song_find_track_for_part (song, part) : 0;

  // sfi_debug ("piano drag event, note=%d (valid=%d)", drag->current_note, drag->current_valid);

  if (project && track)
    {
      if (drag->type == BST_DRAG_START ||
	  (drag->type == BST_DRAG_MOTION &&
	   self->obj_note != drag->current_note))
	{
	  BseErrorType error;
	  bse_project_auto_deactivate (project, 5 * 1000);
	  error = bse_project_activate (project);
	  self->obj_note = drag->current_note;
	  if (error == BSE_ERROR_NONE)
	    bse_song_synthesize_note (song, track, 384 * 4, self->obj_note, 0, 1.0);
	  bst_status_eprintf (error, _("Play note"));
	  drag->state = BST_DRAG_CONTINUE;
	}
    }

  if (drag->type == BST_DRAG_START ||
      drag->type == BST_DRAG_MOTION)
    drag->state = BST_DRAG_CONTINUE;
}
