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
#include "gxkparam.h"
#include "gxklogadjustment.h"
#include <string.h>
#include <math.h>
#include <libintl.h>

/* --- GxkParam functions --- */
static void
param_update_flags (GxkParam *param)
{
  gboolean was_sensitive = param->sensitive;
  GSList *slist;
  param->editable = !param->constant;
  param->sensitive = param->constant || (!param->treadonly && param->ueditable);
  if (was_sensitive != param->sensitive)
    for (slist = param->objects; slist; slist = slist->next)
      if (GTK_IS_WIDGET (slist->data))
        gtk_widget_set_sensitive (slist->data, param->sensitive);
}

static GxkParam*
param_new (GParamSpec      *pspec,
           gboolean         is_constant,
           GxkParamBinding *binding,
           gpointer         user_data)
{
  GxkParam *param = g_malloc0 (sizeof (GxkParam) + sizeof (param->bdata[0]) * (MAX (binding->n_data_fields, 1) - 1));
  param->pspec = g_param_spec_ref (pspec);
  g_param_spec_sink (pspec);
  g_value_init (&param->value, G_PARAM_SPEC_VALUE_TYPE (pspec));
  g_param_value_set_default (param->pspec, &param->value);
  param->objects = NULL;
  param->constant = is_constant ||
                    !(pspec->flags & G_PARAM_WRITABLE) ||
                    g_param_spec_check_option (pspec, "ro");
  param->ueditable = TRUE;
  param->binding = binding;
  if (param->binding->setup)
    {
      param_update_flags (param);
      param->binding->setup (param, user_data);
      /* setup may change param->constant */
    }
  param_update_flags (param);
  return param;
}

GxkParam*
gxk_param_new (GParamSpec      *pspec,
               GxkParamBinding *binding,
               gpointer         user_data)
{
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), NULL);
  g_return_val_if_fail (binding != NULL, NULL);
  return param_new (pspec, FALSE, binding, user_data);
}

GxkParam*
gxk_param_new_constant (GParamSpec      *pspec,
                        GxkParamBinding *binding,
                        gpointer         user_data)
{
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), NULL);
  g_return_val_if_fail (binding != NULL, NULL);
  return param_new (pspec, TRUE, binding, user_data);
}

static void
param_call_update (GxkParam *param,
                   gpointer object)
{
  GxkParamUpdateFunc ufunc = g_object_get_data (object, "GxkParamUpdateFunc");
  if (ufunc)
    {
      gboolean updating = param->updating;
      param->updating = TRUE;   /* protect value from change-notifications during setup */
      ufunc (param, object);
      param->updating = updating;
    }
}

void
gxk_object_set_param_callback (GtkObject          *object,
                               GxkParamUpdateFunc  ufunc)
{
  /* sets the callback to apply param->value to object (update GUI) */
  g_return_if_fail (GTK_IS_OBJECT (object));
  g_object_set_data (object, "GxkParamUpdateFunc", ufunc);
}

void
gxk_param_update (GxkParam *param)
{
  GSList *slist;
  gboolean updating;
  
  g_return_if_fail (GXK_IS_PARAM (param));
  
  updating = param->updating;
  param->updating = TRUE;
  if (param->binding->get_value)
    param->binding->get_value (param, &param->value);
  param_update_flags (param);
  slist = param->objects;
  while (slist)
    {
      GObject *object = slist->data;
      slist = slist->next;
      param_call_update (param, object);
    }
  param->updating = updating;
}

void
gxk_param_add_object (GxkParam          *param,
                      GtkObject         *object)
{
  g_return_if_fail (GXK_IS_PARAM (param));
  if (GTK_IS_WIDGET (object))
    gtk_widget_set_sensitive (GTK_WIDGET (object), param->sensitive);
  param->objects = g_slist_prepend (param->objects, g_object_ref (object));
  g_object_connect (object, "swapped_signal::destroy", gxk_param_remove_object, param, NULL);
  param_call_update (param, object);
}

