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
#include	"bstpartdialog.h"

#include	"bststatusbar.h"
#include	"bstprocedure.h"
#include	"bstmenus.h"
#include	"bsteffectview.h"

enum {
  TOOL_MOVE_RESIZE,
  TOOL_DELETE,
  TOOL_SELECT,
  TOOL_VSELECT,
};


/* --- prototypes --- */
static void	bst_part_dialog_class_init	(BstPartDialogClass	*klass);
static void	bst_part_dialog_init		(BstPartDialog		*part_dialog);
static void	bst_part_dialog_finalize	(GObject		*object);
static void	piano_update_cursor		(BstPartDialog		*part_dialog);
static void	piano_canvas_press		(BstPartDialog		*part_dialog,
						 guint			 button,
						 guint			 tick_position,
						 gfloat			 freq,
						 BstPianoRoll		*proll);
static void	piano_canvas_motion		(BstPartDialog		*part_dialog,
						 guint			 button,
						 guint			 tick_position,
						 gfloat			 freq,
						 BstPianoRoll		*proll);
static void	piano_canvas_release		(BstPartDialog		*part_dialog,
						 guint			 button,
						 guint			 tick_position,
						 gfloat			 freq,
						 BstPianoRoll		*proll);


/* --- static variables --- */
static gpointer	parent_class = NULL;


/* --- functions --- */
GType
bst_part_dialog_get_type (void)
{
  static GType type = 0;
  
  if (!type)
    {
      static const GTypeInfo type_info = {
	sizeof (BstPartDialogClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) bst_part_dialog_class_init,
	NULL,   /* class_finalize */
	NULL,   /* class_data */
	sizeof (BstPartDialog),
	0,      /* n_preallocs */
	(GInstanceInitFunc) bst_part_dialog_init,
      };
      
      type = g_type_register_static (BST_TYPE_DIALOG,
				     "BstPartDialog",
				     &type_info, 0);
    }
  
  return type;
}

static void
bst_part_dialog_class_init (BstPartDialogClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->finalize = bst_part_dialog_finalize;
  
  /* create item factory for menu entries and categories */
  class->popup_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<BstPartDialog>", NULL);
}

static void
vzoom_changed (BstPartDialog *self,
	       GtkAdjustment *adjustment)
{
  if (self->proll)
    bst_piano_roll_set_vzoom (BST_PIANO_ROLL (self->proll), adjustment->value);
}

