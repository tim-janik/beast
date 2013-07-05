// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "gxkaction.hh"
#include <string.h>
#include <libintl.h>


#define intern_null_string(s,sconst)      (s ? (sconst ? g_intern_static_string : g_intern_string) (s) : NULL)
#define intern_i18n_string(idom,s,sconst) (intern_null_string (idom && s ? dgettext (idom, s) : s, sconst))

/* --- caching GxkActionCheck() --- */
#define ACTION_CHECK( func, user_data, action_id)         (!func || action_check_stamped (func, user_data, action_id, TRUE))
#define ACTION_CHECK_CACHED( func, user_data, action_id)  (!func || action_check_stamped (func, user_data, action_id, FALSE))

static guint64 global_action_cache_stamp = 0x100000000LL;

guint64
gxk_action_inc_cache_stamp (void)
{
  do
    global_action_cache_stamp += 1;
  while ((global_action_cache_stamp & 0xffffffff) == 0);
  return global_action_cache_stamp;
}

static inline gboolean
action_check_stamped (GxkActionCheck acheck_func,
                      gpointer       user_data,
                      gulong         action_id,
                      gboolean       alter_stamp)
{
  if (alter_stamp)
    gxk_action_inc_cache_stamp();
  return acheck_func (user_data, action_id, global_action_cache_stamp) != FALSE;
}

/* --- action class ---- */
typedef struct {
  GxkActionCheck  acheck;
  GxkActionExec   aexec;
  gpointer        user_data;
  GxkActionGroup *agroup;
  guint           class_flags;
  /* non-key fields */
  guint           ref_count;
} ActionClass;
static GHashTable *action_class_ht = NULL;

static gboolean
action_class_equals (gconstpointer v1,
                     gconstpointer v2)
{
  const ActionClass *c1 = (const ActionClass*) v1;
  const ActionClass *c2 = (const ActionClass*) v2;
  return (c1->acheck      == c2->acheck &&
          c1->aexec       == c2->aexec &&
          c1->user_data   == c2->user_data &&
          c1->agroup      == c2->agroup &&
          c1->class_flags == c2->class_flags);
}

static guint
action_class_hash (gconstpointer v)
{
  const ActionClass *c = (const ActionClass*) v;
  return (G_HASH_POINTER (c->acheck) ^
          G_HASH_POINTER (c->aexec) ^
          G_HASH_POINTER (c->user_data) ^
          G_HASH_POINTER (c->agroup) ^
          c->class_flags);
}

static ActionClass*
action_class_ref_new (GxkActionCheck  acheck,
                      GxkActionExec   aexec,
                      gpointer        user_data,
                      GxkActionGroup *agroup,
                      guint           class_flags)
{
  ActionClass key, *c;
  key.acheck = acheck;
  key.aexec = aexec;
  key.user_data = user_data;
  key.agroup = agroup;
  key.class_flags = class_flags;
  c = (ActionClass*) g_hash_table_lookup (action_class_ht, &key);
  if (!c)
    {
      key.agroup = agroup ? (GxkActionGroup*) g_object_ref (agroup) : NULL;
      key.ref_count = 1;
      c = (ActionClass*) g_memdup (&key, sizeof (key));
      g_hash_table_insert (action_class_ht, c, c);
    }
  else
    c->ref_count++;
  return c;
}

static ActionClass*
action_class_ref (ActionClass *aclass)
{
  g_return_val_if_fail (aclass->ref_count > 0, NULL);
  aclass->ref_count++;
  return aclass;
}

static void
action_class_unref (ActionClass *aclass)
{
  g_return_if_fail (aclass->ref_count > 0);
  aclass->ref_count--;
  if (!aclass->ref_count)
    {
      g_hash_table_remove (action_class_ht, aclass);
      if (aclass->agroup)
        g_object_unref (aclass->agroup);
      g_free (aclass);
    }
}


/* --- action list --- */
typedef struct {
  ActionClass   *klass;
  const gchar   *key;
  GxkStockAction action;
  GSList        *widgets;
} ActionEntry;
static GQuark quark_action_entry = 0;

struct GxkActionList {
  guint           n_entries;
  ActionEntry   **entries;
  GxkActionGroup *agroup;
};