void
gxk_param_remove_object (GxkParam          *param,
                         GtkObject         *object)
{
  GSList *slist;
  g_return_if_fail (GXK_IS_PARAM (param));
  slist = g_slist_find (param->objects, object);
  g_return_if_fail (slist != NULL);
  param->objects = g_slist_delete_link (param->objects, slist);
  g_object_disconnect (object, "any_signal::destroy", gxk_param_remove_object, param, NULL);
  g_object_unref (object);
}

void
gxk_param_apply_value (GxkParam *param)
{
  g_return_if_fail (GXK_IS_PARAM (param));
  if (param->updating)
    {
      g_warning ("%s: param (%p) currently in update", G_STRFUNC, param);
      return;
    }
  if (param->binding->set_value && param->editable && param->sensitive)
    {
      param->binding->set_value (param, &param->value);
      /* binding->set_value() is supposed to trigger gxk_param_update() sooner or later */
    }
  else
    gxk_param_update (param);
}

void
gxk_param_apply_default (GxkParam *param)
{
  g_return_if_fail (GXK_IS_PARAM (param));
  if (!param->updating && param->editable && param->sensitive)
    {
      g_param_value_set_default (param->pspec, &param->value);
      gxk_param_apply_value (param);
    }
}

void
gxk_param_set_editable (GxkParam *param,
			gboolean  editable)
{
  g_return_if_fail (GXK_IS_PARAM (param));
  editable = editable != FALSE;
  if (param->ueditable != editable)
    {
      GSList *slist;
      param->ueditable = editable;
      param_update_flags (param);
      slist = param->objects;
      while (slist)
        {
          GObject *object = slist->data;
          slist = slist->next;
          param_call_update (param, object);
        }
    }
}

const gchar*
gxk_param_get_name (GxkParam *param)
{
  g_return_val_if_fail (GXK_IS_PARAM (param), NULL);
  return param->pspec->name;
}

static gboolean _param_devel_tips = FALSE;
void
gxk_param_set_devel_tips (gboolean enabled)
{
  _param_devel_tips = enabled != FALSE;
}

gchar*
gxk_param_dup_tooltip (GxkParam *param)
{
  gchar *tooltip;
  const gchar *ctip;
  g_return_val_if_fail (GXK_IS_PARAM (param), NULL);
  ctip = g_param_spec_get_blurb (param->pspec);
  if (!_param_devel_tips)
    tooltip = g_strdup (ctip);
  else if (ctip)
    tooltip = g_strdup_printf ("(%s): %s", g_param_spec_get_name (param->pspec), ctip);
  else
    tooltip = g_strdup_printf ("(%s)", g_param_spec_get_name (param->pspec));
  return tooltip;
}

void
gxk_param_destroy (GxkParam *param)
{
  g_return_if_fail (GXK_IS_PARAM (param));
  g_return_if_fail (param->binding != NULL);
  
  while (param->objects)
    {
      GObject *object = param->objects->data;
      if (GTK_IS_OBJECT (object))
        gtk_object_destroy (GTK_OBJECT (object));
      else
        g_object_unref (object);
    }
  if (param->binding->destroy)
    param->binding->destroy (param);
  param->binding = NULL;
  g_value_unset (&param->value);
  g_param_spec_unref (param->pspec);
  g_free (param);
}


/* --- value binding --- */
static void
param_value_binding_setup (GxkParam       *param,
                           gpointer        user_data)
{
  if (user_data)
    param->constant = TRUE;
}

static void
param_value_binding_set_value (GxkParam     *param,
                               const GValue *value)
{
  GxkParamValueNotify notify = param->bdata[0].v_pointer;
  g_value_copy (value, &param->value);
  if (notify)
    notify (param->bdata[1].v_pointer, param);
  gxk_param_update (param);
}

static void
param_value_binding_get_value (GxkParam *param,
                               GValue   *value)
{
  g_value_copy (&param->value, value);
  g_param_value_validate (param->pspec, value);
}

