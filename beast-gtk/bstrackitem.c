/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002-2003 Tim Janik
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
#include "bstrackitem.h"
#include "bstmenus.h"
#include <string.h>


/* --- prototypes --- */
static void       rack_item_class_init          (BstRackItemClass       *klass);
static void       rack_item_init                (BstRackItem            *item);
static void       rack_item_destroy             (GtkObject              *object);
static void       rack_item_button_press        (GxkRackItem            *xitem,
                                                 GdkEventButton         *event);
static GtkWidget* create_controller_menu        (void);
static void       sensitize_controller_menu     (GtkWidget              *menu,
                                                 GParamSpec             *pspec);
static const gchar* controller_info_from_menu_id (guint id);


/* --- static variables --- */
static gpointer             parent_class = NULL;


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
        (GClassInitFunc) rack_item_class_init,
        NULL,   /* class_finalize */
        NULL,   /* class_data */
        sizeof (BstRackItem),
        0,      /* n_preallocs */
        (GInstanceInitFunc) rack_item_init,
      };
      type = g_type_register_static (GXK_TYPE_RACK_ITEM, "BstRackItem", &type_info, 0);
    }
  return type;
}

static void
rack_item_class_init (BstRackItemClass *class)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GxkRackItemClass *xitem_class = GXK_RACK_ITEM_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  object_class->destroy = rack_item_destroy;
  
  xitem_class->button_press = rack_item_button_press;
}

static void
rack_item_init (BstRackItem *self)
{
  self->proxy = 0;
  self->path = NULL;
  self->block_updates = 0;
  self->param = NULL;
}

