/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002 Tim Janik
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
#include "bstrackitem.h"

#include "bstmenus.h"


/* --- prototypes --- */
static void	  bst_rack_item_class_init	(BstRackItemClass	*klass);
static void	  bst_rack_item_init		(BstRackItem		*item);
static void	  bst_rack_item_destroy		(GtkObject		*object);
static void	  bst_rack_item_parent_set	(GtkWidget		*widget,
						 GtkWidget		*previous_parent);
static void	  bst_rack_item_button_press	(BstRackItem		*item,
						 GdkEventButton		*event);
static GtkWidget* create_controller_menu	(void);
static void	  sensitize_controller_menu	(GtkWidget		*menu,
						 GParamSpec		*pspec);
static BstControllerInfo* controller_info_from_menu_id (guint id);


/* --- static variables --- */
static gpointer		    parent_class = NULL;


/* --- functions --- */
GType
bst_rack_item_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info = {
	sizeof (BstRackItemClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) bst_rack_item_class_init,
	NULL,   /* class_finalize */
	NULL,   /* class_data */
	sizeof (BstRackItem),
	0,      /* n_preallocs */
	(GInstanceInitFunc) bst_rack_item_init,
      };

      type = g_type_register_static (GTK_TYPE_FRAME,
				     "BstRackItem",
				     &type_info, 0);
    }

  return type;
}

static void
bst_rack_item_class_init (BstRackItemClass *class)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);

  object_class->destroy = bst_rack_item_destroy;

  widget_class->parent_set = bst_rack_item_parent_set;

  class->button_press = bst_rack_item_button_press;

  g_signal_new ("button-press",
		G_OBJECT_CLASS_TYPE (class),
		G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET (BstRackItemClass, button_press),
		NULL, NULL,
		bst_marshal_NONE__BOXED,
		G_TYPE_NONE, 1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
}

static void
bst_rack_item_init (BstRackItem *item)
{
  item->rack_child_info.col = -1;
  item->rack_child_info.row = -1;
  item->rack_child_info.hspan = -1;
  item->rack_child_info.vspan = -1;
  item->empty_frame = FALSE;
  item->block_updates = 0;
  item->pocket = 0;
  item->entry = 0;
  item->cinfo = NULL;
  item->cwidget = NULL;
}

