/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2002 Tim Janik and Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bstmenus.h"

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


/* --- prototypes --- */


/* --- variables --- */
static gboolean   modal_loop_running = FALSE;
static gboolean   modal_loop_quit_on_menu_item_activate = FALSE;
static GtkWidget *current_popup_menu = NULL;
static gchar     *bstmenu_category_item = "<BstMenuCategoryItem>";


/* --- functions --- */
static gint
menu_entries_compare (gconstpointer a,
		      gconstpointer b)
{
  const BstMenuConfigEntry *entry_a = a;
  const BstMenuConfigEntry *entry_b = b;

  return strcmp (entry_a->path, entry_b->path);
}

void
bst_menu_config_sort (BstMenuConfig *config)
{
  SfiRing *entry, *branches = NULL, *items = NULL;

  for (entry = config->entries; entry; entry = sfi_ring_walk (entry, config->entries))
    {
      BstMenuConfigEntry *e = entry->data;

      if (!e->item_type || !e->item_type[0] || g_pattern_match_simple ("<*Item>", e->item_type))
	items = sfi_ring_append (items, e);
      else
	branches = sfi_ring_append (branches, e);
    }
  config->entries = sfi_ring_concat (branches, sfi_ring_sort (items, menu_entries_compare));
}

void
bst_menu_config_reverse (BstMenuConfig *config)
{
  config->entries = sfi_ring_reverse (config->entries);
}

static void
menu_config_append (BstMenuConfig      *config,
		    BstMenuConfigEntry *entry)
{
  BstMenuConfigEntry *e = g_new0 (BstMenuConfigEntry, 1);
  config->gcentries = g_slist_prepend (config->gcentries, e);
  e->path = g_strdup (entry->path);
  e->accelerator = g_strdup (entry->accelerator);
  e->callback = entry->callback;
  e->callback_action = entry->callback_action;
  e->item_type = g_strdup (entry->item_type);
  e->extra_data = entry->extra_data;
  config->entries = sfi_ring_append (config->entries, e);
}

static BseIcon*
menu_config_copy_icon (BstMenuConfig *config,
		       BseIcon       *icon)
{
  BseIcon *ic = bse_icon_copy_shallow (icon);
  config->gcicons = g_slist_prepend (config->gcicons, ic);
  return ic;
}

static void
menu_config_append_cat (BstMenuConfig  *mconfig,
                        BseCategory    *cat,
                        BstMenuCatFunc  callback,
                        guint           skip_levels,
                        const gchar    *new_prefix,
                        const gchar    *stock_fallback)
{
  BstMenuConfigEntry e = { 0, };
  const gchar *p = cat->category[0] == '/' ? cat->category + 1 : cat->category;
  gboolean have_icon;           // FIXME: optimize once we support NULL icons
  while (skip_levels--)
    {
      const gchar *d = strchr (p, '/');
      p = d ? d + 1 : p;
    }
  e.path = g_strconcat (new_prefix ? new_prefix : "/", p, NULL);
  e.accelerator = NULL;
  e.callback = (BstMenuUserFunc) callback;
  e.callback_action = cat->category_id;
  have_icon = cat->icon && (cat->icon->width + cat->icon->height) > 0;
  if (!have_icon)
    {
      e.item_type = stock_fallback ? "<StockItem>" : "<Item>";
      e.extra_data = stock_fallback;
    }
  else
    {
      e.item_type = bstmenu_category_item;
      e.extra_data = menu_config_copy_icon (mconfig, cat->icon);
    }
  menu_config_append (mconfig, &e);
  g_free (e.path);
}

BstMenuConfig*
bst_menu_config_append_cat (BstMenuConfig          *mconfig,
                            BseCategory            *cat,
                            BstMenuCatFunc          callback,
                            guint                   skip_levels,
                            const gchar            *new_prefix,
                            const gchar            *stock_fallback)
{
  g_return_val_if_fail (mconfig != NULL, mconfig);
  g_return_val_if_fail (cat != NULL, mconfig);

  menu_config_append_cat (mconfig, cat, callback, skip_levels, new_prefix, stock_fallback);
  return mconfig;
}

BstMenuConfig*
bst_menu_config_from_cats (BseCategorySeq *cseq,
			   BstMenuCatFunc  callback,
			   guint           skip_levels,
                           const gchar    *new_prefix,
                           const gchar    *stock_fallback)
{
  BstMenuConfig *mconfig = g_new0 (BstMenuConfig, 1);
  guint i;

  g_return_val_if_fail (cseq != NULL, mconfig);

  for (i = 0; i < cseq->n_cats; i++)
    menu_config_append_cat (mconfig, cseq->cats[i], callback, skip_levels, new_prefix, stock_fallback);
  return mconfig;
}