static void
action_list_add (GxkActionList        *alist,
                 ActionClass          *klass,
                 gboolean              sconst,
                 const gchar          *key,
                 const GxkStockAction *action,
                 const gchar          *i18n_domain)
{
  ActionEntry *entry = g_new0 (ActionEntry, 1);
  entry->klass = action_class_ref (klass);
  entry->key = g_intern_string (key);
  entry->action = *action;
  entry->action.name = intern_i18n_string (i18n_domain, action->name, sconst);
  entry->action.accelerator = intern_null_string (action->accelerator, sconst);
  entry->action.tooltip = intern_i18n_string (i18n_domain, action->tooltip, sconst);
  entry->action.stock_icon = intern_null_string (action->stock_icon, sconst);
  entry->widgets = NULL;
  guint j = alist->n_entries++;
  alist->entries = g_renew (ActionEntry*, alist->entries, alist->n_entries);
  alist->entries[j] = entry;
}

static void
action_entry_free (ActionEntry *entry)
{
  while (entry->widgets)
    {
      GtkWidget *widget = (GtkWidget*) g_slist_pop_head (&entry->widgets);
      g_object_set_qdata ((GObject*) widget, quark_action_entry, NULL);
      g_object_unref (widget);
    }
  action_class_unref (entry->klass);
  g_free (entry);
}

static ActionEntry*
action_entry_copy (const ActionEntry *source)
{
  ActionEntry *entry = (ActionEntry*) g_memdup (source, sizeof (source[0]));
  action_class_ref (entry->klass);
  entry->widgets = NULL;
  return entry;
}

GxkActionList*
gxk_action_list_create (void)
{
  return gxk_action_list_create_grouped (NULL);
}

GxkActionList*
gxk_action_list_create_grouped (GxkActionGroup *agroup)
{
  GxkActionList *alist = g_new0 (GxkActionList, 1);
  alist->agroup = agroup ? (GxkActionGroup*) g_object_ref (agroup) : NULL;
  return alist;
}

void
gxk_action_list_add_actions (GxkActionList        *alist,
                             guint                 n_actions,
                             const GxkStockAction *actions,
                             const gchar          *i18n_domain,
                             GxkActionCheck        acheck,
                             GxkActionExec         aexec,
                             gpointer              user_data)
{
  const gchar *idomain = i18n_domain ? i18n_domain : textdomain (NULL);
  ActionClass *klass = action_class_ref_new (acheck, aexec, user_data, alist->agroup, 0);
  guint i;
  for (i = 0; i < n_actions; i++)
    {
      g_return_if_fail (actions[i].name != NULL);
      action_list_add (alist, klass, TRUE, actions[i].name, actions + i, idomain);
    }
  action_class_unref (klass);
}

void
gxk_action_list_add_translated (GxkActionList          *alist,
                                const gchar            *key,
                                const gchar            *name,          /* translated (key) */
                                const gchar            *accelerator,
                                const gchar            *tooltip,       /* translated */
                                gulong                  action_id,
                                const gchar            *stock_icon,
                                GxkActionCheck          acheck,
                                GxkActionExec           aexec,
                                gpointer                user_data)
{
  ActionClass *klass = action_class_ref_new (acheck, aexec, user_data, alist->agroup, 0);
  GxkStockAction a = { 0, };
  g_return_if_fail (name);
  if (!key)
    key = name;
  a.name = name;
  a.accelerator = accelerator;
  a.tooltip = tooltip;
  a.action_id = action_id;
  a.stock_icon = stock_icon;
  action_list_add (alist, klass, FALSE, key, &a, NULL);
  action_class_unref (klass);
}

static gint
action_entries_compare (gconstpointer v1,
                        gconstpointer v2,
                        gpointer      user_data)
{
  const ActionEntry *const*p1 = (const ActionEntry*const*) v1, *e1 = *p1;
  const ActionEntry *const*p2 = (const ActionEntry*const*) v2, *e2 = *p2;
  return strcmp (e1->action.name, e2->action.name);
}

GxkActionList*
gxk_action_list_sort (GxkActionList *alist)
{
  g_qsort_with_data (alist->entries, alist->n_entries, sizeof (alist->entries[0]), action_entries_compare, NULL);
  return alist;
}

