/* GXK - Gtk+ Extension Kit
 * Copyright (C) 2002-2006 Tim Janik
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
#include "gxkradget.h"
#include "gxkradgetfactory.h"
#include "gxkauxwidgets.h"
#include "gxkmenubutton.h"
#include "gxknotebook.h"
#include "glewidgets.h"
#include "gxksimplelabel.h"
#include "gxkracktable.h"
#include "gxkrackitem.h"
#include <libintl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define NODE(n)         ((Node*) n)

struct _GxkRadgetArgs {
  guint    n_variables;
  gboolean intern_quarks;
  GQuark  *quarks;
  gchar  **values;
};
#define ARGS_N_ENTRIES(o)    ((o) ? (o)->n_variables : 0)
#define ARGS_NTH_NAME(o,n)   (g_quark_to_string ((o)->quarks[(n)]))
#define ARGS_NTH_VALUE(o,n)  ((o)->values[(n)])

typedef struct {
  guint null_collapse : 1;
} EnvSpecials;

typedef struct {
  GSList       *args_list;              /* GxkRadgetArgs* */
  const gchar  *name;
  EnvSpecials  *specials;               /* per expression level */
  guint         skip_property : 1;      /* per call */
  GData        *hgroups, *vgroups, *hvgroups;
  GxkRadget    *xdef_radget;
} Env;

typedef struct {
  const gchar *name;
  const gchar *value;
} Prop;
typedef struct Node Node;
struct Node {
  const gchar  *domain;
  const gchar  *name;
  GType         type;
  guint         depth; /* == number of ancestors */
  Node         *xdef_node;
  guint         xdef_depth; /* xdef_node->depth */
  GSList       *parent_arg_list; /* of type GxkRadgetArgs* */
  GxkRadgetArgs *call_args;
  GxkRadgetArgs *scope_args;
  GxkRadgetArgs *prop_args;
  GxkRadgetArgs *pack_args;
  GxkRadgetArgs *dfpk_args;
  GxkRadgetArgs *hook_args;
  const gchar  *size_hgroup, *size_vgroup, *size_hvgroup;
  const gchar  *size_window_hgroup, *size_window_vgroup, *size_window_hvgroup;
  const gchar *default_area;
  GSList       *children;       /* Node* */
  GSList       *call_stack;     /* expanded call_args */
};
typedef struct {
  GData *nodes;
  const gchar *domain;
} Domain;
typedef struct {
  Domain      *domain;
  const gchar *i18n_domain;
  guint        tag_opened;
  GSList      *node_stack; /* Node* */
} PData;                /* parser state */
typedef gchar* (*MacroFunc)     (GSList *args,
                                 Env    *env);


/* --- prototypes --- */
static gchar*          expand_expr              (const gchar         *expr,
                                                 Env                 *env);
static MacroFunc       macro_func_lookup        (const gchar         *name);
static inline gboolean boolean_from_string      (const gchar         *value);
static gboolean        boolean_from_expr        (const gchar         *expr,
                                                 Env                 *env);
static inline guint64  num_from_string          (const gchar         *value);
static guint64         num_from_expr            (const gchar         *expr,
                                                 Env                 *env);
static void            radget_define_gtk_menu   (void);
static Node*           node_children_find_area  (Node                *node,
                                                 const gchar         *area);
static const gchar*    radget_args_lookup_quark (const GxkRadgetArgs *args,
                                                 GQuark               quark,
                                                 guint               *nth);
static GParamSpec*     find_hook                (const gchar         *name,
                                                 GxkRadgetHook       *hook_func_p);
static void            gxk_adopt_hint_hook      (GxkRadget           *radget,
                                                 guint                property_id,
                                                 const GValue        *value,
                                                 GParamSpec          *pspec);


/* --- variables --- */
static Domain *standard_domain = NULL;
static GQuark  quark_id = 0;
static GQuark  quark_name = 0;
static GQuark  quark_radget_type = 0;
static GQuark  quark_radget_node = 0;


/* --- functions --- */
static void
set_error (GError     **error,
           const gchar *message_fmt,
           ...)
{
  if (error && !*error)
    {
      va_list args;
      gchar *buffer;
      va_start (args, message_fmt);
      buffer = g_strdup_vprintf (message_fmt, args);
      va_end (args);
      *error = g_error_new_literal (1, 0, buffer);
      (*error)->domain = 0;
      g_free (buffer);
    }
}

#define g_slist_new(d) g_slist_prepend (0, d)

typedef struct {
  Node *source;
  Node *clone;
} NodeClone;
typedef struct {
  guint        n_clones;
  NodeClone   *clones;
} CloneList;

static void
clone_list_add (CloneList *clist,
                Node      *source,
                Node      *clone)
{
  guint i = clist->n_clones++;
  clist->clones = g_renew (NodeClone, clist->clones, clist->n_clones);
  clist->clones[i].source = source;
  clist->clones[i].clone = clone;
}

static Node*
clone_list_find (CloneList *clist,
                 Node      *source)
{
  guint i;
  for (i = 0; i < clist->n_clones; i++)
    if (clist->clones[i].source == source)
      return clist->clones[i].clone;
  g_warning ("failed to find clone for %p", source);
  return NULL;
}

static GxkRadgetArgs*
clone_args (const GxkRadgetArgs *source)
{
  if (source)
    return gxk_radget_args_merge (gxk_radget_const_args (), source);
  return NULL;
}

static Node*
clone_node_intern (Node        *source,
                   const gchar *domain,
                   const gchar *name,
                   gboolean     inherit,
                   CloneList   *clist)
{
  Node *node = g_new0 (Node, 1);
  GSList *slist, *last = NULL;
  clone_list_add (clist, source, node);
  node->domain = domain;
  node->name = g_intern_string (name);
  node->type = source->type;
  if (inherit)
    {
      node->depth = source->depth + 1;
      /* no deep copy needed, since source->parent_arg_list are already parsed */
      node->parent_arg_list = source->parent_arg_list;
      /* "add-on" the new modifiable parent_arg_list slot (works only with slists) */
      node->parent_arg_list = g_slist_prepend (node->parent_arg_list, gxk_radget_const_args ());
    }
  else
    {
      node->depth = source->depth;
      /* no deep copy needed, since source->parent_arg_list are already parsed */
      node->parent_arg_list = source->parent_arg_list;
    }
  if (source->xdef_node)
    {
      g_assert (!inherit);
      node->xdef_node = clone_list_find (clist, source->xdef_node);
      node->xdef_depth = source->xdef_depth;
    }
  node->call_args = clone_args (source->call_args);
  node->scope_args = clone_args (source->scope_args);
  node->prop_args = clone_args (source->prop_args);
  node->pack_args = clone_args (source->pack_args);
  node->dfpk_args = clone_args (source->dfpk_args);
  node->hook_args = clone_args (source->hook_args);
  node->size_hgroup = source->size_hgroup;
  node->size_vgroup = source->size_vgroup;
  node->size_hvgroup = source->size_hvgroup;
  node->size_window_hgroup = source->size_window_hgroup;
  node->size_window_vgroup = source->size_window_vgroup;
  node->size_window_hvgroup = source->size_window_hvgroup;
  node->default_area = source->default_area;
  for (slist = source->children; slist; slist = slist->next)
    {
      Node *child = slist->data;
      child = clone_node_intern (child, domain, child->name, FALSE, clist);
      if (last)
        {
          last->next = g_slist_new (child);
          last = last->next;
        }
      else
        node->children = last = g_slist_new (child);
    }
  g_assert (source->call_stack == NULL);
  return node;
}

static Node*
clone_node (Node        *source,
            const gchar *domain,
            const gchar *name,
            gboolean     inherit)
{
  CloneList clist = { 0, NULL };
  Node *node = clone_node_intern (source, domain, name, inherit, &clist);
  g_free (clist.clones);
  return node;
}

static inline gboolean
boolean_from_string (const gchar *value)
{
  return !(!value || strlen (value) < 1 || value[0] == '0' ||
           value[0] == 'f' || value[0] == 'F' ||
           value[0] == 'n' || value[0] == 'N');
}

static inline gdouble
float_from_string (const gchar *value)
{
  gdouble v_float = value ? g_strtod (value, NULL) : 0;
  return v_float;
}

static inline guint64
num_from_string (const gchar *value)
{
  gdouble v_float = float_from_string (value);
  v_float = v_float > 0 ? v_float + 0.5 : v_float - 0.5;
  return v_float;
}

static inline gchar
char2eval (const gchar c)
{
  if (c >= '0' && c <= '9')
    return c;
  else if (c >= 'A' && c <= 'Z')
    return c - 'A' + 'a';
  else if (c >= 'a' && c <= 'z')
    return c;
  else
    return '-';
}

static inline gboolean
enum_match (const gchar *str1,
            const gchar *str2)
{
  while (*str1 && *str2)
    {
      guchar s1 = char2eval (*str1++);
      guchar s2 = char2eval (*str2++);
      if (s1 != s2)
        return FALSE;
    }
  return *str1 == 0 && *str2 == 0;
}

static gint
enums_match_value (guint        n_values,
                   GEnumValue  *values,
                   const gchar *name,
                   gint         fallback)
{
  guint i, length = strlen (name);
  if (!name)
    return fallback;
  for (i = 0; i < n_values; i++)
    {
      const gchar *vname = values[i].value_name;
      guint n = strlen (vname);
      if (((n > length && char2eval (vname[n - 1 - length]) == '-')
           || n == length)
          && enum_match (vname + n - length, name))
        return values[i].value;
    }
  for (i = 0; i < n_values; i++)
    {
      const gchar *vname = values[i].value_nick;
      guint n = strlen (vname);
      if (((n > length && char2eval (vname[n - 1 - length]) == '-')
           || n == length)
          && enum_match (vname + n - length, name))
        return values[i].value;
    }
  return fallback;
}

static void
env_clear (Env *env)
{
  g_datalist_clear (&env->hgroups);
  g_datalist_clear (&env->vgroups);
  g_datalist_clear (&env->hvgroups);
}

static GtkSizeGroup*
env_get_size_group (Env         *env,
                    const gchar *name,
                    gchar        type)
{
  GData **groups = type == 'h' ? &env->hgroups : type == 'v' ? &env->vgroups : &env->hvgroups;
  GtkSizeGroup *sg = g_datalist_get_data (groups, name);
  if (!sg)
    {
      sg = gtk_size_group_new (type == 'h' ? GTK_SIZE_GROUP_HORIZONTAL :
                               type == 'v' ? GTK_SIZE_GROUP_VERTICAL :
                               GTK_SIZE_GROUP_BOTH);
      g_datalist_set_data_full (groups, name, sg, g_object_unref);
    }
  return sg;
}

