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
#include	"bstpatterndialog.h"

#include	"bststatusbar.h"
#include	"bstprocedure.h"
#include	"bstmenus.h"
#include	"bsteffectview.h"
#include	<gdk/gdkkeysyms.h>



/* --- prototypes --- */
static void	bst_pattern_dialog_class_init	(BstPatternDialogClass	*klass);
static void	bst_pattern_dialog_init		(BstPatternDialog	*pattern_dialog);
static void	bst_pattern_dialog_destroy	(GtkObject		*object);
static void	pattern_dialog_focus_from_popup (BstPatternDialog	*pattern_dialog,
						 gpointer		 popup_data,
						 guint		        *channel,
						 guint		        *row);
static void	pattern_dialog_operate		(GtkWidget		*widget,
						 gulong		     	 op,
						 gpointer		 popup_data);
static void	pattern_dialog_exec_proc	(GtkWidget		*widget,
						 GType  		 proc_type,
						 gpointer		 popup_data);


/* --- menus --- */
static GtkItemFactoryEntry popup_entries[] =
{
#define BST_OP(op) (pattern_dialog_operate), (BST_PATTERN_OP_ ## op)
  { "/<<<<<<",			NULL,		NULL, 0,		"<Tearoff>",	0 },
  { "/Pattern",			NULL,		NULL, 0,		"<Title>",	BST_STOCK_PATTERN },
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
      
      pattern_dialog_type = gtk_type_unique (BST_TYPE_DIALOG, &pattern_dialog_info);
    }
  
  return pattern_dialog_type;
}

static void
bst_pattern_dialog_class_init (BstPatternDialogClass *class)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  // GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  GtkItemFactoryEntry *c1entries, *c2entries;
  GSList *slist;
  BseCategory *cats;
  guint n_c1entries, n_c2entries;

  parent_class = g_type_class_peek_parent (class);

  object_class->destroy = bst_pattern_dialog_destroy;

  /* create item factory for menu entries and categories */
  class->popup_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<BstPattern>", NULL);

  /* categories */
  cats = bse_categories_match_typed ("/Method/BsePattern/*", BSE_TYPE_PROCEDURE, &n_c1entries);
  c1entries = bst_menu_entries_from_cats (n_c1entries, cats, pattern_dialog_exec_proc, TRUE);
  g_free (cats);
  cats = bse_categories_match_typed ("/Proc/Toys/*", BSE_TYPE_PROCEDURE, &n_c2entries);
  c2entries = bst_menu_entries_from_cats (n_c2entries, cats, pattern_dialog_exec_proc, TRUE);
  g_free (cats);

  /* construct menu entry list */
  slist = g_slist_concat (bst_menu_entries_slist (n_c1entries, c1entries),
			  bst_menu_entries_slist (n_c2entries, c2entries));
  slist = bst_menu_entries_sort (slist);
  slist = g_slist_concat (bst_menu_entries_slist (n_popup_entries, popup_entries), slist);

  /* create entries and release allocations */
  bst_menu_entries_create (class->popup_factory, slist, NULL);
  g_slist_free (slist);
  g_free (c1entries);
  g_free (c2entries);
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

      //FIXME: gtk_item_factory_parse_rc_string (string);
      g_free (string);
    }
}

static void
bst_pattern_dialog_init (BstPatternDialog *pattern_dialog)
{
  BstPatternDialogClass *class = BST_PATTERN_DIALOG_GET_CLASS (pattern_dialog);
  GtkWidget *main_vbox;

  g_object_set (pattern_dialog,
		"default_width", 600,
		"default_height", 450,
		"flags", BST_DIALOG_STATUS,
		NULL);
  main_vbox = BST_DIALOG (pattern_dialog)->vbox;
  
  /* setup effect view */
  pattern_dialog->effect_view = g_object_connect (gtk_widget_new (BST_TYPE_EFFECT_VIEW,
								  "visible", TRUE,
								  NULL),
						  "signal::destroy", gtk_widget_destroyed, &pattern_dialog->effect_view,
						  NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), pattern_dialog->effect_view, FALSE, TRUE, 0);

  /* setup pattern editor parent */
  pattern_dialog->scrolled_window =
    g_object_connect (gtk_widget_new (GTK_TYPE_SCROLLED_WINDOW,
				      "visible", TRUE,
				      "hscrollbar_policy", GTK_POLICY_AUTOMATIC,
				      "vscrollbar_policy", GTK_POLICY_AUTOMATIC,
				      "parent", main_vbox,
				      "border_width", 5,
				      NULL),
		      "signal::destroy", gtk_widget_destroyed, &pattern_dialog->scrolled_window,
		      NULL);
  pattern_dialog->pattern_editor = NULL;

  /* setup the popup menu
   */
  gtk_window_add_accel_group (GTK_WINDOW (pattern_dialog),
			      class->popup_factory->accel_group);
  bst_menu_add_accel_owner (class->popup_factory, GTK_WIDGET (pattern_dialog));
}

static void
bst_pattern_dialog_destroy (GtkObject *object)
{
  // BstPatternDialog *pattern_dialog = BST_PATTERN_DIALOG (object);

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
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
		     GdkGC	      *light_gc,
		     GdkGC	      *bg_gc,
		     gpointer	       data)
{
  // BstPatternDialog *pattern_dialog = BST_PATTERN_DIALOG (data);
  GtkWidget *pe_widget = GTK_WIDGET (pe);
  BsePattern *pattern = pe->pattern;
  BsePatternNote *note = bse_pattern_peek_note (pattern, channel, row);
  guint n = note->n_effects;
  
  gdk_draw_string (window,
		   gtk_style_get_font (pe_widget->style),
		   n ? fg_gc : light_gc,
		   x,
		   y + pe->char_height - pe->char_descent,
		   !n ? "-" : n == 1 ? "+" : "*");
}