GxkActionList*
gxk_action_list_merge (GxkActionList *alist1,
                       GxkActionList *alist2)
{
  guint j = alist1->n_entries;
  alist1->n_entries += alist2->n_entries;
  alist1->entries = g_renew (ActionEntry*, alist1->entries, alist1->n_entries);
  memcpy (alist1->entries + j, alist2->entries, sizeof (alist2->entries[0]) * alist2->n_entries);
  if (alist1->agroup != alist2->agroup && alist1->agroup)
    {
      g_object_unref (alist1->agroup);
      alist1->agroup = NULL;
    }
  g_free (alist2->entries);
  if (alist2->agroup)
    g_object_unref (alist2->agroup);
  g_free (alist2);
  return alist1;
}

GxkActionList*
gxk_action_list_copy (GxkActionList *alist)
{
  GxkActionList *al = gxk_action_list_create_grouped (alist->agroup);
  guint i;
  al->n_entries = alist->n_entries;
  al->entries = g_renew (ActionEntry*, al->entries, al->n_entries);
  for (i = 0; i < al->n_entries; i++)
    al->entries[i] = action_entry_copy (alist->entries[i]);
  return al;
}

guint
gxk_action_list_get_n_actions (GxkActionList *alist)
{
  return alist->n_entries;
}

void
gxk_action_list_get_action (GxkActionList          *alist,
                            guint                   nth,
                            GxkAction              *action)
{
  ActionEntry *e;
  g_return_if_fail (nth < alist->n_entries);
  e = alist->entries[nth];
  action->key = e->key;
  action->action_data = e;
  action->name = e->action.name;
  action->accelerator = e->action.accelerator;
  action->tooltip = e->action.tooltip;
  action->action_id = e->action.action_id;
  action->stock_icon = e->action.stock_icon;
}

static void
widget_set_active (GxkActionGroup *agroup,
                   GtkWidget      *widget)
{
  ActionEntry *e = (ActionEntry*) g_object_get_qdata ((GObject*) widget, quark_action_entry);
  if (e)
    {
      gxk_action_group_lock (agroup);
      gxk_widget_regulate (widget,
                           GTK_WIDGET_SENSITIVE (widget), /* preserves sensitivity */
                           agroup->action_id == e->action.action_id);
      gxk_action_group_unlock (agroup);
    }
}

void
gxk_action_list_regulate_widget (GxkActionList          *alist,
                                 guint                   nth,
                                 GtkWidget              *widget)
{
  ActionEntry *e;
  g_return_if_fail (nth < alist->n_entries);
  e = alist->entries[nth];
  e->widgets = g_slist_prepend (e->widgets, g_object_ref (widget));
  g_object_set_qdata ((GObject*) widget, quark_action_entry, e);
  if (e->klass->agroup && gxk_widget_regulate_uses_active (widget))
    g_signal_connect_object (e->klass->agroup, "changed", G_CALLBACK (widget_set_active), widget, GConnectFlags (0));
}

void
gxk_action_list_force_regulate (GtkWidget *widget)
{
  ActionEntry *e = (ActionEntry*) g_object_get_qdata ((GObject*) widget, quark_action_entry);
  if (e)
    {
      gboolean sensitive = ACTION_CHECK (e->klass->acheck, e->klass->user_data, e->action.action_id);
      gboolean active = e->klass->agroup && e->klass->agroup->action_id == e->action.action_id;
      if (e->klass->agroup)
        gxk_action_group_lock (e->klass->agroup);
      gxk_widget_regulate (widget, sensitive, active);
      if (e->klass->agroup)
        gxk_action_group_unlock (e->klass->agroup);
    }
}

void
gxk_action_list_free (GxkActionList *alist)
{
  guint i;
  for (i = 0; i < alist->n_entries; i++)
    action_entry_free (alist->entries[i]);
  g_free (alist->entries);
  if (alist->agroup)
    g_object_unref (alist->agroup);
  g_free (alist);
}

