/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002 Olaf Hoehmann and Tim Janik
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

#include "bststatusbar.h"
#include "bstmenus.h"


/* --- prototypes --- */
static void	bst_rack_item_class_init	(BstRackItemClass	*klass);
static void	bst_rack_item_init		(BstRackItem		*item);
static void	bst_rack_item_destroy		(GtkObject		*object);
static void	bst_rack_item_button_press	(BstRackItem		*item,
						 GdkEventButton		*event);
GtkWidget*	create_controller_menu		(void);
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
  GtkObjectClass *object_class;

  object_class = GTK_OBJECT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);

  object_class->destroy = bst_rack_item_destroy;

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
      item->choice = NULL;
    }

  /* chain parent class' handler */
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bst_rack_item_button_press (BstRackItem    *item,
			    GdkEventButton *event)
{
  if (event->button == 3)
    {
      guint id;

      if (!item->choice)
	{
	  item->choice = bst_choice_menu_createv ("<BEAST-RackItem>/EditPopup",
						  BST_CHOICE_TITLE ("Edit Rack Item"),
						  BST_CHOICE_SEPERATOR,
						  BST_CHOICE_S (1001, "Grow Horizontally", NONE, TRUE),
						  BST_CHOICE_S (1002, "Grow Vertically", NONE, TRUE),
						  BST_CHOICE_S (1003, "Shrink Horizontally", NONE, TRUE),
						  BST_CHOICE_S (1004, "Shrink Vertically", NONE, TRUE),
						  BST_CHOICE_SUBMENU ("Controller", create_controller_menu ()),
						  BST_CHOICE_END);
	}
      id = bst_choice_modal (item->choice, event->button, event->time);
      switch (id)
	{
	  BstControllerInfo *cinfo;
	  gint t;
	case 0:
	  /* menu aborted */
	  break;
	default: /* 1.. are from controller submenu */
	  g_print ("id: %u\n", id);
	  cinfo = controller_info_from_menu_id (id);
	  bsw_data_pocket_set_string (item->pocket, item->entry, "property-controller", cinfo->name);
	  g_printerr("controller: %s\n", cinfo->name);
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
	}
    }
}

static void
rack_item_remove (BstRackItem *item)
{
  bst_rack_item_set_property (item, 0, 0);
}

static void
pocket_entry_changed (BstRackItem *item,
		      guint        entry)
{
  if (item->entry == entry && !item->block_updates)
    {
      BstControllerInfo *cinfo = NULL;
      BstRackChildInfo info;
      GParamSpec *pspec = NULL;
      BswProxy proxy;
      gchar *name;

      proxy = bsw_data_pocket_get_object (item->pocket, item->entry, "property-object");
      name = bsw_data_pocket_get_string (item->pocket, item->entry, "property-name");
      if (proxy && name)
	pspec = bsw_proxy_get_pspec (proxy, name);
      if (pspec)
	{
	  name = bsw_data_pocket_get_string (item->pocket, item->entry, "property-controller");
	  cinfo = bst_controller_lookup (name, pspec);
	}
      bst_rack_item_set_proxy (item, proxy, pspec, cinfo);

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
    }
  item->pocket = pocket;
  item->entry = entry_id;
  if (item->pocket)
    {
      bsw_proxy_connect (item->pocket,
			 "swapped_signal::set_parent", rack_item_remove, item,
			 "swapped_signal::entry-changed", pocket_entry_changed, item,
			 NULL);
      if (item->entry)
	pocket_entry_changed (item, item->entry);
      else
	bst_rack_item_set_proxy (item, 0, NULL, NULL);
    }
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

  if (item->proxy != proxy || item->pspec != pspec || item->cinfo != cinfo)
    {
      if (item->cwidget)
	gtk_widget_destroy (item->cwidget);
      item->cinfo = cinfo;
      
      if (item->proxy)
	bsw_proxy_disconnect (item->pocket,
			      "any_signal", bst_rack_item_model_changed, item,
			      NULL);
      item->proxy = pspec && cinfo ? proxy : 0;
      item->pspec = item->proxy && cinfo ? pspec : 0;
      if (item->proxy)
	{
	  gchar *name = g_strdup_printf ("swapped_signal::notify::%s", item->pspec->name);

	  bsw_proxy_connect (item->proxy,
			     name, bst_rack_item_model_changed, item,
			     NULL);
	  g_free (name);
	  item->cwidget = item->cinfo->create (item->pspec,
					       (GCallback) bst_rack_item_controler_changed,
					       item,
					       item->cinfo->name);
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

  if (item->pocket && item->entry && item->proxy && item->pspec && !item->block_updates)
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

  if (item->pocket && item->entry && item->proxy && item->pspec && !item->block_updates)
    {
      item->block_updates++;
      if (item->cinfo->fetch)
	{
	  GValue value = { 0, };

	  g_value_init (&value, item->cinfo->value_type);
	  item->cinfo->fetch (item->cwidget, item->pspec, &value);
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

  if (item->cinfo && item->cwidget && item->proxy && item->pspec)
    {
      GValue value = { 0, };

      g_value_init (&value, item->cinfo->value_type);
      g_object_get_property (bse_object_from_id (item->proxy), item->pspec->name, &value);
      item->block_updates++;
      item->cinfo->update (item->cwidget, item->pspec, &value);
      item->block_updates--;
      g_value_unset (&value);
    }
}


/* --- controllers --- */
#include "bstrackitem-controllers.c"

static guint n_controllers = G_N_ELEMENTS (controllers);

BstControllerInfo*
bst_controller_lookup (const gchar *name,
		       GParamSpec  *pspec)
{
  guint i;

  if (pspec)
    g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), NULL);
  if (!pspec)
    return NULL;

  if (name)
    for (i = 0; i < n_controllers; i++)
      if (strcmp (name, controllers[i]->name) == 0 &&
	  g_value_type_transformable (G_PARAM_SPEC_VALUE_TYPE (pspec), controllers[i]->value_type) &&
	  (!controllers[i]->fetch || g_value_type_transformable (controllers[i]->value_type, G_PARAM_SPEC_VALUE_TYPE (pspec))))
	return controllers[i];

  /* fallback */
  for (i = 0; i < n_controllers; i++)
    if (g_value_type_transformable (G_PARAM_SPEC_VALUE_TYPE (pspec), controllers[i]->value_type) &&
	(!controllers[i]->fetch || g_value_type_transformable (controllers[i]->value_type, G_PARAM_SPEC_VALUE_TYPE (pspec))))
      return controllers[i];

  return NULL;
}

GtkWidget*
create_controller_menu (void)
{
  GtkWidget *menu;
  guint i;

  menu = g_object_new (GTK_TYPE_MENU,
		       "visible", TRUE,
		       NULL);
  for (i = 0; i < n_controllers; i++)
    bst_choice_menu_add_choice_and_free (menu, BST_CHOICE (i + 1, controllers[i]->name, NONE));
  return menu;
}

static BstControllerInfo*
controller_info_from_menu_id (guint id)
{
  g_return_val_if_fail (id > 0 && id < n_controllers, NULL);

  return controllers[id - 1];
}
