/* BEAST - Bedevilled Audio System
 * Copyright (C) 2003 Tim Janik
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
#include "bstactivatable.h"


#define DEBUG   sfi_debug_keyfunc ("activatable")


/* --- prototypes --- */
static void     bst_activatable_iface_base_init         (BstActivatableIface    *iface);
static void     bst_activatable_iface_base_finalize     (BstActivatableIface    *iface);


/* --- variables --- */
static GSList *update_queue = NULL;
static gulong  queue_handler_id = 0;


/* --- functions --- */
GType
bst_activatable_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static const GTypeInfo type_info = {
        sizeof (BstActivatableIface),                                   /* iface_size */
        (GBaseInitFunc) bst_activatable_iface_base_init,                /* base_init */
        (GBaseFinalizeFunc) bst_activatable_iface_base_finalize,        /* base_finalize */
      };
      type = g_type_register_static (G_TYPE_INTERFACE, "BstActivatable", &type_info, 0);
      g_type_interface_add_prerequisite (type, GTK_TYPE_WIDGET);
    }
  return type;
}

static void
bst_activatable_iface_base_init (BstActivatableIface *iface)
{
  iface->request_update = bst_activatable_default_request_update;
}

static void
bst_activatable_iface_base_finalize (BstActivatableIface *iface)
{
}

void
bst_activatable_activate (BstActivatable *self,
                          gulong          action)
{
  BstActivatableIface *iface;

  g_return_if_fail (BST_ACTIVATABLE (self));

  iface = BST_ACTIVATABLE_GET_IFACE (self);
  if (iface->can_activate (self, action))
    iface->activate (self, action);
  else  /* hum, we got a non activatable action request */
    {
      /* provide user feedback */
      gdk_beep ();
      DEBUG ("failed to execute action (%lu) for %s", action, G_OBJECT_TYPE_NAME (self));
      /* and request updates so this won't happen again */
      bst_activatable_request_update (self);
    }
}

gboolean
bst_activatable_can_activate (BstActivatable *self,
                              gulong          action)
{
  BstActivatableIface *iface;

  g_return_val_if_fail (BST_ACTIVATABLE (self), FALSE);

  iface = BST_ACTIVATABLE_GET_IFACE (self);
  return iface->can_activate (self, action) != FALSE;
}

static gboolean
update_queue_handler (gpointer data)
{
  BstActivatable *self;
  GDK_THREADS_ENTER ();
  self = g_slist_pop_head (&update_queue);
  while (self)
    {
      BstActivatableIface *iface = BST_ACTIVATABLE_GET_IFACE (self);
      if (iface->update)
        iface->update (self);
      g_object_unref (self);
      self = g_slist_pop_head (&update_queue);
    }
  queue_handler_id = 0;
  GDK_THREADS_LEAVE ();
  return FALSE;
}

static void
bst_activatable_queue_update (BstActivatable *self)
{
  g_object_ref (self);
  update_queue = g_slist_prepend (update_queue, self);
  if (!queue_handler_id)
    {
      /* need to have higher priority than GDK_PRIORITY_EVENTS so we update
       * activatable states before the next event (pointer move, key press)
       * is processed
       */
      queue_handler_id = g_timeout_add_full (GDK_PRIORITY_EVENTS - 1, 0,
                                             update_queue_handler, NULL, NULL);
    }
}

void
bst_activatable_update_enqueue (BstActivatable *self)
{
  BstActivatableIface *iface;

  g_return_if_fail (BST_ACTIVATABLE (self));

  iface = BST_ACTIVATABLE_GET_IFACE (self);
  if (G_OBJECT (self)->ref_count &&
      !g_slist_find (update_queue, self))
    bst_activatable_queue_update (self);
}

void
bst_activatable_update_dequeue (BstActivatable *self)
{
  g_return_if_fail (BST_ACTIVATABLE (self));

  if (g_slist_find (update_queue, self))
    {
      update_queue = g_slist_remove (update_queue, self);
      g_object_unref (self);
    }
}

void
bst_activatable_default_request_update (BstActivatable *self)
{
  g_return_if_fail (BST_ACTIVATABLE (self));

  if (GTK_IS_CONTAINER (self))
    {
      GList *list = gtk_container_get_children (GTK_CONTAINER (self));
      BstActivatable *child = g_list_pop_head (&list);
      while (child)
        {
          if (BST_IS_ACTIVATABLE (child))
            bst_activatable_request_update (child);
          child = g_list_pop_head (&list);
        }
    }
}

void
bst_activatable_request_update (BstActivatable *self)
{
  BstActivatableIface *iface;

  g_return_if_fail (BST_ACTIVATABLE (self));

  iface = BST_ACTIVATABLE_GET_IFACE (self);
  if (G_OBJECT (self)->ref_count &&
      !g_slist_find (update_queue, self))
    {
      bst_activatable_queue_update (self);
      iface->request_update (self);
    }
}

void
bst_widget_update_activatable (gpointer widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  while (widget)
    {
      GtkWidget *child = widget;
      if (BST_IS_ACTIVATABLE (widget))
        bst_activatable_request_update (widget);
      widget = child->parent;
    }
}

typedef struct {
  gpointer activate;
  gpointer can_activate;
  gpointer update;
} GenericData;

static void
bst_activatable_generic_init_iface (BstActivatableIface *iface,
                                    gpointer             iface_data)
{
  GenericData *gdata = iface_data;

  iface->activate = gdata->activate;
  iface->can_activate = gdata->can_activate;
  if (gdata->update)
    iface->update = gdata->update;
}

void
bst_type_implement_activatable (GType      type,
                                void     (*activate) (BstActivatable *, gulong),
                                gboolean (*can_activate) (BstActivatable *, gulong),
                                void     (*update)       (BstActivatable *))
{
  GInterfaceInfo activatable_info = {
    (GInterfaceInitFunc) bst_activatable_generic_init_iface,    /* interface_init */
    NULL,                                                       /* interface_finalize */
    NULL                                                        /* interface_data */
  };
  GenericData *gdata = g_new (GenericData, 1);

  g_return_if_fail (activate != NULL);
  g_return_if_fail (can_activate != NULL);

  gdata->activate = activate;
  gdata->can_activate = can_activate;
  gdata->update = update;
  activatable_info.interface_data = gdata;
  g_type_add_interface_static (type, BST_TYPE_ACTIVATABLE, &activatable_info);
}
