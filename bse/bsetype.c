/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
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
#include        "bsetype.h"

#include        "bseenums.h"
#include        "bseplugin.h"
#include        <string.h>




/* --- variables --- */
static GQuark	quark_blurb = 0;


/* --- functions --- */
gchar*
bse_type_blurb (GType   type)
{
  return g_type_get_qdata (type, quark_blurb);
}

void
bse_type_set_blurb (GType        type,
		    const gchar *blurb)
{
  g_return_if_fail (bse_type_blurb (type) == NULL);

  g_type_set_qdata (type, quark_blurb, g_strdup (blurb));
}

GType  
bse_type_register_static (GType              parent_type,
			  const gchar       *type_name,
			  const gchar       *type_blurb,
			  const GTypeInfo *info)
{
  GType type;

  /* some builtin types have destructors eventhough they are registered
   * statically, compensate for that
   */
  if (G_TYPE_IS_INSTANTIATABLE (parent_type) && info->class_destroy)
    {
      GTypeInfo tmp_info;

      tmp_info = *info;
      tmp_info.class_destroy = NULL;
      info = &tmp_info;
    }

  type = g_type_register_static (parent_type, type_name, info);

  bse_type_set_blurb (type, type_blurb);

  return type;
}

GType  
bse_type_register_dynamic (GType        parent_type,
			   const gchar *type_name,
			   const gchar *type_blurb,
			   BsePlugin   *plugin)
{
  GType type = g_type_register_dynamic (parent_type, type_name, &plugin->gtype_plugin);

  bse_type_set_blurb (type, type_blurb);

  return type;
}

/* include type id builtin variable declarations */
#include        "bsegentypes.c"

/* extern decls for other *.h files that implement fundamentals */
extern void     bse_type_register_procedure_info        (GTypeInfo    *info);
extern void     bse_type_register_object_info           (GTypeInfo    *info);
extern void     bse_type_register_enums                 (void);

