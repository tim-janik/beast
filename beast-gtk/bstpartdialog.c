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
#include "bstactivatable.h"
#include "bstmenus.h"


/* --- prototypes --- */
static void	bst_part_dialog_class_init	(BstPartDialogClass	*klass);
static void	bst_part_dialog_init		(BstPartDialog		*part_dialog);
static void	bst_part_dialog_finalize	(GObject		*object);
static void	part_dialog_update_tool		(BstPartDialog		*part_dialog);
static void	piano_canvas_clicked		(BstPartDialog		*part_dialog,
						 guint			 button,
						 guint			 tick_position,
						 gint			 note,
						 GdkEvent		*event,
						 BstPianoRoll		*proll);
static void	part_dialog_run_proc		(GtkWidget		*widget,
						 gulong                  category_id,
						 gpointer		 popup_data);
static void	part_dialog_note_choice		(BstPartDialog		*self,
						 guint			 choice);
static void	part_dialog_qnote_choice	(BstPartDialog		*self,
						 guint			 choice);
static void	menu_select_tool		(GtkWidget              *owner,
						 gulong                  callback_action,
						 gpointer                popup_data);
static void     bst_part_dialog_activate        (BstActivatable         *activatable,
                                                 gulong                  action);
static gboolean bst_part_dialog_can_activate    (BstActivatable         *activatable,
                                                 gulong                  action);
static void     bst_part_dialog_update_activatable (BstActivatable *activatable);
static void	menu_activate_tool		(GtkWidget              *owner,
						 gulong                  callback_action,
						 gpointer                popup_data);


/* --- variables --- */
enum {
  ACTION_CLEAR,
  ACTION_CUT,
  ACTION_COPY,
  ACTION_PASTE,
  ACTION_UNDO,
  ACTION_REDO,
  ACTION_CLEAR_UNDO
};
static BstMenuConfigEntry popup_entries[] =
{
#define MENU_CB(xxx)	menu_select_tool, BST_PIANO_ROLL_TOOL_ ## xxx
#define ACTION_CB(xxx)	menu_activate_tool, ACTION_ ## xxx
  { "/_Tools",		NULL,		NULL,   0,		"<Branch>",	0 },
  { "/Tools/Insert",	"I",		MENU_CB (INSERT),	"<StockItem>",	BST_STOCK_PART_TOOL },
  { "/Tools/Delete",	"D",		MENU_CB (DELETE),	"<StockItem>",	BST_STOCK_TRASHCAN },
  { "/Tools/Select",	"S",		MENU_CB (SELECT),	"<StockItem>",	BST_STOCK_RECT_SELECT },
  { "/Tools/Vertical Select",	"V",	MENU_CB (VSELECT),	"<StockItem>",	BST_STOCK_VERT_SELECT },
  { "/_Edit",		NULL,		NULL,   0,		"<Branch>",	0 },
  { "/Edit/Cut",	"<ctrl>X",	ACTION_CB (CUT),	"<StockItem>",	BST_STOCK_MUSIC_CUT },
  { "/Edit/Copy",	"<ctrl>C",	ACTION_CB (COPY),	"<StockItem>",	BST_STOCK_MUSIC_COPY },
  { "/Edit/Paste",	"<ctrl>V",	ACTION_CB (PASTE),	"<StockItem>",	BST_STOCK_MUSIC_PASTE },
  { "/Edit/Clear",	"<ctrl>K",	ACTION_CB (CLEAR),	"<StockItem>",	BST_STOCK_TRASH_SCISSORS },
  { "/Edit/-----1",	NULL,     	NULL,   0,           	"<Separator>",	0 },
  { "/Edit/Undo",	"<ctrl>Z",      ACTION_CB (UNDO),	"<StockItem>",	BST_STOCK_UNDO },
  { "/Edit/Redo",	"<ctrl>R",      ACTION_CB (REDO),	"<StockItem>",	BST_STOCK_REDO },
  { "/Edit/Clear Undo",	NULL,	        ACTION_CB (CLEAR_UNDO),	"<Item>",	0 },
  { "/-----1",		NULL,		NULL,	0,		"<Separator>",	0 },
  { "/Scripts",		NULL,		NULL,   0,		"<Title>",	0 },
  { "/Test",		NULL,		NULL,	0,		"<Branch>",	0 },
};
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
      bst_type_implement_activatable (type,
                                      bst_part_dialog_activate,
                                      bst_part_dialog_can_activate,
                                      bst_part_dialog_update_activatable);
    }
  
  return type;
}

