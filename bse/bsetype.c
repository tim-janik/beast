/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bsetype.h"

#include "bseplugin.h"
#include <string.h>
#include <gobject/gvaluecollector.h>


/* --- variables --- */
static GQuark	quark_options = 0;
static GQuark	quark_blurb = 0;
static GQuark	quark_loc_file = 0;
static GQuark	quark_loc_line = 0;
static GQuark	quark_authors = 0;
static GQuark	quark_license = 0;
static GQuark	quark_boxed_export_node = 0;
GType bse_type_id_packed_pointer = 0;


/* --- functions --- */
const gchar*
bse_type_get_options (GType   type)
{
  return g_type_get_qdata (type, quark_options);
}

void
bse_type_add_options (GType        type,
                      const gchar *options)
{
  g_return_if_fail (bse_type_get_options (type) == NULL);
  g_type_set_qdata (type, quark_options, g_strdup (options));
}

const gchar*
bse_type_get_blurb (GType   type)
{
  return g_type_get_qdata (type, quark_blurb);
}

const gchar*
bse_type_get_file (GType   type)
{
  return g_type_get_qdata (type, quark_loc_file);
}

guint
bse_type_get_line (GType   type)
{
  return (guint) g_type_get_qdata (type, quark_loc_line);
}

void
bse_type_add_blurb (GType        type,
		    const gchar *blurb,
                    const gchar *file,
                    guint        line)
{
  g_return_if_fail (bse_type_get_blurb (type) == NULL);
  g_type_set_qdata (type, quark_blurb, g_strdup (blurb));
  g_type_set_qdata (type, quark_loc_file, g_strdup (file));
  g_type_set_qdata (type, quark_loc_line, (gpointer) line);
}

const gchar*
bse_type_get_authors (GType   type)
{
  return g_type_get_qdata (type, quark_authors);
}

void
bse_type_add_authors (GType        type,
                      const gchar *authors)
{
  g_return_if_fail (bse_type_get_authors (type) == NULL);
  g_type_set_qdata (type, quark_authors, g_strdup (authors));
}

const gchar*
bse_type_get_license (GType   type)
{
  return g_type_get_qdata (type, quark_license);
}

void
bse_type_add_license (GType        type,
                      const gchar *license)
{
  g_return_if_fail (bse_type_get_license (type) == NULL);
  g_type_set_qdata (type, quark_license, g_strdup (license));
}

GType  
bse_type_register_static (GType            parent_type,
			  const gchar     *type_name,
			  const gchar     *type_blurb,
                          const gchar     *file,
                          guint            line,
                          const GTypeInfo *info)
{
  GType type;
  
  /* some builtin types have destructors eventhough they are registered
   * statically, compensate for that
   */
  if (G_TYPE_IS_INSTANTIATABLE (parent_type) && info->class_finalize)
    {
      GTypeInfo tmp_info;
      
      tmp_info = *info;
      tmp_info.class_finalize = NULL;
      info = &tmp_info;
    }
  
  type = g_type_register_static (parent_type, type_name, info, 0);
  
  bse_type_add_blurb (type, type_blurb, file, line);
  
  return type;
}

GType  
bse_type_register_abstract (GType            parent_type,
                            const gchar     *type_name,
                            const gchar     *type_blurb,
                            const gchar     *file,
                            guint            line,
                            const GTypeInfo *info)
{
  GType type;
  
  /* some builtin types have destructors eventhough they are registered
   * statically, compensate for that
   */
  if (G_TYPE_IS_INSTANTIATABLE (parent_type) && info->class_finalize)
    {
      GTypeInfo tmp_info;
      
      tmp_info = *info;
      tmp_info.class_finalize = NULL;
      info = &tmp_info;
    }
  
  type = g_type_register_static (parent_type, type_name, info, G_TYPE_FLAG_ABSTRACT);
  
  bse_type_add_blurb (type, type_blurb, file, line);
  
  return type;
}