BstMenuConfig*
bst_menu_config_from_entries (guint               n_entries,
			      BstMenuConfigEntry *entries)
{
  BstMenuConfig *config = g_new0 (BstMenuConfig, 1);
  guint i;

  for (i = 0; i < n_entries; i++)
    menu_config_append (config, entries + i);
  return config;
}

BstMenuConfig*
bst_menu_config_merge (BstMenuConfig *config,
		       BstMenuConfig *merge_config)
{
  config->entries = sfi_ring_concat (config->entries, merge_config->entries);
  config->gcentries = g_slist_concat (config->gcentries, merge_config->gcentries);
  config->gcicons = g_slist_concat (config->gcicons, merge_config->gcicons);
  g_free (merge_config);
  return config;
}

void
bst_menu_config_free (BstMenuConfig *config)
{
  GSList *node;
  for (node = config->gcentries; node; node = node->next)
    {
      BstMenuConfigEntry *e = node->data;
      g_free (e->path);
      g_free (e->accelerator);
      g_free (e->item_type);
      g_free (e);
    }
  for (node = config->gcicons; node; node = node->next)
    bse_icon_free (node->data);
  sfi_ring_free (config->entries);
  g_slist_free (config->gcentries);
  g_slist_free (config->gcicons);
  g_free (config);
}

typedef struct {
  GtkWidget	  *owner;
  gpointer         popup_data;
  GtkDestroyNotify destroy;
} PopupData;

static void
bst_menu_item_wrapper (GtkWidget *menu_item,
		       gpointer   data)
{
  GtkItemFactory *ifactory = gtk_item_factory_from_widget (menu_item);
  GtkWidget *owner = g_object_get_data (G_OBJECT (menu_item), "bst-menu-owner");
  gulong callback_action = (gulong) g_object_get_data (G_OBJECT (menu_item), "bst-menu-callback-action");
  PopupData *pdata = gtk_item_factory_popup_data (ifactory);
  BstMenuCatFunc callback = data;

  if (!owner)
    owner = pdata ? pdata->owner : NULL;
  if (!owner)
    {
      GdkEvent *event = gtk_get_current_event ();

      /* possibly an accelerator invocation,
       * the alternative to gtk_get_current_event() would
       * be focus-event snooping...
       */
      if (event)
	{
	  GtkWidget *ewidget = gtk_get_event_widget (event);

	  ewidget = ewidget ? gtk_widget_get_toplevel (ewidget) : NULL;
	  if (GTK_IS_WINDOW (ewidget))
	    {
	      GSList *slist = g_object_get_data (G_OBJECT (ifactory), "bst-menu-accel-owners");

	      ewidget = GTK_WINDOW (ewidget)->focus_widget;
	      while (ewidget && !owner)
		{
		  if (g_slist_find (slist, ewidget))
		    owner = ewidget;
		  ewidget = ewidget->parent;
		}
	    }
	}
    }

  if (owner)
    callback (owner, callback_action, pdata ? pdata->popup_data : NULL);
  else
    gdk_beep ();
}

