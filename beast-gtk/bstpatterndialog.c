/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999, 2000 Olaf Hoehmann and Tim Janik
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
#include "bstpatterndialog.h"

#include "bststatusbar.h"
#include "bstprocedure.h"
#include "bstmenus.h"



/* --- prototypes --- */
static void	bst_pattern_dialog_class_init	(BstPatternDialogClass	*klass);
static void	bst_pattern_dialog_init		(BstPatternDialog	*pattern_dialog);
static void	bst_pattern_dialog_destroy	(GtkObject		*object);
static void	pattern_dialog_operate		(BstPatternDialog	*pattern_dialog,
						 BstPatternOps     	 op,
						 GtkWidget        	*menu_item);
static void	pattern_dialog_exec_proc	(BstPatternDialog	*pattern_dialog,
						 GType  		 proc_type,
						 GtkWidget        	*menu_item);


/* --- menus --- */
static gchar		  *bst_pattern_dialog_factories_path = "<BstPattern>";
static BstMenuEntry popup_entries[] =
{
#define BST_OP(op) (pattern_dialog_operate), (BST_PATTERN_OP_ ## op)
  { "/<<<<<<",			NULL,		NULL, 0,		"<Tearoff>",	0 },
  { "/Pattern",			NULL,		NULL, 0,		"<Title>",	BST_ICON_PATTERN },
  { "/-----",			NULL,		NULL, 0,		"<Separator>",	0 },
  { "/_Edit/<<<<<<",		NULL,		NULL, 0,		"<Tearoff>",	0 },
  { "/_Select/<<<<<<",		NULL,		NULL, 0,		"<Tearoff>",	0 },
  { "/_Tools/<<<<<<",		NULL,		NULL, 0,		"<Tearoff>",	0 },
  { "/-----",			NULL,		NULL, 0,		"<Separator>",	0 },
  { "/To_ys",			NULL,		NULL, 0,		"<LastBranch>",	0 },
  { "/Toys/<<<<<<",		NULL,		NULL, 0,		"<Tearoff>",	0 },
#undef	BST_OP
};
static guint n_popup_entries = sizeof (popup_entries) / sizeof (popup_entries[0]);


/* --- static variables --- */
static gpointer		      parent_class = NULL;
static BstPatternDialogClass *bst_pattern_dialog_class = NULL;


