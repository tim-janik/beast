/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999, 2000, 2001 Tim Janik and Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include	"bstmenus.h"

#define	N_TRACKS 2 // FIXME: hack
#define	MENU_ITEM_PADDING (3)

/* --- structures --- */
struct _BstChoice
{
  BstChoiceFlags type_and_flags;
  BseIcon       *icon;
  const gchar   *name;
  gpointer       p_id;
};


/* --- prototypes --- */


/* --- variables --- */
static gboolean  main_quit_on_menu_item_activate = FALSE;
static GtkWidget *current_popup_menu = NULL;


/* --- functions --- */
static gint
menu_entries_compare (gconstpointer a,
		      gconstpointer b)
{
  const BstFactoryItem *entry_a = a;
  const BstFactoryItem *entry_b = b;

  return strcmp (entry_a->entry.path, entry_b->entry.path);
}

GSList*
bst_menu_entries_sort (GSList *entry_slist)
{
  GSList *slist, *branch_slist = NULL, *item_slist = NULL;

  for (slist = entry_slist; slist; slist = slist->next)
    {
      BstFactoryItem *bitem = slist->data;

      if (!bitem->entry.item_type || !bitem->entry.item_type[0] ||
	  bse_string_equals (bitem->entry.item_type, "<Item>"))
	item_slist = g_slist_prepend (item_slist, bitem);
      else
	branch_slist = g_slist_prepend (branch_slist, bitem);
    }
  g_slist_free (entry_slist);

  return g_slist_concat (g_slist_reverse (branch_slist),
			 g_slist_sort (item_slist, menu_entries_compare));
}

GSList*
bst_menu_entries_add_categories (GSList                *orig_entry_slist,
				 guint                  n_cats,
				 BseCategory           *cats,
				 GtkItemFactoryCallback cat_activate)
{
  BstFactoryItem *bitem, *last_bitem;
  GSList *entry_slist = NULL;

  if (n_cats)
    {
      g_return_val_if_fail (cats != NULL, NULL);
      g_return_val_if_fail (cat_activate != NULL, NULL);
    }

  bitem = g_new0 (BstFactoryItem, n_cats);
  last_bitem = bitem + n_cats;
  cats += n_cats;
  while (bitem < last_bitem)
    {
      entry_slist = g_slist_prepend (entry_slist, bitem);
      cats--;
      bitem->entry.path = cats->category + cats->mindex;
      bitem->entry.callback = cat_activate;
      bitem->entry.callback_action = cats->type;
      bitem->icon = cats->icon;
      bitem++;
    }

  return g_slist_concat (orig_entry_slist, entry_slist);
}

GSList*
bst_menu_entries_add_item_entries (GSList              *orig_entry_slist,
				   guint                n_menu_entries,
				   GtkItemFactoryEntry *menu_entries)
{
  BstFactoryItem *bitem, *last_bitem;
  GSList *entry_slist = NULL;

  if (n_menu_entries)
    g_return_val_if_fail (menu_entries != NULL, NULL);

  bitem = g_new0 (BstFactoryItem, n_menu_entries);
  last_bitem = bitem + n_menu_entries;
  while (bitem < last_bitem)
    {
      entry_slist = g_slist_prepend (entry_slist, bitem);
      bitem->entry = menu_entries[--n_menu_entries];
      bitem->icon = NULL;
      bitem++;
    }

  return g_slist_concat (orig_entry_slist, entry_slist);
}

GSList*
bst_menu_entries_add_bentries (GSList       *orig_entry_slist,
			       guint         n_menu_entries,
			       BstMenuEntry *menu_entries)
{
  BstFactoryItem *bitem, *last_bitem;
  GSList *entry_slist = NULL;

  if (n_menu_entries)
    g_return_val_if_fail (menu_entries != NULL, NULL);

  bitem = g_new0 (BstFactoryItem, n_menu_entries);
  last_bitem = bitem + n_menu_entries;
  while (bitem < last_bitem)
    {
      entry_slist = g_slist_prepend (entry_slist, bitem);
      --n_menu_entries;
      bitem->entry.path = menu_entries[n_menu_entries].path;
      bitem->entry.accelerator = menu_entries[n_menu_entries].accelerator;
      bitem->entry.callback = menu_entries[n_menu_entries].callback;
      bitem->entry.callback_action = menu_entries[n_menu_entries].callback_action;
      bitem->entry.item_type = menu_entries[n_menu_entries].item_type;
      bitem->icon = bst_icon_from_stock (menu_entries[n_menu_entries].stock_icon);
      bitem++;
    }

  return g_slist_concat (orig_entry_slist, entry_slist);
}

