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
#include "bstpartdialog.h"
#include "bstprocedure.h"
#include "bstmenus.h"
#include "bstparam.h"

#define SCROLLBAR_SPACING (3) /* from gtkscrolledwindow.c:DEFAULT_SCROLLBAR_SPACING */


/* --- prototypes --- */
static void	bst_part_dialog_class_init	(BstPartDialogClass	*klass);
static void	bst_part_dialog_init		(BstPartDialog		*part_dialog);
static void	bst_part_dialog_finalize	(GObject		*object);
static void	piano_canvas_clicked		(BstPartDialog		*part_dialog,
						 guint			 button,
						 guint			 tick_position,
						 gint			 note,
						 GdkEvent		*event,
						 BstPianoRoll		*proll);
static void     part_dialog_action_exec         (gpointer                data,
                                                 gulong                  action);
static gboolean part_dialog_action_check        (gpointer                data,
                                                 gulong                  action);


/* --- track actions --- */
enum {
  ACTION_CLEAR = 1,
  ACTION_CUT,
  ACTION_COPY,
  ACTION_PASTE,
  ACTION_UNDO,
  ACTION_REDO,
  ACTION_CLEAR_UNDO
};
static const GxkStockAction piano_edit_actions[] = {
  { N_("Clear"),        "<ctrl>K",   N_("Clear the current selection"),                     ACTION_CLEAR,      BST_STOCK_TRASH_SCISSORS },
  { N_("Cut"),          "<ctrl>X",   N_("Move the current selection into clipboard"),       ACTION_CUT,        BST_STOCK_MUSIC_CUT },
  { N_("Copy"),         "<ctrl>C",   N_("Copy the current selection into clipboard"),       ACTION_COPY,       BST_STOCK_MUSIC_COPY },
  { N_("Paste"),        "<ctrl>V",   N_("Insert clipboard contents as current selection"),  ACTION_PASTE,      BST_STOCK_MUSIC_PASTE },
};
static const GxkStockAction piano_edit_undo[] = {
  { N_("Undo"),         "<ctrl>Z",   N_("Undo last editing step"),                          ACTION_UNDO,       BST_STOCK_UNDO },
  { N_("Redo"),         "<ctrl>R",   N_("Redo the last undone editing step"),               ACTION_REDO,       BST_STOCK_REDO },
};


/* --- variables --- */
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
      type = g_type_register_static (GXK_TYPE_DIALOG,
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
}

static void
hzoom_changed (BstPartDialog *self,
	       GtkAdjustment *adjustment)
{
  if (self->proll)
    bst_piano_roll_set_hzoom (self->proll, adjustment->value * 0.08);
  if (self->eroll)
    bst_event_roll_set_hzoom (self->eroll, adjustment->value * 0.08);
}

static void
vzoom_changed (BstPartDialog *self,
	       GtkAdjustment *adjustment)
{
  if (self->proll)
    bst_piano_roll_set_vzoom (self->proll, adjustment->value);
}

static void
eparam_changed (gpointer         data,
                BstParam        *bparam)
{
  BstPartDialog *self = BST_PART_DIALOG (data);
  if (self->eroll)
    {
      BseMidiSignalType midi_signal_type = bse_midi_signal_type_from_choice (sfi_value_get_choice (&bparam->value));
      bst_event_roll_set_control_type (self->eroll, midi_signal_type);
    }
}

