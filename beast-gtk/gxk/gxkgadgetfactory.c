/* GXK - Gtk+ Extension Kit
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
#include "gxkgadgetfactory.h"
#include <string.h>

enum {
  PROP_0,
  PROP_NAME,
  PROP_PER_LIST,
  PROP_PER_ACTION,
  PROP_ACTION_LIST,
  PROP_ACTIVATABLE,
  PROP_REGULATE
};


/* --- prototypes --- */
static void     gxk_gadget_factory_class_init           (GxkGadgetFactoryClass  *class);
static void     gxk_gadget_factory_init                 (GxkGadgetFactory       *self);
static void     gxk_gadget_factory_finalize             (GObject                *object);
static void     gxk_gadget_factory_set_property         (GObject                *object,
                                                         guint                   param_id,
                                                         const GValue           *value,
                                                         GParamSpec             *pspec);
static void     gxk_gadget_factory_get_property         (GObject                *object,
                                                         guint                   param_id,
                                                         GValue                 *value,
                                                         GParamSpec             *pspec);
static void     gadget_factory_match_action_list        (GxkActionFactory       *afactory,
                                                         const gchar            *prefix,
                                                         GxkActionList          *alist);


/* --- static variables --- */
static gpointer parent_class = NULL;
static GQuark   quark_gadget_factory_hook = 0;


/* --- functions --- */
GType
gxk_gadget_factory_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static const GTypeInfo type_info = {
        sizeof (GxkGadgetFactoryClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gxk_gadget_factory_class_init,
        NULL,   /* class_finalize */
        NULL,   /* class_data */
        sizeof (GxkGadgetFactory),
        0,      /* n_preallocs */
        (GInstanceInitFunc) gxk_gadget_factory_init,
      };
      type = g_type_register_static (GXK_TYPE_ACTION_FACTORY, "GxkGadgetFactory", &type_info, 0);
    }
  return type;
}

