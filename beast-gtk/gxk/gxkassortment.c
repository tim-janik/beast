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
#include "gxkassortment.h"
#include "gxkstock.h"
#include <string.h>
#include <libintl.h>


/* --- assortment --- */
static gpointer
assortment_entry_copy (gpointer boxed)
{
  GxkAssortmentEntry *entry = boxed;
  entry->ref_count += 1;
  return entry;
}

static void
assortment_entry_free (GxkAssortmentEntry *aentry)
{
  g_return_if_fail (aentry->ref_count > 0);
  aentry->ref_count -= 1;
  if (!aentry->ref_count)
    {
      g_free (aentry->label);
      g_free (aentry->stock_icon);
      g_free (aentry->tooltip);
      if (aentry->free_func)
        aentry->free_func (aentry->user_data, aentry->object, aentry->owner);
      if (aentry->object)
        g_object_unref (aentry->object);
      g_free (aentry);
    }
}

static void
assortment_entry_free_boxed (gpointer boxed)
{
  GxkAssortmentEntry *aentry = boxed;
  assortment_entry_free (aentry);
}

GType
gxk_assortment_entry_get_type (void)
{
  static GType type_id = 0;
  if (!type_id)
    type_id = g_boxed_type_register_static ("GxkAssortmentEntry", assortment_entry_copy, assortment_entry_free_boxed);
  return type_id;
}

G_DEFINE_TYPE (GxkAssortment, gxk_assortment, G_TYPE_OBJECT);
static guint signal_entry_added = 0;
static guint signal_entry_changed = 0;
static guint signal_entry_remove = 0;
static guint signal_selection_changed = 0;

void
gxk_assortment_init (GxkAssortment *self)
{
}

GxkAssortment*
gxk_assortment_new (void)
{
  return g_object_new (GXK_TYPE_ASSORTMENT, NULL);
}

void
gxk_assortment_dispose (GxkAssortment *self)
{
  g_return_if_fail (GXK_IS_ASSORTMENT (self));
  g_object_run_dispose ((GObject*) self);
}

void
gxk_assortment_block_selection (GxkAssortment *self)
{
  g_return_if_fail (GXK_IS_ASSORTMENT (self));
  self->block_count++;
}

void
gxk_assortment_unblock_selection (GxkAssortment *self)
{
  g_return_if_fail (GXK_IS_ASSORTMENT (self));
  g_return_if_fail (self->block_count > 0);
  self->block_count--;
}

void
gxk_assortment_select (GxkAssortment      *self,
                       GxkAssortmentEntry *entry)
{
  g_return_if_fail (GXK_IS_ASSORTMENT (self));
  if (!self->block_count && entry != self->selected)
    {
      self->selected = entry;
      g_signal_emit (self, signal_selection_changed, 0);
    }
}

void
gxk_assortment_select_data (GxkAssortment          *self,
                            gpointer                entry_user_data)
{
  g_return_if_fail (GXK_IS_ASSORTMENT (self));
  gxk_assortment_select (self, gxk_assortment_find_data (self, entry_user_data));
}

GxkAssortmentEntry*
gxk_assortment_find_data (GxkAssortment          *self,
                          gpointer                entry_user_data)
{
  g_return_val_if_fail (GXK_IS_ASSORTMENT (self), NULL);
  GSList *slist;
  for (slist = self->entries; slist; slist = slist->next)
    {
      GxkAssortmentEntry *entry = slist->data;
      if (entry->user_data == entry_user_data)
        return entry;
    }
  return NULL;
}

GxkAssortmentEntry*
gxk_assortment_insert (GxkAssortment          *self,
                       guint                   position,
                       const gchar            *label,
                       const gchar            *stock_icon, /* maybe NULL */
                       const gchar            *tooltip,
                       gpointer                user_data,
                       GObject                *object,
                       gpointer                owner,
                       GxkAssortmentDelete     free_func)
{
  g_return_val_if_fail (GXK_IS_ASSORTMENT (self), NULL);

  GxkAssortmentEntry *aentry = g_new0 (GxkAssortmentEntry, 1);
  aentry->label = g_strdup (label);
  aentry->stock_icon = g_strdup (stock_icon);
  aentry->tooltip = g_strdup (tooltip);
  aentry->user_data = user_data;
  aentry->object = object ? g_object_ref (object) : NULL;
  aentry->owner = owner;
  aentry->free_func = free_func;
  aentry->ref_count = 1;
  self->entries = g_slist_insert (self->entries, aentry, position);
  g_signal_emit (self, signal_entry_added, 0, aentry);
  return aentry;
}

