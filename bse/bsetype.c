/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-2002 Tim Janik
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
bse_type_register_static (GType            parent_type,
			  const gchar     *type_name,
			  const gchar     *type_blurb,
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
  
  bse_type_set_blurb (type, type_blurb);
  
  return type;
}

GType  
bse_type_register_dynamic (GType        parent_type,
			   const gchar *type_name,
			   const gchar *type_blurb,
			   GTypePlugin *plugin)
{
  GType type = g_type_register_dynamic (parent_type, type_name, plugin, 0);
  
  bse_type_set_blurb (type, type_blurb);
  
  return type;
}


/* --- SFIDL includes --- */
/* provide common constants */
#include "bseglobals.h"
/* provide IDL type initializers */
#define	sfidl_pspec_Int(group, name, nick, blurb, dflt, min, max, step, hints)	\
  sfi_pspec_set_group (sfi_pspec_int (name, nick, blurb, dflt, min, max, step, hints), group)
#define	sfidl_pspec_Int_default(group, name)	\
  sfi_pspec_set_group (sfi_pspec_int (name, NULL, NULL, 0, G_MININT, G_MAXINT, 256, SFI_PARAM_DEFAULT), group)
#define	sfidl_pspec_Num(group, name, nick, blurb, dflt, min, max, step, hints)	\
  sfi_pspec_set_group (sfi_pspec_num (name, nick, blurb, dflt, min, max, step, hints), group)
#define	sfidl_pspec_Num_default(group, name)	\
  sfi_pspec_set_group (sfi_pspec_num (name, NULL, NULL, 0, SFI_MINNUM, SFI_MAXNUM, 1000, SFI_PARAM_DEFAULT), group)
#define	sfidl_pspec_UInt(group, name, nick, blurb, dflt, hints)	\
  sfi_pspec_set_group (sfi_pspec_int (name, nick, blurb, dflt, 0, G_MAXINT, 1, hints), group)
#define	sfidl_pspec_Real(group, name, nick, blurb, dflt, min, max, step, hints)	\
  sfi_pspec_set_group (sfi_pspec_real (name, nick, blurb, dflt, min, max, step, hints), group)
#define	sfidl_pspec_Real_default(group, name)	\
  sfi_pspec_set_group (sfi_pspec_real (name, NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 10, SFI_PARAM_DEFAULT), group)
#define	sfidl_pspec_Bool(group, name, nick, blurb, dflt, hints)			\
  sfi_pspec_set_group (sfi_pspec_bool (name, nick, blurb, dflt, hints), group)
#define	sfidl_pspec_Bool_default(group, name)	\
  sfi_pspec_set_group (sfi_pspec_bool (name, NULL, NULL, FALSE, SFI_PARAM_DEFAULT), group)
#define	sfidl_pspec_Note(group, name, nick, blurb, dflt, hints)			\
  sfi_pspec_set_group (sfi_pspec_note (name, nick, blurb, dflt, SFI_MIN_NOTE, SFI_MAX_NOTE, FALSE, hints), group)
#define	sfidl_pspec_Octave(group, name, nick, blurb, dflt, hints)			\
  sfi_pspec_set_group (sfi_pspec_int (name, nick, blurb, dflt, BSE_MIN_OCTAVE, BSE_MAX_OCTAVE, 4, hints), group)
#define	sfidl_pspec_Freq(group, name, nick, blurb, dflt, hints)			\
  sfi_pspec_set_group (sfi_pspec_real (name, nick, blurb, dflt, BSE_MIN_OSC_FREQUENCY_f, BSE_MAX_OSC_FREQUENCY_f, 10.0, hints), group)
#define	sfidl_pspec_FineTune(group, name, nick, blurb, hints)			\
  sfi_pspec_set_group (sfi_pspec_int (name, nick, blurb, 0, BSE_MIN_FINE_TUNE, BSE_MAX_FINE_TUNE, 10, hints), group)
#define	sfidl_pspec_String(group, name, nick, blurb, dflt, hints)			\
  sfi_pspec_set_group (sfi_pspec_string (name, nick, blurb, dflt, hints), group)
#define	sfidl_pspec_String_default(group, name)  \
  sfi_pspec_set_group (sfi_pspec_string (name, NULL, NULL, NULL, SFI_PARAM_DEFAULT), group)
#define	sfidl_pspec_Proxy_default(group, name)  \
  sfi_pspec_set_group (sfi_pspec_proxy (name, NULL, NULL, SFI_PARAM_DEFAULT), group)
#define	sfidl_pspec_BoxedSeq(group, name, nick, blurb, hints, element_pspec)		\
  sfi_pspec_set_group (sfi_pspec_seq (name, nick, blurb, element_pspec, hints), group)
#define	sfidl_pspec_BoxedRec(group, name, nick, blurb, hints, fields)			\
  sfi_pspec_set_group (sfi_pspec_rec (name, nick, blurb, fields, hints), group)
#define	sfidl_pspec_BoxedRec_default(group, name, fields)	\
  sfi_pspec_set_group (sfi_pspec_rec (name, NULL, NULL, fields, SFI_PARAM_DEFAULT), group)
#define	sfidl_pspec_BBlock(group, name, nick, blurb, hints)				\
  sfi_pspec_set_group (sfi_pspec_bblock (name, nick, blurb, hints), group)
/* provide IDL constants */
#define	KAMMER_FREQ	BSE_KAMMER_FREQUENCY_f
#define	KAMMER_NOTE	BSE_KAMMER_NOTE
#define	KAMMER_OCTAVE	BSE_KAMMER_OCTAVE
#define	MAXINT		G_MAXINT
#define	MIN_FINE_TUNE	BSE_MIN_FINE_TUNE
#define	MAX_FINE_TUNE	BSE_MAX_FINE_TUNE
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
    /* include type id builtin variable declarations */
#include "bsegentype_array.c"
  };
  const guint n_builtin_types = sizeof (builtin_types) / sizeof (builtin_types[0]);
  static GTypeFundamentalInfo finfo = { 0, };
  guint i;
  
  g_return_if_fail (quark_blurb == 0);
  
  /* type system initialization
   */
  quark_blurb = g_quark_from_static_string ("GType-blurb");
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
  bse_type_set_blurb (BSE_TYPE_PROCEDURE, "BSE Procedure base type");
  g_assert (BSE_TYPE_PROCEDURE == g_type_from_name ("BseProcedure"));
  
  /* initialize builtin types */
  for (i = 0; i < n_builtin_types; i++)
    *(builtin_types[i].type_p) = builtin_types[i].register_type ();
  
  /* initialize SFIDL types */
  _sfidl_types_init ();
}