static void
bst_part_dialog_class_init (BstPartDialogClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseCategorySeq *cseq;
  BstMenuConfig *m1, *m2;

  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->finalize = bst_part_dialog_finalize;
  
  /* create item factory for menu entries and categories */
  class->popup_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<BstPartDialog>", NULL);

  /* standard entries */
  m1 = bst_menu_config_from_entries (G_N_ELEMENTS (popup_entries), popup_entries);
  /* procedures */
  cseq = bse_categories_match_typed ("/Scripts/*", "BseProcedure");
  m2 = bst_menu_config_from_cats (cseq, part_dialog_run_proc, 1);
  bst_menu_config_sort (m2);
  /* merge */
  m1 = bst_menu_config_merge (m1, m2);
  /* and create menu items */
  bst_menu_config_create_items (m1, class->popup_factory, NULL);
  bst_menu_config_free (m1);
}

static void
hzoom_changed (BstPartDialog *self,
	       GtkAdjustment *adjustment)
{
  if (self->proll)
    bst_piano_roll_set_hzoom (self->proll, adjustment->value * 0.08);
}

static void
vzoom_changed (BstPartDialog *self,
	       GtkAdjustment *adjustment)
{
  if (self->proll)
    bst_piano_roll_set_vzoom (self->proll, adjustment->value);
}