static GtkWidget*
create_icon_widget (BseIcon *icon)
{
  GtkWidget *widget;
  const guint size = 16;

  if (icon)
    {
      widget = gtk_widget_new (GNOME_TYPE_FOREST,
			       "visible", TRUE,
			       "width_request", size,
			       "height_request", size,
			       NULL);
      gnome_forest_put_sprite (GNOME_FOREST (widget), 1,
			       (icon->bytes_per_pixel > 3
				? art_pixbuf_new_const_rgba
				: art_pixbuf_new_const_rgb) (icon->pixels,
							     icon->width,
							     icon->height,
							     icon->width *
							     icon->bytes_per_pixel));
      gnome_forest_set_sprite_size (GNOME_FOREST (widget), 1, size, size);
    }
  else
    widget = gtk_widget_new (GTK_TYPE_ALIGNMENT,
			     "visible", TRUE,
			     "width_request", size,
			     NULL);
  
  return widget;
}

void
bst_menu_entries_create_list (GtkItemFactory *ifactory,
			      GSList         *bst_menu_entries,
			      gpointer        callback_data)
{
  GSList *slist;

  g_return_if_fail (GTK_IS_ITEM_FACTORY (ifactory));

  for (slist = bst_menu_entries; slist; slist = slist->next)
    {
      BstFactoryItem *bitem = slist->data;
      GtkWidget *item, *child = NULL;

      gtk_item_factory_create_items (ifactory, 1, &bitem->entry, callback_data);
      item = gtk_item_factory_get_item (ifactory, bitem->entry.path);
      if (GTK_IS_MENU_ITEM (item))
	child = GTK_BIN (item)->child;
      if (child)
	{
	  GtkWidget *hbox;

	  gtk_widget_ref (child);
	  gtk_container_remove (GTK_CONTAINER (item), child);
	  if (bitem->entry.item_type && strcmp (bitem->entry.item_type, "<Title>") == 0)
	    bst_widget_modify_as_title (child);
	  hbox = gtk_widget_new (GTK_TYPE_HBOX,
				 "visible", TRUE,
				 "spacing", MENU_ITEM_PADDING,
				 "parent", item,
				 "child", child,
				 NULL);
	  gtk_container_add_with_properties (GTK_CONTAINER (hbox),
					     create_icon_widget (bitem->icon),
					     "expand", FALSE,
					     "fill", FALSE,
					     "position", 0,
					     NULL);
	  gtk_widget_unref (child);
	}
    }
}

BstChoice*
bst_choice_alloc (BstChoiceFlags type,
		  BstIconId      icon_id,
		  const gchar   *choice_name,
		  gpointer       choice_id)
{
  BstChoice *choice = g_new (BstChoice, 1);

  choice->type_and_flags = type;
  choice->icon = bst_icon_from_stock (icon_id);
  choice->name = choice_name;
  choice->p_id = choice_id;

  return choice;
}

static void
menu_choice_activate (GtkWidget *item,
		      gpointer   data)
{
  g_assert (GTK_IS_MENU (current_popup_menu));

  gtk_object_set_data (GTK_OBJECT (current_popup_menu), "BstChoice",
		       gtk_object_get_user_data (GTK_OBJECT (item)));

  if (main_quit_on_menu_item_activate)
    gtk_main_quit ();
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
check_main_quit (GtkWidget *item)
{
  if (main_quit_on_menu_item_activate)
    gtk_main_quit ();
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

void
bst_choice_menu_add_choice_and_free (GtkWidget *menu,
				     BstChoice *choice)
{
  GtkWidget *item, *hbox;
  guint choice_type, choice_flags;

  g_return_if_fail (GTK_IS_MENU (menu));
  g_return_if_fail (choice != NULL);

  choice_type = choice->type_and_flags & BST_CHOICE_TYPE_MASK;
  choice_flags = choice->type_and_flags & BST_CHOICE_FLAG_MASK;

  item = gtk_widget_new (GTK_TYPE_MENU_ITEM,
			 "visible", TRUE,
			 "sensitive", !((choice_flags & BST_CHOICE_FLAG_INSENSITIVE) ||
					(choice_type != BST_CHOICE_TYPE_ITEM &&
					 choice_type != BST_CHOICE_TYPE_SUBMENU)),
			 "parent", menu,
			 "user_data", choice->p_id,
			 NULL);
  if (choice_type == BST_CHOICE_TYPE_SUBMENU)
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), GTK_WIDGET (choice->p_id));
  else
    menu_item_add_activator (item, menu_choice_activate);
  if (choice->name)
    {
      GtkWidget *any;
      
      hbox = gtk_widget_new (GTK_TYPE_HBOX,
			     "visible", TRUE,
			     "spacing", MENU_ITEM_PADDING,
			     "parent", item,
			     NULL);
      gtk_container_add_with_properties (GTK_CONTAINER (hbox),
					 create_icon_widget (choice->icon),
					 "expand", FALSE,
					 "fill", FALSE,
					 NULL);
      any = gtk_widget_new (GTK_TYPE_ACCEL_LABEL,
			    "visible", TRUE,
			    "label", choice->name,
			    "parent", hbox,
			    "accel_widget", item,
			    "xalign", 0.0,
			    NULL);
      if (choice_type == BST_CHOICE_TYPE_TITLE)
	bst_widget_modify_as_title (any);
    }
  if (choice->icon)
    bse_icon_unref (choice->icon);
  g_free (choice);
}