typedef struct _RecursiveOption RecursiveOption;
struct _RecursiveOption {
  RecursiveOption     *next;
  const GxkRadgetArgs *args;
  GQuark               quark;
};
static RecursiveOption *stack_options = NULL;

static const GxkRadgetArgs*
env_find_quark (Env   *env,
                GQuark quark,
                guint *nthp)
{
  GSList *slist;
  for (slist = env->args_list; slist; slist = slist->next)
    {
      GxkRadgetArgs *args = slist->data;
      if (radget_args_lookup_quark (args, quark, nthp))
        {
          RecursiveOption *ropt;
          for (ropt = stack_options; args && ropt; ropt = ropt->next)
            if (args == ropt->args && quark == ropt->quark)
              args = NULL;
          if (args)
            return args;
        }
    }
  return NULL;
}

static const gchar*
env_lookup (Env         *env,
            const gchar *var)
{
  const GxkRadgetArgs *args;
  GQuark quark = g_quark_try_string (var);
  if (quark == quark_name || quark == quark_id)
    return env->name;
  guint nth;
  args = quark ? env_find_quark (env, quark, &nth) : NULL;
  return args ? ARGS_NTH_VALUE (args, nth) : NULL;
}

static gchar*
env_expand_args_value (Env                 *env,
                       const GxkRadgetArgs *args,
                       guint                nth)
{
  const gchar *value = ARGS_NTH_VALUE (args, nth);
  gchar *exval;
  RecursiveOption ropt;
  ropt.args = args;
  ropt.quark = args->quarks[nth];
  ropt.next = stack_options;
  stack_options = &ropt;
  exval = expand_expr (value, env);
  stack_options = ropt.next;
  return exval;
}

static inline const gchar*
advance_level (const gchar *c)
{
  guint level = 1;
  do                            /* read til ')' */
    switch (*c++)
      {
      case 0:
        c--;
        level = 0;
        break;
      case '(':
        level++;
        break;
      case ')':
        level--;
        break;
      }
  while (level);
  return c;
}

static inline const gchar*
advance_arg (const gchar *c)
{
  while (*c && *c != ')' && *c != ',')
    if (*c == '$' && c[1] == '(')
      c = advance_level (c + 2);
    else
      c++;
  return c;
}

static const gchar*
parse_formula (const gchar *c,
               GString     *result,
               Env         *env)
{
  const gchar *start = c;
  const gchar *last = c;
  GSList *args = NULL;
  while (*c && *c != ')')
    {
      if (*c == ',')
        {
          args = g_slist_prepend (args, g_strndup (last, c - last));
          last = ++c;
        }
      else
        c = advance_arg (c);
    }
  args = g_slist_prepend (args, g_strndup (last, c - last));
  if (!*c)
    g_printerr ("malformed formula: $(%s\n", start);
  else
    c++;        /* ')' */
  if (args)
    {
      gchar *str;
      args = g_slist_reverse (args);
      str = macro_func_lookup (args->data) (args, env);
      if (str)
        g_string_append (result, str);
      g_free (str);
      g_slist_foreach (args, (GFunc) g_free, NULL);
      g_slist_free (args);
    }
  return c;
}

static const gchar*
parse_dollar (const gchar *c,
              GString     *result,
              Env         *env)
{
  const gchar *ident_start = G_CSET_A_2_Z G_CSET_a_2_z "_";
  const gchar *ident_chars = G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS ".-_";
  const gchar *mark = c;
  GQuark quark;
  if (*c == '(')
    return parse_formula (c + 1, result, env);
  if (*c == '$')
    {
      g_string_append_c (result, '$');
      return c + 1;
    }
  if (strchr (ident_start, *c))
    {
      gchar *var;
      c++;
      while (*c && strchr (ident_chars, *c))
        c++;
      var = g_strndup (mark, c - mark);
      quark = g_quark_try_string (var);
      g_free (var);
      if (quark == quark_name || quark == quark_id)
        g_string_append (result, env->name);
      else
        {
          guint nth;
          const GxkRadgetArgs *args = env_find_quark (env, quark, &nth);
          if (args)
            {
              gchar *exval = env_expand_args_value (env, args, nth);
              if (exval)
                g_string_append (result, exval);
              g_free (exval);
            }
        }
      return c;
    }
  if (*c)       /* eat one non-ident char */
    c++;
  return c;
}

static gchar*
expand_expr (const gchar *expr,
             Env         *env)
{
  GString *result = g_string_new ("");
  const gchar *c = expr;
  const gchar *dollar = strchr (c, '$');
  EnvSpecials *saved_specials = env->specials;
  EnvSpecials specials = { 0, };
  env->specials = &specials;
  while (dollar)
    {
      g_string_append_len (result, c, dollar - c);
      c = parse_dollar (dollar + 1, result, env);
      dollar = strchr (c, '$');
    }
  g_string_append (result, c);
  env->specials = saved_specials;
  gboolean null_collapse = specials.null_collapse;
  specials.null_collapse = FALSE;
  if (null_collapse && !result->str[0])
    return g_string_free (result, TRUE);
  else
    return g_string_free (result, FALSE);
}

static guint64
num_from_expr (const gchar    *expr,
               Env            *env)
{
  gchar *result = expand_expr (expr, env);
  guint64 num = num_from_string (result);
  g_free (result);
  return num;
}

static gboolean
boolean_from_expr (const gchar    *expr,
                   Env            *env)
{
  gchar *result = expand_expr (expr, env);
  gboolean boolv = boolean_from_string (result);
  g_free (result);
  return boolv;
}

static Node*
node_children_find_area (Node        *node,
                         const gchar *area)
{
  GSList *children = g_slist_copy (node->children);
  while (children)
    {
      GSList *slist, *newlist = NULL;
      for (slist = children; slist; slist = slist->next)
        if (strcmp (NODE (slist->data)->name, area) == 0)
          {
            node = slist->data;
            g_slist_free (children);
            return node;
          }
      for (slist = children; slist; slist = slist->next)
        newlist = g_slist_concat (g_slist_copy (NODE (slist->data)->children), newlist);
      g_slist_free (children);
      children = newlist;
    }
  return NULL;
}

static Node*
node_find_area (Node        *node,
                const gchar *area)
{
  Node *child;
  const gchar *p;
  if (!area || strcmp ("default", area) == 0)
    area = node->default_area;
  if (!area)
    return node;
  p = strchr (area, '.');
  if (p)
    {
      gchar *str = g_strndup (area, p - area);
      if (strcmp (str, "default") == 0)
        child = !node->default_area ? node : node_children_find_area (node, node->default_area);
      else if (strcmp (str, node->name) == 0)
        child = node;
      else
        child = node_children_find_area (node, str);
      g_free (str);
      if (child)
        return node_find_area (child, p + 1);
      else
        return node_find_area (node, NULL);
    }
  if (strcmp (area, node->name) == 0)
    child = node;
  else
    child = node_children_find_area (node, area);
  if (child)
    return node_find_area (child, NULL);
  else
    return node;
}

static Node*
node_lookup (Domain      *domain,
             const gchar *node_name)
{
  Node *node = g_datalist_get_data (&domain->nodes, node_name);
  if (!node && domain != standard_domain)
    node = g_datalist_get_data (&standard_domain->nodes, node_name);
  return node;
}

static GxkRadgetArgs*
radget_args_intern_set (GxkRadgetArgs  *args,
                        const gchar    *name,
                        const gchar    *value)
{
  if (!args)
    args = gxk_radget_const_args ();
  return gxk_radget_args_set (args, name, value);
}

static Node*
node_define (Domain       *domain,
             const gchar  *node_name,
             GType         direct_type, /* for builtins */
             Node         *source,      /* for calls */
             Node         *xdef_node,   /* for calls */
             const gchar **attribute_names,
             const gchar **attribute_values,
             const gchar  *i18n_domain,
             const gchar **name_p,
             const gchar **area_p,
             const gchar **default_area_p,
             GError       **error)
{
  Node *node = NULL;
  gboolean allow_defs = !source, inherit = FALSE;
  guint i;
  if (direct_type)                              /* builtin definition */
    {
      node = g_new0 (Node, 1);
      node->domain = domain->domain;
      node->name = g_intern_string (node_name);
      node->type = direct_type;
    }
  else if (source)                              /* call */
    {
      node = clone_node (source, domain->domain, node_name, inherit);
      node->xdef_node = xdef_node;
      g_assert (xdef_node && !xdef_node->xdef_node);
      node->xdef_depth = node->xdef_node->depth;
    }
  else for (i = 0; attribute_names[i]; i++)     /* node inheritance */
    if (!node && strcmp (attribute_names[i], "inherit") == 0)
      {
        source = node_lookup (domain, attribute_values[i]);
        inherit = TRUE;
        if (source)
          node = clone_node (source, domain->domain, node_name, inherit);
        break;
      }
  /* apply attributes */
  for (i = 0; attribute_names[i]; i++)
    if (default_area_p && !*default_area_p && strcmp (attribute_names[i], "default-area") == 0)
      {
        *default_area_p = g_intern_string (attribute_values[i]);
      }
    else if (area_p && !*area_p && strcmp (attribute_names[i], "area") == 0)
      {
        *area_p = g_intern_string (attribute_values[i]);
      }
  if (!node)
    set_error (error, "no radget type specified in definition of: %s", node_name);
  if (*error)
    return NULL;
  /* apply property attributes */
  for (i = 0; attribute_names[i]; i++)
    if (strncmp (attribute_names[i], "pack:", 5) == 0)
      node->pack_args = radget_args_intern_set (node->pack_args, attribute_names[i] + 5, attribute_values[i]);
    else if (strncmp (attribute_names[i], "default-pack:", 13) == 0)
      node->dfpk_args = radget_args_intern_set (node->dfpk_args, attribute_names[i] + 13, attribute_values[i]);
    else if (strncmp (attribute_names[i], "hook:", 5) == 0)
      node->hook_args = radget_args_intern_set (node->hook_args, attribute_names[i] + 5, attribute_values[i]);
    else if (strcmp (attribute_names[i], "name") == 0 || strcmp (attribute_names[i], "_name") == 0 ||
             strcmp (attribute_names[i], "id") == 0 || strcmp (attribute_names[i], "_id") == 0)
      {
        if (name_p && !*name_p)
          *name_p = g_intern_string (attribute_values[i]);
      }
    else if (allow_defs && strncmp (attribute_names[i], "prop:", 5) == 0)
      node->prop_args = radget_args_intern_set (node->prop_args, attribute_names[i] + 5, attribute_values[i]);
    else if (strcmp (attribute_names[i], "size:hgroup") == 0 && g_type_is_a (node->type, GTK_TYPE_WIDGET))
      node->size_hgroup = g_intern_string (attribute_values[i]);
    else if (strcmp (attribute_names[i], "size:vgroup") == 0 && g_type_is_a (node->type, GTK_TYPE_WIDGET))
      node->size_vgroup = g_intern_string (attribute_values[i]);
    else if (strcmp (attribute_names[i], "size:hvgroup") == 0 && g_type_is_a (node->type, GTK_TYPE_WIDGET))
      node->size_hvgroup = g_intern_string (attribute_values[i]);
    else if (strcmp (attribute_names[i], "size:window-hgroup") == 0 && g_type_is_a (node->type, GTK_TYPE_WIDGET))
      node->size_window_hgroup = g_intern_string (attribute_values[i]);
    else if (strcmp (attribute_names[i], "size:window-vgroup") == 0 && g_type_is_a (node->type, GTK_TYPE_WIDGET))
      node->size_window_vgroup = g_intern_string (attribute_values[i]);
    else if (strcmp (attribute_names[i], "size:window-hvgroup") == 0 && g_type_is_a (node->type, GTK_TYPE_WIDGET))
      node->size_window_hvgroup = g_intern_string (attribute_values[i]);
    else if (strcmp (attribute_names[i], "inherit") == 0 ||
             strcmp (attribute_names[i], "default-area") == 0 ||
             strcmp (attribute_names[i], "area") == 0)
      ; /* handled above */
#if 0   // if at all, this should be part of an extra <local var=value"/> directive
    else if (strncmp (attribute_names[i], "local:", 6) == 0)
      node->scope_args = radget_args_intern_set (node->scope_args, attribute_names[i] + 6, attribute_values[i]);
#endif
    else if (strchr (attribute_names[i], ':'))
      set_error (error, "invalid attribute \"%s\" in definition of: %s", attribute_names[i], node_name);
    else
      {
        const gchar *name = attribute_names[i];
        const gchar *value = attribute_values[i];
        if (inherit)
          radget_args_intern_set (node->parent_arg_list->data, name, value);
        else
          node->call_args = radget_args_intern_set (node->call_args, name, value);
        if (name[0] == '_') /* i18n version */
          {
            if (inherit)
              radget_args_intern_set (node->parent_arg_list->data, name + 1, dgettext (i18n_domain, value));
            else
              node->call_args = radget_args_intern_set (node->call_args, name + 1, dgettext (i18n_domain, value));
          }
      }
  if (!g_type_is_a (node->type, G_TYPE_OBJECT))
    set_error (error, "no radget type specified in definition of: %s", node_name);
  return node;
}

