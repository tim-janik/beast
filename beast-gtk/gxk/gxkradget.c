/* GXK - Gtk+ Extension Kit
 * Copyright (C) 2002-2003 Tim Janik
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
#include "gxkgadget.h"
#include "gxkgadgetfactory.h"
#include "gxkauxwidgets.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define NODE(n)         ((Node*) n)

struct _GxkGadgetOpt {
  guint         n_variables;
  gboolean      intern_quarks;
  const gchar **names;
  gchar       **values;
};
#define OPTIONS_N_ENTRIES(o)    ((o) ? (o)->n_variables : 0)
#define OPTIONS_NTH_NAME(o,n)   ((o)->names[(n)])
#define OPTIONS_NTH_VALUE(o,n)  ((o)->values[(n)])

typedef struct {
  guint null_collapse : 1;
} EnvSpecials;

typedef struct {
  GSList       *option_list; /* GxkGadgetOpt* */
  const gchar  *name;
  GxkGadgetOpt *options;
  EnvSpecials  *specials;
  GData        *hgroups, *vgroups, *hvgroups;
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
  GxkGadgetOpt *call_options;
  GxkGadgetOpt *prop_options;
  GxkGadgetOpt *pack_options;
  GxkGadgetOpt *dfpk_options;
  const gchar  *size_hgroup;
  const gchar  *size_vgroup;
  const gchar  *size_hvgroup;
  Node         *default_child;
  GSList       *children; /* Node* */
};
typedef struct {
  GData *nodes;
  const gchar *domain;
} Domain;
typedef struct {
  Domain *domain;
  guint   tag_opened;
  GSList *node_stack; /* Node* */
  const gchar *default_area;
  Node        *default_child;
} PData;                /* parser state */
typedef gchar* (*MacroFunc)     (GSList *args,
                                 Env    *env);


/* --- prototypes --- */
static gchar*           expand_expr                     (const gchar    *expr,
                                                         Env            *env);
static MacroFunc        macro_func_lookup               (const gchar    *name);
static inline gboolean  boolean_from_string             (const gchar    *value);
static inline guint64   num_from_string                 (const gchar    *value);
static void             gadget_define_gtk_menu          (void);
static void             gadget_create_children_from_node(Node           *node,
                                                         GxkGadget      *parent,
                                                         Env            *env,
                                                         GError        **error);


/* --- variables --- */
static Domain *standard_domain = NULL;
static GQuark  quark_gadget_type = 0;
static GQuark  quark_gadget_node = 0;


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

static Node*
clone_node_intern (Node        *source,
                   const gchar *domain,
                   const gchar *name,
                   CloneList   *clist)
{
  Node *node = g_new0 (Node, 1);
  GSList *slist, *last = NULL;
  guint i = 0;
  node->domain = domain;
  node->name = g_intern_string (name);
  node->type = source->type;
  node->call_options = gxk_gadget_options_copy (source->call_options);
  node->prop_options = gxk_gadget_options_copy (source->prop_options);
  node->pack_options = gxk_gadget_options_copy (source->pack_options);
  node->dfpk_options = gxk_gadget_options_copy (source->dfpk_options);
  node->size_hgroup = source->size_hgroup;
  node->size_vgroup = source->size_vgroup;
  node->size_hvgroup = source->size_hvgroup;
  if (source->default_child)
    {
      i = clist->n_clones++;
      clist->clones = g_renew (NodeClone, clist->clones, clist->n_clones);
      clist->clones[i].source = source->default_child;
      clist->clones[i].clone = NULL;
    }
  for (slist = source->children; slist; slist = slist->next)
    {
      Node *child = slist->data;
      child = clone_node_intern (child, domain, child->name, clist);
      if (last)
        {
          last->next = g_slist_new (child);
          last = last->next;
        }
      else
        node->children = last = g_slist_new (child);
    }
  if (source->default_child)
    {
      node->default_child = clist->clones[i].clone;
      clist->n_clones--;
      clist->clones = g_renew (NodeClone, clist->clones, clist->n_clones);
    }
  for (i = 0; i < clist->n_clones; i++)
    if (source == clist->clones[i].source)
      clist->clones[i].clone = node;
  return node;
}