static void
param_value_binding_destroy (GxkParam *param)
{
  param->bdata[0].v_pointer = NULL;
  param->bdata[1].v_pointer = NULL;
}

static GxkParamBinding _param_value_binding = {
  2,    /* fields: notify, notify_data */
  param_value_binding_setup,
  param_value_binding_set_value,
  param_value_binding_get_value,
  param_value_binding_destroy,
  /* check_writable */
};

GxkParam*
gxk_param_new_value (GParamSpec          *pspec,
                     GxkParamValueNotify  notify,
                     gpointer             notify_data)
{
  GxkParam *param = gxk_param_new (pspec, &_param_value_binding, (gpointer) FALSE);
  param->bdata[0].v_pointer = notify;
  param->bdata[1].v_pointer = notify_data;
  return param;
}

GxkParam*
gxk_param_new_constant_value (GParamSpec          *pspec,
                              GxkParamValueNotify  notify,
                              gpointer             notify_data)
{
  GxkParam *param = gxk_param_new (pspec, &_param_value_binding, (gpointer) TRUE);
  param->bdata[0].v_pointer = notify;
  param->bdata[1].v_pointer = notify_data;
  return param;
}


/* --- param view/editor --- */
static GSList *_param_editor_list = NULL;
static void
param_register_editor (GxkParamEditor *editor,
                       const gchar    *i18n_domain)
{
  editor->ident.nick = dgettext (i18n_domain, editor->ident.nick);
  _param_editor_list = g_slist_prepend (_param_editor_list, (GxkParamEditor*) editor);
}

static void
params_register_editor_dup_typed (GxkParamEditor *editor,
                                  const gchar    *i18n_domain,
                                  GType           new_type)
{
  GxkParamEditor *ed = g_memdup (editor, sizeof (editor[0]));
  ed->type_match.type = new_type;
  param_register_editor (ed, GXK_I18N_DOMAIN);
}

void
gxk_param_register_editor (GxkParamEditor *editor,
                           const gchar    *i18n_domain)
{
  if (editor->type_match.all_int_nums)
    {
      g_return_if_fail (editor->type_match.type == G_TYPE_NONE);
      params_register_editor_dup_typed (editor, i18n_domain, G_TYPE_CHAR);
      params_register_editor_dup_typed (editor, i18n_domain, G_TYPE_UCHAR);
      params_register_editor_dup_typed (editor, i18n_domain, G_TYPE_INT);
      params_register_editor_dup_typed (editor, i18n_domain, G_TYPE_UINT);
      params_register_editor_dup_typed (editor, i18n_domain, G_TYPE_LONG);
      params_register_editor_dup_typed (editor, i18n_domain, G_TYPE_ULONG);
      params_register_editor_dup_typed (editor, i18n_domain, G_TYPE_INT64);
      params_register_editor_dup_typed (editor, i18n_domain, G_TYPE_UINT64);
    }
  if (editor->type_match.all_float_nums)
    {
      g_return_if_fail (editor->type_match.type == G_TYPE_NONE);
      params_register_editor_dup_typed (editor, i18n_domain, G_TYPE_FLOAT);
      params_register_editor_dup_typed (editor, i18n_domain, G_TYPE_DOUBLE);
    }
  if (!editor->type_match.all_int_nums && !editor->type_match.all_float_nums)
    {
      g_return_if_fail (editor->type_match.type != G_TYPE_NONE);
      param_register_editor (editor, i18n_domain);
    }
}

gchar**
gxk_param_list_editors (void)
{
  guint i, j, l = g_slist_length (_param_editor_list);
  gchar **names = g_new (gchar*, l + 1);
  GSList *slist = _param_editor_list;
  for (i = 0; slist; slist = slist->next)
    {
      GxkParamEditor *editor = slist->data;
      for (j = 0; j < i; j++)   /* find dups */
        if (strcmp (names[j], editor->ident.name) == 0)
          break;
      if (j < i)
        continue;               /* avoid dups */
      names[i++] = g_strdup (editor->ident.name);
    }
  names[i] = NULL;
  return names;
}