static void             /* callback for open tags <foo bar="baz"> */
radget_start_element  (GMarkupParseContext *context,
                       const gchar         *element_name,
                       const gchar        **attribute_names,
                       const gchar        **attribute_values,
                       gpointer             user_data,
                       GError             **error)
{
  PData *pdata = user_data;
  Node *child;
  if (!pdata->tag_opened && strcmp (element_name, "gxk-radget-definitions") == 0)
    {
      /* toplevel tag */
      pdata->tag_opened = TRUE;
    }
  else if (pdata->node_stack == NULL && strncmp (element_name, "xdef:", 5) == 0)
    {
      const gchar *name = element_name + 5;
      if (g_datalist_get_data (&pdata->domain->nodes, name))
        set_error (error, "redefinition of radget: %s", name);
      else
        {
          const gchar *default_area = NULL;
          Node *node = node_define (pdata->domain, name, 0, NULL, NULL, attribute_names, attribute_values,
                                    pdata->i18n_domain, NULL, NULL, &default_area, error);
          if (node)
            {
              if (default_area)
                node->default_area = default_area;
              g_datalist_set_data (&pdata->domain->nodes, name, node);
              pdata->node_stack = g_slist_prepend (pdata->node_stack, node);
            }
        }
    }
  else if (pdata->node_stack && (child = node_lookup (pdata->domain, element_name), child))
    {
      Node *parent = pdata->node_stack->data;
      const gchar *area = NULL, *uname = NULL;
      Node *node = node_define (pdata->domain, element_name, 0, child, g_slist_last (pdata->node_stack)->data,
                                attribute_names, attribute_values, pdata->i18n_domain,
                                &uname, &area, NULL, error);
      if (node)
        {
          if (uname)
            node->name = uname;
          parent = node_find_area (parent, area);
          parent->children = g_slist_append (parent->children, node);
          pdata->node_stack = g_slist_prepend (pdata->node_stack, node);
        }
      else
        set_error (error, "failed to define radget: %s", element_name);
    }
  else
    set_error (error, "unknown element: %s", element_name);
}

static void             /* callback for close tags </foo> */
radget_end_element (GMarkupParseContext *context,
                    const gchar         *element_name,
                    gpointer             user_data,
                    GError             **error)
{
  PData *pdata = user_data;
  if (strcmp (element_name, "gxk-radget-definitions") == 0)
    {
      /* toplevel tag closed */
      pdata->tag_opened = FALSE;
    }
  else if (pdata->node_stack && pdata->node_stack->next == NULL && strncmp (element_name, "xdef:", 5) == 0)
    {
      g_slist_pop_head (&pdata->node_stack);
    }
  else if (pdata->node_stack && pdata->node_stack->next)
    {
      g_slist_pop_head (&pdata->node_stack);
    }
}

static void             /* callback for character data */
radget_text (GMarkupParseContext *context,
             const gchar         *text,    /* text is not 0-terminated */
             gsize                text_len,
             gpointer             user_data,
             GError             **error)
{
  // PData *pdata = user_data;
}

static void             /* callback for comments and processing instructions */
radget_passthrough (GMarkupParseContext *context,
                    const gchar         *passthrough_text, /* text is not 0-terminated. */
                    gsize                text_len,
                    gpointer             user_data,
                    GError             **error)
{
  // PData *pdata = user_data;
}

static void             /* callback for errors, including ones set by other methods in the vtable */
radget_error (GMarkupParseContext *context,
              GError              *error,  /* the GError should not be freed */
              gpointer             user_data)
{
  // PData *pdata = user_data;
}

static void
radget_parser (Domain      *domain,
               const gchar *i18n_domain,
               gint         fd,
               const gchar *text,
               gint         length,
               GError     **error)
{
  static GMarkupParser parser = {
    radget_start_element,
    radget_end_element,
    radget_text,
    radget_passthrough,
    radget_error,
  };
  PData pbuf = { 0, }, *pdata = &pbuf;
  GMarkupParseContext *context = g_markup_parse_context_new (&parser, 0, pdata, NULL);
  guint8 bspace[1024];
  const gchar *buffer = text ? text : (const gchar*) bspace;
  pdata->domain = domain;
  pdata->i18n_domain = i18n_domain;
  if (!text)
    length = read (fd, bspace, 1024);
  while (length > 0)
    {
      if (!g_markup_parse_context_parse (context, buffer, length, error))
        break;
      if (!text)
        length = read (fd, bspace, 1024);
      else
        length = 0;
    }
  if (length < 0)
    set_error (error, "failed to read radget file: %s", g_strerror (errno));
  if (!*error)
    g_markup_parse_context_end_parse (context, error);
  g_markup_parse_context_free (context);
}

static GData *domains = NULL;

/**
 * @param domain_name	radget domain name
 * @param file_name	file containing ragdet definitions
 * @param i18n_domain	i18n domain to translate labels
 * @param error	        GError location
 *
 * Parse radget definitions from @a file_name. See gxk_radget_create() and
 * gxk_radget_complete() to make use of the definitions.
 */
void
gxk_radget_parse (const gchar    *domain_name,
                  const gchar    *file_name,
                  const gchar    *i18n_domain,
                  GError        **error)
{
  Domain *domain;
  GError *myerror = NULL;
  gint fd = open (file_name, O_RDONLY, 0);
  domain = domain_name ? g_datalist_get_data (&domains, domain_name) : standard_domain;
  if (!domain)
    {
      domain = g_new0 (Domain, 1);
      domain->domain = g_intern_string (domain_name);
      g_datalist_set_data (&domains, domain_name, domain);
    }
  radget_parser (domain, i18n_domain, fd, NULL, 0, error ? error : &myerror);
  close (fd);
  if (myerror)
    {
      g_warning ("GxkRadget: parsing error in \"%s\": %s", file_name, myerror->message);
      g_error_free (myerror);
    }
}

/**
 * @param domain_name	radget domain name
 * @param text	radget definition string
 * @param text_len	length of @a text or -1
 * @param i18n_domain	i18n domain to translate labels
 * @param error	GError location
 *
 * Parse radget definitions from @a text. See gxk_radget_create() and
 * gxk_radget_complete() to make use of the definitions.
 */
void
gxk_radget_parse_text (const gchar    *domain_name,
                       const gchar    *text,
                       gint            text_len,
                       const gchar    *i18n_domain,
                       GError        **error)
{
  Domain *domain;
  GError *myerror = NULL;
  g_return_if_fail (text != NULL);
  domain = domain_name ? g_datalist_get_data (&domains, domain_name) : standard_domain;
  if (!domain)
    {
      domain = g_new0 (Domain, 1);
      domain->domain = g_intern_string (domain_name);
      g_datalist_set_data (&domains, domain_name, domain);
    }
  radget_parser (domain, i18n_domain, -1, text, text_len < 0 ? strlen (text) : text_len, error ? error : &myerror);
  if (myerror)
    {
      g_warning ("GxkRadget: parsing error: %s", myerror->message);
      g_error_free (myerror);
    }
}

static GtkSizeGroup*
toplevel_get_size_group (GtkWidget   *toplevel,
                         const gchar *name,
                         gchar        type)
{
  gchar *key = g_strdup_printf ("gxk-toplevel-%cgroup#%s", type, name);
  GtkSizeGroup *sg = g_object_get_data (toplevel, key);
  if (!sg)
    {
      sg = gtk_size_group_new (type == 'h' ? GTK_SIZE_GROUP_HORIZONTAL :
                               type == 'v' ? GTK_SIZE_GROUP_VERTICAL :
                               GTK_SIZE_GROUP_BOTH);
      g_object_set_data_full (toplevel, key, sg, g_object_unref);
    }
  g_free (key);
  return sg;
}