void
gxk_action_activate_callback (gconstpointer action_data)
{
  const ActionEntry *e = (const ActionEntry*) action_data;
  g_return_if_fail (e && e->klass && e->klass->ref_count > 0);
  if (!e->klass->agroup || e->klass->agroup->lock_count == 0)
    {
      if (ACTION_CHECK (e->klass->acheck, e->klass->user_data, e->action.action_id))
        {
          if (e->klass->agroup)
            gxk_action_group_select (e->klass->agroup, e->action.action_id);
          if (e->klass->aexec)
            e->klass->aexec (e->klass->user_data, e->action.action_id);
        }
      else
        {
          gboolean sensitive = FALSE;
          gboolean active = e->klass->agroup && e->klass->agroup->action_id == e->action.action_id;
          GSList *wnode;
          if (e->klass->agroup)
            gxk_action_group_lock (e->klass->agroup);
          for (wnode = e->widgets; wnode; wnode = wnode->next)
            gxk_widget_regulate ((GtkWidget*) wnode->data, sensitive, active);
          if (e->klass->agroup)
            gxk_action_group_unlock (e->klass->agroup);
        }
    }
}


/* --- GtkWindow actions --- */
typedef struct {
  gpointer            client_data;
  GxkActionClient     added_func;
} ActionClient;
static GQuark quark_action_clients = 0;

typedef struct ActionLink ActionLink;
struct ActionLink {
  guint          ref_count;
  gchar         *prefix;
  GtkWidget     *widget;
  GxkActionList *alist;
  ActionLink    *next;
  GtkWidget     *toplevel;
};
static GQuark quark_action_links = 0;
static GQuark quark_action_factories = 0;

static void
action_link_unref (ActionLink *alink)
{
  g_return_if_fail (alink->ref_count > 0);
  alink->ref_count--;
  if (!alink->ref_count)
    {
      gxk_action_list_free (alink->alist);
      g_free (alink->prefix);
      g_free (alink);
    }
}

static void
window_destroy_action_links (gpointer data)
{
  ActionLink *anode = (ActionLink*) data;
  while (anode)
    {
      ActionLink *alink = anode;
      anode = anode->next;
      alink->toplevel = NULL;
      alink->next = NULL;
      action_link_unref (alink);
    }
}

static void
window_add_action_link (GtkWidget *window,
                        ActionLink *alink)
{
  g_return_if_fail (GTK_IS_WIDGET (alink->widget));
  g_return_if_fail (alink->toplevel == NULL);
  alink->next = (ActionLink*) g_object_steal_qdata ((GObject*) window, quark_action_links);
  alink->toplevel = window;
  alink->ref_count++;
  g_object_set_qdata_full ((GObject*) window, quark_action_links, alink, window_destroy_action_links);
  GSList *slist = (GSList*) g_object_get_qdata ((GObject*) window, quark_action_clients);
  while (slist)
    {
      ActionClient *aclient = (ActionClient*) slist->data;
      slist = slist->next;
      if (aclient->added_func)
        aclient->added_func (aclient->client_data, (GtkWindow*) window, alink->prefix, alink->alist, alink->widget);
    }
}

static void
window_remove_action_link (ActionLink *alink)
{
  GtkWidget *window = alink->toplevel;
  g_return_if_fail (GTK_IS_WIDGET (alink->toplevel));

  ActionLink *last = NULL, *anode = (ActionLink*) g_object_get_qdata ((GObject*) window, quark_action_links);
  for (; anode; last = anode, anode = last->next)
    if (anode == alink)
      {
        if (last)
          last->next = anode->next;
        else
          {
            g_object_steal_qdata ((GObject*) window, quark_action_links);
            g_object_set_qdata_full ((GObject*) window, quark_action_links, anode->next, window_destroy_action_links);
          }
        alink->toplevel = NULL;
        alink->next = NULL;
        action_link_unref (alink);
        return;
      }
}

static GSList *window_queue = NULL;
static GQuark  quark_widgets_upwards = 0;
static GQuark  quark_widgets_downwards = 0;

static void
free_widget_slist (gpointer data)
{
  GSList *slist = (GSList*) data;
  while (slist)
    {
      GtkWidget *widget = (GtkWidget*) g_slist_pop_head (&slist);
      g_object_unref (widget);
    }
}

static inline gboolean
check_ancestor (GtkWidget *widget,
                GtkWidget *ancestor)
{
  while (widget)
    {
      if (widget == ancestor)
        return TRUE;
      widget = widget->parent;
    }
  return FALSE;
}