static guint          _param_editor_name_n_aliases = 0;
static const gchar ***_param_editor_name_aliases = NULL;

void
gxk_param_register_aliases (const gchar **aliases)
{
  if (aliases && aliases[0] && aliases[1])
    {
      guint i = _param_editor_name_n_aliases++;
      _param_editor_name_aliases = g_renew (const gchar**, _param_editor_name_aliases, _param_editor_name_n_aliases);
      _param_editor_name_aliases[i] = aliases;
    }
}

static gboolean
param_editor_name_match (const gchar          *editor_name,
                         const GxkParamEditor *editor)
{
  guint i;
  if (!editor_name || !editor_name[0] || strcmp (editor_name, editor->ident.name) == 0)
    return TRUE;
  for (i = 0; i < _param_editor_name_n_aliases; i++)
    {
      const gchar **aliases = _param_editor_name_aliases[i];
      if (strcmp (editor_name, aliases[0]) == 0)
        {
          guint j = 1;
          while (aliases[j])
            if (strcmp (aliases[j], editor->ident.name) == 0)
              return TRUE;
            else
              j++;
        }
    }
  return FALSE;
}

static guint
param_score_editor (const GxkParamEditor *editor,
                    GParamSpec           *pspec,
                    gboolean              annotate)
{
  gboolean fundamental_specific, type_mismatch = FALSE, option_mismatch = FALSE, type_specific = FALSE;
  guint8 bonus = 0, type_distance = 0;
  GType vtype = G_PARAM_SPEC_VALUE_TYPE (pspec);
  GType etype = editor->type_match.type;
  gboolean is_editable = editor->features.editing;
  if (editor->type_match.type_name)
    {
      GType ftype = etype;
      etype = g_type_from_name (editor->type_match.type_name);
      type_mismatch |= etype == 0;
      if (ftype)
        type_mismatch |= !g_type_is_a (etype, ftype);
      bonus++;
    }
  fundamental_specific = etype != 0;
  if (etype)
    {
      type_specific = !G_TYPE_IS_FUNDAMENTAL (etype);
      is_editable &= g_type_is_a (etype, vtype);	/* write value */
      type_mismatch |= !g_type_is_a (vtype, etype);     /* read value */
      type_distance = g_type_depth (vtype) - g_type_depth (etype);
    }
  if (editor->features.options)
    {
      option_mismatch |= !g_param_spec_provides_options (pspec, editor->features.options);
      bonus++;
    }

  if (annotate)
    g_printerr ("  %s(%s): fundamental=%s derived=%s%s%s editing=%s bonus=%+d%s%s: ",
                editor->ident.name, editor->ident.nick,
                g_type_name (editor->type_match.type),
                editor->type_match.type_name ? editor->type_match.type_name : "<any>",
                editor->type_match.all_int_nums ? "(all-ints)" : "",
                editor->type_match.all_float_nums ? "(all-floats)" : "",
                editor->features.editing ? "yes" : "no",
                editor->features.rating,
                editor->features.options ? " options=" : "",
                editor->features.options ? editor->features.options : "");

  if (type_mismatch || option_mismatch) /* bail out on mismatches */
    {
      if (annotate)
        g_printerr ("%s mismatch\n", type_mismatch ? "type" : "option");
      return 0;
    }
  
  guint rating = 0;
  rating |= 256 - type_distance;
  rating <<= 1;
  rating |= is_editable;
  rating <<= 1;
  rating |= fundamental_specific;
  rating <<= 1;
  rating |= type_specific;
  rating <<= 8;
  rating += 128 + editor->features.rating;      /* rating is signed, 8bit */
  rating <<= 8;
  rating += bonus;      /* bonus is provided for overcomming additional mismatch possibilities */
  
  if (annotate)
    g_printerr ("%d\n", rating);
  return rating;
}

