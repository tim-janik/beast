/* GXK - Gtk+ Extension Kit
 * Copyright (C) 2003-2004 Tim Janik
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
#include "gxkmenubutton.h"
#include "gxkstock.h"
#include <gdk/gdkkeysyms.h>

enum {
  MENU_BUTTON_PROP_0,
  MENU_BUTTON_PROP_MENU,
  MENU_BUTTON_PROP_COMBO_ARROW,
  MENU_BUTTON_PROP_PUSH_IN,
  MENU_BUTTON_PROP_STOCK_SIZE,
  MENU_BUTTON_PROP_SHOW_SELECTION
};

/* --- prototypes --- */
static void     menu_button_remove_contents          (GxkMenuButton       *self);


/* --- variables --- */
static guint menu_button_signal_changed = 0;


/* --- functions --- */
G_DEFINE_TYPE (GxkMenuButton, gxk_menu_button, GTK_TYPE_BUTTON);

static void
menu_button_popup (GxkMenuButton *self,
                   guint          button,
                   guint32        time)
{
  GdkEvent *event = gtk_get_current_event ();
  GtkWidget *menu_item, *widget = GTK_WIDGET (self);
  gint x, y;
  gtk_widget_grab_focus (widget);
  /* fixate sizes across removing child */
  if (self->show_selection)
    g_object_set (self->islot,
                  "width-request", self->islot->requisition.width,
                  "height-request", self->islot->requisition.height,
                  NULL);
  menu_button_remove_contents (self);
  gdk_window_get_origin (widget->window, &x, &y);
  x += widget->allocation.x;
  y += widget->allocation.y;
  gxk_menu_popup (self->menu, x, y, self->push_in, button, event ? gdk_event_get_time (event) : 0);
  menu_item = gtk_menu_get_active (self->menu);
  if (menu_item)
    gtk_menu_shell_select_item (GTK_MENU_SHELL (self->menu), menu_item);
}

static gboolean
menu_button_button_press (GtkWidget      *widget,
                          GdkEventButton *event)
{
  GxkMenuButton *self = GXK_MENU_BUTTON (widget);
  gtk_widget_grab_focus (GTK_WIDGET (self));
  if (self->menu && event->type == GDK_BUTTON_PRESS && event->button == 1)
    menu_button_popup (self, event->button, event->time);
  return TRUE;
}

static gboolean
menu_button_key_press (GtkWidget   *widget,
                       GdkEventKey *event)
{
  GxkMenuButton *self = GXK_MENU_BUTTON (widget);
  switch (event->keyval)
    {
    case GDK_KP_Space: case GDK_space:
      menu_button_popup (self, 0, event->time);
      return TRUE;
    }
  return FALSE;
}

static gboolean
menu_button_mnemonic_activate (GtkWidget *widget,
                               gboolean   group_cycling)
{
  gtk_widget_grab_focus (widget);
  return TRUE;
}

static void
menu_button_proxy_state (GxkMenuButton *self)
{
  if (self->child)
    gtk_widget_set_sensitive (self->child, GTK_WIDGET_IS_SENSITIVE (self->menu_item));
  if (self->image)
    gtk_widget_set_sensitive (self->image, GTK_WIDGET_IS_SENSITIVE (self->menu_item));
}

static void
menu_button_max_size (GxkMenuButton *self)
{
  if (self->child)
    {
      GList *list, *children = GTK_MENU_SHELL (self->menu)->children;
      GtkRequisition child_requisition = { 0, };
      guint width = 0, height = 0;
      for (list = children; list; list = list->next)
        {
          GtkWidget *mitem = list->data;
          if (GTK_WIDGET_VISIBLE (mitem))
            {
              GtkWidget *child = GTK_BIN (mitem)->child;
              if (child && GTK_WIDGET_VISIBLE (child))
                {
                  gtk_widget_size_request (child, &child_requisition);
                  width = MAX (width, child_requisition.width);
                  height = MAX (height, child_requisition.height);
                }
            }
        }
      gtk_widget_size_request (self->child, &child_requisition);
      width = MAX (width, child_requisition.width);
      height = MAX (height, child_requisition.height);
      g_object_set (self->cslot,
                    "width-request", width,
                    "height-request", height,
                    NULL);
    }
}