void
gxk_assortment_changed (GxkAssortment          *self,
                        GxkAssortmentEntry     *entry)
{
  g_return_if_fail (GXK_IS_ASSORTMENT (self));
  g_return_if_fail (g_slist_find (self->entries, entry) != NULL);

  g_signal_emit (self, signal_entry_changed, 0, entry);
}

void
gxk_assortment_remove (GxkAssortment          *self,
                       GxkAssortmentEntry     *entry)
{
  g_return_if_fail (GXK_IS_ASSORTMENT (self));

  GSList *last = NULL, *slist;
  for (slist = self->entries; slist; last = slist, slist = last->next)
    if (slist->data == entry)
      break;
  g_return_if_fail (slist != NULL);

  gboolean selection_changed = FALSE;
  if (self->selected == entry)
    {
      if (slist->next)
        self->selected = slist->next->data;
      else if (last)
        self->selected = last->data;
      else
        self->selected = NULL;
      selection_changed = TRUE;
    }
  if (last)
    last->next = slist->next;
  else
    self->entries = slist->next;
  g_slist_free_1 (slist);
  g_signal_emit (self, signal_entry_remove, 0, entry);
  assortment_entry_free (entry);
  if (selection_changed)
    g_signal_emit (self, signal_selection_changed, 0);
}

static void
assortment_dispose (GObject *object)
{
  GxkAssortment *self = GXK_ASSORTMENT (object);
  g_return_if_fail (GXK_IS_ASSORTMENT (self));
  gboolean was_selected = self->selected != NULL;
  self->selected = NULL;
  while (self->entries)
    assortment_entry_free (g_slist_pop_head (&self->entries));
  if (was_selected)
    g_signal_emit (self, signal_selection_changed, 0);
  G_OBJECT_CLASS (gxk_assortment_parent_class)->dispose (object);
}

static void
assortment_finalize (GObject *object)
{
  GxkAssortment *self = GXK_ASSORTMENT (object);
  g_return_if_fail (GXK_IS_ASSORTMENT (self));
  self->selected = NULL;
  while (self->entries)
    assortment_entry_free (g_slist_pop_head (&self->entries));
  g_free (self->publishing_name);
  self->publishing_name = NULL;
  G_OBJECT_CLASS (gxk_assortment_parent_class)->finalize (object);
}

void
gxk_assortment_class_init (GxkAssortmentClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->dispose = assortment_dispose;
  gobject_class->finalize = assortment_finalize;

  signal_entry_added = g_signal_new ("entry-added", G_OBJECT_CLASS_TYPE (class),
                                     G_SIGNAL_RUN_LAST,
                                     G_STRUCT_OFFSET (GxkAssortmentClass, entry_added),
                                     NULL, NULL,
                                     gxk_marshal_VOID__BOXED, G_TYPE_NONE, 1, GXK_TYPE_ASSORTMENT_ENTRY);
  signal_entry_changed = g_signal_new ("entry-changed", G_OBJECT_CLASS_TYPE (class),
                                       G_SIGNAL_RUN_LAST,
                                       G_STRUCT_OFFSET (GxkAssortmentClass, entry_changed),
                                       NULL, NULL,
                                       gxk_marshal_VOID__BOXED, G_TYPE_NONE, 1, GXK_TYPE_ASSORTMENT_ENTRY);
  signal_entry_remove = g_signal_new ("entry-remove", G_OBJECT_CLASS_TYPE (class),
                                      G_SIGNAL_RUN_LAST,
                                      G_STRUCT_OFFSET (GxkAssortmentClass, entry_remove),
                                      NULL, NULL,
                                      gxk_marshal_VOID__BOXED, G_TYPE_NONE, 1, GXK_TYPE_ASSORTMENT_ENTRY);
  signal_selection_changed = g_signal_new ("selection-changed", G_OBJECT_CLASS_TYPE (class),
                                           G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
                                           G_STRUCT_OFFSET (GxkAssortmentClass, selection_changed),
                                           NULL, NULL, gtk_signal_default_marshaller, G_TYPE_NONE, 0);
}

/* --- publishing --- */
static GQuark quark_assortment_clients = 0;
static GQuark quark_window_assortments = 0;
static GQuark quark_widget_assortments = 0;

