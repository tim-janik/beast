// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "gxkradgetfactory.hh"
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

/* --- static variables --- */
static GQuark   quark_radget_factory_hook = 0;


/* --- functions --- */
G_DEFINE_TYPE (GxkRadgetFactory, gxk_radget_factory, G_TYPE_OBJECT);

static void
gxk_radget_factory_init (GxkRadgetFactory *self)
{
  self->window = NULL;
  self->radget = NULL;
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
gxk_radget_factory_set_property (GObject      *object,
                                 guint         param_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GxkRadgetFactory *self = GXK_RADGET_FACTORY (object);
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
gxk_radget_factory_get_property (GObject    *object,
                                 guint       param_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GxkRadgetFactory *self = GXK_RADGET_FACTORY (object);
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
gxk_radget_factory_finalize (GObject *object)
{
  GxkRadgetFactory *self = GXK_RADGET_FACTORY (object);

  g_assert (self->window == NULL);
  g_datalist_clear (&self->branch_widgets);
  g_free (self->action_root);
  g_free (self->per_action);
  g_free (self->per_list);
  g_free (self->name);
  g_free (self->action_list);
  g_free (self->activatable);
  g_free (self->regulate);
  gxk_radget_free_args (self->call_args);
  while (self->branches)
    {
      GxkFactoryBranch *b = (GxkFactoryBranch*) g_slist_pop_head (&self->branches);
      g_object_unref (b);
    }

  g_return_if_fail (self->timer == 0);

  /* chain parent class' handler */
  G_OBJECT_CLASS (gxk_radget_factory_parent_class)->finalize (object);
}

static GxkRadget*
radget_factory_retrieve_branch (GxkRadgetFactory *self,
                                const gchar      *key_path,     /* uline_unescaped */
                                const gchar      *label_path,   /* translated, contains ulines */
                                GxkRadget        *parent,
                                const gchar      *path_prefix,
                                GxkRadgetArgs    *branch_args)
{
  GxkRadget *radget = g_datalist_get_data (&self->branch_widgets, key_path);
  if (!radget)
    {
      const gchar *key_leaf = gxk_factory_path_get_leaf (key_path);
      const gchar *label_leaf = gxk_factory_path_get_leaf (label_path);
      gchar *action_path = g_strconcat (path_prefix, key_path, NULL);
      gchar *unescaped = g_strcompress (label_leaf ? label_leaf : key_leaf);
      GSList *args_list = NULL;
      GxkRadgetArgs *action_args = gxk_radget_args ("action-name", unescaped,
                                                    "action-path", action_path,
                                                    NULL);
      g_free (unescaped);
      g_free (action_path);
      if (key_leaf > key_path)
        {
          gchar *key_ppath = g_strndup (key_path, key_leaf - key_path - 1);
          gchar *label_ppath = label_leaf && label_leaf > label_path ? g_strndup (label_path, label_leaf - label_path - 1) : NULL;
          parent = radget_factory_retrieve_branch (self, key_ppath, label_ppath, parent, path_prefix, branch_args);
          g_free (key_ppath);
          g_free (label_ppath);
        }
      if (self->call_args)
        args_list = g_slist_prepend (args_list, self->call_args);
      args_list = g_slist_prepend (args_list, action_args);
      if (branch_args)
        args_list = g_slist_prepend (args_list, branch_args);
      radget = gxk_radget_creator (NULL, gxk_radget_get_domain (self),
                                   self->per_branch, parent, args_list, NULL);
      g_slist_free (args_list);
      gxk_radget_free_args (action_args);
      if (parent == self->radget)
        gxk_container_slot_reorder_child (GTK_CONTAINER (self->radget), (GtkWidget*) radget, self->cslot);
      radget = gxk_radget_find_area (radget, NULL);
      g_datalist_set_data (&self->branch_widgets, key_path, radget);
    }
  return radget;
}

static gboolean
match_action_root (GxkRadgetFactory *self,
                   GtkWidget        *publisher)
{
  if (!self->action_root)
    return TRUE;
#if 0
  // FIXME: is requirering publishing by a factory ancestor better than ":xdef" constraint?
  GtkWidget *widget = self->radget;
  while (widget)
    {
      if (widget == publisher)
        return TRUE;
      if (GTK_IS_MENU (widget))
        widget = gtk_menu_get_attach_widget ((GtkMenu*) widget);
      else
        widget = widget->parent;
    }
#endif
  GtkWidget *ancestor;
  if (strcmp (self->action_root, ":xdef") == 0)
    ancestor = (GtkWidget*) self->xdef_radget;
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
radget_factory_action_list_added (gpointer                client_data,
                                  GtkWindow              *window,
                                  const gchar            *prefix,
                                  GxkActionList          *alist,
                                  GtkWidget              *publisher)
{
  GxkRadgetFactory *self = GXK_RADGET_FACTORY (client_data);
  guint n_actions = gxk_action_list_get_n_actions (alist);
  if (n_actions && self->action_list &&
      strcmp (self->action_list, prefix) == 0 &&
      match_action_root (self, publisher))
    {
      const gchar *domain = gxk_radget_get_domain (self);
      const gchar *wname = GTK_WIDGET (self->window)->name;
      gchar *path_prefix = wname ? g_strdup_printf ("<%s>/%s/", wname, prefix) : NULL;
      GSList *slist;
      guint i;
      if (self->per_list)
        {
          GxkRadgetArgs *action_args = gxk_radget_args ("action-path", path_prefix, NULL);
          GSList *args_list = NULL;
          GxkRadget *radget;
          if (self->call_args)
            args_list = g_slist_prepend (args_list, self->call_args);
          args_list = g_slist_prepend (args_list, action_args);
          radget = gxk_radget_creator (NULL, domain, self->per_list, self->radget, args_list, NULL);
          g_slist_free (args_list);
          gxk_radget_free_args (action_args);
          gxk_container_slot_reorder_child (GTK_CONTAINER (self->radget), (GtkWidget*) radget, self->cslot);
        }
      for (slist = self->branches; slist; slist = slist->next)
        {
          GxkFactoryBranch *branch = (GxkFactoryBranch*) slist->data;
          if (branch->key_label)
            {
              gchar *key_path = gxk_factory_path_unescape_uline (branch->key_label);
              radget_factory_retrieve_branch (self, strip_slashes (key_path),
                                              strip_slashes (branch->uline_label ? branch->uline_label : key_path),
                                              self->radget, path_prefix, branch->branch_args);
              g_free (key_path);
            }
        }
      for (i = self->per_action ? 0 : G_MAXINT; i < n_actions; i++)
        {
          GxkRadget *radget, *parent;
          GxkRadgetArgs *action_args;
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
              parent = radget_factory_retrieve_branch (self, key_ppath, label_ppath, self->radget, path_prefix, NULL);
              g_free (key_ppath);
              g_free (label_ppath);
            }
          else
            parent = self->radget;
          action_args = gxk_radget_args ("action-name", unescaped,
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
          radget = gxk_radget_creator (NULL, domain, self->per_action, parent, args_list, NULL);
          g_slist_free (args_list);
          gxk_radget_free_args (action_args);
          if (parent == self->radget)
            gxk_container_slot_reorder_child (GTK_CONTAINER (self->radget), (GtkWidget*) radget, self->cslot);
          if (GTK_IS_WIDGET (radget))
            {
              GxkRadget *achild = self->activatable ? gxk_radget_find (radget, self->activatable) : NULL;
              GxkRadget *rchild = self->regulate ? gxk_radget_find (radget, self->regulate) : NULL;
              if (GTK_IS_WIDGET (achild))
                {
                  const gchar *signal = GTK_IS_BUTTON (achild) ? "clicked" : "activate"; /* work around buttons */
                  g_signal_connect_swapped (achild, signal,
                                            G_CALLBACK (gxk_action_activate_callback),
                                            (gpointer) action.action_data);
                }
              if (GTK_IS_WIDGET (rchild))
                {
                  gxk_action_list_regulate_widget (alist, i, (GtkWidget*) rchild);
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
radget_factory_check_anchored (gpointer data)
{
  GxkRadgetFactory *self = GXK_RADGET_FACTORY (data);
  GtkWidget *toplevel;
  gboolean is_window;
  GDK_THREADS_ENTER ();
  toplevel = self->radget ? gxk_widget_get_attach_toplevel ((GtkWidget*) self->radget) : NULL;
  is_window = GTK_IS_WINDOW (toplevel);
  if (is_window && !self->window)
    {
      self->window = (GtkWindow*) g_object_ref (toplevel);
      gxk_window_add_action_client (self->window, radget_factory_action_list_added, self);
    }
  else if (self->window && !is_window)
    {
      gxk_window_remove_action_client (self->window, self);
      g_object_unref (self->window);
      self->window = NULL;
    }
  self->timer = 0;
  GDK_THREADS_LEAVE ();
  return FALSE;
}

void
gxk_radget_factory_check_anchored (GxkRadgetFactory *self)
{
  if (self->radget && !self->timer)
    self->timer = g_idle_add_full (GXK_ACTION_PRIORITY - 1,
                                   radget_factory_check_anchored,
                                   self, NULL);
}

static void
destroy_factory_slist (gpointer data)
{
  GSList *slist = (GSList*) data;
  while (slist)
    {
      GxkRadgetFactory *fact = (GxkRadgetFactory*) g_slist_pop_head (&slist);
      g_object_run_dispose (G_OBJECT (fact));
      fact->radget = NULL;
      g_object_unref (fact);
    }
}

void
gxk_radget_factory_attach (GxkRadgetFactory *self,
                           GxkRadget        *radget)
{
  GSList *slist;

  g_return_if_fail (self->radget == NULL);
  g_return_if_fail (GTK_IS_WIDGET (radget));

  g_object_ref (self);
  slist = (GSList*) g_object_steal_qdata ((GObject*) radget, quark_radget_factory_hook);
  slist = g_slist_prepend (slist, self);
  g_object_set_qdata_full ((GObject*) radget, quark_radget_factory_hook, slist, destroy_factory_slist);
  self->radget = radget;
  g_signal_connect_object (radget, "hierarchy-changed",
                           G_CALLBACK (gxk_radget_factory_check_anchored), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (radget, "attached-hierarchy-changed",
                           G_CALLBACK (gxk_radget_factory_check_anchored), self, G_CONNECT_SWAPPED);
  gxk_radget_factory_check_anchored (self);
}

static void
gxk_radget_factory_class_init (GxkRadgetFactoryClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  quark_radget_factory_hook = g_quark_from_static_string ("GxkRadgetFactory-hook");

  gobject_class->set_property = gxk_radget_factory_set_property;
  gobject_class->get_property = gxk_radget_factory_get_property;
  gobject_class->finalize = gxk_radget_factory_finalize;

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


/* --- radget type hooks --- */
static GParamSpec*
radget_factory_find_prop (GTypeClass  *klass,
                          const gchar *prop_name)
{
  return g_object_class_find_property (G_OBJECT_CLASS (klass), prop_name);
}

static GxkRadget*
radget_factory_create (GType               type,
                       const gchar        *name,
                       guint               n_construct_params,
                       GParameter         *construct_params,
                       GxkRadgetData      *gdgdata)
{
  GtkWidget *widget = (GtkWidget*) g_object_newv (type, n_construct_params, construct_params);
  g_object_set (widget, "name", name, NULL);
  return widget;
}

static gboolean
radget_factory_adopt (GxkRadget          *radget,
                      GxkRadget          *parent,
                      GxkRadgetData      *gdgdata)
{
  GxkRadgetFactory *self = (GxkRadgetFactory*) radget;
  self->call_args = gxk_radget_data_copy_call_args (gdgdata);
  self->xdef_radget = gxk_radget_data_get_scope_radget (gdgdata);
  gxk_radget_factory_attach ((GxkRadgetFactory*) radget, GTK_WIDGET (parent));
  self->cslot = gxk_container_get_insertion_slot (GTK_CONTAINER (parent));
  return FALSE; /* no support for packing options */
}

static const GxkRadgetType radget_factory_def = {
  radget_factory_find_prop,
  radget_factory_create,
  (void(*)(GxkRadget*,const gchar*,const GValue*)) g_object_set_property,
  radget_factory_adopt,
  NULL, /* find_pack */
  NULL, /* set_pack */
};
const GxkRadgetType *gxk_radget_factory_def = &radget_factory_def;


/* --- GxkFactoryBranch --- */
G_DEFINE_TYPE (GxkFactoryBranch, gxk_factory_branch, G_TYPE_OBJECT);

enum {
  FACTORY_BRANCH_PROP_0,
  FACTORY_BRANCH_PROP_ULINE_LABEL,
  FACTORY_BRANCH_PROP_KEY_LABEL
};

static void
gxk_factory_branch_init (GxkFactoryBranch *self)
{
}

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
  gxk_radget_free_args (self->branch_args);
  /* chain parent class' handler */
  G_OBJECT_CLASS (gxk_factory_branch_parent_class)->finalize (object);
}

static void
gxk_factory_branch_class_init (GxkFactoryBranchClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = gxk_factory_branch_set_property;
  gobject_class->finalize = gxk_factory_branch_finalize;

  g_object_class_install_property (gobject_class, FACTORY_BRANCH_PROP_ULINE_LABEL,
                                   g_param_spec_string ("uline-label", NULL, NULL, NULL, G_PARAM_WRITABLE));
  g_object_class_install_property (gobject_class, FACTORY_BRANCH_PROP_KEY_LABEL,
                                   g_param_spec_string ("key-label", NULL, NULL, NULL, G_PARAM_WRITABLE));
}

/* --- GxkFactoryBranch radget type hooks --- */
static GParamSpec*
factory_branch_find_prop (GTypeClass  *klass,
                          const gchar *prop_name)
{
  return g_object_class_find_property (G_OBJECT_CLASS (klass), prop_name);
}

static GxkRadget*
factory_branch_create (GType               type,
                       const gchar        *name,
                       guint               n_construct_params,
                       GParameter         *construct_params,
                       GxkRadgetData      *gdgdata)
{
  return g_object_newv (type, n_construct_params, construct_params);
}

static gboolean
factory_branch_adopt (GxkRadget          *radget,
                      GxkRadget          *parent,
                      GxkRadgetData      *gdgdata)
{
  GxkFactoryBranch *self = GXK_FACTORY_BRANCH (radget);
  GxkRadgetFactory *factory = GXK_RADGET_FACTORY (parent);
  GxkRadgetArgs *args = gxk_radget_data_copy_call_args (gdgdata);
  self->branch_args = gxk_radget_args_merge (self->branch_args, args);
  gxk_radget_free_args (args);
  factory->branches = g_slist_append (factory->branches, radget);
  return FALSE; /* no support for packing args */
}

static const GxkRadgetType factory_branch_def = {
  factory_branch_find_prop,
  factory_branch_create,
  (void(*)(GxkRadget*,const gchar*,const GValue*)) g_object_set_property,
  factory_branch_adopt,
  NULL, /* find_pack */
  NULL, /* set_pack */
};
const GxkRadgetType *gxk_factory_branch_def = &factory_branch_def;