static void
menu_button_remove_contents (GxkMenuButton *self)
{
  if (self->menu_item)
    {
      if (self->child)
        {
          gtk_widget_set_sensitive (self->child, TRUE);
          gtk_widget_reparent (self->child, self->menu_item);
          self->child = NULL;
        }
      if (self->image)
        {
          g_object_ref (self->image);
          g_object_set (self->image, "sensitive", TRUE, "icon-size", self->old_icon_size, NULL);
          gtk_container_remove (GTK_CONTAINER (self->image->parent), self->image);
          gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (self->menu_item), self->image);
          g_object_unref (self->image);
        }
      self->image = NULL;
      g_signal_handlers_disconnect_by_func (self->menu_item, menu_button_proxy_state, self);
      g_object_unref (self->menu_item);
      self->menu_item = NULL;
      gtk_tooltips_set_tip (GXK_TOOLTIPS, GTK_WIDGET (self), NULL, NULL);
    }
}

void
gxk_menu_button_update (GxkMenuButton *self)
{
  GtkWidget *old_menu_item = self->menu_item;
  if (self->menu)
    {
      menu_button_remove_contents (self);
      self->menu_item = gtk_menu_get_active (self->menu);
      if (self->menu_item)
        {
          GtkTooltipsData *tipdata;
          g_object_ref (self->menu_item);
          if (self->show_selection)
            {
              self->child = GTK_BIN (self->menu_item)->child;
              if (self->child)
                gtk_widget_reparent (self->child, self->cslot);
              if (GTK_IS_IMAGE_MENU_ITEM (self->menu_item))
                self->image = gtk_image_menu_item_get_image (GTK_IMAGE_MENU_ITEM (self->menu_item));
              if (self->image)
                {
                  g_object_get (self->image, "icon-size", &self->old_icon_size, NULL);
                  gtk_widget_reparent (self->image, self->islot);
                  if (self->icon_size)
                    g_object_set (self->image, "icon-size", self->icon_size, NULL);
                }
              g_object_set (self->islot,        /* make room for cslot */
                            "visible", GTK_BIN (self->islot)->child != NULL,
                            NULL);
              gtk_container_child_set (GTK_CONTAINER (self->cslot->parent), self->cslot,
                                       "expand", GTK_BIN (self->islot)->child == NULL,
                                       NULL);   /* vertically center cslot */
              g_object_connect (self->menu_item, "swapped_signal::state_changed", menu_button_proxy_state, self, NULL);
              menu_button_proxy_state (self);
              tipdata = gtk_tooltips_data_get (self->menu_item);
              if (tipdata && tipdata->tip_text)
                gtk_tooltips_set_tip (GXK_TOOLTIPS, GTK_WIDGET (self), tipdata->tip_text, tipdata->tip_private);
              else
                gtk_tooltips_set_tip (GXK_TOOLTIPS, GTK_WIDGET (self),
                                      gxk_widget_get_latent_tooltip (self->menu_item), NULL);
              gtk_widget_queue_resize (GTK_WIDGET (self));
              /* restore slot sizes */
              g_object_set (self->islot,
                            "width-request", -1,
                            "height-request", -1,
                            NULL);
              menu_button_max_size (self);
            }
        }
    }
  if (old_menu_item != self->menu_item)
    g_signal_emit (self, menu_button_signal_changed, 0);
}

static void
menu_button_detacher (GtkWidget *widget,
                      GtkMenu   *menu)
{
  GxkMenuButton *self = GXK_MENU_BUTTON (widget);
  menu_button_remove_contents (self);
  g_signal_handlers_disconnect_by_func (self->menu, gxk_menu_button_update, self);
  g_signal_handlers_disconnect_by_func (self->menu, menu_button_max_size, self);
  self->menu = NULL;
  g_object_notify (self, "menu");
}