static void
bst_rack_item_destroy (GtkObject *object)
{
  BstRackItem *item = BST_RACK_ITEM (object);

  if (item->cwidget)
    gtk_widget_destroy (item->cwidget);
  item->cwidget = NULL;
  item->cinfo = NULL;

  bst_rack_item_set_property (item, 0, 0);

  if (item->choice)
    {
      bst_choice_destroy (item->choice);
      item->controller_choice = NULL;
      item->choice = NULL;
    }

  /* chain parent class' handler */
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
update_frame (GtkWidget *widget)
{
  /* BstRackItem *item = BST_RACK_ITEM (widget); */
  BstRackTable *rtable = BST_RACK_TABLE (widget->parent);

  g_object_set (widget,
		"shadow_type", rtable->edit_mode ? GTK_SHADOW_ETCHED_OUT : GTK_SHADOW_NONE,
		NULL);
}

static void
bst_rack_item_parent_set (GtkWidget *widget,
			  GtkWidget *previous_parent)
{
  if (BST_IS_RACK_TABLE (widget->parent))
    {
      g_object_connect (widget->parent, "swapped_signal::edit_mode_changed", update_frame, widget, NULL);
      update_frame (widget);
    }
  else if (BST_IS_RACK_TABLE (previous_parent))
    g_object_disconnect (previous_parent, "any_signal", update_frame, widget, NULL);

  /* chain parent class' handler */
  if (GTK_WIDGET_CLASS (parent_class)->parent_set)
    GTK_WIDGET_CLASS (parent_class)->parent_set (widget, previous_parent);
}

static void
bst_rack_item_button_press (BstRackItem    *item,
			    GdkEventButton *event)
{
  if (event->button == 3)
    {
      gboolean can_clone = item->pocket && item->cinfo && item->proxy && item->pspec;
      guint id;

      if (!item->choice)
	{
	  item->controller_choice = create_controller_menu ();
	  item->choice = bst_choice_menu_createv ("<BEAST-RackItem>/EditPopup",
						  BST_CHOICE_TITLE ("Edit Rack Item"),
						  BST_CHOICE_SEPERATOR,
						  BST_CHOICE_SUBMENU ("Controller", item->controller_choice),
						  BST_CHOICE_S (1005, "Duplicate", NONE, FALSE),
						  BST_CHOICE_SEPERATOR,
						  BST_CHOICE_S (1001, "Grow Horizontally", NONE, TRUE),
						  BST_CHOICE_S (1002, "Grow Vertically", NONE, TRUE),
						  BST_CHOICE_S (1003, "Shrink Horizontally", NONE, TRUE),
						  BST_CHOICE_S (1004, "Shrink Vertically", NONE, TRUE),
						  BST_CHOICE_SEPERATOR,
						  BST_CHOICE (1006, "Delete", DELETE),
						  BST_CHOICE_END);
	}
      bst_choice_menu_set_item_sensitive (item->choice, 1005, can_clone);
      sensitize_controller_menu (item->controller_choice, item->pspec);
      id = bst_choice_modal (item->choice, event->button, event->time);
      switch (id)
	{
	  BstControllerInfo *cinfo;
	  gint t;
	case 0:
	  /* menu aborted */
	  break;
	default: /* 1.. are from controller submenu */
	  cinfo = controller_info_from_menu_id (id);
	  bsw_data_pocket_set_string (item->pocket, item->entry, "property-controller", cinfo->name);
	  break;
	case 1001:
	  t = bsw_data_pocket_get_int (item->pocket, item->entry, "property-hspan");
	  t += 1;
	  t = MAX (t, 1);
	  bsw_data_pocket_set_int (item->pocket, item->entry, "property-hspan", t);
	  break;
	case 1002:
	  t = bsw_data_pocket_get_int (item->pocket, item->entry, "property-vspan");
	  t += 1;
	  t = MAX (t, 1);
	  bsw_data_pocket_set_int (item->pocket, item->entry, "property-vspan", t);
	  break;
	case 1003:
	  t = bsw_data_pocket_get_int (item->pocket, item->entry, "property-hspan");
	  t -= 1;
	  t = MAX (t, 1);
	  bsw_data_pocket_set_int (item->pocket, item->entry, "property-hspan", t);
	  break;
	case 1004:
	  t = bsw_data_pocket_get_int (item->pocket, item->entry, "property-vspan");
	  t -= 1;
	  t = MAX (t, 1);
	  bsw_data_pocket_set_int (item->pocket, item->entry, "property-vspan", t);
	  break;
	case 1005:
	  id = bsw_data_pocket_create_entry (item->pocket);
	  bsw_data_pocket_set_string (item->pocket, id, "property-controller", item->cinfo->name);
	  bsw_data_pocket_set_object (item->pocket, id, "property-object", item->proxy);
	  bsw_data_pocket_set_string (item->pocket, id, "property-name", item->pspec->name);
	  break;
	case 1006:
	  bsw_data_pocket_delete_entry (item->pocket, item->entry);
	  break;
	}
    }
}

static void
pocket_entry_changed (BstRackItem *item,
		      guint        entry)
{
  if (item->entry == entry && !item->block_updates)
    {
      BstControllerInfo *cinfo = NULL;
      GParamSpec *pspec = NULL;
      BswProxy proxy = 0;
      gchar *controller;

      controller = bsw_data_pocket_get_string (item->pocket, item->entry, "property-controller");
      cinfo = bst_controller_lookup (controller, NULL);
      if (cinfo)
	{
	  gchar *name = bsw_data_pocket_get_string (item->pocket, item->entry, "property-name");

	  proxy = bsw_data_pocket_get_object (item->pocket, item->entry, "property-object");
	  if (proxy && name)
	    pspec = bsw_proxy_get_pspec (proxy, name);
	}
      bst_rack_item_set_proxy (item, pspec ? proxy : 0, pspec, cinfo);

      if (item->cinfo)
	{
	  BstRackChildInfo info;

	  info.col = bsw_data_pocket_get_int (item->pocket, item->entry, "property-x");
	  info.row = bsw_data_pocket_get_int (item->pocket, item->entry, "property-y");
	  info.hspan = bsw_data_pocket_get_int (item->pocket, item->entry, "property-hspan");
	  info.vspan = bsw_data_pocket_get_int (item->pocket, item->entry, "property-vspan");
	  if ((info.col != item->rack_child_info.col ||
	       info.row != item->rack_child_info.row ||
	       info.hspan != item->rack_child_info.hspan ||
	       info.vspan != item->rack_child_info.vspan) &&
	      info.hspan > 0 && info.vspan > 0)
	    bst_rack_child_set_info (GTK_WIDGET (item), info.col, info.row, info.hspan, info.vspan);
	}
    }
}

static void
rack_item_remove (BstRackItem *item)
{
  bst_rack_item_set_property (item, 0, 0);
}

void
bst_rack_item_set_property (BstRackItem *item,
			    BswProxy     pocket,
			    guint        entry_id)
{
  g_return_if_fail (BST_IS_RACK_ITEM (item));
  if (pocket)
    {
      g_return_if_fail (BSW_IS_DATA_POCKET (pocket));
      g_return_if_fail (entry_id > 0);
    }

  if (item->pocket)
    {
      bsw_proxy_disconnect (item->pocket,
			    "any_signal", rack_item_remove, item,
			    "any_signal", pocket_entry_changed, item,
			    NULL);
      bst_rack_item_set_proxy (item, 0, NULL, NULL);
    }
  item->pocket = pocket;
  item->entry = entry_id;
  if (item->pocket)
    {
      bsw_proxy_connect (item->pocket,
			 "swapped_signal::set_parent", rack_item_remove, item,
			 "swapped_signal::entry-changed", pocket_entry_changed, item,
			 NULL);
    }
  if (item->entry)
    pocket_entry_changed (item, item->entry);
}

void
bst_rack_item_set_proxy (BstRackItem       *item,
			 BswProxy	    proxy,
			 GParamSpec        *pspec,
			 BstControllerInfo *cinfo)
{
  g_return_if_fail (BST_IS_RACK_ITEM (item));
  if (pspec)
    g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  /* have to optimize this for non-changes */
  if (item->proxy != proxy || item->pspec != pspec || item->cinfo != cinfo)
    {
      if (pspec && !bst_controller_check (cinfo, pspec))
	cinfo = NULL;

      if (item->cwidget)
	gtk_widget_destroy (item->cwidget);
      item->cinfo = cinfo;
      
      if (item->proxy)
	bsw_proxy_disconnect (item->proxy,
			      "any_signal", bst_rack_item_model_changed, item,
			      NULL);
      item->proxy = pspec && cinfo ? proxy : 0;
      item->pspec = item->proxy ? pspec : NULL;
      if (item->proxy)
	{
	  gchar *name = g_strdup_printf ("swapped_signal::notify::%s", item->pspec->name);

	  bsw_proxy_connect (item->proxy,
			     name, bst_rack_item_model_changed, item,
			     NULL);
	  g_free (name);
	  item->cwidget = bst_controller_create (cinfo, item->pspec,
						 (GCallback) bst_rack_item_controler_changed,
						 item);
	  g_object_connect (item->cwidget,
			    "swapped_signal::destroy", g_nullify_pointer, &item->cwidget,
			    NULL);
	  gtk_container_add (GTK_CONTAINER (item), item->cwidget);
	  gtk_widget_show (item->cwidget);
	}

      if (item->cinfo)
	{
	  gchar *name = bsw_data_pocket_get_string (item->pocket, item->entry, "property-controller");

	  if (!name || strcmp (name, cinfo->name) != 0)
	    bsw_data_pocket_set_string (item->pocket, item->entry, "property-controller", cinfo->name);
	}
      bst_rack_item_model_changed (item);
    }
}

void
bst_rack_item_gui_changed (BstRackItem *item)
{
  g_return_if_fail (BST_IS_RACK_ITEM (item));

  if (item->pocket && item->entry && item->cinfo && !item->block_updates)
    {
      item->block_updates++;
      bsw_data_pocket_set_int (item->pocket, item->entry, "property-x", item->rack_child_info.col);
      bsw_data_pocket_set_int (item->pocket, item->entry, "property-y", item->rack_child_info.row);
      bsw_data_pocket_set_int (item->pocket, item->entry, "property-hspan", item->rack_child_info.hspan);
      bsw_data_pocket_set_int (item->pocket, item->entry, "property-vspan", item->rack_child_info.vspan);
      item->block_updates--;
    }
}

void
bst_rack_item_controler_changed (BstRackItem *item)
{
  g_return_if_fail (BST_IS_RACK_ITEM (item));

  if (item->cwidget && item->proxy && item->pspec && !item->block_updates)
    {
      item->block_updates++;
      if (item->cinfo->fetch)
	{
	  GValue value = { 0, };
	  GType type = item->cinfo->value_type ? item->cinfo->value_type : G_PARAM_SPEC_VALUE_TYPE (item->pspec);
	  
	  g_value_init (&value, type);
	  bst_controller_fetch (item->cwidget, &value);
	  g_object_set_property (bse_object_from_id (item->proxy), item->pspec->name, &value);
	  g_value_unset (&value);
	}
      item->block_updates--;
    }
}

void
bst_rack_item_model_changed (BstRackItem *item)
{
  g_return_if_fail (BST_IS_RACK_ITEM (item));

  if (item->cwidget && item->proxy && item->pspec)
    {
      GValue value = { 0, };
      GType type = item->cinfo->value_type ? item->cinfo->value_type : G_PARAM_SPEC_VALUE_TYPE (item->pspec);

      g_value_init (&value, type);
      g_object_get_property (bse_object_from_id (item->proxy), item->pspec->name, &value);
      item->block_updates++;
      bst_controller_update (item->cwidget, &value);
      item->block_updates--;
      g_value_unset (&value);
    }
}

GtkWidget*
create_controller_menu (void)
{
  GtkWidget *menu;
  GSList *slist = bst_controller_list ();
  guint i;

  menu = g_object_new (GTK_TYPE_MENU,
		       "visible", TRUE,
		       NULL);
  for (i = 1; slist; slist = slist->next, i++)
    {
      BstControllerInfo *cinfo = slist->data;

      bst_choice_menu_add_choice_and_free (menu, BST_CHOICE (i, cinfo->name, NONE));
    }

  return menu;
}

static void
sensitize_controller_menu (GtkWidget  *menu,
			   GParamSpec *pspec)
{
  GSList *slist;
  guint id = 1;

  for (slist = bst_controller_list (); slist; slist = slist->next)
    bst_choice_menu_set_item_sensitive (menu, id++, bst_controller_check (slist->data, pspec));
}

static BstControllerInfo*
controller_info_from_menu_id (guint id)
{
  g_return_val_if_fail (id > 0, NULL);

  return g_slist_nth_data (bst_controller_list (), id - 1);
}