static void
gxk_gadget_factory_class_init (GxkGadgetFactoryClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GxkActionFactoryClass *afactory_class = GXK_ACTION_FACTORY_CLASS (class);
  parent_class = g_type_class_peek_parent (class);

  quark_gadget_factory_hook = g_quark_from_static_string ("GxkGadgetFactory-hook");

  gobject_class->set_property = gxk_gadget_factory_set_property;
  gobject_class->get_property = gxk_gadget_factory_get_property;
  gobject_class->finalize = gxk_gadget_factory_finalize;
  afactory_class->match_action_list = gadget_factory_match_action_list;
  g_object_class_install_property (gobject_class, PROP_NAME,
                                   g_param_spec_string ("name", NULL, NULL, NULL, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_ACTION_LIST,
                                   g_param_spec_string ("action-list", NULL, NULL, NULL, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_PER_LIST,
                                   g_param_spec_string ("per-list", NULL, NULL, NULL, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_PER_ACTION,
                                   g_param_spec_string ("per-action", NULL, NULL, NULL, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_ACTIVATABLE,
                                   g_param_spec_string ("activatable", NULL, NULL, NULL, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_REGULATE,
                                   g_param_spec_string ("regulate", NULL, NULL, NULL, G_PARAM_READWRITE));
}

static void
gxk_gadget_factory_init (GxkGadgetFactory *self)
{
  self->window = NULL;
  self->gadget = NULL;
  self->timer = 0;
  self->name = NULL;
}

static void
gxk_gadget_factory_set_property (GObject      *object,
                                 guint         param_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GxkGadgetFactory *self = GXK_GADGET_FACTORY (object);
  switch (param_id)
    {
    case PROP_NAME:
      g_free (self->name);
      self->name = g_value_dup_string (value);
      break;
    case PROP_ACTION_LIST:
      g_free (self->action_list);
      self->action_list = g_value_dup_string (value);
      break;
    case PROP_PER_LIST:
      g_free (self->per_list);
      self->per_list = g_value_dup_string (value);
      break;
    case PROP_PER_ACTION:
      g_free (self->per_action);
      self->per_action = g_value_dup_string (value);
      break;
    case PROP_ACTIVATABLE:
      g_free (self->activatable);
      self->activatable = g_value_dup_string (value);
      break;
    case PROP_REGULATE:
      g_free (self->regulate);
      self->regulate = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
gxk_gadget_factory_get_property (GObject    *object,
                                 guint       param_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GxkGadgetFactory *self = GXK_GADGET_FACTORY (object);
  switch (param_id)
    {
    case PROP_NAME:
      g_value_set_string (value, self->name);
      break;
    case PROP_ACTION_LIST:
      g_value_set_string (value, self->action_list);
      break;
    case PROP_PER_LIST:
      g_value_set_string (value, self->per_list);
      break;
    case PROP_PER_ACTION:
      g_value_set_string (value, self->per_action);
      break;
    case PROP_ACTIVATABLE:
      g_value_set_string (value, self->activatable);
      break;
    case PROP_REGULATE:
      g_value_set_string (value, self->regulate);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
gxk_gadget_factory_finalize (GObject *object)
{
  GxkGadgetFactory *self = GXK_GADGET_FACTORY (object);

  g_assert (self->window == NULL);
  g_assert (self->timer == 0);
  g_free (self->per_action);
  g_free (self->per_list);
  g_free (self->name);
  g_free (self->activatable);
  g_free (self->action_list);
  g_free (self->regulate);
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gadget_factory_match_action_list (GxkActionFactory       *afactory,
                                  const gchar            *prefix,
                                  GxkActionList          *alist)
{
  GxkGadgetFactory *self = GXK_GADGET_FACTORY (afactory);
  if (self->action_list && strcmp (self->action_list, prefix) == 0)
    {
      const gchar *domain = gxk_gadget_get_domain (self);
      const gchar *wname = GTK_WIDGET (self->window)->name;
      gchar *name = g_strdup_printf ("<%s>/%s/", wname ? wname : "xUNNAMEDx", prefix);
      guint i, n = self->per_action ? gxk_action_list_get_n_actions (alist) : 0;
      if (self->per_list)
        {
          GxkGadgetOpt *options;
          options = gxk_gadget_options ("action-path", name, NULL);
          gxk_gadget_create_add (domain, self->per_list, self->gadget, options);
          gxk_gadget_free_options (options);
        }
      for (i = 0; i < n; i++)
        {
          GxkGadgetOpt *options;
          GxkAction action;
          GxkGadget *gadget;
          gchar *str1;
          gxk_action_list_get_action (alist, i, &action);
          str1 = g_strconcat (name, action.key, NULL);
          options = gxk_gadget_options ("action-name", action.name,
                                        "action-key", action.key,
                                        "action-path", str1,
                                        "action-accel", action.accelerator,
                                        "action-tooltip", action.tooltip,
                                        "action-stock", action.stock_icon,
                                        NULL);
          g_free (str1);
          gadget = gxk_gadget_create_add (domain, self->per_action, self->gadget, options);
          gxk_gadget_free_options (options);
          if (GTK_IS_WIDGET (gadget))
            {
              GxkGadget *child = self->activatable ? gxk_gadget_find (gadget, self->activatable) : NULL;
              if (GTK_IS_WIDGET (child))
                {
                  const gchar *signal = GTK_IS_BUTTON (child) ? "clicked" : "activate"; /* work around buttons */
                  g_signal_connect_swapped (child, signal,
                                            G_CALLBACK (gxk_action_activate_callback),
                                            (gpointer) action.action_data);
                }
              child = self->regulate ? gxk_gadget_find (gadget, self->regulate) : NULL;
              if (GTK_IS_WIDGET (child))
                gxk_action_list_regulate_widget (alist, i, child);
            }
        }
      g_free (name);
      if (!wname && n)
        g_warning ("GxkGadgetFactory: factory toplevel has NULL name: %p", self->window);
    }
}

static gboolean
gadget_factory_check_anchored (gpointer data)
{
  GxkGadgetFactory *self = data;
  GtkWidget *toplevel;
  gboolean is_window;
  GDK_THREADS_ENTER ();
  toplevel = self->gadget ? gxk_widget_get_attach_toplevel (self->gadget) : NULL;
  is_window = GTK_IS_WINDOW (toplevel);
  if (is_window && !self->window)
    {
      self->window = g_object_ref (toplevel);
      gxk_window_add_action_factory (self->window, GXK_ACTION_FACTORY (self));
    }
  else if (self->window && !is_window)
    {
      gxk_window_remove_action_factory (self->window, GXK_ACTION_FACTORY (self));
      g_object_unref (self->window);
      self->window = NULL;
    }
  self->timer = 0;
  GDK_THREADS_LEAVE ();
  return FALSE;
}

void
gxk_gadget_factory_check_anchored (GxkGadgetFactory *self)
{
  if (self->gadget && !self->timer)
    self->timer = g_idle_add_full (G_PRIORITY_HIGH,
                                   gadget_factory_check_anchored,
                                   self, NULL);
}

static void
destroy_factory_slist (gpointer data)
{
  GSList *slist = data;
  while (slist)
    {
      GxkGadgetFactory *fact = g_slist_pop_head (&slist);
      g_object_run_dispose (G_OBJECT (fact));
      fact->gadget = NULL;
      g_object_unref (fact);
    }
}

void
gxk_gadget_factory_attach (GxkGadgetFactory *self,
                           GxkGadget        *gadget)
{
  GSList *slist;

  g_return_if_fail (self->gadget == NULL);
  g_return_if_fail (GTK_IS_WIDGET (gadget));

  g_object_ref (self);
  slist = g_object_steal_qdata (gadget, quark_gadget_factory_hook);
  slist = g_slist_prepend (slist, self);
  g_object_set_qdata_full (gadget, quark_gadget_factory_hook, slist, destroy_factory_slist);
  self->gadget = gadget;
  g_signal_connect_object (gadget, "hierarchy-changed",
                           G_CALLBACK (gxk_gadget_factory_check_anchored), self, G_CONNECT_SWAPPED);
  gxk_gadget_factory_check_anchored (self);
}


/* --- gadget type hooks --- */
static GxkGadget*
gadget_factory_create (GType         type,
                       const gchar  *name)
{
  return g_object_new (type, "name", name, NULL);
}

static GParamSpec*
gadget_factory_find_prop (GxkGadget    *gadget,
                          const gchar  *prop_name)
{
  return g_object_class_find_property (G_OBJECT_GET_CLASS (gadget), prop_name);
}

static void
gadget_factory_adopt (GxkGadget *gadget,
                      GxkGadget *parent)
{
  gxk_gadget_factory_attach (gadget, GTK_WIDGET (parent));
}

static void* return_NULL (void) { return NULL; }

static const GxkGadgetType gadget_factory_def = {
  gadget_factory_create,
  gadget_factory_find_prop,
  (void(*)(GxkGadget*,const gchar*,const GValue*)) g_object_set_property,
  gadget_factory_adopt,
  (void*) return_NULL,/* find_pack */
  NULL,               /* set_pack */
};
const GxkGadgetType *_gxk_gadget_factory_def = &gadget_factory_def;
