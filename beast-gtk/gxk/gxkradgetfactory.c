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
#include "gxkradgetfactory.h"
#include <string.h>

enum {
  PROP_0,
  PROP_NAME,
  PROP_PER_LIST,
  PROP_PER_BRANCH,
  PROP_PER_ACTION,
  PROP_ACTION_ROOT,
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
                                                         GxkActionList          *alist,
                                                         GtkWidget              *publisher);


/* --- static variables --- */
static gpointer gadget_factory_parent_class = NULL;
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
  gadget_factory_parent_class = g_type_class_peek_parent (class);
  
  quark_gadget_factory_hook = g_quark_from_static_string ("GxkGadgetFactory-hook");
  
  gobject_class->set_property = gxk_gadget_factory_set_property;
  gobject_class->get_property = gxk_gadget_factory_get_property;
  gobject_class->finalize = gxk_gadget_factory_finalize;
  afactory_class->match_action_list = gadget_factory_match_action_list;
  g_object_class_install_property (gobject_class, PROP_NAME,
                                   g_param_spec_string ("name", NULL, NULL, NULL, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_ACTION_ROOT,
                                   g_param_spec_string ("action-root", NULL, NULL, ":xdef", G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_ACTION_LIST,
                                   g_param_spec_string ("action-list", NULL, NULL, NULL, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_PER_LIST,
                                   g_param_spec_string ("per-list", NULL, NULL, NULL, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_PER_BRANCH,
                                   g_param_spec_string ("per-branch", NULL, NULL, NULL, G_PARAM_READWRITE));
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
  self->action_root = g_strdup (":xdef");
}

static gchar*
value_dup_string_non_empty (const GValue *value)
{
  const gchar *str = g_value_get_string (value);
  return str && str[0] ? g_strdup (str) : NULL;
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
      self->name = value_dup_string_non_empty (value);
      break;
    case PROP_ACTION_ROOT:
      g_free (self->action_root);
      self->action_root = value_dup_string_non_empty (value);
      break;
    case PROP_ACTION_LIST:
      g_free (self->action_list);
      self->action_list = g_value_dup_string (value);
      break;
    case PROP_PER_LIST:
      g_free (self->per_list);
      self->per_list = value_dup_string_non_empty (value);
      break;
    case PROP_PER_BRANCH:
      g_free (self->per_branch);
      self->per_branch = value_dup_string_non_empty (value);
      break;
    case PROP_PER_ACTION:
      g_free (self->per_action);
      self->per_action = value_dup_string_non_empty (value);
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
    case PROP_ACTION_ROOT:
      g_value_set_string (value, self->action_root);
      break;
    case PROP_ACTION_LIST:
      g_value_set_string (value, self->action_list);
      break;
    case PROP_PER_LIST:
      g_value_set_string (value, self->per_list);
      break;
    case PROP_PER_BRANCH:
      g_value_set_string (value, self->per_branch);
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
  g_datalist_clear (&self->branch_widgets);
  g_free (self->action_root);
  g_free (self->per_action);
  g_free (self->per_list);
  g_free (self->name);
  g_free (self->action_list);
  g_free (self->activatable);
  g_free (self->regulate);
  gxk_gadget_free_args (self->call_args);
  while (self->branches)
    {
      GxkFactoryBranch *b = g_slist_pop_head (&self->branches);
      g_object_unref (b);
    }
  
  g_return_if_fail (self->timer == 0);
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (gadget_factory_parent_class)->finalize (object);
}

static GxkGadget*
gadget_factory_retrieve_branch (GxkGadgetFactory *self,
                                const gchar      *key_path,     /* uline_unescaped */
                                const gchar      *label_path,   /* translated, contains ulines */
                                GxkGadget        *parent,
                                const gchar      *path_prefix,
                                GxkGadgetArgs    *branch_args)
{
  GxkGadget *gadget = g_datalist_get_data (&self->branch_widgets, key_path);
  if (!gadget)
    {
      const gchar *key_leaf = gxk_factory_path_get_leaf (key_path);
      const gchar *label_leaf = gxk_factory_path_get_leaf (label_path);
      gchar *action_path = g_strconcat (path_prefix, key_path, NULL);
      gchar *unescaped = g_strcompress (label_leaf ? label_leaf : key_leaf);
      GSList *args_list = NULL;
      GxkGadgetArgs *action_args = gxk_gadget_args ("action-name", unescaped,
                                                    "action-path", action_path,
                                                    NULL);
      g_free (unescaped);
      g_free (action_path);
      if (key_leaf > key_path)
        {
          gchar *key_ppath = g_strndup (key_path, key_leaf - key_path - 1);
          gchar *label_ppath = label_leaf && label_leaf > label_path ? g_strndup (label_path, label_leaf - label_path - 1) : NULL;
          parent = gadget_factory_retrieve_branch (self, key_ppath, label_ppath, parent, path_prefix, branch_args);
          g_free (key_ppath);
          g_free (label_ppath);
        }
      if (self->call_args)
        args_list = g_slist_prepend (args_list, self->call_args);
      args_list = g_slist_prepend (args_list, action_args);
      if (branch_args)
        args_list = g_slist_prepend (args_list, branch_args);
      gadget = gxk_gadget_creator (NULL, gxk_gadget_get_domain (self),
                                   self->per_branch, parent, args_list, NULL);
      g_slist_free (args_list);
      gxk_gadget_free_args (action_args);
      if (parent == self->gadget)
        gxk_container_slot_reorder_child (GTK_CONTAINER (self->gadget), gadget, self->cslot);
      gadget = gxk_gadget_find_area (gadget, NULL);
      g_datalist_set_data (&self->branch_widgets, key_path, gadget);
    }
  return gadget;
}

static gboolean
match_action_root (GxkGadgetFactory *self,
                   GtkWidget        *publisher)
{
  GtkWidget *ancestor;
  if (!self->action_root)
    return TRUE;
  if (strcmp (self->action_root, ":xdef") == 0)
    ancestor = self->xdef_gadget;
  else
    ancestor = gxk_widget_find_level_ordered ((GtkWidget*) self->window, self->action_root);
  return ancestor && gxk_widget_has_ancestor (publisher, ancestor);
}

static inline const gchar*
strip_slashes (const gchar *string)
{
  while (string[0] == '/')
    string++;
  return string;
}

static void
gadget_factory_match_action_list (GxkActionFactory       *afactory,
                                  const gchar            *prefix,
                                  GxkActionList          *alist,
                                  GtkWidget              *publisher)
{
  GxkGadgetFactory *self = GXK_GADGET_FACTORY (afactory);
  guint n_actions = gxk_action_list_get_n_actions (alist);
  if (n_actions && self->action_list &&
      strcmp (self->action_list, prefix) == 0 &&
      match_action_root (self, publisher))
    {
      const gchar *domain = gxk_gadget_get_domain (self);
      const gchar *wname = GTK_WIDGET (self->window)->name;
      gchar *path_prefix = wname ? g_strdup_printf ("<%s>/%s/", wname, prefix) : NULL;
      GSList *slist;
      guint i;
      if (self->per_list)
        {
          GxkGadgetArgs *action_args = gxk_gadget_args ("action-path", path_prefix, NULL);
          GSList *args_list = NULL;
          GxkGadget *gadget;
          if (self->call_args)
            args_list = g_slist_prepend (args_list, self->call_args);
          args_list = g_slist_prepend (args_list, action_args);
          gadget = gxk_gadget_creator (NULL, domain, self->per_list, self->gadget, args_list, NULL);
          g_slist_free (args_list);
          gxk_gadget_free_args (action_args);
          gxk_container_slot_reorder_child (GTK_CONTAINER (self->gadget), gadget, self->cslot);
        }
      for (slist = self->branches; slist; slist = slist->next)
        {
          GxkFactoryBranch *branch = slist->data;
          if (branch->key_label)
            {
              gchar *key_path = gxk_factory_path_unescape_uline (branch->key_label);
              gadget_factory_retrieve_branch (self, strip_slashes (key_path),
                                              strip_slashes (branch->uline_label ? branch->uline_label : key_path),
                                              self->gadget, path_prefix, branch->branch_args);
              g_free (key_path);
            }
        }
      for (i = self->per_action ? 0 : G_MAXINT; i < n_actions; i++)
        {
          GxkGadget *gadget, *parent;
          GxkGadgetArgs *action_args;
          GSList *args_list = NULL;
          GxkAction action;
          gxk_action_list_get_action (alist, i, &action);
          gchar *key_path = gxk_factory_path_unescape_uline (strip_slashes (action.key));
          const gchar *label_path = strip_slashes (action.name);
          gchar *action_path = g_strconcat (path_prefix, key_path, NULL);
          const gchar *key_leaf = gxk_factory_path_get_leaf (key_path);
          const gchar *label_leaf = gxk_factory_path_get_leaf (label_path);
          gchar *unescaped = g_strcompress (label_leaf ? label_leaf : key_leaf);
          if (key_leaf > key_path && self->per_branch)
            {
              gchar *key_ppath = g_strndup (key_path, key_leaf - key_path - 1);
              gchar *label_ppath = label_leaf && label_leaf > label_path ? g_strndup (label_path, label_leaf - label_path - 1) : NULL;
              parent = gadget_factory_retrieve_branch (self, key_ppath, label_ppath, self->gadget, path_prefix, NULL);
              g_free (key_ppath);
              g_free (label_ppath);
            }
          else
            parent = self->gadget;
          action_args = gxk_gadget_args ("action-name", unescaped,
                                         "action-key", key_path,
                                         "action-path", action_path,
                                         "action-accel", action.accelerator,
                                         "action-tooltip", action.tooltip,
                                         "action-stock", action.stock_icon,
                                         NULL);
          g_free (key_path);
          g_free (action_path);
          g_free (unescaped);
          if (self->call_args)
            args_list = g_slist_prepend (args_list, self->call_args);
          args_list = g_slist_prepend (args_list, action_args);
          gadget = gxk_gadget_creator (NULL, domain, self->per_action, parent, args_list, NULL);
          g_slist_free (args_list);
          gxk_gadget_free_args (action_args);
          if (parent == self->gadget)
            gxk_container_slot_reorder_child (GTK_CONTAINER (self->gadget), gadget, self->cslot);
          if (GTK_IS_WIDGET (gadget))
            {
              GxkGadget *achild = self->activatable ? gxk_gadget_find (gadget, self->activatable) : NULL;
              GxkGadget *rchild = self->regulate ? gxk_gadget_find (gadget, self->regulate) : NULL;
              if (GTK_IS_WIDGET (achild))
                {
                  const gchar *signal = GTK_IS_BUTTON (achild) ? "clicked" : "activate"; /* work around buttons */
                  g_signal_connect_swapped (achild, signal,
                                            G_CALLBACK (gxk_action_activate_callback),
                                            (gpointer) action.action_data);
                }
              if (GTK_IS_WIDGET (rchild))
                {
                  gxk_action_list_regulate_widget (alist, i, rchild);
                  const gchar *signal = GTK_IS_BUTTON (rchild) ? "clicked" : "activate"; /* work around buttons */
                  if (g_signal_lookup (signal, G_OBJECT_TYPE (rchild)))
                    g_signal_connect_after (rchild, signal,
                                            G_CALLBACK (gxk_action_list_force_regulate), NULL);
                }
            }
        }
      g_free (path_prefix);
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
    self->timer = g_idle_add_full (GXK_ACTION_PRIORITY - 1,
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
gadget_factory_create (GType               type,
                       const gchar        *name,
                       GxkGadgetData      *gdgdata)
{
  return g_object_new (type, "name", name, NULL);
}

static GParamSpec*
gadget_factory_find_prop (GxkGadget    *gadget,
                          const gchar  *prop_name)
{
  return g_object_class_find_property (G_OBJECT_GET_CLASS (gadget), prop_name);
}

static gboolean
gadget_factory_adopt (GxkGadget          *gadget,
                      GxkGadget          *parent,
                      GxkGadgetData      *gdgdata)
{
  GxkGadgetFactory *self = gadget;
  self->call_args = gxk_gadget_data_copy_call_args (gdgdata);
  self->xdef_gadget = gxk_gadget_data_get_scope_gadget (gdgdata);
  gxk_gadget_factory_attach (gadget, GTK_WIDGET (parent));
  self->cslot = gxk_container_get_insertion_slot (GTK_CONTAINER (parent));
  return FALSE; /* no support for packing options */
}

static const GxkGadgetType gadget_factory_def = {
  gadget_factory_create,
  gadget_factory_find_prop,
  (void(*)(GxkGadget*,const gchar*,const GValue*)) g_object_set_property,
  gadget_factory_adopt,
  NULL, /* find_pack */
  NULL, /* set_pack */
};
const GxkGadgetType *_gxk_gadget_factory_def = &gadget_factory_def;


/* --- GxkFactoryBranch --- */
enum {
  FACTORY_BRANCH_PROP_0,
  FACTORY_BRANCH_PROP_ULINE_LABEL,
  FACTORY_BRANCH_PROP_KEY_LABEL
};
static gpointer factory_branch_parent_class = NULL;
static void
gxk_factory_branch_set_property (GObject      *object,
                                 guint         param_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GxkFactoryBranch *self = GXK_FACTORY_BRANCH (object);
  switch (param_id)
    {
    case FACTORY_BRANCH_PROP_ULINE_LABEL:
      g_free (self->uline_label);
      self->uline_label = g_value_dup_string (value);
      break;
    case FACTORY_BRANCH_PROP_KEY_LABEL:
      g_free (self->key_label);
      self->key_label = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
  if (!self->key_label && self->uline_label)
    self->key_label = g_strdup (self->uline_label);
}
static void
gxk_factory_branch_finalize (GObject *object)
{
  GxkFactoryBranch *self = GXK_FACTORY_BRANCH (object);
  g_free (self->uline_label);
  g_free (self->key_label);
  gxk_gadget_free_args (self->branch_args);
  /* chain parent class' handler */
  G_OBJECT_CLASS (factory_branch_parent_class)->finalize (object);
}
static void
gxk_factory_branch_class_init (GxkFactoryBranchClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  factory_branch_parent_class = g_type_class_peek_parent (class);
  gobject_class->set_property = gxk_factory_branch_set_property;
  gobject_class->finalize = gxk_factory_branch_finalize;
  g_object_class_install_property (gobject_class, FACTORY_BRANCH_PROP_ULINE_LABEL,
                                   g_param_spec_string ("uline-label", NULL, NULL, NULL, G_PARAM_WRITABLE));
  g_object_class_install_property (gobject_class, FACTORY_BRANCH_PROP_KEY_LABEL,
                                   g_param_spec_string ("key-label", NULL, NULL, NULL, G_PARAM_WRITABLE));
}
GType
gxk_factory_branch_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static const GTypeInfo type_info = {
        sizeof (GxkFactoryBranchClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gxk_factory_branch_class_init,
        NULL,   /* class_finalize */
        NULL,   /* class_data */
        sizeof (GxkFactoryBranch),
        0,      /* n_preallocs */
        (GInstanceInitFunc) NULL,
      };
      type = g_type_register_static (G_TYPE_OBJECT, "GxkFactoryBranch", &type_info, 0);
    }
  return type;
}
/* --- GxkFactoryBranch gadget type hooks --- */
static GxkGadget*
factory_branch_create (GType               type,
                       const gchar        *name,
                       GxkGadgetData      *gdgdata)
{
  return g_object_new (type, NULL);
}
static GParamSpec*
factory_branch_find_prop (GxkGadget    *gadget,
                          const gchar  *prop_name)
{
  return g_object_class_find_property (G_OBJECT_GET_CLASS (gadget), prop_name);
}
static gboolean
factory_branch_adopt (GxkGadget          *gadget,
                      GxkGadget          *parent,
                      GxkGadgetData      *gdgdata)
{
  GxkFactoryBranch *self = GXK_FACTORY_BRANCH (gadget);
  GxkGadgetFactory *factory = GXK_GADGET_FACTORY (parent);
  GxkGadgetArgs *args = gxk_gadget_data_copy_call_args (gdgdata);
  self->branch_args = gxk_gadget_args_merge (self->branch_args, args);
  gxk_gadget_free_args (args);
  factory->branches = g_slist_append (factory->branches, gadget);
  return FALSE; /* no support for packing args */
}
static const GxkGadgetType factory_branch_def = {
  factory_branch_create,
  factory_branch_find_prop,
  (void(*)(GxkGadget*,const gchar*,const GValue*)) g_object_set_property,
  factory_branch_adopt,
  NULL, /* find_pack */
  NULL, /* set_pack */
};
const GxkGadgetType *_gxk_factory_branch_def = &factory_branch_def;