void
gxk_init_assortments (void)
{
  quark_assortment_clients = g_quark_from_static_string ("gxk-assortment-clients");
  quark_window_assortments = g_quark_from_static_string ("gxk-window-assortments");
  quark_widget_assortments = g_quark_from_static_string ("gxk-widget-assortments");
}

typedef struct AssortmentLink AssortmentLink;
struct AssortmentLink {
  GxkAssortment  *assortment;
  guint           ref_count;
  GtkWidget      *widget;       /* publisher */
  GtkWidget      *toplevel;
  AssortmentLink *next;         /* linked list on toplevel */
};

static void
assortment_link_unref (AssortmentLink *alink)
{
  g_return_if_fail (alink->ref_count > 0);
  alink->ref_count--;
  if (!alink->ref_count)
    {
      g_object_unref (alink->assortment);
      g_free (alink);
    }
}

static void
window_destroy_assortments (gpointer data)
{
  AssortmentLink *anode = data;
  while (anode)
    {
      AssortmentLink *alink = anode;
      anode = anode->next;
      alink->toplevel = NULL;
      alink->next = NULL;
      assortment_link_unref (alink);
    }
}

typedef struct {
  gpointer                client_data;
  GxkAssortmentClient     added_func;
  GxkAssortmentClient     removed_func;
} AssortmentClient;

static void
window_add_assortment_link (GtkWidget      *window,
                            AssortmentLink *alink)
{
  g_return_if_fail (GTK_IS_WINDOW (window));
  g_return_if_fail (GTK_IS_WIDGET (alink->widget));
  g_return_if_fail (alink->toplevel == NULL);
  g_return_if_fail (alink->next == NULL);
  alink->next = g_object_steal_qdata (window, quark_window_assortments);
  alink->toplevel = window;
  alink->ref_count++;
  g_object_set_qdata_full (window, quark_window_assortments, alink, window_destroy_assortments);
  GSList *slist = g_object_get_qdata (window, quark_assortment_clients);
  while (slist)
    {
      AssortmentClient *aclient = slist->data;
      slist = slist->next;
      if (aclient->added_func)
        aclient->added_func (aclient->client_data, (GtkWindow*) window, alink->assortment, alink->widget);
    }
}

static void
window_remove_assortment_link (AssortmentLink *alink)
{
  GtkWidget *window = alink->toplevel;
  g_return_if_fail (GTK_IS_WINDOW (window));

  AssortmentLink *last = NULL, *anode = g_object_get_qdata (window, quark_window_assortments);
  for (; anode; last = anode, anode = last->next)
    if (anode == alink)
      {
        if (last)
          last->next = alink->next;
        else
          {
            g_object_steal_qdata (window, quark_window_assortments);
            g_object_set_qdata_full (window, quark_window_assortments, alink->next, window_destroy_assortments);
          }
        alink->toplevel = NULL;
        alink->next = NULL;
        GSList *slist = g_object_get_qdata (window, quark_assortment_clients);
        while (slist)
          {
            AssortmentClient *aclient = slist->data;
            slist = slist->next;
            if (aclient->removed_func)
              aclient->removed_func (aclient->client_data, (GtkWindow*) window, alink->assortment, alink->widget);
          }
        assortment_link_unref (alink);
        return;
      }
}

static GSList *publisher_list = NULL;
static gboolean
assortment_idle_publisher (gpointer data)
{
  GDK_THREADS_ENTER ();
  while (publisher_list)
    {
      GtkWidget *widget = g_slist_pop_head (&publisher_list);
      GtkWidget *toplevel = gtk_widget_get_toplevel (widget);
      GSList *slist = g_object_get_qdata (widget, quark_widget_assortments);
      while (slist)
        {
          AssortmentLink *alink = slist->data;
          if (alink->toplevel != toplevel)
            {
              if (alink->toplevel)
                window_remove_assortment_link (alink);
              if (toplevel)
                window_add_assortment_link (toplevel, alink);
            }
          slist = slist->next;
        }
      g_object_unref (widget);
    }
  GDK_THREADS_LEAVE ();
  return FALSE;
}

static void
publisher_update_assortments (GtkWidget *widget)
{
  if (!g_slist_find (publisher_list, widget))
    {
      if (!publisher_list)
        g_idle_add_full (GXK_ASSORTMENT_PRIORITY, assortment_idle_publisher, NULL, NULL);
      publisher_list = g_slist_prepend (publisher_list, g_object_ref (widget));
    }
}