GType  
bse_type_register_dynamic (GType        parent_type,
			   const gchar *type_name,
			   GTypePlugin *plugin)
{
  GType type = g_type_register_dynamic (parent_type, type_name, plugin, 0);
  return type;
}

static void
bse_boxed_value_init (GValue *value)
{
  value->data[0].v_pointer = NULL;
}

static void
bse_boxed_value_free (GValue *value)
{
  if (value->data[0].v_pointer && !(value->data[1].v_uint & G_VALUE_NOCOPY_CONTENTS))
    {
      BseExportNodeBoxed *bnode = g_type_get_qdata (G_VALUE_TYPE (value), quark_boxed_export_node);
      if (bnode)
        bnode->free (value->data[0].v_pointer);
      else
        g_critical ("%s: %s due to missing implementation: %s", G_STRFUNC, "leaking boxed structure", g_type_name (G_VALUE_TYPE (value)));
    }
}

static void
bse_boxed_value_copy (const GValue *src_value,
                      GValue       *dest_value)
{
  dest_value->data[0].v_pointer = NULL;
  if (src_value->data[0].v_pointer)
    {
      BseExportNodeBoxed *bnode = g_type_get_qdata (G_VALUE_TYPE (src_value), quark_boxed_export_node);
      if (bnode)
        dest_value->data[0].v_pointer = bnode->copy (src_value->data[0].v_pointer);
      else
        g_critical ("%s: %s due to missing implementation: %s", G_STRFUNC, "not copying boxed structure", g_type_name (G_VALUE_TYPE (src_value)));
    }
}

static gpointer
bse_boxed_value_peek_pointer (const GValue *value)
{
  return value->data[0].v_pointer;
}

static gchar*
bse_boxed_collect_value (GValue      *value,
                         guint        n_collect_values,
                         GTypeCValue *collect_values,
                         guint        collect_flags)
{
  if (!collect_values[0].v_pointer)
    value->data[0].v_pointer = NULL;
  else
    {
      if (collect_flags & G_VALUE_NOCOPY_CONTENTS)
        {
          value->data[0].v_pointer = collect_values[0].v_pointer;
          value->data[1].v_uint = G_VALUE_NOCOPY_CONTENTS;
        }
      else
        {
          BseExportNodeBoxed *bnode = g_type_get_qdata (G_VALUE_TYPE (value), quark_boxed_export_node);
          if (bnode)
            value->data[0].v_pointer = bnode->copy (collect_values[0].v_pointer);
          else
            g_critical ("%s: %s due to missing implementation: %s", G_STRFUNC, "not copying boxed structure", g_type_name (G_VALUE_TYPE (value)));
        }
    }
  return NULL;
}

static gchar*
bse_boxed_lcopy_value (const GValue *value,
                       guint         n_collect_values,
                       GTypeCValue  *collect_values,
                       guint         collect_flags)
{
  gpointer *boxed_p = collect_values[0].v_pointer;
  if (!boxed_p)
    return g_strdup_printf ("value location for `%s' passed as NULL", G_VALUE_TYPE_NAME (value));
  if (!value->data[0].v_pointer)
    *boxed_p = NULL;
  else if (collect_flags & G_VALUE_NOCOPY_CONTENTS)
    *boxed_p = value->data[0].v_pointer;
  else
    {
      BseExportNodeBoxed *bnode = g_type_get_qdata (G_VALUE_TYPE (value), quark_boxed_export_node);
      if (bnode)
        *boxed_p = bnode->copy (value->data[0].v_pointer);
      else
        g_critical ("%s: %s due to missing implementation: %s", G_STRFUNC, "not copying boxed structure", g_type_name (G_VALUE_TYPE (value)));
    }
  return NULL;
}

static void
bse_boxed_to_record (const GValue *src_value,
                     GValue       *dest_value)
{
  BseExportNodeBoxed *bnode = g_type_get_qdata (G_VALUE_TYPE (src_value), quark_boxed_export_node);
  if (bnode)
    bnode->boxed2recseq (src_value, dest_value);
  else
    g_critical ("%s: %s due to missing implementation: %s", G_STRFUNC, "not converting boxed structure", g_type_name (G_VALUE_TYPE (src_value)));
}

