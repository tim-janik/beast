// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstpartdialog.hh"
#include "bstprocedure.hh"
#include "bstmenus.hh"
#include "bstparam.hh"
#include "bstgrowbar.hh"

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
static gboolean part_dialog_action_check        (gpointer                data,
                                                 size_t                  action,
                                                 guint64                 action_stamp);
static void     part_dialog_action_exec         (gpointer                data,
                                                 size_t                  action);
static void     part_dialog_run_script_proc     (gpointer data, size_t action_id);


/* --- track actions --- */
enum {
  ACTION_NONE           = BST_COMMON_ROLL_TOOL_LAST,
  ACTION_CLEAR,
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
  { N_("Redo"),         "<ctrl>Y",   N_("Redo the last undone editing step"),               ACTION_REDO,       BST_STOCK_REDO },
};
static const GxkStockAction piano_clear_undo[] = {
  { N_("_Clear Undo"),          NULL,           NULL,
    ACTION_CLEAR_UNDO,          BST_STOCK_CLEAR_UNDO, },
};


/* --- functions --- */
G_DEFINE_TYPE (BstPartDialog, bst_part_dialog, GXK_TYPE_DIALOG);

static void
bst_part_dialog_class_init (BstPartDialogClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

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
      Bse::MidiSignal midi_signal_type = Aida::enum_value_from_string<Bse::MidiSignal> (sfi_value_get_choice (&param->value));
      bst_event_roll_set_control_type (self->eroll, midi_signal_type);
      gxk_widget_update_actions (self); /* update controllers */
    }
}

