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
#include "bstgrowbar.h"

#define SCROLLBAR_SPACING (3) /* from gtkscrolledwindow.c:DEFAULT_SCROLLBAR_SPACING */


/* --- prototypes --- */
static void	bst_part_dialog_finalize	(GObject		*object);
static void	piano_canvas_clicked		(BstPianoRoll           *proll,
						 guint			 button,
						 guint			 tick_position,
						 gint			 note,
						 GdkEvent		*event,
                                                 BstPartDialog		*self);
static void	event_canvas_clicked		(BstEventRoll           *eroll,
                                                 guint                   button,
                                                 guint                   tick_position,
                                                 gfloat                  value,
                                                 GdkEvent               *event,
                                                 BstPartDialog          *self);
static void     part_dialog_action_exec         (gpointer                data,
                                                 gulong                  action);
static gboolean part_dialog_action_check        (gpointer                data,
                                                 gulong                  action);
static void     part_dialog_run_script_proc     (gpointer                data,
                                                 gulong                  category_id);


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
static const GxkStockAction piano_clear_undo[] = {
  { N_("_Clear Undo"),          NULL,           NULL,
    ACTION_CLEAR_UNDO,          BST_STOCK_CLEAR_UNDO, },
};
     

/* --- functions --- */
G_DEFINE_TYPE (BstPartDialog, bst_part_dialog, GXK_TYPE_DIALOG);