GtkWidget*
bst_choice_menu_createv (const gchar *menu_path,
			 BstChoice *first_choice,
			 ...)
{
  BstChoice *choice;
  GtkWidget *menu;
  va_list args;
  
  g_return_val_if_fail (first_choice != NULL, NULL);

  va_start (args, first_choice);

  menu = g_object_connect (gtk_widget_new (GTK_TYPE_MENU,
					   NULL),
			   "signal::selection-done", check_main_quit, NULL,
			   NULL);
  gtk_menu_set_accel_path (GTK_MENU (menu), menu_path);
  gtk_widget_ref (menu);
  gtk_object_sink (GTK_OBJECT (menu));

  choice = first_choice;
  do
    {
      bst_choice_menu_add_choice_and_free (menu, choice);
      choice = va_arg (args, BstChoice*);
    }
  while (choice);

  va_end (args);

  return menu;
}

GtkWidget*
bst_choice_dialog_createv (BstChoice *first_choice,
			   ...)
{
  BstChoice *choice;
  GtkWidget *dialog, *vbox;
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
	  /* any = bst_wrap_text_create (choice->name, TRUE, NULL); */
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
  dialog = bst_adialog_new (NULL, NULL,
			    vbox,
			    BST_ADIALOG_POPUP_POS | BST_ADIALOG_MODAL | BST_ADIALOG_FORCE_HBOX,
			    NULL);
  gtk_box_set_homogeneous (GTK_BOX (BST_ADIALOG (dialog)->hbox), TRUE);
  gtk_widget_ref (dialog);
  gtk_object_sink (GTK_OBJECT (dialog));
  
  /* add items
   */
  va_start (args, first_choice);
  choice = first_choice;
  do
    {
      BstADialog *adialog = BST_ADIALOG (dialog);
      guint choice_type = choice->type_and_flags & BST_CHOICE_TYPE_MASK;
      guint choice_flags = choice->type_and_flags & BST_CHOICE_FLAG_MASK;

      switch (choice_type)
	{
	  GtkWidget *any, *hbox;
	case BST_CHOICE_TYPE_TITLE:
	  gtk_widget_set (dialog,
			  "title", choice->name,
			  NULL);
	  break;
	case BST_CHOICE_TYPE_ITEM:
	  hbox = gtk_widget_new (GTK_TYPE_HBOX,
				 "visible", TRUE,
				 "spacing", MENU_ITEM_PADDING,
				 NULL);
	  if (choice->icon)
	    gtk_container_add_with_properties (GTK_CONTAINER (hbox),
					       create_icon_widget (choice->icon),
					       "expand", FALSE,
					       "fill", FALSE,
					       NULL);
	  any = gtk_widget_new (GTK_TYPE_LABEL,
				"visible", TRUE,
				"label", choice->name,
				"parent", hbox,
				"xalign", 0.5,
				NULL);
	  any = g_object_connect (gtk_widget_new (GTK_TYPE_BUTTON,
						  "visible", TRUE,
						  "can_default", TRUE,
						  "sensitive", !(choice_flags & BST_CHOICE_FLAG_INSENSITIVE),
						  "parent", adialog->hbox,
						  "child", hbox,
						  NULL),
				  "signal::clicked", button_choice_activate, choice->p_id,
				  NULL);
	  if (choice_flags & BST_CHOICE_FLAG_DEFAULT)
	    adialog->default_widget = any;
	  break;
	}
      
      if (choice->icon)
	bse_icon_unref (choice->icon);
      g_free (choice);
      
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
	  GtkBin *bin = list->data;
	  
	  if (GTK_WIDGET_IS_SENSITIVE (bin) && GTK_WIDGET_VISIBLE (bin) && bin->child)
	    {
	      selectable = TRUE;
	      break;
	    }
	}
      g_list_free (children);
    }
  else if (BST_IS_ADIALOG (widget))
    {
      GList *list, *children = gtk_container_children (GTK_CONTAINER (BST_ADIALOG (widget)->hbox));

      for (list = children; list; list = list->next)
	{
	  GtkBin *bin = list->data;
	  
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
	  main_quit_on_menu_item_activate = TRUE;
	  current_popup_menu = GTK_WIDGET (menu);
	  gtk_menu_popup (menu, NULL, NULL, NULL, NULL, mouse_button, time);
	  gtk_main ();
	  current_popup_menu = NULL;
	  main_quit_on_menu_item_activate = FALSE;
	}
      
      data = gtk_object_get_data (GTK_OBJECT (menu), "BstChoice");
    }
  else if (BST_IS_ADIALOG (choice))
    {
      BstADialog *adialog = BST_ADIALOG (choice);

      gtk_object_set_data (GTK_OBJECT (adialog), "BstChoice", data);

      if (bst_choice_selectable (choice))
	{
	  gtk_widget_show (choice);

	  do
	    {
	      GDK_THREADS_LEAVE ();
	      g_main_iteration (TRUE);
	      GDK_THREADS_ENTER ();
	    }
	  while (GTK_WIDGET_VISIBLE (choice));
	}

      data = gtk_object_get_data (GTK_OBJECT (adialog), "BstChoice");
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
 { "/Edit/Redo",		"<Control>R",		},
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
