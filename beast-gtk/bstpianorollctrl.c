/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002 Tim Janik
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

#include "bststatusbar.h"


#define	FREQ_FUDGE	(0.00001)

/* --- prototypes --- */
void		controller_canvas_drag		(BstPianoRollController	*self,
						 BstPianoRollDrag	*drag);
static void	controller_update_cursor	(BstPianoRollController *self,
						 BstPianoRollTool	 tool);


/* --- functions --- */
BstPianoRollController*
bst_piano_roll_controller_new (BstPianoRoll *proll)
{
  BstPianoRollController *self;

  g_return_val_if_fail (BST_IS_PIANO_ROLL (proll), NULL);

  self = g_new0 (BstPianoRollController, 1);
  self->proll = proll;
  self->ref_count = 1;

  self->ref_count++;
  self->note_length = 1;
  g_signal_connect_data (proll, "canvas-drag",
			 G_CALLBACK (controller_canvas_drag),
			 self, (GClosureNotify) bst_piano_roll_controller_unref,
			 G_CONNECT_SWAPPED);

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
    g_free (self);
}

void
bst_piano_roll_controller_set_obj_tools (BstPianoRollController *self,
					 BstPianoRollTool        tool1,
					 BstPianoRollTool        tool2,
					 BstPianoRollTool        tool3)
{
  g_return_if_fail (self != NULL);

  self->obj_tool1 = tool1;
  self->obj_tool2 = tool2;
  self->obj_tool3 = tool3;
}

void
bst_piano_roll_controller_set_bg_tools (BstPianoRollController *self,
					BstPianoRollTool        tool1,
					BstPianoRollTool        tool2,
					BstPianoRollTool        tool3)
{
  g_return_if_fail (self != NULL);

  self->bg_tool1 = tool1;
  self->bg_tool2 = tool2;
  self->bg_tool3 = tool3;
  controller_update_cursor (self, self->bg_tool1);
}

static void
controller_update_cursor (BstPianoRollController *self,
			  BstPianoRollTool        tool)
{
  switch (tool)
    {
    case BST_PIANO_ROLL_TOOL_INSERT:
      bst_piano_roll_set_canvas_cursor (self->proll, GDK_PENCIL);
      break;
    case BST_PIANO_ROLL_TOOL_RESIZE:
      bst_piano_roll_set_canvas_cursor (self->proll, GDK_SB_H_DOUBLE_ARROW);
      break;
    case BST_PIANO_ROLL_TOOL_MOVE:
      bst_piano_roll_set_canvas_cursor (self->proll, GDK_FLEUR);
      break;
    case BST_PIANO_ROLL_TOOL_DELETE:
      bst_piano_roll_set_canvas_cursor (self->proll, GDK_TARGET);
      break;
    case BST_PIANO_ROLL_TOOL_SELECT:
      bst_piano_roll_set_canvas_cursor (self->proll, GDK_CROSSHAIR);
      break;
    case BST_PIANO_ROLL_TOOL_VSELECT:
      bst_piano_roll_set_canvas_cursor (self->proll, GDK_LEFT_SIDE);
      break;
    default:
      bst_piano_roll_set_canvas_cursor (self->proll, GDK_X_CURSOR);
      break;
    }
}

static gboolean
check_overlap (BswProxy part,
               guint    tick,
               guint    duration,
               gfloat   freq,
               guint    except_tick,
               guint    except_duration)
{
  BswIterPartNote *iter;
  BswPartNote *pnote;
  
  iter = bsw_part_check_overlap (part, tick, duration, freq);
  if (bsw_iter_n_left (iter) == 0)
    {
      bsw_iter_free (iter);
      return FALSE;     /* no overlap */
    }
  if (bsw_iter_n_left (iter) > 1)
    {
      bsw_iter_free (iter);
      return TRUE;      /* definite overlap */
    }
  pnote = bsw_iter_get_part_note (iter);
  if (pnote->tick == except_tick &&
      pnote->duration == except_duration)
    {
      bsw_iter_free (iter);
      return FALSE;     /* overlaps with exception */
    }
  bsw_iter_free (iter);
  return TRUE;
}