static void
bst_part_dialog_class_init (BstPartDialogClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  
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
eparam_changed (gpointer  data,
                GxkParam *param)
{
  BstPartDialog *self = BST_PART_DIALOG (data);
  if (self->eroll)
    {
      BseMidiSignalType midi_signal_type = bse_midi_signal_type_from_choice (sfi_value_get_choice (&param->value));
      bst_event_roll_set_control_type (self->eroll, midi_signal_type);
    }
}

static void
bst_part_dialog_init (BstPartDialog *self)
{
  GtkWidget *eb, *child, *box;
  GtkRange *srange;
  BseCategorySeq *cseq;
  GxkActionList *al1;
  GtkPaned *paned;
  GtkObject *adjustment;
  GtkAdjustment *adj;
  GParamSpec *pspec;
  GxkRadget *radget;

  /* configure self */
  g_object_set (self,
                "name", "BEAST-PartDialog",
                "default_width", 600,
		"default_height", 450,
		"flags", GXK_DIALOG_STATUS_SHELL,
		NULL);

  /* radget-complete GUI */
  radget = gxk_radget_create ("beast", "piano-roll-box", NULL);
  gtk_container_add (GTK_CONTAINER (GXK_DIALOG (self)->vbox), radget);

  /* publish actions */
  gxk_widget_publish_actions (self, "piano-edit-actions",
                              G_N_ELEMENTS (piano_edit_actions), piano_edit_actions,
                              NULL, part_dialog_action_check, part_dialog_action_exec);
  gxk_widget_publish_actions (self, "piano-edit-undo",
                              G_N_ELEMENTS (piano_edit_undo), piano_edit_undo,
                              NULL, part_dialog_action_check, part_dialog_action_exec);
  if (BST_DVL_HINTS)
    gxk_widget_publish_actions (self, "piano-clear-undo", G_N_ELEMENTS (piano_clear_undo), piano_clear_undo,
                                NULL, part_dialog_action_check, part_dialog_action_exec);

  /* publish /Part/ scripts */
  cseq = bse_categories_match ("/Part/*");
  al1 = bst_action_list_from_cats (cseq, 1, BST_STOCK_EXECUTE, NULL, part_dialog_run_script_proc, self);
  gxk_action_list_sort (al1);
  gxk_widget_publish_action_list (self, "part-scripts", al1);

  /* FIXME: use paned child-properties after gtk+-2.4 */
  paned = gxk_radget_find (radget, "piano-paned");
  child = g_object_ref (paned->child1);
  gtk_container_remove (GTK_CONTAINER (paned), child);
  gtk_paned_pack1 (paned, child, TRUE, TRUE);
  g_object_unref (child);
  child = g_object_ref (paned->child2);
  gtk_container_remove (GTK_CONTAINER (paned), child);
  gtk_paned_pack2 (paned, child, FALSE, TRUE);
  g_object_unref (child);

  BstGrowBar *grow_bar = gxk_radget_find (radget, "piano-roll-hgrow-bar");

  /* piano roll */
  self->proll = gxk_radget_find (radget, "piano-roll");
  gxk_nullify_in_object (self, &self->proll);
  g_signal_connect (self->proll, "canvas-clicked", G_CALLBACK (piano_canvas_clicked), self);
  srange = gxk_radget_find (radget, "piano-roll-vscrollbar");
  gxk_scroll_canvas_set_vadjustment (GXK_SCROLL_CANVAS (self->proll), gtk_range_get_adjustment (srange));
  adj = bst_grow_bar_get_adjustment (grow_bar);
  gxk_scroll_canvas_set_hadjustment (GXK_SCROLL_CANVAS (self->proll), adj);
  self->pictrl = bst_piano_roll_controller_new (self->proll);
  gxk_widget_publish_action_list (self, "pctrl-canvas-tools", bst_piano_roll_controller_canvas_actions (self->pictrl));
  gxk_widget_publish_action_list (self, "pctrl-note-tools", bst_piano_roll_controller_note_actions (self->pictrl));
  gxk_widget_publish_action_list (self, "pctrl-quant-tools", bst_piano_roll_controller_quant_actions (self->pictrl));

  /* event roll */
  self->eroll = gxk_radget_find (radget, "event-roll");
  gxk_nullify_in_object (self, &self->eroll);
  g_signal_connect (self->eroll, "canvas-clicked", G_CALLBACK (event_canvas_clicked), self);
  self->ectrl = bst_event_roll_controller_new (self->eroll, self->pictrl->quant_rtools, self->pictrl->canvas_rtools);
  adj = bst_grow_bar_get_adjustment (grow_bar);
  gxk_scroll_canvas_set_hadjustment (GXK_SCROLL_CANVAS (self->eroll), adj);
  bst_event_roll_set_vpanel_width_hook (self->eroll, (gpointer) bst_piano_roll_get_vpanel_width, self->proll);

  grow_bar = gxk_radget_find (radget, "pattern-view-vgrow-bar");

  /* pattern view */
  self->pview = gxk_radget_find (radget, "pattern-view");
  gxk_nullify_in_object (self, &self->pview);
  srange = gxk_radget_find (radget, "pattern-view-hscrollbar");
  gxk_scroll_canvas_set_hadjustment (GXK_SCROLL_CANVAS (self->pview), gtk_range_get_adjustment (srange));
  adj = bst_grow_bar_get_adjustment (grow_bar);
  gxk_scroll_canvas_set_vadjustment (GXK_SCROLL_CANVAS (self->pview), adj);
  self->pvctrl = bst_pattern_controller_new (self->pview, self->pictrl->quant_rtools);

  /* pattern view controls */
  g_signal_connect_swapped (gxk_radget_find (radget, "configure-button"), "clicked",
                            G_CALLBACK (bst_pattern_column_layouter_popup), self->pview);
  box = gxk_radget_find (radget, "pattern-control-box");
  gxk_radget_add (radget, "pattern-control-box", gxk_param_create_editor (self->pvctrl->vraster, "name"));
  gxk_radget_add (radget, "pattern-control-box", gxk_param_create_editor (self->pvctrl->vraster, "combo-button"));
  gxk_radget_add (radget, "pattern-control-box", gxk_vseparator_space_new (TRUE));
  gxk_radget_add (radget, "pattern-control-box", gxk_param_create_editor (self->pvctrl->steps, "name"));
  gxk_radget_add (radget, "pattern-control-box", gxk_param_create_editor (self->pvctrl->steps, NULL));
  gxk_radget_add (radget, "pattern-control-box", gxk_vseparator_space_new (FALSE));
  gxk_radget_add (radget, "pattern-control-box", gxk_param_create_editor (self->pvctrl->step_dir, "name"));
  gxk_radget_add (radget, "pattern-control-box", gxk_param_create_editor (self->pvctrl->step_dir, "combo-button"));
  gxk_radget_add (radget, "pattern-control-box", gxk_vseparator_space_new (FALSE));
  gxk_radget_add (radget, "pattern-control-box", gxk_param_create_editor (self->pvctrl->hwrap, NULL));
  gxk_radget_add (radget, "pattern-control-box", gxk_vseparator_space_new (TRUE));
  gxk_radget_add (radget, "pattern-control-box", gxk_param_create_editor (self->pvctrl->base_octave, "name"));
  gxk_radget_add (radget, "pattern-control-box", gxk_param_create_editor (self->pvctrl->base_octave, NULL));

  /* event roll children */
  g_object_new (GTK_TYPE_LABEL, "visible", TRUE, "label", "C", "parent", self->eroll, NULL);

  /* event roll control type */
  eb = g_object_new (GTK_TYPE_VBOX, "spacing", SCROLLBAR_SPACING, NULL);
  pspec = bst_procedure_ref_pspec ("BsePart+change-control", "control-type");
  if (pspec)
    {
      GxkParam *param = gxk_param_new_value (pspec, eparam_changed, self);
      GtkWidget *rwidget = gxk_param_create_editor (param, "choice-button");
      gxk_radget_add (radget, "event-roll-control-area", rwidget);
      g_object_connect (radget, "swapped_signal::destroy", gxk_param_destroy, param, NULL);
      g_param_spec_unref (pspec);
      sfi_value_set_choice (&param->value, bse_midi_signal_type_to_choice (BSE_MIDI_SIGNAL_VELOCITY));
      gxk_param_apply_value (param); /* update model, auto updates GUI */
    }

  /* hzoom */
  adjustment = gtk_adjustment_new (13, 0, 100, 1, 5, 0);
  g_object_connect (adjustment,
		    "swapped_signal_after::value_changed", hzoom_changed, self,
		    NULL);
#if 0
  srange = gxk_radget_find (radget, "piano-roll-hzoom-scale");
  gtk_range_set_adjustment (srange, GTK_ADJUSTMENT (adjustment));
#endif
  gxk_radget_add (self, "hzoom-area",
                  g_object_new (GTK_TYPE_SPIN_BUTTON,
                                "visible", TRUE,
                                "adjustment", adjustment,
                                "digits", 1,
                                "width_request", 2 * gxk_size_width (GXK_ICON_SIZE_TOOLBAR),
                                NULL));
  /* vzoom */
  adjustment = gtk_adjustment_new (4, 1, 16, 1, 4, 0);
  g_object_connect (adjustment,
		    "swapped_signal_after::value_changed", vzoom_changed, self,
		    NULL);
  gxk_radget_add (self, "vzoom-area",
                  g_object_new (GTK_TYPE_SPIN_BUTTON,
                                "visible", TRUE,
                                "adjustment", adjustment,
                                "digits", 0,
                                "width_request", 2 * gxk_size_width (GXK_ICON_SIZE_TOOLBAR),
                                NULL));
}

static void
bst_part_dialog_finalize (GObject *object)
{
  BstPartDialog *self = BST_PART_DIALOG (object);

  bst_part_dialog_set_proxy (self, 0);

  bst_piano_roll_controller_unref (self->pictrl);
  bst_event_roll_controller_unref (self->ectrl);
  
  G_OBJECT_CLASS (bst_part_dialog_parent_class)->finalize (object);
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
      if (self->pview)
        bst_pattern_view_set_proxy (self->pview, part);
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
piano_canvas_clicked (BstPianoRoll           *proll,
                      guint                   button,
                      guint                   tick_position,
                      gint                    note,
                      GdkEvent               *event,
                      BstPartDialog          *self)
{
  if (button == 3 && event)
    gxk_menu_popup (gxk_radget_find (self, "piano-popup"),
                    event->button.x_root, event->button.y_root, FALSE,
                    event->button.button, event->button.time);
}

static void
event_canvas_clicked (BstEventRoll           *eroll,
                      guint                   button,
                      guint                   tick_position,
                      gfloat                  value,
                      GdkEvent               *event,
                      BstPartDialog          *self)
{
  if (button == 3 && event)
    gxk_menu_popup (gxk_radget_find (self, "event-popup"),
                    event->button.x_root, event->button.y_root, FALSE,
                    event->button.button, event->button.time);
}

static void
part_dialog_run_script_proc (gpointer                data,
                             gulong                  category_id)
{
  BstPartDialog *self = BST_PART_DIALOG (data);
  BseCategory *cat = bse_category_from_id (category_id);
  SfiProxy part = self->proll->proxy;

  bst_procedure_exec_auto (cat->type,
                           "project", SFI_TYPE_PROXY, bse_item_get_project (part),
                           "part", SFI_TYPE_PROXY, part,
                           NULL);
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
      bst_piano_roll_controller_clear (self->pictrl);
      bst_event_roll_controller_clear (self->ectrl);
      break;
    case ACTION_CUT:
      bst_piano_roll_controller_cut (self->pictrl);
      bst_event_roll_controller_cut (self->ectrl);
      break;
    case ACTION_COPY:
      if (!bst_piano_roll_controller_copy (self->pictrl))
        bst_event_roll_controller_copy (self->ectrl);
      break;
    case ACTION_PASTE:
      bst_piano_roll_controller_paste (self->pictrl);
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
      return (bst_piano_roll_controler_clipboard_full (self->pictrl) ||
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