static void
bst_part_dialog_init (BstPartDialog *self)
{
  BstPartDialogClass *class = BST_PART_DIALOG_GET_CLASS (self);
  GtkWidget *main_vbox, *entry, *any;
  GtkObject *adjustment;
  
  g_object_set (self,
		"default_width", 600,
		"default_height", 450,
		"flags", BST_DIALOG_STATUS,
		NULL);
  main_vbox = BST_DIALOG (self)->vbox;

  /* create toolbar */
  self->toolbar = g_object_new (GTK_TYPE_TOOLBAR,
				"visible", TRUE,
				"orientation", GTK_ORIENTATION_HORIZONTAL,
				"toolbar_style", GTK_TOOLBAR_BOTH,
				NULL);
  g_object_connect (self->toolbar,
		    "swapped_signal::destroy", g_nullify_pointer, &self->toolbar,
		    NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), self->toolbar, FALSE, TRUE, 0);

  /* create scrolled window */
  self->scrolled_window = g_object_new (GTK_TYPE_SCROLLED_WINDOW,
					"visible", TRUE,
					"hscrollbar_policy", GTK_POLICY_AUTOMATIC,
					"vscrollbar_policy", GTK_POLICY_AUTOMATIC,
					"parent", main_vbox,
					"border_width", 5,
					NULL);
  g_object_connect (self->scrolled_window,
		    "swapped_signal::destroy", g_nullify_pointer, &self->scrolled_window,
		    NULL);
  
  /* piano roll */
  self->proll = g_object_new (BST_TYPE_PIANO_ROLL,
			      "visible", TRUE,
			      "parent", self->scrolled_window,
			      NULL);
  g_object_connect (self->proll,
		    "swapped_signal::destroy", g_nullify_pointer, &self->proll,
		    "swapped_signal::canvas_press", piano_canvas_press, self,
		    "swapped_signal::canvas_motion", piano_canvas_motion, self,
		    "swapped_signal::canvas_release", piano_canvas_release, self,
		    NULL);
  
  /* radio tools */
  self->rtools = bst_radio_tools_new ();
  g_object_ref (self->rtools);
  gtk_object_sink (GTK_OBJECT (self->rtools));
  g_object_connect (self->rtools,
		    "swapped_signal::set_tool", piano_update_cursor, self,
		    NULL);
  bst_radio_tools_set_tool (self->rtools, TOOL_MOVE_RESIZE);

  /* register tools */
  bst_radio_tools_add_stock_tool (self->rtools, TOOL_MOVE_RESIZE,
				  "Edit", "Insert/resize/move notes (mouse button 1 and 2)", NULL,
				  BST_STOCK_PART_TOOL, BST_RADIO_TOOLS_EVERYWHERE);
  bst_radio_tools_add_stock_tool (self->rtools, TOOL_DELETE,
				  "Delete", "Delete note (mouse button 1)", NULL,
				  BST_STOCK_TRASHCAN, BST_RADIO_TOOLS_EVERYWHERE);
  bst_radio_tools_add_stock_tool (self->rtools, TOOL_SELECT,
				  "Select", "Rectangle select notes", NULL,
				  BST_STOCK_RECT_SELECT, BST_RADIO_TOOLS_EVERYWHERE);
  bst_radio_tools_add_stock_tool (self->rtools, TOOL_VSELECT,
				  "VSelect", "Select tick range vertically", NULL,
				  BST_STOCK_VERT_SELECT, BST_RADIO_TOOLS_EVERYWHERE);
  bst_radio_tools_build_toolbar (self->rtools, GTK_TOOLBAR (self->toolbar));
  gtk_toolbar_append_space (GTK_TOOLBAR (self->toolbar));

  /* hzoom */
  adjustment = gtk_adjustment_new (4, 1, 16, 1, 1, 0);
  g_object_connect (adjustment,
		    "swapped_signal_after::value_changed", vzoom_changed, self,
		    NULL);
  any = g_object_new (GTK_TYPE_VBOX,
		      "visible", TRUE,
		      "width_request", 2 * bst_size_width (BST_SIZE_TOOLBAR),
		      NULL);
  entry = g_object_new (GTK_TYPE_SPIN_BUTTON,
			"visible", TRUE,
			"adjustment", adjustment,
			"digits", 0,
			"parent", any,
			NULL);
  gtk_box_pack_end (GTK_BOX (any),
		    g_object_new (GTK_TYPE_LABEL,
				  "visible", TRUE,
				  "label", "HZoom",
				  NULL),
		    FALSE, FALSE, 0);
  gtk_toolbar_append_element (GTK_TOOLBAR (self->toolbar), GTK_TOOLBAR_CHILD_WIDGET, any,
			      NULL, "Horizontal Zoom", NULL, NULL, NULL, NULL);

  
  /* setup the popup menu
   */
  gtk_window_add_accel_group (GTK_WINDOW (self),
			      class->popup_factory->accel_group);
  bst_menu_add_accel_owner (class->popup_factory, GTK_WIDGET (self));
}