static void
move_start (BstPianoRollController *self,
	    BstPianoRollDrag       *drag)
{
  if (drag->obj_duration)	/* got note to move */
    {
      drag->obj_tick = bst_piano_roll_quantize (drag->proll, drag->obj_tick);
      drag->x = drag->start_tick - drag->obj_tick;	/* drag offset */
      controller_update_cursor (self, BST_PIANO_ROLL_TOOL_MOVE);
      bst_status_set (BST_STATUS_WAIT, "Move Note", NULL);
      drag->state = BST_DRAG_CONTINUE;
    }
  else
    {
      bst_status_set (BST_STATUS_ERROR, "Move Note", "No target");
      drag->state = BST_DRAG_HANDLED;
    }
}

static void
move_motion (BstPianoRollController *self,
	     BstPianoRollDrag       *drag)
{
  BswProxy part = self->proll->proxy;
  guint new_tick;
  gboolean freq_changed;
  
  new_tick = MAX (drag->current_tick, drag->x) - drag->x;
  freq_changed = !bsw_part_freq_equals (part, drag->obj_freq, drag->current_freq);
  if (!check_overlap (part, new_tick, drag->obj_duration, drag->current_freq,
                      drag->obj_tick, freq_changed ? 0 : drag->obj_duration))
    {
      BswIterPartNote *iter = bsw_part_get_note (part, drag->obj_tick, drag->obj_freq);
      if (bsw_iter_n_left (iter))
        {
          BswPartNote *pnote = bsw_iter_get_part_note (iter);
          if (pnote->tick != new_tick || freq_changed)
            {
              bsw_part_delete_note (part, pnote->tick, pnote->freq);
              bsw_part_insert_note (part, new_tick, pnote->duration, drag->current_freq, pnote->velocity);
	      drag->obj_tick = new_tick;
	      drag->obj_duration = pnote->duration;
	      drag->obj_freq = drag->current_freq;
            }
        }
      else /* eek, lost note during drag */
        drag->state = BST_DRAG_ERROR;
      bsw_iter_free (iter);
    }
  if (drag->type == BST_DRAG_DONE)
    controller_update_cursor (self, self->bg_tool1);
}

static void
move_abort (BstPianoRollController *self,
	    BstPianoRollDrag       *drag)
{
  bst_status_set (BST_STATUS_ERROR, "Move Note", "Lost Note");
  controller_update_cursor (self, self->bg_tool1);
}

static void
resize_start (BstPianoRollController *self,
	      BstPianoRollDrag       *drag)
{
  if (drag->obj_duration)	/* got note for resize */
    {
      guint bound = drag->obj_tick + drag->obj_duration + 1;

      /* set the fix-point (either note start or note end) */
      if (drag->start_tick - drag->obj_tick <= bound - drag->start_tick)
	drag->x = bound;
      else
	drag->x = drag->obj_tick;
      drag->fdata1 = 1.0;
      controller_update_cursor (self, BST_PIANO_ROLL_TOOL_RESIZE);
      bst_status_set (BST_STATUS_WAIT, "Resize Note", NULL);
      drag->state = BST_DRAG_CONTINUE;
    }
  else
    {
      bst_status_set (BST_STATUS_ERROR, "Resize Note", "No target");
      drag->state = BST_DRAG_HANDLED;
    }
}