static gboolean
gxk_action_timer_update_window (gpointer data)
{
  GDK_THREADS_ENTER ();
  gxk_action_inc_cache_stamp();
  while (window_queue)
    {
      GtkWidget *window = (GtkWidget*) g_slist_pop_head (&window_queue);
      ActionLink *anode = (ActionLink*) g_object_get_qdata ((GObject*) window, quark_action_links);
      GSList *upwards = (GSList*) g_object_steal_qdata ((GObject*) window, quark_widgets_upwards);
      GSList *downwards = (GSList*) g_object_steal_qdata ((GObject*) window, quark_widgets_downwards);
      GtkWidget *last = NULL;
      gboolean needs_update = FALSE;
      for (; anode; anode = anode->next)
        {
          ActionLink *alink = anode;
          GxkActionList *alist = alink ? alink->alist : NULL;
          guint i;
          if (!alink)
            continue;
          if (last != alink->widget) /* minor caching */
            {
              GSList *wnode;
              last = alink->widget;
              needs_update = FALSE;
              for (wnode = upwards; wnode; wnode = wnode->next)
                if (check_ancestor ((GtkWidget*) wnode->data, alink->widget))
                  {
                    needs_update = TRUE;
                    break;
                  }
              for (wnode = needs_update ? NULL : downwards; wnode; wnode = wnode->next)
                if (check_ancestor (alink->widget, (GtkWidget*) wnode->data))
                  {
                    needs_update = TRUE;
                    break;
                  }
            }
          for (i = 0; i < alist->n_entries; i++)
            {
              ActionEntry *e = alist->entries[i];
              gboolean sensitive, active;
              GSList *wnode;
              if (!e->widgets)
                continue;
              sensitive = ACTION_CHECK_CACHED (e->klass->acheck, e->klass->user_data, e->action.action_id);
              active = e->klass->agroup && e->klass->agroup->action_id == e->action.action_id;
              if (e->klass->agroup)
                gxk_action_group_lock (e->klass->agroup);
              for (wnode = e->widgets; wnode; wnode = wnode->next)
                gxk_widget_regulate ((GtkWidget*) wnode->data, sensitive, active);
              if (e->klass->agroup)
                gxk_action_group_unlock (e->klass->agroup);
            }
        }
      free_widget_slist (upwards);
      free_widget_slist (downwards);
      g_object_unref (window);
    }
  GDK_THREADS_LEAVE ();
  return FALSE;
}

static void
window_queue_action_updates (GtkWidget *window,
                             GtkWidget *upwards,
                             GtkWidget *downwards)
{
  gboolean need_timer = FALSE;
  if (((GObject*) window)->ref_count && upwards && ((GObject*) upwards)->ref_count)
    {
      GSList *slist = (GSList*) g_object_get_qdata ((GObject*) window, quark_widgets_upwards);
      if (!g_slist_find (slist, upwards))
        {
          if (!slist)
            g_object_set_qdata_full ((GObject*) window, quark_widgets_upwards,
                                     g_slist_prepend (NULL, g_object_ref (upwards)),
                                     free_widget_slist);
          else
            slist->next = g_slist_prepend (slist->next, g_object_ref (upwards));
          need_timer = TRUE;
        }
    }
  if (((GObject*) window)->ref_count && downwards && ((GObject*) downwards)->ref_count)
    {
      GSList *slist = (GSList*) g_object_get_qdata ((GObject*) window, quark_widgets_downwards);
      if (!g_slist_find (slist, downwards))
        {
          if (!slist)
            g_object_set_qdata_full ((GObject*) window, quark_widgets_downwards,
                                     g_slist_prepend (NULL, g_object_ref (downwards)),
                                     free_widget_slist);
          else
            slist->next = g_slist_prepend (slist->next, g_object_ref (downwards));
          need_timer = TRUE;
        }
    }
  if (need_timer && !g_slist_find (window_queue, window))
    {
      if (!window_queue)
        g_timeout_add (50, gxk_action_timer_update_window, NULL);
      window_queue = g_slist_prepend (window_queue, g_object_ref (window));
    }
}