static void
delete_assortment_link_list (gpointer data)
{
  GSList *slist = data;
  while (slist)
    {
      AssortmentLink *alink = g_slist_pop_head (&slist);
      if (alink->toplevel)
        window_remove_assortment_link (alink);
      assortment_link_unref (alink);
    }
}

void
gxk_widget_publish_assortment (gpointer       widget,
                               const gchar   *publishing_name,
                               GxkAssortment *assortment)
{
  AssortmentLink *alink = g_new0 (AssortmentLink, 1);
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (GXK_IS_ASSORTMENT (assortment));
  g_return_if_fail (assortment->publishing_name == NULL);
  assortment->publishing_name = g_strdup (publishing_name);
  alink->assortment = g_object_ref (assortment);
  alink->ref_count = 1;
  alink->widget = widget;
  GSList *alink_list = g_object_steal_qdata (widget, quark_widget_assortments);
  alink_list = g_slist_prepend (alink_list, alink);
  g_object_set_qdata_full (widget, quark_widget_assortments, alink_list, delete_assortment_link_list);
  if (!gxk_signal_handler_exists (widget, "hierarchy_changed", G_CALLBACK (publisher_update_assortments), NULL))
    g_object_connect (widget, "signal_after::hierarchy-changed", publisher_update_assortments, NULL, NULL);
  publisher_update_assortments (widget);
}

void
gxk_window_add_assortment_client (GtkWindow              *window,
                                  GxkAssortmentClient     added_func,
                                  GxkAssortmentClient     removed_func,
                                  gpointer                client_data)
{
  g_return_if_fail (GTK_IS_WINDOW (window));
  g_return_if_fail (client_data != NULL);
  AssortmentClient *aclient = g_new0 (AssortmentClient, 1);
  aclient->client_data = client_data;
  aclient->added_func = added_func;
  aclient->removed_func = removed_func;
  GSList *slist = g_object_get_qdata (window, quark_assortment_clients);
  g_object_set_qdata (window, quark_assortment_clients, g_slist_prepend (slist, aclient));
  AssortmentLink *alink;
  for (alink = g_object_get_qdata (window, quark_window_assortments); alink; alink = alink->next)
    if (aclient->added_func)
      aclient->added_func (aclient->client_data, window, alink->assortment, alink->widget);
}

void
gxk_window_remove_assortment_client (GtkWindow *window,
                                     gpointer   client_data)
{
  g_return_if_fail (GTK_IS_WINDOW (window));
  g_return_if_fail (client_data != NULL);
  GSList *last = NULL, *slist = g_object_get_qdata (window, quark_assortment_clients);
  while (slist)
    {
      AssortmentClient *aclient = slist->data;
      if (aclient->client_data == client_data)
        {
          if (last)
            last->next = slist->next;
          else
            g_object_set_qdata (window, quark_assortment_clients, slist->next);
          g_free (aclient);
          return;
        }
      last = slist;
      slist = last->next;
    }
  g_warning ("failed to remove assortment client (%p) from GtkWindow (%p)", client_data, window);
}

/* --- menus --- */
static void     assortment_menu_entry_changed (GxkAssortment          *self,
                                               GxkAssortmentEntry     *entry,
                                               GtkMenu                *menu);

static void
assortment_menu_entry_added (GxkAssortment          *self,
                             GxkAssortmentEntry     *entry,
                             GtkMenu                *menu)
{
  gxk_assortment_block_selection (self);
  GtkWidget *menu_item = g_object_new (GTK_TYPE_IMAGE_MENU_ITEM,
                                       "visible", TRUE,
                                       "user-data", entry->user_data,
                                       NULL);
  GtkWidget *image = g_object_new (GTK_TYPE_IMAGE, "visible", 1, NULL);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), image);
  GtkWidget *label = g_object_new (GTK_TYPE_ACCEL_LABEL,
                                   "visible", entry->label != NULL,
                                   "parent", menu_item,
                                   "accel-widget", menu_item,
                                   "xalign", 0.0,
                                   NULL);
  g_object_set_data_full (menu_item, "gxk-assortment-label", g_object_ref (label), g_object_unref);
  g_object_set_data_full (menu_item, "gxk-assortment-image", g_object_ref (image), g_object_unref);
  gtk_menu_shell_insert (GTK_MENU_SHELL (menu), menu_item, g_slist_index (self->entries, entry));
  gxk_assortment_unblock_selection (self);
  assortment_menu_entry_changed (self, entry, menu);
}