static void
radget_widget_hierarchy_changed (GtkWidget *widget,
                                 GtkWidget *previous_toplevel)
{
  if (previous_toplevel)
    return;
  const gchar *hgroup = g_object_get_data (widget, "gxk-window-hgroup");
  const gchar *vgroup = g_object_get_data (widget, "gxk-window-vgroup");
  const gchar *bgroup = g_object_get_data (widget, "gxk-window-hvgroup");
  if (!hgroup && !vgroup && !bgroup)
    return;
  GtkWidget *toplevel = gtk_widget_get_toplevel (widget);
  g_assert (GTK_WIDGET_TOPLEVEL (toplevel));
  if (hgroup)
    {
      GtkSizeGroup *sg = toplevel_get_size_group (toplevel, hgroup, 'h');
      gtk_size_group_add_widget (sg, widget);
      g_object_set_data (widget, "gxk-window-hgroup", NULL);    /* gtk_size_group_add_widget() <= 2.4.4 may not be called twice */
    }
  if (vgroup)
    {
      GtkSizeGroup *sg = toplevel_get_size_group (toplevel, vgroup, 'v');
      gtk_size_group_add_widget (sg, widget);
      g_object_set_data (widget, "gxk-window-vgroup", NULL);    /* gtk_size_group_add_widget() <= 2.4.4 may not be called twice */
    }
  if (bgroup)
    {
      GtkSizeGroup *sg = toplevel_get_size_group (toplevel, bgroup, 'b');
      gtk_size_group_add_widget (sg, widget);
      g_object_set_data (widget, "gxk-window-hvgroup", NULL);   /* gtk_size_group_add_widget() <= 2.4.4 may not be called twice */
    }
}

static void
property_value_from_string (GtkType      widget_type,
                            GParamSpec  *pspec,
                            GValue      *value,
                            const gchar *pname,
                            const gchar *pvalue,
                            Env         *env,
                            GError     **error)
{
  GType vtype = G_PARAM_SPEC_VALUE_TYPE (pspec);
  gint edefault = 0;
  gchar *exvalue;
  if (G_IS_PARAM_SPEC_ENUM (pspec))
    edefault = G_PARAM_SPEC_ENUM (pspec)->default_value;
  else if (g_type_is_a (widget_type, GTK_TYPE_TEXT_TAG) &&
           strcmp (pname, "weight") == 0)
    {
      /* special case GtkTextTag::weight which is an enum really */
      vtype = PANGO_TYPE_WEIGHT;
      edefault = G_PARAM_SPEC_INT (pspec)->default_value;
    }
  env->skip_property = FALSE;
  exvalue = expand_expr (pvalue, env);
  if (env->skip_property)
    {
      env->skip_property = FALSE;
      g_free (exvalue);
      return;
    }
  switch (G_TYPE_FUNDAMENTAL (vtype))
    {
      GEnumClass *eclass;
      GFlagsClass *fclass;
      gdouble v_float;
    case G_TYPE_BOOLEAN:
      g_value_init (value, G_TYPE_BOOLEAN);
      g_value_set_boolean (value, boolean_from_string (exvalue));
      break;
    case G_TYPE_STRING:
      g_value_init (value, G_TYPE_STRING);
      g_value_set_string (value, exvalue);
      break;
    case G_TYPE_INT:
    case G_TYPE_UINT:
    case G_TYPE_LONG:
    case G_TYPE_ULONG:
      g_value_init (value, G_TYPE_FUNDAMENTAL (vtype));
      v_float = float_from_string (exvalue);
      v_float = v_float > 0 ? v_float + 0.5 : v_float - 0.5;
      switch (G_TYPE_FUNDAMENTAL (vtype))
        {
        case G_TYPE_INT:        g_value_set_int (value, v_float); break;
        case G_TYPE_UINT:       g_value_set_uint (value, v_float); break;
        case G_TYPE_LONG:       g_value_set_long (value, v_float); break;
        case G_TYPE_ULONG:      g_value_set_ulong (value, v_float); break;
        }
      break;
    case G_TYPE_FLOAT:
    case G_TYPE_DOUBLE:
      g_value_init (value, G_TYPE_DOUBLE);
      g_value_set_double (value, float_from_string (exvalue));
      break;
    case G_TYPE_ENUM:
      eclass = g_type_class_peek (vtype);
      if (eclass)
        {
          g_value_init (value, vtype);
          g_value_set_enum (value, enums_match_value (eclass->n_values, eclass->values, exvalue, edefault));
        }
      break;
    case G_TYPE_FLAGS:
      fclass = g_type_class_peek (vtype);
      if (fclass && exvalue)
        {
          gchar **fnames = g_strsplit (exvalue, "|", -1);
          guint i, v = 0;
          g_value_init (value, vtype);
          for (i = 0; fnames[i]; i++)
            v |= enums_match_value (fclass->n_values, (GEnumValue*) fclass->values, fnames[i], 0);
          g_value_set_flags (value, v);
          g_strfreev (fnames);
        }
      break;
    default:
      set_error (error, "unsupported property: %s::%s", g_type_name (widget_type), pname);
      break;
    }
  if (0 && G_VALUE_TYPE (value) && strchr (pvalue, '$'))
    g_print ("property[%s]: expr=%s result=%s GValue=%s\n", pspec->name, pvalue, exvalue, g_strdup_value_contents (value));
  g_free (exvalue);
}

static GxkRadgetArgs*
merge_args_list (GxkRadgetArgs *args,
                 GSList        *call_args)
{
  if (call_args)
    {
      if (call_args->next)
        args = merge_args_list (args, call_args->next);
      args = gxk_radget_args_merge (args, call_args->data);
    }
  return args;
}

static GxkRadgetArgs*
node_expand_call_args (Node   *node,
                       GSList *call_args,
                       Env    *env)
{
  /* precedence for value lookups:
   * x for call_args intra lookups are _not_ possible
   * - xdef scope_args
   * - xdef call_args
   * - xdef ancestry call_args (dependant on parent level)
   * $name is special cased in the value lookup function
   */
  guint i, n_pops = 0;
  /* flatten call args */
  GxkRadgetArgs *args = gxk_radget_args (NULL);
  args = merge_args_list (args, call_args);
  args = gxk_radget_args_merge (args, node->call_args);
  /* prepare for $name lookups */
  env->name = node->name;
  /* push args lists according to precedence */
  if (node->xdef_node)
    {
      guint n = node->xdef_node->depth;
      GSList *polist, *slist = NULL;
      for (polist = node->xdef_node->parent_arg_list; n > node->xdef_depth; n--, polist = polist->next)
        slist = g_slist_prepend (slist, polist->data);
      while (slist)     /* two times prepending keeps original order */
        {
          GxkRadgetArgs *pargs = g_slist_pop_head (&slist);
          n_pops++, env->args_list = g_slist_prepend (env->args_list, pargs);
        }
    }
  if (node->xdef_node && node->xdef_node->call_stack)
    n_pops++, env->args_list = g_slist_prepend (env->args_list, node->xdef_node->call_stack->data);
  if (node->xdef_node && node->xdef_node->scope_args)
    n_pops++, env->args_list = g_slist_prepend (env->args_list, node->xdef_node->scope_args);
  /* expand args */
  for (i = 0; i < ARGS_N_ENTRIES (args); i++)
    {
      gchar *value = ARGS_NTH_VALUE (args, i);
      if (value && strchr (value, '$'))
        {
          gchar *exval = env_expand_args_value (env, args, i);
          ARGS_NTH_VALUE (args, i) = exval;
          g_free (value);
        }
    }
  /* cleanup */
  while (n_pops--)
    g_slist_pop_head (&env->args_list);
  return args;
}

struct GxkRadgetData {
  Node         *node;
  GxkRadgetArgs *call_stack_top;
  GxkRadget    *xdef_radget;
  Env          *env;
};

static GxkRadget*
radget_create_from_node (Node         *node,
                         GxkRadget    *radget,
                         Env          *env,
                         GError      **error)
{
  GxkRadgetType tinfo = { 0, };
  guint i, n_pops = 0;
  /* prepare for $id/$name lookups */
  env->name = node->name;
  /* retrive type info */
  if (!gxk_radget_type_lookup (node->type, &tinfo))
    g_error ("invalid radget type: %s", g_type_name (node->type));
  /* precedence for property value lookups:
   * - all node ancestry args
   * - expanded call_args
   */
  if (node->call_stack)
    n_pops++, env->args_list = g_slist_prepend (env->args_list, node->call_stack->data);
  if (node->parent_arg_list)
    n_pops += g_slist_length (node->parent_arg_list), env->args_list = g_slist_concat (g_slist_copy (node->parent_arg_list),
                                                                                       env->args_list);
  /* create radget */
  GTypeClass *klass = g_type_class_ref (node->type);
  if (!radget)
    {
      GParameter *cparams = g_alloca (sizeof (*cparams) * ARGS_N_ENTRIES (node->prop_args));
      memset (cparams, 0, sizeof (*cparams) * ARGS_N_ENTRIES (node->prop_args));
      guint n_cparams = 0;
      /* construct properties */
      for (i = 0; i < ARGS_N_ENTRIES (node->prop_args); i++)
        {
          const gchar *pname = ARGS_NTH_NAME (node->prop_args, i);
          const gchar *pvalue = ARGS_NTH_VALUE (node->prop_args, i);
          GParamSpec *pspec = tinfo.find_prop (klass, pname);
          if (pspec && (pspec->flags & (G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY)))
            {
              guint j = n_cparams++;
              cparams[j].name = pspec->name;
              property_value_from_string (node->type, pspec, &cparams[j].value, pname, pvalue, env, error);
              if (!G_VALUE_TYPE (&cparams[j].value))
                n_cparams--;
            }
        }
      GxkRadgetData gdgdata;
      gdgdata.node = node;
      gdgdata.call_stack_top = node->call_stack->data;
      gdgdata.xdef_radget = env->xdef_radget;
      gdgdata.env = env;
      radget = tinfo.create (node->type, node->name, n_cparams, cparams, &gdgdata);
      for (i = 0; i < n_cparams; i++)
        g_value_unset (&cparams[i].value);
    }
  g_object_set_qdata (radget, quark_radget_node, node);
  /* keep global xdef_radget for gdg_data */
  if (!env->xdef_radget)
    env->xdef_radget = radget;
  /* widget specific patchups (size-groups) */
  if (node->size_hgroup)
    gtk_size_group_add_widget (env_get_size_group (env, node->size_hgroup, 'h'), radget);
  if (node->size_vgroup)
    gtk_size_group_add_widget (env_get_size_group (env, node->size_vgroup, 'v'), radget);
  if (node->size_hvgroup)
    gtk_size_group_add_widget (env_get_size_group (env, node->size_hvgroup, 'b'), radget);
  g_object_set_data (radget, "gxk-window-hgroup", (gchar*) node->size_window_hgroup);
  g_object_set_data (radget, "gxk-window-vgroup", (gchar*) node->size_window_vgroup);
  g_object_set_data (radget, "gxk-window-hvgroup", (gchar*) node->size_window_hvgroup);
  /* set properties */
  for (i = 0; i < ARGS_N_ENTRIES (node->prop_args); i++)
    {
      const gchar *pname = ARGS_NTH_NAME (node->prop_args, i);
      const gchar *pvalue = ARGS_NTH_VALUE (node->prop_args, i);
      GParamSpec *pspec = tinfo.find_prop (klass, pname);
      if (pspec && !(pspec->flags & (G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY)))
        {
          GValue value = { 0 };
          property_value_from_string (node->type, pspec, &value, pname, pvalue, env, error);
          if (G_VALUE_TYPE (&value))
            {
              tinfo.set_prop (radget, pname, &value);
              g_value_unset (&value);
            }
        }
      else if (!pspec)
        set_error (error, "radget \"%s\" has no property: %s", node->name, pname);
    }
  /* hierarchy-changed setup */
  if (GTK_IS_WIDGET (radget) &&
      !gxk_signal_handler_exists (radget, "hierarchy-changed", G_CALLBACK (radget_widget_hierarchy_changed), NULL))
    {
      g_signal_connect_after (radget, "hierarchy-changed", G_CALLBACK (radget_widget_hierarchy_changed), NULL);
      /* check anchored */
      if (GTK_WIDGET_TOPLEVEL (gtk_widget_get_toplevel (radget)))
        radget_widget_hierarchy_changed (radget, NULL);
    }
  /* cleanup */
  g_type_class_unref (klass);
  while (n_pops--)
    g_slist_pop_head (&env->args_list);
  return radget;
}