/* --- functions --- */
GtkType
bst_pattern_dialog_get_type (void)
{
  static GtkType pattern_dialog_type = 0;
  
  if (!pattern_dialog_type)
    {
      GtkTypeInfo pattern_dialog_info =
      {
	"BstPatternDialog",
	sizeof (BstPatternDialog),
	sizeof (BstPatternDialogClass),
	(GtkClassInitFunc) bst_pattern_dialog_class_init,
	(GtkObjectInitFunc) bst_pattern_dialog_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      pattern_dialog_type = gtk_type_unique (GTK_TYPE_WINDOW, &pattern_dialog_info);
    }
  
  return pattern_dialog_type;
}

static void
bst_pattern_dialog_class_init (BstPatternDialogClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  BseCategory *cats;
  guint n_cats;
  
  object_class = GTK_OBJECT_CLASS (class);
  widget_class = GTK_WIDGET_CLASS (class);
  parent_class = gtk_type_class (GTK_TYPE_WINDOW);
  bst_pattern_dialog_class = class;

  object_class->destroy = bst_pattern_dialog_destroy;

  class->popup_entries = NULL;
  class->popup_entries = bst_menu_entries_add_bentries (class->popup_entries,
							n_popup_entries,
							popup_entries);
  cats = bse_categories_match_typed ("/Method/BsePattern/*", BSE_TYPE_PROCEDURE, &n_cats);
  class->popup_entries = bst_menu_entries_add_categories (class->popup_entries,
							  n_cats,
							  cats,
							  pattern_dialog_exec_proc);
  g_free (cats);
  cats = bse_categories_match_typed ("/Proc/Toys/*", BSE_TYPE_PROCEDURE, &n_cats);
  class->popup_entries = bst_menu_entries_add_categories (class->popup_entries,
							  n_cats,
							  cats,
							  pattern_dialog_exec_proc);
  g_free (cats);
  class->popup_entries = bst_menu_entries_sort (class->popup_entries);
}

void
bst_pattern_dialog_gtkfix_default_accels (void)
{
  static gchar *accel_defaults[][2] = {
    { "<BstPattern>/Select/All",               "<Control>A", },
    { "<BstPattern>/Select/None",              "<Shift><Control>A", },
    { "<BstPattern>/Select/Invert",            "<Control>I", },
    { "<BstPattern>/Edit/Undo",                "<Control>Z", },
    { "<BstPattern>/Edit/Redo",                "<Control>R", },
    { "<BstPattern>/Edit/Copy",                "<Control>C", },
    { "<BstPattern>/Edit/Cut",                 "<Control>X", },
    { "<BstPattern>/Edit/Cut Named...",        "<Shift><Control>X", },
    { "<BstPattern>/Edit/Paste",               "<Control>V", },
    { "<BstPattern>/Edit/Paste Named...",      "<Shift><Control>V", },
    { "<BstPattern>/Edit/Clear",               "<Control>K", },
    { "<BstPattern>/Edit/Fill",                "<Control>period", },
    { "<BstPattern>/Edit/Clear",               "<Control>K", },
  };
  static guint n_accel_defaults = sizeof (accel_defaults) / sizeof (accel_defaults[0]);
  guint i;

  for (i = 0; i < n_accel_defaults; i++)
    {
      gchar *string = g_strconcat ("(menu-path \"",
				   accel_defaults[i][0],
				   "\" \"",
				   accel_defaults[i][1],
				   "\")",
				   NULL);

      gtk_item_factory_parse_rc_string (string);
      g_free (string);
    }
}

static void
bst_pattern_dialog_init (BstPatternDialog *pattern_dialog)
{
  BstPatternDialogClass *class = BST_PATTERN_DIALOG_GET_CLASS (pattern_dialog);
  GtkItemFactory *factory;

  bst_status_bar_ensure (GTK_WINDOW (pattern_dialog));
  pattern_dialog->main_vbox = GTK_BIN (pattern_dialog)->child;
  gtk_widget_set (pattern_dialog->main_vbox,
		  "signal::destroy", gtk_widget_destroyed, &pattern_dialog->main_vbox,
		  NULL);
  pattern_dialog->scrolled_window =
    gtk_widget_new (GTK_TYPE_SCROLLED_WINDOW,
		    "visible", TRUE,
		    "signal::destroy", gtk_widget_destroyed, &pattern_dialog->scrolled_window,
		    "hscrollbar_policy", GTK_POLICY_AUTOMATIC,
		    "vscrollbar_policy", GTK_POLICY_AUTOMATIC,
		    "parent", pattern_dialog->main_vbox,
		    "border_width", 5,
		    NULL);
  pattern_dialog->pattern_editor = NULL;

  /* setup the popup menu
   */
  factory = gtk_item_factory_new (GTK_TYPE_MENU, bst_pattern_dialog_factories_path, NULL);
  gtk_window_add_accel_group (GTK_WINDOW (pattern_dialog), factory->accel_group);
  bst_menu_entries_create (factory, class->popup_entries, pattern_dialog);
  pattern_dialog->popup = factory->widget;
  gtk_object_set_data_full (GTK_OBJECT (pattern_dialog),
			    bst_pattern_dialog_factories_path,
			    factory,
			    (GtkDestroyNotify) gtk_object_unref);
  pattern_dialog->proc_dialog = NULL;
}

static void
bst_pattern_dialog_destroy (GtkObject *object)
{
  BstPatternDialog *pattern_dialog = BST_PATTERN_DIALOG (object);

  if (pattern_dialog->proc_dialog)
    gtk_widget_destroy (pattern_dialog->proc_dialog);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static guint
pe_effect_area_width (BstPatternEditor *pe,
		      gpointer		data)
{
  BstPatternDialog *pattern_dialog;
  
  pattern_dialog = BST_PATTERN_DIALOG (data);
  
  return pe->char_width;
}

static void
pe_effect_area_draw (BstPatternEditor *pe,
		     guint	       channel,
		     guint	       row,
		     GdkWindow	      *window,
		     guint	       x,
		     guint	       y,
		     guint	       width,
		     guint	       height,
		     GdkGC	      *fg_gc,
		     GdkGC	      *bg_gc,
		     gpointer	       data)
{
  // BstPatternDialog *pattern_dialog = BST_PATTERN_DIALOG (data);
  GtkWidget *pe_widget = GTK_WIDGET (pe);
  BsePattern *pattern = pe->pattern;
  BsePatternNote *note = bse_pattern_peek_note (pattern, channel, row);
  guint n = note->n_effects;
  
  gdk_draw_string (window,
		   pe_widget->style->font,
		   fg_gc,
		   x,
		   y + pe->char_height - pe->char_descent,
		   !n ? "+" : n == 1 ? "*" : "#");
}

static inline void
pe_channel_row_from_popup (BstPatternDialog *pattern_dialog,
			   GtkWidget        *menu_item,
			   guint            *channel,
			   guint            *row)
{
  gpointer data = menu_item ? gtk_item_factory_popup_data_from_widget (menu_item) : NULL;
  guint index = GPOINTER_TO_UINT (data);

  if (index)
    {
      *channel = (index >> 16) - 1;
      *row = (index & 0xffff) - 1;
    }
  else
    {
      *channel = BST_PATTERN_EDITOR (pattern_dialog->pattern_editor)->focus_channel;
      *row = BST_PATTERN_EDITOR (pattern_dialog->pattern_editor)->focus_row;
    }
}

static void
pe_cell_activate (BstPatternEditor *pe,
		  guint		    channel,
		  guint		    row,
		  BstCellType	    cell_type,
		  guint		    root_x,
		  guint		    root_y,
		  guint		    button,
		  guint		    time,
		  BstPatternDialog *pattern_dialog)
{
  if (button == 3 || button == 0)
    {
      GtkItemFactory *popup_factory = gtk_object_get_data (GTK_OBJECT (pattern_dialog),
								bst_pattern_dialog_factories_path);
      guint index = (channel + 1) << 16 | (row + 1);

      // bst_pattern_editor_set_focus (pe, channel, row, FALSE);
      if (!bse_pattern_has_selection (pe->pattern))
	bse_pattern_select_note (pe->pattern, pe->focus_channel, pe->focus_row);
      
      gtk_item_factory_popup_with_data (popup_factory,
					GUINT_TO_POINTER (index),
					NULL,
					root_x,
					root_y,
					button,
					time);
    }
}

static gint
pe_key_press (GtkObject        *pattern_editor,
	      GdkEventKey      *event,
	      BstPatternDialog *pattern_dialog)
{
  gboolean handled = FALSE;

  if (event->keyval == GDK_Insert)
    {
      GType type = g_type_from_name ("BsePattern+insert-note");

      if (type)
	{
	  handled = TRUE;
	  pattern_dialog_exec_proc (pattern_dialog, type, NULL);
	}
    }
  else if (event->keyval == GDK_Delete)
    {
      GType type = g_type_from_name ("BsePattern+delete-note");

      if (type)
	{
	  handled = TRUE;
	  pattern_dialog_exec_proc (pattern_dialog, type, NULL);
	}
    }

  if (handled)
    gtk_signal_emit_stop_by_name (pattern_editor, "key-press-event");

  return handled;
}

GtkWidget*
bst_pattern_dialog_new (BsePattern *pattern)
{
  GtkWidget *widget;
  BstPatternDialog *pattern_dialog;
  
  g_return_val_if_fail (BSE_IS_PATTERN (pattern), NULL);
  
  widget = gtk_widget_new (BST_TYPE_PATTERN_DIALOG, NULL);
  pattern_dialog = BST_PATTERN_DIALOG (widget);
  
  pattern_dialog->pattern_editor = bst_pattern_editor_new (pattern);
  gtk_widget_set (pattern_dialog->pattern_editor,
		  "visible", TRUE,
		  "signal::destroy", gtk_widget_destroyed, &pattern_dialog->pattern_editor,
		  "signal::pattern-step", bst_pattern_editor_dfl_stepper, NULL,
		  "signal::cell-activate", pe_cell_activate, pattern_dialog,
		  "signal::key-press-event", pe_key_press, pattern_dialog,
		  "parent", pattern_dialog->scrolled_window,
		  NULL);
  bst_pattern_editor_set_effect_hooks (BST_PATTERN_EDITOR (pattern_dialog->pattern_editor),
				       pe_effect_area_width,
				       pe_effect_area_draw,
				       pattern_dialog,
				       NULL);
  
  return widget;
}

static void
pattern_dialog_exec_proc (BstPatternDialog *pattern_dialog,
			  GType             proc_type,
			  GtkWidget        *menu_item)
{
  BseParamSpec *pspec_pattern, *pspec_focus_channel, *pspec_focus_row; /* FIXME: cache these */
  BseParam param_pattern = { NULL, }, param_focus_channel = { NULL, }, param_focus_row = { NULL, };
  BstPatternEditor *pattern_editor;
  BseProcedureClass *procedure;
  BstProcedureShell *proc_shell;
  BsePattern *pattern;
  GtkWidget *widget;
  GSList *slist = NULL;
  guint channel, row;

  widget = GTK_WIDGET (pattern_dialog);
  pattern_editor = BST_PATTERN_EDITOR (pattern_dialog->pattern_editor);
  pattern = BSE_PATTERN (pattern_editor->pattern);
  pe_channel_row_from_popup (pattern_dialog, menu_item, &channel, &row);

  gtk_widget_ref (widget);
  bse_object_ref (BSE_OBJECT (pattern));

  /* ensure procedure shell
   */
  if (!pattern_dialog->proc_dialog)
    {
      proc_shell = BST_PROCEDURE_SHELL (bst_procedure_shell_new (NULL));
      pattern_dialog->proc_dialog = bst_procedure_dialog_from_shell (proc_shell, &pattern_dialog->proc_dialog);
    }
  else
    proc_shell = bst_procedure_dialog_get_shell (pattern_dialog->proc_dialog);

  /* setup procedure
   */
  procedure = g_type_class_ref (proc_type);
  bst_procedure_shell_set_proc (proc_shell, procedure);
  g_type_class_unref (procedure);

  /* ok, now we build a list of possible preset parameters to
   * pass into the procedure
   */
  pspec_pattern = bse_param_spec_item ("pattern", NULL, NULL,
				       BSE_TYPE_PATTERN, BSE_PARAM_DEFAULT);
  pspec_focus_channel = bse_param_spec_uint ("focus_channel", NULL, NULL,
					     0, BSE_MAX_N_CHANNELS - 1, 1, 0, BSE_PARAM_DEFAULT);
  pspec_focus_row = bse_param_spec_uint ("focus_row", NULL, NULL,
					 0, BSE_MAX_N_ROWS - 1, 1, 0, BSE_PARAM_DEFAULT);
  bse_param_init (&param_pattern, pspec_pattern);
  bse_param_init (&param_focus_channel, pspec_focus_channel);
  bse_param_init (&param_focus_row, pspec_focus_row);
  bse_param_set_item (&param_pattern, BSE_ITEM (pattern));
  bse_param_set_uint (&param_focus_channel, pattern_editor->focus_channel);
  bse_param_set_uint (&param_focus_row, pattern_editor->focus_row);
  slist = g_slist_prepend (slist, &param_pattern);
  slist = g_slist_prepend (slist, &param_focus_channel);
  slist = g_slist_prepend (slist, &param_focus_row);

  /* apply preset params and clean up
   */
  bst_procedure_shell_preset (proc_shell, TRUE, slist);
  while (slist)
    {
      BseParam *param = slist->data;
      GSList *tmp = slist->next;

      bse_param_free_value (param);
      bse_param_spec_free (param->pspec);
      g_slist_free_1 (slist);
      slist = tmp;
    }
  
  /* invoke procedure with selection already residing in the pattern
   */
  bst_status_window_push (widget);
  bst_procedure_dialog_exec_modal (pattern_dialog->proc_dialog, FALSE);
  bst_status_window_pop ();

  bst_pattern_dialog_update (pattern_dialog);

  bse_object_unref (BSE_OBJECT (pattern));
  gtk_widget_unref (widget);
}

static void
pattern_dialog_operate (BstPatternDialog *pattern_dialog,
			BstPatternOps     op,
			GtkWidget        *menu_item)
{
  BsePattern *pattern;
  GtkWidget *widget;
  guint channel, row;
  
  widget = GTK_WIDGET (pattern_dialog);
  pattern = BSE_PATTERN (BST_PATTERN_EDITOR (pattern_dialog->pattern_editor)->pattern);
  pe_channel_row_from_popup (pattern_dialog, menu_item, &channel, &row);
  
  gtk_widget_ref (widget);
  bse_object_ref (BSE_OBJECT (pattern));
  
  switch (op)
    {
    case BST_PATTERN_OP_HUHU:
      g_message ("HUHU c%u r%u", channel, row);
      break;
    default:
      break;
    }
  
  bst_pattern_dialog_update (pattern_dialog);
  
  bse_object_unref (BSE_OBJECT (pattern));
  gtk_widget_unref (widget);
}

void
bst_pattern_dialog_operate (BstPatternDialog *pattern_dialog,
			    BstPatternOps     op)
{
  g_return_if_fail (BST_IS_PATTERN_DIALOG (pattern_dialog));
  g_return_if_fail (bst_pattern_dialog_can_operate (pattern_dialog, op));

  pattern_dialog_operate (pattern_dialog, op, NULL);
}

gboolean
bst_pattern_dialog_can_operate (BstPatternDialog *pattern_dialog,
				BstPatternOps	  op)
{
  BsePattern *pattern;
  GtkWidget *widget;
  
  g_return_val_if_fail (BST_IS_PATTERN_DIALOG (pattern_dialog), FALSE);
  
  widget = GTK_WIDGET (pattern_dialog);
  pattern = BSE_PATTERN (BST_PATTERN_EDITOR (pattern_dialog->pattern_editor)->pattern);
  
  switch (op)
    {
    case BST_PATTERN_OP_HUHU:
      return TRUE;
    default:
      return FALSE;
    }
}

void
bst_pattern_dialog_update (BstPatternDialog *pattern_dialog)
{
  g_return_if_fail (BST_IS_PATTERN_DIALOG (pattern_dialog));
  
}