void
bse_type_init (void)
{
  GTypeInfo info;
  static const struct {
    GType        type;
    const gchar *name;
    const gchar *blurb;
  } param_types[] = {
    { BSE_TYPE_PARAM_BOOL,      "BseParamSpecBool",     "Boolean parameter", },
    { BSE_TYPE_PARAM_INT,       "BseParamSpecInt",      "Integer parameter", },
    { BSE_TYPE_PARAM_UINT,      "BseParamSpecUInt",     "Unsigned integer parameter", },
    { BSE_TYPE_PARAM_ENUM,      "BseParamSpecEnum",     "Enumeration parameter", },
    { BSE_TYPE_PARAM_FLAGS,     "BseParamSpecFlags",    "Flag enumeration parameter", },
    { BSE_TYPE_PARAM_FLOAT,     "BseParamSpecFloat",    "Floating point parameter", },
    { BSE_TYPE_PARAM_DOUBLE,    "BseParamSpecDouble",   "Double precision floating point parameter", },
    { BSE_TYPE_PARAM_TIME,      "BseParamSpecTime",     "Time parameter", },
    { BSE_TYPE_PARAM_NOTE,      "BseParamSpecNote",     "Note parameter", },
    { BSE_TYPE_PARAM_INDEX_2D,  "BseParamSpecIndex2D",  "Packed 2D index parameter", },
    { BSE_TYPE_PARAM_STRING,    "BseParamSpecString",   "String parameter", },
    { BSE_TYPE_PARAM_DOTS,      "BseParamSpecDots",     "Line graph parameter", },
    { BSE_TYPE_PARAM_ITEM,      "BseParamSpecItem",     "Item object reference", },
  };
  const guint n_param_types = sizeof (param_types) / sizeof (param_types[0]);
  static const struct {
    GType   *const type_p;
    GType   (*register_type) (void);
  } builtin_types[] = {
    /* include type id builtin variable declarations */
#include "bsegentype_array.c"
  };
  const guint n_builtin_types = sizeof (builtin_types) / sizeof (builtin_types[0]);
  static GTypeFundamentalInfo finfo = { 0, 0, NULL, };
  GType type;
  guint i;

  g_return_if_fail (quark_blurb == 0);

  /* type system initialization
   */
  quark_blurb = g_quark_from_static_string ("GType  -blurb");
  g_type_init ();

  /* BSE_TYPE_PARAM types
   */
  memset (&info, 0, sizeof (info));
  finfo.type_flags = G_TYPE_FLAG_DERIVABLE;
  g_type_register_fundamental (BSE_TYPE_PARAM, "BseParam", &finfo, &info);
  bse_type_set_blurb (BSE_TYPE_PARAM, "BSE Parameter base type");
  g_assert (BSE_TYPE_PARAM == g_type_from_name ("BseParam"));
  for (i = 0; i < n_param_types; i++)
    {
      memset (&info, 0, sizeof (info));
      type = bse_type_register_static (BSE_TYPE_PARAM, param_types[i].name, param_types[i].blurb, &info);
      g_assert (param_types[i].type == type);
    }

  /* BSE_TYPE_PROCEDURE
   */
  memset (&finfo, 0, sizeof (finfo));
  finfo.type_flags = G_TYPE_FLAG_CLASSED | G_TYPE_FLAG_DERIVABLE;
  memset (&info, 0, sizeof (info));
  bse_type_register_procedure_info (&info);
  g_type_register_fundamental (BSE_TYPE_PROCEDURE, "BseProcedure", &finfo, &info);
  bse_type_set_blurb (BSE_TYPE_PROCEDURE, "BSE Procedure base type");
  g_assert (BSE_TYPE_PROCEDURE == g_type_from_name ("BseProcedure"));

  /* BSE_TYPE_ENUM
   */
  memset (&finfo, 0, sizeof (finfo));
  finfo.type_flags = G_TYPE_FLAG_CLASSED | G_TYPE_FLAG_DERIVABLE;
  memset (&info, 0, sizeof (info));
  info.class_size = sizeof (BseEnumClass);
  info.class_init = (GClassInitFunc) NULL;
  g_type_register_fundamental (BSE_TYPE_ENUM, "BseEnum", &finfo, &info);
  bse_type_set_blurb (BSE_TYPE_ENUM, "Enumeration base type");
  g_assert (BSE_TYPE_ENUM == g_type_from_name ("BseEnum"));

  /* BSE_TYPE_FLAGS
   */
  memset (&finfo, 0, sizeof (finfo));
  finfo.type_flags = G_TYPE_FLAG_CLASSED | G_TYPE_FLAG_DERIVABLE;
  memset (&info, 0, sizeof (info));
  info.class_size = sizeof (BseFlagsClass);
  info.class_init = (GClassInitFunc) NULL;
  g_type_register_fundamental (BSE_TYPE_FLAGS, "BseFlags", &finfo, &info);
  bse_type_set_blurb (BSE_TYPE_FLAGS, "Flag enumeration base type");
  g_assert (BSE_TYPE_FLAGS == g_type_from_name ("BseFlags"));

  memset (&finfo, 0, sizeof (finfo));
  finfo.type_flags = G_TYPE_FLAG_CLASSED | G_TYPE_FLAG_INSTANTIATABLE | G_TYPE_FLAG_DERIVABLE | G_TYPE_FLAG_DEEP_DERIVABLE;
  memset (&info, 0, sizeof (info));
  bse_type_register_object_info (&info);
  g_type_register_fundamental (BSE_TYPE_OBJECT, "BseObject", &finfo, &info);
  bse_type_set_blurb (BSE_TYPE_OBJECT, "BSE Object Hierarchy base type");
  g_assert (BSE_TYPE_OBJECT == g_type_from_name ("BseObject"));

  /* initialize assistant fundamentals */
  bse_type_register_enums ();
  
  /* initialize builtin types */
  for (i = 0; i < n_builtin_types; i++)
    *(builtin_types[i].type_p) = builtin_types[i].register_type ();
}