void
bst_menu_config_create_items (BstMenuConfig  *config,
			      GtkItemFactory *ifactory,
			      GtkWidget      *owner)
{
  SfiRing *ring;

  g_return_if_fail (GTK_IS_ITEM_FACTORY (ifactory));
  if (!GTK_IS_MENU (ifactory->widget))
    g_return_if_fail (GTK_IS_WIDGET (owner));

  for (ring = config->entries; ring; ring = sfi_ring_walk (ring, config->entries))
    {
      GtkItemFactoryEntry ife = { 0, };
      BstMenuConfigEntry *e = ring->data;
      GtkWidget *item;

      /* create menu item */
      ife.path = e->path;
      ife.accelerator = e->accelerator;
      ife.callback = NULL;
      ife.callback_action = e->callback_action; /* gtk_item_factory_get_widget_by_action() */
      if (e->item_type && strcmp (e->item_type, bstmenu_category_item) == 0)
	{
	  ife.item_type = "<ImageItem>";
	  /* we want an image menu item without an image, which gtk
	   * doesn't properly support, so we just provide an empty
	   * serialized pixbuf
	   */
	  ife.extra_data = BST_PIXDATA_EMPTY1x1;
	}
      else if (e->item_type && strcmp (e->item_type, "<StockItem>") == 0)
        {
          /* create just an image item, we want the stock IDs to affect only the image (not accels etc.) */
          ife.item_type = "<ImageItem>";
          ife.extra_data = BST_PIXDATA_EMPTY1x1;
        }
      else
	{
	  ife.item_type = e->item_type;
	  ife.extra_data = e->extra_data;
	}
      gtk_item_factory_create_items (ifactory, 1, &ife, NULL);
      item = gxk_item_factory_get_item (ifactory, ife.path);

      /* connect callback */
      if (e->callback)
	{
	  g_object_set_data (G_OBJECT (item), "bst-menu-owner", owner);
	  // g_object_set_data (G_OBJECT (item), "bst-menu-callback", e->callback);
	  g_object_set_data (G_OBJECT (item), "bst-menu-callback-action", (void*) e->callback_action);
	  gtk_signal_connect (GTK_OBJECT (item),
			      "activate",
			      GTK_SIGNAL_FUNC (bst_menu_item_wrapper),
			      e->callback);
	}

      /* create image */
      if (e->item_type && strcmp (e->item_type, bstmenu_category_item) == 0)
	{
	  GtkWidget *image = bst_image_from_icon ((BseIcon*) e->extra_data, BST_SIZE_MENU);
	  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
	}
      else if (e->item_type && strcmp (e->item_type, "<StockItem>") == 0)
        {
          GtkWidget *image = e->extra_data ? gtk_image_new_from_stock ((gchar*) e->extra_data, GTK_ICON_SIZE_MENU) : NULL;
          gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
        }

      /* fixup Title items */
      if (ife.item_type && strcmp (ife.item_type, "<Title>") == 0)
	gxk_widget_modify_as_title (GTK_BIN (item)->child);
    }
}

static void
popup_data_destroy (gpointer data)
{
  PopupData *pdata = data;

  if (pdata->destroy)
    pdata->destroy (pdata->popup_data);
  g_free (pdata);
}

void
bst_menu_popup (GtkItemFactory  *ifactory,
		GtkWidget	*owner,
		gpointer         popup_data,
		GtkDestroyNotify data_destroy,
		guint            x,
		guint            y,
		guint            mouse_button,
		guint32          time)
{
  PopupData *pdata;

  g_return_if_fail (GTK_IS_ITEM_FACTORY (ifactory));
  g_return_if_fail (GTK_IS_WIDGET (owner));

  pdata = g_new (PopupData, 1);
  pdata->owner = owner;
  pdata->popup_data = popup_data;
  pdata->destroy = data_destroy;

  gtk_item_factory_popup_with_data (ifactory,
				    pdata, popup_data_destroy,
				    x, y, mouse_button, time);
}

void
bst_menu_add_accel_owner (GtkItemFactory  *ifactory,
			  GtkWidget       *owner)
{
  GSList *slist;

  g_return_if_fail (GTK_IS_ITEM_FACTORY (ifactory));
  g_return_if_fail (GTK_IS_WIDGET (owner));

  slist = g_object_steal_data (G_OBJECT (ifactory), "bst-menu-accel-owners");
  slist = g_slist_prepend (slist, owner);
  g_object_set_data_full (G_OBJECT (ifactory), "bst-menu-accel-owners", slist, (GDestroyNotify) g_slist_free);
}

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
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), GTK_WIDGET (choice->p_id));
  else
    menu_item_add_activator (item, menu_choice_activate);
  if (choice->name)
    {
      GtkWidget *any;

      if (choice->icon_stock_id)
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item),
				       gxk_stock_image (choice->icon_stock_id, BST_SIZE_MENU));
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
	  gtk_widget_set_sensitive (list->data, sensitive);
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
  
  g_return_val_if_fail (first_choice != NULL, NULL);

  va_start (args, first_choice);

  menu = g_object_connect (gtk_widget_new (GTK_TYPE_MENU,
					   NULL),
			   "signal::selection-done", check_modal_quit, NULL,
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
  dialog = gxk_dialog_new (NULL, NULL, GXK_DIALOG_POPUP_POS | GXK_DIALOG_MODAL,
			   NULL, vbox);
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
					 button_choice_activate, choice->p_id,
					 choice->icon_stock_id,
					 (choice_flags & BST_CHOICE_FLAG_DEFAULT) ? GXK_DIALOG_MULTI_DEFAULT : 0);
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
	  GtkBin *bin = list->data;
	  
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
