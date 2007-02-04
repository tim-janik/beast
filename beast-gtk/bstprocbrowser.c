/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "bstprocbrowser.h"

#include "bstprocedure.h"

#if 0

enum {
  COL_SNAME,	/* scheme name */
  COL_TNAME,	/* type name */
  COL_CAT,	/* category */
  N_COLS
};

/* --- prototypes --- */
static void	bst_proc_browser_class_init	(BstProcBrowserClass	*class);
static void	bst_proc_browser_init		(BstProcBrowser		*self);
static void	bst_proc_browser_destroy	(GtkObject		*object);
static void     proc_list_fill_value		(BstProcBrowser		*self,
						 guint			 column,
						 guint			 row,
						 GValue			*value);
static void	tree_row_activated		(BstProcBrowser		*self,
						 GtkTreePath		*path,
						 GtkTreeViewColumn	*column,
						 GtkTreeView		*tree_view);


/* --- static variables --- */
static gpointer             parent_class = NULL;


/* --- functions --- */
GType
bst_proc_browser_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info = {
	sizeof (BstProcBrowserClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) bst_proc_browser_class_init,
	NULL,   /* class_finalize */
	NULL,   /* class_data */
	sizeof (BstProcBrowser),
	0,      /* n_preallocs */
	(GInstanceInitFunc) bst_proc_browser_init,
      };

      type = g_type_register_static (GTK_TYPE_VBOX,
				     "BstProcBrowser",
				     &type_info, 0);
    }

  return type;
}

static void
bst_proc_browser_class_init (BstProcBrowserClass *class)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  object_class->destroy = bst_proc_browser_destroy;
}

static void
bst_proc_browser_init (BstProcBrowser *self)
{
  GtkWidget *scwin, *tree;
  GtkTreeSelection *tsel;

  /* main HBox
   */
  self->hbox = g_object_new (GTK_TYPE_HBOX,
			     "visible", TRUE,
			     "border_width", 3,
			     "parent", self,
			     NULL);

  /* fetch categories
   */
  self->cats = bse_categories_match_typed ("*", BSE_TYPE_PROCEDURE, &self->n_cats);

  /* setup procedure list model
   */
  self->proc_list = gtk_list_wrapper_new (N_COLS,
					  G_TYPE_STRING,  /* COL_SNAME */
					  G_TYPE_STRING,  /* COL_TNAME */
					  G_TYPE_STRING   /* COL_CAT */
					  );
  g_signal_connect_object (self->proc_list, "fill-value",
			   G_CALLBACK (proc_list_fill_value),
			   self, G_CONNECT_SWAPPED);
  gtk_list_wrapper_notify_prepend (self->proc_list, self->n_cats);


  /* setup tree view and it's scrolled window
   */
  scwin = g_object_new (GTK_TYPE_SCROLLED_WINDOW,
			"visible", TRUE,
			"hscrollbar_policy", GTK_POLICY_AUTOMATIC,
			"vscrollbar_policy", GTK_POLICY_ALWAYS,
			"height_request", 320,
			"width_request", 320,
			"border_width", 5,
			"shadow_type", GTK_SHADOW_IN,
			"parent", self->hbox,
			NULL);
  tree = g_object_new (GTK_TYPE_TREE_VIEW,
		       "visible", TRUE,
		       "can_focus", TRUE,
		       "model", self->proc_list,
		       "border_width", 10,
		       "parent", scwin,
		       NULL);
  gxk_tree_view_append_text_columns (GTK_TREE_VIEW (tree), N_COLS,
				     COL_SNAME, "", 0.0, "Scheme Name",
				     COL_TNAME, "", 0.0, "Type Name",
				     COL_CAT,   "", 0.0, "Category"
				     );
  g_object_connect (tree,
		    "swapped_object_signal::row_activated", tree_row_activated, self,
		    NULL);

  /* ensure selection
   */
  tsel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  gtk_tree_selection_set_mode (tsel, GTK_SELECTION_SINGLE);
  gxk_tree_selection_select_spath (tsel, "0");

  /* text entry
   */
  self->entry = g_object_new (GTK_TYPE_ENTRY,
			      "visible", TRUE,
			      "activates_default", TRUE,
			      NULL);
  gtk_box_pack_start (GTK_BOX (self), GTK_WIDGET (self->entry), FALSE, TRUE, 0);
}

static void
bst_proc_browser_destroy (GtkObject *object)
{
  BstProcBrowser *self = BST_PROC_BROWSER (object);

  g_free (self->cats);
  self->n_cats = 0;

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
proc_list_fill_value (BstProcBrowser *self,
		      guint           column,
		      guint           row,
		      GValue         *value)
{
  BseCategory *cat;

  g_return_if_fail (row < self->n_cats);

  cat = self->cats + row;

  switch (column)
    {
    case COL_TNAME:
      g_value_set_string (value, g_type_name (cat->type));
      break;
    case COL_SNAME:
      g_value_set_string_take_ownership (value, bsw_type_name_to_sname (g_type_name (cat->type)));
      break;
    case COL_CAT:
      g_value_set_string (value, cat->category);
      break;
    }
}

static void
tree_row_activated (BstProcBrowser    *self,
		    GtkTreePath       *path,
		    GtkTreeViewColumn *column,
		    GtkTreeView       *tree_view)
{
  // GtkTreeSelection *tsel = gtk_tree_view_get_selection (tree_view);
  BseCategory *cat;

  cat = self->cats + gtk_tree_path_get_indices (path)[0];
  bst_procedure_exec (cat->type, NULL, NULL);
}

GtkWidget*
bst_proc_browser_new (void)
{
  GtkWidget *sbrowser = g_object_new (BST_TYPE_PROC_BROWSER, NULL);

  return sbrowser;
}

static void
bst_proc_browser_execute (BstProcBrowser *self)
{
  gchar *text;
  guint argc;
  char **argv;

  g_return_if_fail (BST_IS_PROC_BROWSER (self));

  text = gtk_entry_get_text (self->entry);
  if (!g_shell_parse_argv (text, &argc, &argv, NULL))
    g_printerr ("failed to parse: %s\n", text);
  else
    {
      GType ptype = g_type_from_name (argv[0]);

      if (BSE_TYPE_IS_PROCEDURE (ptype))
	{
	  g_printerr ("proc-call: %s %s\n", argv[0], g_type_name (ptype));
	  bst_procedure_exec (ptype, NULL, NULL);
	}
      else
	g_printerr ("no-such-procedure: %s \n", argv[0]);
      
      // g_printerr ("return: %d\n", bsw_server_exec_proc (BSE_SERVER, argv[0], argv[1]));
      g_strfreev (argv);
    }
}

void
bst_proc_browser_create_buttons (BstProcBrowser *self,
				 GxkDialog      *dialog)
{
  GtkWidget *widget;

  g_return_if_fail (BST_IS_PROC_BROWSER (self));
  g_return_if_fail (GXK_IS_DIALOG (dialog));
  g_return_if_fail (self->execute == NULL);

  /* Execute
   */
  if (0)
    {
      self->execute = g_object_connect (gxk_dialog_default_action (dialog, BST_STOCK_EXECUTE, NULL, NULL),
					"swapped_signal::clicked", bst_proc_browser_execute, self,
					"swapped_signal::destroy", g_nullify_pointer, &self->execute,
					NULL);
      gxk_widget_set_tooltip (self->execute, "Execute the current line.");
    }

  /* Close
   */
  widget = gxk_dialog_action (dialog, BST_STOCK_CLOSE, gxk_toplevel_delete, NULL);
}
#endif