static void
bst_part_dialog_init (BstPartDialog *self)
{
  new (&self->project) Bse::ProjectH();
  GtkRange *srange;
  GxkActionList *al1;
  GtkObject *adjustment;
  GtkAdjustment *adj;
  GxkRadget *radget;

  /* configure self */
  g_object_set (self,
                "name", "BEAST-PartDialog",
                "flags", GXK_DIALOG_STATUS_BAR,
		NULL);
  gxk_dialog_set_sizes (GXK_DIALOG (self), 640, 400, 1005, 650);

  /* radget-complete GUI */
  radget = gxk_radget_create ("beast", "piano-roll-box", NULL);
  gtk_container_add (GTK_CONTAINER (GXK_DIALOG (self)->vbox), (GtkWidget*) radget);

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
  Bse::CategorySeq cseq = bse_server.category_match ("/Part/*");
  al1 = bst_action_list_from_cats (cseq, 1, BST_STOCK_EXECUTE, NULL, part_dialog_run_script_proc, self);
  gxk_action_list_sort (al1);
  gxk_widget_publish_action_list (self, "part-scripts", al1);

  BstGrowBar *grow_bar = (BstGrowBar*) gxk_radget_find (radget, "piano-roll-hgrow-bar");

  /* piano roll */
  self->proll = (BstPianoRoll*) gxk_radget_find (radget, "piano-roll");
  gxk_nullify_in_object (self, &self->proll);
  g_signal_connect (self->proll, "canvas-clicked", G_CALLBACK (piano_canvas_clicked), self);
  srange = (GtkRange*) gxk_radget_find (radget, "piano-roll-vscrollbar");
  gxk_scroll_canvas_set_vadjustment (GXK_SCROLL_CANVAS (self->proll), gtk_range_get_adjustment (srange));
  adj = bst_grow_bar_get_adjustment (grow_bar);
  gxk_scroll_canvas_set_hadjustment (GXK_SCROLL_CANVAS (self->proll), adj);
  self->pctrl = bst_piano_roll_controller_new (self->proll);
  gxk_widget_publish_action_list (self, "pctrl-select-actions", bst_piano_roll_controller_select_actions (self->pctrl));
  gxk_widget_publish_action_list (self, "pctrl-canvas-tools", bst_piano_roll_controller_canvas_actions (self->pctrl));
  gxk_widget_publish_action_list (self, "pctrl-note-tools", bst_piano_roll_controller_note_actions (self->pctrl));
  gxk_widget_publish_action_list (self, "pctrl-quant-tools", bst_piano_roll_controller_quant_actions (self->pctrl));

  /* event roll */
  self->eroll = (BstEventRoll*) gxk_radget_find (radget, "event-roll");
  gxk_nullify_in_object (self, &self->eroll);
  g_signal_connect (self->eroll, "canvas-clicked", G_CALLBACK (event_canvas_clicked), self);
  self->ectrl = bst_event_roll_controller_new (self->eroll, self->pctrl->quant_rtools, self->pctrl->canvas_rtools);
  adj = bst_grow_bar_get_adjustment (grow_bar);
  gxk_scroll_canvas_set_hadjustment (GXK_SCROLL_CANVAS (self->eroll), adj);
  bst_event_roll_set_vpanel_width_hook (self->eroll, (int (*) (void*)) bst_piano_roll_get_vpanel_width, self->proll);
  gxk_widget_publish_action_list (self, "ectrl-select-actions", bst_event_roll_controller_select_actions (self->ectrl));

  grow_bar = (BstGrowBar*) gxk_radget_find (radget, "pattern-view-vgrow-bar");

  /* pattern view */
  self->pview = (BstPatternView*) gxk_radget_find (radget, "pattern-view");
  gxk_nullify_in_object (self, &self->pview);
  srange = (GtkRange*) gxk_radget_find (radget, "pattern-view-hscrollbar");
  gxk_scroll_canvas_set_hadjustment (GXK_SCROLL_CANVAS (self->pview), gtk_range_get_adjustment (srange));
  adj = bst_grow_bar_get_adjustment (grow_bar);
  gxk_scroll_canvas_set_vadjustment (GXK_SCROLL_CANVAS (self->pview), adj);
  self->pvctrl = bst_pattern_controller_new (self->pview, self->pctrl->quant_rtools);

  /* pattern view controls */
  g_signal_connect_swapped (gxk_radget_find (radget, "configure-button"), "clicked",
                            G_CALLBACK (bst_pattern_column_layouter_popup), self->pview);
  // box = (GtkWidget*) gxk_radget_find (radget, "pattern-control-box");
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
  gxk_radget_add (radget, "pattern-control-box", gxk_vseparator_space_new (TRUE));
  gxk_radget_add (radget, "pattern-control-box", gxk_param_create_editor (self->pvctrl->row_shading, "name"));
  gxk_radget_add (radget, "pattern-control-box", gxk_param_create_editor (self->pvctrl->row_shading, "combo-button"));

  /* event roll children */
  g_object_new (GTK_TYPE_LABEL, "visible", TRUE, "label", "C", "parent", self->eroll, NULL);

  // event roll control type
  static GParamSpec *control_type_pspec =
    g_param_spec_ref (sfi_pspec_choice ("control_type", NULL, NULL, "MidiSignal::PITCH_BEND",
                                        Bse::choice_values_from_enum<Bse::MidiSignal>(),
                                        ":r:w:S:G:"));
  if (control_type_pspec)
    {
      GxkParam *param = gxk_param_new_value (control_type_pspec, eparam_changed, self);
      GtkWidget *rwidget = gxk_param_create_editor (param, "choice-button");
      gxk_radget_add (radget, "event-roll-control-area", rwidget);
      g_object_connect (radget, "swapped_signal::destroy", gxk_param_destroy, param, NULL);
      sfi_value_set_choice (&param->value, Aida::enum_value_to_string<Bse::MidiSignal> (Bse::MidiSignal::VELOCITY).c_str());
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

  bst_part_dialog_set_part (self);

  bst_piano_roll_controller_unref (self->pctrl);
  bst_event_roll_controller_unref (self->ectrl);

  G_OBJECT_CLASS (bst_part_dialog_parent_class)->finalize (object);
  using namespace Bse;
  self->project.~ProjectH();
}

void
bst_part_dialog_set_part (BstPartDialog *self, Bse::PartH part)
{
  assert_return (BST_IS_PART_DIALOG (self));

  if (self->project)
    {
      bse_proxy_disconnect (self->project.proxy_id(),
                            "any_signal", gxk_widget_update_actions_downwards, self,
                            NULL);
      self->project = Bse::ProjectH();
    }

  SfiProxy projectid = part ? bse_item_get_project (part.proxy_id()) : 0;
  Bse::ProjectH project = Bse::ProjectH::down_cast (bse_server.from_proxy (projectid));
  if (project)
    {
      bst_window_sync_title_to_proxy (GXK_DIALOG (self), part.proxy_id(), "%s");
      if (self->pview)
        bst_pattern_view_set_part (self->pview, part);
      if (self->proll)
        bst_piano_roll_set_part (self->proll, part);
      if (self->eroll)
        bst_event_roll_set_part (self->eroll, part);
      self->project = project;
      bse_proxy_connect (self->project.proxy_id(),
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
    gxk_menu_popup ((GtkMenu*) gxk_radget_find (self, "piano-popup"),
                    event->button.x_root, event->button.y_root,
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
    gxk_menu_popup ((GtkMenu*) gxk_radget_find (self, "event-popup"),
                    event->button.x_root, event->button.y_root,
                    event->button.button, event->button.time);
}

static void
part_dialog_run_script_proc (gpointer data, size_t action_id)
{
  BstPartDialog *self = BST_PART_DIALOG (data);
  Bse::Category cat = bst_category_find (g_quark_to_string (action_id));
  Bse::PartH part = self->proll->part;

  bst_procedure_exec_auto (cat.otype.c_str(),
                           "project", SFI_TYPE_PROXY, bse_item_get_project (part.proxy_id()),
                           "part", SFI_TYPE_PROXY, part.proxy_id(),
                           NULL);
}

static gboolean
part_dialog_action_check (gpointer data,
                          size_t   action,
                          guint64  action_stamp)
{
  BstPartDialog *self = BST_PART_DIALOG (data);
  switch (action)
    {
    case ACTION_CLEAR:
    case ACTION_CUT:
    case ACTION_COPY:
      return (bst_piano_roll_controller_has_selection (self->pctrl, action_stamp) ||
              bst_event_roll_controller_has_selection (self->ectrl, action_stamp));
    case ACTION_PASTE:
      return (bst_piano_roll_controller_clipboard_full (self->pctrl) ||
              bst_event_roll_controller_clipboard_full (self->ectrl));
    case ACTION_UNDO:
      return self->project && self->project.undo_depth() > 0;
    case ACTION_REDO:
      return self->project && self->project.redo_depth() > 0;
    case ACTION_CLEAR_UNDO:
      return self->project && self->project.undo_depth() + self->project.redo_depth() > 0;
      /* tools */
    case BST_COMMON_ROLL_TOOL_INSERT:
    case BST_COMMON_ROLL_TOOL_RESIZE:
    case BST_COMMON_ROLL_TOOL_MOVE:
    case BST_COMMON_ROLL_TOOL_DELETE:
    case BST_COMMON_ROLL_TOOL_ALIGN:
    case BST_COMMON_ROLL_TOOL_SELECT:
    case BST_COMMON_ROLL_TOOL_VSELECT:
      return TRUE;
    default:
      if (action >= BST_COMMON_ROLL_TOOL_FIRST)
        {
          return TRUE;
        }
      return FALSE;
    }
}

static void
part_dialog_action_exec (gpointer data,
                         size_t   action)
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
      bse_item_undo (self->proll->part.proxy_id());
      break;
    case ACTION_REDO:
      bse_item_redo (self->proll->part.proxy_id());
      break;
    case ACTION_CLEAR_UNDO:
      bse_item_clear_undo (self->proll->part.proxy_id());
      break;
    default:
      break;
    }

  gxk_status_window_pop ();

  gxk_widget_update_actions_downwards (self);
}
