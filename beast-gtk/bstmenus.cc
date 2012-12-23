/* BEAST - Better Audio System
 * Copyright (C) 1998-2002 Tim Janik and Red Hat, Inc.
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
#include "bstmenus.hh"
#include <string.h>


/* --- structures --- */
struct _BstChoice
{
  BstChoiceFlags type_and_flags;
  const gchar   *icon_stock_id;
  BseIcon       *bse_icon;
  const gchar   *name;
  gpointer       p_id;
};


/* --- variables --- */
static gboolean   modal_loop_running = FALSE;
static gboolean   modal_loop_quit_on_menu_item_activate = FALSE;
static GtkWidget *current_popup_menu = NULL;


/* --- functions --- */
BstChoice*
bst_choice_alloc (BstChoiceFlags type,
		  const gchar   *choice_name,
		  gpointer       choice_id,
		  const gchar   *icon_stock_id,
		  BseIcon       *icon)
{
  BstChoice *choice = g_new (BstChoice, 1);
  
  choice->type_and_flags = type;
  choice->icon_stock_id = icon_stock_id;
  choice->bse_icon = icon ? bse_icon_copy_shallow (icon) : NULL;
  choice->name = choice_name;
  choice->p_id = choice_id;
  
  return choice;
}

static void
menu_choice_activate (GtkWidget *item,
		      gpointer   data)
{
  gpointer udata = gtk_object_get_user_data (GTK_OBJECT (item));
  
  if (GTK_IS_MENU (current_popup_menu))
    {
      gtk_object_set_data (GTK_OBJECT (current_popup_menu), "BstChoice", udata);
      
      if (modal_loop_quit_on_menu_item_activate)
	modal_loop_running = FALSE;
    }
  else	/* current_popup_menu is not set e.g. for option menus */
    {
      while (GTK_IS_MENU (item->parent))
	{
	  GtkWidget *tmp;
          
	  item = item->parent;
	  tmp = gtk_menu_get_attach_widget (GTK_MENU (item));
	  if (GTK_IS_MENU_ITEM (tmp))
	    item = tmp;
	}
      g_assert (GTK_IS_MENU (item));
      
      gtk_object_set_data (GTK_OBJECT (item), "BstChoice", udata);
    }
}

static void
button_choice_activate (GtkWidget *item,
			gpointer   data)
{
  GtkWidget *window = gtk_widget_get_ancestor (item, GTK_TYPE_WINDOW);
  
  gtk_object_set_data (GTK_OBJECT (window), "BstChoice", data);
  
  gtk_widget_hide (window);
}

static void
check_modal_quit (GtkWidget *item)
{
  if (modal_loop_quit_on_menu_item_activate)
    modal_loop_running = FALSE;
}

static void
menu_item_add_activator (GtkWidget *widget,
			 gpointer   function)
{
  GtkWidget *menu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (widget));
  
  if (GTK_IS_CONTAINER (menu))
    gtk_container_forall (GTK_CONTAINER (menu), menu_item_add_activator, NULL);
  else
    g_object_connect (widget,
		      "signal::activate", function, NULL,
		      NULL);
}

static void
free_choice (BstChoice *choice)
{
  if (choice->bse_icon)
    bse_icon_free (choice->bse_icon);
  g_free (choice);
}

void
bst_choice_menu_add_choice_and_free (GtkWidget *menu,
				     BstChoice *choice)
{
  guint choice_type, choice_flags;
  GtkWidget *item;
  
  g_return_if_fail (GTK_IS_MENU (menu));
  g_return_if_fail (choice != NULL);
  
  choice_type = choice->type_and_flags & BST_CHOICE_TYPE_MASK;
  choice_flags = choice->type_and_flags & BST_CHOICE_FLAG_MASK;
  
  item = gtk_widget_new (GTK_TYPE_IMAGE_MENU_ITEM,
			 "visible", TRUE,
			 "sensitive", !((choice_flags & BST_CHOICE_FLAG_INSENSITIVE) ||
					(choice_type != BST_CHOICE_TYPE_ITEM &&
					 choice_type != BST_CHOICE_TYPE_SUBMENU)),
			 "parent", menu,
			 "user_data", choice->p_id,
			 NULL);
  if (choice_type == BST_CHOICE_TYPE_SUBMENU)
    gxk_menu_attach_as_submenu (GTK_MENU (choice->p_id), GTK_MENU_ITEM (item));
  else
    menu_item_add_activator (item, (void*) menu_choice_activate);
  if (choice->name)
    {
      GtkWidget *any;
      
      if (choice->icon_stock_id)
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item),
				       gxk_stock_image (choice->icon_stock_id, GXK_ICON_SIZE_MENU));
      any = gtk_widget_new (GTK_TYPE_ACCEL_LABEL,
			    "visible", TRUE,
			    "label", choice->name,
			    "parent", item,
			    "accel_widget", item,
			    "xalign", 0.0,
			    NULL);
      if (choice_type == BST_CHOICE_TYPE_TITLE)
	gxk_widget_modify_as_title (any);
    }
  free_choice (choice);
}