static void
rack_item_destroy (GtkObject *object)
{
  BstRackItem *self = BST_RACK_ITEM (object);
  
  if (self->param)
    {
      gxk_param_destroy (self->param);
      self->param = NULL;
    }
  
  bst_rack_item_set_parasite (self, 0, NULL);
  
  if (self->choice)
    {
      bst_choice_destroy (self->choice);
      self->controller_choice = NULL;
      self->choice = NULL;
    }
  
  /* chain parent class' handler */
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget*
bst_rack_item_new (SfiProxy        proxy,
                   const gchar    *path)
{
  BstRackItem *self;
  SfiRec *rec = bse_item_get_parasite (proxy, path);
  g_print ("bst_rack_item_new: %p\n", rec);
  if (!rec)
    return NULL;
  self = g_object_new (BST_TYPE_RACK_ITEM, NULL);
  bst_rack_item_set_parasite (self, proxy, path);
  return GTK_WIDGET (self);
}

static void
rack_item_parasite_changed (BstRackItem *self)
{
  if (!self->block_updates)
    {
#if 0
      GParamSpec *pspec = NULL;
      SfiProxy proxy = 0;
      const gchar *controller, *name;
      
      controller = bse_data_pocket_get_string (self->pocket, self->entry, "property-controller");
      name = bse_data_pocket_get_string (self->pocket, self->entry, "property-name");
      proxy = bse_data_pocket_get_object (self->pocket, self->entry, "property-object");
      if (proxy && name)
        pspec = bse_proxy_get_pspec (proxy, name);
      bst_rack_item_set_proxy (self, pspec ? proxy : 0, pspec, controller);
      
      if (self->bparam)
        {
          BstRackChildInfo info;
          
          info.col = bse_data_pocket_get_int (self->pocket, self->entry, "property-x");
          info.row = bse_data_pocket_get_int (self->pocket, self->entry, "property-y");
          info.hspan = bse_data_pocket_get_int (self->pocket, self->entry, "property-hspan");
          info.vspan = bse_data_pocket_get_int (self->pocket, self->entry, "property-vspan");
          if ((info.col != self->rack_child_info.col ||
               info.row != self->rack_child_info.row ||
               info.hspan != self->rack_child_info.hspan ||
               info.vspan != self->rack_child_info.vspan) &&
              info.hspan > 0 && info.vspan > 0)
            bst_rack_child_set_info (GTK_WIDGET (self), info.col, info.row, info.hspan, info.vspan);
        }
#endif
    }
}

static void
rack_item_remove_proxy (BstRackItem *self)
{
  bst_rack_item_set_parasite (self, 0, NULL);
}

void
bst_rack_item_set_parasite (BstRackItem    *self,
                            SfiProxy        proxy,
                            const gchar    *path)
{
  g_return_if_fail (BST_IS_RACK_ITEM (self));
  
  if (self->proxy)
    {
      bse_proxy_disconnect (self->proxy,
                            "any_signal", rack_item_remove_proxy, self,
                            "any_signal", rack_item_parasite_changed, self,
                            NULL);
      self->block_updates = 0;
    }
  self->proxy = proxy;
  self->path = g_intern_string (self->path);
  if (self->proxy)
    {
      bse_proxy_connect (self->proxy,
                         "swapped_signal::release", rack_item_remove_proxy, self,
                         "swapped_signal::parasite-changed", rack_item_parasite_changed, self,
                         NULL);
      rack_item_parasite_changed (self);
    }
}

static void
rack_item_button_press (GxkRackItem    *xitem,
                        GdkEventButton *event)
{
#if 0
  BstRackItem *self = BST_RACK_ITEM (xitem);
  if (event->button == 3)
    {
      gboolean can_clone = self->pocket && self->bparam && self->proxy && self->pspec;
      guint id;
      
      if (!self->choice)
        {
          self->controller_choice = create_controller_menu ();
          self->choice = bst_choice_menu_createv ("<BEAST-RackItem>/EditPopup",
                                                  BST_CHOICE_TITLE (_("Edit Rack Item")),
                                                  BST_CHOICE_SEPERATOR,
                                                  BST_CHOICE_SUBMENU (_("Controller"), self->controller_choice),
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
      bst_choice_menu_set_item_sensitive (self->choice, 1005, can_clone);
      sensitize_controller_menu (self->controller_choice, self->pspec);
      id = bst_choice_modal (self->choice, event->button, event->time);
      switch (id)
        {
          gint t;
        case 0:
          /* menu aborted */
          break;
        default: /* 1.. are from controller submenu */
          bse_data_pocket_set_string (self->pocket, self->entry, "property-controller",
                                      controller_info_from_menu_id (id));
          break;
        case 1001:
          t = bse_data_pocket_get_int (self->pocket, self->entry, "property-hspan");
          t += 1;
          t = MAX (t, 1);
          bse_data_pocket_set_int (self->pocket, self->entry, "property-hspan", t);
          break;
        case 1002:
          t = bse_data_pocket_get_int (self->pocket, self->entry, "property-vspan");
          t += 1;
          t = MAX (t, 1);
          bse_data_pocket_set_int (self->pocket, self->entry, "property-vspan", t);
          break;
        case 1003:
          t = bse_data_pocket_get_int (self->pocket, self->entry, "property-hspan");
          t -= 1;
          t = MAX (t, 1);
          bse_data_pocket_set_int (self->pocket, self->entry, "property-hspan", t);
          break;
        case 1004:
          t = bse_data_pocket_get_int (self->pocket, self->entry, "property-vspan");
          t -= 1;
          t = MAX (t, 1);
          bse_data_pocket_set_int (self->pocket, self->entry, "property-vspan", t);
          break;
        case 1005:
          id = bse_data_pocket_create_entry (self->pocket);
          bse_data_pocket_set_string (self->pocket, id, "property-controller", bst_param_get_view_name (self->bparam));
          bse_data_pocket_set_object (self->pocket, id, "property-object", self->proxy);
          bse_data_pocket_set_string (self->pocket, id, "property-name", self->pspec->name);
          break;
        case 1006:
          bse_data_pocket_delete_entry (self->pocket, self->entry);
          break;
        }
    }
#endif
}