static GxkParamEditor*
param_lookup_editor (const gchar *editor_name,
                     GParamSpec  *pspec)
{
  GxkParamEditor *best = NULL;
  guint rating = 0;             /* threshold for mismatch */
  GSList *slist;
  for (slist = _param_editor_list; slist; slist = slist->next)
    {
      GxkParamEditor *editor = slist->data;
      if (param_editor_name_match (editor_name, editor))
        {
          guint r = param_score_editor (editor, pspec, FALSE);
          if (r > rating)       /* only notice improvements */
            {
              best = editor;
              rating = r;
            }
        }
    }
  if (!best)                    /* if editor_name!=NULL, best might be NULL */
    best = param_lookup_editor (NULL, pspec);
  return best;
}

guint
gxk_param_editor_score (const gchar *editor_name,
                        GParamSpec  *pspec)
{
  guint rating = 0;             /* threshold for mismatch */
  GSList *slist;
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), 0);
  g_return_val_if_fail (editor_name != NULL, 0);
  for (slist = _param_editor_list; slist; slist = slist->next)
    {
      GxkParamEditor *editor = slist->data;
      if (param_editor_name_match (editor_name, editor))
        {
          guint r = param_score_editor (editor, pspec, FALSE);
          rating = MAX (r, rating);
        }
    }
  return rating;
}

void
gxk_param_editor_debug_score (GParamSpec *pspec)
{
  guint rating = 0;             /* threshold for mismatch */
  GSList *slist;
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  const gchar *options = g_param_spec_get_options (pspec);
  g_printerr ("GxkParamEditor: rating for pspec: name=%s fundamental=%s type=%s options=%s nick=\"%s\" blurb=\"%s\"\n",
              pspec->name,
              g_type_name (G_TYPE_FUNDAMENTAL (G_PARAM_SPEC_VALUE_TYPE (pspec))),
              g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
              options ? options : "",
              g_param_spec_get_nick (pspec), g_param_spec_get_blurb (pspec));
  for (slist = _param_editor_list; slist; slist = slist->next)
    {
      GxkParamEditor *editor = slist->data;
      guint r = param_score_editor (editor, pspec, TRUE);
      rating = MAX (r, rating);
    }
}

const gchar*
gxk_param_lookup_editor (const gchar *editor_name,
                         GParamSpec  *pspec)
{
  g_return_val_if_fail (editor_name != NULL, 0);
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), 0);
  return param_lookup_editor (editor_name, pspec)->ident.name;
}

GtkWidget*
gxk_param_create_editor (GxkParam               *param,
                         const gchar            *editor_name)
{
  GxkParamEditor *editor = param_lookup_editor (editor_name, param->pspec);
  gchar *tooltip = gxk_param_dup_tooltip (param);
  GtkWidget *widget, *toplevel;
  gboolean updating = param->updating;
  param->updating = TRUE;       /* protect value from change-notifications during setup */
  widget = editor->create_widget (param, tooltip, editor->variant);
  param->updating = updating;
  g_free (tooltip);
  gxk_object_set_param_callback (GTK_OBJECT (widget), (gpointer) editor->update);
  gxk_param_add_object (param, GTK_OBJECT (widget));
  toplevel = gtk_widget_get_toplevel (widget);
  if (toplevel != widget)
    gxk_param_add_object (param, GTK_OBJECT (toplevel));
  return toplevel;
}

/* --- param editor sizes --- */
static const GxkParamEditorSizes param_editor_default_sizes = {
  1, 3,         /* char */
  0, 3,         /* uchar */
  1, 10,        /* int */
  0, 10,        /* uint */
#if GLIB_SIZEOF_LONG <= 4
  1, 10,        /* long */
  0, 10,        /* ulong */
#else
  1, 19,        /* long */
  0, 20,        /* ulong */
#endif
  1, 19,        /* int64 */
  0, 20,        /* uint64 */
  2, 7,         /* float */
  2, 17,        /* double */
  8, 2,         /* string */
  TRUE,         /* may_shrink */
};
static GxkParamEditorSizes *size_groups = NULL;
static guint                n_size_groups = 0;