static void
radget_add_to_parent (GxkRadget    *parent,
                      GxkRadget    *radget,
                      Env          *env,
                      GError      **error)
{
  Node *pnode = g_object_get_qdata (parent, quark_radget_node);
  Node *cnode = g_object_get_qdata (radget, quark_radget_node);
  GxkRadgetType tinfo = { 0, };
  guint i, needs_packing, n_pops = 0;
  /* prepare for $name lookups */
  env->name = cnode->name;
  /* retrive type info */
  gxk_radget_type_lookup (cnode->type, &tinfo);
  /* precedence for property value lookups:
   * - all node ancestry args
   * - expanded call_args
   */
  if (cnode->call_stack)
    n_pops++, env->args_list = g_slist_prepend (env->args_list, cnode->call_stack->data);
  if (cnode->parent_arg_list)
    n_pops += g_slist_length (cnode->parent_arg_list), env->args_list = g_slist_concat (g_slist_copy (cnode->parent_arg_list),
                                                                                        env->args_list);
  /* perform set_parent() */
  {
    GxkRadgetData gdgdata;
    gdgdata.node = cnode;
    gdgdata.call_stack_top = cnode->call_stack->data;
    gdgdata.xdef_radget = env->xdef_radget;
    gdgdata.env = env;
    needs_packing = tinfo.adopt (radget, parent, &gdgdata);
  }
  /* construct set of pack args and apply */
  if (needs_packing)
    {
      GxkRadgetArgs *args = gxk_radget_args_merge (gxk_radget_const_args (),
                                                   pnode ? pnode->dfpk_args : NULL);
      args = gxk_radget_args_merge (args, cnode->pack_args);
      /* set pack args */
      for (i = 0; i < ARGS_N_ENTRIES (args); i++)
        {
          const gchar *pname = ARGS_NTH_NAME (args, i);
          const gchar *pvalue = ARGS_NTH_VALUE (args, i);
          GParamSpec *pspec = tinfo.find_pack (radget, pname);
          if (pspec)
            {
              GValue value = { 0 };
              property_value_from_string (0, pspec, &value, pname, pvalue, env, error);
              if (G_VALUE_TYPE (&value))
                {
                  tinfo.set_pack (radget, pname, &value);
                  g_value_unset (&value);
                }
            }
          else
            g_printerr ("GXK: no such pack property: %s,%s,%s\n", G_OBJECT_TYPE_NAME (parent), G_OBJECT_TYPE_NAME (radget), pname);
        }
      gxk_radget_free_args (args);
    }
  /* cleanup */
  while (n_pops--)
    g_slist_pop_head (&env->args_list);
}

static void
radget_apply_hooks (GxkRadget    *radget,
                    Env          *env,
                    GError      **error)
{
  Node *cnode = g_object_get_qdata (radget, quark_radget_node);
  GxkRadgetType tinfo;
  guint i, n_pops = 0;
  /* prepare for $name lookups */
  env->name = cnode->name;
  /* retrive type info */
  gxk_radget_type_lookup (cnode->type, &tinfo);
  /* precedence for property value lookups:
   * - all node ancestry args
   * - expanded call_args
   */
  if (cnode->call_stack)
    n_pops++, env->args_list = g_slist_prepend (env->args_list, cnode->call_stack->data);
  if (cnode->parent_arg_list)
    n_pops += g_slist_length (cnode->parent_arg_list), env->args_list = g_slist_concat (g_slist_copy (cnode->parent_arg_list),
                                                                                        env->args_list);
  /* set hook args */
  const GxkRadgetArgs *args = cnode->hook_args;
  for (i = 0; i < ARGS_N_ENTRIES (args); i++)
    {
      const gchar *hname = ARGS_NTH_NAME (args, i);
      const gchar *hvalue = ARGS_NTH_VALUE (args, i);
      GxkRadgetHook hook_func;
      GParamSpec *pspec = find_hook (hname, &hook_func);
      if (pspec)
        {
          GValue value = { 0 };
          property_value_from_string (0, pspec, &value, hname, hvalue, env, error);
          if (G_VALUE_TYPE (&value))
            {
              /* we always assume G_PARAM_LAX_VALIDATION */
              g_param_value_validate (pspec, &value);
              hook_func (radget, pspec->param_id, &value, pspec);
              g_value_unset (&value);
            }
        }
      else
        g_printerr ("GXK: no such hook property: %s (radget=%s)\n", hname, G_OBJECT_TYPE_NAME (radget));
    }
  /* cleanup */
  while (n_pops--)
    g_slist_pop_head (&env->args_list);
}

static void
radget_create_children (GxkRadget    *parent,
                        Env          *env,
                        GError      **error)
{
  Node *pnode = g_object_get_qdata (parent, quark_radget_node);
  GSList *slist;
  /* create children */
  for (slist = pnode->children; slist; slist = slist->next)
    {
      Node *cnode = slist->data;
      GxkRadget *radget;
      /* node_expand_call_args() sets env->name */
      GxkRadgetArgs *call_args = node_expand_call_args (cnode, NULL, env);
      cnode->call_stack = g_slist_prepend (cnode->call_stack, call_args);
      /* create child */
      radget = radget_create_from_node (cnode, NULL, env, error);
      if (cnode->children)
        radget_create_children (radget, env, error);
      radget_add_to_parent (parent, radget, env, error);
      radget_apply_hooks (radget, env, error);
      g_slist_pop_head (&cnode->call_stack);
      gxk_radget_free_args (call_args);
    }
}

static GxkRadget*
radget_creator (GxkRadget          *radget,
                const gchar        *domain_name,
                const gchar        *name,
                GxkRadget          *parent,
                GSList             *user_args,
                GSList             *env_args)
{
  Domain *domain = g_datalist_get_data (&domains, domain_name);
  if (domain)
    {
      Node *node = g_datalist_get_data (&domain->nodes, name);
      if (node)
        {
          GxkRadgetArgs *call_args;
          Env env = { NULL, };
          GError *error = NULL;
          guint n_pops = 0;
          if (env_args)
            {
              n_pops += g_slist_length (node->parent_arg_list);
              env.args_list = g_slist_concat (g_slist_copy (env_args), env.args_list);
            }
          call_args = node_expand_call_args (node, user_args, &env);
          n_pops++, node->call_stack = g_slist_prepend (node->call_stack, call_args);
          if (radget && !g_type_is_a (G_OBJECT_TYPE (radget), node->type))
            g_warning ("GxkRadget: radget domain \"%s\": radget `%s' differs from defined type: %s",
                       domain_name, G_OBJECT_TYPE_NAME (radget), node->name);
          else
            {
              radget = radget_create_from_node (node, radget, &env, &error);
              radget_create_children (radget, &env, &error);
            }
          if (parent && radget)
            radget_add_to_parent (parent, radget, &env, &error);
          if (radget)
            radget_apply_hooks (radget, &env, &error);
          /* cleanup */
          while (n_pops--)
            g_slist_pop_head (&env.args_list);
          gxk_radget_free_args (call_args);
          env_clear (&env);
          if (error)
            g_warning ("GxkRadget: while constructing radget \"%s\": %s", node->name, error->message);
          g_clear_error (&error);
        }
      else
        g_warning ("GxkRadget: radget domain \"%s\": no such node: %s", domain_name, name);
    }
  else
    g_warning ("GxkRadget: no such radget domain: %s", domain_name);
  return radget;
}

/* --- radget args --- */
GxkRadgetArgs*
gxk_radget_data_copy_call_args (GxkRadgetData *gdgdata)
{
  GxkRadgetArgs *args;
  GSList *olist = NULL;
  olist = g_slist_copy (gdgdata->node->parent_arg_list);
  olist = g_slist_prepend (olist, gdgdata->call_stack_top);
  args = node_expand_call_args (gdgdata->node, olist, gdgdata->env);
  g_slist_free (olist);
  return args;
}

GxkRadgetArgs*
gxk_radget_const_args (void)
{
  GxkRadgetArgs *args = g_new0 (GxkRadgetArgs, 1);
  args->intern_quarks = TRUE;
  return args;
}

GxkRadgetArgs*
gxk_radget_args_valist (const gchar        *name1,
                        va_list             var_args)
{
  GxkRadgetArgs *args = g_new0 (GxkRadgetArgs, 1);
  const gchar *name = name1;
  while (name)
    {
      const gchar *value = va_arg (var_args, const gchar*);
      args = gxk_radget_args_set (args, name, value);
      name = va_arg (var_args, const gchar*);
    }
  return args;
}

GxkRadgetArgs*
gxk_radget_args (const gchar *name1,
                 ...)
{
  GxkRadgetArgs *args;
  va_list vargs;
  va_start (vargs, name1);
  args = gxk_radget_args_valist (name1, vargs);
  va_end (vargs);
  return args;
}