static void
bse_boxed_from_record (const GValue *src_value,
                       GValue       *dest_value)
{
  BseExportNodeBoxed *bnode = g_type_get_qdata (G_VALUE_TYPE (dest_value), quark_boxed_export_node);
  if (bnode)
    bnode->seqrec2boxed (src_value, dest_value);
  else
    g_critical ("%s: %s due to missing implementation: %s", G_STRFUNC, "not converting boxed structure", g_type_name (G_VALUE_TYPE (dest_value)));
}

GType
bse_type_register_loadable_boxed (BseExportNodeBoxed *bnode,
                                  GTypePlugin        *plugin)
{
  static const GTypeValueTable boxed_vtable = {
    bse_boxed_value_init,
    bse_boxed_value_free,
    bse_boxed_value_copy,
    bse_boxed_value_peek_pointer,
    "p",
    bse_boxed_collect_value,
    "p",
    bse_boxed_lcopy_value,
  };
  static const GTypeInfo info = {
    0,                          /* class_size */
    NULL,                       /* base_init */
    NULL,                       /* base_destroy */
    NULL,                       /* class_init */
    NULL,                       /* class_destroy */
    NULL,                       /* class_data */
    0,                          /* instance_size */
    0,                          /* n_preallocs */
    NULL,                       /* instance_init */
    &boxed_vtable,              /* value_table */
  };
  GType type;
  g_return_val_if_fail (bnode->node.name != NULL, 0);
  g_return_val_if_fail (bnode->copy != NULL, 0);
  g_return_val_if_fail (bnode->free != NULL, 0);
  g_return_val_if_fail (bnode->node.ntype == BSE_EXPORT_NODE_RECORD || bnode->node.ntype == BSE_EXPORT_NODE_SEQUENCE, 0);
  g_return_val_if_fail (g_type_from_name (bnode->node.name) == 0, 0);
  
  type = g_type_register_static (G_TYPE_BOXED, bnode->node.name, &info, 0);
  if (bnode->boxed2recseq)
    g_value_register_transform_func (type,
                                     bnode->node.ntype == BSE_EXPORT_NODE_RECORD
                                     ? SFI_TYPE_REC : SFI_TYPE_SEQ,
                                     bse_boxed_to_record);
  if (bnode->seqrec2boxed)
    g_value_register_transform_func (bnode->node.ntype == BSE_EXPORT_NODE_RECORD
                                     ? SFI_TYPE_REC : SFI_TYPE_SEQ,
                                     type,
                                     bse_boxed_from_record);
  return type;
}

void
bse_type_reinit_boxed (BseExportNodeBoxed *bnode)
{
  g_return_if_fail (G_TYPE_IS_BOXED (bnode->node.type));
  g_type_set_qdata (bnode->node.type, quark_boxed_export_node, bnode);
  switch (bnode->node.ntype)
    {
    case BSE_EXPORT_NODE_RECORD:
      sfi_boxed_type_set_rec_fields (bnode->node.type, bnode->func.get_fields());
      break;
    case BSE_EXPORT_NODE_SEQUENCE:
      sfi_boxed_type_set_seq_element (bnode->node.type, bnode->func.get_element());
      break;
    default:    g_assert_not_reached();
    }
}

void
bse_type_uninit_boxed (BseExportNodeBoxed *bnode)
{
  static SfiRecFields rfields = { 0, NULL };
  g_return_if_fail (G_TYPE_IS_BOXED (bnode->node.type));
  switch (bnode->node.ntype)
    {
    case BSE_EXPORT_NODE_RECORD:
      sfi_boxed_type_set_rec_fields (bnode->node.type, rfields);
      break;
    case BSE_EXPORT_NODE_SEQUENCE:
      sfi_boxed_type_set_seq_element (bnode->node.type, NULL);
      break;
    default:    g_assert_not_reached();
    }
  g_type_set_qdata (bnode->node.type, quark_boxed_export_node, NULL);
}