guint
gxk_param_create_size_group (void)
{
  guint i;
  g_assert (n_size_groups < 0xffff);
  i = n_size_groups++;
  size_groups = g_renew (GxkParamEditorSizes, size_groups, n_size_groups);
  size_groups[i] = param_editor_default_sizes;
  size_groups[i].may_shrink = FALSE;
  return n_size_groups;
}

const GxkParamEditorSizes*
gxk_param_get_editor_sizes (GxkParam *param)
{
  if (param->size_group && param->size_group <= n_size_groups)
    return &size_groups[param->size_group - 1];
  return &param_editor_default_sizes;
}

void
gxk_param_set_size_group (GxkParam                  *param,
                          guint                      size_group)
{
  g_return_if_fail (GXK_IS_PARAM (param));
  g_return_if_fail (size_group <= n_size_groups);
  param->size_group = size_group;
}

void
gxk_param_set_sizes (guint                      size_group,
                     const GxkParamEditorSizes *esizes)
{
  g_return_if_fail (size_group > 0 && size_group <= n_size_groups);
  size_groups[size_group - 1] = esizes ? *esizes : param_editor_default_sizes;
}

guint
gxk_param_get_digits (gdouble                    value,
                      guint                      base)
{
  if (value <= 1 || base < 2)
    return 1;
  return 1 + log (value) / log (base);
}


/* --- param editor widgets --- */
#include "gxkparam-entry.c"
#include "gxkparam-label.c"
#include "gxkparam-scale.c"
#include "gxkparam-spinner.c"
#include "gxkparam-toggle.c"

void
_gxk_init_params (void)
{
  gxk_param_register_editor (&param_entry, GXK_I18N_DOMAIN);
  gxk_param_register_editor (&param_label1, GXK_I18N_DOMAIN);
  gxk_param_register_editor (&param_label2, GXK_I18N_DOMAIN);
  gxk_param_register_editor (&param_label3, GXK_I18N_DOMAIN);
  gxk_param_register_editor (&param_scale1, GXK_I18N_DOMAIN);
  gxk_param_register_editor (&param_scale2, GXK_I18N_DOMAIN);
  gxk_param_register_editor (&param_scale3, GXK_I18N_DOMAIN);
  gxk_param_register_editor (&param_scale4, GXK_I18N_DOMAIN);
  gxk_param_register_aliases (param_scale_aliases1);
  gxk_param_register_aliases (param_scale_aliases2);
  gxk_param_register_editor (&param_spinner1, GXK_I18N_DOMAIN);
  gxk_param_register_editor (&param_spinner2, GXK_I18N_DOMAIN);
  gxk_param_register_editor (&param_toggle, GXK_I18N_DOMAIN);
}

/* --- param implementation utils --- */
gboolean
gxk_param_entry_key_press (GtkEntry    *entry,
			   GdkEventKey *event)
{
  GtkEditable *editable = GTK_EDITABLE (entry);
  gboolean intercept = FALSE;
  
  if (event->state & GDK_MOD1_MASK)
    switch (event->keyval)
      {
      case 'b': /* check gtk_move_backward_word() */
	intercept = gtk_editable_get_position (editable) <= 0;
	break;
      case 'd': /* gtk_delete_forward_word() */
	intercept = TRUE;
	break;
      case 'f': /* check gtk_move_forward_word() */
	intercept = gtk_editable_get_position (editable) >= entry->text_length;
	break;
      default:
	break;
      }
  return intercept;
}

void
gxk_param_entry_set_text (GxkParam    *param,
                          GtkWidget   *_entry,
                          const gchar *text)
{
  GtkEntry *entry = (GtkEntry*) _entry;
  g_return_if_fail (GTK_IS_ENTRY (entry));
  if (!text)
    text = "";
  if (!g_str_equal (gtk_entry_get_text (entry), text))
    {
      gtk_entry_set_text (entry, text);
      gtk_editable_set_position ((GtkEditable*) entry, param->editable ? G_MAXINT : 0);
    }
  gtk_editable_set_editable (GTK_EDITABLE (entry), param->editable);
}