GxkRadgetArgs*
gxk_radget_args_set (GxkRadgetArgs  *args,
                     const gchar    *name,
                     const gchar    *value)
{
  GQuark quark = g_quark_from_string (name);
  guint i;
  g_return_val_if_fail (name != NULL, args);
  if (!args)
    args = gxk_radget_args (NULL);
  for (i = 0; i < ARGS_N_ENTRIES (args); i++)
    if (quark == args->quarks[i])
      break;
  if (i >= ARGS_N_ENTRIES (args))
    {
      i = args->n_variables++;
      args->quarks = g_renew (GQuark, args->quarks, ARGS_N_ENTRIES (args));
      args->values = g_renew (gchar*, args->values, ARGS_N_ENTRIES (args));
      args->quarks[i] = quark;
    }
  else if (!args->intern_quarks)
    g_free (args->values[i]);
  if (args->intern_quarks)
    args->values[i] = (gchar*) g_intern_string (value);
  else
    args->values[i] = g_strdup (value);
  return args;
}

static const gchar*
radget_args_lookup_quark (const GxkRadgetArgs *args,
                          GQuark               quark,
                          guint               *nthp)
{
  guint i;
  for (i = 0; i < ARGS_N_ENTRIES (args); i++)
    if (quark == args->quarks[i])
      {
        if (nthp)
          *nthp = i;
        return ARGS_NTH_VALUE (args, i);
      }
  return NULL;
}

const gchar*
gxk_radget_args_get (const GxkRadgetArgs *args,
                     const gchar         *name)
{
  GQuark quark = g_quark_try_string (name);
  if (args && quark)
    return radget_args_lookup_quark (args, quark, NULL);
  return NULL;
}

GxkRadgetArgs*
gxk_radget_args_merge (GxkRadgetArgs       *args,
                       const GxkRadgetArgs *source)
{
  if (source)
    {
      guint i;
      if (!args)
        args = gxk_radget_args (NULL);
      for (i = 0; i < ARGS_N_ENTRIES (source); i++)
        gxk_radget_args_set (args, ARGS_NTH_NAME (source, i), ARGS_NTH_VALUE (source, i));
    }
  return args;
}

void
gxk_radget_free_args (GxkRadgetArgs *args)
{
  if (args)
    {
      guint i;
      if (!args->intern_quarks)
        for (i = 0; i < ARGS_N_ENTRIES (args); i++)
          g_free (args->values[i]);
      g_free (args->values);
      g_free (args->quarks);
      g_free (args);
    }
}

/* --- radget functions --- */
GxkRadget*
gxk_radget_data_get_scope_radget (GxkRadgetData *gdgdata)
{
  return gdgdata->xdef_radget;
}

gchar*
gxk_radget_data_dup_expand (GxkRadgetData       *gdgdata,
                            const gchar         *expression)
{
  gboolean skip_property = gdgdata->env->skip_property;
  gchar *string = expand_expr (expression, gdgdata->env);
  gdgdata->env->skip_property = skip_property;
  return string;
}

GxkRadget*
gxk_radget_creator (GxkRadget          *radget,
                    const gchar        *domain_name,
                    const gchar        *name,
                    GxkRadget          *parent,
                    GSList             *call_args,
                    GSList             *env_args)
{
  g_return_val_if_fail (domain_name != NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);
  if (radget)
    {
      Node *radget_node = g_object_get_qdata (radget, quark_radget_node);
      g_return_val_if_fail (radget_node == NULL, NULL);
    }
  return radget_creator (radget, domain_name, name, parent, call_args, env_args);
}

/**
 * @param domain_name	radget domain
 * @param name	        radget definition name
 * @param ...           NULL terminated list of variable (name, value) strings pairs
 *
 * Create the radget (GtkWidget or GObject) defined as @a name within @a domain,
 * using the variable bindings as defined in the @a ... paired string list.
 */
GxkRadget*
gxk_radget_create (const gchar        *domain_name,
                   const gchar        *name,
                   const gchar        *var1,
                   ...)
{
  GxkRadgetArgs *gargs;
  GxkRadget *radget;
  GSList olist = { 0, };
  va_list vargs;
  va_start (vargs, var1);
  gargs = gxk_radget_args_valist (var1, vargs);
  olist.data = gargs;
  radget = gxk_radget_creator (NULL, domain_name, name, NULL, &olist, NULL);
  gxk_radget_free_args (gargs);
  va_end (vargs);
  return radget;
}

/**
 * @param radget       toplevel ragdet container
 * @param domain_name  radget domain
 * @param name	       radget definition name
 * @param ...          NULL terminated list of variable (name, value) strings pairs
 *
 * Create the children/contents of the radget defined under @a name within @a domain
 * as part of the container object passed in as @a radget,
 * using the variable bindings as defined in the @a ... paired string list.
 */
GxkRadget*
gxk_radget_complete (GxkRadget          *radget,
                     const gchar        *domain_name,
                     const gchar        *name,
                     const gchar        *var1,
                     ...)
{
  GxkRadgetArgs *gargs;
  GSList olist = { 0, };
  va_list vargs;
  va_start (vargs, var1);
  gargs = gxk_radget_args_valist (var1, vargs);
  olist.data = gargs;
  radget = gxk_radget_creator (radget, domain_name, name, NULL, &olist, NULL);
  gxk_radget_free_args (gargs);
  va_end (vargs);
  return radget;
}

/**
 * @param radget	a valid radget
 * @return		radget domain
 *
 * Return the domain within which the definition was found @a radget
 * was created from.
 */
const gchar*
gxk_radget_get_domain (GxkRadget *radget)
{
  Node *radget_node = g_object_get_qdata (radget, quark_radget_node);
  g_return_val_if_fail (radget_node != NULL, NULL);
  return radget_node->domain;
}

void
gxk_radget_sensitize (GxkRadget      *radget,
                      const gchar    *name,
                      gboolean        sensitive)
{
  GtkWidget *widget = gxk_radget_find (radget, name);
  if (GTK_IS_WIDGET (widget))
    {
      /* special guard for menu items */
      if (sensitive && GTK_IS_MENU_ITEM (widget))
        {
          GtkMenuItem *mitem = GTK_MENU_ITEM (widget);
          if (mitem && mitem->submenu)
            sensitive = gxk_menu_check_sensitive (GTK_MENU (mitem->submenu));
        }
      gtk_widget_set_sensitive (widget, sensitive);
    }
}

/**
 * @param radget	a valid radget
 * @param name	radget name
 * @return		radget named @a name or NULL
 *
 * Recursively find the radget named @a name within the container radget @a radget.
 * The @a name may consist of a list of parent radget names, seperated by a dot '.'.
 */
gpointer
gxk_radget_find (GxkRadget      *radget,
                 const gchar    *name)
{
  const gchar *next, *c = name;
  
  g_return_val_if_fail (radget != NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);
  
  if (!GTK_IS_WIDGET (radget))
    return NULL;
  
  next = strchr (c, '.');
  while (radget && next)
    {
      gchar *name = g_strndup (c, next - c);
      c = next + 1;
      radget = gxk_widget_find_level_ordered (radget, name);
      g_free (name);
    }
  if (radget)
    radget = gxk_widget_find_level_ordered (radget, c);
  return radget;
}

/**
 * @param radget	a valid radget
 * @param area	radget name
 * @return		radget named @a name or NULL
 *
 * Recursively find the radget named @a name within the container radget @a radget.
 */
gpointer
gxk_radget_find_area (GxkRadget      *radget,
                      const gchar    *area)
{
  Node *node;
  radget = area ? gxk_radget_find (radget, area) : radget;
  if (!GTK_IS_WIDGET (radget))
    return NULL;
  node = g_object_get_qdata (radget, quark_radget_node);
  while (node && node->default_area)
    {
      radget = gxk_widget_find_level_ordered (radget, node->default_area);
      node = radget ? g_object_get_qdata (radget, quark_radget_node) : NULL;
    }
  return radget;
}

/**
 * @param radget	a valid radget
 * @param area	radget name
 * @param widget	valid GtkWidget
 *
 * Add the unparanted widget @a widget to @a radget within area @a area.
 */
void
gxk_radget_add (GxkRadget      *radget,
                const gchar    *area,
                gpointer        widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  radget = gxk_radget_find_area (radget, area);
  if (GTK_IS_CONTAINER (radget))
    gtk_container_add (radget, widget);
  else
    g_error ("GxkRadget: failed to find area \"%s\"", area);
}


/* --- radget types --- */
static void
radget_define_type (GType           type,
                    const gchar    *name,
                    const gchar   **attribute_names,
                    const gchar   **attribute_values,
                    const gchar    *i18n_domain)
{
  GError *error = NULL;
  Node *node;
  node = node_define (standard_domain, name, type, NULL, NULL,
                      attribute_names, attribute_values,
                      i18n_domain, NULL, NULL, NULL, &error);
  g_datalist_set_data (&standard_domain->nodes, name, node);
  if (error)
    g_error ("while registering standard radgets: %s", error->message);
}