/* --- customized pspec constructors --- */
#define NULL_CHECKED(x)         ((x) && (x)[0] ? x : NULL)
GParamSpec*
bse_param_spec_enum (const gchar    *name,
                     const gchar    *nick,
                     const gchar    *blurb,
                     gint            default_value,
                     GType           enum_type,
                     const gchar    *hints)
{
  GParamSpec *pspec;
  
  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);
  g_return_val_if_fail (enum_type != G_TYPE_ENUM, NULL);

  /* g_param_spec_enum() validates default_value, which we allways allow
   * to be 0, so we might need to adjust it to pass validation
   */
  if (default_value == 0)
    {
      GEnumClass *enum_class = g_type_class_ref (enum_type);
      if (!g_enum_get_value (enum_class, default_value))
        default_value = enum_class->values[0].value;
      g_type_class_unref (enum_class);
    }

  pspec = g_param_spec_enum (name, NULL_CHECKED (nick), NULL_CHECKED (blurb), enum_type, default_value, 0);
  sfi_pspec_set_options (pspec, hints);

  return pspec;
}


/* --- SFIDL includes --- */
/* include SFIDL generations */
#include        "bsegentypes.c"


/* --- type initializations --- */
/* FIXME: extern decls for other *.h files that implement fundamentals */
extern void     bse_type_register_procedure_info        (GTypeInfo    *info);
extern void     bse_type_register_object_info           (GTypeInfo    *info);
extern void     bse_type_register_enums                 (void);
extern void     bse_param_types_init			(void);

void
bse_type_init (void)
{
  GTypeInfo info;
  static const struct {
    GType   *const type_p;
    GType   (*register_type) (void);
  } builtin_types[] = {
    /* include class type id builtin variable declarations */
#include "bsegentype_array.c"
  };
  const guint n_builtin_types = sizeof (builtin_types) / sizeof (builtin_types[0]);
  static GTypeFundamentalInfo finfo = { 0, };
  guint i;
  
  g_return_if_fail (quark_blurb == 0);
  
  /* type system initialization
   */
  quark_options = g_quark_from_static_string ("BseType-options");
  quark_blurb = g_quark_from_static_string ("BseType-blurb");
  quark_loc_file = g_quark_from_static_string ("BseType-file");
  quark_loc_line = g_quark_from_static_string ("BseType-line");
  quark_authors = g_quark_from_static_string ("BseType-authors");
  quark_license = g_quark_from_static_string ("BseType-license");
  quark_boxed_export_node = g_quark_from_static_string ("BseType-boxed-export-node");
  g_type_init ();
  
  /* initialize parameter types */
  bse_param_types_init ();
  
  /* initialize builtin enumerations */
  bse_type_register_enums ();
  
  /* BSE_TYPE_PROCEDURE
   */
  memset (&finfo, 0, sizeof (finfo));
  finfo.type_flags = G_TYPE_FLAG_CLASSED | G_TYPE_FLAG_DERIVABLE;
  memset (&info, 0, sizeof (info));
  bse_type_register_procedure_info (&info);
  g_type_register_fundamental (BSE_TYPE_PROCEDURE, "BseProcedure", &info, &finfo, 0);
  bse_type_add_blurb (BSE_TYPE_PROCEDURE, "BSE Procedure base type", __FILE__, __LINE__);
  g_assert (BSE_TYPE_PROCEDURE == g_type_from_name ("BseProcedure"));

  /* initialize extra types */
  {
    static const GTypeInfo dummy = { 0, };
    bse_type_id_packed_pointer = g_type_register_static (G_TYPE_STRING, "BseTypePackedPointer", &dummy, 0);
  }

  /* initialize builtin class types */
  for (i = 0; i < n_builtin_types; i++)
    *(builtin_types[i].type_p) = builtin_types[i].register_type ();
}