static void
assortment_menu_entry_changed (GxkAssortment          *self,
                               GxkAssortmentEntry     *entry,
                               GtkMenu                *menu)
{
  GList *list;
  for (list = GTK_MENU_SHELL (menu)->children; list; list = list->next)
    {
      gpointer user_data;
      g_object_get (list->data, "user-data", &user_data, NULL);
      if (user_data == entry->user_data)
        break;
    }
  if (list)
    {
      GtkWidget *menu_item = list->data;
      /* using qdata to retrieve label and image because they might have been reparented (GxkMenuButton) */
      GtkWidget *label = g_object_get_data (menu_item, "gxk-assortment-label");
      GtkWidget *image = g_object_get_data (menu_item, "gxk-assortment-image");
      if (GTK_IS_LABEL (label))
        {
          if (entry->label && strcmp (gtk_label_get_text (GTK_LABEL (label)), entry->label) != 0)
            gtk_label_set_text (GTK_LABEL (label), entry->label);
          if (entry->label)
            gtk_widget_show (label);
          else
            gtk_widget_hide (label);
        }
      if (GTK_IS_IMAGE (image))
        {
          gchar *ostock = NULL;
          GtkIconSize isize = 0;
          gtk_image_get_stock (GTK_IMAGE (image), &ostock, &isize);
          if (entry->stock_icon && (!ostock || strcmp (ostock, entry->stock_icon) != 0 || isize != GXK_ICON_SIZE_MENU))
            gtk_image_set_from_stock (GTK_IMAGE (image), entry->stock_icon, GXK_ICON_SIZE_MENU);
          if (entry->stock_icon)
            gtk_widget_show (image);
          else
            gtk_widget_hide (image);
        }
      else if (!image && entry->stock_icon)
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), gxk_stock_image (entry->stock_icon, GXK_ICON_SIZE_MENU));
      gxk_widget_set_tooltip (menu_item, entry->tooltip);
    }
}

static void
assortment_menu_entry_remove (GxkAssortment          *self,
                              GxkAssortmentEntry     *entry,
                              GtkMenu                *menu)
{
  GList *list;
  for (list = GTK_MENU_SHELL (menu)->children; list; list = list->next)
    {
      gpointer user_data;
      g_object_get (list->data, "user-data", &user_data, NULL);
      if (user_data == entry->user_data)
        break;
    }
  if (list)
    gtk_widget_destroy (list->data);
}

static void
assortment_menu_selection_changed (GxkAssortment          *self,
                                   GtkMenu                *menu)
{
  if (self->selected)
    {
      GList *list;
      for (list = GTK_MENU_SHELL (menu)->children; list; list = list->next)
        {
          gpointer user_data;
          g_object_get (list->data, "user-data", &user_data, NULL);
          if (user_data == self->selected->user_data)
            break;
        }
      if (list)
        gxk_menu_set_active (menu, list->data);
    }
}

static void
assortment_menu_selection_done (GtkMenu       *menu,
                                GxkAssortment *self)
{
  if (menu->old_active_menu_item)
    {
      gpointer user_data;
      g_object_get (menu->old_active_menu_item, "user-data", &user_data, NULL);
      gxk_assortment_select_data (self, user_data);
    }
}

void
gxk_assortment_manage_menu (GxkAssortment          *self,
                            GtkMenu                *menu)
{
  g_return_if_fail (GXK_IS_ASSORTMENT (self));
  g_return_if_fail (GTK_IS_MENU (menu));

  g_signal_connect_object (self, "entry-added", G_CALLBACK (assortment_menu_entry_added), menu, 0);
  g_signal_connect_object (self, "entry-changed", G_CALLBACK (assortment_menu_entry_changed), menu, 0);
  g_signal_connect_object (self, "entry-remove", G_CALLBACK (assortment_menu_entry_remove), menu, 0);
  g_signal_connect_object (self, "selection-changed", G_CALLBACK (assortment_menu_selection_changed), menu, 0);
  g_signal_connect_object (menu, "selection-done", G_CALLBACK (assortment_menu_selection_done), self, 0);

  GSList *slist;
  for (slist = self->entries; slist; slist = slist->next)
    assortment_menu_entry_added (self, slist->data, menu);
  assortment_menu_selection_changed (self, menu);
}