static void
bst_part_dialog_init (BstPartDialog *self)
{
  BstPartDialogClass *class = BST_PART_DIALOG_GET_CLASS (self);
  GtkWidget *main_vbox, *entry, *choice, *button;
  GtkObject *adjustment;

  /* configure self */
  g_object_set (self,
		"default_width", 600,
		"default_height", 450,
		"flags", GXK_DIALOG_STATUS_SHELL,
		NULL);
  main_vbox = GXK_DIALOG (self)->vbox;

  /* create toolbar */
  self->toolbar = gxk_toolbar_new (&self->toolbar);
  gtk_box_pack_start (GTK_BOX (main_vbox), GTK_WIDGET (self->toolbar), FALSE, TRUE, 0);

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
  gxk_nullify_on_destroy (self->proll, &self->proll);
  self->proll_ctrl = bst_piano_roll_controller_new (self->proll);
  g_object_connect (self->proll,
		    "swapped_signal::destroy", g_nullify_pointer, &self->proll,
		    "swapped_signal::canvas-clicked", piano_canvas_clicked, self,
		    NULL);

  /* radio tools */
  self->rtools = bst_radio_tools_new ();
  g_object_connect (self->rtools,
		    "swapped_signal::set_tool", part_dialog_update_tool, self,
		    NULL);
  bst_radio_tools_set_tool (self->rtools, BST_PIANO_ROLL_TOOL_INSERT);

  /* register tools */
  bst_radio_tools_add_stock_tool (self->rtools, BST_PIANO_ROLL_TOOL_INSERT,
				  "Insert", "Insert/resize/move notes (mouse button 1 and 2)", NULL,
				  BST_STOCK_PART_TOOL, BST_RADIO_TOOLS_EVERYWHERE);
  bst_radio_tools_add_stock_tool (self->rtools, BST_PIANO_ROLL_TOOL_DELETE,
				  "Delete", "Delete note (mouse button 1)", NULL,
				  BST_STOCK_TRASHCAN, BST_RADIO_TOOLS_EVERYWHERE);
  bst_radio_tools_add_stock_tool (self->rtools, BST_PIANO_ROLL_TOOL_SELECT,
				  "Select", "Rectangle select notes", NULL,
				  BST_STOCK_RECT_SELECT, BST_RADIO_TOOLS_EVERYWHERE);
  bst_radio_tools_add_stock_tool (self->rtools, BST_PIANO_ROLL_TOOL_VSELECT,
				  "VSelect", "Select tick range vertically", NULL,
				  BST_STOCK_VERT_SELECT, BST_RADIO_TOOLS_EVERYWHERE);
  bst_radio_tools_build_toolbar (self->rtools, self->toolbar);

  /* selection ops (copy/cut/...) */
  gxk_toolbar_append_separator (self->toolbar);
  button = gxk_toolbar_append_stock (self->toolbar, GXK_TOOLBAR_BUTTON, "Clear", "Clear the current selection", BST_STOCK_TRASH_SCISSORS);
  g_object_connect (button, "swapped_signal::clicked", bst_piano_roll_controller_clear, self->proll_ctrl, NULL);
  button = gxk_toolbar_append_stock (self->toolbar, GXK_TOOLBAR_BUTTON, "Cut", "Move the current selection into clipboard", BST_STOCK_MUSIC_CUT);
  g_object_connect (button, "swapped_signal::clicked", bst_piano_roll_controller_cut, self->proll_ctrl, NULL);
  button = gxk_toolbar_append_stock (self->toolbar, GXK_TOOLBAR_BUTTON, "Copy", "Copy the current selection into clipboard", BST_STOCK_MUSIC_COPY);
  g_object_connect (button, "swapped_signal::clicked", bst_piano_roll_controller_copy, self->proll_ctrl, NULL);
  button = gxk_toolbar_append_stock (self->toolbar, GXK_TOOLBAR_BUTTON, "Paste", "Insert clipboard contents as current selection", BST_STOCK_MUSIC_PASTE);
  g_object_connect (button, "swapped_signal::clicked", bst_piano_roll_controller_paste, self->proll_ctrl, NULL);

  /* note selection */
  gxk_toolbar_append_separator (self->toolbar);
  choice = gxk_toolbar_append_choice (self->toolbar, GXK_TOOLBAR_TRUNC_BUTTON,
				      (GxkToolbarChoiceFunc) part_dialog_note_choice, self, NULL);
  gxk_toolbar_choice_add (choice, "1/1", "Insert full notes",
			  gxk_stock_image (BST_STOCK_NOTE_1, BST_SIZE_TOOLBAR), 1);
  gxk_toolbar_choice_add (choice, "1/2", "Insert half notes",
			  gxk_stock_image (BST_STOCK_NOTE_2, BST_SIZE_TOOLBAR), 2);
  gxk_toolbar_choice_set (choice, "1/4", "Insert quarter notes",
			  gxk_stock_image (BST_STOCK_NOTE_4, BST_SIZE_TOOLBAR), 4);
  gxk_toolbar_choice_add (choice, "1/8", "Insert eighths note",
			  gxk_stock_image (BST_STOCK_NOTE_8, BST_SIZE_TOOLBAR), 8);
  gxk_toolbar_choice_add (choice, "1/16", "Insert sixteenth note",
			  gxk_stock_image (BST_STOCK_NOTE_16, BST_SIZE_TOOLBAR), 16);
  
  /* quantization selection */
  choice = gxk_toolbar_append_choice (self->toolbar, GXK_TOOLBAR_TRUNC_BUTTON,
				      (GxkToolbarChoiceFunc) part_dialog_qnote_choice, self, NULL);
  gxk_toolbar_choice_add (choice, "None", "No quantization selected",
			  gxk_stock_image (BST_STOCK_QNOTE_NONE, BST_SIZE_TOOLBAR), 0);
  gxk_toolbar_choice_add (choice, "Q: 1/1", "Quantize to full note boundaries",
			  gxk_stock_image (BST_STOCK_QNOTE_1, BST_SIZE_TOOLBAR), 1);
  gxk_toolbar_choice_add (choice, "Q: 1/2", "Quantize to half note boundaries",
			  gxk_stock_image (BST_STOCK_QNOTE_2, BST_SIZE_TOOLBAR), 2);
  gxk_toolbar_choice_add (choice, "Q: 1/4", "Quantize to quarter note boundaries",
			  gxk_stock_image (BST_STOCK_QNOTE_4, BST_SIZE_TOOLBAR), 4);
  gxk_toolbar_choice_set (choice, "Q: 1/8", "Quantize to eighths note boundaries",
			  gxk_stock_image (BST_STOCK_QNOTE_8, BST_SIZE_TOOLBAR), 8);
  gxk_toolbar_choice_add (choice, "Q: 1/16", "Quantize to sixteenth note boundaries",
			  gxk_stock_image (BST_STOCK_QNOTE_16, BST_SIZE_TOOLBAR), 16);

  /* vzoom */
  gxk_toolbar_append_separator (self->toolbar);
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

  /* hzoom */
  // gxk_toolbar_append_space (self->toolbar);
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

  bst_part_dialog_set_proxy (self, 0);

  g_object_unref (self->rtools);
  bst_piano_roll_controller_unref (self->proll_ctrl);
  
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
                            "any_signal", bst_widget_update_activatable, self,
                            NULL);
      self->project = 0;
    }

  project = part ? bse_item_get_project (part) : 0;

  if (project)
    {
      bst_window_sync_title_to_proxy (GXK_DIALOG (self), part, "%s");
      if (self->proll)
        bst_piano_roll_set_proxy (self->proll, part);
      self->project = project;
      bse_proxy_connect (self->project,
                         "swapped_signal::property-notify::dirty", bst_widget_update_activatable, self,
                         NULL);
    }

  bst_widget_update_activatable (self);
}