static Node*
clone_node (Node        *source,
            const gchar *domain,
            const gchar *name)
{
  CloneList clist = { 0, NULL };
  Node *node = clone_node_intern (source, domain, name, &clist);
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

static inline guint64
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

static const gchar*
env_lookup (Env         *env,
            const gchar *var)
{
  const gchar *cval = gxk_gadget_options_get (env->options, var);
  guint l = cval ? 0 : strlen (var);
  GSList *slist;
  if (l == 4 && strcmp (var, "name") == 0)
    cval = env->name;
  for (slist = env->option_list; !cval && slist; slist = slist->next)
    {
      GxkGadgetOpt *opt = slist->data;
      cval = gxk_gadget_options_get (opt, var);
    }
  return cval;
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
    g_printerr ("malformed formula: $(%s", start);
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
  if (*c == '(')
    return parse_formula (c + 1, result, env);
  if (*c == '$')
    {
      g_string_append_c (result, '$');
      return c + 1;
    }
  if (strchr (ident_start, *c))
    {
      const gchar *cval;
      gchar *var;
      c++;
      while (*c && strchr (ident_chars, *c))
        c++;
      var = g_strndup (mark, c - mark);
      cval = env_lookup (env, var);
      g_free (var);
      if (cval)
        {
          gchar *exval = expand_expr (cval, env);
          g_string_append (result, exval);
          g_free (exval);
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
  EnvSpecials *ospecials = env->specials;
  EnvSpecials specials = { 0, };
  env->specials = &specials;
  while (dollar)
    {
      g_string_append_len (result, c, dollar - c);
      c = parse_dollar (dollar + 1, result, env);
      dollar = strchr (c, '$');
    }
  g_string_append (result, c);
  env->specials = ospecials;
  if (specials.null_collapse && !result->str[0])
    return g_string_free (result, TRUE);
  else
    return g_string_free (result, FALSE);
}

static Node*
node_children_find_area (Node        *node,
                         const gchar *area)
{
  GSList *slist;
  for (slist = node->children; slist; slist = slist->next)
    {
      if (strcmp (NODE (slist->data)->name, area) == 0)
        return slist->data;
    }
  return NULL;
}

static Node*
node_find_area (Node        *node,
                const gchar *area)
{
  Node *child;
  const gchar *p;
  if (!area || strcmp (area, "default") == 0)
    {
      child = node->default_child;
      if (child)
        return node_find_area (child, NULL);
      else
        return node;
    }
  p = strchr (area, '.');
  if (p)
    {
      gchar *str = g_strndup (area, p - area);
      if (strcmp (str, "default") == 0)
        child = node->default_child;
      else
        child = node_children_find_area (node, str);
      g_free (str);
      if (child)
        return node_find_area (child, p + 1);
      else
        return node_find_area (node, NULL);
    }
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

static GxkGadgetOpt*
gadget_options_intern_set (GxkGadgetOpt   *opt,
                           const gchar    *name,
                           const gchar    *value)
{
  if (!opt)
    opt = gxk_gadget_const_options ();
  return gxk_gadget_options_set (opt, name, value);
}

static Node*
node_define (Domain       *domain,
             const gchar  *node_name,
             GType         direct_type,
             Node         *source,
             const gchar **attribute_names,
             const gchar **attribute_values,
             const gchar **name_p,
             const gchar **area_p,
             const gchar **default_area_p,
             GError       **error)
{
  Node *node = NULL;
  gboolean allow_defs = !source;
  guint i;
  /* inherit from gadget/type */
  if (direct_type)
    {
      node = g_new0 (Node, 1);
      node->domain = domain->domain;
      node->name = g_intern_string (node_name);
      node->type = direct_type;
    }
  else if (source)
    {
      node = clone_node (source, domain->domain, node_name);
    }
  else for (i = 0; attribute_names[i]; i++)
    if (!node && strcmp (attribute_names[i], "inherit") == 0)
      {
        source = node_lookup (domain, attribute_values[i]);
        if (source)
          node = clone_node (source, domain->domain, node_name);
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
    set_error (error, "no gadget type specified in definition of: %s", node_name);
  if (*error)
    return NULL;
  allow_defs = TRUE; // FIXME: hack?
  /* apply property attributes */
  for (i = 0; attribute_names[i]; i++)
    if (strncmp (attribute_names[i], "pack:", 5) == 0)
      node->pack_options = gadget_options_intern_set (node->pack_options, attribute_names[i] + 5, attribute_values[i]);
    else if (strncmp (attribute_names[i], "default-pack:", 13) == 0)
      node->dfpk_options = gadget_options_intern_set (node->dfpk_options, attribute_names[i] + 13, attribute_values[i]);
    else if (strcmp (attribute_names[i], "name") == 0 || strcmp (attribute_names[i], "_name") == 0)
      {
        if (name_p && !*name_p)
          *name_p = g_intern_string (attribute_values[i]);
      }
    else if (allow_defs && strncmp (attribute_names[i], "prop:", 5) == 0)
      node->prop_options = gadget_options_intern_set (node->prop_options, attribute_names[i] + 5, attribute_values[i]);
    else if (strcmp (attribute_names[i], "size:hgroup") == 0 && g_type_is_a (node->type, GTK_TYPE_WIDGET))
      node->size_hgroup = g_intern_string (attribute_values[i]);
    else if (strcmp (attribute_names[i], "size:vgroup") == 0 && g_type_is_a (node->type, GTK_TYPE_WIDGET))
      node->size_vgroup = g_intern_string (attribute_values[i]);
    else if (strcmp (attribute_names[i], "size:hvgroup") == 0 && g_type_is_a (node->type, GTK_TYPE_WIDGET))
      node->size_hvgroup = g_intern_string (attribute_values[i]);
    else if (strcmp (attribute_names[i], "inherit") == 0 ||
             strcmp (attribute_names[i], "default-area") == 0 ||
             strcmp (attribute_names[i], "area") == 0)
      ; /* handled above */
    else if (strchr (attribute_names[i], ':'))
      set_error (error, "invalid attribute \"%s\" in definition of: %s", attribute_names[i], node_name);
    else
      {
        const gchar *name = attribute_names[i];
        const gchar *value = attribute_values[i];
        if (name[0] == '_') /* i18n hook */
          {
            name++;
            value = dgettext (NULL, value);
          }
        node->call_options = gadget_options_intern_set (node->call_options, name, value);
      }
  if (!g_type_is_a (node->type, G_TYPE_OBJECT))
    set_error (error, "no gadget type specified in definition of: %s", node_name);
  return node;
}

static void             /* callback for open tags <foo bar="baz"> */
gadget_start_element  (GMarkupParseContext *context,
                       const gchar         *element_name,
                       const gchar        **attribute_names,
                       const gchar        **attribute_values,
                       gpointer             user_data,
                       GError             **error)
{
  PData *pdata = user_data;
  Node *child;
  if (!pdata->tag_opened && strcmp (element_name, "gxk-gadget-definitions") == 0)
    {
      /* toplevel tag */
      pdata->tag_opened = TRUE;
    }
  else if (pdata->node_stack == NULL && strncmp (element_name, "xdef:", 5) == 0)
    {
      const gchar *name = element_name + 5;
      if (g_datalist_get_data (&pdata->domain->nodes, name))
        set_error (error, "redefinition of gadget: %s", name);
      else
        {
          Node *node = node_define (pdata->domain, name, 0, NULL, attribute_names, attribute_values,
                                    NULL, NULL, &pdata->default_area, error);
          if (node)
            {
              g_datalist_set_data (&pdata->domain->nodes, name, node);
              pdata->node_stack = g_slist_prepend (pdata->node_stack, node);
            }
        }
    }
  else if (pdata->node_stack && (child = node_lookup (pdata->domain, element_name), child))
    {
      Node *parent = pdata->node_stack->data;
      const gchar *area = NULL, *uname = NULL;
      Node *node = node_define (pdata->domain, element_name, 0, child, attribute_names, attribute_values,
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
        set_error (error, "failed to define gadget: %s", element_name);
    }
  else
    set_error (error, "unknown element: %s", element_name);
}

static void             /* callback for close tags </foo> */
gadget_end_element (GMarkupParseContext *context,
                    const gchar         *element_name,
                    gpointer             user_data,
                    GError             **error)
{
  PData *pdata = user_data;
  if (strcmp (element_name, "gxk-gadget-definitions") == 0)
    {
      /* toplevel tag closed */
      pdata->tag_opened = FALSE;
    }
  else if (pdata->node_stack && pdata->node_stack->next == NULL && strncmp (element_name, "xdef:", 5) == 0)
    {
      Node *node = g_slist_pop_head (&pdata->node_stack);
      if (pdata->default_child)
        node->default_child = pdata->default_child;
      pdata->default_child = NULL;
      pdata->default_area = NULL;
    }
  else if (pdata->node_stack && pdata->node_stack->next)
    {
      Node *node = g_slist_pop_head (&pdata->node_stack);
      if (pdata->default_area && strcmp (node->name, pdata->default_area) == 0)
        pdata->default_child = node;
    }
}

static void             /* callback for character data */
gadget_text (GMarkupParseContext *context,
             const gchar         *text,    /* text is not 0-terminated */
             gsize                text_len,
             gpointer             user_data,
             GError             **error)
{
  // PData *pdata = user_data;
}

static void             /* callback for comments and processing instructions */
gadget_passthrough (GMarkupParseContext *context,
                    const gchar         *passthrough_text, /* text is not 0-terminated. */
                    gsize                text_len,
                    gpointer             user_data,
                    GError             **error)
{
  // PData *pdata = user_data;
}

static void             /* callback for errors, including ones set by other methods in the vtable */
gadget_error (GMarkupParseContext *context,
              GError              *error,  /* the GError should not be freed */
              gpointer             user_data)
{
  // PData *pdata = user_data;
}

static void
gadget_parser (Domain      *domain,
               gint         fd,
               const gchar *text,
               gint         length,
               GError     **error)
{
  static GMarkupParser parser = {
    gadget_start_element,
    gadget_end_element,
    gadget_text,
    gadget_passthrough,
    gadget_error,
  };
  PData pbuf = { 0, }, *pdata = &pbuf;
  GMarkupParseContext *context = g_markup_parse_context_new (&parser, 0, pdata, NULL);
  guint8 bspace[1024];
  const gchar *buffer = text ? text : (const gchar*) bspace;
  pdata->domain = domain;
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
    set_error (error, "failed to read gadget file: %s", g_strerror (errno));
  if (!*error)
    g_markup_parse_context_end_parse (context, error);
  g_markup_parse_context_free (context);
}

static GData *domains = NULL;

void
gxk_gadget_parse (const gchar    *domain_name,
                  const gchar    *file_name,
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
  gadget_parser (domain, fd, NULL, 0, error ? error : &myerror);
  close (fd);
  if (myerror)
    {
      g_warning ("GxkGadget: while parsing \"%s\": %s", file_name, myerror->message);
      g_error_free (myerror);
    }
}

void
gxk_gadget_parse_text (const gchar    *domain_name,
                       const gchar    *text,
                       gint            text_len,
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
  gadget_parser (domain, -1, text, text_len < 0 ? strlen (text) : text_len, error ? error : &myerror);
  if (myerror)
    {
      g_warning ("GxkGadget: while parsing: %s", myerror->message);
      g_error_free (myerror);
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
  exvalue = expand_expr (pvalue, env);
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

static GxkGadget*
gadget_create_from_node (Node      *node,
                         GxkGadget *gadget,
                         gboolean   allow_children,
                         Env       *env,
                         GError   **error)
{
  GxkGadgetType tinfo;
  guint i;
  if (!gxk_gadget_type_lookup (node->type, &tinfo))
    g_error ("invalid gadget type: %s", g_type_name (node->type));
  env->name = node->name;
  if (!gadget)
    gadget = tinfo.create (node->type, node->name);
  g_object_set_qdata (gadget, quark_gadget_node, node);
  if (node->size_hgroup)
    gtk_size_group_add_widget (env_get_size_group (env, node->size_hgroup, 'h'), gadget);
  if (node->size_vgroup)
    gtk_size_group_add_widget (env_get_size_group (env, node->size_vgroup, 'v'), gadget);
  if (node->size_hvgroup)
    gtk_size_group_add_widget (env_get_size_group (env, node->size_hvgroup, 'b'), gadget);
  for (i = 0; i < OPTIONS_N_ENTRIES (node->prop_options); i++)
    {
      const gchar *pname = OPTIONS_NTH_NAME (node->prop_options, i);
      const gchar *pvalue = OPTIONS_NTH_VALUE (node->prop_options, i);
      GParamSpec *pspec = tinfo.find_prop (gadget, pname);
      if (pspec)
        {
          GValue value = { 0 };
          property_value_from_string (node->type, pspec, &value, pname, pvalue, env, error);
          if (G_VALUE_TYPE (&value))
            {
              tinfo.set_prop (gadget, pname, &value);
              g_value_unset (&value);
            }
        }
      else
        set_error (error, "gadget \"%s\" has no property: %s", node->name, pname);
    }
  if (node->children && allow_children)
    gadget_create_children_from_node (node, gadget, env, error);
  return gadget;
}

static void
gadget_add_to_parent (GxkGadget *parent,
                      GxkGadget *gadget,
                      Env       *env,
                      GError   **error)
{
  Node *pnode = g_object_get_qdata (parent, quark_gadget_node);
  Node *cnode = g_object_get_qdata (gadget, quark_gadget_node);
  GxkGadgetType tinfo;
  guint i;
  env->name = cnode->name;
  if (cnode->call_options)
    env->option_list = g_slist_prepend (env->option_list, cnode->call_options);
  gxk_gadget_type_lookup (cnode->type, &tinfo);
  tinfo.adopt (gadget, parent);                                 /* set_parent() */
  for (i = 0; i < (pnode ? OPTIONS_N_ENTRIES (pnode->dfpk_options) : 0); i++)
    {
      const gchar *pname = OPTIONS_NTH_NAME (pnode->dfpk_options, i);
      const gchar *pvalue = OPTIONS_NTH_VALUE (pnode->dfpk_options, i);
      GParamSpec *pspec = tinfo.find_pack (gadget, pname);
      if (pspec)
        {
          GValue value = { 0 };
          property_value_from_string (0, pspec, &value, pname, pvalue, env, error);
          if (G_VALUE_TYPE (&value))
            {
              tinfo.set_pack (gadget, pname, &value);
              g_value_unset (&value);
            }
        }
    }
  for (i = 0; i < OPTIONS_N_ENTRIES (cnode->pack_options); i++)
    {
      const gchar *pname = OPTIONS_NTH_NAME (cnode->pack_options, i);
      const gchar *pvalue = OPTIONS_NTH_VALUE (cnode->pack_options, i);
      GParamSpec *pspec = tinfo.find_pack (gadget, pname);
      if (pspec)
        {
          GValue value = { 0 };
          property_value_from_string (0, pspec, &value, pname, pvalue, env, error);
          if (G_VALUE_TYPE (&value))
            {
              tinfo.set_pack (gadget, pname, &value);
              g_value_unset (&value);
            }
        }
      else
        g_printerr ("GXK: no such pack property: %s,%s,%s\n", G_OBJECT_TYPE_NAME (parent), G_OBJECT_TYPE_NAME (gadget), pname);
    }
  if (cnode->call_options)
    g_slist_pop_head (&env->option_list);
}

static void
gadget_create_children_from_node (Node      *pnode,
                                  GxkGadget *parent,
                                  Env       *env,
                                  GError   **error)
{
  Node *dcnode = g_object_get_qdata (parent, quark_gadget_node);
  GSList *slist;
  env->name = pnode->name;
  if (!dcnode)
    dcnode = pnode;
  for (slist = pnode->children; slist; slist = slist->next)
    {
      Node *cnode = slist->data;
      GxkGadget *gadget;
      if (cnode->call_options)
        env->option_list = g_slist_prepend (env->option_list, cnode->call_options);
      gadget = gadget_create_from_node (cnode, NULL, FALSE, env, error);     /* env->name = cnode->name; */
      gadget_add_to_parent (parent, gadget, env, error);
      if (cnode->children)
        gadget_create_children_from_node (cnode, gadget, env, error);
      if (cnode->call_options)
        g_slist_pop_head (&env->option_list);
    }
}

static GxkGadget*
gadget_creator (const gchar    *domain_name,
                const gchar    *name,
                GxkGadgetOpt   *options,
                GxkGadget      *parent,
                GxkGadget      *gadget)
{
  Domain *domain = g_datalist_get_data (&domains, domain_name);
  if (domain)
    {
      Node *node = g_datalist_get_data (&domain->nodes, name);
      if (node)
        {
          Env env = { NULL, };
          GError *error = NULL;
          env.options = options;
          if (node->call_options)
            env.option_list = g_slist_prepend (env.option_list, node->call_options);
          if (gadget && !g_type_is_a (G_OBJECT_TYPE (gadget), node->type))
            g_warning ("GxkGadget: gadget domain \"%s\": gadget `%s' differs from defined type: %s",
                       domain_name, G_OBJECT_TYPE_NAME (gadget), node->name);
          else
            gadget = gadget_create_from_node (node, gadget, TRUE, &env, &error);
          if (parent && gadget)
            gadget_add_to_parent (parent, gadget, &env, &error);
          if (node->call_options)
            g_slist_pop_head (&env.option_list);
          env_clear (&env);
          if (error)
            g_warning ("GxkGadget: while constructing gadget \"%s\": %s", node->name, error->message);
          g_clear_error (&error);
        }
      else
        g_warning ("GxkGadget: gadget domain \"%s\": no such node: %s", domain_name, name);
    }
  else
    g_warning ("GxkGadget: no such gadget domain: %s", domain_name);
  return gadget;
}

GxkGadget*
gxk_gadget_create_add (const gchar    *domain_name,
                       const gchar    *name,
                       GxkGadget      *parent,
                       GxkGadgetOpt   *options)
{
  g_return_val_if_fail (domain_name != NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (parent != NULL, NULL);
  return gadget_creator (domain_name, name, options, parent, NULL);
}

GxkGadget*
gxk_gadget_create (const gchar    *domain_name,
                   const gchar    *name,
                   GxkGadgetOpt   *options)
{
  g_return_val_if_fail (domain_name != NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);
  return gadget_creator (domain_name, name, options, NULL, NULL);
}

GxkGadget*
gxk_gadget_complete (GxkGadget      *gadget,
                     const gchar    *domain_name,
                     const gchar    *name,
                     GxkGadgetOpt   *options)
{
  Node *gadget_node = g_object_get_qdata (gadget, quark_gadget_node);
  g_return_val_if_fail (domain_name != NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (gadget_node == NULL, NULL);
  return gadget_creator (domain_name, name, options, NULL, gadget);
}

GxkGadgetOpt*
gxk_gadget_const_options (void)
{
  GxkGadgetOpt *opt = g_new0 (GxkGadgetOpt, 1);
  opt->intern_quarks = TRUE;
  return opt;
}

GxkGadgetOpt*
gxk_gadget_options (const gchar *name1,
                    ...)
{
  GxkGadgetOpt *opt = g_new0 (GxkGadgetOpt, 1);
  const gchar *name = name1;
  va_list args;

  va_start (args, name1);
  while (name)
    {
      const gchar *value = va_arg (args, const gchar*);
      opt = gxk_gadget_options_set (opt, name, value);
      name = va_arg (args, const gchar*);
    }
  va_end (args);
  return opt;
}

GxkGadgetOpt*
gxk_gadget_options_set (GxkGadgetOpt   *opt,
                        const gchar    *name,
                        const gchar    *value)
{
  guint i;
  g_return_val_if_fail (name != NULL, opt);
  if (!opt)
    opt = gxk_gadget_options (NULL);
  for (i = 0; i < OPTIONS_N_ENTRIES (opt); i++)
    if (strcmp (name, opt->names[i]) == 0)
      break;
  if (i >= OPTIONS_N_ENTRIES (opt))
    {
      i = opt->n_variables++;
      opt->names = g_renew (const gchar*, opt->names, OPTIONS_N_ENTRIES (opt));
      opt->values = g_renew (gchar*, opt->values, OPTIONS_N_ENTRIES (opt));
      opt->names[i] = g_intern_string (name);
    }
  else if (!opt->intern_quarks)
    g_free (opt->values[i]);
  if (opt->intern_quarks)
    opt->values[i] = (gchar*) g_intern_string (value);
  else
    opt->values[i] = g_strdup (value);
  return opt;
}

const gchar*
gxk_gadget_options_get (GxkGadgetOpt   *opt,
                        const gchar    *name)
{
  guint i;
  if (opt)
    for (i = 0; i < OPTIONS_N_ENTRIES (opt); i++)
      if (strcmp (name, opt->names[i]) == 0)
        return opt->values[i];
  return NULL;
}

GxkGadgetOpt*
gxk_gadget_options_copy (GxkGadgetOpt *source)
{
  GxkGadgetOpt *opt = g_memdup (source, sizeof (*source));
  if (opt)
    {
      opt->names = g_memdup (source->names, source->n_variables * sizeof (*source->names));
      opt->values = g_memdup (source->values, source->n_variables * sizeof (*source->values));
      if (!opt->intern_quarks)
        {
          guint i;
          for (i = 0; i < OPTIONS_N_ENTRIES (opt); i++)
            opt->values[i] = g_strdup (opt->values[i]);
        }
    }
  return opt;
}

void
gxk_gadget_free_options (GxkGadgetOpt *opt)
{
  if (opt)
    {
      guint i;
      if (!opt->intern_quarks)
        for (i = 0; i < OPTIONS_N_ENTRIES (opt); i++)
          g_free (opt->values[i]);
      g_free (opt->values);
      g_free (opt->names);
      g_free (opt);
    }
}

const gchar*
gxk_gadget_get_domain (GxkGadget *gadget)
{
  Node *gadget_node = g_object_get_qdata (gadget, quark_gadget_node);
  g_return_val_if_fail (gadget_node != NULL, NULL);
  return gadget_node->domain;
}

static GtkWidget*
widget_find_level_ordered (GtkWidget   *widget,
                           const gchar *name)
{
  GList *children = g_list_prepend (NULL, widget);
  while (children)
    {
      GList *list, *newlist = NULL;
      for (list = children; list; list = list->next)
        {
          GtkWidget *child = list->data;
          if (child->name && strcmp (child->name, name) == 0)
            {
              g_list_free (children);
              return child;
            }
        }
      /* none found, search next level */
      for (list = children; list; list = list->next)
        {
          if (GTK_IS_CONTAINER (list->data))
            newlist = g_list_concat (gtk_container_get_children (list->data), newlist);
          if (GTK_IS_MENU_ITEM (list->data))
            {
              GtkMenuItem *mitem = list->data;
              if (mitem->submenu)
                newlist = g_list_prepend (newlist, mitem->submenu);
            }
        }
      g_list_free (children);
      children = newlist;
    }
  return NULL;
}

gpointer
gxk_gadget_find (GxkGadget      *gadget,
                 const gchar    *region)
{
  const gchar *next, *c = region;

  g_return_val_if_fail (gadget != NULL, NULL);
  g_return_val_if_fail (region != NULL, NULL);

  if (!GTK_IS_WIDGET (gadget))
    return NULL;

  next = strchr (c, '.');
  while (gadget && next)
    {
      gchar *name = g_strndup (c, next - c);
      c = next + 1;
      gadget = widget_find_level_ordered (gadget, name);
      g_free (name);
    }
  if (gadget)
    gadget = widget_find_level_ordered (gadget, c);
  return gadget;
}

void
gxk_gadget_sensitize (GxkGadget      *gadget,
                      const gchar    *region,
                      gboolean        sensitive)
{
  GtkWidget *widget = gxk_gadget_find (gadget, region);
  if (GTK_IS_WIDGET (widget))
    gtk_widget_set_sensitive (widget, sensitive);
}

void
gxk_gadget_add (GxkGadget      *gadget,
                const gchar    *region,
                gpointer        widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  gadget = gxk_gadget_find (gadget, region);
  if (!gadget || !GTK_IS_CONTAINER (gadget))
    g_warning ("GxkGadget: failed to find region \"%s\"", region);
  else
    gtk_container_add (GTK_CONTAINER (gadget), widget);
}


/* --- gadget types --- */
static void
gadget_define_type (GType           type,
                    const gchar    *name,
                    const gchar   **attribute_names,
                    const gchar   **attribute_values)
{
  GError *error = NULL;
  Node *node;
  node = node_define (standard_domain, name, type, NULL,
                      attribute_names, attribute_values,
                      NULL, NULL, NULL, &error);
  g_datalist_set_data (&standard_domain->nodes, name, node);
  if (error)
    g_error ("while registering standard gadgets: %s", error->message);
}

void
_gxk_init_gadget_types (void)
{
  GType types[1024], *t = types;
  g_assert (quark_gadget_type == 0);
  quark_gadget_type = g_quark_from_static_string ("GxkGadget-type");
  quark_gadget_node = g_quark_from_static_string ("GxkGadget-node");
  standard_domain = g_new0 (Domain, 1);
  standard_domain->domain = g_intern_string ("standard");
  g_datalist_set_data (&domains, standard_domain->domain, standard_domain);
  *t++ = GTK_TYPE_WINDOW;       *t++ = GTK_TYPE_ARROW;  *t++ = GTK_TYPE_SCROLLED_WINDOW;
  *t++ = GTK_TYPE_TABLE;        *t++ = GTK_TYPE_FRAME;  *t++ = GTK_TYPE_ALIGNMENT;
  *t++ = GTK_TYPE_NOTEBOOK;     *t++ = GTK_TYPE_BUTTON; *t++ = GTK_TYPE_MENU_BAR;
  *t++ = GTK_TYPE_TREE_VIEW;    *t++ = GTK_TYPE_LABEL;  *t++ = GTK_TYPE_PROGRESS_BAR;
  *t++ = GTK_TYPE_HPANED;       *t++ = GTK_TYPE_VPANED; *t++ = GTK_TYPE_SPIN_BUTTON;
  *t++ = GTK_TYPE_EVENT_BOX;    *t++ = GTK_TYPE_IMAGE;  *t++ = GTK_TYPE_OPTION_MENU;
  *t++ = GTK_TYPE_HBOX;         *t++ = GTK_TYPE_VBOX;
  *t++ = GTK_TYPE_CHECK_BUTTON; *t++ = GTK_TYPE_ENTRY;  *t++ = GXK_TYPE_MENU_ITEM;
  *t++ = GTK_TYPE_HSCROLLBAR;   *t++ = GTK_TYPE_HSCALE; *t++ = GTK_TYPE_TEAROFF_MENU_ITEM;
  *t++ = GTK_TYPE_VSCROLLBAR;   *t++ = GTK_TYPE_VSCALE; *t++ = GXK_TYPE_IMAGE;
  while (t-- > types)
    gxk_gadget_define_widget_type (*t);
  gadget_define_gtk_menu ();
  gxk_gadget_define_type (GXK_TYPE_GADGET_FACTORY, _gxk_gadget_factory_def);
  gxk_gadget_define_type (GXK_TYPE_WIDGET_PATCHER, _gxk_widget_patcher_def);
}

gboolean
gxk_gadget_type_lookup (GType           type,
                        GxkGadgetType  *ggtype)
{
  GxkGadgetType *tdata = g_type_get_qdata (type, quark_gadget_type);
  if (tdata)
    {
      *ggtype = *tdata;
      return TRUE;
    }
  return FALSE;
}

void
gxk_gadget_define_type (GType                type,
                        const GxkGadgetType *ggtype)
{
  const gchar *attribute_names[1] = { NULL };
  const gchar *attribute_values[1] = { NULL };

  g_return_if_fail (!G_TYPE_IS_ABSTRACT (type));
  g_return_if_fail (G_TYPE_IS_OBJECT (type));
  g_return_if_fail (g_type_get_qdata (type, quark_gadget_type) == NULL);

  g_type_set_qdata (type, quark_gadget_type, (gpointer) ggtype);
  gadget_define_type (type, g_type_name (type), attribute_names, attribute_values);
}


/* --- widget types --- */
static GxkGadget*
widget_create (GType         type,
               const gchar  *name)
{
  return g_object_new (type, "name", name, NULL);
}

static GParamSpec*
widget_find_prop (GxkGadget    *gadget,
                  const gchar  *prop_name)
{
  return g_object_class_find_property (G_OBJECT_GET_CLASS (gadget), prop_name);
}

static void
widget_adopt (GxkGadget *gadget,
              GxkGadget *parent)
{
  gtk_container_add (GTK_CONTAINER (parent), GTK_WIDGET (gadget));
}

static GParamSpec*
widget_find_pack (GxkGadget    *gadget,
                  const gchar  *pack_name)
{
  GtkWidget *parent = GTK_WIDGET (gadget)->parent;
  return gtk_container_class_find_child_property (G_OBJECT_GET_CLASS (parent), pack_name);
}

static void
widget_set_pack (GxkGadget    *gadget,
                 const gchar  *pack_name,
                 const GValue *value)
{
  GtkWidget *parent = GTK_WIDGET (gadget)->parent;
  gtk_container_child_set_property (GTK_CONTAINER (parent), gadget, pack_name, value);
}

void
gxk_gadget_define_widget_type (GType type)
{
  static const GxkGadgetType widget_info = {
    widget_create,
    widget_find_prop,
    (void(*)(GxkGadget*,const gchar*,const GValue*)) g_object_set_property,
    widget_adopt,
    widget_find_pack,
    widget_set_pack,
  };
  const gchar *attribute_names[2] = { NULL, NULL };
  const gchar *attribute_values[2] = { NULL, NULL };
  
  g_return_if_fail (!G_TYPE_IS_ABSTRACT (type));
  g_return_if_fail (g_type_is_a (type, GTK_TYPE_WIDGET));
  g_return_if_fail (g_type_get_qdata (type, quark_gadget_type) == NULL);
  
  g_type_set_qdata (type, quark_gadget_type, (gpointer) &widget_info);
  attribute_names[0] = "prop:visible";
  attribute_values[0] = "$(ifdef,visible,$visible,1)";
  gadget_define_type (type, g_type_name (type), attribute_names, attribute_values);
}

static void
menu_adopt (GxkGadget    *gadget,
            GxkGadget    *parent)
{
  gxk_submenu_attach_to_item (GTK_MENU (gadget), GTK_MENU_ITEM (parent));
}

static void* return_NULL (void) { return NULL; }

static void
gadget_define_gtk_menu (void)
{
  static const GxkGadgetType widget_info = {
    widget_create,
    widget_find_prop,
    (void(*)(GxkGadget*,const gchar*,const GValue*)) g_object_set_property,
    menu_adopt,
    (void*) return_NULL,/* find_pack */
    NULL,               /* set_pack */
  };
  const gchar *attribute_names[2] = { NULL, NULL };
  const gchar *attribute_values[2] = { NULL, NULL };
  GType type = GTK_TYPE_MENU;
  g_type_set_qdata (type, quark_gadget_type, (gpointer) &widget_info);
  attribute_names[0] = "prop:visible";
  attribute_values[0] = "$(ifdef,visible,$visible,1)";
  gadget_define_type (type, g_type_name (type), attribute_names, attribute_values);
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
  if (value[0] == 0 && env->specials)
    env->specials->null_collapse = TRUE;
  return value;
}

static gchar*
mf_empty (GSList *args,
          Env    *env)
{
  return g_strdup ("");
}

static MacroFunc
macro_func_lookup (const gchar *name)
{
  static const struct { const gchar *name; MacroFunc mfunc; } macros[] = {
    { "if",             mf_if, },
    { "nth",            mf_nth, },
    { "ifdef",          mf_ifdef, },
    { "null-collapse",  mf_null_collapse, },
    { "empty",          mf_empty, },
  };
  guint i;
  for (i = 0; i < G_N_ELEMENTS (macros); i++)
    if (strcmp (name, macros[i].name) == 0)
      return macros[i].mfunc;
  return mf_empty;
}
