/* GXK - Gtk+ Extension Kit
 * Copyright (C) 2005 Tim Janik
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
#include "gxknotebook.h"
#include <string.h>


/* --- properties --- */
enum {
  PROP_0,
  PROP_ASSORTMENT
};

/* --- variables --- */
static GQuark quark_page_data = 0;

/* --- functions --- */
G_DEFINE_TYPE (GxkNotebook, gxk_notebook, GTK_TYPE_NOTEBOOK);

static void
gxk_notebook_init (GxkNotebook *self)
{
}

static void
gxk_notebook_assortment_added (gpointer                client_data,
                               GtkWindow              *window,
                               GxkAssortment          *assortment,
                               GtkWidget              *publisher)
{
  GxkNotebook *self = GXK_NOTEBOOK (client_data);
  if (self->assortment_name && assortment->publishing_name &&
      strcmp (self->assortment_name, assortment->publishing_name) == 0)
    gxk_notebook_set_assortment (self, assortment);
}

static void
gxk_notebook_assortment_removed (gpointer                client_data,
                                 GtkWindow              *window,
                                 GxkAssortment          *assortment,
                                 GtkWidget              *publisher)
{
  GxkNotebook *self = GXK_NOTEBOOK (client_data);
  if (self->assortment == assortment)
    gxk_notebook_set_assortment (self, NULL);
}