static void
resize_motion (BstPianoRollController *self,
	       BstPianoRollDrag       *drag)
{
  BswProxy part = self->proll->proxy;
  guint new_bound, new_tick, new_duration;

  /* calc new note around fix-point */
  new_bound = MAX (drag->current_tick, drag->x);
  new_tick = MIN (drag->current_tick, drag->x);
  new_duration = new_bound - new_tick;
  new_duration = MAX (new_duration, 1) - 1;

  /* apply new note size */
  if ((new_duration != drag->obj_duration || drag->obj_tick != new_tick) &&
      (!new_duration || !check_overlap (part, new_tick, new_duration,
					drag->obj_freq, drag->obj_tick, drag->obj_duration)))
    {
      BswIterPartNote *iter = bsw_part_get_note (part, drag->obj_tick, drag->obj_freq);
      if (bsw_iter_n_left (iter) || !drag->obj_duration)
	{
	  BswPartNote *pnote = bsw_iter_n_left (iter) ? bsw_iter_get_part_note (iter) : NULL;
	  if (pnote)
	    {
	      bsw_part_delete_note (part, pnote->tick, pnote->freq);
	      drag->obj_freq = pnote->freq;
	      drag->fdata1 = pnote->velocity;
	    }
	  drag->obj_tick = new_tick;
	  drag->obj_duration = new_duration;
	  if (new_duration)
	    bsw_part_insert_note (part, drag->obj_tick, drag->obj_duration, drag->obj_freq, drag->fdata1);
	}
      else /* eek, lost note during drag */
	drag->state = BST_DRAG_ERROR;
      bsw_iter_free (iter);
    }
  if (drag->type == BST_DRAG_DONE)
    controller_update_cursor (self, self->bg_tool1);
}

static void
resize_abort (BstPianoRollController *self,
	      BstPianoRollDrag       *drag)
{
  bst_status_set (BST_STATUS_ERROR, "Resize Note", "Lost Note");
  controller_update_cursor (self, self->bg_tool1);
}

static void
delete_start (BstPianoRollController *self,
	      BstPianoRollDrag       *drag)
{
  BswProxy part = self->proll->proxy;
  if (drag->obj_duration)	/* got note to delete */
    {
      bsw_part_delete_note (part, drag->obj_tick, drag->obj_freq);
      bst_status_set (BST_STATUS_DONE, "Delete Note", NULL);
    }
  else
    bst_status_set (BST_STATUS_ERROR, "Delete Note", "No target");
  drag->state = BST_DRAG_HANDLED;
}

static void
insert_start (BstPianoRollController *self,
	      BstPianoRollDrag       *drag)
{
  BswProxy part = self->proll->proxy;
  if (drag->start_valid)
    {
      BswErrorType error = bsw_part_insert_note (part,
						 drag->start_tick,
						 drag->proll->ppqn * 4 / self->note_length,
						 drag->start_freq, 1.0);
      bst_status_eprintf (error, "Insert Note");
    }
  else
    bst_status_set (BST_STATUS_ERROR, "Insert Note", "No target");
  drag->state = BST_DRAG_HANDLED;
}

static void
select_start (BstPianoRollController *self,
	      BstPianoRollDrag       *drag)
{
  BswProxy part = self->proll->proxy;
  bsw_part_set_selection (part, drag->start_tick, 0, drag->start_freq, drag->start_freq);
  bst_status_set (BST_STATUS_WAIT, "Select Region", NULL);
  drag->state = BST_DRAG_CONTINUE;
}

static void
select_motion (BstPianoRollController *self,
	       BstPianoRollDrag       *drag)
{
  BswProxy part = self->proll->proxy;
  guint start_tick = MIN (drag->start_tick, drag->current_tick);
  guint end_tick = MAX (drag->start_tick, drag->current_tick);
  gfloat min_freq = MIN (drag->start_freq, drag->current_freq);
  gfloat max_freq = MAX (drag->start_freq, drag->current_freq);

  /* make sure the frequencies aren't completely equal */
  min_freq -= FREQ_FUDGE;
  max_freq += FREQ_FUDGE;
  bsw_part_set_selection (part, start_tick, end_tick - start_tick, min_freq, max_freq);
}