static void
gxk_menu_button_set_property (GObject      *object,
                              guint         param_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GxkMenuButton *self = GXK_MENU_BUTTON (object);
  switch (param_id)
    {
      const gchar *cstr;
      gboolean b;
    case MENU_BUTTON_PROP_SHOW_SELECTION:       /* construct */
      b = g_value_get_boolean (value);
      if (self->show_selection != b)
        {
          self->show_selection = b;
          menu_button_remove_contents (self);
          if (GTK_BIN (self)->child)
            gtk_widget_destroy (GTK_BIN (self)->child);
          self->islot = NULL;
          self->cslot = NULL;
          if (self->show_selection)
            {
              GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
              GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
              self->islot = gtk_alignment_new (0.5, 0.5, 1, 1);
              gtk_box_pack_start (GTK_BOX (vbox), self->islot, TRUE, TRUE, 0);
              self->cslot = gtk_alignment_new (0.5, 0.5, 0, 0);
              gtk_box_pack_end (GTK_BOX (vbox), self->cslot, FALSE, TRUE, 0);
              gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
              gtk_container_add (GTK_CONTAINER (self), hbox);
              gtk_widget_show_all (GTK_WIDGET (self));
            }
        }
      break;
    case MENU_BUTTON_PROP_MENU:
      if (self->menu)
        gtk_menu_detach (self->menu);
      self->menu = g_value_get_object (value);
      if (self->menu)
        {
          gxk_menu_attach_as_popup_with_func (self->menu, GTK_WIDGET (self), menu_button_detacher);
          g_object_connect (self->menu,
                            "swapped_signal_after::selection_done", gxk_menu_button_update, self,
                            "swapped_signal_after::size-request", menu_button_max_size, self,
                            NULL);
          gtk_widget_queue_resize (GTK_WIDGET (self));
          gxk_menu_button_update (self);
        }
      break;
    case MENU_BUTTON_PROP_COMBO_ARROW:
      b = g_value_get_boolean (value);
      if (b != self->combo_arrow)
        {
          self->combo_arrow = b;
          if (self->combo_arrow && self->show_selection)
            {
              GtkWidget *alignment = g_object_new (GTK_TYPE_ALIGNMENT,
                                                   "visible", TRUE,
                                                   "yscale", 0.0,
                                                   NULL);
              g_object_new (GTK_TYPE_VBOX,
                            "visible", TRUE,
                            "parent", alignment,
                            "child", g_object_new (GTK_TYPE_ARROW,
                                                   "visible", TRUE,
                                                   "arrow-type", GTK_ARROW_UP,
                                                   "yalign", 1.0,
                                                   NULL),
                            "child", g_object_new (GTK_TYPE_ARROW,
                                                   "visible", TRUE,
                                                   "arrow-type", GTK_ARROW_DOWN,
                                                   "yalign", 0.0,
                                                   NULL),
                            NULL);
              gtk_box_pack_end (GTK_BOX (GTK_BIN (self)->child), alignment, FALSE, TRUE, 0);
            }
        }
      break;
    case MENU_BUTTON_PROP_PUSH_IN:
      self->push_in = g_value_get_boolean (value);
      break;
    case MENU_BUTTON_PROP_STOCK_SIZE:
      cstr = g_value_get_string (value);
      if (cstr)
        self->icon_size = gtk_icon_size_from_name (cstr);
      else
        self->icon_size = 0;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
gxk_menu_button_get_property (GObject    *object,
                              guint       param_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GxkMenuButton *self = GXK_MENU_BUTTON (object);
  switch (param_id)
    {
    case MENU_BUTTON_PROP_MENU:
      g_value_set_object (value, self->menu);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
menu_button_dispose (GObject *object)
{
  GxkMenuButton *self = GXK_MENU_BUTTON (object);
  if (self->menu)
    gtk_menu_detach (self->menu);
  G_OBJECT_CLASS (gxk_menu_button_parent_class)->dispose (object);
}

static void
gxk_menu_button_init (GxkMenuButton *self)
{
}

static void
gxk_menu_button_class_init (GxkMenuButtonClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  gobject_class->set_property = gxk_menu_button_set_property;
  gobject_class->get_property = gxk_menu_button_get_property;
  gobject_class->dispose = menu_button_dispose;
  g_object_class_install_property (gobject_class, MENU_BUTTON_PROP_SHOW_SELECTION,
                                   g_param_spec_boolean ("show-selection", NULL, NULL, TRUE, G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (gobject_class, MENU_BUTTON_PROP_MENU,
                                   g_param_spec_object ("menu", NULL, NULL, GTK_TYPE_MENU, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, MENU_BUTTON_PROP_COMBO_ARROW,
                                   g_param_spec_boolean ("combo-arrow", NULL, NULL, TRUE, G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (gobject_class, MENU_BUTTON_PROP_PUSH_IN,
                                   g_param_spec_boolean ("push-in", NULL, NULL, FALSE, G_PARAM_WRITABLE));
  g_object_class_install_property (gobject_class, MENU_BUTTON_PROP_STOCK_SIZE,
                                   g_param_spec_string ("stock-size", NULL, NULL, NULL, G_PARAM_WRITABLE));
  widget_class->button_press_event = menu_button_button_press;
  widget_class->key_press_event = menu_button_key_press;
  widget_class->mnemonic_activate = menu_button_mnemonic_activate;
  menu_button_signal_changed = g_signal_new ("changed", G_OBJECT_CLASS_TYPE (class),
                                             G_SIGNAL_RUN_FIRST, 0, NULL, NULL,
                                             gtk_signal_default_marshaller, G_TYPE_NONE, 0);
}