static void
bst_part_dialog_init (BstPartDialog *self)
{
  GtkWidget *entry, *button, *hscroll, *vscroll, *eb, *child;
  GtkPaned *paned;
  GtkObject *adjustment;
  GParamSpec *pspec;
  GxkGadget *gadget;

  /* configure self */
  g_object_set (self,
                "name", "BEAST-PartDialog",
                "default_width", 600,
		"default_height", 450,
		"flags", GXK_DIALOG_STATUS_SHELL,
		NULL);

  /* gadget-complete GUI */
  gadget = gxk_gadget_create ("beast", "piano-roll-box", NULL);
  gtk_container_add (GTK_CONTAINER (GXK_DIALOG (self)->vbox), gadget);
  hscroll = gxk_gadget_find (gadget, "piano-roll-hscrollbar");
  vscroll = gxk_gadget_find (gadget, "piano-roll-vscrollbar");

  /* publish actions */
  gxk_widget_publish_actions (self, "piano-edit-actions",
                              G_N_ELEMENTS (piano_edit_actions), piano_edit_actions,
                              NULL, part_dialog_action_check, part_dialog_action_exec);
  gxk_widget_publish_actions (self, "piano-edit-undo",
                              G_N_ELEMENTS (piano_edit_undo), piano_edit_undo,
                              NULL, part_dialog_action_check, part_dialog_action_exec);

  /* FIXME: use paned child-properties after gtk+-2.4 */
  paned = gxk_gadget_find (gadget, "piano-paned");
  child = g_object_ref (paned->child1);
  gtk_container_remove (GTK_CONTAINER (paned), child);
  gtk_paned_pack1 (paned, child, TRUE, TRUE);
  g_object_unref (child);
  child = g_object_ref (paned->child2);
  gtk_container_remove (GTK_CONTAINER (paned), child);
  gtk_paned_pack2 (paned, child, FALSE, TRUE);
  g_object_unref (child);
  
  /* piano roll */
  self->proll = gxk_gadget_find (gadget, "piano-roll");
  gxk_nullify_on_destroy (self->proll, &self->proll);
  g_object_connect (self->proll,
                    "swapped_signal::canvas-clicked", piano_canvas_clicked, self,
                    NULL);
  bst_piano_roll_set_hadjustment (self->proll, gtk_range_get_adjustment (GTK_RANGE (hscroll)));
  bst_piano_roll_set_vadjustment (self->proll, gtk_range_get_adjustment (GTK_RANGE (vscroll)));
  self->pctrl = bst_piano_roll_controller_new (self->proll);
  gxk_widget_publish_action_list (self, "pctrl-canvas-tools", bst_piano_roll_controller_canvas_actions (self->pctrl));
  gxk_widget_publish_action_list (self, "pctrl-note-tools", bst_piano_roll_controller_note_actions (self->pctrl));
  gxk_widget_publish_action_list (self, "pctrl-quant-tools", bst_piano_roll_controller_quant_actions (self->pctrl));

  /* event roll */
  self->eroll = gxk_gadget_find (gadget, "event-roll");
  gxk_nullify_on_destroy (self->eroll, &self->eroll);
  self->ectrl = bst_event_roll_controller_new (self->eroll, self->pctrl->quant_atools, self->pctrl->canvas_atools);
  bst_event_roll_set_hadjustment (self->eroll, gtk_range_get_adjustment (GTK_RANGE (hscroll)));
  bst_event_roll_set_vpanel_width_hook (self->eroll, (gpointer) bst_piano_roll_get_vpanel_width, self->proll);

  /* event roll children */
  g_object_new (GTK_TYPE_LABEL, "label", "C", "parent", self->eroll, NULL);

  /* event roll control type */
  eb = g_object_new (GTK_TYPE_VBOX, "spacing", SCROLLBAR_SPACING, NULL);
  pspec = bst_procedure_ref_pspec ("BsePart+change-control", "control-type");
  if (pspec)
    {
      BstParam *bparam = bst_param_value_create (pspec, TRUE, NULL, eparam_changed, self);
      GtkWidget *rwidget = bst_param_rack_widget (bparam);
      gxk_gadget_add (gadget, "event-roll-control-area", rwidget);
      g_object_connect (rwidget, "swapped_signal::destroy", bst_param_destroy, bparam, NULL);
      g_param_spec_unref (pspec);
      sfi_value_set_choice (&bparam->value, bse_midi_signal_type_to_choice (BSE_MIDI_SIGNAL_VELOCITY));
      bst_param_update (bparam); /* update GUI */
      bst_param_apply_value (bparam); /* update model */
    }

  /* create toolbar */
  self->toolbar = gxk_toolbar_new (&self->toolbar);
  gxk_gadget_add (gadget, "toolbar-area", self->toolbar);

  /* add note tools to toolbar */
  // bst_radio_tools_build_toolbar (self->pctrl->canvas_rtools, self->toolbar);
  // gxk_widget_activate_accel_group (GTK_WIDGET (self), self->pctrl->canvas_rtools->accel_group);
  
  /* selection ops (copy/cut/...) */
  gxk_toolbar_append_separator (self->toolbar);
  button = gxk_toolbar_append_stock (self->toolbar, GXK_TOOLBAR_BUTTON, "Clear", "Clear the current selection", BST_STOCK_TRASH_SCISSORS);
  g_object_connect (button, "swapped_signal::clicked", bst_piano_roll_controller_clear, self->pctrl, NULL);
  g_object_connect (button, "swapped_signal::clicked", bst_event_roll_controller_clear, self->ectrl, NULL);
  button = gxk_toolbar_append_stock (self->toolbar, GXK_TOOLBAR_BUTTON, "Cut", "Move the current selection into clipboard", BST_STOCK_MUSIC_CUT);
  g_object_connect (button, "swapped_signal::clicked", bst_piano_roll_controller_cut, self->pctrl, NULL);
  g_object_connect (button, "swapped_signal::clicked", bst_event_roll_controller_cut, self->ectrl, NULL);
  button = gxk_toolbar_append_stock (self->toolbar, GXK_TOOLBAR_BUTTON, "Copy", "Copy the current selection into clipboard", BST_STOCK_MUSIC_COPY);
  g_object_connect (button, "swapped_signal::clicked", bst_piano_roll_controller_copy, self->pctrl, NULL);
  g_object_connect (button, "swapped_signal::clicked", bst_event_roll_controller_copy, self->ectrl, NULL);
  button = gxk_toolbar_append_stock (self->toolbar, GXK_TOOLBAR_BUTTON, "Paste", "Insert clipboard contents as current selection", BST_STOCK_MUSIC_PASTE);
  g_object_connect (button, "swapped_signal::clicked", bst_piano_roll_controller_paste, self->pctrl, NULL);
  g_object_connect (button, "swapped_signal::clicked", bst_event_roll_controller_paste, self->ectrl, NULL);

  /* add note-length choice to toolbar */
  //bst_radio_tools_build_toolbar_choice (self->pctrl->note_rtools, self->toolbar);
  //gxk_widget_activate_accel_group (GTK_WIDGET (self), self->pctrl->note_rtools->accel_group);
  
  /* add quantization selection to toolbar */
  //bst_radio_tools_build_toolbar_choice (self->pctrl->quant_rtools, self->toolbar);
  //gxk_widget_activate_accel_group (GTK_WIDGET (self), self->pctrl->quant_rtools->accel_group);
  
  /* hzoom */
  gxk_toolbar_append_separator (self->toolbar);
  adjustment = gtk_adjustment_new (13, 0, 100, 1, 5, 0);
  g_object_connect (adjustment,
		    "swapped_signal_after::value_changed", hzoom_changed, self,
		    NULL);
  entry = g_object_new (GTK_TYPE_SPIN_BUTTON,
			"visible", TRUE,
			"adjustment", adjustment,
			"digits", 1,
			"width_request", 2 * gxk_size_width (BST_SIZE_TOOLBAR),
			NULL);
  gxk_toolbar_append (self->toolbar, GXK_TOOLBAR_EXTRA_WIDGET,
		      "HZoom", "Horizontal Zoom", entry);

  /* vzoom */
  adjustment = gtk_adjustment_new (4, 1, 16, 1, 4, 0);
  g_object_connect (adjustment,
		    "swapped_signal_after::value_changed", vzoom_changed, self,
		    NULL);
  entry = g_object_new (GTK_TYPE_SPIN_BUTTON,
			"visible", TRUE,
			"adjustment", adjustment,
			"digits", 0,
			"width_request", 2 * gxk_size_width (BST_SIZE_TOOLBAR),
			NULL);
  gxk_toolbar_append (self->toolbar, GXK_TOOLBAR_EXTRA_WIDGET,
		      "VZoom", "Vertical Zoom", entry);
}