static void
gxk_entry_changed_marshaller (GClosure       *closure,
                              GValue         *return_value,
                              guint           n_param_values,
                              const GValue   *param_values,
                              gpointer        invocation_hint,
                              gpointer        marshal_data)
{
  GCClosure *cclosure = (GCClosure*) closure;
  void (*changed) (GtkWidget*, GxkParam*) = cclosure->callback;
  GxkParam *param = closure->data;
  if (!param->updating)
    changed (g_value_get_object (param_values + 0), param);
}

void
gxk_param_entry_connect_handlers (GxkParam    *param,
                                  GtkWidget   *entry,
                                  void       (*changed) (GtkWidget*,
                                                         GxkParam*))
{
  g_signal_connect_data (entry, "key-press-event", G_CALLBACK (gxk_param_entry_key_press), NULL, NULL, 0);
  if (changed)
    {
      GClosure *closure = g_cclosure_new (G_CALLBACK (changed), param, NULL);
      g_closure_set_marshal (closure, gxk_entry_changed_marshaller);
      g_signal_connect_closure (entry, "activate", closure, FALSE);
      g_signal_connect_closure (entry, "focus-out-event", closure, FALSE);
    }
}

gboolean
gxk_param_ensure_focus (GtkWidget *widget)
{
  gtk_widget_grab_focus (widget);
  return FALSE;
}

static void
param_adjustment_update (GxkParam       *param,
                         GtkObject      *object)
{
  GValue dvalue = { 0, };
  g_value_init (&dvalue, G_TYPE_DOUBLE);
  g_value_transform (&param->value, &dvalue);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (object), g_value_get_double (&dvalue));
  g_value_unset (&dvalue);
}

static void
param_adjustment_value_changed (GtkAdjustment *adjustment,
                                GxkParam      *param)
{
  if (!param->updating)
    {
      GValue dvalue = { 0, };
      g_value_init (&dvalue, G_TYPE_DOUBLE);
      g_value_set_double (&dvalue, adjustment->value);
      g_value_transform (&dvalue, &param->value);
      g_value_unset (&dvalue);
      gxk_param_apply_value (param);
    }
}