static void
pattern_dialog_focus_from_popup (BstPatternDialog *pattern_dialog,
				 gpointer	   popup_data,
				 guint            *channel,
				 guint            *row)
{
  guint index = GPOINTER_TO_UINT (popup_data);

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
      GtkItemFactory *popup_factory = BST_PATTERN_DIALOG_GET_CLASS (pattern_dialog)->popup_factory;
      guint index = (channel + 1) << 16 | (row + 1);

      // bst_pattern_editor_set_focus (pe, channel, row, FALSE);
      if (!bse_pattern_has_selection (pe->pattern))
	bse_pattern_select_note (pe->pattern, pe->focus_channel, pe->focus_row);

      bst_menu_popup (popup_factory,
		      GTK_WIDGET (pattern_dialog),
		      GUINT_TO_POINTER (index), NULL,
		      root_x, root_y,
		      button, time);
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
	  pattern_dialog_exec_proc (GTK_WIDGET (pattern_dialog), type, NULL);
	}
    }
  else if (event->keyval == GDK_Delete)
    {
      GType type = g_type_from_name ("BsePattern+delete-note");

      if (type)
	{
	  handled = TRUE;
	  pattern_dialog_exec_proc (GTK_WIDGET (pattern_dialog), type, NULL);
	}
    }

  return handled;
}

static void
pe_focus_changed (GtkObject        *pattern_editor,
		  guint             channel,
		  guint             row,
		  BstPatternDialog *pattern_dialog)
{
  BstPatternEditor *pe = BST_PATTERN_EDITOR (pattern_editor);

  if (GTK_WIDGET_DRAWABLE (pattern_dialog->effect_view))
    bst_effect_view_set_note (BST_EFFECT_VIEW (pattern_dialog->effect_view), pe->pattern, channel, row);
}

GtkWidget*
bst_pattern_dialog_new (BsePattern *pattern)
{
  GtkWidget *widget;
  BstPatternDialog *pattern_dialog;
  
  g_return_val_if_fail (BSE_IS_PATTERN (pattern), NULL);
  
  widget = gtk_widget_new (BST_TYPE_PATTERN_DIALOG, NULL);
  pattern_dialog = BST_PATTERN_DIALOG (widget);
  
  pattern_dialog->pattern_editor = bst_pattern_editor_new (NULL);
  gtk_widget_set (pattern_dialog->pattern_editor,
		  "visible", TRUE,
		  "parent", pattern_dialog->scrolled_window,
		  NULL);
  bst_pattern_editor_set_pattern (BST_PATTERN_EDITOR (pattern_dialog->pattern_editor), pattern);	/* updates title */
  g_object_connect (pattern_dialog->pattern_editor,
		    "signal::destroy", gtk_widget_destroyed, &pattern_dialog->pattern_editor,
		    "signal::pattern-step", bst_pattern_editor_dfl_stepper, NULL,
		    "signal::cell-activate", pe_cell_activate, pattern_dialog,
		    "signal::key-press-event", pe_key_press, pattern_dialog,
		    "signal::focus-changed", pe_focus_changed, pattern_dialog,
		    NULL);
  bst_pattern_editor_set_effect_hooks (BST_PATTERN_EDITOR (pattern_dialog->pattern_editor),
				       pe_effect_area_width,
				       pe_effect_area_draw,
				       pattern_dialog,
				       NULL);
  
  return widget;
}

static void
pattern_dialog_exec_proc (GtkWidget *widget,
			  GType      proc_type,
			  gpointer   popup_data)
{
  BstPatternDialog *pattern_dialog = BST_PATTERN_DIALOG (widget);
  BstPatternEditor *pattern_editor = BST_PATTERN_EDITOR (pattern_dialog->pattern_editor);
  BsePattern *pattern = BSE_PATTERN (pattern_editor->pattern);
  guint channel, row;

  bse_object_ref (BSE_OBJECT (pattern));
  pattern_dialog_focus_from_popup (pattern_dialog, popup_data, &channel, &row);
  bst_procedure_exec_modal (proc_type,
			    "pattern", BSE_TYPE_PATTERN, pattern,
			    "focus-channel", G_TYPE_UINT, channel,
			    "focus-row", G_TYPE_UINT, row,
			    NULL);
  bse_object_unref (BSE_OBJECT (pattern));
}

static void
pattern_dialog_operate (GtkWidget *widget,
			gulong     op,
			gpointer   popup_data)
{
  BstPatternDialog *pattern_dialog = BST_PATTERN_DIALOG (widget);
  BstPatternEditor *pattern_editor = BST_PATTERN_EDITOR (pattern_dialog->pattern_editor);
  BsePattern *pattern = BSE_PATTERN (pattern_editor->pattern);
  guint channel, row;

  pattern_dialog_focus_from_popup (pattern_dialog, popup_data, &channel, &row);
  
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
  
  bse_object_unref (BSE_OBJECT (pattern));
  gtk_widget_unref (widget);
}

void
bst_pattern_dialog_operate (BstPatternDialog *pattern_dialog,
			    BstPatternOps     op)
{
  g_return_if_fail (BST_IS_PATTERN_DIALOG (pattern_dialog));
  g_return_if_fail (bst_pattern_dialog_can_operate (pattern_dialog, op));

  pattern_dialog_operate (GTK_WIDGET (pattern_dialog), op, NULL);
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