static void
select_abort (BstPianoRollController *self,
	      BstPianoRollDrag       *drag)
{
  BswProxy part = self->proll->proxy;
  bst_status_set (BST_STATUS_ERROR, "Select Region", "Aborted");
  bsw_part_set_selection (part, 0, 0, 1, 0);
  controller_update_cursor (self, self->bg_tool1);
}

static void
vselect_start (BstPianoRollController *self,
	       BstPianoRollDrag       *drag)
{
  BswProxy part = self->proll->proxy;
  bsw_part_set_selection (part, drag->start_tick, 0, drag->proll->min_freq - FREQ_FUDGE, drag->proll->max_freq + FREQ_FUDGE);
  bst_status_set (BST_STATUS_WAIT, "Vertical Select", NULL);
  drag->state = BST_DRAG_CONTINUE;
}

static void
vselect_motion (BstPianoRollController *self,
		BstPianoRollDrag       *drag)
{
  BswProxy part = self->proll->proxy;
  guint start_tick = MIN (drag->start_tick, drag->current_tick);
  guint end_tick = MAX (drag->start_tick, drag->current_tick);

  bsw_part_set_selection (part, start_tick, end_tick - start_tick,
			  drag->proll->min_freq - FREQ_FUDGE, drag->proll->max_freq + FREQ_FUDGE);
}

static void
vselect_abort (BstPianoRollController *self,
	       BstPianoRollDrag       *drag)
{
  BswProxy part = self->proll->proxy;
  bst_status_set (BST_STATUS_ERROR, "Vertical Region", "Aborted");
  bsw_part_set_selection (part, 0, 0, 1, 0);
  controller_update_cursor (self, self->bg_tool1);
}

#if 0
static void
generic_abort (BstPianoRollController *self,
	       BstPianoRollDrag       *drag)
{
  bst_status_set (BST_STATUS_ERROR, "Abortion", NULL);
  controller_update_cursor (self, self->bg_tool1);
}
#endif

typedef void (*DragFunc) (BstPianoRollController *,
			  BstPianoRollDrag       *);

void
controller_canvas_drag (BstPianoRollController *self,
			BstPianoRollDrag       *drag)
{
  static struct {
    BstPianoRollTool tool;
    DragFunc start, motion, abort;
  } tool_table[] = {
    { BST_PIANO_ROLL_TOOL_INSERT,	insert_start,	NULL,		NULL,		},
    { BST_PIANO_ROLL_TOOL_RESIZE,	resize_start,	resize_motion,	resize_abort,	},
    { BST_PIANO_ROLL_TOOL_MOVE,		move_start,	move_motion,	move_abort,	},
    { BST_PIANO_ROLL_TOOL_DELETE,	delete_start,	NULL,		NULL,		},
    { BST_PIANO_ROLL_TOOL_SELECT,	select_start,	select_motion,	select_abort,	},
    { BST_PIANO_ROLL_TOOL_VSELECT,	vselect_start,	vselect_motion,	vselect_abort,	},
  };
  guint i;

  if (drag->type == BST_DRAG_START)	/* find drag tool */
    {
      BstPianoRollTool tool = BST_PIANO_ROLL_TOOL_NONE;
      if (drag->obj_duration)	/* have object */
	{
	  if (drag->button == 1)
	    tool = self->obj_tool1;
	  else if (drag->button == 2)
	    tool = self->obj_tool2;
	  else if (drag->button == 3)
	    tool = self->obj_tool3;
	}
      else
	{
	  if (drag->button == 1)
	    tool = self->bg_tool1;
	  else if (drag->button == 2)
	    tool = self->bg_tool2;
	  else if (drag->button == 3)
	    tool = self->bg_tool3;
	}
      for (i = 0; i < G_N_ELEMENTS (tool_table); i++)
	if (tool_table[i].tool == tool)
	  break;
      drag->tdata = GUINT_TO_POINTER (i);
      if (i >= G_N_ELEMENTS (tool_table))
	return;		/* unhandled */
    }
  i = GPOINTER_TO_UINT (drag->tdata);
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
}