static void
bst_part_dialog_finalize (GObject *object)
{
  BstPartDialog *self = BST_PART_DIALOG (object);

  g_object_unref (self->rtools);
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

void
bst_part_dialog_set_proxy (BstPartDialog *self,
			   BswProxy       part)
{
  g_return_if_fail (BST_IS_PART_DIALOG (self));
  if (part)
    g_return_if_fail (BSW_IS_PART (part));

  bst_dialog_sync_title_to_proxy (BST_DIALOG (self), part, "%s");
  bst_piano_roll_set_proxy (BST_PIANO_ROLL (self->proll), part);
}

static void
piano_update_cursor (BstPartDialog *self)
{
  switch (self->rtools->tool_id)
    {
    case TOOL_MOVE_RESIZE:
      bst_piano_roll_set_canvas_cursor (BST_PIANO_ROLL (self->proll), GDK_PENCIL);
      break;
    case TOOL_DELETE:
      bst_piano_roll_set_canvas_cursor (BST_PIANO_ROLL (self->proll), GDK_TARGET);
      break;
    case TOOL_SELECT:
      bst_piano_roll_set_canvas_cursor (BST_PIANO_ROLL (self->proll), GDK_CROSSHAIR);
      break;
    case TOOL_VSELECT:
      bst_piano_roll_set_canvas_cursor (BST_PIANO_ROLL (self->proll), GDK_LEFT_SIDE);
      break;
    }
}

static void
piano_canvas_press (BstPartDialog *self,
                    guint          button,
                    guint          tick,
                    gfloat         freq,
                    BstPianoRoll  *proll)
{
  BswProxy part = proll->proxy;

  switch (self->rtools->tool_id)
    {
      BswIterPartNote *iter;
      BswPartNote *pnote;
    case TOOL_MOVE_RESIZE:
      iter = bsw_part_get_note (part, tick, freq);
      if (bsw_iter_n_left (iter))
        {
          pnote = bsw_iter_get_part_note (iter);
          if (button == 2)      /* move */
            {
              bst_piano_roll_set_drag_data1 (proll, pnote->tick, pnote->duration, pnote->freq);
              bst_piano_roll_set_drag_data2 (proll, tick - pnote->tick, 0, 0); /* drag offset */
              bst_piano_roll_set_canvas_cursor (BST_PIANO_ROLL (self->proll), GDK_FLEUR);
              bst_status_set (BST_STATUS_WAIT, "Move Note", NULL);
            }
          else if (button == 1) /* resize */
            {
              guint bound = pnote->tick + pnote->duration;

              bst_piano_roll_set_drag_data1 (proll, pnote->tick, pnote->duration, pnote->freq);
              if (tick - pnote->tick <= bound - tick)
                bst_piano_roll_set_drag_data2 (proll, bound - 1, 0, 0);
              else
                bst_piano_roll_set_drag_data2 (proll, pnote->tick, 0, 0);
              bst_piano_roll_set_canvas_cursor (BST_PIANO_ROLL (self->proll), GDK_SB_H_DOUBLE_ARROW);
              bst_status_set (BST_STATUS_WAIT, "Resize Note", NULL);
            }
          else
            bst_piano_roll_set_pointer (proll, BST_PIANO_ROLL_POINTER_IGNORE);
        }
      else if (button == 1)     /* insert */
        {
          BswErrorType error = bsw_part_insert_note (part, tick, 384, freq, 1);

          bst_piano_roll_set_pointer (proll, BST_PIANO_ROLL_POINTER_IGNORE);
          bst_status_eprintf (error, "Insert Note");
        }
      else
        bst_piano_roll_set_pointer (proll, BST_PIANO_ROLL_POINTER_IGNORE);
      bsw_iter_free (iter);
      break;
    case TOOL_DELETE:
      iter = bsw_part_get_note (part, tick, freq);
      if (bsw_iter_n_left (iter))
	{
          pnote = bsw_iter_get_part_note (iter);
	  if (button == 1)	/* delete */
	    {
	      bsw_part_delete_note (part, pnote->tick, pnote->freq);
	      bst_piano_roll_set_pointer (proll, BST_PIANO_ROLL_POINTER_IGNORE);
	      bst_status_set (BST_STATUS_DONE, "Delete Note", NULL);
	    }
	  else if (button == 2)	/* move */
	    {
	      bst_piano_roll_set_drag_data1 (proll, pnote->tick, pnote->duration, pnote->freq);
	      bst_piano_roll_set_drag_data2 (proll, tick - pnote->tick, 0, 0); /* drag offset */
	      bst_piano_roll_set_canvas_cursor (BST_PIANO_ROLL (self->proll), GDK_FLEUR);
	      bst_status_set (BST_STATUS_WAIT, "Move Note", NULL);
	    }
	  else
	    bst_piano_roll_set_pointer (proll, BST_PIANO_ROLL_POINTER_IGNORE);
	}
      else
	{
	  bst_piano_roll_set_pointer (proll, BST_PIANO_ROLL_POINTER_IGNORE);
	  bst_status_set (BST_STATUS_ERROR, "Delete Note", "No target");
	}
      break;
    default:
      bst_piano_roll_set_pointer (proll, BST_PIANO_ROLL_POINTER_IGNORE);
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
piano_move (BstPartDialog *self,
            guint          tick,
            gfloat         freq,
            BstPianoRoll  *proll)
{
  BswProxy part = proll->proxy;
  BswIterPartNote *iter;
  BswPartNote *pnote;
  guint note_tick, note_duration, offset;
  gboolean freq_changed;
  gfloat note_freq;
  
  bst_piano_roll_get_drag_data1 (proll, &note_tick, &note_duration, &note_freq);
  bst_piano_roll_get_drag_data2 (proll, &offset, NULL, NULL); /* drag offset */
  tick = MAX (tick, offset) - offset;
  freq_changed = !bsw_part_freq_equals (part, note_freq, freq);
  if (!check_overlap (part, tick, note_duration, freq,
                      note_tick, freq_changed ? 0 : note_duration))
    {
      iter = bsw_part_get_note (part, note_tick, note_freq);
      if (bsw_iter_n_left (iter))
        {
          pnote = bsw_iter_get_part_note (iter);
          if (pnote->tick != tick || freq_changed)
            {
              bsw_part_delete_note (part, pnote->tick, pnote->freq);
              bsw_part_insert_note (part, tick, pnote->duration, freq, pnote->velocity);
              bst_piano_roll_set_drag_data1 (proll, tick, pnote->duration, freq);
            }
        }
      else /* eek, lost note during drag */
        {
          bst_piano_roll_set_pointer (proll, BST_PIANO_ROLL_POINTER_IGNORE);
          bst_status_set (BST_STATUS_ERROR, "Move Note", "Lost Note");
          piano_update_cursor (self);
        }
      bsw_iter_free (iter);
    }
}

static void
piano_resize (BstPartDialog *self,
              guint          tick,
              gfloat         freq,
              BstPianoRoll  *proll)
{
  BswProxy part = proll->proxy;
  BswIterPartNote *iter;
  BswPartNote *pnote;
  guint note_tick, note_duration, new_tick, new_duration;
  gfloat note_freq;

  bst_piano_roll_get_drag_data1 (proll, &note_tick, &note_duration, &note_freq);
  bst_piano_roll_get_drag_data2 (proll, &new_tick, NULL, NULL);
  new_duration = MAX (new_tick, tick);
  new_tick = MIN (new_tick, tick);
  new_duration = new_duration - new_tick + 1;
  if (new_duration && (new_duration != note_duration || note_tick != new_tick) &&
      !check_overlap (part, new_tick, new_duration, note_freq,
		      note_tick, note_duration))
    {
      iter = bsw_part_get_note (part, note_tick, note_freq);
      if (bsw_iter_n_left (iter))
        {
          pnote = bsw_iter_get_part_note (iter);
	  bsw_part_delete_note (part, pnote->tick, pnote->freq);
	  bsw_part_insert_note (part, new_tick, new_duration, pnote->freq, pnote->velocity);
	  bst_piano_roll_set_drag_data1 (proll, new_tick, new_duration, pnote->freq);
        }
      else /* eek, lost note during drag */
        {
          bst_piano_roll_set_pointer (proll, BST_PIANO_ROLL_POINTER_IGNORE);
          bst_status_set (BST_STATUS_ERROR, "Resize Note", "Lost Note");
          piano_update_cursor (self);
        }
      bsw_iter_free (iter);
    }
}

static void
piano_canvas_motion (BstPartDialog *self,
                     guint          button,
                     guint          tick,
                     gfloat         freq,
                     BstPianoRoll  *proll)
{
  switch (self->rtools->tool_id)
    {
    case TOOL_MOVE_RESIZE:
      if (button == 1)		/* resize */
        piano_resize (self, tick, freq, proll);
      else if (button == 2)	/* move */
        piano_move (self, tick, freq, proll);
      break;
    case TOOL_DELETE:
      if (button == 2)	/* move */
        piano_move (self, tick, freq, proll);
      break;
    }
}

static void
piano_canvas_release (BstPartDialog *self,
                      guint          button,
                      guint          tick,
                      gfloat         freq,
                      BstPianoRoll  *proll)
{
  gchar *action = NULL;

  switch (self->rtools->tool_id)
    {
    case TOOL_MOVE_RESIZE:
      if (button == 1)		/* resize */
	{
	  piano_resize (self, tick, freq, proll);
	  action = "Resize Note";
	}
      else if (button == 2)	/* move */
        {
          piano_move (self, tick, freq, proll);
          action = "Move Note";
        }
      break;
    case TOOL_DELETE:
      if (button == 2)	/* move */
	{
	  piano_move (self, tick, freq, proll);
	  action = "Move Note";
	}
      break;
    }

  if (!action)
    bst_status_set (BST_STATUS_ERROR, "Abortion", NULL);
  else
    bst_status_set (BST_STATUS_DONE, action, NULL);
  piano_update_cursor (self);
}