void
gxk_widget_update_actions_upwards (gpointer widget)
{
  GtkWidget *toplevel = gtk_widget_get_toplevel ((GtkWidget*) widget);
  g_return_if_fail (GTK_IS_WIDGET (widget));
  if (GTK_IS_WINDOW (toplevel))
    window_queue_action_updates (toplevel, (GtkWidget*) widget, NULL);
}

void
gxk_widget_update_actions_downwards (gpointer widget)
{
  GtkWidget *toplevel = gtk_widget_get_toplevel ((GtkWidget*) widget);
  g_return_if_fail (GTK_IS_WIDGET (widget));
  if (GTK_IS_WINDOW (toplevel))
    window_queue_action_updates (toplevel, NULL, (GtkWidget*) widget);
}

void
gxk_widget_update_actions (gpointer widget)
{
  GtkWidget *toplevel = gtk_widget_get_toplevel ((GtkWidget*) widget);
  g_return_if_fail (GTK_IS_WIDGET (widget));
  if (GTK_IS_WINDOW (toplevel))
    window_queue_action_updates (toplevel, (GtkWidget*) widget, (GtkWidget*) widget);
}


/* --- publishing --- */
static GQuark  quark_widget_actions = 0;
static GSList *publisher_list = NULL;

static gboolean
action_idle_publisher (gpointer data)
{
  GDK_THREADS_ENTER ();
  while (publisher_list)
    {
      GtkWidget *widget = (GtkWidget*) g_slist_pop_head (&publisher_list);
      GtkWidget *toplevel = gtk_widget_get_toplevel (widget);
      GSList *slist = (GSList*) g_object_get_qdata ((GObject*) widget, quark_widget_actions);
      while (slist)
        {
          ActionLink *alink = (ActionLink*) slist->data;
          if (alink->toplevel != toplevel)
            {
              if (alink->toplevel)
                window_remove_action_link (alink);
              if (toplevel)
                window_add_action_link (toplevel, alink);
            }
          slist = slist->next;
        }
      g_object_unref (widget);
    }
  GDK_THREADS_LEAVE ();
  return FALSE;
}

static void
publisher_update_actions (GtkWidget *widget)
{
  if (!g_slist_find (publisher_list, widget))
    {
      if (!publisher_list)
        g_idle_add_full (GXK_ACTION_PRIORITY, action_idle_publisher, NULL, NULL);
      publisher_list = g_slist_prepend (publisher_list, g_object_ref (widget));
    }
}

static void
publisher_destroy_action_links (gpointer data)
{
  GSList *slist = (GSList*) data;
  while (slist)
    {
      ActionLink *alink = (ActionLink*) g_slist_pop_head (&slist);
      if (alink->toplevel)
        window_remove_action_link (alink);
      action_link_unref (alink);
    }
}

void
gxk_widget_publish_action_list (gpointer       widget,
                                const gchar   *prefix,
                                GxkActionList *alist)
{
  ActionLink *alink = g_new0 (ActionLink, 1);
  g_return_if_fail (GTK_IS_WIDGET (widget));
  alink->ref_count = 1;
  alink->widget = (GtkWidget*) widget;
  alink->prefix = g_strdup (prefix);
  alink->alist = alist;
  g_object_set_qdata_full ((GObject*) widget, quark_widget_actions,
                           g_slist_prepend ((GSList*) g_object_steal_qdata ((GObject*) widget, quark_widget_actions), alink),
                           publisher_destroy_action_links);
  if (!gxk_signal_handler_exists (widget, "hierarchy_changed", G_CALLBACK (publisher_update_actions), NULL))
    g_object_connect (widget, "signal_after::hierarchy-changed", publisher_update_actions, NULL, NULL);
  publisher_update_actions ((GtkWidget*) widget);
  if (!gxk_signal_handler_exists (widget, "realize", G_CALLBACK (gxk_widget_update_actions), NULL))
    g_object_connect (widget, "signal_after::realize", gxk_widget_update_actions, NULL, NULL);
  gxk_widget_update_actions (widget);
}

GSList*
gxk_widget_peek_action_widgets (gpointer                widget,
                                const gchar            *prefix,
                                gulong                  action_id)
{
  GSList *alinklist = (GSList*) g_object_get_qdata ((GObject*) widget, quark_widget_actions);
  for (; alinklist; alinklist = alinklist->next)
    {
      ActionLink *alink = (ActionLink*) alinklist->data;
      if (strcmp (alink->prefix, prefix) == 0)
        {
          guint i;
          for (i = 0; i < alink->alist->n_entries; i++)
            if (alink->alist->entries[i]->action.action_id == action_id)
              return alink->alist->entries[i]->widgets;
          return NULL;
        }
    }
  return NULL;
}