void
gxk_init_radget_types (void)
{
  GType types[1024], *t = types;
  g_assert (quark_radget_type == 0);
  quark_id = g_quark_from_static_string ("id");
  quark_name = g_quark_from_static_string ("name");
  quark_radget_type = g_quark_from_static_string ("GxkRadget-type");
  quark_radget_node = g_quark_from_static_string ("GxkRadget-node");
  standard_domain = g_new0 (Domain, 1);
  standard_domain->domain = g_intern_string ("standard");
  g_datalist_set_data (&domains, standard_domain->domain, standard_domain);
  *t++ = GTK_TYPE_WINDOW;       *t++ = GTK_TYPE_ARROW;          *t++ = GTK_TYPE_SCROLLED_WINDOW;
  *t++ = GTK_TYPE_VIEWPORT;     *t++ = GTK_TYPE_HRULER;         *t++ = GTK_TYPE_VRULER;
  *t++ = GTK_TYPE_TABLE;        *t++ = GTK_TYPE_FRAME;          *t++ = GTK_TYPE_ALIGNMENT;
  *t++ = GXK_TYPE_NOTEBOOK;     *t++ = GTK_TYPE_BUTTON;         *t++ = GTK_TYPE_MENU_BAR;
  *t++ = GTK_TYPE_TREE_VIEW;    *t++ = GTK_TYPE_LABEL;          *t++ = GTK_TYPE_PROGRESS_BAR;
  *t++ = GTK_TYPE_HPANED;       *t++ = GTK_TYPE_VPANED;         *t++ = GTK_TYPE_SPIN_BUTTON;
  *t++ = GTK_TYPE_EVENT_BOX;    *t++ = GTK_TYPE_IMAGE;          *t++ = GTK_TYPE_OPTION_MENU;
  *t++ = GTK_TYPE_HBOX;         *t++ = GTK_TYPE_VBOX;           *t++ = GXK_TYPE_MENU_BUTTON;
  *t++ = GTK_TYPE_CHECK_BUTTON; *t++ = GTK_TYPE_ENTRY;          *t++ = GXK_TYPE_MENU_ITEM;
  *t++ = GTK_TYPE_HSCROLLBAR;   *t++ = GTK_TYPE_HSCALE;         *t++ = GTK_TYPE_TEAROFF_MENU_ITEM;
  *t++ = GTK_TYPE_VSCROLLBAR;   *t++ = GTK_TYPE_VSCALE;         *t++ = GXK_TYPE_FREE_RADIO_BUTTON;
  *t++ = GTK_TYPE_VSEPARATOR;   *t++ = GXK_TYPE_SIMPLE_LABEL;   *t++ = GTK_TYPE_HSEPARATOR;
  *t++ = GTK_TYPE_HWRAP_BOX;    *t++ = GTK_TYPE_VWRAP_BOX;      *t++ = GTK_TYPE_HANDLE_BOX;
  *t++ = GXK_TYPE_IMAGE;        *t++ = GXK_TYPE_BACK_SHADE;     *t++ = GXK_TYPE_SCROLLED_WINDOW;
  *t++ = GXK_TYPE_RACK_TABLE;   *t++ = GXK_TYPE_RACK_ITEM;
  while (t-- > types)
    gxk_radget_define_widget_type (*t);
  radget_define_gtk_menu ();
  gxk_radget_define_type (GXK_TYPE_RADGET_FACTORY, gxk_radget_factory_def);
  gxk_radget_define_type (GXK_TYPE_FACTORY_BRANCH, gxk_factory_branch_def);
  gxk_radget_define_type (GXK_TYPE_WIDGET_PATCHER, gxk_widget_patcher_def);
  gxk_radget_register_hook (g_param_spec_string ("gxk-adopt-hint", NULL, NULL, NULL, G_PARAM_READWRITE), 1, gxk_adopt_hint_hook);
}

gboolean
gxk_radget_type_lookup (GType           type,
                        GxkRadgetType  *ggtype)
{
  GxkRadgetType *tdata = g_type_get_qdata (type, quark_radget_type);
  if (tdata)
    {
      *ggtype = *tdata;
      return TRUE;
    }
  return FALSE;
}

void
gxk_radget_define_type (GType                type,
                        const GxkRadgetType *ggtype)
{
  const gchar *attribute_names[1] = { NULL };
  const gchar *attribute_values[1] = { NULL };
  
  g_return_if_fail (!G_TYPE_IS_ABSTRACT (type));
  g_return_if_fail (G_TYPE_IS_OBJECT (type));
  g_return_if_fail (g_type_get_qdata (type, quark_radget_type) == NULL);
  
  g_type_set_qdata (type, quark_radget_type, (gpointer) ggtype);
  radget_define_type (type, g_type_name (type), attribute_names, attribute_values, NULL);
}


/* --- widget types --- */
static GParamSpec*
widget_find_prop (GTypeClass   *klass,
                  const gchar  *construct_param_name)
{
  return g_object_class_find_property (G_OBJECT_CLASS (klass), construct_param_name);
}

static GxkRadget*
widget_create (GType               type,
               const gchar        *name,
               guint               n_construct_params,
               GParameter         *construct_params,
               GxkRadgetData      *gdgdata)
{
  GtkWidget *widget = g_object_newv (type, n_construct_params, construct_params);
  g_object_set (widget, "name", name, NULL);
  return widget;
}

static void
gxk_adopt_hint_hook (GxkRadget           *radget,
                     guint                property_id,
                     const GValue        *value,
                     GParamSpec          *pspec)
{
  g_object_set_data_full (radget, "gxk-adopt-hint", g_value_dup_string (value), g_free);
}

static gboolean
widget_adopt (GxkRadget          *radget,
              GxkRadget          *parent,
              GxkRadgetData      *gdgdata)
{
  gboolean need_adding = TRUE;
  gchar *str = gxk_radget_data_dup_expand (gdgdata, "$gxk-adopt-prop");
  if (str && str[0])
    {
      GxkRadgetType tinfo;
      if (gxk_radget_type_lookup (G_OBJECT_TYPE (parent), &tinfo))
        {
          GParamSpec *pspec = tinfo.find_prop ((GTypeClass*) G_OBJECT_GET_CLASS (parent), str);
          if (pspec && g_type_is_a (G_OBJECT_TYPE (radget), G_PARAM_SPEC_VALUE_TYPE (pspec)))
            {
              GValue value = { 0, };
              g_value_init (&value, G_OBJECT_TYPE (radget));
              g_value_set_object (&value, radget);
              tinfo.set_prop (parent, str, &value);
              g_value_unset (&value);
              need_adding = FALSE;
            }
        }
    }
  g_free (str);
  if (need_adding)
    gtk_container_add (GTK_CONTAINER (parent), GTK_WIDGET (radget));
  return TRUE;
}

static GParamSpec*
widget_find_pack (GxkRadget    *radget,
                  const gchar  *pack_name)
{
  GtkWidget *parent = GTK_WIDGET (radget)->parent;
  return gtk_container_class_find_child_property (G_OBJECT_GET_CLASS (parent), pack_name);
}

static void
widget_set_pack (GxkRadget    *radget,
                 const gchar  *pack_name,
                 const GValue *value)
{
  GtkWidget *parent = GTK_WIDGET (radget)->parent;
  gtk_container_child_set_property (GTK_CONTAINER (parent), radget, pack_name, value);
}

void
gxk_radget_define_widget_type (GType type)
{
  static const GxkRadgetType widget_info = {
    widget_find_prop,
    widget_create,
    (void(*)(GxkRadget*,const gchar*,const GValue*)) g_object_set_property,
    widget_adopt,
    widget_find_pack,
    widget_set_pack,
  };
  static const struct { const gchar *name, *value; } widget_def[] = {
    {   "prop:visible",         "$(first-occupied,$visible,1)" },
    {   "prop:sensitive",       "$(first-occupied,$sensitive,1)" },
    {   "prop:width-request",   "$(first-occupied,$width,0)" },
    {   "prop:height-request",  "$(first-occupied,$height,0)" },
    {   "prop:events",          "$events" },
    {   "prop:can-focus",       "$(first-occupied,$can-focus,$focus,$(skip-property))" },
    {   "prop:has-focus",       "$(first-occupied,$has-focus,$focus,0)" },
    {   "prop:can-default",     "$(first-occupied,$can-default,$default,0)" },
    {   "prop:has-default",     "$(first-occupied,$has-default,$default,0)" },
    {   "prop:receives-default","$(first-occupied,$receives-default,0)" },
    {   "prop:extension-events","$extension-events" },
    // gtk+-2.4: {   "prop:no-show-all",     "$(first-occupied,$no-show-all,$hidden,0)" },
  };
  static const struct { const gchar *name, *value; } container_def[] = {
    {   "prop:border-width",    "$(first-occupied,$border-width,0)" },
  };
  const gchar *attribute_names[G_N_ELEMENTS (widget_def) + G_N_ELEMENTS (container_def) + 1];
  const gchar *attribute_values[G_N_ELEMENTS (widget_def) + G_N_ELEMENTS (container_def) + 1];
  guint i, j = 0;
  
  g_return_if_fail (!G_TYPE_IS_ABSTRACT (type));
  g_return_if_fail (g_type_is_a (type, GTK_TYPE_WIDGET));
  g_return_if_fail (g_type_get_qdata (type, quark_radget_type) == NULL);
  
  g_type_set_qdata (type, quark_radget_type, (gpointer) &widget_info);
  for (i = 0; i < G_N_ELEMENTS (widget_def); i++)
    {
      attribute_names[j] = widget_def[i].name;
      attribute_values[j] = widget_def[i].value;
      j++;
    }
  if (g_type_is_a (type, GTK_TYPE_CONTAINER))
    for (i = 0; i < G_N_ELEMENTS (container_def); i++)
      {
        attribute_names[j] = container_def[i].name;
        attribute_values[j] = container_def[i].value;
        j++;
      }
  attribute_names[j] = NULL;
  attribute_values[j] = NULL;
  radget_define_type (type, g_type_name (type), attribute_names, attribute_values, NULL);
}

static gboolean
menu_adopt (GxkRadget          *radget,
            GxkRadget          *parent,
            GxkRadgetData      *gdgdata)
{
  if (GTK_IS_MENU_ITEM (parent))
    gxk_menu_attach_as_submenu (radget, parent);
  else if (GTK_IS_OPTION_MENU (parent))
    gtk_option_menu_set_menu (parent, radget);
  else if (GXK_IS_MENU_BUTTON (parent))
    g_object_set (parent, "menu", radget, NULL);
  else
    gxk_menu_attach_as_popup (radget, parent);
  return TRUE;
}

static void* return_NULL (void) { return NULL; }

static void
radget_define_gtk_menu (void)
{
  static const GxkRadgetType widget_info = {
    widget_find_prop,
    widget_create,
    (void(*)(GxkRadget*,const gchar*,const GValue*)) g_object_set_property,
    menu_adopt,
    (void*) return_NULL,/* find_pack */
    NULL,               /* set_pack */
  };
  const gchar *attribute_names[2] = { NULL, NULL };
  const gchar *attribute_values[2] = { NULL, NULL };
  GType type = GTK_TYPE_MENU;
  g_type_set_qdata (type, quark_radget_type, (gpointer) &widget_info);
  attribute_names[0] = "prop:visible";
  attribute_values[0] = "$(ifdef,visible,$visible,1)";
  radget_define_type (type, g_type_name (type), attribute_names, attribute_values, NULL);
}

/* --- radget hooks --- */
typedef struct RadgetHook RadgetHook;
struct RadgetHook {
  GParamSpec          *pspec;
  GxkRadgetHook        hook_func;
  RadgetHook          *next;
};
static RadgetHook *radget_hooks = NULL;