static void
part_dialog_update_tool (BstPartDialog *self)
{
  switch (self->rtools->tool_id)
    {
    case BST_PIANO_ROLL_TOOL_INSERT:
      bst_piano_roll_controller_set_obj_tools (self->proll_ctrl,
					       BST_PIANO_ROLL_TOOL_RESIZE,
					       BST_PIANO_ROLL_TOOL_MOVE,
					       BST_PIANO_ROLL_TOOL_NONE);
      bst_piano_roll_controller_set_bg_tools (self->proll_ctrl,
					      BST_PIANO_ROLL_TOOL_INSERT,
					      BST_PIANO_ROLL_TOOL_MOVE,		/* error */
					      BST_PIANO_ROLL_TOOL_NONE);
      break;
    case BST_PIANO_ROLL_TOOL_DELETE:
      bst_piano_roll_controller_set_obj_tools (self->proll_ctrl,
					       BST_PIANO_ROLL_TOOL_DELETE,
					       BST_PIANO_ROLL_TOOL_MOVE,
					       BST_PIANO_ROLL_TOOL_NONE);
      bst_piano_roll_controller_set_bg_tools (self->proll_ctrl,
					      BST_PIANO_ROLL_TOOL_DELETE,	/* error */
					      BST_PIANO_ROLL_TOOL_MOVE,		/* error */
					      BST_PIANO_ROLL_TOOL_NONE);
      break;
    case BST_PIANO_ROLL_TOOL_SELECT:
      bst_piano_roll_controller_set_obj_tools (self->proll_ctrl,
					       BST_PIANO_ROLL_TOOL_SELECT,
					       BST_PIANO_ROLL_TOOL_MOVE,
					       BST_PIANO_ROLL_TOOL_NONE);
      bst_piano_roll_controller_set_bg_tools (self->proll_ctrl,
					      BST_PIANO_ROLL_TOOL_SELECT,
					      BST_PIANO_ROLL_TOOL_NONE,
					      BST_PIANO_ROLL_TOOL_NONE);
      break;
    case BST_PIANO_ROLL_TOOL_VSELECT:
      bst_piano_roll_controller_set_obj_tools (self->proll_ctrl,
					       BST_PIANO_ROLL_TOOL_VSELECT,
					       BST_PIANO_ROLL_TOOL_NONE,
					       BST_PIANO_ROLL_TOOL_NONE);
      bst_piano_roll_controller_set_bg_tools (self->proll_ctrl,
					      BST_PIANO_ROLL_TOOL_VSELECT,
					      BST_PIANO_ROLL_TOOL_NONE,
					      BST_PIANO_ROLL_TOOL_NONE);
      break;
    default:	/* fallback */
      bst_radio_tools_set_tool (self->rtools, BST_PIANO_ROLL_TOOL_INSERT);
      break;
    }
}