void
bst_choice_menu_set_item_sensitive (GtkWidget *menu,
				    gulong     id,
				    gboolean   sensitive)
{
  GtkMenuShell *shell;
  GList *list;
  
  g_return_if_fail (GTK_IS_MENU (menu));
  
  shell = GTK_MENU_SHELL (menu);
  for (list = shell->children; list; list = list->next)
    {
      if (id == (gulong) gtk_object_get_user_data (GTK_OBJECT (list->data)))
	{
	  gtk_widget_set_sensitive ((GtkWidget*) list->data, sensitive);
	  return;
	}
    }
  g_warning ("unable to find item with id %lu in menu %p", id, menu);
}

GtkWidget*
bst_choice_menu_createv (const gchar *menu_path,
			 BstChoice *first_choice,
			 ...)
{
  BstChoice *choice;
  GtkWidget *menu;
  va_list args;
  va_start (args, first_choice);

  menu = gtk_widget_new (GTK_TYPE_MENU, NULL);
  g_object_connect (menu,
                    "signal::selection-done", check_modal_quit, NULL,
                    NULL);
  gtk_menu_set_accel_path (GTK_MENU (menu), menu_path);
  gtk_widget_ref (menu);
  gtk_object_sink (GTK_OBJECT (menu));

  choice = first_choice;
  while (choice)
    {
      bst_choice_menu_add_choice_and_free (menu, choice);
      choice = va_arg (args, BstChoice*);
    }
  
  va_end (args);
  
  return menu;
}

GtkWidget*
bst_choice_dialog_createv (BstChoice *first_choice,
			   ...)
{
  BstChoice *choice;
  GtkWidget *vbox, *dialog;
  va_list args;
  
  g_return_val_if_fail (first_choice != NULL, NULL);
  
  /* text portions
   */
  vbox = gtk_widget_new (GTK_TYPE_VBOX,
			 "visible", TRUE,
			 "border_width", 5,
			 NULL);
  va_start (args, first_choice);
  choice = first_choice;
  do
    {
      guint choice_type = choice->type_and_flags & BST_CHOICE_TYPE_MASK;
      guint choice_flags = choice->type_and_flags & BST_CHOICE_FLAG_MASK;
      
      switch (choice_type)
	{
	  GtkWidget *any;
	case BST_CHOICE_TYPE_TEXT:
	  /* any = bst_scroll_text_create (BST_TEXT_VIEW_CENTER, choice->name); */
	  any = gtk_widget_new (GTK_TYPE_LABEL,
				"visible", TRUE,
				"justify", GTK_JUSTIFY_CENTER,
				"label", choice->name,
				"wrap", FALSE,
				NULL);
	  gtk_box_pack_start (GTK_BOX (vbox), any, TRUE, TRUE, 0);
	  gtk_widget_set_sensitive (any, !(choice_flags & BST_CHOICE_FLAG_INSENSITIVE));
	  break;
	case BST_CHOICE_TYPE_SEPARATOR:
	  any = gtk_widget_new (GTK_TYPE_HSEPARATOR,
				"visible", TRUE,
				"sensitive", FALSE,
				NULL);
	  gtk_box_pack_start (GTK_BOX (vbox), any, TRUE, TRUE, 0);
	  gtk_widget_set_sensitive (any, !(choice_flags & BST_CHOICE_FLAG_INSENSITIVE));
	  break;
	}
      choice = va_arg (args, BstChoice*);
    }
  while (choice);
  va_end (args);

  /* create dialog
   */
  dialog = (GtkWidget*) gxk_dialog_new (NULL, NULL, GXK_DIALOG_POPUP_POS | GXK_DIALOG_MODAL, NULL, vbox);
  gtk_widget_ref (dialog);
  gtk_object_sink (GTK_OBJECT (dialog));

  /* add items
   */
  va_start (args, first_choice);
  choice = first_choice;
  do
    {
      guint choice_type = choice->type_and_flags & BST_CHOICE_TYPE_MASK;
      guint choice_flags = choice->type_and_flags & BST_CHOICE_FLAG_MASK;
      
      switch (choice_type)
	{
	  GtkWidget *any;
	case BST_CHOICE_TYPE_TITLE:
	  gtk_widget_set (dialog, "title", choice->name, NULL);
	  break;
	case BST_CHOICE_TYPE_ITEM:
	  any = gxk_dialog_action_multi (GXK_DIALOG (dialog), choice->name,
					 (void*) button_choice_activate, choice->p_id,
					 choice->icon_stock_id,
					 (choice_flags & BST_CHOICE_FLAG_DEFAULT) ? GXK_DIALOG_MULTI_DEFAULT : GxkDialogMultiFlags (0));
	  if (choice_flags & BST_CHOICE_FLAG_INSENSITIVE)
	    gtk_widget_set_sensitive (any, FALSE);
	  break;
	}
      
      free_choice (choice);
      
      choice = va_arg (args, BstChoice*);
    }
  while (choice);
  va_end (args);
  
  return dialog;
}

void
bst_choice_destroy (GtkWidget *choice)
{
  g_return_if_fail (GTK_IS_CONTAINER (choice));
  
  gtk_widget_destroy (choice);
  gtk_widget_unref (choice);
}

