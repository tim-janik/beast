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

#include <string.h>

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
static const gchar* controller_info_from_menu_id (guint id);


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
  item->bparam = NULL;
}

static void
bst_rack_item_destroy (GtkObject *object)
{
  BstRackItem *item = BST_RACK_ITEM (object);

  if (item->bparam)
    {
      bst_param_destroy (item->bparam);
      item->bparam = NULL;
    }

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
      gboolean can_clone = item->pocket && item->bparam && item->proxy && item->pspec;
      guint id;

      if (!item->choice)
	{
	  item->controller_choice = create_controller_menu ();
	  item->choice = bst_choice_menu_createv ("<BEAST-RackItem>/EditPopup",
						  BST_CHOICE_TITLE (_("Edit Rack Item")),
						  BST_CHOICE_SEPERATOR,
						  BST_CHOICE_SUBMENU (_("Controller"), item->controller_choice),
						  BST_CHOICE_S (1005, _("Duplicate"), NONE, FALSE),
						  BST_CHOICE_SEPERATOR,
						  BST_CHOICE_S (1001, _("Grow Horizontally"), NONE, TRUE),
						  BST_CHOICE_S (1002, _("Grow Vertically"), NONE, TRUE),
						  BST_CHOICE_S (1003, _("Shrink Horizontally"), NONE, TRUE),
						  BST_CHOICE_S (1004, _("Shrink Vertically"), NONE, TRUE),
						  BST_CHOICE_SEPERATOR,
						  BST_CHOICE (1006, _("Delete"), DELETE),
						  BST_CHOICE_END);
	}
      bst_choice_menu_set_item_sensitive (item->choice, 1005, can_clone);
      sensitize_controller_menu (item->controller_choice, item->pspec);
      id = bst_choice_modal (item->choice, event->button, event->time);
      switch (id)
	{
	  gint t;
	case 0:
	  /* menu aborted */
	  break;
	default: /* 1.. are from controller submenu */
	  bse_data_pocket_set_string (item->pocket, item->entry, "property-controller",
				      controller_info_from_menu_id (id));
	  break;
	case 1001:
	  t = bse_data_pocket_get_int (item->pocket, item->entry, "property-hspan");
	  t += 1;
	  t = MAX (t, 1);
	  bse_data_pocket_set_int (item->pocket, item->entry, "property-hspan", t);
	  break;
	case 1002:
	  t = bse_data_pocket_get_int (item->pocket, item->entry, "property-vspan");
	  t += 1;
	  t = MAX (t, 1);
	  bse_data_pocket_set_int (item->pocket, item->entry, "property-vspan", t);
	  break;
	case 1003:
	  t = bse_data_pocket_get_int (item->pocket, item->entry, "property-hspan");
	  t -= 1;
	  t = MAX (t, 1);
	  bse_data_pocket_set_int (item->pocket, item->entry, "property-hspan", t);
	  break;
	case 1004:
	  t = bse_data_pocket_get_int (item->pocket, item->entry, "property-vspan");
	  t -= 1;
	  t = MAX (t, 1);
	  bse_data_pocket_set_int (item->pocket, item->entry, "property-vspan", t);
	  break;
	case 1005:
	  id = bse_data_pocket_create_entry (item->pocket);
	  bse_data_pocket_set_string (item->pocket, id, "property-controller", bst_param_get_view_name (item->bparam));
	  bse_data_pocket_set_object (item->pocket, id, "property-object", item->proxy);
	  bse_data_pocket_set_string (item->pocket, id, "property-name", item->pspec->name);
	  break;
	case 1006:
	  bse_data_pocket_delete_entry (item->pocket, item->entry);
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
      GParamSpec *pspec = NULL;
      SfiProxy proxy = 0;
      const gchar *controller, *name;

      controller = bse_data_pocket_get_string (item->pocket, item->entry, "property-controller");
      name = bse_data_pocket_get_string (item->pocket, item->entry, "property-name");
      proxy = bse_data_pocket_get_object (item->pocket, item->entry, "property-object");
      if (proxy && name)
	pspec = bse_proxy_get_pspec (proxy, name);
      bst_rack_item_set_proxy (item, pspec ? proxy : 0, pspec, controller);

      if (item->bparam)
	{
	  BstRackChildInfo info;

	  info.col = bse_data_pocket_get_int (item->pocket, item->entry, "property-x");
	  info.row = bse_data_pocket_get_int (item->pocket, item->entry, "property-y");
	  info.hspan = bse_data_pocket_get_int (item->pocket, item->entry, "property-hspan");
	  info.vspan = bse_data_pocket_get_int (item->pocket, item->entry, "property-vspan");
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
			    SfiProxy     pocket,
			    guint        entry_id)
{
  g_return_if_fail (BST_IS_RACK_ITEM (item));
  if (pocket)
    {
      g_return_if_fail (BSE_IS_DATA_POCKET (pocket));
      g_return_if_fail (entry_id > 0);
    }

  if (item->pocket)
    {
      bse_proxy_disconnect (item->pocket,
			    "any_signal", rack_item_remove, item,
			    "any_signal", pocket_entry_changed, item,
			    NULL);
      bst_rack_item_set_proxy (item, 0, NULL, NULL);
    }
  item->pocket = pocket;
  item->entry = entry_id;
  if (item->pocket)
    {
      bse_proxy_connect (item->pocket,
			 "swapped_signal::release", rack_item_remove, item,
			 "swapped_signal::entry-changed", pocket_entry_changed, item,
			 NULL);
    }
  if (item->entry)
    pocket_entry_changed (item, item->entry);
}

void
bst_rack_item_set_proxy (BstRackItem	*item,
			 SfiProxy	 proxy,
			 GParamSpec	*pspec,
			 const gchar	*view_name)
{
  g_return_if_fail (BST_IS_RACK_ITEM (item));
  if (pspec)
    g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  view_name = pspec ? bst_param_lookup_view (pspec, TRUE, view_name, bst_param_binding_proxy ()) : NULL;

  /* have to optimize this for non-changes */
  if (item->proxy != proxy ||
      item->pspec != pspec ||
      (!view_name && item->bparam) ||
      (view_name && !item->bparam) ||
      (view_name && item->bparam &&
       strcmp (view_name, bst_param_get_view_name (item->bparam)) != 0))
    {
      if (item->bparam)
	{
	  bst_param_destroy (item->bparam);
	  item->bparam = NULL;
	}

      if (item->pspec)
	g_param_spec_unref (item->pspec);
      if (proxy && pspec && view_name)
	{
	  item->proxy = proxy;
	  item->pspec = pspec;
	}
      else
	{
	  item->proxy = 0;
	  item->pspec = NULL;
	}
      if (item->pspec)
	g_param_spec_ref (item->pspec);

      if (item->proxy)
	{
	  const gchar *name = bse_data_pocket_get_string (item->pocket, item->entry, "property-controller");
	  item->bparam = bst_param_proxy_create (item->pspec, TRUE, view_name, item->proxy);
	  // notify via: (GCallback) bst_rack_item_controler_changed, item
	  gtk_container_add (GTK_CONTAINER (item), bst_param_rack_widget (item->bparam));
	  if (!name || strcmp (name, view_name) != 0)
	    bse_data_pocket_set_string (item->pocket, item->entry, "property-controller", view_name);
	  bst_param_update (item->bparam);
          g_return_if_fail (item->bparam->binding != NULL);
	}
    }
}

void
bst_rack_item_gui_changed (BstRackItem *item)
{
  g_return_if_fail (BST_IS_RACK_ITEM (item));

  if (item->pocket && item->entry && item->bparam && !item->block_updates)
    {
      item->block_updates++;
      bse_data_pocket_set_int (item->pocket, item->entry, "property-x", item->rack_child_info.col);
      bse_data_pocket_set_int (item->pocket, item->entry, "property-y", item->rack_child_info.row);
      bse_data_pocket_set_int (item->pocket, item->entry, "property-hspan", item->rack_child_info.hspan);
      bse_data_pocket_set_int (item->pocket, item->entry, "property-vspan", item->rack_child_info.vspan);
      item->block_updates--;
    }
}

GtkWidget*
create_controller_menu (void)
{
  const gchar **names = bst_param_list_names (TRUE, NULL);
  GtkWidget *menu;
  guint i;

  menu = g_object_new (GTK_TYPE_MENU,
		       "visible", TRUE,
		       NULL);
  for (i = 0; names[i]; i++)
    bst_choice_menu_add_choice_and_free (menu, BST_CHOICE (1 + i, names[i], NONE));
  return menu;
}

static void
sensitize_controller_menu (GtkWidget  *menu,
			   GParamSpec *pspec)
{
  const gchar **names = bst_param_list_names (TRUE, NULL);
  guint i;

  for (i = 0; names[i]; i++)
    bst_choice_menu_set_item_sensitive (menu, 1 + i,
					bst_param_rate_check (pspec, TRUE, names[i],
							      bst_param_binding_proxy ()) > 0);
}

static const gchar*
controller_info_from_menu_id (guint id)
{
  guint n;
  const gchar **names = bst_param_list_names (TRUE, &n);

  return id > 0 && id <= n ? names[id - 1] : NULL;
}