static void
gxk_notebook_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  GxkNotebook *self = GXK_NOTEBOOK (object);
  GtkWidget *widget = GTK_WIDGET (self);
  switch (prop_id)
    {
    case PROP_ASSORTMENT:
      if (self->assortment_name)
        {
          GtkWidget *toplevel = gtk_widget_get_toplevel (widget);
          if (GTK_IS_WINDOW (toplevel))
            gxk_window_remove_assortment_client (GTK_WINDOW (toplevel), self);
          g_free (self->assortment_name);
        }
      self->assortment_name = g_value_dup_string (value);
      if (self->assortment_name)
        {
          GtkWidget *toplevel = gtk_widget_get_toplevel (widget);
          if (GTK_IS_WINDOW (toplevel))
            gxk_window_add_assortment_client (GTK_WINDOW (toplevel), gxk_notebook_assortment_added, gxk_notebook_assortment_removed, self);
        }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gxk_notebook_get_property (GObject     *object,
                           guint        prop_id,
                           GValue      *value,
                           GParamSpec  *pspec)
{
  GxkNotebook *self = GXK_NOTEBOOK (object);
  switch (prop_id)
    {
    case PROP_ASSORTMENT:
      g_value_set_string (value, self->assortment_name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gxk_notebook_hierarchy_changed (GtkWidget *widget,
                                GtkWidget *previous_toplevel)
{
  GxkNotebook *self = GXK_NOTEBOOK (widget);
  if (GTK_IS_WINDOW (previous_toplevel) && self->assortment_name)
    gxk_window_remove_assortment_client (GTK_WINDOW (previous_toplevel), self);
  if (GTK_WIDGET_CLASS (gxk_notebook_parent_class)->hierarchy_changed)
    GTK_WIDGET_CLASS (gxk_notebook_parent_class)->hierarchy_changed (widget, previous_toplevel);
  if (self->assortment_name)
    {
      GtkWidget *toplevel = gtk_widget_get_toplevel (widget);
      if (GTK_IS_WINDOW (toplevel))
        gxk_window_add_assortment_client (GTK_WINDOW (toplevel), gxk_notebook_assortment_added, gxk_notebook_assortment_removed, self);
    }
}

static void
notebook_assortment_entry_added (GxkAssortment      *assortment,
                                 GxkAssortmentEntry *entry,
                                 GxkNotebook        *self)
{
  if (GTK_IS_WIDGET (entry->object))
    {
      GtkWidget *child = GTK_WIDGET (entry->object);
      if (!GTK_IS_WINDOW (child) && child->parent == NULL)
        {
          GtkNotebook *notebook = GTK_NOTEBOOK (self);
          GtkWidget *tab = gxk_notebook_create_tabulator (entry->label, entry->stock_icon, entry->tooltip);
          g_object_set_qdata (child, quark_page_data, entry->user_data);
          gtk_notebook_insert_page (notebook, child, tab, g_slist_index (assortment->entries, entry));
        }
    }
}

static void
notebook_assortment_entry_changed (GxkAssortment      *assortment,
                                   GxkAssortmentEntry *entry,
                                   GxkNotebook        *self)
{
  GtkNotebook *notebook = GTK_NOTEBOOK (self);
  GList *list, *children = gtk_container_get_children (GTK_CONTAINER (self));
  for (list = children; list; list = list->next)
    if (g_object_get_qdata (list->data, quark_page_data) == entry->user_data)
      {
        GtkWidget *tab = gtk_notebook_get_tab_label (notebook, list->data);
        gxk_notebook_change_tabulator (tab, entry->label, entry->stock_icon, entry->tooltip);
        break;
      }
  g_list_free (children);
}

static void
notebook_assortment_entry_remove (GxkAssortment      *assortment,
                                  GxkAssortmentEntry *entry,
                                  GxkNotebook        *self)
{
  GList *list, *children = gtk_container_get_children (GTK_CONTAINER (self));
  for (list = children; list; list = list->next)
    if (g_object_get_qdata (list->data, quark_page_data) == entry->user_data)
      {
        GtkContainer *container = GTK_CONTAINER (self);
        gtk_container_remove (container, list->data);
        break;
      }
  g_list_free (children);
}

static void
notebook_assortment_selection_changed (GxkAssortment *assortment,
                                       GxkNotebook   *self)
{
  GtkNotebook *notebook = GTK_NOTEBOOK (self);
  if (assortment->selected)
    {
      GList *list, *children = gtk_container_get_children (GTK_CONTAINER (self));
      for (list = children; list; list = list->next)
        if (g_object_get_qdata (list->data, quark_page_data) == assortment->selected->user_data)
          {
            gxk_notebook_set_current_page_widget (notebook, list->data);
            break;
          }
      g_list_free (children);
    }
}

void
gxk_notebook_set_assortment (GxkNotebook    *self,
                             GxkAssortment  *assortment)
{
  g_return_if_fail (GXK_IS_NOTEBOOK (self));
  GtkNotebook *notebook = GTK_NOTEBOOK (self);
  if (self->assortment == assortment)
    return;
  if (self->assortment)
    {
      GtkContainer *container = GTK_CONTAINER (self);
      g_signal_handlers_disconnect_by_func (self->assortment, notebook_assortment_entry_added, self);
      g_signal_handlers_disconnect_by_func (self->assortment, notebook_assortment_entry_changed, self);
      g_signal_handlers_disconnect_by_func (self->assortment, notebook_assortment_entry_remove, self);
      g_signal_handlers_disconnect_by_func (self->assortment, notebook_assortment_selection_changed, self);
      g_object_unref (self->assortment);
      GtkWidget *child = gtk_notebook_current_widget (notebook);
      while (child)
        {
          gtk_container_remove (container, child);
          child = gtk_notebook_current_widget (notebook);
        }
    }
  self->assortment = assortment;
  if (self->assortment)
    {
      g_object_ref (self->assortment);
      g_object_connect (self->assortment,
                        "signal::entry-added", notebook_assortment_entry_added, self,
                        "signal::entry-changed", notebook_assortment_entry_changed, self,
                        "signal::entry-remove", notebook_assortment_entry_remove, self,
                        "signal::selection-changed", notebook_assortment_selection_changed, self,
                        NULL);
      GSList *slist;
      for (slist = self->assortment->entries; slist; slist = slist->next)
        notebook_assortment_entry_added (self->assortment, slist->data, self);
    }
}

static void
gxk_notebook_switch_page (GtkNotebook     *notebook,
                          GtkNotebookPage *page,
                          guint            page_num)
{
  GxkNotebook *self = GXK_NOTEBOOK (notebook);
  GTK_NOTEBOOK_CLASS (gxk_notebook_parent_class)->switch_page (notebook, page, page_num);
  gxk_widget_viewable_changed (GTK_WIDGET (self));
  if (self->assortment)
    {
      GtkWidget *child = gtk_notebook_current_widget (notebook);
      if (child)
        gxk_assortment_select_data (self->assortment, g_object_get_qdata (child, quark_page_data));
    }
}

static void
gxk_notebook_destroy (GtkObject *object)
{
  GxkNotebook *self = GXK_NOTEBOOK (object);
  g_free (self->assortment_name);
  self->assortment_name = NULL;
  gxk_notebook_set_assortment (self, NULL);
  GTK_OBJECT_CLASS (gxk_notebook_parent_class)->destroy (object);
}

static void
gxk_notebook_finalize (GObject *object)
{
  GxkNotebook *self = GXK_NOTEBOOK (object);
  g_free (self->assortment_name);
  self->assortment_name = NULL;
  gxk_notebook_set_assortment (self, NULL);
  G_OBJECT_CLASS (gxk_notebook_parent_class)->finalize (object);
}

static void
gxk_notebook_class_init (GxkNotebookClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  GtkNotebookClass *notebook_class = GTK_NOTEBOOK_CLASS (class);
  
  quark_page_data = g_quark_from_static_string ("gxk-notebook-page-data");

  gobject_class->finalize = gxk_notebook_finalize;
  gobject_class->set_property = gxk_notebook_set_property;
  gobject_class->get_property = gxk_notebook_get_property;
  
  object_class->destroy = gxk_notebook_destroy;
  
  widget_class->hierarchy_changed = gxk_notebook_hierarchy_changed;

  notebook_class->switch_page = gxk_notebook_switch_page;

  g_object_class_install_property (G_OBJECT_CLASS (object_class),
				   PROP_ASSORTMENT,
				   g_param_spec_string ("assortment", NULL, NULL,
							NULL, G_PARAM_READWRITE));
}