void
gxk_widget_republish_actions (gpointer                widget,
                              const gchar            *prefix,
                              gpointer                source_widget)
{
  GSList *slist, *alinks = (GSList*) g_object_get_qdata ((GObject*) source_widget, quark_widget_actions);
  for (slist = alinks; slist; slist = slist->next)
    {
      ActionLink *alink = (ActionLink*) slist->data;
      if (strcmp (alink->prefix, prefix) == 0)
        {
          gxk_widget_publish_action_list (widget, prefix, gxk_action_list_copy (alink->alist));
          return;
        }
    }
}

void
gxk_widget_publish_actions_grouped (gpointer                widget,
                                    GxkActionGroup         *group,
                                    const gchar            *prefix,
                                    guint                   n_actions,
                                    const GxkStockAction   *actions,
                                    const gchar            *i18n_domain,
                                    GxkActionCheck          acheck,
                                    GxkActionExec           aexec)
{
  GxkActionList *alist = gxk_action_list_create_grouped (group);
  g_return_if_fail (GTK_IS_WIDGET (widget));
  gxk_action_list_add_actions (alist, n_actions, actions, i18n_domain, acheck, aexec, widget);
  gxk_widget_publish_action_list (widget, prefix, alist);
}

void
gxk_widget_publish_actions (gpointer                widget,
                            const gchar            *prefix,
                            guint                   n_actions,
                            const GxkStockAction   *actions,
                            const gchar            *i18n_domain,
                            GxkActionCheck          acheck,
                            GxkActionExec           aexec)
{
  gxk_widget_publish_actions_grouped (widget, NULL, prefix, n_actions, actions, i18n_domain, acheck, aexec);
}

void
gxk_widget_publish_grouped_translated (gpointer                widget,
                                       GxkActionGroup         *group,
                                       const gchar            *prefix,
                                       const gchar            *key,             /* untranslated name */
                                       const gchar            *name,            /* translated (key) */
                                       const gchar            *accelerator,
                                       const gchar            *tooltip,         /* translated */
                                       gulong                  action_id,
                                       const gchar            *stock_icon,
                                       GxkActionCheck          acheck,
                                       GxkActionExec           aexec)
{
  GxkActionList *alist = gxk_action_list_create_grouped (group);
  g_return_if_fail (name != NULL);
  gxk_action_list_add_translated (alist, g_intern_string (key ? key : name), name,
                                  accelerator, tooltip, action_id, stock_icon,
                                  acheck, aexec, widget);
  gxk_widget_publish_action_list (widget, prefix, alist);
}

void
gxk_widget_publish_translated (gpointer                widget,
                               const gchar            *prefix,
                               const gchar            *key,             /* untranslated name */
                               const gchar            *name,            /* translated (part of key) */
                               const gchar            *accelerator,
                               const gchar            *tooltip,         /* translated */
                               gulong                  action_id,
                               const gchar            *stock_icon,
                               GxkActionCheck          acheck,
                               GxkActionExec           aexec)
{
  g_return_if_fail (name != NULL);
  gxk_widget_publish_grouped_translated (widget, NULL, prefix, key ? key : name, name, accelerator,
                                         tooltip, action_id, stock_icon, acheck, aexec);
}

void
gxk_window_add_action_client (GtkWindow              *window,
                              GxkActionClient         added_func,
                              gpointer                client_data)
{
  g_return_if_fail (GTK_IS_WINDOW (window));
  g_return_if_fail (client_data != NULL);
  ActionClient *aclient = g_new0 (ActionClient, 1);
  aclient->client_data = client_data;
  aclient->added_func = added_func;
  GSList *slist = (GSList*) g_object_get_qdata ((GObject*) window, quark_action_clients);
  g_object_set_qdata ((GObject*) window, quark_action_clients, g_slist_prepend (slist, aclient));
  ActionLink *alink;
  for (alink = (ActionLink*) g_object_get_qdata ((GObject*) window, quark_action_links); alink; alink = alink->next)
    aclient->added_func (aclient->client_data, window, alink->prefix, alink->alist, alink->widget);
}