void
gxk_radget_register_hook (GParamSpec   *pspec,
                          guint         property_id,
                          GxkRadgetHook hook_func)
{
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  g_return_if_fail (pspec->flags & G_PARAM_WRITABLE);
  g_return_if_fail (property_id > 0);
  g_return_if_fail (pspec->param_id == 0);
  g_return_if_fail (pspec->owner_type == 0);
  g_return_if_fail (hook_func != NULL);
  if (find_hook (pspec->name, NULL))
    {
      g_printerr ("GXK: not re-registering hook property: %s\n", pspec->name);
      return;
    }
  RadgetHook *hook = g_new0 (RadgetHook, 1);
  hook->pspec = g_param_spec_ref (pspec);
  g_param_spec_sink (pspec);
  g_quark_from_static_string (pspec->name);
  hook->pspec->param_id = property_id;
  hook->pspec->owner_type = G_TYPE_OBJECT; /* common base for all radgets */
  hook->hook_func = hook_func;
  hook->next = radget_hooks;
  radget_hooks = hook;
}

static GParamSpec*
find_hook (const gchar   *name,
           GxkRadgetHook *hook_func_p)
{
  RadgetHook *hook = radget_hooks;
  while (hook && strcmp (hook->pspec->name, name) != 0)
    hook = hook->next;
  if (hook && hook_func_p)
    *hook_func_p = hook->hook_func;
  return hook ? hook->pspec : NULL;
}

/* --- macro functions --- */
static inline const gchar*
argiter_pop (GSList **slist_p)
{
  gpointer d = NULL;
  if (*slist_p)
    {
      d = (*slist_p)->data;
      *slist_p = (*slist_p)->next;
    }
  return d;
}

static inline gchar*
argiter_exp (GSList **slist_p,
             Env     *env)
{
  const gchar *s = argiter_pop (slist_p);
  return s ? expand_expr (s, env) : NULL;
}

static gchar*
mf_if (GSList *args,
       Env    *env)
{
  GSList      *argiter = args->next; /* skip func name */
  gchar       *cond = argiter_exp (&argiter, env);
  const gchar *then = argiter_pop (&argiter);
  const gchar *elze = argiter_pop (&argiter);
  gboolean b = boolean_from_string (cond);
  g_free (cond);
  if (b)
    return then ? expand_expr (then, env) : g_strdup ("1");
  else
    return elze ? expand_expr (elze, env) : g_strdup ("0");
}

static gchar*
mf_not (GSList *args,
        Env    *env)
{
  GSList      *argiter = args->next; /* skip func name */
  gchar       *cond = argiter_exp (&argiter, env);
  gboolean b = boolean_from_string (cond);
  g_free (cond);
  return g_strdup (b ? "0" : "1");
}

static gchar*
mf_blogic (GSList *args,
           Env    *env)
{
  GSList      *argiter = args;
  const gchar *name = argiter_pop (&argiter);
  gboolean result = strcmp (name, "and") == 0 || strcmp (name, "nand") == 0;
  while (argiter)
    {
      gchar *vbool = argiter_exp (&argiter, env);
      gboolean b = boolean_from_string (vbool);
      g_free (vbool);
      if (strcmp (name, "xor") == 0)
        result ^= b;
      else if (strcmp (name, "or") == 0 || strcmp (name, "nor") == 0)
        result |= b;
      else if (strcmp (name, "and") == 0 || strcmp (name, "nand") == 0)
        result &= b;
    }
  if (strcmp (name, "nor") == 0 || strcmp (name, "nand") == 0)
    result = !result;
  return g_strdup (result ? "1" : "0");
}

static inline int
null_strcmp (const gchar *s1,
             const gchar *s2)
{
  if (!s1 || !s2)
    return s2 ? -1 : s1 != NULL;
  else
    return strcmp (s1, s2);
}

static gchar*
mf_str_cmp (GSList *args,
            Env    *env)
{
  GSList      *argiter = args;
  const gchar *name = argiter_pop (&argiter);
  gchar *arg = argiter_exp (&argiter, env);
  gchar *last = arg ? arg : g_strdup ("");
  gboolean result = TRUE;
  arg = argiter_exp (&argiter, env);
  while (arg)
    {
      gboolean match = FALSE;
      if (strcmp (name, "strlt") == 0)
        match = null_strcmp (last, arg) < 0;
      else if (strcmp (name, "strle") == 0)
        match = null_strcmp (last, arg) <= 0;
      else if (strcmp (name, "strgt") == 0)
        match = null_strcmp (last, arg) > 0;
      else if (strcmp (name, "strge") == 0)
        match = null_strcmp (last, arg) >= 0;
      else if (strcmp (name, "strne") == 0)
        match = null_strcmp (last, arg) != 0;
      else if (strcmp (name, "streq") == 0)
        match = null_strcmp (last, arg) == 0;
      g_free (last);
      last = arg;
      if (!match)
        {
          result = match;
          break;
        }
      arg = argiter_exp (&argiter, env);
    }
  g_free (last);
  return g_strdup (result ? "1" : "0");
}

#define EQ_FLAG 0x80

static gchar*
mf_floatcmp (GSList *args,
             Env    *env)
{
  GSList      *argiter = args;
  const gchar *name = argiter_pop (&argiter);
  gchar *arg = argiter_exp (&argiter, env);
  double last = arg ? float_from_string (arg) : 0;
  gboolean result = TRUE;
  g_free (arg);
  arg = argiter_exp (&argiter, env);
  while (arg)
    {
      gboolean match = FALSE;
      double v = float_from_string (arg);
      g_free (arg);
      if (strcmp (name, "lt") == 0)
        match = last < v;
      else if (strcmp (name, "le") == 0)
        match = last <= v;
      else if (strcmp (name, "gt") == 0)
        match = last > v;
      else if (strcmp (name, "ge") == 0)
        match = last >= v;
      else if (strcmp (name, "ne") == 0)
        match = last != v;
      else if (strcmp (name, "eq") == 0)
        match = last == v;
      if (!match)
        {
          result = match;
          break;
        }
      last = v;
      arg = argiter_exp (&argiter, env);
    }
  return g_strdup (result ? "1" : "0");
}

static gchar*
mf_floatcollect (GSList *args,
                 Env    *env)
{
  GSList      *argiter = args;
  const gchar *name = argiter_pop (&argiter);
  gchar *arg = argiter_exp (&argiter, env);
  double last = arg ? float_from_string (arg) : 0;
  guint n = arg ? 1 : 0;
  double accu = last;
  g_free (arg);
  arg = argiter_exp (&argiter, env);
  while (arg)
    {
      double v = float_from_string (arg);
      g_free (arg);
      if (strcmp (name, "max") == 0)
        accu = MAX (accu, v);
      else if (strcmp (name, "min") == 0)
        accu = MIN (accu, v);
      else if (strcmp (name, "avg") == 0)
        accu += v;
      else if (strcmp (name, "sum") == 0)
        accu += v;
      last = v;
      n++;
      arg = argiter_exp (&argiter, env);
    }
  if (strcmp (name, "avg") == 0 && n)
    accu /= n;
  return g_strdup_printf ("%.17g", accu);
}

static gchar*
mf_first_occupied (GSList *args,
                   Env    *env)
{
  GSList      *argiter = args->next; /* skip func name */
  gchar       *name = argiter_exp (&argiter, env);
  while (name && !name[0])
    {
      g_free (name);
      name = argiter_exp (&argiter, env);
    }
  return name ? name : g_strdup ("");
}

static gchar*
mf_ifdef (GSList *args,
          Env    *env)
{
  GSList      *argiter = args->next; /* skip func name */
  gchar       *name = argiter_exp (&argiter, env);
  const gchar *then = argiter_pop (&argiter);
  const gchar *elze = argiter_pop (&argiter);
  gboolean b = env_lookup (env, name) != NULL;
  g_free (name);
  if (b)
    return then ? expand_expr (then, env) : g_strdup ("1");
  else
    return elze ? expand_expr (elze, env) : g_strdup ("0");
}

static gchar*
mf_nth (GSList *args,
        Env    *env)
{
  GSList *argiter = args->next; /* skip func name */
  gchar  *num = argiter_exp (&argiter, env);
  guint i = num_from_string (num);
  const gchar *d = g_slist_nth_data (args->next, i);
  g_free (num);
  return d ? expand_expr (d, env) : NULL;
}

static gchar*
mf_null_collapse (GSList *args,
                  Env    *env)
{
  GSList *argiter = args->next; /* skip func name */
  gchar  *value = argiter_exp (&argiter, env);
  if ((!value || value[0] == 0) && env->specials)
    env->specials->null_collapse = TRUE;
  return value;
}

static gchar*
mf_skip_property (GSList *args,
                  Env    *env)
{
  env->skip_property = TRUE;
  return g_strdup ("");
}

static gchar*
mf_empty (GSList *args,
          Env    *env)
{
  return g_strdup ("");
}

static gchar*
mf_println (GSList *args,
            Env    *env)
{
  GSList      *argiter = args->next; /* skip func name */
  while (argiter)
    {
      gchar *arg = argiter_exp (&argiter, env);
      g_print ("%s", arg ? arg : "(null)");
      g_free (arg);
    }
  g_print ("\n");
  return NULL;
}

static MacroFunc
macro_func_lookup (const gchar *name)
{
  static const struct { const gchar *name; MacroFunc mfunc; } macros[] = {
    { "if",             mf_if, },
    { "not",            mf_not, },
    { "xor",            mf_blogic, },
    { "or",             mf_blogic, },
    { "nor",            mf_blogic, },
    { "and",            mf_blogic, },
    { "nand",           mf_blogic, },
    { "lt",             mf_floatcmp, },
    { "le",             mf_floatcmp, },
    { "gt",             mf_floatcmp, },
    { "ge",             mf_floatcmp, },
    { "ne",             mf_floatcmp, },
    { "eq",             mf_floatcmp, },
    { "min",            mf_floatcollect, },
    { "max",            mf_floatcollect, },
    { "sum",            mf_floatcollect, },
    { "avg",            mf_floatcollect, },
    { "nth",            mf_nth, },
    { "strlt",          mf_str_cmp, },
    { "strle",          mf_str_cmp, },
    { "strgt",          mf_str_cmp, },
    { "strge",          mf_str_cmp, },
    { "strne",          mf_str_cmp, },
    { "streq",          mf_str_cmp, },
    { "ifdef",          mf_ifdef, },
    { "first-occupied", mf_first_occupied, },
    { "null-collapse",  mf_null_collapse, },
    { "skip-property",  mf_skip_property, },
    { "empty",          mf_empty, },
    { "println",        mf_println, },
  };
  guint i;
  for (i = 0; i < G_N_ELEMENTS (macros); i++)
    if (strcmp (name, macros[i].name) == 0)
      return macros[i].mfunc;
  return mf_empty;
}