GtkAdjustment*
gxk_param_get_adjustment_with_stepping (GxkParam  *param,
                                        gdouble    pstepping)
{
  gdouble min, max, dfl, stepping;
  GParamSpec *pspec = param->pspec;
  GtkObject *adjustment;
  gboolean isint = TRUE;
  GSList *slist;
  for (slist = param->objects; slist; slist = slist->next)
    if (GTK_IS_ADJUSTMENT (slist->data) &&
        g_object_get_data (slist->data, "gxk-param-func") == gxk_param_get_adjustment_with_stepping)
      return slist->data;
  
#define EXTRACT_FIELDS(p,T,mi,ma,df) { T *t = (T*) p; mi = t->minimum; ma = t->maximum; df = t->default_value; }
  if (G_IS_PARAM_SPEC_CHAR (pspec))
    {
      EXTRACT_FIELDS (pspec, GParamSpecChar, min, max, dfl);
      stepping = g_param_spec_get_istepping (pspec);
    }
  else if (G_IS_PARAM_SPEC_UCHAR (pspec))
    {
      EXTRACT_FIELDS (pspec, GParamSpecUChar, min, max, dfl);
      stepping = g_param_spec_get_istepping (pspec);
    }
  else if (G_IS_PARAM_SPEC_INT (pspec))
    {
      EXTRACT_FIELDS (pspec, GParamSpecInt, min, max, dfl);
      stepping = g_param_spec_get_istepping (pspec);
    }
  else if (G_IS_PARAM_SPEC_UINT (pspec))
    {
      EXTRACT_FIELDS (pspec, GParamSpecUInt, min, max, dfl);
      stepping = g_param_spec_get_istepping (pspec);
    }
  else if (G_IS_PARAM_SPEC_LONG (pspec))
    {
      EXTRACT_FIELDS (pspec, GParamSpecLong, min, max, dfl);
      stepping = g_param_spec_get_istepping (pspec);
    }
  else if (G_IS_PARAM_SPEC_ULONG (pspec))
    {
      EXTRACT_FIELDS (pspec, GParamSpecULong, min, max, dfl);
      stepping = g_param_spec_get_istepping (pspec);
    }
  else if (G_IS_PARAM_SPEC_INT64 (pspec))
    {
      EXTRACT_FIELDS (pspec, GParamSpecInt64, min, max, dfl);
      stepping = g_param_spec_get_istepping (pspec);
    }
  else if (G_IS_PARAM_SPEC_UINT64 (pspec))
    {
      EXTRACT_FIELDS (pspec, GParamSpecUInt64, min, max, dfl);
      stepping = g_param_spec_get_istepping (pspec);
    }
  else if (G_IS_PARAM_SPEC_FLOAT (pspec))
    {
      EXTRACT_FIELDS (pspec, GParamSpecFloat, min, max, dfl);
      stepping = g_param_spec_get_fstepping (pspec);
      isint = FALSE;
    }
  else if (G_IS_PARAM_SPEC_DOUBLE (pspec))
    {
      EXTRACT_FIELDS (pspec, GParamSpecDouble, min, max, dfl);
      stepping = g_param_spec_get_fstepping (pspec);
      isint = FALSE;
    }
  else
    return NULL;
#undef EXTRACT_FIELDS

  if (pstepping <= 0)
    pstepping = isint ? 1.0 : 0.1;
  if (pstepping == stepping)
    {
      pstepping = ABS (max - min) / 10;
      if (isint && 0 == (guint64) pstepping)
        pstepping = ABS (max - min) > 2 ? 2 : 1;
    }
  adjustment = gtk_adjustment_new (dfl, min, max,
                                   MIN (pstepping, stepping),
                                   MAX (pstepping, stepping),
                                   0);
  gxk_object_set_param_callback (adjustment, param_adjustment_update);
  gxk_param_add_object (param, adjustment);
  gtk_object_sink (adjustment);
  /* catch notifies *after* the widgets are updated */
  g_object_connect (adjustment,
                    "signal_after::value-changed", param_adjustment_value_changed, param,
                    NULL);
  /* recognize adjustments created by this function */
  g_object_set_data (adjustment, "gxk-param-func", gxk_param_get_adjustment_with_stepping);
  return GTK_ADJUSTMENT (adjustment);
}

GtkAdjustment*
gxk_param_get_adjustment (GxkParam *param)
{
  return gxk_param_get_adjustment_with_stepping (param, 0);
}

GtkAdjustment*
gxk_param_get_log_adjustment (GxkParam *param)
{
  GtkAdjustment *adjustment;
  GSList *slist;
  for (slist = param->objects; slist; slist = slist->next)
    if (GXK_IS_LOG_ADJUSTMENT (slist->data) &&
        g_object_get_data (slist->data, "gxk-param-func") == gxk_param_get_log_adjustment)
      return slist->data;
  adjustment = gxk_param_get_adjustment (param);
  if (adjustment)
    {
      gdouble center, base, n_steps;
      if (g_param_spec_get_log_scale (param->pspec, &center, &base, &n_steps))
        {
          GtkAdjustment *log_adjustment = gxk_log_adjustment_from_adj (adjustment);
          GtkObject *object = GTK_OBJECT (log_adjustment);
          gxk_log_adjustment_setup (GXK_LOG_ADJUSTMENT (log_adjustment), center, base, n_steps);
          gxk_param_add_object (param, object);
          gtk_object_sink (object);
          /* recognize adjustments created by this function */
          g_object_set_data (log_adjustment, "gxk-param-func", gxk_param_get_log_adjustment);
          return log_adjustment;
        }
    }
  return NULL;
}