void
gxk_window_remove_action_client (GtkWindow              *window,
                                 gpointer                client_data)
{
  g_return_if_fail (GTK_IS_WINDOW (window));
  g_return_if_fail (client_data != NULL);
  GSList *last = NULL, *slist = (GSList*) g_object_get_qdata ((GObject*) window, quark_action_clients);
  while (slist)
    {
      ActionClient *aclient = (ActionClient*) slist->data;
      if (aclient->client_data == client_data)
        {
          if (last)
            last->next = slist->next;
          else
            g_object_set_qdata ((GObject*) window, quark_action_clients, slist->next);
          g_free (aclient);
          return;
        }
      last = slist;
      slist = last->next;
    }
  g_warning ("failed to remove action client (%p) from GtkWindow (%p)", client_data, window);
}

/* --- action group --- */
static gulong action_group_signal_changed = 0;

static void
gxk_action_group_class_init (GxkActionGroupClass *klass)
{
  action_group_signal_changed = g_signal_new ("changed", G_OBJECT_CLASS_TYPE (klass),
                                              G_SIGNAL_RUN_LAST,
                                              G_STRUCT_OFFSET (GxkActionGroupClass, changed),
                                              NULL, NULL, gtk_signal_default_marshaller,
                                              G_TYPE_NONE, 0);
}

GType
gxk_action_group_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static const GTypeInfo type_info = {
        sizeof (GxkActionGroupClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gxk_action_group_class_init,
        NULL,   /* class_finalize */
        NULL,   /* class_data */
        sizeof (GxkActionGroup),
        0,      /* n_preallocs */
        (GInstanceInitFunc) NULL,
      };
      type = g_type_register_static (G_TYPE_OBJECT, "GxkActionGroup", &type_info, GTypeFlags (0));
    }
  return type;
}

GxkActionGroup*
gxk_action_toggle_new (void)
{
  GxkActionGroup *group = gxk_action_group_new ();
  group->invert_dups = TRUE;
  return group;
}

GxkActionGroup*
gxk_action_group_new (void)
{
  return (GxkActionGroup*) g_object_new (GXK_TYPE_ACTION_GROUP, NULL);
}

void
gxk_action_group_select (GxkActionGroup *self,
                         gulong          action_id)
{
  g_return_if_fail (GXK_IS_ACTION_GROUP (self));
  if (!self->lock_count && (action_id != self->action_id || self->invert_dups))
    {
      if (action_id == self->action_id)
        self->action_id = !self->action_id;
      else
        self->action_id = action_id;
      self->lock_count++;
      g_signal_emit (self, action_group_signal_changed, 0);
      self->lock_count--;
    }
}

void
gxk_action_group_lock (GxkActionGroup *self)
{
  g_return_if_fail (GXK_IS_ACTION_GROUP (self));
  self->lock_count++;
}

void
gxk_action_group_unlock (GxkActionGroup *self)
{
  g_return_if_fail (GXK_IS_ACTION_GROUP (self));
  g_return_if_fail (self->lock_count > 0);
  self->lock_count--;
}

void
gxk_action_group_dispose (GxkActionGroup *self)
{
  g_return_if_fail (GXK_IS_ACTION_GROUP (self));
  g_object_run_dispose ((GObject*) self);
}


/* --- initialization --- */
void
gxk_init_actions (void)
{
  g_assert (action_class_ht == NULL);
  action_class_ht = g_hash_table_new (action_class_hash, action_class_equals);
  quark_action_links = g_quark_from_static_string ("gxk-action-links");
  quark_action_factories = g_quark_from_static_string ("GxkActionFactory-list");
  quark_action_entry = g_quark_from_static_string ("gxk-action-entry");
  quark_action_clients = g_quark_from_static_string ("gxk-action-clients");
  quark_widget_actions = g_quark_from_static_string ("gxk-action-widget-action-links");
  quark_widgets_upwards = g_quark_from_static_string ("gxk-action-widgets-upwards");
  quark_widgets_downwards = g_quark_from_static_string ("gxk-action-widgets-downwards");
}