static void
bst_part_dialog_finalize (GObject *object)
{
  BstPartDialog *self = BST_PART_DIALOG (object);

  bst_part_dialog_set_proxy (self, 0);

  bst_piano_roll_controller_unref (self->pctrl);
  bst_event_roll_controller_unref (self->ectrl);
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

void
bst_part_dialog_set_proxy (BstPartDialog *self,
			   SfiProxy       part)
{
  SfiProxy project;

  g_return_if_fail (BST_IS_PART_DIALOG (self));
  if (part)
    g_return_if_fail (BSE_IS_PART (part));

  if (self->project)
    {
      bse_proxy_disconnect (self->project,
                            "any_signal", gxk_widget_update_actions_downwards, self,
                            NULL);
      self->project = 0;
    }

  project = part ? bse_item_get_project (part) : 0;

  if (project)
    {
      bst_window_sync_title_to_proxy (GXK_DIALOG (self), part, "%s");
      if (self->proll)
        bst_piano_roll_set_proxy (self->proll, part);
      if (self->eroll)
        bst_event_roll_set_proxy (self->eroll, part);
      self->project = project;
      bse_proxy_connect (self->project,
                         "swapped_signal::property-notify::dirty", gxk_widget_update_actions_downwards, self,
                         NULL);
    }

  gxk_widget_update_actions_downwards (self);
}

static void
piano_canvas_clicked (BstPartDialog *self,
		      guint          button,
		      guint          tick,
		      gint           note,
		      GdkEvent      *event,
		      BstPianoRoll  *proll)
{
  if (button == 3 && event)
    gxk_menu_popup (gxk_gadget_find (self, "piano-popup"),
                    event->button.x_root, event->button.y_root, FALSE,
                    event->button.button, event->button.time);
}

static void
part_dialog_action_exec (gpointer data,
                         gulong   action)
{
  BstPartDialog *self = BST_PART_DIALOG (data);

  gxk_status_window_push (self);

  switch (action)
    {
    case ACTION_CLEAR:
      bst_piano_roll_controller_clear (self->pctrl);
      bst_event_roll_controller_clear (self->ectrl);
      break;
    case ACTION_CUT:
      bst_piano_roll_controller_cut (self->pctrl);
      bst_event_roll_controller_cut (self->ectrl);
      break;
    case ACTION_COPY:
      if (!bst_piano_roll_controller_copy (self->pctrl))
        bst_event_roll_controller_copy (self->ectrl);
      break;
    case ACTION_PASTE:
      bst_piano_roll_controller_paste (self->pctrl);
      bst_event_roll_controller_paste (self->ectrl);
      break;
    case ACTION_UNDO:
      bse_item_undo (self->proll->proxy);
      break;
    case ACTION_REDO:
      bse_item_redo (self->proll->proxy);
      break;
    case ACTION_CLEAR_UNDO:
      bse_item_clear_undo (self->proll->proxy);
      break;
    default:
      if (action >= BST_GENERIC_ROLL_TOOL_FIRST)
        {
          // FIXME: if (self->pctrl && self->pctrl->canvas_rtools) bst_radio_tools_set_tool (self->pctrl->canvas_rtools, action);
        }
      break;
    }

  gxk_status_window_pop ();

  gxk_widget_update_actions_downwards (self);
}

static gboolean
part_dialog_action_check (gpointer data,
                          gulong   action)
{
  BstPartDialog *self = BST_PART_DIALOG (data);
  switch (action)
    {
    case ACTION_CLEAR:
    case ACTION_CUT:
    case ACTION_COPY:
      return TRUE;
    case ACTION_PASTE:
      return (bst_piano_roll_controler_clipboard_full (self->pctrl) ||
              bst_event_roll_controler_clipboard_full (self->ectrl));
    case ACTION_UNDO:
      return self->project && bse_project_undo_depth (self->project) > 0;
    case ACTION_REDO:
      return self->project && bse_project_redo_depth (self->project) > 0;
    case ACTION_CLEAR_UNDO:
      return self->project && bse_project_undo_depth (self->project) + bse_project_redo_depth (self->project) > 0;
      /* tools */
    case BST_GENERIC_ROLL_TOOL_INSERT:
    case BST_GENERIC_ROLL_TOOL_RESIZE:
    case BST_GENERIC_ROLL_TOOL_MOVE:
    case BST_GENERIC_ROLL_TOOL_DELETE:
    case BST_GENERIC_ROLL_TOOL_ALIGN:
    case BST_GENERIC_ROLL_TOOL_SELECT:
    case BST_GENERIC_ROLL_TOOL_VSELECT:
      return TRUE;
    default:
      if (action >= BST_GENERIC_ROLL_TOOL_FIRST)
        {
          return TRUE;
        }
      return FALSE;
    }
}