gboolean
bst_choice_selectable (GtkWidget *widget)
{
  gboolean selectable = FALSE;
  
  g_return_val_if_fail (GTK_IS_CONTAINER (widget), FALSE);
  
  if (GTK_IS_MENU (widget))
    {
      GList *list, *children = gtk_container_children (GTK_CONTAINER (widget));
      
      for (list = children; list; list = list->next)
	{
	  GtkBin *bin = (GtkBin*) list->data;
	  
	  if (GTK_WIDGET_IS_SENSITIVE (bin) && GTK_WIDGET_VISIBLE (bin) && bin->child)
	    {
	      selectable = TRUE;
	      break;
	    }
	}
      g_list_free (children);
    }
  else if (GXK_IS_DIALOG (widget))
    {
      GList *list, *children = gtk_container_children (GTK_CONTAINER (GXK_DIALOG (widget)->hbox));
      
      for (list = children; list; list = list->next)
	{
	  GtkBin *bin = (GtkBin*) list->data;
	  
	  if (GTK_IS_BUTTON (bin) && GTK_WIDGET_IS_SENSITIVE (bin) && GTK_WIDGET_VISIBLE (bin))
	    {
	      selectable = TRUE;
	      break;
	    }
	}
      g_list_free (children);
    }
  
  return selectable;
}

guint
bst_choice_modal (GtkWidget *choice,
		  guint      mouse_button,
		  guint32    time)
{
  gpointer data = GUINT_TO_POINTER (0);
  
  if (GTK_IS_MENU (choice))
    {
      GtkMenu *menu = GTK_MENU (choice);
      
      gtk_object_set_data (GTK_OBJECT (menu), "BstChoice", data);
      
      if (bst_choice_selectable (choice))
	{
	  modal_loop_quit_on_menu_item_activate = TRUE;
	  modal_loop_running = TRUE;
	  current_popup_menu = GTK_WIDGET (menu);
	  gtk_menu_popup (menu, NULL, NULL, NULL, NULL, mouse_button, time);
	  while (modal_loop_running)
	    {
	      GDK_THREADS_LEAVE ();
	      g_main_iteration (TRUE);
	      GDK_THREADS_ENTER ();
	    }
	  current_popup_menu = NULL;
	  modal_loop_quit_on_menu_item_activate = FALSE;
	}
      
      data = gtk_object_get_data (GTK_OBJECT (menu), "BstChoice");
    }
  else if (GXK_IS_DIALOG (choice))
    {
      gtk_object_set_data (GTK_OBJECT (choice), "BstChoice", data);
      
      if (bst_choice_selectable (choice))
	{
	  gtk_widget_show (choice);
          
          while (GTK_WIDGET_VISIBLE (choice))
	    {
	      GDK_THREADS_LEAVE ();
	      g_main_iteration (TRUE);
	      GDK_THREADS_ENTER ();
	    }
	}
      
      data = gtk_object_get_data (GTK_OBJECT (choice), "BstChoice");
    }
  
  return GPOINTER_TO_UINT (data);
}

guint
bst_choice_get_last (GtkWidget *widget)
{
  gpointer data = gtk_object_get_data (GTK_OBJECT (widget), "BstChoice");
  
  return GPOINTER_TO_UINT (data);
}

/* Accelerator recommendations:
 *
 { "/File/New",			"<Control>N",		},
 { "/File/Open...",		"<Control>O",		},
 { "/File/Quit",		"<Control>Q",		},
 { "/File/Close",		"<Control>W",		},
 { "/File/Save",		"<Control>S",		},
 { "/Select/All",		"<Control>A",		},
 { "/Select/None",		"<Shift><Control>A",	},
 { "/Select/Invert",		"<Control>I",		},
 { "/Edit/Undo",		"<Control>Z",		},
 { "/Edit/Redo",		"<Control>Y",		},
 { "/Edit/Copy",		"<Control>C",		},
 { "/Edit/Duplicate",		"<Control>C",		},
 { "/Edit/Cut",			"<Control>X",		},
 { "/Edit/Cut Named...",	"<Shift><Control>X",	},
 { "/Edit/Paste",		"<Control>V",		},
 { "/Edit/Paste Named...",	"<Shift><Control>V",	},
 { "/Edit/Clear",		"<Control>K",		},
 { "/Edit/Fill",		"<Control>period",	},
 { "/Help/Help...",		"F1",			},
 { "/Help/Context Help...",	"<Shift>F1",		},
 { "Item/New...",		"<Control>N",		},
 { "Item/Delete",		"<Control>X",		},
 { "Item/Previous",		"Prior",		},
 { "Item/Next",			"Next",			},
 { "Item/Lower",		"<Control>B",		},
 { "Item/Lower",		"<Shift>Next",		},
 { "Item/Raise",		"<Control>F",		},
 { "Item/Raise",		"<Shift>Prior",		},
 { "Item/To Top",		"<Shift><Control>F",	},
 { "Item/To Top",		"<Control>Prior",	},
 { "Item/To Bottom",		"<Shift><Control>B",	},
 { "Item/To Bottom",		"<Control>Next",	},
*/