static void
part_dialog_run_proc (GtkWidget *widget,
		      gulong     category_id,
		      gpointer   popup_data)
{
  BstPartDialog *self = BST_PART_DIALOG (widget);
  BseCategory *cat = bse_category_from_id (category_id);
  
  bst_procedure_exec_auto (cat->type,
			   "part", SFI_TYPE_PROXY, self->proll->proxy,
			   NULL);
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
    {
      GtkItemFactory *popup_factory = BST_PART_DIALOG_GET_CLASS (self)->popup_factory;

      bst_menu_popup (popup_factory,
		      GTK_WIDGET (self),
		      NULL, NULL,
		      event->button.x_root, event->button.y_root,
		      event->button.button, event->button.time);
    }
}

static void
part_dialog_note_choice (BstPartDialog *self,
			 guint          choice)
{
  self->proll_ctrl->note_length = choice;
}

static void
part_dialog_qnote_choice (BstPartDialog *self,
			  guint          choice)
{
  bst_piano_roll_set_quantization (self->proll, choice);
}

static void
menu_select_tool (GtkWidget *owner,
		  gulong     callback_action,
		  gpointer   popup_data)
{
  BstPartDialog *self = BST_PART_DIALOG (owner);
  bst_radio_tools_set_tool (self->rtools, callback_action);
}

static void
menu_activate_tool (GtkWidget *owner,
		    gulong     callback_action,
		    gpointer   popup_data)
{
  bst_activatable_activate (BST_ACTIVATABLE (owner), callback_action);
}

static void
bst_part_dialog_activate (BstActivatable         *activatable,
                          gulong                  action)
{
  BstPartDialog *self = BST_PART_DIALOG (activatable);

  gxk_status_window_push (self);

  switch (action)
    {
    case ACTION_CLEAR:
      bst_piano_roll_controller_clear (self->proll_ctrl);
      break;
    case ACTION_CUT:
      bst_piano_roll_controller_cut (self->proll_ctrl);
      break;
    case ACTION_COPY:
      bst_piano_roll_controller_copy (self->proll_ctrl);
      break;
    case ACTION_PASTE:
      bst_piano_roll_controller_paste (self->proll_ctrl);
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
    }

  gxk_status_window_pop ();

  bst_widget_update_activatable (activatable);
}

static gboolean
bst_part_dialog_can_activate (BstActivatable         *activatable,
                              gulong                  action)
{
  BstPartDialog *self = BST_PART_DIALOG (activatable);
  switch (action)
    {
    case ACTION_CLEAR:
    case ACTION_CUT:
    case ACTION_COPY:
      return TRUE;
    case ACTION_PASTE:
      return bst_piano_roll_controler_clipboard_full (self->proll_ctrl);
    case ACTION_UNDO:
      return self->project && bse_project_undo_depth (self->project) > 0;
    case ACTION_REDO:
      return self->project && bse_project_redo_depth (self->project) > 0;
    case ACTION_CLEAR_UNDO:
      return self->project && bse_project_undo_depth (self->project) + bse_project_redo_depth (self->project) > 0;
      /* tools */
    case BST_PIANO_ROLL_TOOL_INSERT:
    case BST_PIANO_ROLL_TOOL_RESIZE:
    case BST_PIANO_ROLL_TOOL_MOVE:
    case BST_PIANO_ROLL_TOOL_DELETE:
    case BST_PIANO_ROLL_TOOL_SELECT:
    case BST_PIANO_ROLL_TOOL_VSELECT:
      return TRUE;
    default:
      return FALSE;
    }
}

static void
bst_part_dialog_update_activatable (BstActivatable *activatable)
{
  BstPartDialog *self = BST_PART_DIALOG (activatable);
  GtkItemFactory *popup_factory = BST_PART_DIALOG_GET_CLASS (self)->popup_factory;
  GtkWidget *widget;
  gulong i;

  /* check if the app (its widget tree) was already destroyed */
  if (!GTK_BIN (self)->child)
    return;

  /* update app actions */
  for (i = 0; i < G_N_ELEMENTS (popup_entries); i++)
    {
      gulong action = popup_entries[i].callback_action;
      widget = gtk_item_factory_get_widget_by_action (popup_factory, action);
      if (widget && action)
        gtk_widget_set_sensitive (widget, bst_activatable_can_activate (activatable, action));
    }
}
